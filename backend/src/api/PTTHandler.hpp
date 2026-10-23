#pragma once

#include "OpusTranscoder.hpp"
#include "PeerConnection.hpp"
#include <rtc/rtc.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace CAM::API {

    struct BufferedFrame {
        // Decoded PCM float samples: 320 samples (20ms at 16kHz mono)
        std::array<float, OpusTranscoder::kDecodedPacketSize> pcm_{};

        // RTP header fields preserved to reconstruct valid RTP packets upon release
        uint16_t sequence_number_ = 0;
        uint32_t timestamp_ = 0;
        uint32_t ssrc_ = 0;
        uint8_t payload_type_ = PeerConnection::kOpusPayloadType;
        bool is_marked_ = false;
    };

    class PTTHandler {
    public:
        static std::expected<std::unique_ptr<PTTHandler>, std::string> create(std::string session_id);

        PTTHandler(std::string session_id, OpusTranscoder transcoder);
        ~PTTHandler() = default;

        PTTHandler(const PTTHandler&) = delete;
        PTTHandler& operator=(const PTTHandler&) = delete;
        PTTHandler(PTTHandler&&) = delete;
        PTTHandler& operator=(PTTHandler&&) = delete;

        void start();
        void stop();
        bool is_active() const noexcept;

        // If PTT is active, extracts the Opus payload from the RTP packet,
        // decodes it with OpusTranscoder, saves the decoded PCM frame in the buffer,
        // and returns true (packet consumed).
        // If PTT is not active, immediately returns false without acquiring mutex.
        bool process_packet(const rtc::binary& packet);

        // Stops PTT and releases all buffered decoded frames as re-encoded RTP packets.
        std::vector<rtc::binary> stop_and_release();

        size_t buffered_frame_count() const;

    private:
        std::string session_id_;
        OpusTranscoder transcoder_;

        mutable std::mutex mutex_;
        std::atomic<bool> active_{false};
        std::vector<BufferedFrame> buffered_frames_;
    };
}
