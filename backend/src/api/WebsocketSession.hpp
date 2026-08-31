#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <spdlog/spdlog.h>
#include <rtc/rtc.hpp>
#include <memory>
#include <string>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace Websocket = boost::beast::websocket;
    namespace HTTP = boost::beast::http;

    class WebsocketManager;

    class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
    public: 
        WebsocketSession(TCP::socket, std::shared_ptr<WebsocketManager> manager);
        boost::asio::awaitable<void> start(HTTP::request<HTTP::string_body> req);
        boost::asio::awaitable<void> send_message(const std::string message);

    private:
        Websocket::stream<boost::beast::tcp_stream> ws_;
        std::shared_ptr<WebsocketManager> manager_;
    };
}