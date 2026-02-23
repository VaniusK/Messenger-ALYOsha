#pragma once

#include <gmock/gmock.h>
#include <optional>
#include "models/Users.h"
#include "repositories/UserRepository.hpp"

using User = drogon_model::messenger_db::Users;

class MockUserRepository
    : public messenger::repositories::UserRepositoryInterface {
public:
    MOCK_METHOD(
        drogon::Task<std::optional<User>>,
        getById,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::optional<User>>,
        getByHandle,
        (std::string),
        (override)
    );
    MOCK_METHOD(drogon::Task<std::vector<User>>, getAll, (), (override));
    MOCK_METHOD(
        drogon::Task<bool>,
        create,
        (std::string, std::string, std::string),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<User>>,
        getByIds,
        (std::vector<int64_t>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<User>>,
        search,
        (std::string, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        updateProfile,
        (int64_t,
         std::optional<std::string>,
         std::optional<std::string>,
         std::optional<std::string>),
        (override)
    );
};