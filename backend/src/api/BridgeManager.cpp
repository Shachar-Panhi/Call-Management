#include "BridgeManager.hpp"

namespace CAM::API {

    BridgeManager::BridgeManager() = default;

    void BridgeManager::add_bridge(const std::shared_ptr<Bridge>& bridge) {
        bridged_sessions_[bridge->get_session1()] = bridge;
        bridged_sessions_[bridge->get_session2()] = bridge;
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