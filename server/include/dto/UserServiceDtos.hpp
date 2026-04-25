#pragma once
#include <drogon/HttpRequest.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <vector>
#include "dto/ServicesDtoBase.hpp"
#include "models/Users.h"

using User = drogon_model::messenger_db::Users;

namespace messenger::dto {

struct SearchUserRequestDto : RequestDto {
    std::string query;
    int64_t limit;

    SearchUserRequestDto(drogon::HttpRequestPtr req) {
        query = req->getParameter("query");
        try {
            limit = std::stoll(req->getParameter("limit"));
            limit = std::clamp<int64_t>(limit, 1, 100);
        } catch (...) {
            limit = 20;
        }
    }
};

struct SearchUserResponseDto : ResponseDto {
    std::vector<User> users;

    SearchUserResponseDto(std::vector<User> &&users_)
        : users(std::move(users_)) {
    }

    Json::Value toJson() override {
        Json::Value jsonArray(Json::arrayValue);
        for (const auto &user : users) {
            Json::Value user_json = user.toJson();
            user_json.removeMember("password_hash");
            jsonArray.append(user_json);
        }
        return jsonArray;
    }
};

struct GetUserResponseDto : ResponseDto {
    User user;

    GetUserResponseDto(User &&user_) : user(std::move(user_)) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        response_json = user.toJson();
        response_json.removeMember("password_hash");
        return response_json;
    }
};

struct RegisterUserRequestDto : RequestDto {
    std::string handle;
    std::string password;
    std::string display_name;

    RegisterUserRequestDto(std::shared_ptr<Json::Value> request_json) {
        handle = (*request_json)["handle"].asString();
        password = (*request_json)["password"].asString();
        display_name = (*request_json)["display_name"].asString();
    }
};

struct RegisterUserResponseDto : ResponseDto {
    Json::Value toJson() override {
        Json::Value response_json;
        response_json["message"] = "New user was successfully created";
        return response_json;
    }
};

struct LoginUserRequestDto : RequestDto {
    std::string handle;
    std::string password;

    LoginUserRequestDto(std::shared_ptr<Json::Value> request_json) {
        handle = (*request_json)["handle"].asString();
        password = (*request_json)["password"].asString();
    }
};

struct LoginUserResponseDto : ResponseDto {
    std::string token;

    LoginUserResponseDto(std::string &&token_) : token(token_) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        response_json["message"] = "Login successful";
        response_json["token"] = std::move(token);
        return response_json;
    }
};

}  // namespace messenger::dto
