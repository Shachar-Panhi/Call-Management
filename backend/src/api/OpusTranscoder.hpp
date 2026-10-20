#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>

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
        static constexpr int kBitrate = 64000; // 64kbps provides high-fidelity wideband speech
        static constexpr int kFrameDurationMs = 20; // 20ms is the standard VoIP/WebRTC frame duration

        // calculated array sizes for 20ms duration:
        // sample rate: 16k -> 16000 * 20 / 1000 = 320 samples (decoded payload)
        static constexpr int kDecodedPacketSize = (kSampleRate * kFrameDurationMs) / 1000;
        static constexpr int kFrameSize = kDecodedPacketSize; // 320 samples

        // bit rate: 64k -> (64000 * 20) / (8 * 1000) = 160 bytes (encoded payload)
        static constexpr int kEncodedPacketSize = (kBitrate * kFrameDurationMs) / (8 * 1000);

        // constructors according to the rule of 5. we want to be able to move the OpusTranscoder but not copy it, since it contains unique_ptrs
        ~OpusTranscoder() = default;

        OpusTranscoder(const OpusTranscoder&) = delete;
        OpusTranscoder& operator=(const OpusTranscoder&) = delete;

        OpusTranscoder(OpusTranscoder&&) noexcept = default;
        OpusTranscoder& operator=(OpusTranscoder&&) noexcept = default;


        static std::expected<OpusTranscoder, std::string> create();

        std::expected<std::array<uint8_t, kEncodedPacketSize>, std::string> // takes decoded float span and returns fixed uint-8 array
        encode(std::span<const float> decoded_packet); // receives a span of decoded pcm audio (320 samples)

        std::expected<std::array<float, kDecodedPacketSize>, std::string> // takes encoded uint-8 span and returns fixed float array
        decode(std::span<const uint8_t> encoded_packet); // receives a span of encoded opus packet (160 bytes)

    private:
        // private constructor to enforce the use of create() and prevent exceptions
        OpusTranscoder(EncoderHandle encoder, DecoderHandle decoder) noexcept;
        // constructor marked as no except because it doesn't throw exceptions, and the unique_ptrs will clean up the resources automatically.
        
        EncoderHandle encoder_;
        DecoderHandle decoder_;
    };
}