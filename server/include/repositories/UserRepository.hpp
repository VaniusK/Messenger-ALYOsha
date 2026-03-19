#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/utils/coroutine.h>
#include "models/Users.h"

namespace messenger::repositories {

using User = drogon_model::messenger_db::Users;

class UserRepositoryInterface {
public:
    virtual ~UserRepositoryInterface() = default;
    virtual drogon::Task<std::optional<User>> getById(int64_t id) = 0;
    virtual drogon::Task<std::optional<User>> getByHandle(std::string handle
    ) = 0;
    virtual drogon::Task<std::vector<User>> getAll() = 0;
    virtual drogon::Task<bool> create(
        std::string handle,
        std::string display_name,
        std::string password_hash
    ) = 0;
    virtual drogon::Task<std::vector<User>> getByIds(std::vector<int64_t> ids
    ) = 0;
    virtual drogon::Task<std::vector<User>>
    search(std::string query, int64_t limit) = 0;
    virtual drogon::Task<bool> updateProfile(
        int64_t user_id,
        std::optional<std::string> display_name,
        std::optional<std::string> avatar,
        std::optional<std::string> description
    ) = 0;
};

class UserRepository : public UserRepositoryInterface {
public:
    drogon::Task<std::optional<User>> getById(int64_t id) override;
    drogon::Task<std::optional<User>> getByHandle(std::string handle) override;
    drogon::Task<std::vector<User>> getAll() override;
    drogon::Task<bool> create(
        std::string handle,
        std::string display_name,
        std::string password_hash
    ) override;
    drogon::Task<std::vector<User>> getByIds(std::vector<int64_t> ids) override;
    drogon::Task<std::vector<User>> search(std::string query, int64_t limit)
        override;
    drogon::Task<bool> updateProfile(
        int64_t user_id,
        std::optional<std::string> display_name,
        std::optional<std::string> avatar,
        std::optional<std::string> description
    ) override;

private:
    drogon::orm::CoroMapper<User> getMapper() {
        return drogon::orm::CoroMapper<User>(drogon::app().getDbClient());
    }
};

}  // namespace messenger::repositories