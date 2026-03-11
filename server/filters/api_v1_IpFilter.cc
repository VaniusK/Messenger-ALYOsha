/**
 *
 *  api_v1_IpFilter.cc
 *
 */

#include "api_v1_IpFilter.h"
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <json/value.h>

using namespace drogon;
using namespace api::v1;

std::unordered_map<uint32_t, ClientRequestsCounter> IpFilter::clients_;
std::shared_mutex IpFilter::mutex_;

IpFilter::IpFilter(){
    drogon::app().getLoop()->runEvery(std::stoi(std::getenv("SERVER_IP_LIST_CLEANING_SECONDS_COOLDOWN")), [this](){
        this->cleanUpOldClients();
    });
}

void IpFilter::cleanUpOldClients(){
    auto now = std::chrono::steady_clock::now();

    std::unique_lock<std::shared_mutex> lock(mutex_);

    std::size_t size_before = clients_.size();
    std::erase_if(clients_, [now, this](const auto& item){
        auto [ip, client] = item;
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.window_start);

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
    auto now = std::chrono::steady_clock::now();
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto &client = clients_[ip];
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.window_start);
        if (time_diff > std::chrono::seconds(WINDOW_SECONDS)){
            client.requests_count = 1;
            client.window_start = now;
        }
        else{
            client.requests_count++;
            if (client.requests_count > MAX_REQUESTS){
                Json::Value response_json;
                response_json["message"] = "Too many requests";
                LOG_WARN << "Too many requests from " << ip;
                auto response = HttpResponse::newHttpJsonResponse(response_json);
                response->setStatusCode(k429TooManyRequests);
                fcb(response);
                return;
            } 
        }
        fccb();
    }
}
