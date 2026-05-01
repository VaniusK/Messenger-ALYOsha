#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <cstdlib>
#include <memory>
#include <string>
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
    std::cout << "Executing result"
              << "\n";
    auto result1 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/register",
        makeJson(
            {{"handle", "Ivan"},
             {"password", "secret123"},
             {"display_name", "Ivan"}}
        )
    )));

    ASSERT_EQ(result1->getStatusCode(), k201Created);

    // Иван пытается войти в аккаунт, но неправильно вводит пароль
    std::cout << "Executing result"
              << "\n";
    auto result2 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/login",
        makeJson({
            {"handle", "Ivan"},
            {"password", "secret12"},
        })
    )));

    ASSERT_EQ(result2->getStatusCode(), k401Unauthorized);

    // Иван входит в аккаунт
    std::cout << "Executing result"
              << "\n";
    auto result3 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/login",
        makeJson({
            {"handle", "Ivan"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result3->getStatusCode(), k200OK);
    std::string ivan_token = (*result3->getJsonObject())["token"].asString();

    // Получаем айди Ивана
    std::cout << "Executing result"
              << "\n";
    auto result4 = sync_wait(sendReqTask(form_request(
        Get, "/v1/users/handle/Ivan",
        makeJson({

        })
    )));

    ASSERT_EQ(result4->getStatusCode(), k200OK);
    int64_t ivan_id = (*result4->getJsonObject())["id"].asInt64();

    // Пётр регистрируется
    std::cout << "Executing result"
              << "\n";
    auto result5 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/register",
        makeJson(
            {{"handle", "Petr"},
             {"password", "secret123"},
             {"display_name", "Petr"}}
        )
    )));

    ASSERT_EQ(result5->getStatusCode(), k201Created);

    // Пётр входит в аккаунт
    std::cout << "Executing result"
              << "\n";
    auto result6 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/login",
        makeJson({
            {"handle", "Petr"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result6->getStatusCode(), k200OK);
    std::string petr_token = (*result6->getJsonObject())["token"].asString();

    // Получаем айди Петра
    std::cout << "Executing result"
              << "\n";
    auto result7 = sync_wait(sendReqTask(form_request(
        Get, "/v1/users/handle/Petr",
        makeJson({

        })
    )));

    ASSERT_EQ(result7->getStatusCode(), k200OK);
    int64_t petr_id = (*result7->getJsonObject())["id"].asInt64();

    // Дима регистрируется
    std::cout << "Executing result"
              << "\n";
    auto result8 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/register",
        makeJson(
            {{"handle", "Dima"},
             {"password", "secret123"},
             {"display_name", "Dima"}}
        )
    )));

    ASSERT_EQ(result8->getStatusCode(), k201Created);

    // Дима входит в аккаунт
    std::cout << "Executing result"
              << "\n";
    auto result9 = sync_wait(sendReqTask(form_request(
        Post, "/v1/auth/login",
        makeJson({
            {"handle", "Dima"},
            {"password", "secret123"},
        })
    )));

    ASSERT_EQ(result9->getStatusCode(), k200OK);
    std::string dima_token = (*result9->getJsonObject())["token"].asString();

    // Получаем айди Димы
    std::cout << "Executing result"
              << "\n";
    auto result10 = sync_wait(sendReqTask(form_request(
        Get, "/v1/users/handle/Dima",
        makeJson({

        })
    )));

    ASSERT_EQ(result10->getStatusCode(), k200OK);
    int64_t dima_id = (*result10->getJsonObject())["id"].asInt64();

    // Иван смотрит свои чаты - там только сохранёнки
    std::cout << "Executing result"
              << "\n";
    auto result11 = sync_wait(sendReqTask(form_request(
        Get, "/v1/chats/user/" + std::to_string(ivan_id), makeJson({}),
        ivan_token
    )));

    ASSERT_EQ(result11->getStatusCode(), k200OK);
    ASSERT_EQ((*result11->getJsonObject())["chats"].size(), 1);

    // Иван создаёт личный чат с Петром
    std::cout << "Executing result"
              << "\n";
    auto result12 = sync_wait(sendReqTask(form_request(
        Post, "/v1/chats/direct", makeJson({{"target_user_id", petr_id}}),
        ivan_token
    )));

    ASSERT_EQ(result12->getStatusCode(), k201Created);

    // Пётр пытает создать личный чат с Иваном, но он уже есть, просто получает
    std::cout << "Executing result"
              << "\n";
    auto result13 = sync_wait(sendReqTask(form_request(
        Post, "/v1/chats/direct", makeJson({{"target_user_id", ivan_id}}),
        petr_token
    )));

    ASSERT_EQ(result13->getStatusCode(), k200OK);

    // Дима создаёт личный чат с Петром
    std::cout << "Executing result"
              << "\n";
    auto result14 = sync_wait(sendReqTask(form_request(
        Post, "/v1/chats/direct", makeJson({{"target_user_id", petr_id}}),
        dima_token
    )));

    ASSERT_EQ(result14->getStatusCode(), k201Created);

    // Дима получает свои чаты: там сохранённые и чат с Петром
    std::cout << "Executing result"
              << "\n";
    auto result15 = sync_wait(sendReqTask(form_request(
        Get, "/v1/chats/user/" + std::to_string(dima_id), makeJson({}),
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
    std::cout << "Executing result"
              << "\n";
    auto result16 = sync_wait(sendReqTask(form_request(
        Post,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson({{"text", "Прив ну чё там с проектом"}, {"type", "text"}}),
        dima_token
    )));

    ASSERT_EQ(result16->getStatusCode(), k201Created);
    ASSERT_EQ(
        (*result16->getJsonObject())["message"]["text"].asString(),
        "Прив ну чё там с проектом"
    );

    // Петя отправляет Диме сообщение
    std::cout << "Executing result"
              << "\n";
    auto result17 = sync_wait(sendReqTask(form_request(
        Post,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson({{"text", "Работаем"}, {"type", "text"}}), petr_token
    )));

    ASSERT_EQ(result17->getStatusCode(), k201Created);

    // Дима снова получает свои чаты: там сохранённые и чат с Петром
    std::cout << "Executing result"
              << "\n";
    auto result18 = sync_wait(sendReqTask(form_request(
        Get, "/v1/chats/user/" + std::to_string(dima_id), makeJson({}),
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
    std::cout << "Executing result"
              << "\n";
    auto result19 = sync_wait(sendReqTask(form_request(
        Post,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson({{"text", "Всем привет"}, {"type", "text"}}), ivan_token
    )));

    ASSERT_EQ(result19->getStatusCode(), k403Forbidden);

    // Дима получает историю сообщений чата с Петей
    std::cout << "Executing result"
              << "\n";
    auto result20 = sync_wait(sendReqTask(form_request(
        Get,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson({}), dima_token
    )));

    ASSERT_EQ(result20->getStatusCode(), k200OK);

    auto direct_chat_dima_petr_messages =
        (*result20->getJsonObject())["messages"];
    ASSERT_EQ(direct_chat_dima_petr_messages.size(), 2);

    // Дима получает историю сообщений чата с Петей, но с before_id

    // Отправляем 100 сообщений
    for (int i = 0; i < 100; i++) {
        std::cout << "Executing result"
                  << "\n";
        auto result = sync_wait(sendReqTask(form_request(
            Post,
            "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
                "/messages",
            makeJson({{"text", "Работаем"}, {"type", "text"}}), petr_token
        )));

        ASSERT_EQ(result->getStatusCode(), k201Created);
    }
    // Проверяем, что можем достать старые
    std::cout << "Executing result"
              << "\n";
    auto result21 = sync_wait(sendReqTask(form_request(
        Get,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) +
            "/messages?before_id=" +
            std::to_string(
                (*result17->getJsonObject())["message"]["id"].asInt64()
            ),
        makeJson({}), dima_token
    )));

    ASSERT_EQ(result21->getStatusCode(), k200OK);

    auto direct_chat_dima_petr_messages_with_before_id =
        (*result21->getJsonObject())["messages"];
    ASSERT_EQ(direct_chat_dima_petr_messages_with_before_id.size(), 1);

    // Петя получает ссылку для загрузки картинки
    Json::Value file_obj = makeJson(
        {{"chat_id", direct_chat_dima_petr_id},
         {"original_filename", "eggplant.png"},
         {"file_size_bytes", 1337},
         {"upload_as_file", false}}
    );
    Json::Value files_arr(Json::arrayValue);
    files_arr.append(file_obj);
    auto result22 = sync_wait(sendReqTask(form_request(
        Post, "/v1/chats/attachments/presigned-links",
        makeJson(
            {{"message_type", "text"},
             {"chat_id", direct_chat_dima_petr_id},
             {"files", files_arr}}
        ),
        petr_token
    )));

    ASSERT_EQ(result22->getStatusCode(), k200OK);

    std::string eggplant_token =
        (*result22->getJsonObject())["attachments"][0]["token"].asString();

    std::string eggplant_upload_url =
        (*result22->getJsonObject())["attachments"][0]["upload_url"].asString();

    // Обрезаем ссылку безопасно, находя первый слеш после http://
    size_t upload_path_pos = eggplant_upload_url.find("/", 8);
    ASSERT_NE(upload_path_pos, std::string::npos);
    eggplant_upload_url = eggplant_upload_url.substr(upload_path_pos);
    auto eggplant_attachment_key =
        (*result22->getJsonObject())["attachment_key"].asString();
    auto eggplant_content_type =
        (*result22->getJsonObject())["content_type"].asString();

    // Петя загружает картинку
    std::cout << "Executing result"
              << "\n";
    auto result23 = sync_wait(sendMinioReqTask(form_file_upload_request(
        eggplant_upload_url, "tests/test_data/eggplant.png"
    )));

    ASSERT_EQ(result23->getStatusCode(), k200OK);

    // Петя отправляет Диме сообщение с картинкой

    Json::Value tokens_arr(Json::arrayValue);
    tokens_arr.append(eggplant_token);
    auto result24 = sync_wait(sendReqTask(form_request(
        Post,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson(
            {{"text", "Смотри на баклажан"},
             {"type", "text"},
             {"attachment_tokens", tokens_arr}}
        ),
        petr_token
    )));

    ASSERT_EQ(result24->getStatusCode(), k201Created);

    int64_t eggplant_message_id =
        (*result24->getJsonObject())["message"]["id"].asInt64();

    // Дима получает сообщения чата - у последнего должна быть картинка
    std::cout << "Executing result"
              << "\n";
    auto result25 = sync_wait(sendReqTask(form_request(
        Get,
        "/v1/chats/" + std::to_string(direct_chat_dima_petr_id) + "/messages",
        makeJson({{"limit", 1}}), dima_token
    )));

    ASSERT_EQ(result25->getStatusCode(), k200OK);
    ASSERT_EQ(
        (*result25->getJsonObject())["messages"][0]["attachments"].size(), 1
    );

    std::string eggplant_download_url = (*result25->getJsonObject()
    )["messages"][0]["attachments"][0]["download_url"]
                                            .asString();

    size_t download_path_pos = eggplant_download_url.find("/", 8);
    ASSERT_NE(download_path_pos, std::string::npos);
    eggplant_download_url = eggplant_download_url.substr(download_path_pos);

    // Проверяем, что ссылка рабочая(проверка на идентичность файла/подобное уже
    // будут борьбой с клиентом дрогона. Предполагаем, что херобрин не может
    // прилететь и заменить баклажан по ссылке на арбуз.)
    std::cout << "Executing result"
              << "\n";
    auto result26 = sync_wait(
        sendMinioReqTask(form_file_download_request(Get, eggplant_download_url))
    );

    ASSERT_EQ(result26->getStatusCode(), k200OK);
}