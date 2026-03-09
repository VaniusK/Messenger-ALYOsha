#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "models/Messages.h"

using Message = drogon_model::messenger_db::Messages;

namespace messenger::dto {

struct ChatPreview {
    int64_t chat_id;
    std::string title;
    std::optional<std::string> avatar_path;
    std::optional<Message> last_message;
    int64_t unread_count;
    std::string type;
};

}  // namespace messenger::dto
