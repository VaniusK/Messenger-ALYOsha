#pragma once

#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <shared_mutex>

using namespace drogon;

namespace api
{
namespace v1
{
class ServerWebSocketController : public drogon::WebSocketController<ServerWebSocketController>
{
  public:
     void handleNewMessage(const WebSocketConnectionPtr&,
                                  std::string &&,
                                  const WebSocketMessageType &) override;
    void handleNewConnection(const HttpRequestPtr &,
                                     const WebSocketConnectionPtr&) override;
    void handleConnectionClosed(const WebSocketConnectionPtr&) override;
    static void notifyUser(int64_t reciever_id, const std::string &payload);
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/chat");
    WS_PATH_LIST_END
  private:
      static std::unordered_map<int64_t, WebSocketConnectionPtr> clients_;
      static std::shared_mutex clients_mutex_;
};
}
}
