/**
 *
 *  api_v1_JsonValidatorFilter.cc
 *
 */

#include "api_v1_JsonValidatorFilter.h"
#include <drogon/HttpTypes.h>
#include <json/value.h>
#include "utils/server_response_macro.hpp"

using namespace drogon;
using namespace api::v1;

void JsonValidatorFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    auto reqJson = req->getJsonObject();
    if (reqJson)
    {
        LOG_INFO << "Json body is valid";
        fccb();
        return;
    }
    Json::Value resp_json;
    resp_json["message"] = "Invalid request: missed json body";
    auto res = drogon::HttpResponse::newHttpJsonResponse(resp_json);
    res->setStatusCode(drogon::k400BadRequest);
    LOG_WARN << "Invalid request: missed json body";
    fcb(res);
}
