#include "api/Listener.hpp"
#include "api/coordinator.hpp"
#include "api/PeerConnectionManager.hpp"

#include <memory>

int main() {
    boost::asio::io_context io_context;

    std::shared_ptr<CAM::API::WebsocketManager> ws_manager = std::make_shared<CAM::API::WebsocketManager>();
    std::shared_ptr<CAM::API::PeerConnectionManager> pc_manager = std::make_shared<CAM::API::PeerConnectionManager>();

    CAM::API::Coordinator coordinator(ws_manager, pc_manager);
    auto callback = coordinator.process_callback();

    auto listener = std::make_shared<CAM::API::Listener>(io_context.get_executor(), std::move(callback));
    
    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}