#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <spdlog/spdlog.h>

namespace CAM::API {
    using  TCP = boost::asio::ip::tcp;
    
    class HttpSession : public std::enable_shared_from_this<HttpSession> {
    public: 
        explicit HttpSession(TCP::socket);
        boost::asio::awaitable<void> start();
    private:
        boost::beast::tcp_stream stream_;
    };

}