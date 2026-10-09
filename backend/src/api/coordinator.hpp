#pragma once

#include "WebsocketSession.hpp"
#include "WebsocketManager.hpp"
#include "PeerConnectionManager.hpp"
#include "types.hpp"
#include "BridgeManager.hpp"

#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using Callback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;

    class Coordinator {
    public:
        explicit Coordinator(std::shared_ptr<WebsocketManager> ws_manager, std::shared_ptr<PeerConnectionManager> pc_manager, std::shared_ptr<BridgeManager> bridge_manager);
        Callback process_callback(); 
        
        void handle_api_request(const ApiRequestPacket& req, const std::string& session_id);
        void disconnect_session(const std::string& session_id);

    private:
        std::shared_ptr<WebsocketManager> ws_manager_;    
        std::shared_ptr<PeerConnectionManager> pc_manager_;    
        std::shared_ptr<BridgeManager> bridge_manager_;
    };
}