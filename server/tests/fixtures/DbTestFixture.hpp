#pragma once
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gtest/gtest.h>
#include <chrono>
#include <vector>

using namespace drogon;
using namespace drogon::orm;

inline void runDrogon() {
    app().addListener("0.0.0.0", 5555);
    app().run();
}

class DbTestFixture : public ::testing::Test {
public:
    void SetUp() override {
        auto dbClient = app().getDbClient();
        dbClient->execSqlSync("SET client_min_messages TO WARNING;");
        dbClient->execSqlSync("TRUNCATE TABLE users CASCADE;");
    }

    void TearDown() override {
    }
};