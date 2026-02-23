#pragma once
#include "DbTestFixture.hpp"
#include "repositories/MessageRepository.hpp"

using MessageRepository = messenger::repositories::MessageRepository;

class MessageTestFixture : public DbTestFixture {
protected:
    MessageRepository repo_ = MessageRepository();
};