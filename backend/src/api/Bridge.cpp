#include "Bridge.hpp"

#include <string>
#include <utility>

namespace CAM::API {
    Bridge::Bridge(std::weak_ptr<PeerConnection> peer1, std::weak_ptr<PeerConnection> peer2,
        std::string session1_id, std::string session2_id)
    : peer1_(std::move(peer1)), peer2_(std::move(peer2)),
        session1_id_(std::move(session1_id)), session2_id_(std::move(session2_id)) {}

    bool Bridge::contains(const std::string& session_id) const {
        return session1_id_ == session_id || session2_id_ == session_id;
    }

    std::string Bridge::get_partner(const std::string& session_id) const {
        if (session1_id_ == session_id) {
            return session2_id_;
        } else if (session2_id_ == session_id) {
            return session1_id_;
        }
        return "";
    }

    void Bridge::setup_routing() {
        if (auto pc1 = peer1_.lock()) {
            pc1->set_audio_callback([weak_p2 = peer2_](const rtc::binary& packet) {
                if (auto pc2 = weak_p2.lock()) {
                    pc2->send_audio_packet(packet);
                }
            });
        }

        if (auto pc2 = peer2_.lock()) {
            pc2->set_audio_callback([weak_p1 = peer1_](const rtc::binary& packet) {
                if (auto pc1 = weak_p1.lock()) {
                    pc1->send_audio_packet(packet);
                }
            });
        }
    }

    const std::string& Bridge::get_session1() const {
        return session1_id_;
    }

    const std::string& Bridge::get_session2() const {
        return session2_id_;
    }
}