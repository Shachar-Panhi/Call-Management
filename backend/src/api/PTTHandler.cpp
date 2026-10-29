#include "PTTHandler.hpp"
#include "../ThirdParty/RtpCpp/RtpPacket.hpp"
#include "PeerConnection.hpp"

#include <cstring>
#include <spdlog/spdlog.h>
#include <utility>

namespace CAM::API {

    PTTHandler::PTTHandler(std::string session_id, OpusTranscoder transcoder, std::shared_ptr<STTHandler> stt_handler) noexcept
        : session_id_(std::move(session_id)),
          transcoder_(std::move(transcoder)),
          stt_handler_(std::move(stt_handler)) {}

    std::expected<std::unique_ptr<PTTHandler>, std::string>
    PTTHandler::create(std::string session_id, std::shared_ptr<STTHandler> stt_handler) {
        auto transcoder = OpusTranscoder::create();
        if (!transcoder) {
            return std::unexpected("PTTHandler: failed to create OpusTranscoder - " + transcoder.error());
        }
        return std::unique_ptr<PTTHandler>(
            new PTTHandler(std::move(session_id), std::move(transcoder.value()), std::move(stt_handler)));
    }

    void PTTHandler::start() {
        std::lock_guard<std::mutex> lock(mutex_);
        pcm_buffer_.clear();
        active_.store(true, std::memory_order_release);
        spdlog::info("[{}] PTT started", session_id_);
    }

    bool PTTHandler::process_packet(const rtc::binary& packet) {
        if (!active_.load(std::memory_order_acquire)) {
            return false;
        }

        if (packet.empty() || packet.size() > RtpCpp::kMaxRtpPacketSize) {
            spdlog::warn("[{}] PTT: Invalid RTP packet size ({})", session_id_, packet.size());
            return true;
        }

        std::array<uint8_t, RtpCpp::kMaxRtpPacketSize> buffer{};
        std::memcpy(buffer.data(), packet.data(), packet.size());

        RtpCpp::StaticRtpPacket rtp(buffer);
        if (rtp.parse(packet.size()) != RtpCpp::Result::kSuccess) {
            spdlog::warn("[{}] PTT: Failed to parse RTP packet", session_id_);
            return true;
        }

        const auto& header = rtp.get_header();
        if (header.payload_type_ != PeerConnection::kOpusPayloadType) {
            return false;
        }

        auto payload = rtp.payload();
        if (payload.empty()) {
            spdlog::warn("[{}] PTT: Empty RTP payload", session_id_);
            return true;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_.load(std::memory_order_relaxed)) {
            return false;
        }

        auto decoded_res = transcoder_.decode(payload);
        if (!decoded_res) {
            if (decoded_res.error() == "BUFFERING") {
                return true;
            }
            spdlog::warn("[{}] PTT: Failed to decode Opus payload: {}", session_id_, decoded_res.error());
            return true;
        }

        pcm_buffer_.insert(pcm_buffer_.end(), decoded_res->begin(), decoded_res->end());

        return true;
    }

    void PTTHandler::stop() {
        std::vector<float> audio_samples;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!active_.exchange(false, std::memory_order_acq_rel)) {
                return;
            }
            audio_samples.swap(pcm_buffer_);
        }

        const double duration_sec = static_cast<double>(audio_samples.size()) / OpusTranscoder::kSampleRate;

        spdlog::info("[{}] PTT stopped - transcribing {:.2f} seconds of audio with {} samples",
                     session_id_, duration_sec, audio_samples.size());

        
        if (stt_handler_ && !audio_samples.empty()) {
            auto transcript_res = stt_handler_->transcribe(audio_samples);
            if (transcript_res) {
                spdlog::info("[{}] STT Transcription: {}", session_id_, transcript_res.value());
            } else {
                spdlog::error("[{}] STT Failed: {}", session_id_, transcript_res.error());
            }
        }
    }
}