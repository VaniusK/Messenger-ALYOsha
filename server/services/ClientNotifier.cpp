#include "services/ClientNotifier.hpp"
#include <cstdint>
#include <string>
#include "controllers/ServerWebSocketController.h"

namespace api::v1 {
class WebSocketClientNotifier : public ClientNotifierInterface {
public:
    bool notifyUser(int64_t user_id, const std::string &payload) {
        return ServerWebSocketController::notifyUser(user_id, payload);
    }
};
}  // namespace api::v1