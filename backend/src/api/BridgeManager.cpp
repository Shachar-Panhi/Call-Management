#include "BridgeManager.hpp"

#include <algorithm>
#include <vector>

namespace CAM::API  {

    BridgeManager::BridgeManager() = default;

    void BridgeManager::add_bridge(const std::shared_ptr<Bridge>& bridge) {
        bridged_sessions_.push_back(bridge);
    }

    void BridgeManager::remove_bridge(const std::string& session_id) {
        std::erase_if(bridged_sessions_, [&session_id](const std::shared_ptr<Bridge>& bridge) {
            return bridge->contains(session_id);
        });
    }

    bool BridgeManager::contains(const std::string& session_id) {
        return std::ranges::any_of(bridged_sessions_, [&session_id](const std::shared_ptr<Bridge>& bridge) {
            return bridge->contains(session_id);
        });
    }

    std::string BridgeManager::get_partner(const std::string& session_id) {
        auto iterator = std::ranges::find_if(bridged_sessions_, [&session_id](const std::shared_ptr<Bridge>& bridge) {
            return bridge->contains(session_id);
        });

        if (iterator != bridged_sessions_.end()) {
            return (*iterator)->get_partner(session_id);
        }
        return "";
    }
}