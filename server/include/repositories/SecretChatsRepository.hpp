#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <json/value.h>
#include <cstdint>
#include <string>
#include <vector>
#include "enums/WebsocketsMessagesTypes.h"

namespace messenger::repositories {
struct HandshakeSignal {
    int64_t sender_id;
    int64_t acceptor_id;
    std::string public_key;
};

struct EncryptedMessage {
    int64_t sender_id;
    int64_t acceptor_id;
    int32_t message_type;
    Json::Value payload;
};

class SecretChatsRepositoryInterface {
public:
    virtual ~SecretChatsRepositoryInterface() = default;
    virtual drogon::Task<bool> saveHandshakeSignal(
        int64_t sender_id,
        int64_t acceptor_id,
        std::string pub_key
    ) = 0;
    virtual drogon::Task<std::vector<HandshakeSignal>> popHandshakeSignals(
        int64_t acceptor_id
    ) = 0;

    virtual drogon::Task<bool> saveEncryptedMessage(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t message_type,
        Json::Value payload
    ) = 0;
    virtual drogon::Task<std::vector<EncryptedMessage>> popEncryptedMessages(
        int64_t acceptor_id
    ) = 0;
};

class SecretChatsRepository : public SecretChatsRepositoryInterface {
public:
    drogon::Task<bool> saveHandshakeSignal(
        int64_t sender_id,
        int64_t acceptor_id,
        std::string pub_key
    ) override;
    drogon::Task<std::vector<HandshakeSignal>> popHandshakeSignals(
        int64_t acceptor_id
    ) override;

    drogon::Task<bool> saveEncryptedMessage(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t message_type,
        Json::Value payload
    ) override;
    drogon::Task<std::vector<EncryptedMessage>> popEncryptedMessages(
        int64_t acceptor_id
    ) override;

private:
    drogon::orm::DbClientPtr getDbClient() {
        return drogon::app().getDbClient();
    }
};
}  // namespace messenger::repositories
