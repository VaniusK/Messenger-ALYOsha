#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

namespace api::v1::utils {
#define RETURN_RESPONSE_CODE_400(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k400BadRequest);                             \
    co_return resp;
#define RETURN_RESPONSE_CODE_401(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k401Unauthorized);                           \
    co_return resp;
#define RETURN_RESPONSE_CODE_404(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k404NotFound);                               \
    co_return resp;
#define RETURN_RESPONSE_CODE_409(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k409Conflict);                               \
    co_return resp;
#define RETURN_RESPONSE_CODE_500(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k500InternalServerError);                    \
    co_return resp;
#define RETURN_RESPONSE_CODE_200(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k200OK);                                     \
    co_return resp;
#define RETURN_RESPONSE_CODE_201(response_json)                              \
    HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(response_json); \
    resp->setStatusCode(drogon::k201Created);                                \
    co_return resp;
}  // namespace api::v1::utils