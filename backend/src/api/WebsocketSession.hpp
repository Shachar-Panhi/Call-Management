#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <spdlog/spdlog.h>
#include <rtc/rtc.hpp>
#include <memory>
#include <string>
#include <functional>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace Websocket = boost::beast::websocket;
    namespace HTTP = boost::beast::http;

    class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
    public: 
        using SessionCallback = std::function<void(std::shared_ptr<WebsocketSession>)>;
        using MessageCallback = std::function<void(std::string)>;
        
        explicit WebsocketSession(TCP::socket socket, SessionCallback on_join, SessionCallback on_leave);
        boost::asio::awaitable<void> start(HTTP::request<HTTP::string_body> req);
        boost::asio::awaitable<void> send_message(std::string message);
        
        void set_message_callback(MessageCallback callback);
        void dispatch_message(std::string message);

    private:
        Websocket::stream<boost::beast::tcp_stream> ws_;
        SessionCallback on_join_;
        SessionCallback on_leave_;
        MessageCallback on_message_;
    };
}