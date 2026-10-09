#include "coordinator.hpp"
#include "../utils/Utils.hpp"
#include "../utils/JsonUtils.hpp"

#include <functional>
#include <utility>

namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> ws_manager, std::shared_ptr<PeerConnectionManager> pc_manager, std::shared_ptr<BridgeManager> bridge_manager)
    : ws_manager_(std::move(ws_manager)), pc_manager_(std::move(pc_manager)), bridge_manager_(std::move(bridge_manager)) {}

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

        return [this, on_join, on_leave, on_peer_join, on_peer_leave](TCP::socket socket, HTTP::request<HTTP::string_body> req) {
            auto executor = socket.get_executor();
            
            auto session_id = CAM::Utils::generate_session_id();
            auto peer_connection = std::make_shared<PeerConnection>(on_peer_join, on_peer_leave, session_id);

            auto linked_session_leave = [this, on_leave, session_id](const std::shared_ptr<WebsocketSession>& session) {
                on_leave(session);
                disconnect_session(session_id);

                active_ws_sessions_.erase(session_id);
                active_peer_connections_.erase(session_id);
            };
            
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), on_join, linked_session_leave, session_id);

            active_ws_sessions_[session_id] = websocket_session;
            active_peer_connections_[session_id] = peer_connection;

            peer_connection->set_signaling_callback([weak_ws = std::weak_ptr<WebsocketSession>(websocket_session)](std::string msg) {
                if (auto websocket = weak_ws.lock()) {
                    websocket->dispatch_message(std::move(msg));
                }
            });

            websocket_session->set_message_callback([this, session_id, peer = peer_connection](const std::string& msg) {
                auto api_req = CAM::Utils::parse_json<ApiRequestPacket>(msg);
                if (api_req && api_req.value().action) {
                    handle_api_request(api_req.value(), session_id);
                } else {
                    peer->handle_signaling_message(msg);
                }
            });

            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }

    void Coordinator::disconnect_session(const std::string& session_id) {
        if (active_peer_connections_.contains(session_id)) {
            active_peer_connections_[session_id]->close();
        }

        if (bridge_manager_->contains(session_id)) {
            std::string partner_id = bridge_manager_->get_partner(session_id);
            bridge_manager_->remove_bridge(session_id);

            if (active_peer_connections_.contains(partner_id)) {
                active_peer_connections_[partner_id]->close();
            }

            if (active_ws_sessions_.contains(partner_id)) {
                ApiResponsePacket partner_res;
                partner_res.action = "disconnect";
                partner_res.status = "partner_disconnected";

                auto json_str = CAM::Utils::serialize_json(partner_res);
                if (json_str) {
                    active_ws_sessions_[partner_id]->dispatch_message(json_str.value());
                }
            }
        }
    }

    void Coordinator::handle_api_request(const ApiRequestPacket& req, const std::string& session_id) {
        if (req.action == "connect_to" && req.target_session_id) {
            std::string target_id = req.target_session_id.value();

            if (target_id == session_id) {
                ApiResponsePacket err_res;
                err_res.action = "connect_to";
                err_res.status = "cannot_call_self";
                auto json_str = CAM::Utils::serialize_json(err_res);
                if (json_str && active_ws_sessions_.contains(session_id)) {
                    active_ws_sessions_[session_id]->dispatch_message(json_str.value());
                }
                return;
            }

            if (active_ws_sessions_.contains(target_id) && !bridge_manager_->contains(target_id)) {
                auto pc1 = active_peer_connections_[session_id];
                auto pc2 = active_peer_connections_[target_id];

                std::shared_ptr<Bridge> bridge = std::make_shared<Bridge>(pc1, pc2, session_id, target_id);
                bridge_manager_->add_bridge(bridge);
                bridge->setup_routing();

                ApiResponsePacket match_res;
                match_res.action = "match";
                match_res.status = "matched";
                
                auto json_str = CAM::Utils::serialize_json(match_res);
                if (json_str) {
                    active_ws_sessions_[session_id]->dispatch_message(json_str.value());
                    active_ws_sessions_[target_id]->dispatch_message(json_str.value());
                }

                pc1->initialize_webrtc();
                pc2->initialize_webrtc();
            } else {
                ApiResponsePacket err_res;
                err_res.action = "connect_to";
                err_res.status = "failed";
                auto json_str = CAM::Utils::serialize_json(err_res);
                if (json_str && active_ws_sessions_.contains(session_id)) {
                    active_ws_sessions_[session_id]->dispatch_message(json_str.value());
                }
            }
        } else if (req.action == "disconnect") {
            disconnect_session(session_id);

            ApiResponsePacket response;
            response.action = "disconnect";
            response.status = "disconnected";

            auto json_str = CAM::Utils::serialize_json(response);
            if (json_str) {
                if (active_ws_sessions_.contains(session_id)) {
                    active_ws_sessions_[session_id]->dispatch_message(json_str.value());
                }
            }
        }
    }
}