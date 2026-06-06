#include "ServerWebSocketController.h"
#include <drogon/HttpAppFramework.h>
#include <drogon/WebSocketConnection.h>
#include <drogon/utils/coroutine.h>
#include <jwt-cpp/jwt.h>
#include <exception>
#include <vector>
#include "controllers/SecretChatsController.h"
#include "services/ClientNotifier.hpp"

using namespace api::v1;

void ServerWebSocketController::handleNewMessage(
    const WebSocketConnectionPtr &wsConnPtr,
    std::string &&message,
    const WebSocketMessageType &type
) {
    wsConnPtr->send("Otsosi pidoras");
}

void ServerWebSocketController::handleNewConnection(
    const HttpRequestPtr &req,
    const WebSocketConnectionPtr &wsConnPtr
) {
    int64_t user_id = req->getAttributes()->get<int64_t>("user_id");
    wsConnPtr->setContext(std::make_shared<int64_t>(user_id));

    auto notifier = drogon::app().getPlugin<api::v1::WebsocketClientNotifier>();
    auto service = drogon::app().getPlugin<api::v1::SecretChatsService>();

    notifier->addConnection(user_id, wsConnPtr);
    LOG_INFO << "User " << user_id << " connected and put in synced mode";

    drogon::async_run([notifier, service, user_id]() -> drogon::Task<void> {
        try {
            auto offline_messages = co_await service->syncOfflineData(user_id);
            notifier->finishSync(user_id, offline_messages);
            LOG_INFO << "Offline data synced and buffer flushed for user " << user_id;
        }
        catch (const std::exception &e) {
            LOG_ERROR << "Failed to sync " << e.what();
            notifier->finishSync(user_id, {});
        }
    });
}

void ServerWebSocketController::handleConnectionClosed(
    const WebSocketConnectionPtr &wsConnPtr
) {
    int64_t user_id = (*wsConnPtr->getContext<int64_t>());
    auto notifier = drogon::app().getPlugin<api::v1::WebsocketClientNotifier>();
    notifier->removeConnection(user_id);
}

