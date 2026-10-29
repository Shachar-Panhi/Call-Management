#include "BridgeManager.hpp"

#include <utility>

namespace CAM::API {

    BridgeManager::BridgeManager(std::shared_ptr<STTHandler> stt_handler)
        : stt_handler_(std::move(stt_handler)) {}

    void BridgeManager::create_bridge(const std::shared_ptr<PeerConnection>& pc1,
                                      const std::shared_ptr<PeerConnection>& pc2,
                                      const std::string& session1_id,
                                      const std::string& session2_id) {
        auto bridge = std::make_shared<Bridge>(pc1, pc2, session1_id, session2_id, stt_handler_);
        bridged_sessions_[session1_id] = bridge;
        bridged_sessions_[session2_id] = bridge;
        bridge->setup_routing();
    }

    void BridgeManager::remove_bridge(const std::string& session_id) {
        auto iterator = bridged_sessions_.find(session_id);
        if (iterator != bridged_sessions_.end()) {
            std::string partner_id = iterator->second->get_partner(session_id);
            bridged_sessions_.erase(iterator);
            bridged_sessions_.erase(partner_id);
        }
    }

    bool BridgeManager::contains(const std::string& session_id) const {
        return bridged_sessions_.contains(session_id);
    }

    std::string BridgeManager::get_partner(const std::string& session_id) const {
        auto iterator = bridged_sessions_.find(session_id);
        if (iterator != bridged_sessions_.end()) {
            return iterator->second->get_partner(session_id);
        }
        return "";
    }

    bool BridgeManager::set_ptt(const std::string& session_id, bool active) {
        auto iterator = bridged_sessions_.find(session_id);
        const bool found = (iterator != bridged_sessions_.end());
        if (found) {
            iterator->second->set_ptt(session_id, active);
        }
        return found;
    }
}