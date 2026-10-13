#include "api/Listener.hpp"
#include "api/coordinator.hpp"
#include "api/PeerConnectionManager.hpp"

#include <memory>

int main() {
    boost::asio::io_context io_context;

    std::shared_ptr<CAM::API::WebsocketManager> ws_manager = std::make_shared<CAM::API::WebsocketManager>();
    std::shared_ptr<CAM::API::PeerConnectionManager> pc_manager = std::make_shared<CAM::API::PeerConnectionManager>();
    std::shared_ptr<CAM::API::BridgeManager> bridge_manager = std::make_shared<CAM::API::BridgeManager>();

    CAM::API::Coordinator coordinator(ws_manager, pc_manager, bridge_manager);
    auto ws_callback = coordinator.process_ws_callback();
    auto api_callback = coordinator.process_http_callback();

    auto listener = std::make_shared<CAM::API::Listener>(io_context.get_executor(), std::move(ws_callback), std::move(api_callback));    
    
    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}