#pragma once

#include <boost/uuid.hpp>

namespace CAM::Utils {

    constexpr std::string generate_session_id() {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return boost::uuids::to_string(uuid);
    }

} // namespace CAM::Utils