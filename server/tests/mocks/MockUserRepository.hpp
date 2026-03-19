#pragma once

#include <gmock/gmock.h>
#include <optional>
#include "gmock/gmock.h"
#include "models/Users.h"
#include "repositories/UserRepository.hpp"
#include "utils/password_hasher.hpp"

using User = drogon_model::messenger_db::Users;

class MockPasswordHasher : public messenger::utils::PasswordHasherInterface {
public:
    MOCK_METHOD(std::string, generateHash, (const std::string &), (override));
    MOCK_METHOD(
        bool,
        verifyPassword,
        (const std::string &, const std::string &),
        (override)
    );
};

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