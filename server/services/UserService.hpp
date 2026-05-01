#pragma once
#include <drogon/HttpController.h>
#include <json/value.h>
#include <memory>
#include "dto/UserServiceDtos.hpp"
#include "include/repositories/UserRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "utils/password_hasher.hpp"

using namespace drogon;
using namespace messenger::dto;

namespace api {
namespace v1 {
class UserService {
public:
    Task<RegisterUserResponseDto> registerUser(
        RegisterUserRequestDto request_dto
    );
    Task<LoginUserResponseDto> loginUser(LoginUserRequestDto request_dto);
    Task<GetUserResponseDto> getUserById(int64_t user_id);
    Task<GetUserResponseDto> getUserByHandle(std::string user_handle);
    Task<SearchUserResponseDto> searchUser(SearchUserRequestDto request_dto);

    void setUserRepo(
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    ) {
        this->user_repo = user_repo;
    }

    void setChatRepo(
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    ) {
        this->chat_repo = chat_repo;
    }

    void setPasswordHasher(
        const std::shared_ptr<messenger::utils::PasswordHasherInterface>
            password_hasher
    ) {
        this->password_hasher = password_hasher;
    }

private:
    std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo;
    std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo;
    std::shared_ptr<messenger::utils::PasswordHasherInterface> password_hasher =
        std::make_shared<messenger::utils::BCryptPasswordHasher>();
};
}  // namespace v1
}  // namespace api
