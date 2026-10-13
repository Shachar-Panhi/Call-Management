#include "Listener.hpp"

#include <utility>

namespace CAM::API {
    constexpr int kPort = 8080;
    constexpr auto kIPAddress = "127.0.0.1";
    
    Listener::Listener(const boost::asio::any_io_executor& io_context, WsCallback ws_callback, ApiCallback api_callback)
    : acceptor_(io_context), ws_callback_(std::move(ws_callback)), api_callback_(std::move(api_callback)) {}
    
    boost::asio::awaitable<void> Listener::listen() {
        auto executor = acceptor_.get_executor();
        boost::system::error_code errc;

        TCP::endpoint endpoint(boost::asio::ip::make_address(kIPAddress, errc), kPort);
        if (errc){
            spdlog::error("invalid ip address {}", errc.message());
            co_return;
        }

        errc = acceptor_.open(endpoint.protocol(), errc);
        if (errc) {
            spdlog::error("failed to start server: {}", errc.message());
            co_return;
        }

        errc = acceptor_.set_option(boost::asio::socket_base::reuse_address(true), errc);
        if (errc) {
            spdlog::error("failed to start server: {}", errc.message());
            co_return;
        }
        
        errc = acceptor_.bind(endpoint, errc);
        if (errc) {
            spdlog::error("failed to start server: {}", errc.message());
            co_return;
        }

        errc = acceptor_.listen(boost::asio::socket_base::max_listen_connections, errc);
        if (errc) {
            spdlog::error("failed to start server: {}", errc.message());
            co_return;
        }
        
        spdlog::info("Signaling server listening on port {}", kPort);
        
        while (true) {
            TCP::socket socket = co_await acceptor_.async_accept(boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            if (!errc) {
                auto http_session = std::make_shared<HttpSession>(std::move(socket), ws_callback_, api_callback_);
                co_spawn(executor, http_session->start(), boost::asio::detached);
            } else {
                spdlog::error("Accept error: {}", errc.message());
            }
        }
    }
}