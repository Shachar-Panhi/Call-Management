#include "WebsocketManager.hpp"

namespace CAM::API {
    WebsocketManager::WebsocketManager() = default;

    void WebsocketManager::join(std::shared_ptr<WebsocketSession> session) {
        sessions_.push_back(session);
    }
    void WebsocketManager::leave(std::shared_ptr<WebsocketSession> session) {
        std::erase(sessions_, session);
    }
}