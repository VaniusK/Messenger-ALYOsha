/**
 *
 *  api_v1_AuthFilter.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;
namespace api
{
namespace v1
{

class AuthFilter : public HttpFilter<AuthFilter>
{
  public:
    AuthFilter() {}
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};

}
}
