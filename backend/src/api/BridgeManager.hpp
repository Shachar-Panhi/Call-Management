#pragma once

#include "Bridge.hpp"
#include "STTHandler.hpp"

#include <unordered_map>
#include <memory>
#include <string>

namespace CAM::API {
    class BridgeManager {
    public:
        explicit BridgeManager(std::shared_ptr<STTHandler> stt_handler);

        void create_bridge(const std::shared_ptr<PeerConnection>& pc1,
                           const std::shared_ptr<PeerConnection>& pc2,
                           const std::string& session1_id,
                           const std::string& session2_id);
        void remove_bridge(const std::string& session_id);

        bool contains(const std::string& session_id) const;
        std::string get_partner(const std::string& session_id) const;

        bool set_ptt(const std::string& session_id, bool active);

    private:
        std::shared_ptr<STTHandler> stt_handler_;
        std::unordered_map<std::string, std::shared_ptr<Bridge>> bridged_sessions_;
    };
}