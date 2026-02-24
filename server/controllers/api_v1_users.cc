#include "api_v1_users.h"
#include <drogon/drogon.h>

using namespace api::v1;

Task<HttpResponsePtr> users::getUserById(const HttpRequestPtr req, std::string &&user_id){
    // UserRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = user_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> users::getUserByHandle(const HttpRequestPtr req, std::string &&user_handle){
    // UserRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = user_handle;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> users::searchUser(const HttpRequestPtr req){
    // UserRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = "ok";
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}   
