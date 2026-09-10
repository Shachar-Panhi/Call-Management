#include "WebsocketManager.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    WebsocketManager::WebsocketManager() = default;

    void WebsocketManager::join(std::shared_ptr<WebsocketSession> session) {
        sessions_.push_back(session);
        spdlog::info("Session added to manager vector");
    }
    void WebsocketManager::leave(std::shared_ptr<WebsocketSession> session) {
        std::erase(sessions_, session);
        spdlog::info("Session removed from manager vector");
    }
}