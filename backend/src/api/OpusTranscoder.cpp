#include "OpusTranscoder.hpp"

#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>
#include <utility>

namespace CAM::API {
    OpusTranscoder::OpusTranscoder(EncoderHandle encoder, DecoderHandle decoder) noexcept
        : encoder_(std::move(encoder)), decoder_(std::move(decoder)) {}

    std::expected<OpusTranscoder, std::string> OpusTranscoder::create() {
        int error = OPUS_OK; // OPUS_OK basically means no error

        EncoderHandle encoder(
            opus_encoder_create(kSampleRate, kChannels, OPUS_APPLICATION_VOIP, &error),
            opus_encoder_destroy);
            // OPUS_APPLICATION_VOIP means optimise for VOIP which is basically optimise for human speech processing.
            // opus has 2 engines, one for speech and one for music and high end audio. we wanna tell it to use the speech one.

        if (error != OPUS_OK) {
            auto err_msg = std::format("Failed to create OpusEncoder: {}", opus_strerror(error));
            spdlog::error(err_msg);
            return std::unexpected(std::move(err_msg));
            // the unexpected is a string used to represent the error message.
        }
        
        DecoderHandle decoder(
            opus_decoder_create(kSampleRate, kChannels, &error),
            opus_decoder_destroy);

        if (error != OPUS_OK) {
            auto err_msg = std::format("Failed to create OpusDecoder: {}", opus_strerror(error));
            spdlog::error(err_msg);
            return std::unexpected(std::move(err_msg));
        }


        // encoder configuration. the ctl command is for setting parameters and each line is a different parameter we need.
        // it's a C library so we have to add the no error comment.

        opus_encoder_ctl(encoder.get(), OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); //NOLINT
        // optimises for human speech processing once again. the other option is for music.

        opus_encoder_ctl(encoder.get(), OPUS_SET_BITRATE(kBitrate)); //NOLINT
        // this is to set the bit rate which we defined in the hpp file.

        opus_encoder_ctl(encoder.get(), OPUS_SET_VBR(1)); //NOLINT
        // 0 means CBR, 1 means VBR
        // CBR is for constant packet size, VBR is for dynamically changing packet size.
        // we want VBR because it's better for human speech processing since speech is dynamic.

        opus_encoder_ctl(encoder.get(), OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND)); //NOLINT
        // WIDEBAND means maximum of 8kHz frequency range. 
        // we use a sample rate of 16kHz which means we need half of that for the maximum frequency range. higher bands are for like music and stuff.

        opus_encoder_ctl(encoder.get(), OPUS_SET_COMPLEXITY(7)); //NOLINT
        // this sets how much CPU resources the encoder will use. the higher the number, the better the quality but the more CPU resources.
        // 7 is a good balance which gives us good quality without using too much CPU resources. it could be lower but that might interfere with the quality.

        opus_encoder_ctl(encoder.get(), OPUS_SET_INBAND_FEC(1)); //NOLINT
        // this enables inband forward error correction which allows us to reconstuct lost packets and improve the quality of the call.

        opus_encoder_ctl(encoder.get(), OPUS_SET_PACKET_LOSS_PERC(5)); //NOLINT
        // we also need to tell the encoder how much packet loss we expect there to be. 
        // 5% is a good guesstimate and we can increase it if we want the system to work in worse network conditions.

        opus_encoder_ctl(encoder.get(), OPUS_SET_DTX(1)); //NOLINT
        // this enables discontinuous transmission which means that when there's silence, the encoder will save bandwidth by not sending packets.
        // this is important because there's a lot of silence during calls since most of the time only one side is speaking.

        spdlog::info("OpusTranscoder created: sample_rate={}, channels={}, frame_size={}, bitrate={}",
                     kSampleRate, kChannels, kFrameSize, kBitrate);

        return OpusTranscoder(std::move(encoder), std::move(decoder));
        // returns the transcoder because the constructor is private.
    }

    std::expected<std::vector<uint8_t>, std::string>
    OpusTranscoder::encode(const std::array<float, kFrameSize>& pcm_input) {
        //check if the input is in range of the float samples that are used for human speech processing.
        const bool out_of_bounds = std::ranges::any_of(pcm_input, [](float sample) {
            return sample < -1.0F || sample > 1.0F;
        });

        if (out_of_bounds) {
            return std::unexpected("PCM sample out of range [-1.0, 1.0]");
        }

        std::array<uint8_t, kMaxPacketSize> output{};

        const int encoded_bytes = opus_encode_float(
            encoder_.get(),
            pcm_input.data(),
            kFrameSize,
            output.data(),
            kMaxPacketSize);

        // if we couldn't encode the packet then there won't be any encoded bytes.
        if (encoded_bytes < 0) {
            return std::unexpected(
                std::format("opus_encode_float failed: {}", opus_strerror(encoded_bytes)));
        }

        // the span streches from the start of the array to the end of the encoded bytes.
        std::span<const uint8_t> packet_view(output.data(), static_cast<size_t>(encoded_bytes));
        
        // we need a vector because the packet size is dynamic so we can't use an array like what we did in the pcm input.
        return std::vector<uint8_t>(packet_view.begin(), packet_view.end());
    }

    std::expected<std::array<float, OpusTranscoder::kFrameSize>, std::string>
    OpusTranscoder::decode(std::span<const uint8_t> opus_packet) {
        std::array<float, kFrameSize> output{};

        const int decoded_samples = opus_decode_float(
            decoder_.get(),
            opus_packet.empty() ? nullptr : opus_packet.data(), // make sure we don't pass an empty packet
            static_cast<opus_int32>(opus_packet.size()),
            output.data(),
            kFrameSize,
            0);

        // if we couldn't decode the packet then there won't be any decoded samples.
        if (decoded_samples < 0) {
            return std::unexpected(
                std::format("opus_decode_float failed: {}", opus_strerror(decoded_samples)));
        }

        // go over all 160 samples individually
        std::ranges::for_each(output, [](float& sample) {
            sample = std::clamp(sample, -1.0F, 1.0F);
            // makes all of the samples be in range.
            // if a sample is below -1.0 or above 1.0 then it will automatically be set to -1.0 or 1.0 respectively.
        });

        return output;
    }
}