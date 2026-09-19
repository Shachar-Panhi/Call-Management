#pragma once

#include <memory>
#include <vector>

namespace CAM::API {
    class WebsocketSession;

    class WebsocketManager {
    public: 
        explicit WebsocketManager();
        void join(const std::shared_ptr<WebsocketSession>& session);
        void leave(const std::shared_ptr<WebsocketSession>& session);
        
    private:    
        std::vector<std::shared_ptr<WebsocketSession>> sessions_;
    };
} 