#pragma once

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include <whisper.h>

namespace CAM::API {
    class STTHandler : public std::enable_shared_from_this<STTHandler> {
    public:
        using ContextHandle = std::unique_ptr<whisper_context, decltype(&whisper_free)>;
        using TranscribeCallback = std::function<void(std::expected<std::string, std::string>)>;

        static std::expected<std::shared_ptr<STTHandler>, std::string>
        create(const std::filesystem::path& model_path,
               const std::filesystem::path& vad_model_path = {},
               bool use_gpu = true);

        ~STTHandler();

        STTHandler(const STTHandler&) = delete;
        STTHandler& operator=(const STTHandler&) = delete;

        STTHandler(STTHandler&&) = delete;
        STTHandler& operator=(STTHandler&&) = delete;

        std::expected<std::string, std::string>
        transcribe(std::span<const float> pcm_samples);

        void transcribe_async(std::vector<float> pcm_samples, TranscribeCallback callback);

    private:
        STTHandler(ContextHandle context, std::string vad_model_path) noexcept;

        ContextHandle context_;
        std::string vad_model_path_;
        std::mutex mutex_;
        boost::asio::thread_pool worker_pool_{1};
    };
}