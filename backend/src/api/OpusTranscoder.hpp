#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <opus.h>

namespace CAM::API {
    class OpusTranscoder {
    public:
        // the unique_ptrs manage cleanup automatically and prevent memory leaks. 
        // the custom deleters are used to call the correct cleanup functions. they are provided by opus library.
        using EncoderHandle = std::unique_ptr<OpusEncoder, decltype(&opus_encoder_destroy)>;
        using DecoderHandle = std::unique_ptr<OpusDecoder, decltype(&opus_decoder_destroy)>;

        static constexpr int kSampleRate = 16000; // 16kHz is the default for human speech processing
        static constexpr int kChannels = 1; // one channel means no distinction between left and right speakers, like in a phone call
        static constexpr int kFrameSize = 160; // standard for human speech processing at 16kHz. 160 samples means 10ms of audio
        static constexpr int kBitrate = 64000; // pretty low bitrate but sufficent for human speech.
        static constexpr int kMaxPacketSize = 4000; // safety size so the encoding/decoding don't fail if the packet is too big


        // constructors according to the rule of 5. we want to be able to move the OpusTranscoder but not copy it, since it contains unique_ptrs
        ~OpusTranscoder() = default;

        OpusTranscoder(const OpusTranscoder&) = delete;
        OpusTranscoder& operator=(const OpusTranscoder&) = delete;

        OpusTranscoder(OpusTranscoder&&) noexcept = default;
        OpusTranscoder& operator=(OpusTranscoder&&) noexcept = default;


        static std::expected<OpusTranscoder, std::string> create();

        std::expected<std::vector<uint8_t>, std::string> // takes float and returns a uint-8 vector
        encode(const std::array<float, kFrameSize>& pcm_input); // frame size is always 160 which means we can put the pcm input in a fixed array cause it's not dynamic

        std::expected<std::array<float, kFrameSize>, std::string> // takes a uint-8 span and returns float. once again, we can return a fixed array becuase frame size doesn't change.
        decode(std::span<const uint8_t> opus_packet);

    private:
        // private constructor to enforce the use of create() and prevent exceptions
        OpusTranscoder(EncoderHandle encoder, DecoderHandle decoder) noexcept;
        // constructor marked as no except because it doesn't throw exceptions, and the unique_ptrs will clean up the resources automatically.
        
        EncoderHandle encoder_;
        DecoderHandle decoder_;
    };
}