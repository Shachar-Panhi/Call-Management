#pragma once

#include "WebsocketSession.hpp"
#include "WebsocketManager.hpp"

#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using Callback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;

    class Coordinator {
    public:
        explicit Coordinator(std::shared_ptr<WebsocketManager> manager);
        Callback process_callback(); 
        WebsocketSession::SessionCallback get_join_callback();
        WebsocketSession::SessionCallback get_leave_callback();
    private:
        std::shared_ptr<WebsocketManager> manager_;    
    };
}