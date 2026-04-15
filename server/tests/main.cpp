#include <arpa/inet.h>
#include <drogon/drogon.h>
#include <gtest/gtest.h>
#include <netdb.h>
#include <cstdlib>
#include <thread>

class GlobalDrogonEnvironment : public ::testing::Environment {
public:
    ~GlobalDrogonEnvironment() override {
    }

    void SetUp() override {
        // ОЧЕНЬ ВАЖНЫЙ ХАК ДЛЯ MINIO В CI:
        // Если указан хост minio, minio-cpp может пытаться делать запросы в
        // Virtual-Hosted style (bucket.minio), что приводит к ошибке 404
        // ResourceNotFound от Minio. Заменив S3_BASE_URL на IP-адрес, мы
        // "принуждаем" minio-cpp использовать корректный Path-Style.
        const char *minio_env = std::getenv("S3_BASE_URL");
        if (minio_env &&
            std::string(minio_env).find("minio") != std::string::npos) {
            struct hostent *he = gethostbyname("minio");
            if (he != nullptr && he->h_addr_list[0] != nullptr) {
                std::string minio_ip =
                    inet_ntoa(**(struct in_addr **)he->h_addr_list);
                std::string new_url = minio_ip + ":9000";
                setenv("S3_BASE_URL", new_url.c_str(), 1);
                std::cout << "Patched S3_BASE_URL to " << new_url
                          << " to force Path-Style" << std::endl;
            }
        }

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
