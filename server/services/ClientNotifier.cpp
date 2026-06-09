#include "services/ClientNotifier.hpp"
#include <json/value.h>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace api::v1 {
void WebsocketClientNotifier::initAndStart(const Json::Value &config) {
    LOG_INFO << "WebSocketClientNotifier successfully initialized";
}

void WebsocketClientNotifier::shutdown() {
    LOG_INFO << "Shutting down WebSocketClientNotifier...";
    std::unique_lock<std::shared_mutex> l(clients_mutex_);
    clients_.clear();
}

bool WebsocketClientNotifier::notifyClient(
    int64_t user_id,
    const std::string &message
) {
    {
        std::shared_lock<std::shared_mutex> l(clients_mutex_);
        auto it = clients_.find(user_id);
        if (it != clients_.end() && !it->second.is_syncing) {
            it->second.connection->send(message);
            return true;
        }
    }
    std::unique_lock<std::shared_mutex> l(clients_mutex_);
    auto it = clients_.find(user_id);
    if (it != clients_.end()) {
        if (it->second.is_syncing) {
            it->second.pending_messages.push_back(message);
        } else {
            it->second.connection->send(message);
        }
        return true;
    }
    return false;
}

void WebsocketClientNotifier::addConnection(
    int64_t user_id,
    const drogon::WebSocketConnectionPtr &conn
) {
    std::unique_lock<std::shared_mutex> l(clients_mutex_);
    clients_[user_id] = ClientSession{conn, true, {}};
}

void WebsocketClientNotifier::removeConnection(int64_t user_id) {
    std::unique_lock<std::shared_mutex> l(clients_mutex_);
    clients_.erase(user_id);
}

void WebsocketClientNotifier::finishSync(
    int64_t user_id,
    const std::vector<std::string> &offline_messages
) {
    std::unique_lock<std::shared_mutex> l(clients_mutex_);
    auto it = clients_.find(user_id);
    if (it != clients_.end()) {
        for (const auto &msg : offline_messages) {
            it->second.connection->send(msg);
        }

        for (const auto &msg : it->second.pending_messages) {
            it->second.connection->send(msg);
        }
        it->second.pending_messages.clear();
        it->second.is_syncing = false;
    }
}

}  // namespace api::v1