#include <qqml.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtMessageHandler>
#include "AuthManager.hpp"
#include "ChatManager.hpp"
#include "ConnectionManager.hpp"
#include "StateManager.hpp"

void noMessageOutput(
    QtMsgType type,
    const QMessageLogContext &context,
    const QString &msg
) {
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

#ifdef QT_DEBAG
    qInstallMessageHandler(noMessageOutput);
#endif

    auto *stateManager = new StateManager(&app);
    auto *connectionManager = new ConnectionManager(stateManager, &app);
    auto *authManager = new AuthManager(connectionManager, &app);
    auto *chatManager = new ChatManager(connectionManager, &app);

    QQmlApplicationEngine engine;
    qmlRegisterSingletonInstance("Messenger", 1, 0, "AppState", stateManager);
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "Connection", connectionManager
    );
    qmlRegisterSingletonInstance("Messenger", 1, 0, "Auth", authManager);
    qmlRegisterSingletonInstance("Messenger", 1, 0, "ChatLayer", chatManager);
    const QUrl url(u"qrc:/messenger_client_uri/src/ui/main.qml"_qs);
    engine.load(url);
    return app.exec();
}
