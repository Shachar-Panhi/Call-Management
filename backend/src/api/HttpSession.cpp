#include "HttpSession.hpp"

#include <utility>
#include <spdlog/spdlog.h>

namespace CAM::API {
    HttpSession::HttpSession(TCP::socket socket, WsCallback ws_callback, ApiCallback api_callback)
    : stream_(std::move(socket)), ws_callback_(std::move(ws_callback)), api_callback_(std::move(api_callback)) {}

    boost::asio::awaitable<void> HttpSession::start() {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        boost::beast::flat_buffer buffer;

        boost::system::error_code ep_errc;                    
        auto remote_endpoint = stream_.socket().remote_endpoint(ep_errc);
        if (!ep_errc) {
            spdlog::info("client connected successfully from {}:{}", 
                         remote_endpoint.address().to_string(), 
                         remote_endpoint.port());
        } else {
            spdlog::info("client connected successfully (unknown endpoint)");
        }
        
        while (stream_.socket().is_open()) {
            HTTP::request<HTTP::string_body> req;   
            buffer.clear();

            co_await HTTP::async_read(stream_, buffer, req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (errc) {
                if (errc == HTTP::error::end_of_stream || errc == boost::asio::error::connection_reset) {
                    if (!ep_errc) {
                        spdlog::info("client disconnected from {}:{}", 
                                    remote_endpoint.address().to_string(), 
                                    remote_endpoint.port());
                    } else {
                        spdlog::info("client disconnected (unknown endpoint)");
                    }
                } else {
                    spdlog::error("read error: {}", errc.message());
                }
                break;
            }

            if (boost::beast::websocket::is_upgrade(req)) {
                ws_callback_(stream_.release_socket(), std::move(req));
                break; 
            }

            auto res = api_callback_(std::move(req));
            co_await HTTP::async_write(stream_, res, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        
            if (errc || !res.keep_alive()) {
                break;
            }
        }        
    }
}