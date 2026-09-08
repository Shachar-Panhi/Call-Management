#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

namespace CAM::API {
    using  TCP = boost::asio::ip::tcp;
    namespace HTTP = boost::beast::http;
    
    using Callback = std::function<void(TCP::socket, HTTP::request<HTTP::string_body>)>;
    
    class HttpSession : public std::enable_shared_from_this<HttpSession> {
    public: 
        explicit HttpSession(TCP::socket, Callback callback);
        boost::asio::awaitable<void> start();
    private:
        boost::beast::tcp_stream stream_;
        Callback callback_;
    };

}