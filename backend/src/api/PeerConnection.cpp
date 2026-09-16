#include "PeerConnection.hpp"
#include "types.hpp"

#include <spdlog/spdlog.h>
#include <glaze/glaze.hpp>
#include <utility>

namespace CAM::API {
    PeerConnection::PeerConnection(PeerCallback on_join, PeerCallback on_leave)
    : on_join_(std::move(on_join)), on_leave_(std::move(on_leave)) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        session_id_ = boost::uuids::to_string(uuid);
    }

    void PeerConnection::send_message(const std::string& message) {
        if (data_channel_ && data_channel_->isOpen()) {
            data_channel_->send(message);
        }
    }

    void PeerConnection::set_signaling_callback(SignalingCallback callback) {
        send_signaling_ = std::move(callback);
    }

    void PeerConnection::initialize_webrtc() {
        rtc::Configuration config;
        rtc_connection_ = std::make_shared<rtc::PeerConnection>(config);

        spdlog::info("peerconnection created with session id: {}", session_id_);

        rtc_connection_->onStateChange([weak_self = weak_from_this()](rtc::PeerConnection::State state) {
            if (auto self = weak_self.lock()) {
                self->handle_state(state);
            }
        });

        rtc_connection_->onLocalDescription([weak_self = weak_from_this()](const rtc::Description& description) {
            if (auto self = weak_self.lock()) {
                self->handle_description(description);
            }
        });

        rtc_connection_->onLocalCandidate([weak_self = weak_from_this()](const rtc::Candidate& candidate) {
            if (auto self = weak_self.lock()) {
                self->handle_candidate(candidate);
            }
        });

        if (on_join_) {
            on_join_(shared_from_this());
        }
    }

    void PeerConnection::handle_state(rtc::PeerConnection::State state) {
        if (state == rtc::PeerConnection::State::Closed || 
            state == rtc::PeerConnection::State::Disconnected || 
            state == rtc::PeerConnection::State::Failed) {
            if (on_leave_) {
                on_leave_(shared_from_this());
            }
        }
    }

    void PeerConnection::handle_description(const rtc::Description& description) {
        if (!send_signaling_) {
            return;
        }

        SignalingPacket packet;
        packet.type = description.typeString();
        packet.sdp = std::string(description);
        
        std::string json_message;
        auto glz_errc = glz::write_json(packet, json_message);

        if (glz_errc) {
            spdlog::error("Failed to write the signaling packet to json");
            return;
        }

        send_signaling_(std::move(json_message));
    }

    void PeerConnection::handle_candidate(const rtc::Candidate& candidate) {
        if (!send_signaling_) {
            return;
        }

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
        
        send_signaling_(std::move(json_message));
    }

    void PeerConnection::handle_signaling_message(const std::string& message) {
        spdlog::info("peer connection received {}", message);
        
        if (message == "start") {
            data_channel_ = rtc_connection_->createDataChannel("chat");
            
            data_channel_->onOpen([]() {
                spdlog::info("datachannel opened successfully");
            });
            
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