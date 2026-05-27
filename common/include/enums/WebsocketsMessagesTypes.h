#pragma once

#include <cstdint>

#ifndef Q_NAMESPACE
#define Q_NAMESPACE
#define Q_ENUM_NS(x)
#define QML_NAMED_ELEMENT(x)
#endif

namespace api::v1 {
enum class WebsocketMessageType : uint16_t {

    Q_NAMESPACE
    QML_NAMED_ELEMENT(ApiEnums)

    UNKNOWN = 0,

    COMMON_NEW_MESSAGE = 100,
    COMMON_MESSAGE_READ = 101,

    SECRET_REQUEST = 200,
    SECRET_ACCEPT = 201,
    SECRET_NEW_MESSAGE = 202,
    SECRET_MESSAGE_READ = 203

    Q_ENUM_NS(WebsocketMessageType)
};
}
