#pragma once

#include <string>
#include <optional>

namespace CAM::API {
    struct SignalingPacket {
        std::string type;
        std::optional<std::string> sdp;
        std::optional<std::string> candidate;
        std::optional<std::string> sdpMid;
    };

} //namespace CAM::Signaling