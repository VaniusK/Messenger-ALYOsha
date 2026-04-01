#include "AuthManager.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

AuthManager::AuthManager(
    ConnectionManager *connection,
    StateManager *stateManager,
    QObject *parent
)
    : QObject(parent), m_connection(connection), m_stateManager(stateManager) {
}

void AuthManager::registerUser(
    const QString &handle,
    const QString &displayName,
    const QString &password
) {
    QJsonObject json;
    json["handle"] = handle;
    json["display_name"] = displayName;
    json["password"] = password;

    QNetworkReply *reply =
        m_connection->post("/auth/register", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 201) {
            emit registerSuccess();
        } else if (statusCode == 409) {
            emit registerFailed("Пользователь с таким логином уже существует");
        } else if (statusCode == 400) {
            emit registerFailed("Проверьте заполненные поля");
        } else {
            emit registerFailed(
                "Ошибка сервера: " + QString::number(statusCode)
            );
        }
    });
}

void AuthManager::loginUser(const QString &handle, const QString &password) {
    QJsonObject json;
    json["handle"] = handle;
    json["password"] = password;

    QNetworkReply *reply =
        m_connection->post("/auth/login", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, handle]() {
        reply->deleteLater();
        int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 200) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QString token = obj["token"].toString();

            if (token.isEmpty()) {
                emit loginFailed("Error: Server didn't return an access token."
                );
                return;
            }

            m_stateManager->setToken(token);
            emit loginSuccess(token);
            fetchUserId(handle);
        } else if (statusCode == 401 || statusCode == 404 || statusCode == 400) {
            emit loginFailed("Неверный логин или пароль");
        } else {
            emit loginFailed(
                "Ошибка авторизации (" + QString::number(statusCode) + ")"
            );
        }
    });
}

void AuthManager::fetchUserId(const QString &handle) {
    QNetworkReply *reply = m_connection->get("/users/handle/" + handle);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonValue idValue = obj["id"];
            int userId = idValue.isString() ? idValue.toString().toInt()
                                            : idValue.toInt();
            m_stateManager->setUserId(userId);
            emit userIdFetched(userId);
        }
    });
}
