#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using WsCallback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;
    using ApiCallback = std::function<HTTP::response<HTTP::string_body>(HTTP::request<HTTP::string_body>)>;
    
    class HttpSession : public std::enable_shared_from_this<HttpSession> {
    public: 
        explicit HttpSession(TCP::socket socket, WsCallback ws_callback, ApiCallback api_callback);
        boost::asio::awaitable<void> start();
    private:
        boost::beast::tcp_stream stream_;
        WsCallback ws_callback_;
        ApiCallback api_callback_;
    };
}