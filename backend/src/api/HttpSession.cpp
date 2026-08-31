#include "HttpSession.hpp"

#include <utility>
#include "WebsocketSession.hpp"

namespace CAM::API {

    HttpSession::HttpSession(TCP::socket socket, std::shared_ptr<WebsocketManager> manager)
    : stream_(std::move(socket)), manager_(std::move(manager)) {}

    boost::asio::awaitable<void> HttpSession::start() {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        boost::beast::flat_buffer buffer;
        HTTP::request<HTTP::string_body> req;
        
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
            
            if (Websocket::is_upgrade(req)) {
                auto websocket_session = std::make_shared<WebsocketSession>(stream_.release_socket(), manager_);
                co_spawn(stream_.get_executor(), websocket_session->start(std::move(req)), boost::asio::detached);
                break;
            }

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
            //co_await handle_request(std::move(req));
        }
    }
}