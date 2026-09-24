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
}