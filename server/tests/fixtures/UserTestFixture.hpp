#pragma once
#include "DbTestFixture.hpp"

using UserRepository = messenger::repositories::UserRepository;
using User = drogon_model::messenger_db::Users;

class UserTestFixture : public DbTestFixture {
protected:
    UserRepository repo_ = UserRepository();
};