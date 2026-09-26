#pragma once

#include <string>
#include <functional>
#include <memory>
#include <rtc/rtc.hpp>

namespace CAM::API {
    class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
    public:
        static constexpr int kOpusCodecNum = 111;

        using SignalingCallback = std::function<void(std::string)>;
        using PeerCallback = std::function<void(const std::shared_ptr<PeerConnection>&)>;

        PeerConnection(PeerCallback on_join, PeerCallback on_leave, std::string session_id);
        void set_signaling_callback(SignalingCallback callback);
        void handle_signaling_message(const std::string& message);
        void initialize_webrtc();

        void setup_media_tracks();
        static std::string enforce_16khz(std::string sdp);

        void close();

        void handle_state(rtc::PeerConnection::State state);
        void handle_description(const rtc::Description& description);
        void handle_candidate(const rtc::Candidate& candidate);

    private:
        std::string session_id_;
        
        std::shared_ptr<rtc::PeerConnection> rtc_connection_;
        std::shared_ptr<rtc::Track> audio_track_;

        SignalingCallback send_signaling_;
        
        PeerCallback on_join_;
        PeerCallback on_leave_;
    };
}