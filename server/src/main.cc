#include <drogon/drogon.h>
#include <iostream>

int main() {
    setbuf(stdout, nullptr); // disabling stdout buffer for real-time logs in console(comment on realease and turn on file logs)
    
    LOG_INFO << "Starting server on port 5555";

    drogon::app().loadConfigFile("config.json");

    drogon::orm::PostgresConfig dbConfig;
    dbConfig.host = std::getenv("POSTGRES_HOST") ?: "localhost";
    dbConfig.port = 5432;
    dbConfig.databaseName = std::getenv("POSTGRES_DB") ?: "messenger_db";
    dbConfig.username = std::getenv("POSTGRES_USER") ?: "messenger";
    dbConfig.password = std::getenv("POSTGRES_PASSWORD") ?: "";
    dbConfig.connectionNumber = 1;
    dbConfig.name = "default";
    dbConfig.timeout = -1.0;
    dbConfig.isFast = false;
    drogon::app().addDbClient(dbConfig);
    drogon::app().run();
    return 0;
}
