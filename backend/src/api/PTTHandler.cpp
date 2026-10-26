#include "PTTHandler.hpp"
#include "../ThirdParty/RtpCpp/RtpPacket.hpp"

#include <algorithm>
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
        buffered_frames_.clear();
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
            spdlog::warn("[{}] PTT: Failed to decode Opus payload: {}", session_id_, decoded_res.error());
            return true;
        }

        const auto& header = rtp.get_header();
        buffered_frames_.push_back(BufferedFrame{
            .pcm_ = decoded_res.value(),
            .sequence_number_ = header.sequence_number_,
            .timestamp_ = header.timestamp_,
            .ssrc_ = header.ssrc_,
            .payload_type_ = header.payload_type_,
            .is_marked_ = header.is_marked_
        });

        return true;
    }

    std::vector<rtc::binary> PTTHandler::stop_and_release() {
        std::vector<BufferedFrame> frames_to_process;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!active_.exchange(false, std::memory_order_acq_rel)) {
                return {};
            }
            frames_to_process.swap(buffered_frames_);
        }

        spdlog::info("[{}] PTT stopped - processing {} frames", session_id_, frames_to_process.size());

        std::vector<float> contiguous_pcm;
        contiguous_pcm.reserve(frames_to_process.size() * OpusTranscoder::kDecodedPacketSize);

        std::vector<rtc::binary> released;
        released.reserve(frames_to_process.size());

        for (const auto& frame : frames_to_process) {
            std::ranges::copy(frame.pcm_, std::back_inserter(contiguous_pcm));

            auto encoded_res = transcoder_.encode(frame.pcm_);
            if (!encoded_res) {
                spdlog::error("[{}] PTT: Failed to encode PCM frame: {}", session_id_, encoded_res.error());
                continue;
            }

            RtpCpp::StaticRtpPacket rtp;
            rtp.set_payload_type(frame.payload_type_);
            rtp.set_sequence_number(frame.sequence_number_);
            rtp.set_timestamp(frame.timestamp_);
            rtp.set_ssrc(frame.ssrc_);
            rtp.set_marker(frame.is_marked_);
            if (rtp.set_payload_size(OpusTranscoder::kEncodedPacketSize) != RtpCpp::Result::kSuccess) {
                continue;
            }

            std::memcpy(rtp.payload().data(), encoded_res->data(), OpusTranscoder::kEncodedPacketSize);

            auto pkt_span = rtp.packet();
            rtc::binary bin(pkt_span.size());
            std::memcpy(bin.data(), pkt_span.data(), pkt_span.size());

            released.push_back(std::move(bin));
        }

        if (stt_handler_ && !contiguous_pcm.empty()) {
            auto transcript_res = stt_handler_->transcribe(contiguous_pcm);
            if (transcript_res) {
                spdlog::info("[{}] STT Transcription: {}", session_id_, transcript_res.value());
            } else {
                spdlog::error("[{}] STT Failed: {}", session_id_, transcript_res.error());
            }
        }

        return released;
    }

}