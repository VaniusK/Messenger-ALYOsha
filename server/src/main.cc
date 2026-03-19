#include <drogon/drogon.h>

/*
Copyright (C) 2026  Alesha Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

int main() {
    setbuf(
        stdout, nullptr
    );  // disabling stdout buffer for real-time logs in console(comment on
        // realease and turn on file logs)

    drogon::app().loadConfigFile("config.json");

    drogon::orm::PostgresConfig dbConfig;
    dbConfig.host = std::getenv("POSTGRES_HOST") ?: "localhost";
    dbConfig.port = 5432;
    dbConfig.databaseName = std::getenv("POSTGRES_DB") ?: "messenger_db";
    dbConfig.username = std::getenv("POSTGRES_USER") ?: "messenger";
    dbConfig.password = std::getenv("POSTGRES_PASSWORD") ?: "";
    dbConfig.connectionNumber = 50;
    dbConfig.name = "default";
    dbConfig.timeout = -1.0;
    dbConfig.isFast = false;
    drogon::app().addDbClient(dbConfig);
    drogon::app().setThreadNum(64);
    LOG_INFO << "Starting server on port 5555";
    drogon::app().run();
    return 0;
}
