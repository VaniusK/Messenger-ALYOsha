#include "ServerWebSocketController.h"
#include <drogon/WebSocketConnection.h>
#include <jwt-cpp/jwt.h>
#include <mutex>
#include <shared_mutex>

using namespace api::v1;

std::unordered_map<int64_t, WebSocketConnectionPtr> ServerWebSocketController::clients_;
std::shared_mutex ServerWebSocketController::clients_mutex_;

void ServerWebSocketController::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr, std::string &&message, const WebSocketMessageType &type)
{
    wsConnPtr->send("Otsosi pidoras");
}

void ServerWebSocketController::handleNewConnection(const HttpRequestPtr &req, const WebSocketConnectionPtr& wsConnPtr)
{
    int64_t user_id = req->getAttributes()->get<int64_t>("user_id");
    wsConnPtr->setContext(std::make_shared<int64_t>(user_id));
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        clients_[user_id] = wsConnPtr;
    }
    wsConnPtr->send("Successfully connected");
    LOG_INFO << "User " << user_id << " connected";
}

void ServerWebSocketController::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr)
{
    int64_t user_id = (*wsConnPtr->getContext<int64_t>());
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        clients_.erase(user_id);
    }
    LOG_INFO << "User " << user_id << " disconnected";
}

bool ServerWebSocketController::notifyUser(int64_t reciever_id, const std::string &payload){
    drogon::WebSocketConnectionPtr recieverWsConnPtr;
    {
        std::shared_lock<std::shared_mutex> lock(clients_mutex_);
        auto it = clients_.find(reciever_id);
        if (it != clients_.end()){
            recieverWsConnPtr = it->second;
        }
    }
    if (recieverWsConnPtr){
        recieverWsConnPtr->send(payload);
        LOG_INFO << "Message sent to user " << reciever_id;
        return true;
    } else {
        LOG_INFO << "User " << reciever_id << " is not connected";
        return false;
    }
}