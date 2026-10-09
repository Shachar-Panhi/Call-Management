#pragma once

#include "PeerConnection.hpp"

#include <memory>
#include <string>
#include <unordered_map>


namespace CAM::API {
    class PeerConnection;

    class PeerConnectionManager {
    public: 
        PeerConnectionManager();
        void add(const std::shared_ptr<PeerConnection>& peer, const std::string& session_id);
        void remove(const std::string& session_id);
        
        bool contains(const std::string& session_id) const;
        std::shared_ptr<PeerConnection> get_peer(const std::string& session_id) const;
        
    private:    
        std::unordered_map<std::string, std::shared_ptr<PeerConnection>> peers_;
    };
}