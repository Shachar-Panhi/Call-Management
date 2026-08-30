#pragma once

#include <boost/asio.hpp>
#include "boost/asio/any_io_executor.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    using tcp = boost::asio::ip::tcp;
    
    class Listener {
    public: 
        explicit Listener(const boost::asio::any_io_executor&);
        boost::asio::awaitable<void> listen();
    private:
        tcp::acceptor acceptor_;
    };
}