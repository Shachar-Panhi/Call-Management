#pragma once

#include "HttpSession.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/asio/any_io_executor.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using WsCallback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;
    using ApiCallback = std::function<HTTP::response<HTTP::string_body>(HTTP::request<HTTP::string_body>)>;
    
    class Listener {
    public: 
        Listener(const boost::asio::any_io_executor& io_context, 
            WsCallback ws_callback, ApiCallback api_callback);
        boost::asio::awaitable<void> listen();
    private:
        TCP::acceptor acceptor_;
        WsCallback ws_callback_;
        ApiCallback api_callback_;
    };
}