#include "WebsocketManager.hpp"
#include "WebsocketSession.hpp"
#include <spdlog/spdlog.h>

namespace CAM::API {
    WebsocketManager::WebsocketManager() = default;

    void WebsocketManager::add(const std::shared_ptr<WebsocketSession>& session, const std::string& session_id) {
        sessions_[session_id] = session;
        spdlog::info("WebsocketSession added to manager: {}", session_id);
    }

    void WebsocketManager::remove(const std::string& session_id) {
        sessions_.erase(session_id);
        spdlog::info("WebsocketSession removed from manager: {}", session_id);
    }

    bool WebsocketManager::contains(const std::string& session_id) const {
        return sessions_.contains(session_id);
    }

    std::shared_ptr<WebsocketSession> WebsocketManager::get_session(const std::string& session_id) const {
        if (auto iterator = sessions_.find(session_id); iterator != sessions_.end()) {
            return iterator->second;
        }
        return nullptr;
    }
}