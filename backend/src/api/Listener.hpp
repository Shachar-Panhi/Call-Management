#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/asio/any_io_executor.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    using TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using Callback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;
    
    class Listener {
    public: 
        Listener(const boost::asio::any_io_executor&, Callback callback);
        boost::asio::awaitable<void> listen();
    private:
        TCP::acceptor acceptor_;
        Callback callback_;
    };
}