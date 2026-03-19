#pragma once
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gtest/gtest.h>

class ControllerTestFixture : public ::testing::Test {
protected:
    drogon::HttpClientPtr client_;

    inline Json::Value makeJson(
        std::vector<std::pair<std::string, Json::Value>> &&fields
    ) {
        Json::Value j;
        for (auto &[field, value] : fields) {
            j[field] = value;
        }
        return j;
    }

    drogon::HttpRequestPtr form_request(
        drogon::HttpMethod method,
        std::string url,
        Json::Value body,
        std::string token = ""
    ) {
        auto req = drogon::HttpRequest::newHttpJsonRequest(body);
        req->setMethod(method);
        req->setPath(url);
        req->addHeader("Authorization", "Bearer " + token);
        return req;
    }

    drogon::Task<drogon::HttpResponsePtr> sendReqTask(drogon::HttpRequestPtr req
    ) {
        co_return co_await client_->sendRequestCoro(req);
    }

public:
    void SetUp() override {
        client_ = drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:5555", drogon::app().getLoop()
        );
    }
};