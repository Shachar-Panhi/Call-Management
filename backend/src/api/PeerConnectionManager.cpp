#include "PeerConnectionManager.hpp"
#include "PeerConnection.hpp"

#include <spdlog/spdlog.h>
#include <vector>

namespace CAM::API {
    PeerConnectionManager::PeerConnectionManager() = default;

    void PeerConnectionManager::join(const std::shared_ptr<PeerConnection>& peer) {
        peers_.push_back(peer);
        spdlog::info("PeerConnection added to manager vector");
    }

    void PeerConnectionManager::leave(const std::shared_ptr<PeerConnection>& peer) {
        std::erase(peers_, peer);
        spdlog::info("PeerConnection removed from manager vector");
    }
}