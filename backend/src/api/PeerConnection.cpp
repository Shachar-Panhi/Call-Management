#include "PeerConnection.hpp"

#include <spdlog/spdlog.h>
#include <utility>

namespace CAM::API {
    PeerConnection::PeerConnection() = default;

    void PeerConnection::set_signaling_callback(SignalingCallback callback) {
        send_signaling_ = std::move(callback);
    }

    void PeerConnection::initialize_webrtc() {
        rtc::Configuration config;

        rtc_connection_ = std::make_shared<rtc::PeerConnection>(config);

        rtc_connection_->onLocalDescription([weak_self = weak_from_this()](rtc::Description description) {
            if (auto self = weak_self.lock()) {
                if (self->send_signaling_) {
                    std::string sdp_message = std::string(description);
                    self->send_signaling_(std::move(sdp_message));
                }
            }
        });

        rtc_connection_->onLocalCandidate([weak_self = weak_from_this()](rtc::Candidate candidate) {
            if (auto self = weak_self.lock()) {
                if (self->send_signaling_) {
                    std::string ice_message = std::string(candidate);
                    self->send_signaling_(std::move(ice_message));
                }
            }
        });
    }

    void PeerConnection::handle_signaling_message(const std::string& message) {
        spdlog::info("peer connection received: {}", message);
        
        if (message == "start") {
            spdlog::info("Generating WebRTC offer");
            auto data_channel = rtc_connection_->createDataChannel("chat");
            rtc_connection_->setLocalDescription();
        }
    }
}