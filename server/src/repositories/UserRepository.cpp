#include "repositories/UserRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <stdexcept>

using User = drogon_model::messenger_db::Users;
using UserRepository = messenger::repositories::UserRepository;

using namespace drogon;
using namespace drogon::orm;

Task<std::optional<User>> UserRepository::getById(int64_t id) {
    auto mapper = getMapper();

    try {
        User user = co_await mapper.findByPrimaryKey(id);

        co_return user;
    } catch (const UnexpectedRows &e) {
        co_return std::nullopt;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::optional<User>> UserRepository::getByHandle(std::string handle) {
    auto mapper = getMapper();

    try {
        User user = co_await mapper.findOne(
            Criteria(User::Cols::_handle, CompareOperator::EQ, handle)
        );

        co_return user;
    } catch (const UnexpectedRows &e) {
        co_return std::nullopt;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::vector<User>> UserRepository::getAll() {
    auto mapper = getMapper();

    try {
        std::vector<User> users = co_await mapper.findAll();

        co_return users;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> UserRepository::create(
    std::string handle,
    std::string display_name,
    std::string password_hash
) {
    auto mapper = getMapper();

    try {
        User user;
        user.setHandle(handle);
        user.setDisplayName(display_name);
        user.setPasswordHash(password_hash);
        co_await mapper.insert(user);
        co_return true;
    } catch (const DrogonDbException &e) {
        // assuming it means UNIQUE constraint violation
        // idk how to distinguish it from generic db exeption
        co_return false;
    }
}

Task<std::vector<User>> UserRepository::getByIds(std::vector<int64_t> ids) {
    auto mapper = getMapper();

    try {
        std::vector<User> users = co_await mapper.findBy(
            Criteria(User::Cols::_id, CompareOperator::In, ids)
        );
        co_return users;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::vector<User>>
UserRepository::search(std::string query, int64_t limit) {
    auto mapper = getMapper();

    query = "%" + query + "%";
    try {
        std::vector<User> users = co_await mapper.limit(limit).findBy(
            Criteria(User::Cols::_handle, CompareOperator::Like, query) ||
            Criteria(User::Cols::_display_name, CompareOperator::Like, query)
        );
        co_return users;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> UserRepository::updateProfile(
    int64_t user_id,
    std::optional<std::string> display_name,
    std::optional<std::string> avatar,
    std::optional<std::string> description
) {
    auto mapper = getMapper();
    try {
        auto user = co_await mapper.findByPrimaryKey(user_id);
        if (display_name.has_value()) {
            user.setDisplayName(display_name.value());
        }
        if (avatar.has_value()) {
            user.setAvatarPath(avatar.value());
        }
        if (description.has_value()) {
            user.setDescription(description.value());
        }
        co_await mapper.update(user);
        co_return true;
    } catch (const UnexpectedRows &e) {
        co_return false;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}