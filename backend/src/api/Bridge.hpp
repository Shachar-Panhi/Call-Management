#pragma once 

#include "PeerConnection.hpp"

#include <string>
#include <memory>

namespace CAM::API {
    class Bridge : public std::enable_shared_from_this<Bridge> {
    public:
        Bridge(std::weak_ptr<PeerConnection> peer1, std::weak_ptr<PeerConnection> peer2, std::string session1_id, std::string session2_id);

        bool contains(const std::string& session_id) const;
        std::string get_partner(const std::string& session_id) const;
        void setup_routing();
        
        const std::string& get_session1() const;
        const std::string& get_session2() const;

    private:
        std::weak_ptr<PeerConnection> peer1_;
        std::weak_ptr<PeerConnection> peer2_;
        std::string session1_id_;
        std::string session2_id_;
    };
}