#include  "coordinator.hpp"

namespace CAM::API {
    Coordinator::Coordinator() : manager_(std::make_shared<WebsocketManager>()) {}

    Callback Coordinator::process_callback() {
        return [manager = manager_](TCP::socket socket, HTTP::request<HTTP::string_body> req)
        {
            auto executor = socket.get_executor();
            auto websocket_session = std::make_shared<WebsocketSession>(std::move(socket), manager);
            boost::asio::co_spawn(executor, websocket_session->start(std::move(req)), boost::asio::detached);
        };
    }
}