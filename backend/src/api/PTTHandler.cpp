#include "PTTHandler.hpp"
#include "../ThirdParty/RtpCpp/RtpPacket.hpp"

#include <spdlog/spdlog.h>
#include <cstring>
#include <span>
#include <utility>

namespace CAM::API {

    PTTHandler::PTTHandler(std::string session_id, OpusTranscoder transcoder)
        : session_id_(std::move(session_id)), transcoder_(std::move(transcoder)) {}

    std::expected<std::unique_ptr<PTTHandler>, std::string> PTTHandler::create(std::string session_id) {
        auto transcoder = OpusTranscoder::create();
        if (!transcoder) {
            return std::unexpected("PTTHandler: failed to create OpusTranscoder — " + transcoder.error());
        }
        return std::make_unique<PTTHandler>(std::move(session_id), std::move(transcoder.value()));
    }

    void PTTHandler::start() {
        std::lock_guard<std::mutex> lock(mutex_);
        active_.store(true, std::memory_order_release);
        buffered_frames_.clear();
        spdlog::info("[{}] PTT started — buffering audio", session_id_);
    }

    void PTTHandler::stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        active_.store(false, std::memory_order_release);
        spdlog::info("[{}] PTT stopped — {} frames buffered", session_id_, buffered_frames_.size());
    }

    bool PTTHandler::is_active() const noexcept {
        return active_.load(std::memory_order_relaxed);
    }

    bool PTTHandler::process_packet(const rtc::binary& packet) {
        // Fast path: lockless check when PTT is inactive
        if (!active_.load(std::memory_order_relaxed)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_.load(std::memory_order_relaxed)) {
            return false;
        }

        // In-place zero-allocation RTP parsing using RtpPacketView
        auto* raw_bytes = reinterpret_cast<const uint8_t*>(packet.data());
        std::span<uint8_t> span(const_cast<uint8_t*>(raw_bytes), packet.size());

        RtpCpp::RtpPacketView rtp;
        auto parse_res = rtp.parse(span);
        if (parse_res != RtpCpp::Result::kSuccess) {
            spdlog::warn("[{}] PTT: Failed to parse RTP packet, dropping", session_id_);
            return true;
        }

        auto payload = rtp.payload();
        if (payload.empty()) {
            spdlog::warn("[{}] PTT: Empty RTP payload", session_id_);
            return true;
        }

        // Decode ONLY the payload with the Opus transcoder
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
        std::lock_guard<std::mutex> lock(mutex_);
        active_.store(false, std::memory_order_release);
        spdlog::info("[{}] PTT stopped — releasing {} buffered frames", session_id_, buffered_frames_.size());

        std::vector<rtc::binary> released;
        released.reserve(buffered_frames_.size());

        for (const auto& frame : buffered_frames_) {
            // Re-encode the decoded PCM samples back to Opus
            auto encoded_res = transcoder_.encode(frame.pcm_);
            if (!encoded_res) {
                spdlog::error("[{}] PTT: Failed to encode PCM frame: {}", session_id_, encoded_res.error());
                continue;
            }

            // Reconstruct the RTP packet using stack-allocated StaticRtpPacket (no heap allocation)
            RtpCpp::StaticRtpPacket rtp;
            rtp.set_payload_type(frame.payload_type_);
            rtp.set_sequence_number(frame.sequence_number_);
            rtp.set_timestamp(frame.timestamp_);
            rtp.set_ssrc(frame.ssrc_);
            rtp.set_marker(frame.is_marked_);
            (void)rtp.set_payload_size(OpusTranscoder::kEncodedPacketSize);

            const auto& encoded_bytes = encoded_res.value();
            std::memcpy(rtp.payload().data(), encoded_bytes.data(), OpusTranscoder::kEncodedPacketSize);

            auto pkt_span = rtp.packet();
            rtc::binary bin(pkt_span.size());
            std::memcpy(bin.data(), pkt_span.data(), pkt_span.size());

            released.push_back(std::move(bin));
        }

        buffered_frames_.clear();
        return released;
    }

    size_t PTTHandler::buffered_frame_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffered_frames_.size();
    }

} // namespace CAM::API