#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <cstdlib>
#include <memory>
#include "../fixtures/ControllerTestFixture.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "models/Chats.h"
#include "models/Users.h"
#include "utils/Enum.hpp"

using namespace drogon;

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

TEST_F(ControllerTestFixture, E2ETest) {
    // Иван регистрируется
    auto result1 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/register",
        makeJson(
            {{"handle", "Ivan"},
             {"password", "secret123"},
             {"display_name", "Ivan"}}
        )
    )));

    ASSERT_EQ(result1->getStatusCode(), k201Created);

    // Иван пытается войти в аккаунт, но неправильно вводит пароль
    auto result2 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/login",
        makeJson({
            {"handle", "Ivan"},
            {"password", "secret12"},
        })
    )));

    ASSERT_EQ(result2->getStatusCode(), k401Unauthorized);

    // Иван входит в аккаунт
    auto result3 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/login",
        makeJson({
            {"handle", "Ivan"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result3->getStatusCode(), k200OK);
    std::string ivan_token = (*result3->getJsonObject())["token"].asString();

    // Получаем айди Ивана
    auto result4 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/users/handle/Ivan",
        makeJson({

        })
    )));

    ASSERT_EQ(result4->getStatusCode(), k200OK);
    int64_t ivan_id = (*result4->getJsonObject())["id"].asInt64();

    // Пётр регистрируется
    auto result5 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/register",
        makeJson(
            {{"handle", "Petr"},
             {"password", "secret123"},
             {"display_name", "Petr"}}
        )
    )));

    ASSERT_EQ(result5->getStatusCode(), k201Created);

    // Пётр входит в аккаунт
    auto result6 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/login",
        makeJson({
            {"handle", "Petr"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result6->getStatusCode(), k200OK);
    std::string petr_token = (*result6->getJsonObject())["token"].asString();

    // Получаем айди Петра
    auto result7 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/users/handle/Petr",
        makeJson({

        })
    )));

    ASSERT_EQ(result7->getStatusCode(), k200OK);
    int64_t petr_id = (*result7->getJsonObject())["id"].asInt64();

    // Дима регистрируется
    auto result8 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/register",
        makeJson(
            {{"handle", "Dima"},
             {"password", "secret123"},
             {"display_name", "Dima"}}
        )
    )));

    ASSERT_EQ(result8->getStatusCode(), k201Created);

    // Дима входит в аккаунт
    auto result9 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/auth/login",
        makeJson({
            {"handle", "Dima"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result9->getStatusCode(), k200OK);
    std::string dima_token = (*result9->getJsonObject())["token"].asString();

    // Получаем айди Димы
    auto result10 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/users/handle/Dima",
        makeJson({

        })
    )));

    ASSERT_EQ(result10->getStatusCode(), k200OK);
    int64_t dima_id = (*result10->getJsonObject())["id"].asInt64();

    // Иван смотрит свои чаты - там только сохранёнки
    auto result11 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/chats/user/" + std::to_string(ivan_id), makeJson({}),
        ivan_token
    )));

    ASSERT_EQ(result11->getStatusCode(), k200OK);
    ASSERT_EQ((*result11->getJsonObject())["chats"].size(), 1);

    // Иван создаёт личный чат с Петром
    auto result12 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/chats/direct", makeJson({{"target_user_id", petr_id}}),
        ivan_token
    )));

    ASSERT_EQ(result12->getStatusCode(), k201Created);

    // Пётр пытает создать личный чат с Иваном, но он уже есть, просто получает
    auto result13 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/chats/direct", makeJson({{"target_user_id", ivan_id}}),
        petr_token
    )));

    ASSERT_EQ(result13->getStatusCode(), k200OK);

    // Дима создаёт личный чат с Петром
    auto result14 = sync_wait(sendReqTask(form_request(
        Post, "/api/v1/chats/direct", makeJson({{"target_user_id", petr_id}}),
        dima_token
    )));

    ASSERT_EQ(result14->getStatusCode(), k201Created);

    // Дима получает свои чаты: там сохранённые и чат с Петром
    auto result15 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/chats/user/" + std::to_string(dima_id), makeJson({}),
        dima_token
    )));

    ASSERT_EQ(result15->getStatusCode(), k200OK);
    Json::Value dima_chats = (*result15->getJsonObject())["chats"];
    ASSERT_EQ(dima_chats.size(), 2);
    int64_t direct_chat_dima_petr_id = (*std::find_if(
        dima_chats.begin(), dima_chats.end(),
        [](const Json::Value &e) {
            return e["type"].asString() == messenger::models::ChatType::Direct;
        }
    ))["chat_id"]
                                           .asInt64();

    // Дима отправляет Пете сообщение
    auto result16 = sync_wait(sendReqTask(form_request(
        Post,
        "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages",
        makeJson({{"text", "Прив ну чё там с проектом"}}), dima_token
    )));

    ASSERT_EQ(result16->getStatusCode(), k201Created);
    ASSERT_EQ(
        (*result16->getJsonObject())["message"]["text"].asString(),
        "Прив ну чё там с проектом"
    );

    // Петя отправляет Диме сообщение
    auto result17 = sync_wait(sendReqTask(form_request(
        Post,
        "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages",
        makeJson({{"text", "Работаем"}}), petr_token
    )));

    ASSERT_EQ(result17->getStatusCode(), k201Created);

    // Дима снова получает свои чаты: там сохранённые и чат с Петром
    auto result18 = sync_wait(sendReqTask(form_request(
        Get, "/api/v1/chats/user/" + std::to_string(dima_id), makeJson({}),
        dima_token
    )));

    ASSERT_EQ(result18->getStatusCode(), k200OK);
    dima_chats = (*result18->getJsonObject())["chats"];
    ASSERT_EQ(dima_chats.size(), 2);
    auto direct_chat_dima_petr = (*std::find_if(
        dima_chats.begin(), dima_chats.end(),
        [](const Json::Value &e) {
            return e["type"].asString() == messenger::models::ChatType::Direct;
        }
    ));
    ASSERT_EQ(direct_chat_dima_petr["unread_count"].asInt64(), 1);

    // Иван пытается отправить сообщение в чужой чат, не может
    auto result19 = sync_wait(sendReqTask(form_request(
        Post,
        "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages",
        makeJson({{"text", "Всем привет"}}), ivan_token
    )));

    ASSERT_EQ(result19->getStatusCode(), k403Forbidden);

    // Дима получает историю сообщений чата с Петей
    auto result20 = sync_wait(sendReqTask(form_request(
        Get,
        "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages",
        makeJson({}), dima_token
    )));

    ASSERT_EQ(result20->getStatusCode(), k200OK);

    auto direct_chat_dima_petr_messages =
        (*result20->getJsonObject())["messages"];
    ASSERT_EQ(direct_chat_dima_petr_messages.size(), 2);

    // Дима получает историю сообщений чата с Петей, но с before_id

    // Отправляем 100 сообщений
    for (int i = 0; i < 100; i++) {
        auto result = sync_wait(sendReqTask(form_request(
            Post,
            "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
                "/messages",
            makeJson({{"text", "Работаем"}}), petr_token
        )));

        ASSERT_EQ(result->getStatusCode(), k201Created);
    }
    // Проверяем, что можем достать старые
    auto result21 = sync_wait(sendReqTask(form_request(
        Get,
        "/api/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages",
        makeJson(
            {{"before_id",
              (*result17->getJsonObject())["message"]["id"].asInt64()}}
        ),
        dima_token
    )));

    ASSERT_EQ(result21->getStatusCode(), k200OK);

    auto direct_chat_dima_petr_messages_with_before_id =
        (*result21->getJsonObject())["messages"];
    ASSERT_EQ(direct_chat_dima_petr_messages_with_before_id.size(), 2);
}