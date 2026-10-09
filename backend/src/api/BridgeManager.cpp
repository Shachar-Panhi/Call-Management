#include "BridgeManager.hpp"

namespace CAM::API  {

    BridgeManager::BridgeManager() = default;

    void BridgeManager::add_bridge(const std::shared_ptr<Bridge>& bridge) {
        bridged_sessions_[bridge->get_session1()] = bridge;
        bridged_sessions_[bridge->get_session2()] = bridge;
    }

    void BridgeManager::remove_bridge(const std::string& session_id) {
        if (bridged_sessions_.contains(session_id)) {
            std::string partner_id = bridged_sessions_[session_id]->get_partner(session_id);
            bridged_sessions_.erase(session_id);
            bridged_sessions_.erase(partner_id);
        }
    }

    bool BridgeManager::contains(const std::string& session_id) const {
        return bridged_sessions_.contains(session_id);
    }

    std::string BridgeManager::get_partner(const std::string& session_id) const {
        if (bridged_sessions_.contains(session_id)) {
            return bridged_sessions_.at(session_id)->get_partner(session_id);
        }
        return "";
    }
}