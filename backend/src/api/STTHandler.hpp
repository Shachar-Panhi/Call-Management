#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <string>

#include <whisper.h>

namespace CAM::API {
    class STTHandler : public std::enable_shared_from_this<STTHandler>{
    public:
        using ContextHandle = std::unique_ptr<whisper_context, decltype(&whisper_free)>;

        static std::expected< std::shared_ptr<STTHandler>, std::string >
        create(const std::filesystem::path& model_path,
            bool use_gpu = true,
            const std::filesystem::path& vad_model_path = {});

        ~STTHandler() = default;

        STTHandler(const STTHandler&) = delete;
        STTHandler& operator=(const STTHandler&) = delete;

        STTHandler(STTHandler&&) = delete;
        STTHandler& operator=(STTHandler&&) = delete;

        std::expected<std::string, std::string>
        transcribe(std::span<const float> pcm_samples);

    private:
        STTHandler(ContextHandle context, std::string vad_model_path) noexcept;

        ContextHandle context_;
        std::string vad_model_path_;
        std::mutex mutex_;
    };
}