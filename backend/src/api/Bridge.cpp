#include "Bridge.hpp"

#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace CAM::API {
    namespace {
        std::unique_ptr<PTTHandler> init_ptt_handler(
            const std::string& session_id,
            const std::shared_ptr<STTHandler>& stt_handler)
        {
            auto ptt = PTTHandler::create(session_id, stt_handler);
            if (!ptt) {
                spdlog::error("Failed to create PTTHandler for session {}: {}", session_id, ptt.error());
                return nullptr;
            }
            return std::move(ptt.value());
        }
    }

    Bridge::Bridge(std::weak_ptr<PeerConnection> peer1,
                   std::weak_ptr<PeerConnection> peer2,
                   std::string session1_id,
                   std::string session2_id,
                   const std::shared_ptr<STTHandler>& stt_handler)
        : peer1_(std::move(peer1)),
          peer2_(std::move(peer2)),
          session1_id_(std::move(session1_id)),
          session2_id_(std::move(session2_id)),
          ptt_handler1_(init_ptt_handler(session1_id_, stt_handler)),
          ptt_handler2_(init_ptt_handler(session2_id_, stt_handler)) {}

    bool Bridge::contains(const std::string& session_id) const {
        return session1_id_ == session_id || session2_id_ == session_id;
    }

    std::string Bridge::get_partner(const std::string& session_id) const {
        if (session1_id_ == session_id) {
            return session2_id_;
        }
        if (session2_id_ == session_id) {
            return session1_id_;
        }
        return "";
    }

    void Bridge::setup_routing() {
        bind_peer_audio(peer1_, peer2_, ptt_handler1_);
        bind_peer_audio(peer2_, peer1_, ptt_handler2_);
    }

    void Bridge::bind_peer_audio(const std::weak_ptr<PeerConnection>& source_peer,
                                 const std::weak_ptr<PeerConnection>& target_peer,
                                 const std::unique_ptr<PTTHandler>& ptt_handler) {
        auto source = source_peer.lock();
        if (!source) {
            return;
        }

        PTTHandler* handler_ptr = ptt_handler.get();
        source->set_audio_callback([weak_self = weak_from_this(), target_peer, handler_ptr](const rtc::binary& packet) {
            auto self = weak_self.lock();
            if (!self) {
                return;
            }

            if (handler_ptr != nullptr && handler_ptr->process_packet(packet)) {
                return;
            }

            if (auto target = target_peer.lock()) {
                target->send_audio_packet(packet);
            }
        });
    }

    void Bridge::set_ptt(const std::string& session_id, bool active) {
        const bool is_peer1 = (session_id == session1_id_);
        const bool is_peer2 = (session_id == session2_id_);
        if (!is_peer1 && !is_peer2) {
            return;
        }

        auto& handler = is_peer1 ? ptt_handler1_ : ptt_handler2_;
        auto& target_peer = is_peer1 ? peer2_ : peer1_;
        if (!handler) {
            return;
        }

        if (active) {
            handler->start();
            return;
        }

        auto packets = handler->stop_and_release();
        if (auto target = target_peer.lock()) {
            for (const auto& packet : packets) {
                target->send_audio_packet(packet);
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