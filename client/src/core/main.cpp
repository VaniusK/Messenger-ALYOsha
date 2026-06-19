#include <qobject.h>
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
#include "SecretChatManager.hpp"
#include "SecretDatabaseManager.hpp"
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
        [stateManager]() { return stateManager->getToken(); },
        "https://api.alyosha.su/v1", "wss://api.alyosha.su/ws/chat", &app
    );
    auto *authManager = new AuthManager(connectionManager, stateManager, &app);
    auto *mediaCacheManager = new MediaCacheManager(connectionManager, &app);
    auto *localChatStorage = new LocalChatStorage(false, "default", &app);
    auto *chatManager = new ChatManager(
        connectionManager, stateManager, mediaCacheManager, localChatStorage,
        &app
    );
    auto *mediaManager = new MediaManager(
        connectionManager, stateManager, localChatStorage, chatManager, &app
    );
    auto *voiceManager = new VoiceManager(&app);
    auto *secretDatabaseManager = new client::db::SecretDatabaseManager(&app);
    auto *secretChatManager = new client::core::SecretChatManager(
        secretDatabaseManager, stateManager, connectionManager, &app
    );

    if (!client::crypto::CryptoManager::init(
        )) {  // Maybe it can be prettier with qDebug. I don't know on
              // which stage it's initialized
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
    qmlRegisterSingletonInstance(
        "Messenger", 1, 0, "SecretChatManager", secretChatManager
    );

    QObject::connect(
        stateManager, &StateManager::stateCleared, secretChatManager,
        &client::core::SecretChatManager::logout
    );
    QObject::connect(
        chatManager, &ChatManager::incomingSecretPayload, secretChatManager,
        &client::core::SecretChatManager::processIncomingSecretPayload
    );

    const QUrl url(u"qrc:/messenger_client_uri/src/ui/main.qml"_qs);
    engine.load(url);
    return app.exec();
}
