#include <memory>
#include "api/Listener.hpp"
#include "api/coordinator.hpp"

int main() {
    boost::asio::io_context io_context;

    CAM::API::Coordinator coordinator;
    auto callback = coordinator.process_callback();

    auto listener = std::make_shared<CAM::API::Listener>(io_context.get_executor(), std::move(callback));
    
    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}