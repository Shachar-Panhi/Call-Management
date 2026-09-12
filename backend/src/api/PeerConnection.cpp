#include "PeerConnection.hpp"
#include "types.hpp"

#include <spdlog/spdlog.h>
#include <glaze/glaze.hpp>
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
                    SignalingPacket packet;
                    packet.type = description.typeString();
                    packet.sdp = std::string(description);
                    
                    std::string json_message;
                    auto glz_errc = glz::write_json(packet, json_message);

                    if (glz_errc) {
                        spdlog::error("Failed to write the signaling packet to json");
                        return;
                    }

                    self->send_signaling_(std::move(json_message));
                }
            }
        });

        rtc_connection_->onLocalCandidate([weak_self = weak_from_this()](rtc::Candidate candidate) {
            if (auto self = weak_self.lock()) {
                if (self->send_signaling_) {
                    SignalingPacket packet;
                    packet.type = "candidate";
                    packet.candidate = std::string(candidate);
                    packet.sdpMid = candidate.mid();
                    
                    std::string json_message;
                    auto glz_errc = glz::write_json(packet, json_message);
                    
                    if (glz_errc) {
                        spdlog::error("Failed to write candidate to json");
                        return;
                    }
                    
                    self->send_signaling_(std::move(json_message));
                }
            }
        });
    }

    void PeerConnection::handle_signaling_message(const std::string& message) {
        spdlog::info("peer connection received {}", message);
        
       if (message == "start") {
            auto data_channel = rtc_connection_->createDataChannel("chat");
            rtc_connection_->setLocalDescription();
            return;
        }

        SignalingPacket packet;
        auto glz_errc = glz::read_json(packet, message);
        
        if (glz_errc) {
            spdlog::error("json parsing error");
            return;
        }

        if (packet.type == "answer" && packet.sdp.has_value()) {
            rtc_connection_->setRemoteDescription(rtc::Description(packet.sdp.value(), packet.type));
        } else if (packet.type == "candidate" && packet.candidate.has_value() && packet.sdpMid.has_value()) {
            rtc_connection_->addRemoteCandidate(rtc::Candidate(packet.candidate.value(), packet.sdpMid.value()));
        }
    }
}