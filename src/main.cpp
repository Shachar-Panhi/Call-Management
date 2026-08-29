#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <rtc/rtc.hpp>
#include "api/Listener.hpp"
#include <memory>

int main() {
    boost::asio::io_context io_context{1};

    auto listener = std::make_shared<CAM::Signaling::Listener>(io_context.get_executor());
    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}