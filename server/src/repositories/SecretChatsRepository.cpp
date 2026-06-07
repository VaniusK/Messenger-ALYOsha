#include "repositories/SecretChatsRepository.hpp"
#include <drogon/orm/Exception.h>
#include <json/value.h>
#include <exception>
#include "utils/server_exceptions.hpp"

namespace messenger::repositories {
drogon::Task<void> SecretChatsRepository::saveHandshakeSignal(
    int64_t sender_id,
    int64_t acceptor_id,
    int32_t type,
    std::string pub_key,
    std::string chat_id
) {
    auto db_client = getDbClient();
    std::exception_ptr eptr;

    // Check both user exist
    try {
        std::string check_users_query =
            "SELECT COUNT(id) FROM public.users WHERE id IN ($1, $2)";
        auto check_users_result = co_await db_client->execSqlCoro(
            check_users_query, sender_id, acceptor_id
        );
        if (check_users_result.empty() ||
            check_users_result[0][0].as<int64_t>() != 2) {
            throw exceptions::NotFoundException(
                "Couldn't find user with sender's or acceptor's id"
            );
        }
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during validation " << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Service temporarily unavaible during validation"
        );
    }

    // Inserting signal
    try {
        std::string insert_query =
            R"(INSERT INTO e2e.handshakes_pool (sender_id, acceptor_id, message_type, chat_id, public_key)
            VALUES ($1, $2, $3, $4, $5)
            ON CONFLICT (sender_id, acceptor_id)
            DO UPDATE SET public_key = EXCLUDED.public_key, message_type = EXCLUDED.message_type, created_at = NOW();)";
        co_await db_client->execSqlCoro(
            insert_query, sender_id, acceptor_id, type, chat_id, pub_key
        );
        co_return;
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during inserting into handshakes_pool "
                  << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Couldn't save new handshage_signal for offline user"
        );
    }
}

drogon::Task<std::vector<HandshakeSignal>>
SecretChatsRepository::popHandshakeSignals(int64_t acceptor_id) {
    auto db_client = getDbClient();

    try {
        std::string pop_signals_query = R"(DELETE FROM e2e.handshakes_pool 
            WHERE acceptor_id = $1
            RETURNING sender_id, acceptor_id, message_type, public_key;)";
        auto pop_signals_result =
            co_await db_client->execSqlCoro(pop_signals_query, acceptor_id);

        std::vector<HandshakeSignal> result;
        for (const auto &row : pop_signals_result) {
            result.push_back(
                {row["sender_id"].as<int64_t>(),
                 row["acceptor_id"].as<int64_t>(),
                 row["message_type"].as<int32_t>(),
                 row["public_key"].as<std::string>(),
                 row["chat_id"].as<std::string>()}
            );
        }
        co_return result;
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during extracting user's handshakes signals "
                  << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Couldn't get user's handshakes signals"
        );
    }
}

drogon::Task<void> SecretChatsRepository::saveEncryptedMessage(
    int64_t sender_id,
    int64_t acceptor_id,
    int32_t message_type,
    std::string payload,
    std::string chat_id
) {
    auto db_client = getDbClient();

    // Validate users exist
    try {
        std::string check_users_query =
            "SELECT COUNT(id) FROM public.users WHERE id IN ($1, $2)";
        auto check_users_result = co_await db_client->execSqlCoro(
            check_users_query, sender_id, acceptor_id
        );

        if (check_users_result.empty() ||
            check_users_result[0][0].as<int64_t>() != 2) {
            throw exceptions::NotFoundException(
                "Couldn't find user with sender's or acceptor's id"
            );
        }
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during validation " << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Service temporarily unavaible during validation"
        );
    }

    // Inserting
    try {
        std::string insert_message_query = R"(INSERT INTO e2e.messages_pool
            (sender_id, acceptor_id, message_type, chat_id, encrypted_payload)
            VALUES ($1, $2, $3, $4, $5);)";
        auto insert_message_result = co_await db_client->execSqlCoro(
            insert_message_query, sender_id, acceptor_id, message_type, chat_id,
            payload
        );
        co_return;
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during inserting into messages_pool "
                  << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Couldn't save new encrypted message for offline user"
        );
    }
}

drogon::Task<std::vector<EncryptedMessage>>
SecretChatsRepository::popEncryptedMessages(int64_t acceptor_id) {
    auto db_client = getDbClient();

    try {
        std::string pop_query =
            "DELETE FROM e2e.messages_pool WHERE acceptor_id = $1 RETURNING "
            "sender_id, acceptor_id, message_type, chat_id, encrypted_payload;";
        auto pop_result =
            co_await db_client->execSqlCoro(pop_query, acceptor_id);
        std::vector<EncryptedMessage> result;
        for (const auto &row : pop_result) {
            result.push_back(
                {row["sender_id"].as<int64_t>(),
                 row["acceptor_id"].as<int64_t>(),
                 row["message_type"].as<int32_t>(),
                 row["encrypted_payload"].as<std::string>(),
                 row["chat_id"].as<std::string>()}
            );
        }
        co_return result;
    } catch (drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during extracting user's encrypted messages "
                  << e.base().what();
        throw exceptions::InternalServerErrorException(
            "Couldn't get user's offline messages"
        );
    }
}

drogon::Task<void> SecretChatsRepository::removeStaleRecords() {
    auto db_client = getDbClient();
    try {
        co_await db_client->execSqlCoro(
            "DELETE FROM e2e.handshakes_pool WHERE created_at < NOW() - "
            "INTERVAL '30 days'"
        );
        co_await db_client->execSqlCoro(
            "DELETE FROM e2e.messages_pool WHERE created_at < NOW() - INTERVAL "
            "'30 days'"
        );
        LOG_INFO << "Successfullu cleaned stale E2E records";
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error during stale records cleanup: "
                  << e.base().what();
    }
}
}  // namespace messenger::repositories
