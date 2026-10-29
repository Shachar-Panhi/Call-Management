#pragma once

#include <string>
#include <optional>

namespace CAM::API {
    struct ConnectionPacket {
        std::string session_id; //NOLINT
    };

    struct SdpOfferPacket {
        std::string session_id; //NOLINT
        std::string sdp; //NOLINT
    };

    struct IceOfferPacket {
        std::string session_id; //NOLINT
        std::string candidate; //NOLINT
        std::optional<std::string> sdpMid; //NOLINT
    };

    struct HttpCallRequest {
        std::string session_id; //NOLINT
        std::string target_session_id; //NOLINT
    };

    struct HttpEndCallRequest {
        std::string session_id; //NOLINT
    };

    struct HttpPttRequest {
        std::string session_id; //NOLINT
        bool active = false; //NOLINT
    };

    struct ApiResponsePacket {
        std::string type = "api_response"; //NOLINT
        std::string action; //NOLINT
        std::string status; //NOLINT
    };
}