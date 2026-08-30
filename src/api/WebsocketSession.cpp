#include "WebsocketSession.hpp"
#include "types.hpp"
#include <glaze/glaze.hpp>
#include <variant>

namespace CAM::API { 
    WebsocketSession::WebsocketSession(TCP::socket socket)
    : ws_(std::move(socket)) {}
    
    boost::asio::awaitable<void> WebsocketSession::start(HTTP::request<HTTP::string_body> req) {
        auto self = shared_from_this();
        boost::system::error_code errc;                    
        
        co_await ws_.async_accept(req, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        if (errc) {
            spdlog::error("WebSocket accept error: {}", errc.message());
            co_return;
        }

        spdlog::info("WebSocket client connected successfully");

        boost::beast::flat_buffer buffer;

        while (ws_.is_open()) {
            buffer.clear();

            co_await ws_.async_read(buffer, boost::asio::redirect_error(boost::asio::use_awaitable, errc));
            
            if (errc) {
                if (errc == Websocket::error::closed || errc == boost::asio::error::eof) {
                    spdlog::info("WebSocket client disconnected cleanly");
                } else {
                    spdlog::error("WebSocket read error: {}", errc.message());
                }
                break;
            }

            std::string message = boost::beast::buffers_to_string(buffer.data());
            co_await handle_message(message);
        }
    }

    boost::asio::awaitable<void> WebsocketSession::handle_message(const std::string& message) {
        SignalingMessage sig_msg;
        auto parse_error = glz::read_json(sig_msg, message);
        
        if (parse_error) {
            spdlog::error("JSON parse error");
            co_return;
        }

        spdlog::info("Received signaling message of type: {}", sig_msg.type);

        if (sig_msg.type == "offer") {            
            rtc::Configuration config;
            pc_ = std::make_shared<rtc::PeerConnection>(config);

            auto weak_self = weak_from_this();

            pc_->onLocalDescription([weak_self](rtc::Description description) {
                if (auto self = weak_self.lock()) {                    
                    SignalingMessage answer_msg{"answer", std::string(description)};
                    std::string answer_json;
                    auto errc = glz::write_json(answer_msg, answer_json);
                    if (errc) {
                        spdlog::info("parsing error: {}", errc.custom_error_message);
                        return;
                    }

                    boost::asio::co_spawn(self->ws_.get_executor(), self->send_message(answer_json), boost::asio::detached);
                }
            });

            pc_->onTrack([](std::shared_ptr<rtc::Track> track) {
                track->onMessage([](rtc::message_variant data) {
                    if (std::holds_alternative<rtc::binary>(data)) {
                        spdlog::info("Packet received, size: {}", std::get<rtc::binary>(data).size());
                    }
                });
            });

            rtc::Description offer(sig_msg.sdp, sig_msg.type);
            pc_->setRemoteDescription(offer);
        }
        
        co_return;
    }

    boost::asio::awaitable<void> WebsocketSession::send_message(std::string message) {
        boost::system::error_code errc;
        
        ws_.text(true);
        co_await ws_.async_write(boost::asio::buffer(message), boost::asio::redirect_error(boost::asio::use_awaitable, errc));
        
        if (errc) {
            spdlog::error("WebSocket write error: {}", errc.message());
        }
    }
}