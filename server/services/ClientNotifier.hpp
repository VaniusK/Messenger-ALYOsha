#pragma once

#include <drogon/WebSocketConnection.h>
#include <drogon/plugins/Plugin.h>
#include <json/value.h>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace api::v1 {
class ClientNotifierInterface {
public:
    virtual ~ClientNotifierInterface() = default;
    virtual bool notifyClient(int64_t user_id, const std::string &payload) = 0;
    virtual void addConnection(
        int64_t user_id,
        const drogon::WebSocketConnectionPtr conn
    ) = 0;
    virtual void removeConnection(int64_t user_id) = 0;
};

class WebsocketClientNotifier : public drogon::Plugin<WebsocketClientNotifier> {
private:
    struct ClientSession {
        drogon::WebSocketConnectionPtr connection;
        bool is_syncing = true;
        std::vector<std::string> pending_messages;
    };

    std::unordered_map<int64_t, ClientSession> clients_;
    std::shared_mutex clients_mutex_;

public:
    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    bool notifyClient(int64_t user_id, const std::string &message);
    void
    addConnection(int64_t user_id, const drogon::WebSocketConnectionPtr &conn);
    void removeConnection(int64_t user_id);
    void finishSync(
        int64_t user_id,
        const std::vector<std::string> &offline_messages
    );
};
}  // namespace api::v1