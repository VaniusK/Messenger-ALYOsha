#include <stdexcept>

namespace messenger::exceptions {
class NotFoundException : public std::runtime_error {
public:
    explicit NotFoundException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class UnauthorizedException : public std::runtime_error {
public:
    explicit UnauthorizedException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class ForbiddenException : public std::runtime_error {
public:
    explicit ForbiddenException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class ConflictException : public std::runtime_error {
public:
    explicit ConflictException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class TooManyRequestsException : public std::runtime_error {
public:
    explicit TooManyRequestsException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class InternalServerErrorException : public std::runtime_error {
public:
    explicit InternalServerErrorException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};

class BadRequestException : public std::runtime_error {
public:
    explicit BadRequestException(const std::string &msg)
        : std::runtime_error(msg) {
    }
};
}  // namespace messenger::exceptions
