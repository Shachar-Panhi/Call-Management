#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include <functional>
#include <queue>

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
        void set_message_callback(MessageCallback callback);
        void dispatch_message(std::string message);

        void queue_message(std::string message);
        boost::asio::awaitable<void> process_write_queue();

    private:
        Websocket::stream<boost::beast::tcp_stream> ws_;
        SessionCallback on_join_;
        SessionCallback on_leave_;
        MessageCallback on_message_;
        
        std::queue<std::string> write_queue_;
        bool is_writing_ = false;
    };
}