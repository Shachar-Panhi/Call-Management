#pragma once

#include <string>
#include <functional>
#include <memory>
#include <rtc/rtc.hpp>

namespace CAM::API {
    class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
    public:
        using SignalingCallback = std::function<void(std::string)>;

        PeerConnection();
        void set_signaling_callback(SignalingCallback callback);
        void handle_signaling_message(const std::string& message);
        void initialize_webrtc();

    private:
        std::shared_ptr<rtc::PeerConnection> rtc_connection_;
        SignalingCallback send_signaling_;
    };
}