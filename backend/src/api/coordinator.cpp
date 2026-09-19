#include  "coordinator.hpp"
#include "../utils/Utils.hpp"


#include <functional>
#include <utility>

namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> ws_manager, std::shared_ptr<PeerConnectionManager> pc_manager) 
    : ws_manager_(std::move(ws_manager)), pc_manager_(std::move(pc_manager)) {}

    WebsocketSession::SessionCallback Coordinator::get_join_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(ws_manager_)](const std::shared_ptr<WebsocketSession>& session) {
            if (auto locked = weak_manager.lock()) {
                locked->join(session);
            }
        };
    }

    WebsocketSession::SessionCallback Coordinator::get_leave_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(ws_manager_)](const std::shared_ptr<WebsocketSession>& session) {
            if (auto locked = weak_manager.lock()) {
                locked->leave(session);
            }
        };
    }

    PeerConnection::PeerCallback Coordinator::get_peer_join_callback() {
        return [weak_manager = std::weak_ptr<PeerConnectionManager>(pc_manager_)](const std::shared_ptr<PeerConnection>& session) {
            if (auto locked = weak_manager.lock()) {
                locked->join(session);
            }
        };
    }

    PeerConnection::PeerCallback Coordinator::get_peer_leave_callback() {
        return [weak_manager = std::weak_ptr<PeerConnectionManager>(pc_manager_)](const std::shared_ptr<PeerConnection>& session) {
            if (auto locked = weak_manager.lock()) {
                locked->leave(session);
            }
        };
    }

    Callback Coordinator::process_callback() {
        auto on_join = get_join_callback();
        auto on_leave = get_leave_callback();
        auto on_peer_join = get_peer_join_callback();
        auto on_peer_leave = get_peer_leave_callback();

        return [on_join, on_leave, on_peer_join, on_peer_leave](TCP::socket socket, HTTP::request<HTTP::string_body> req) {
            auto executor = socket.get_executor();
            
            auto ws_id = CAM::Utils::generate_session_id();
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), on_join, on_leave, ws_id);
            
            auto pc_id = CAM::Utils::generate_session_id();
            auto peer_connection = std::make_shared<PeerConnection>(on_peer_join, on_peer_leave, pc_id);

            peer_connection->set_signaling_callback([weak_ws = std::weak_ptr<WebsocketSession>(websocket_session)](std::string msg) {
                if (auto websocket = weak_ws.lock()) {
                    websocket->dispatch_message(std::move(msg));
                }
            });

            websocket_session->set_message_callback([peer = peer_connection](const std::string& msg) {
                peer->handle_signaling_message(msg);
            });

            peer_connection->initialize_webrtc();

            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }
}