#include "services/UserService.hpp"
#include <drogon/HttpController.h>
#include <json/value.h>
#include <jwt-cpp/jwt.h>
#include <bcrypt/BCrypt.hpp>
#include <chrono>
#include <optional>
#include "models/Users.h"
#include "repositories/UserRepository.hpp"
#include "utils/server_response_macro.hpp"

using namespace drogon;
using namespace api::v1;

using UserRepo = messenger::repositories::UserRepository;
using User = drogon_model::messenger_db::Users;

Task<HttpResponsePtr> UserService::registerUser(
    const std::shared_ptr<Json::Value> request_json
) {
    Json::Value response_json;
    std::optional<User> user;
    try {
        user =
            co_await user_repo->getByHandle((*request_json)["handle"].asString()
            );
    } catch (std::exception &e) {
        LOG_WARN << "Couldnt't get user by handle: " << e.what();
        response_json["message"] =
            "Internal server error: failed to check user exists";
        RETURN_RESPONSE_CODE_500(response_json)
    }

    if (user == std::nullopt) {
        std::string password_hash =
            BCrypt::generateHash((*request_json)["password"].asString(), 10);
        bool success = co_await user_repo->create(
            (*request_json)["handle"].asCString(),
            (*request_json)["display_name"].asCString(), password_hash
        );
        if (success) {
            response_json["message"] = "New user was successfully created";
            RETURN_RESPONSE_CODE_201(response_json)
        } else {
            LOG_WARN << "User wasn't created";
            response_json["message"] =
                "Internal server error: user wasn't created";
            RETURN_RESPONSE_CODE_500(response_json)
        }
    } else {
        response_json["message"] = "User already exists";
        RETURN_RESPONSE_CODE_409(response_json)
    }
}

Task<HttpResponsePtr> UserService::loginUser(
    const std::shared_ptr<Json::Value> request_json
) {
    Json::Value response_json;
    std::optional<User> user;
    try {
        user =
            co_await user_repo->getByHandle((*request_json)["handle"].asString()
            );
    } catch (std::exception &e) {
        LOG_WARN << "Couldnt't get user by handle: " << e.what();
        response_json["message"] = "Internal server error: user wasn't found";
        RETURN_RESPONSE_CODE_500(response_json)
    }
    if (user == std::nullopt) {
        response_json["message"] = "Login error: Invalid handle or password";
        RETURN_RESPONSE_CODE_401(response_json)
    } else {
        if (!BCrypt::validatePassword(
                std::string((*request_json)["password"].asCString()),
                user->getValueOfPasswordHash()
            )) {
            response_json["message"] =
                "Login error: Invalid handle or password";
            RETURN_RESPONSE_CODE_401(response_json)
        }
        const std::string JWT_KEY = std::getenv("JWT_KEY");
        std::string token =
            jwt::create()
                .set_issuer("alesha_messenger")
                .set_type("JWT")
                .set_issued_at(std::chrono::system_clock::now())
                //.set_expires_at(std::chrono::system_clock.now() +
                // std::chrono::hours(24)) // uncomment when we have refresh
                // tokens
                .set_payload_claim(
                    "user_id", jwt::claim(std::to_string(user->getValueOfId()))
                )
                .set_payload_claim(
                    "handle", jwt::claim(user->getValueOfHandle())
                )
                .sign(jwt::algorithm::hs256(JWT_KEY));
        response_json["message"] = "Login successful";
        response_json["token"] = token;
        RETURN_RESPONSE_CODE_200(response_json)
    }
}

Task<HttpResponsePtr> UserService::getUserById(int64_t user_id) {
    Json::Value response_json;
    std::optional<User> user;
    try {
        user = co_await user_repo->getById(user_id);
    } catch (std::exception &e) {
        LOG_WARN << "Couldnt't get user by id: " << e.what();
        response_json["message"] = "Internal server error: failed to get user";
        RETURN_RESPONSE_CODE_500(response_json)
    }
    if (!user.has_value()) {
        response_json["message"] =
            "User with id " + std::to_string(user_id) + " doesn't exist";
        RETURN_RESPONSE_CODE_404(response_json)
    } else {
        response_json = user->toJson();
        response_json.removeMember("password_hash");
        RETURN_RESPONSE_CODE_200(response_json)
    }
}

Task<HttpResponsePtr> UserService::getUserByHandle(std::string &&user_handle) {
    Json::Value response_json;
    std::optional<User> user;
    try {
        user = co_await user_repo->getByHandle(user_handle);
    } catch (std::exception &e) {
        LOG_WARN << "Couldnt't get user by handle: " << e.what();
        response_json["message"] = "Internal server error: failed to get user";
        RETURN_RESPONSE_CODE_500(response_json)
    }
    if (!user.has_value()) {
        response_json["message"] =
            "User with handle " + std::string(user_handle) + " doesn't exist";
        RETURN_RESPONSE_CODE_404(response_json)
    } else {
        response_json = user->toJson();
        response_json.removeMember("password_hash");
        RETURN_RESPONSE_CODE_200(response_json)
    }
}

Task<HttpResponsePtr> UserService::searchUser(
    const std::shared_ptr<Json::Value> request_json
) {
    Json::Value response_json;
    std::vector<User> users;
    try {
        users = co_await user_repo->search(
            (*request_json)["query"].asString(),
            (*request_json)["limit"].asInt64()
        );
    } catch (std::exception &e) {
        LOG_WARN << "Failed to search: " << e.what();
        response_json["message"] =
            "Internal server error: failed to search users";
        RETURN_RESPONSE_CODE_500(response_json)
    }
    Json::Value jsonArray(Json::arrayValue);
    for (const auto &user : users) {
        Json::Value user_json = user.toJson();
        user_json.removeMember("password_hash");
        jsonArray.append(user_json);
    }
    response_json["results"] = jsonArray;
    RETURN_RESPONSE_CODE_200(response_json)
}
