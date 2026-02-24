/**
 *
 *  api_v1_AuthFilter.cc
 *
 */

#include "api_v1_AuthFilter.h"
#include <jwt-cpp/jwt.h>
#include <iostream>

using namespace drogon;
using namespace api::v1;

void AuthFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    auto header = req->getHeader("Authorization");
    try{
        std::string token = header.substr(7);
        auto decoded = jwt::decode(token);
        LOG_INFO << "Token decoded";
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256(std::getenv("JWT_KEY")))
            .with_issuer("alesha_messenger");
        verifier.verify(decoded);
        LOG_INFO << "Token verified";
        std::string user_id = decoded.get_payload_claim("user_id").as_string();
        LOG_INFO << "Successfully got user_id " << user_id << " from token";
        req->getAttributes()->insert("user_id", user_id);
        fccb();
    }
    catch(const std::exception &e){
        Json::Value response_json;
        LOG_WARN << "Accession failed: " << e.what();
        response_json["message"] = "Accession failed: Invalid token";
        auto res = drogon::HttpResponse::newHttpJsonResponse(response_json);
        res->setStatusCode(k401Unauthorized);
        fcb(res);
    }
}
