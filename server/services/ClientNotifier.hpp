#pragma once

#include <cstdint>
#include <string>

namespace api::v1 {
class ClientNotifierInterface {
public:
    virtual ~ClientNotifierInterface() = default;
    virtual bool notifyClient(int64_t user_id, const std::string &payload) = 0;
};

class WebsocketClientNotifier : public ClientNotifierInterface {
    bool notifyClient(int64_t user_id, const std::string &payload);
};
}  // namespace api::v1