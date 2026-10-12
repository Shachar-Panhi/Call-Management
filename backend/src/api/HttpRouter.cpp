#include "HttpRouter.hpp"
#include "coordinator.hpp"
#include "../utils/JsonUtils.hpp"
#include "types.hpp"

#include <spdlog/spdlog.h>

namespace CAM::API {

    HttpRouter::HttpRouter(std::shared_ptr<WebsocketManager> ws_manager,
        std::shared_ptr<PeerConnectionManager> pc_manager,
        std::shared_ptr<BridgeManager> bridge_manager,
        Coordinator& coordinator)
    : ws_manager_(std::move(ws_manager)), pc_manager_(std::move(pc_manager)),
        bridge_manager_(std::move(bridge_manager)), coordinator_(coordinator) {}

    HTTP::response<HTTP::string_body> HttpRouter::make_json_response(
        HTTP::status status, const std::string& body, unsigned version, bool keep_alive)
    {
        HTTP::response<HTTP::string_body> res{status, version};
        res.set(HTTP::field::access_control_allow_origin, "*");
        res.set(HTTP::field::content_type, "application/json");
        res.keep_alive(keep_alive);
        res.body() = body;
        res.prepare_payload();
        return res;
    }

    HTTP::response<HTTP::string_body> HttpRouter::route(const HTTP::request<HTTP::string_body>& req) {
        if (req.method() == HTTP::verb::options) {
            return handle_options(req);
        }

        if (req.target() == "/api/call" && req.method() == HTTP::verb::post) {
            return handle_call(req);
        }

        if (req.target() == "/api/end_call" && req.method() == HTTP::verb::post) {
            return handle_end_call(req);
        }

        return make_json_response(HTTP::status::bad_request,
            R"({"status":"unknown_endpoint"})", req.version(), req.keep_alive());
    }

    HTTP::response<HTTP::string_body> HttpRouter::handle_options(const HTTP::request<HTTP::string_body>& req) {
        HTTP::response<HTTP::string_body> res{HTTP::status::no_content, req.version()};
        res.set(HTTP::field::access_control_allow_origin, "*");
        res.set(HTTP::field::access_control_allow_methods, "POST, OPTIONS");
        res.set(HTTP::field::access_control_allow_headers, "Content-Type");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    }

    HTTP::response<HTTP::string_body> HttpRouter::handle_call(const HTTP::request<HTTP::string_body>& req) {
        auto body = CAM::Utils::parse_json<HttpCallRequest>(req.body());
        if (!body) {
            return make_json_response(HTTP::status::bad_request,
                R"({"status":"invalid_body"})", req.version(), req.keep_alive());
        }

        std::string session_id = body.value().session_id;
        std::string target_id = body.value().target_session_id;

        if (target_id == session_id) {
            return make_json_response(HTTP::status::bad_request,
                R"({"status":"cannot_call_self"})", req.version(), req.keep_alive());
        }

        if (!ws_manager_->contains(target_id) || bridge_manager_->contains(target_id)) {
            return make_json_response(HTTP::status::not_found,
                R"({"status":"failed"})", req.version(), req.keep_alive());
        }

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

        return make_json_response(HTTP::status::ok,
            R"({"status":"success"})", req.version(), req.keep_alive());
    }

    HTTP::response<HTTP::string_body> HttpRouter::handle_end_call(const HTTP::request<HTTP::string_body>& req) {
        auto body = CAM::Utils::parse_json<HttpEndCallRequest>(req.body());
        if (!body) {
            return make_json_response(HTTP::status::bad_request,
                R"({"status":"invalid_body"})", req.version(), req.keep_alive());
        }

        std::string session_id = body.value().session_id;
        coordinator_.disconnect_session(session_id);

        ApiResponsePacket response;
        response.action = "disconnect";
        response.status = "disconnected";

        auto json_str = CAM::Utils::serialize_json(response);
        if (json_str && ws_manager_->contains(session_id)) {
            ws_manager_->get_session(session_id)->dispatch_message(json_str.value());
        }

        return make_json_response(HTTP::status::ok,
            R"({"status":"success"})", req.version(), req.keep_alive());
    }
}

