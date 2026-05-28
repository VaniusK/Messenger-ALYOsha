#pragma once

#include <drogon/WebSocketController.h>

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
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/chat", "api::v1::AuthFilter");
    WS_PATH_LIST_END
};
}
}
