#include "PeerConnection.hpp"
#include "types.hpp"
#include "../utils/JsonUtils.hpp"
#include "../ThirdParty/RtpCpp/RtpPacket.hpp"

#include <spdlog/spdlog.h>
#include <utility>
#include <span>
#include <string_view>

namespace {
    std::string_view state_to_string(rtc::PeerConnection::State state) {
        switch (state) {
            case rtc::PeerConnection::State::New: return "New";
            case rtc::PeerConnection::State::Connecting: return "Connecting";
            case rtc::PeerConnection::State::Connected: return "Connected";
            case rtc::PeerConnection::State::Disconnected: return "Disconnected";
            case rtc::PeerConnection::State::Failed: return "Failed";
            case rtc::PeerConnection::State::Closed: return "Closed";
            default: return "Unknown";
        }
    }
}

namespace CAM::API {
    PeerConnection::PeerConnection(PeerCallback on_join, PeerCallback on_leave, std::string session_id)
    : on_join_(std::move(on_join)), on_leave_(std::move(on_leave)), session_id_(std::move(session_id)) {}

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

        setup_media_tracks();
        process_packets();
    }

    void PeerConnection::setup_media_tracks() {
        rtc::Description::Audio media("audio", rtc::Description::Direction::SendRecv);
        media.addOpusCodec(kOpusPayloadType);
        
        audio_track_ = rtc_connection_->addTrack(media);

        rtc_connection_->setLocalDescription();
    }

    void PeerConnection::process_packets() {
        audio_track_->onMessage([](rtc::message_variant message) {
            auto* data = std::get_if<rtc::binary>(&message);
            if (!data) {
                return;
            }
            
            std::span<std::uint8_t> buffer(
                reinterpret_cast<std::uint8_t*>(data->data()), 
                data->size()
            );

            RtpCpp::RtpPacketView rtp_packet(buffer);
            
            if (rtp_packet.parse() == RtpCpp::Result::kSuccess) {
                auto header = rtp_packet.get_header();
                spdlog::info("RTP Packet - Seq: {}, TS: {}", header.sequence_number_, header.timestamp_);
            }
        });
    }

    std::string PeerConnection::enforce_16khz(std::string sdp) {
        size_t fmtp_pos = sdp.find("a=fmtp:");
        if (fmtp_pos != std::string::npos) {
            size_t end_of_line = sdp.find("\r\n", fmtp_pos);
            
            if (end_of_line != std::string::npos) {
                sdp.insert(end_of_line, ";maxplaybackrate=16000;sprop-maxcapturerate=16000");
            }
        }
        
        return sdp;
    }

    void PeerConnection::handle_state(rtc::PeerConnection::State state) {
        spdlog::info("[{}] Current WebRTC State: {}", session_id_, state_to_string(state));

        if (state == rtc::PeerConnection::State::Closed || 
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
        
        std::string modified_sdp = enforce_16khz(std::string(description));
        packet.sdp = modified_sdp;
        
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

    std::expected<void, std::string> PeerConnection::set_remote_description(const std::string& sdp) {
        try {
            std::string sdp_type = "answer"; // the client is the one sending the offer so the server will send the answer
            rtc_connection_->setRemoteDescription(rtc::Description(sdp, sdp_type));
            return {};
        } catch (std::exception& error) {
            return std::unexpected(error.what());
        }
    }

    void PeerConnection::handle_signaling_message(const std::string& message) {

        auto sdp_result = CAM::Utils::parse_json<SdpOfferPacket>(message);
        if (sdp_result) {
            spdlog::info("[{}] Received WebRTC SDP Answer", session_id_);
            
            auto result = set_remote_description(sdp_result.value().sdp);
            if (!result.has_value()) {
                spdlog::error("[{}] Failed to apply remote description: {}", session_id_, result.error());
            }
            return;
        }

        auto ice_result = CAM::Utils::parse_json<IceOfferPacket>(message);
        if (ice_result) {
            spdlog::info("[{}] Received ICE Candidate", session_id_);
            rtc_connection_->addRemoteCandidate(rtc::Candidate(ice_result.value().candidate, ice_result.value().sdpMid.value_or("")));
            return;
        }

        spdlog::error("json parsing error: message did not match any known struct");
    }

    void PeerConnection::close() {
        if (rtc_connection_) {
            rtc_connection_->close();
        }
    }
}