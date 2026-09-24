#include "PeerConnection.hpp"
#include "types.hpp"
#include "../utils/JsonUtils.hpp"

#include <spdlog/spdlog.h>
#include <utility>

namespace CAM::API {
    PeerConnection::PeerConnection(PeerCallback on_join, PeerCallback on_leave, std::string session_id)
    : on_join_(std::move(on_join)), on_leave_(std::move(on_leave)), session_id_(std::move(session_id)) {}

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

        create_data_channel();
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

        SdpOfferPacket packet;
        packet.session_id = session_id_;
        packet.sdp = std::string(description);
        
        auto json_result = CAM::Utils::serialize_json(packet);
        if (!json_result) {
            spdlog::error("Failed to write the signaling packet to json");
            return;
        }

        send_signaling_(std::move(json_result.value()));
    }

    void PeerConnection::handle_candidate(const rtc::Candidate& candidate) {
        if (!send_signaling_) {
            return;
        }

        IceOfferPacket packet;
        packet.session_id = session_id_;
        packet.candidate = std::string(candidate);
        packet.sdpMid = candidate.mid();
        
        auto json_result = CAM::Utils::serialize_json(packet);
        
        if (!json_result) {
            spdlog::error("failed to write candidate to json");
            return;
        }
        
        send_signaling_(std::move(json_result.value()));
    }

    void PeerConnection::create_data_channel() {
        data_channel_ = rtc_connection_->createDataChannel("chat");
        data_channel_->onOpen([]() {
            spdlog::info("datachannel opened successfully");
        });
        
        rtc_connection_->setLocalDescription();
    }

    void PeerConnection::handle_signaling_message(const std::string& message) {
        spdlog::info("peer connection received {}", message);

        auto sdp_result = CAM::Utils::parse_json<SdpOfferPacket>(message);
        if (sdp_result) {
            std::string sdp_type = "answer"; // will always be the answer since the offer is always sent by the client
            rtc_connection_->setRemoteDescription(rtc::Description(sdp_result.value().sdp, sdp_type));
            return;
        }

        auto ice_result = CAM::Utils::parse_json<IceOfferPacket>(message);
        if (ice_result) {
            rtc_connection_->addRemoteCandidate(rtc::Candidate(ice_result.value().candidate, ice_result.value().sdpMid.value_or("")));
            return;
        }

        spdlog::error("json parsing error: message did not match any known struct");
    }
}