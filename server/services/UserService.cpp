#include "services/UserService.hpp"
#include <drogon/HttpController.h>
#include <json/value.h>
#include <jwt-cpp/jwt.h>
#include <trantor/utils/Logger.h>
#include <chrono>
#include <optional>
#include "dto/UserServiceDtos.hpp"
#include "models/Users.h"
#include "repositories/UserRepository.hpp"
#include "utils/server_exceptions.hpp"
#include "utils/server_response_macro.hpp"

using namespace drogon;
using namespace api::v1;
using namespace messenger::dto;

using UserRepo = messenger::repositories::UserRepository;
using User = drogon_model::messenger_db::Users;

Task<RegisterUserResponseDto> UserService::registerUser(
    RegisterUserRequestDto request_dto
) {
    std::string password_hash =
        password_hasher->generateHash(request_dto.password);
    bool success = co_await user_repo->create(
        request_dto.handle, request_dto.display_name, password_hash
    );
    if (success) {
        auto user = co_await user_repo->getByHandle(request_dto.handle);
        auto saved_chat = co_await chat_repo->createSaved(user->getValueOfId());
        co_return RegisterUserResponseDto();
    } else {
        throw messenger::exceptions::ConflictException("User already exists");
    }
}

Task<LoginUserResponseDto> UserService::loginUser(
    LoginUserRequestDto request_dto
) {
    std::optional<User> user =
        co_await user_repo->getByHandle(request_dto.handle);
    if (user == std::nullopt) {
        throw messenger::exceptions::UnauthorizedException(
            "Invalid handle or password"
        );
    } else {
        if (!password_hasher->verifyPassword(
                request_dto.password, user->getValueOfPasswordHash()
            )) {
            throw messenger::exceptions::UnauthorizedException(
                "Invalid handle or password"
            );
        }
        const char *env_key = std::getenv("JWT_KEY");
        if (!env_key) {
            throw messenger::exceptions::InternalServerErrorException(
                "JWT_KEY is not set"
            );
        }
        const std::string JWT_KEY = env_key;

        std::string token =
            jwt::create()
                .set_issuer("alesha_messenger")
                .set_type("JWT")
                .set_issued_at(std::chrono::system_clock::now())
                //.set_expires_at(std::chrono::system_clock.now() +
                // std::chrono::hours(24)) // uncomment when we have refresh
                // tokens
                .set_payload_claim(
                    "user_id", jwt::claim(std::to_string(user->getValueOfId()))
                )
                .set_payload_claim(
                    "handle", jwt::claim(user->getValueOfHandle())
                )
                .sign(jwt::algorithm::hs256(JWT_KEY));
        LoginUserResponseDto response_dto(std::move(token));
        co_return response_dto;
    }
}

Task<GetUserResponseDto> UserService::getUserById(int64_t user_id) {
    std::optional<User> user = co_await user_repo->getById(user_id);
    if (!user.has_value()) {
        throw messenger::exceptions::NotFoundException(
            "User with id " + std::to_string(user_id) + " doesn't exist"
        );
    }
    GetUserResponseDto response_dto(std::move(user.value()));
    co_return response_dto;
}

Task<GetUserResponseDto> UserService::getUserByHandle(std::string user_handle) {
    std::optional<User> user =
        co_await user_repo->getByHandle(std::move(user_handle));
    if (!user.has_value()) {
        throw messenger::exceptions::NotFoundException(
            "User with handle " + user_handle + " doesn't exist"
        );
    }
    GetUserResponseDto response_dto(std::move(user.value()));
    co_return response_dto;
}

Task<SearchUserResponseDto> UserService::searchUser(
    SearchUserRequestDto request_dto
) {
    std::vector<User> users =
        co_await user_repo->search(request_dto.query, request_dto.limit);
    SearchUserResponseDto response_dto(std::move(users));
    co_return response_dto;
}
