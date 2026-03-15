/**
 *
 *  api_v1_IpFilter.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <chrono>
#include <shared_mutex>
#include <unordered_set>
using namespace drogon;
namespace api
{
namespace v1
{

struct ClientRequestsCounter{
  int requests_count = 0;
  std::chrono::steady_clock::time_point window_start;
};

class IpFilter : public HttpFilter<IpFilter>
{
  public:
    IpFilter();
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

  private:
    static std::unordered_map<uint32_t, ClientRequestsCounter> clients_;
    static std::shared_mutex mutex_;

    std::unordered_set<uint32_t> whitelist_ips_;

    void cleanUpOldClients();

    const int MAX_REQUESTS = std::stoi(std::getenv("SERVER_MAX_REQUESTS_PER_WINDOW"));
    const int WINDOW_SECONDS = std::stoi(std::getenv("SERVER_WINDOW_FOR_REQUESTS_SECONDS"));
};

}
}
