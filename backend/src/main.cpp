#include "api/Listener.hpp"
#include "api/WebsocketManager.hpp"

int main() {
    boost::asio::io_context io_context;

    std::shared_ptr<CAM::API::WebsocketManager> manager = std::make_shared<CAM::API::WebsocketManager>();
    auto listener = std::make_shared<CAM::API::Listener>(io_context.get_executor(), std::move(manager));
    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}