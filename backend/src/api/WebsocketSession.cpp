#include "WebsocketSession.hpp"
#include "WebsocketManager.hpp"
#include "types.hpp"
#include <variant>

namespace CAM::API { 
    WebsocketSession::WebsocketSession(TCP::socket socket, SessionCallback on_join, SessionCallback on_leave)
    : ws_(std::move(socket)), on_join_(std::move(on_join)), on_leave_(std::move(on_leave)) {}
    
    void WebsocketSession::set_message_callback(MessageCallback callback) {
        on_message_ = std::move(callback);
    }

    void WebsocketSession::dispatch_message(std::string message) {
        boost::asio::co_spawn(ws_.get_executor(), send_message(std::move(message)), boost::asio::detached);
    }

    boost::asio::awaitable<void> WebsocketSession::start(HTTP::request<HTTP::string_body> req) {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        
        co_await ws_.async_accept(req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        if (errc) {
            spdlog::error("WebSocket accept error: {}", errc.message());
            co_return;
        }

        on_join_(self);

        boost::system::error_code ep_errc;                    
        auto remote_endpoint = ws_.next_layer().socket().remote_endpoint(ep_errc);
        if (!ep_errc) {
            spdlog::info("WebSocket client connected successfully from {}:{}", 
                         remote_endpoint.address().to_string(), 
                         remote_endpoint.port());
        }

        boost::beast::flat_buffer buffer;
        while (ws_.is_open()) {
            buffer.clear();

            co_await ws_.async_read(buffer, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (errc) {
                break;
            }

            std::string message = boost::beast::buffers_to_string(buffer.data());
            
            if (on_message_) {
                on_message_(message);
            }
        }
        
        on_leave_(self);
    }

    boost::asio::awaitable<void> WebsocketSession::send_message(std::string message) {
        boost::system::error_code errc;
        
        co_await ws_.async_write(boost::asio::buffer(message), boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        
        if (errc) {
            spdlog::error("WebSocket write error: {}", errc.message());
        }
    }
}