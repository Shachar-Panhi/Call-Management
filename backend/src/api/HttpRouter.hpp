#pragma once

#include "WebsocketManager.hpp"
#include "PeerConnectionManager.hpp"
#include "BridgeManager.hpp"

#include <memory>
#include <boost/beast.hpp>
#include <string>

namespace CAM::API {
    namespace HTTP = boost::beast::http;

    class Coordinator;

    class HttpRouter {
    public:
        explicit HttpRouter(std::shared_ptr<WebsocketManager> ws_manager,
            std::shared_ptr<PeerConnectionManager> pc_manager,
            std::shared_ptr<BridgeManager> bridge_manager,
            Coordinator& coordinator);

        HTTP::response<HTTP::string_body> route(const HTTP::request<HTTP::string_body>& req);

    private:
        static HTTP::response<HTTP::string_body> handle_options(const HTTP::request<HTTP::string_body>& req);
        HTTP::response<HTTP::string_body> handle_call(const HTTP::request<HTTP::string_body>& req);
        HTTP::response<HTTP::string_body> handle_end_call(const HTTP::request<HTTP::string_body>& req);
        HTTP::response<HTTP::string_body> handle_ptt(const HTTP::request<HTTP::string_body>& req);

        static HTTP::response<HTTP::string_body> make_json_response(HTTP::status status,
            const std::string& body, unsigned version, bool keep_alive);

        std::shared_ptr<WebsocketManager> ws_manager_;
        std::shared_ptr<PeerConnectionManager> pc_manager_;
        std::shared_ptr<BridgeManager> bridge_manager_;
        Coordinator* coordinator_ = nullptr;
    };
}