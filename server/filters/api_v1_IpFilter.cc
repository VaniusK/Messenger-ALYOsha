/**
 *
 *  api_v1_IpFilter.cc
 *
 */

#include "api_v1_IpFilter.h"
#include <drogon/HttpAppFramework.h>
#include <json/value.h>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <json/value.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace drogon;
using namespace api::v1;

std::unordered_map<uint32_t, ClientRequestsCounter> IpFilter::clients_;
std::shared_mutex IpFilter::mutex_;

IpFilter::IpFilter() {
    drogon::app().getLoop()->runEvery(
        std::stoi(std::getenv("SERVER_IP_LIST_CLEANING_SECONDS_COOLDOWN")),
        [this]() { this->cleanUpOldClients(); }
    );
    auto custom_config = drogon::app().getCustomConfig();
    if (custom_config.isMember("whitelist_ips") && custom_config["whitelist_ips"].isArray()) {
        auto ips_list = custom_config["whitelist_ips"];
        for (auto &ip_val : ips_list){
            std::string ip_str = ip_val.asString();
            struct in_addr addr;
            if (inet_pton(AF_INET, ip_str.c_str(), &addr) == 1){
                whitelist_ips_.insert(addr.s_addr);
                LOG_INFO << "Added new ip to whitelist: " << ip_str;
            }
            else{
                LOG_INFO << "Failed to add ip to whitelist: " << ip_str;
            }
        }
    }
}

void IpFilter::cleanUpOldClients() {
    auto now = std::chrono::steady_clock::now();

    std::unique_lock<std::shared_mutex> lock(mutex_);

    std::size_t size_before = clients_.size();
    std::erase_if(clients_, [now, this](const auto &item) {
        auto [ip, client] = item;
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - client.window_start
        );

        return time_diff > std::chrono::seconds(WINDOW_SECONDS);
    });
    LOG_INFO << "Cleaned " << (size_before - clients_.size()) << " old ips.";
}

void IpFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    uint32_t ip = req->peerAddr().ipNetEndian();
    if (ip == 0){
        LOG_WARN << "Failed to get ip-address from request";
        Json::Value response_json;
        response_json["message"] = "Bad ip";
        auto response = HttpResponse::newHttpJsonResponse(response_json);
        response->setStatusCode(drogon::k400BadRequest);
        fcb(response);
        return;
    }
    if (whitelist_ips_.count(ip) > 0) {
        fccb();
        return;
    }
    auto now = std::chrono::steady_clock::now();
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto &client = clients_[ip];
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - client.window_start
        );
        if (time_diff > std::chrono::seconds(WINDOW_SECONDS)) {
            client.requests_count = 1;
            client.window_start = now;
        } else {
            client.requests_count++;
            if (client.requests_count > MAX_REQUESTS) {
                Json::Value response_json;
                response_json["message"] = "Too many requests";
                LOG_WARN << "Too many requests from " << ip;
                auto response =
                    HttpResponse::newHttpJsonResponse(response_json);
                response->setStatusCode(k429TooManyRequests);
                fcb(response);
                return;
            }
        }
        fccb();
    }
}
