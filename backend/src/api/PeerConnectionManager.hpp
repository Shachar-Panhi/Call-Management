#pragma once

#include <memory>
#include <vector>

namespace CAM::API {
    class PeerConnection;

    class PeerConnectionManager {
    public: 
        explicit PeerConnectionManager();
        void join(std::shared_ptr<PeerConnection> peer);
        void leave(std::shared_ptr<PeerConnection> peer);
    private:    
        std::vector<std::shared_ptr<PeerConnection>> peers_;
    };
}