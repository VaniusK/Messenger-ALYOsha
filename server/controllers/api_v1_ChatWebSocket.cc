#include "api_v1_ChatWebSocket.h"
#include <drogon/WebSocketConnection.h>
#include <jwt-cpp/jwt.h>
#include <mutex>
#include <shared_mutex>

using namespace api::v1;

std::unordered_map<int64_t, WebSocketConnectionPtr> ChatWebSocket::clients_;
std::shared_mutex ChatWebSocket::clients_mutex_;

void ChatWebSocket::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr, std::string &&message, const WebSocketMessageType &type)
{
    wsConnPtr->send("There is nothing useful");
}

void ChatWebSocket::handleNewConnection(const HttpRequestPtr &req, const WebSocketConnectionPtr& wsConnPtr)
{
    std::string header = req->getHeader("Authorization");
    if (header.empty() || header.substr(0, 7) != "Bearer "){
        LOG_WARN << "No token provided or invalid token format";
        wsConnPtr->forceClose();
        return;
    }
    std::string token = header.substr(7);
    int64_t user_id;
    try{
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256(std::getenv("JWT_KEY")))
            .with_issuer("alesha_messenger");
        verifier.verify(decoded);
        std::string user_id_str = decoded.get_payload_claim("user_id").as_string();
        user_id = std::stoll(user_id_str);
        LOG_INFO << "Successfully got user_id " << user_id << " from token";
    }
    catch(const std::exception &e){
        LOG_WARN << "Accession failed: " << e.what();
        wsConnPtr->forceClose();
        return;
    }
    wsConnPtr->setContext(std::make_shared<int64_t>(user_id));
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        clients_[user_id] = wsConnPtr;
    }
    LOG_INFO << "User " << user_id << " connected";
}

void ChatWebSocket::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr)
{
    int64_t user_id = (*wsConnPtr->getContext<int64_t>());
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        clients_.erase(user_id);
    }
    LOG_INFO << "User " << user_id << " disconnected";
}

void ChatWebSocket::notifyUser(int64_t reciever_id, const std::string &payload){
    drogon::WebSocketConnectionPtr recieverWsConnPtr;
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        auto it = clients_.find(reciever_id);
        if (it != clients_.end()){
            recieverWsConnPtr = it->second;
        }
    }
    if (recieverWsConnPtr){
        recieverWsConnPtr->send(payload);
        LOG_INFO << "Message sent to user " << reciever_id;
    } else {
        LOG_INFO << "User " << reciever_id << " is not connected";
    }
}