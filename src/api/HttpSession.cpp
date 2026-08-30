#include "HttpSession.hpp"
#include "WebsocketSession.hpp"

namespace CAM::API {

    HttpSession::HttpSession(TCP::socket socket)
    : stream_(std::move(socket)) {}

    boost::asio::awaitable<void> HttpSession::start() {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        boost::beast::flat_buffer buffer;
        HTTP::request<HTTP::string_body> req;
        
        spdlog::info("client connected successfully");

        while (stream_.socket().is_open()) {
            HTTP::request<HTTP::string_body> req;   
            buffer.clear();

            co_await HTTP::async_read(stream_, buffer, req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (Websocket::is_upgrade(req)) {
                auto websocket_session = std::make_shared<WebsocketSession>(stream_.release_socket());
                co_spawn(stream_.get_executor(), websocket_session->start(std::move(req)), boost::asio::detached);
                break;
            }

            if (errc) {
                if (errc == HTTP::error::end_of_stream || errc == boost::asio::error::connection_reset) {
                    spdlog::info("client disconnected");
                } else {
                    spdlog::error("read error: {}", errc.message());
                }
                break;
            }
            //co_await handle_request(std::move(req));
        }
    }
}