#pragma once

#include <boost/asio.hpp>
#include "WebsocketManager.hpp"
#include "boost/asio/any_io_executor.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    using tcp = boost::asio::ip::tcp;
    
    class Listener {
    public: 
        Listener(const boost::asio::any_io_executor&, std::shared_ptr<WebsocketManager> manager);
        boost::asio::awaitable<void> listen();
    private:
        tcp::acceptor acceptor_;
        std::shared_ptr<WebsocketManager> manager_;

    };
}