#pragma once

#include <string>

namespace CAM::API {

        struct SignalingMessage {
        std::string type;
        std::string sdp;
    };

} //namespace CAM::Signaling