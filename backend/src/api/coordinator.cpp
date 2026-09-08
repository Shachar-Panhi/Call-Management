#include  "coordinator.hpp"

#include <utility>
#include "WebsocketSession.hpp"


namespace CAM::API {
    Coordinator::Coordinator(std::shared_ptr<WebsocketManager> manager) : manager_(std::move(manager)) {}

    Callback Coordinator::process_callback() {
        return [weak_manager = std::weak_ptr<WebsocketManager>(manager_)](TCP::socket socket, HTTP::request<HTTP::string_body> req)
        {
            auto executor = socket.get_executor();

            auto on_join = [weak_manager](std::shared_ptr<WebsocketSession> session) {
                if (auto locked = weak_manager.lock()) {
                    locked->join(std::move(session));
                }
            };

            auto on_leave = [weak_manager](std::shared_ptr<WebsocketSession> session) {
                if (auto locked = weak_manager.lock()) {
                    locked->leave(std::move(session));
                }
            };

            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), std::move(on_join), std::move(on_leave));
            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }
}