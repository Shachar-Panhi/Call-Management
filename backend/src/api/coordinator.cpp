#include  "coordinator.hpp"

#include <functional>
#include <utility>
#include "WebsocketSession.hpp"


namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> manager) : manager_(std::move(manager)) {}

    WebsocketSession::SessionCallback Coordinator::get_join_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(manager_)](std::shared_ptr<WebsocketSession> session) {
            if (auto locked = weak_manager.lock()) {
                locked->join(std::move(session));
            }
        };
    }

    WebsocketSession::SessionCallback Coordinator::get_leave_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(manager_)](std::shared_ptr<WebsocketSession> session) {
            if (auto locked = weak_manager.lock()) {
                locked->leave(std::move(session));
            }
        };
    }

    Callback Coordinator::process_callback() {
        auto on_join = get_join_callback();
        auto on_leave = get_leave_callback();

        return [on_join, on_leave](TCP::socket socket, HTTP::request<HTTP::string_body> req) {
            auto executor = socket.get_executor();
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), on_join, on_leave);
            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }
}