#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace CAM::API {
    class WebsocketSession;

    class WebsocketManager {
    public: 
        WebsocketManager();
        void add(const std::shared_ptr<WebsocketSession>& session, const std::string& session_id);
        void remove(const std::string& session_id);
        
        bool contains(const std::string& session_id) const;
        std::shared_ptr<WebsocketSession> get_session(const std::string& session_id) const;
        
    private:    
        std::unordered_map<std::string, std::shared_ptr<WebsocketSession>> sessions_;
    };
}