#include "api/Listener.hpp"
#include "api/coordinator.hpp"
#include "api/PeerConnectionManager.hpp"
#include "api/STTHandler.hpp"

#include <memory>
#include <spdlog/spdlog.h>

int main() {
    constexpr auto kWhisperModelPath = WHISPER_MODEL_PATH;

    auto stt_result = CAM::API::STTHandler::create(kWhisperModelPath);
    if (!stt_result) {
        spdlog::error("Startup failed: {}", stt_result.error());
        return 1;
    }

    boost::asio::io_context io_context;

    auto ws_manager = std::make_shared<CAM::API::WebsocketManager>();
    auto pc_manager = std::make_shared<CAM::API::PeerConnectionManager>();
    auto bridge_manager = std::make_shared<CAM::API::BridgeManager>(std::move(*stt_result));

    CAM::API::Coordinator coordinator(ws_manager, pc_manager, bridge_manager);
    auto ws_callback = coordinator.process_ws_callback();
    auto api_callback = coordinator.process_http_callback();

    auto listener = std::make_shared<CAM::API::Listener>(
        io_context.get_executor(),
        std::move(ws_callback),
        std::move(api_callback));

    boost::asio::co_spawn(io_context, listener->listen(), boost::asio::detached);

    io_context.run();
    return 0;
}