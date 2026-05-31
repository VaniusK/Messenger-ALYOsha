#include <qqml.h>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtMessageHandler>
#include <iostream>
#include "AuthManager.hpp"
#include "ChatManager.hpp"
#include "ConnectionManager.hpp"
#include "CryptoManager.hpp"
#include "LocalChatStorage.hpp"
#include "MediaCacheManager.hpp"
#include "MediaManager.hpp"
#include "StateManager.hpp"
#include "VoiceManager.hpp"

void noMessageOutput(
    QtMsgType type,
    const QMessageLogContext &context,
    const QString &msg
) {
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("AlyoshaTeam");
    QCoreApplication::setOrganizationDomain("alyosha.su");
    QCoreApplication::setApplicationName("Alyosha");

    auto *stateManager = new StateManager(&app);
    auto *connectionManager = new ConnectionManager(
        [stateManager]() { return stateManager->getToken(); }, &app
    );
    auto *authManager = new AuthManager(connectionManager, stateManager, &app);
    auto *mediaCacheManager = new MediaCacheManager(connectionManager, &app);
    auto *localChatStorage = new LocalChatStorage(&app);
    auto *chatManager = new ChatManager(
        connectionManager, stateManager, mediaCacheManager, localChatStorage,
        &app
    );
    auto *mediaManager = new MediaManager(
        connectionManager, stateManager, localChatStorage, chatManager, &app
    );
    auto *voiceManager = new VoiceManager(&app);

    if (!client::crypto::CryptoManager::init(
        )) {  // Maybe it can be prettier with qDebug. I don't know on which
              // stage it's initialized
        std::cerr << "Error while initializing CryptoManager. Terminate...";
        return 1;
    }

    QQmlApplicationEngine engine;
    qmlRegisterSingletonInstance("Messenger", 1, 0, "AppState", stateManager);
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "Connection", connectionManager
    );
    qmlRegisterSingletonInstance("Messenger", 1, 0, "Auth", authManager);
    qmlRegisterSingletonInstance("Messenger", 1, 0, "ChatLayer", chatManager);
    qmlRegisterSingletonInstance("Messenger", 1, 0, "MediaLayer", mediaManager);
    qmlRegisterSingletonInstance("Messenger", 1, 0, "VoiceLayer", voiceManager);
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "MediaCacheLayer", mediaCacheManager
    );
    const QUrl url(u"qrc:/messenger_client_uri/src/ui/main.qml"_qs);
    engine.load(url);
    return app.exec();
}
