#include "Bridge.hpp"

#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace CAM::API {
    Bridge::Bridge(std::weak_ptr<PeerConnection> peer1, std::weak_ptr<PeerConnection> peer2,
        std::string session1_id, std::string session2_id)
    : peer1_(std::move(peer1)), peer2_(std::move(peer2)),
        session1_id_(std::move(session1_id)), session2_id_(std::move(session2_id))
    {
        auto ptt1 = PTTHandler::create(session1_id_);
        if (ptt1) {
            ptt_handler1_ = std::move(ptt1.value());
        } else {
            spdlog::error("Failed to create PTTHandler for session {}: {}", session1_id_, ptt1.error());
        }

        auto ptt2 = PTTHandler::create(session2_id_);
        if (ptt2) {
            ptt_handler2_ = std::move(ptt2.value());
        } else {
            spdlog::error("Failed to create PTTHandler for session {}: {}", session2_id_, ptt2.error());
        }
    }

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
            pc1->set_audio_callback([weak_self = weak_from_this()](const rtc::binary& packet) {
                auto self = weak_self.lock();
                if (!self) {
                    return;
                }

                if (self->ptt_handler1_ && self->ptt_handler1_->process_packet(packet)) {
                    return;
                }

                if (auto pc2 = self->peer2_.lock()) {
                    pc2->send_audio_packet(packet);
                }
            });
        }

        if (auto pc2 = peer2_.lock()) {
            pc2->set_audio_callback([weak_self = weak_from_this()](const rtc::binary& packet) {
                auto self = weak_self.lock();
                if (!self) {
                    return;
                }

                if (self->ptt_handler2_ && self->ptt_handler2_->process_packet(packet)) {
                    return;
                }

                if (auto pc1 = self->peer1_.lock()) {
                    pc1->send_audio_packet(packet);
                }
            });
        }
    }

    void Bridge::start_ptt(const std::string& session_id) {
        if (session_id == session1_id_ && ptt_handler1_) {
            ptt_handler1_->start();
        } else if (session_id == session2_id_ && ptt_handler2_) {
            ptt_handler2_->start();
        }
    }

    void Bridge::stop_ptt(const std::string& session_id) {
        if (session_id == session1_id_ && ptt_handler1_) {
            auto packets = ptt_handler1_->stop_and_release();
            if (auto pc2 = peer2_.lock()) {
                for (const auto& packet : packets) {
                    pc2->send_audio_packet(packet);
                }
            }
        } else if (session_id == session2_id_ && ptt_handler2_) {
            auto packets = ptt_handler2_->stop_and_release();
            if (auto pc1 = peer1_.lock()) {
                for (const auto& packet : packets) {
                    pc1->send_audio_packet(packet);
                }
            }
        }
    }

    const std::string& Bridge::get_session1() const {
        return session1_id_;
    }

    const std::string& Bridge::get_session2() const {
        return session2_id_;
    }
}
