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
        std::array<float, OpusTranscoder::kDecodedPacketSize> pcm_{};
        uint16_t sequence_number_ = 0;
        uint32_t timestamp_ = 0;
        uint32_t ssrc_ = 0;
        uint8_t payload_type_ = PeerConnection::kOpusPayloadType;
        bool is_marked_ = false;
    };

    class PTTHandler {
    public:
        static std::expected<std::unique_ptr<PTTHandler>, std::string> create(std::string session_id);

        ~PTTHandler() = default;

        PTTHandler(const PTTHandler&) = delete;
        PTTHandler& operator=(const PTTHandler&) = delete;
        PTTHandler(PTTHandler&&) = delete;
        PTTHandler& operator=(PTTHandler&&) = delete;

        void start();
        bool process_packet(const rtc::binary& packet);
        std::vector<rtc::binary> stop_and_release();

    private:
        PTTHandler(std::string session_id, OpusTranscoder transcoder) noexcept;

        std::string session_id_;
        OpusTranscoder transcoder_;

        std::mutex mutex_;
        std::atomic<bool> active_{false};
        std::vector<BufferedFrame> buffered_frames_;
    };
}