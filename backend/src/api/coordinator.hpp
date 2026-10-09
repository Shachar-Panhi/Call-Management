#pragma once

#include "WebsocketSession.hpp"
#include "WebsocketManager.hpp"
#include "PeerConnectionManager.hpp"
#include "PeerConnection.hpp"
#include "types.hpp"
#include "BridgeManager.hpp"

#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <queue>
#include <string>
#include <unordered_map>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using Callback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;

    class Coordinator {
    public:
        explicit Coordinator(std::shared_ptr<WebsocketManager> ws_manager, std::shared_ptr<PeerConnectionManager> pc_manager, std::shared_ptr<BridgeManager> bridge_manager);
        Callback process_callback(); 

        WebsocketSession::SessionCallback get_join_callback();
        WebsocketSession::SessionCallback get_leave_callback();

        PeerConnection::PeerCallback get_peer_join_callback();
        PeerConnection::PeerCallback get_peer_leave_callback();  
        
        void handle_api_request(const ApiRequestPacket& req, const std::string& session_id);
        void match_peers();
        void disconnect_session(const std::string& session_id);

    private:
        std::shared_ptr<WebsocketManager> ws_manager_;    
        std::shared_ptr<PeerConnectionManager> pc_manager_;    
        std::shared_ptr<BridgeManager> bridge_manager_;

        std::queue<std::string> matching_queue_;
        std::unordered_map<std::string, std::shared_ptr<WebsocketSession>> active_ws_sessions_;
        std::unordered_map<std::string, std::shared_ptr<PeerConnection>> active_peer_connections_;
    };
}