#include "STTHandler.hpp"

#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>
#include <thread>
#include <utility>

namespace CAM::API {

    STTHandler::STTHandler(ContextHandle context) noexcept
        : context_(std::move(context)) {}

    std::expected< std::shared_ptr<STTHandler>, std::string >
    STTHandler::create(const std::filesystem::path& model_path, bool use_gpu) {
        if (!std::filesystem::exists(model_path)) {
            return std::unexpected(
                std::format("Whisper model file not found: {}", model_path.string()));
        }

        whisper_log_set([](enum ggml_log_level, const char*, void*) {}, nullptr);

        whisper_context_params cparams = whisper_context_default_params();
        cparams.use_gpu = use_gpu;
        cparams.flash_attn = true;

        const std::string path_str = model_path.string();
        ContextHandle ctx(
            whisper_init_from_file_with_params(path_str.c_str(), cparams),
            whisper_free);

        if (!ctx) {
            return std::unexpected(
                std::format("Failed to initialize Whisper context from: {}", path_str));
        }

        spdlog::info("Whisper STT initialized. System optimizations: {}", whisper_print_system_info());

        if (whisper_is_multilingual(ctx.get()) != 0) {
            spdlog::warn("Loaded Whisper model is multilingual; an English-specific (.en) model is recommended");
        }

        return std::shared_ptr<STTHandler>(new STTHandler(std::move(ctx)));
    }

    std::expected<std::string, std::string>
    STTHandler::transcribe(std::span<const float> pcm_samples) {
        if (pcm_samples.empty()) {
            return std::string{};
        }

        std::lock_guard<std::mutex> lock(mutex_);

        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        const int max_threads = static_cast<int>(std::thread::hardware_concurrency());
        wparams.n_threads = std::max(1, std::min(4, max_threads > 0 ? max_threads / 2 : 1));
        wparams.language = "en";
        wparams.translate = false;
        wparams.no_context = true;
        wparams.single_segment = true;
        wparams.print_progress = false;
        wparams.print_realtime = false;
        wparams.print_special = false;
        wparams.print_timestamps = false;

        const int ret = whisper_full(
            context_.get(),
            wparams,
            pcm_samples.data(),
            static_cast<int>(pcm_samples.size()));

        if (ret != 0) {
            return std::unexpected(std::format("whisper_full failed with code {}", ret));
        }

        const int n_segments = whisper_full_n_segments(context_.get());
        std::string transcript;

        for (int i = 0; i < n_segments; ++i) {
            const char* segment_text = whisper_full_get_segment_text(context_.get(), i);
            if (segment_text != nullptr) {
                transcript += segment_text;
            }
        }

        return transcript;
    }

}