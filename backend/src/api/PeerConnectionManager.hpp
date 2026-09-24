#pragma once

#include <memory>
#include <vector>

namespace CAM::API {
    class PeerConnection;

    class PeerConnectionManager {
    public: 
        explicit PeerConnectionManager();
        void join(const std::shared_ptr<PeerConnection>& peer);
        void leave(const std::shared_ptr<PeerConnection>& peer);
        
    private:    
        std::vector<std::shared_ptr<PeerConnection>> peers_;
    };
}