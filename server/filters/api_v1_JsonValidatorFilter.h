/**
 *
 *  api_v1_JsonValidatorFilter.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;
namespace api
{
namespace v1
{

class JsonValidatorFilter : public HttpFilter<JsonValidatorFilter>
{
  public:
    JsonValidatorFilter() {}
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};

}
}
