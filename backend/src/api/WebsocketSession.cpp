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
        boost::asio::post(ws_.get_executor(), [self = shared_from_this(), msg = std::move(message)]()
        {
            self->queue_message(msg);
        });
    }

    void WebsocketSession::queue_message(std::string message) {
        write_queue_.push(std::move(message));
        
        if (!is_writing_) {
            is_writing_ = true;
            boost::asio::co_spawn(ws_.get_executor(), process_write_queue(), boost::asio::detached);
        }
    }

    boost::asio::awaitable<void> WebsocketSession::process_write_queue() {
        auto self = shared_from_this();
        
        while (!write_queue_.empty()) {
            std::string message = write_queue_.front();
            boost::system::error_code errc;
            
            co_await ws_.async_write(boost::asio::buffer(message), boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (errc) {
                spdlog::error("Websocket write error {}", errc.message());
                break;
            }
            
            write_queue_.pop();
        }
        
        is_writing_ = false;
    }

    boost::asio::awaitable<void> WebsocketSession::start(HTTP::request<HTTP::string_body> req) {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        
        co_await ws_.async_accept(req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        if (errc) {
            spdlog::error("Websocket accept error {}", errc.message());
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
}