#include "coordinator.hpp"
#include "../utils/Utils.hpp"
#include "../utils/JsonUtils.hpp"
#include "types.hpp"

#include <utility>

namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> ws_manager,
        std::shared_ptr<PeerConnectionManager> pc_manager, std::shared_ptr<BridgeManager> bridge_manager)
    : ws_manager_(std::move(ws_manager)), pc_manager_(std::move(pc_manager)),
        bridge_manager_(std::move(bridge_manager)) {}

    WsCallback Coordinator::process_ws_callback() {
        return [this](TCP::socket socket, HTTP::request<HTTP::string_body> req) {
            auto executor = socket.get_executor();
            
            auto session_id = CAM::Utils::generate_session_id();
            
            auto on_peer_join = [](const std::shared_ptr<PeerConnection>&) {};
            auto on_peer_leave = [this, session_id](const std::shared_ptr<PeerConnection>&) {
                disconnect_session(session_id);
            };

            auto peer_connection = std::make_shared<PeerConnection>(on_peer_join, on_peer_leave, session_id);

            auto on_ws_join = [](const std::shared_ptr<WebsocketSession>&) {};
            auto on_ws_leave = [this, session_id](const std::shared_ptr<WebsocketSession>&) {
                disconnect_session(session_id);
                ws_manager_->remove(session_id);
                pc_manager_->remove(session_id);
            };
            
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), on_ws_join, on_ws_leave, session_id);

            ws_manager_->add(websocket_session, session_id);
            pc_manager_->add(peer_connection, session_id);

            peer_connection->set_signaling_callback([weak_ws = std::weak_ptr<WebsocketSession>(websocket_session)](std::string msg) {
                if (auto websocket = weak_ws.lock()) {
                    websocket->dispatch_message(std::move(msg));
                }
            });

            websocket_session->set_message_callback([peer = peer_connection](const std::string& msg) {
                peer->handle_signaling_message(msg);
            });

            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }

    ApiCallback Coordinator::process_http_callback() {
        return [this](HTTP::request<HTTP::string_body> req) -> HTTP::response<HTTP::string_body> {
           
            if (req.method() == HTTP::verb::options) {
                HTTP::response<HTTP::string_body> res{HTTP::status::no_content, req.version()};
                res.set(HTTP::field::access_control_allow_origin, "*");
                res.set(HTTP::field::access_control_allow_methods, "POST, OPTIONS");
                res.set(HTTP::field::access_control_allow_headers, "Content-Type");
                res.keep_alive(req.keep_alive());
                return res;
            }

            HTTP::response<HTTP::string_body> res{HTTP::status::bad_request, req.version()};
            res.set(HTTP::field::access_control_allow_origin, "*");
            res.set(HTTP::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());

            if (req.target() == "/api/call" && req.method() == HTTP::verb::post) {
                auto body = CAM::Utils::parse_json<HttpCallRequest>(req.body());
                if (body) {
                    std::string session_id = body.value().session_id;
                    std::string target_id = body.value().target_session_id;

                    if (target_id == session_id) {
                        res.body() = R"({"status":"cannot_call_self"})";
                    } else if (ws_manager_->contains(target_id) && !bridge_manager_->contains(target_id)) {
                        spdlog::info("Call started between {} and {}", session_id, target_id);
                        
                        auto pc1 = pc_manager_->get_peer(session_id);
                        auto pc2 = pc_manager_->get_peer(target_id);

                        std::shared_ptr<Bridge> bridge = std::make_shared<Bridge>(pc1, pc2, session_id, target_id);
                        bridge_manager_->add_bridge(bridge);
                        bridge->setup_routing();

                        ApiResponsePacket match_res;
                        match_res.action = "match";
                        match_res.status = "matched";
                        
                        auto json_str = CAM::Utils::serialize_json(match_res);
                        if (json_str) {
                            ws_manager_->get_session(session_id)->dispatch_message(json_str.value());
                            ws_manager_->get_session(target_id)->dispatch_message(json_str.value());
                        }

                        pc1->initialize_webrtc();
                        pc2->initialize_webrtc();

                        res.result(HTTP::status::ok);
                        res.body() = R"({"status":"success"})";
                    } else {
                        res.result(HTTP::status::not_found);
                        res.body() = R"({"status":"failed"})";
                    }
                }
            } else if (req.target() == "/api/end_call" && req.method() == HTTP::verb::post) {
                auto body = CAM::Utils::parse_json<HttpEndCallRequest>(req.body());
                if (body) {
                    std::string session_id = body.value().session_id;
                    disconnect_session(session_id);

                    ApiResponsePacket response;
                    response.action = "disconnect";
                    response.status = "disconnected";

                    auto json_str = CAM::Utils::serialize_json(response);
                    if (json_str && ws_manager_->contains(session_id)) {
                        ws_manager_->get_session(session_id)->dispatch_message(json_str.value());
                    }
                    
                    res.result(HTTP::status::ok);
                    res.body() = R"({"status":"success"})";
                }
            }

            res.prepare_payload();
            return res;
        };
    }

    void Coordinator::disconnect_session(const std::string& session_id) {
        if (pc_manager_->contains(session_id)) {
            pc_manager_->get_peer(session_id)->close();
        }

        if (bridge_manager_->contains(session_id)) {
            std::string partner_id = bridge_manager_->get_partner(session_id);
            bridge_manager_->remove_bridge(session_id);

            if (pc_manager_->contains(partner_id)) {
                pc_manager_->get_peer(partner_id)->close();
            }

            spdlog::info("Call ended between {} and {}", session_id, partner_id);

            if (ws_manager_->contains(partner_id)) {
                ApiResponsePacket partner_res;
                partner_res.action = "disconnect";
                partner_res.status = "partner_disconnected";

                auto json_str = CAM::Utils::serialize_json(partner_res);
                if (json_str) {
                    ws_manager_->get_session(partner_id)->dispatch_message(json_str.value());
                }
            }
        }
    }
}