#pragma once
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include "repositories/UserRepository.hpp"

using namespace drogon;
using namespace drogon::orm;

void runDrogon() {
    app().addListener("0.0.0.0", 5555);
    app().run();
}

class DbTestFixture : public ::testing::Test {
public:
    static inline std::thread serverThread_;

    static void SetUpTestSuite() {
        app().createDbClient(
            "postgresql",  // rdbms
            std::getenv("POSTGRES_HOST") ?: "localhost",
            5432,  // port
            std::getenv("POSTGRES_DB") ?: "messenger_db",
            std::getenv("POSTGRES_USER") ?: "messenger",
            std::getenv("POSTGRES_PASSWORD") ?: "",
            10  // connections
        );

        serverThread_ = std::thread(runDrogon);
        auto start = std::chrono::steady_clock::now();
        while (!app().isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (std::chrono::steady_clock::now() - start >
                std::chrono::seconds(10)) {
                throw std::runtime_error("Server failed to start in 10 seconds"
                );
            }
        }
    }

    static void TearDownTestSuite() {
        app().quit();
        serverThread_.join();
    }

    void SetUp() override {
        auto dbClient = app().getDbClient();
        dbClient->execSqlSync("SET client_min_messages TO WARNING;");
        dbClient->execSqlSync("TRUNCATE TABLE users CASCADE;");
    }

    void TearDown() override {
    }
};