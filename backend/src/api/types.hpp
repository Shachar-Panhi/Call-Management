#pragma once

#include <string>
#include <optional>

namespace CAM::API {
    struct ConnectionPacket {
        std::string session_id;
    };

    struct SdpOfferPacket {
        std::string session_id;
        std::string sdp;
    };

    struct IceOfferPacket {
        std::string session_id;
        std::string candidate;
        std::optional<std::string> sdpMid;
    };

    struct ApiRequestPacket {
        std::optional<std::string> action;
        std::optional<std::string> target_session_id;
    };

    struct ApiResponsePacket {
        std::string type = "api_response";
        std::string action;
        std::string status;
    };
}