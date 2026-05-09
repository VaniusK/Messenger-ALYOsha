#pragma once
#include <json/value.h>

namespace messenger::dto {

struct ResponseDto {
    virtual Json::Value toJson() = 0;
    virtual ~ResponseDto() = default;
};

struct RequestDto {};

}  // namespace messenger::dto
