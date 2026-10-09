#pragma once

#include "Bridge.hpp"

#include <unordered_map>
#include <memory>
#include <string>

namespace CAM::API {
    class BridgeManager {
    public:
        BridgeManager();
        void add_bridge(const std::shared_ptr<Bridge>& bridge);
        void remove_bridge(const std::string& session_id);
        
        bool contains(const std::string& session_id) const;
        std::string get_partner(const std::string& session_id) const;
        
    private:
        std::unordered_map<std::string, std::shared_ptr<Bridge>> bridged_sessions_;
    };
}