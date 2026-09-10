#include  "coordinator.hpp"
#include "WebsocketSession.hpp"
#include "PeerConnection.hpp"

#include <functional>
#include <utility>

namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> manager) : manager_(std::move(manager)) {}

    WebsocketSession::SessionCallback Coordinator::get_join_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(manager_)](std::shared_ptr<WebsocketSession> session) {
            if (auto locked = weak_manager.lock()) {
                locked->join(std::move(session));
            }
        };
    }

    WebsocketSession::SessionCallback Coordinator::get_leave_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(manager_)](std::shared_ptr<WebsocketSession> session) {
            if (auto locked = weak_manager.lock()) {
                locked->leave(std::move(session));
            }
        };
    }

    Callback Coordinator::process_callback() {
        auto on_join = get_join_callback();
        auto on_leave = get_leave_callback();

        return [on_join, on_leave](TCP::socket socket, HTTP::request<HTTP::string_body> req) {
            auto executor = socket.get_executor();
            
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), on_join, on_leave);
            auto peer_connection = std::make_shared<PeerConnection>();

            peer_connection->set_signaling_callback([weak_ws = std::weak_ptr<WebsocketSession>(websocket_session)](std::string msg) {
                if (auto websocket = weak_ws.lock()) {
                    websocket->dispatch_message(std::move(msg));
                }
            });

            websocket_session->set_message_callback([peer = peer_connection](std::string msg) {
                peer->handle_signaling_message(msg);
            });

            peer_connection->initialize_webrtc();

            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }
}