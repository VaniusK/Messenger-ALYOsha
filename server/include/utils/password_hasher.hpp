#pragma once
#include <bcrypt/BCrypt.hpp>
#include <string>

namespace messenger::utils {
struct PasswordHasherInterface {
public:
    virtual ~PasswordHasherInterface() = default;
    virtual std::string generateHash(const std::string &password) = 0;
    virtual bool
    verifyPassword(const std::string &password, const std::string &hash) = 0;
};

struct BCryptPasswordHasher : PasswordHasherInterface {
public:
    std::string generateHash(const std::string &password) {
        return BCrypt::generateHash(password, 10);
    }

    bool verifyPassword(const std::string &password, const std::string &hash) {
        return BCrypt::validatePassword(password, hash);
    }
};
}  // namespace messenger::utils
