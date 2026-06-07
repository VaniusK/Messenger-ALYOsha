#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <json/value.h>
#include <cstdint>
#include <string>
#include <vector>

namespace messenger::repositories {
struct HandshakeSignal {
    int64_t sender_id;
    int64_t acceptor_id;
    int32_t message_type;
    std::string public_key;
    std::string chat_id;
};

struct EncryptedMessage {
    int64_t sender_id;
    int64_t acceptor_id;
    int32_t message_type;
    Json::Value payload;
    std::string chat_id;
};

class SecretChatsRepositoryInterface {
public:
    virtual ~SecretChatsRepositoryInterface() = default;
    virtual drogon::Task<void> saveHandshakeSignal(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t type,
        std::string pub_key,
        std::string chat_id
    ) = 0;
    virtual drogon::Task<std::vector<HandshakeSignal>> popHandshakeSignals(
        int64_t acceptor_id
    ) = 0;

    virtual drogon::Task<void> saveEncryptedMessage(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t message_type,
        Json::Value payload,
        std::string chat_id
    ) = 0;
    virtual drogon::Task<std::vector<EncryptedMessage>> popEncryptedMessages(
        int64_t acceptor_id
    ) = 0;
    virtual drogon::Task<void> removeStaleRecords() = 0;
};

class SecretChatsRepository : public SecretChatsRepositoryInterface {
public:
    drogon::Task<void> saveHandshakeSignal(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t type,
        std::string pub_key,
        std::string chat_id
    ) override;
    drogon::Task<std::vector<HandshakeSignal>> popHandshakeSignals(
        int64_t acceptor_id
    ) override;

    drogon::Task<void> saveEncryptedMessage(
        int64_t sender_id,
        int64_t acceptor_id,
        int32_t message_type,
        Json::Value payload,
        std::string chat_id
    ) override;
    drogon::Task<std::vector<EncryptedMessage>> popEncryptedMessages(
        int64_t acceptor_id
    ) override;
    drogon::Task<void> removeStaleRecords() override;

private:
    drogon::orm::DbClientPtr getDbClient() {
        return drogon::app().getDbClient();
    }
};
}  // namespace messenger::repositories
