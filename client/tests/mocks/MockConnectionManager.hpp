#pragma once
#include <gmock/gmock.h>
#include <QNetworkReply>
#include "ConnectionManager.hpp"

class MockConnectionManager : public ConnectionManager {
public:
    MockConnectionManager()
        : ConnectionManager(
              []() { return QString(); },
              "http://127.0.1:8080/v1",
              "ws://127.0.1:8080/ws/chat"
          ) {
    }

    MOCK_METHOD(QNetworkReply *, get, (const QString &endpoint), (override));
    MOCK_METHOD(
        QNetworkReply *,
        post,
        (const QString &endpoint, const QByteArray &body),
        (override)
    );
};