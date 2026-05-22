#pragma once
#include <arpa/inet.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gtest/gtest.h>
#include <netdb.h>
#include <fstream>

class ControllerTestFixture : public ::testing::Test {
protected:
    drogon::HttpClientPtr client_;
    drogon::HttpClientPtr minio_client_;

    inline Json::Value makeJson(
        std::vector<std::pair<std::string, Json::Value>> &&fields
    ) {
        Json::Value j;
        for (auto &[field, value] : fields) {
            j[field] = value;
        }
        return j;
    }

    void parse_path_with_params(drogon::HttpRequestPtr &req, std::string url) {
        std::string path = url.substr(0, url.find("?"));
        std::string params =
            url.substr(url.find("?") + 1, url.size() - url.find("?") - 1);
        req->setPath(path);
        std::string param;
        std::string value;
        bool is_value = false;
        for (char i : params) {
            if (i == '&') {
                req->setParameter(param, drogon::utils::urlDecode(value));
                param = "";
                value = "";
                is_value = false;
                continue;
            }
            if (i == '=') {
                is_value = true;
                continue;
            }
            if (!is_value) {
                param += i;
            } else {
                value += i;
            }
        }
        if (param.size() > 0) {
            req->setParameter(param, drogon::utils::urlDecode(value));
        }
    }

    drogon::HttpRequestPtr form_request(
        drogon::HttpMethod method,
        std::string url,
        Json::Value body,
        std::string token = ""
    ) {
        auto req = drogon::HttpRequest::newHttpJsonRequest(body);
        req->setMethod(method);
        parse_path_with_params(req, url);
        req->addHeader("Authorization", "Bearer " + token);
        return req;
    }

    drogon::HttpRequestPtr
    form_file_download_request(drogon::HttpMethod method, std::string url) {
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(method);
        parse_path_with_params(req, url);
        return req;
    }

    drogon::HttpRequestPtr form_file_upload_request(
        const std::string &presigned_url,
        const std::string &file_path
    ) {
        std::ifstream file(file_path, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string file_content = buffer.str();
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Put);
        parse_path_with_params(req, presigned_url);
        req->setBody(std::move(file_content));
        req->setContentTypeCode(drogon::CT_IMAGE_PNG);

        return req;
    }

    drogon::Task<drogon::HttpResponsePtr> sendReqTask(drogon::HttpRequestPtr req
    ) {
        co_return co_await client_->sendRequestCoro(req);
    }

    drogon::Task<drogon::HttpResponsePtr> sendMinioReqTask(
        drogon::HttpRequestPtr req
    ) {
        co_return co_await minio_client_->sendRequestCoro(req);
    }

public:
    void SetUp() override {
        client_ = drogon::HttpClient::newHttpClient(
            "http://127.0.0.1:5555", drogon::app().getLoop()
        );

        std::string minio_url = "http://minio:9000";
        struct hostent *he = gethostbyname("minio");
        if (he != nullptr && he->h_addr_list[0] != nullptr) {
            struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
            std::string minio_ip = inet_ntoa(*addr_list[0]);
            minio_url = "http://" + minio_ip + ":9000";
            std::cout << "Resolved minio to " << minio_ip << std::endl;
        }

        minio_client_ = drogon::HttpClient::newHttpClient(
            minio_url, drogon::app().getLoop()
        );
    }
};