#include "PeerConnectionManager.hpp"

#include <spdlog/spdlog.h>

namespace CAM::API {
    PeerConnectionManager::PeerConnectionManager() = default;

    void PeerConnectionManager::add(const std::shared_ptr<PeerConnection>& peer, const std::string& session_id) {
        peers_[session_id] = peer;
        spdlog::info("PeerConnection added to manager: {}", session_id);
    }

    void PeerConnectionManager::remove(const std::string& session_id) {
        peers_.erase(session_id);
        spdlog::info("PeerConnection removed from manager: {}", session_id);
    }

    bool PeerConnectionManager::contains(const std::string& session_id) const {
        return peers_.contains(session_id);
    }

    std::shared_ptr<PeerConnection> PeerConnectionManager::get_peer(const std::string& session_id) const {
        if (auto iterator = peers_.find(session_id); iterator != peers_.end()) {
            return iterator->second;
        }
        return nullptr;
    }
}