#include <drogon/drogon.h>
#include <gtest/gtest.h>
#include <thread>

class GlobalDrogonEnvironment : public ::testing::Environment {
public:
    ~GlobalDrogonEnvironment() override {
    }

    void SetUp() override {
        drogon::app().addListener("0.0.0.0", 5555);
        drogon::app().createDbClient(
            "postgresql",  // rdbms
            std::getenv("POSTGRES_HOST") ? std::getenv("POSTGRES_HOST")
                                         : "localhost",
            5432,  // port
            std::getenv("POSTGRES_DB") ? std::getenv("POSTGRES_DB")
                                       : "messenger_db",
            std::getenv("POSTGRES_USER") ? std::getenv("POSTGRES_USER")
                                         : "messenger",
            std::getenv("POSTGRES_PASSWORD") ? std::getenv("POSTGRES_PASSWORD")
                                             : "",
            10  // connections
        );

        serverThread_ = std::thread([]() { drogon::app().run(); });

        auto start = std::chrono::steady_clock::now();
        while (!drogon::app().isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (std::chrono::steady_clock::now() - start >
                std::chrono::seconds(5)) {
                throw std::runtime_error(
                    "Drogon failed to start within 5 seconds"
                );
            }
        }
    }

    void TearDown() override {
        drogon::app().quit();
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }

private:
    std::thread serverThread_;
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "Starting global Drogon Environment..." << std::endl;
    ::testing::AddGlobalTestEnvironment(new GlobalDrogonEnvironment);
    return RUN_ALL_TESTS();
}
