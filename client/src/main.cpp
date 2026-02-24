#include <qqml.h>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include "AuthManager.hpp"
#include "ChatManager.hpp"
#include "StateManager.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "Auth", new AuthManager(&app)
    );
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "AppState", new StateManager(&app)
    );
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "ChatLayer", new ChatManager(&app)
    );
    const QUrl url(u"qrc:/messenger_client_uri/src/main.qml"_qs);
    engine.load(url);
    return app.exec();
}
