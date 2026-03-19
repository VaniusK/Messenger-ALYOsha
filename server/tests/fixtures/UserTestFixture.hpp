#pragma once
#include "DbTestFixture.hpp"
#include "repositories/UserRepository.hpp"

using UserRepository = messenger::repositories::UserRepository;

class UserTestFixture : public DbTestFixture {
protected:
    UserRepository repo_ = UserRepository();
};