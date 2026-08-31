#include "WebsocketSession.hpp"
#include "WebsocketManager.hpp"
#include "types.hpp"
#include <variant>

namespace CAM::API { 
    WebsocketSession::WebsocketSession(TCP::socket socket, std::shared_ptr<WebsocketManager> manager)
    : ws_(std::move(socket)), manager_(std::move(manager)) {}
    
    boost::asio::awaitable<void> WebsocketSession::start(HTTP::request<HTTP::string_body> req) {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        
        co_await ws_.async_accept(req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        if (errc) {
            spdlog::error("WebSocket accept error: {}", errc.message());
            co_return;
        }

        manager_->join(self);

        boost::system::error_code ep_errc;                    
        auto remote_endpoint = ws_.next_layer().socket().remote_endpoint(ep_errc);
        if (!ep_errc) {
            spdlog::info("WebSocket client connected successfully from {}:{}", 
                         remote_endpoint.address().to_string(), 
                         remote_endpoint.port());
        } else {
            spdlog::info("WebSocket client connected successfully (unknown endpoint)");
        }

        boost::beast::flat_buffer buffer;
        while (ws_.is_open()) {
            buffer.clear();

            co_await ws_.async_read(buffer, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (errc) {
                if (errc == Websocket::error::closed || errc == boost::asio::error::eof) {
                    if (!ep_errc) {
                        spdlog::info("WebSocket client disconnected from {}:{}", 
                                    remote_endpoint.address().to_string(), 
                                    remote_endpoint.port());
                    } else {
                        spdlog::info("WebSocket client connected successfully (unknown endpoint)");
                    }
                } else {
                    spdlog::error("WebSocket read error: {}", errc.message());
                }
                break;
            }

            std::string message = boost::beast::buffers_to_string(buffer.data());
            co_await send_message(message);
        }
        manager_->leave(self);
    }

    boost::asio::awaitable<void> WebsocketSession::send_message(std::string message) {
        boost::system::error_code errc;
        
        co_await ws_.async_write(boost::asio::buffer(message), boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        
        if (errc) {
            spdlog::error("WebSocket write error: {}", errc.message());
        }
    }
}