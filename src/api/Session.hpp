#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <spdlog/spdlog.h>
#include <rtc/rtc.hpp>
#include <memory>
#include <string>

namespace CAM::Signaling {
    using tcp = boost::asio::ip::tcp;
    namespace websocket = boost::beast::websocket;

    class Session : public std::enable_shared_from_this<Session> {
    public: 
        explicit Session(tcp::socket socket);
        boost::asio::awaitable<void> start();
        boost::asio::awaitable<void> handle_message(const std::string& message);
        boost::asio::awaitable<void> send_message(const std::string message);

    private:
        websocket::stream<boost::beast::tcp_stream> ws_;
        std::shared_ptr<rtc::PeerConnection> pc_;
    };
}