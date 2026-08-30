#include "Listener.hpp"
#include "HttpSession.hpp"

namespace CAM::API {
    constexpr int kPort = 8080;
    constexpr auto kIPAddress = "127.0.0.1";
    
    Listener::Listener(const boost::asio::any_io_executor& io_context)
    : acceptor_(io_context) {}
    
    boost::asio::awaitable<void> Listener::listen() {
        auto executor = acceptor_.get_executor();
        boost::system::error_code errc;

        tcp::endpoint endpoint(boost::asio::ip::make_address(kIPAddress, errc), kPort);
        if (errc){
            spdlog::error("invalid ip address {}", errc.message());
            co_return;
        }

        errc = acceptor_.open(endpoint.protocol(), errc);
        errc = acceptor_.set_option(boost::asio::socket_base::reuse_address(true), errc);
        errc = acceptor_.bind(endpoint, errc);
        errc = acceptor_.listen(boost::asio::socket_base::max_listen_connections, errc);

        if (errc) {
            spdlog::error("failed to start server: {}", errc.message());
            co_return;
        }
        
        spdlog::info("Signaling server listening on port {}", kPort);
        
        while (true) {
            tcp::socket socket = co_await acceptor_.async_accept(boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            if (!errc) {
                auto http_session = std::make_shared<HttpSession>(std::move(socket));
                co_spawn(executor, http_session->start(), boost::asio::detached);
            } else {
                spdlog::error("Accept error: {}", errc.message());
            }
        }
    }
}