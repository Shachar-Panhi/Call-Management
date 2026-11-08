#pragma once

#include "OpusTranscoder.hpp"
#include "STTHandler.hpp"
#include <rtc/rtc.hpp>

#include <atomic>
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace CAM::API {

    class PTTHandler {
    public:
        static std::expected<std::unique_ptr<PTTHandler>, std::string>
        create(std::string session_id, std::shared_ptr<STTHandler> stt_handler);

        ~PTTHandler() = default;

        PTTHandler(const PTTHandler&) = delete;
        PTTHandler& operator=(const PTTHandler&) = delete;
        PTTHandler(PTTHandler&&) = delete;
        PTTHandler& operator=(PTTHandler&&) = delete;

        void start();
        void stop();
        bool process_packet(const rtc::binary& packet);

    private:
        PTTHandler(std::string session_id, OpusTranscoder transcoder, std::shared_ptr<STTHandler> stt_handler) noexcept;

        std::string session_id_;
        OpusTranscoder transcoder_;
        std::shared_ptr<STTHandler> stt_handler_;

        std::mutex mutex_;
        std::atomic<bool> active_{false};
        std::vector<float> pcm_buffer_;
    };
}