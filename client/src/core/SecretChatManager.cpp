#include "include/SecretChatManager.hpp"
#include "CryptoManager.hpp"
#include "SecretDatabaseManager.hpp"
#include "StateManager.hpp"

namespace client::core {
bool SecretChatManager::initSession() {
    if (!m_dbManager || !m_stateManager) {
        return false;
    }

    QString dbPath = m_stateManager->getSecretDatabasePath();

    if (!m_dbManager->init(dbPath)) {
        qCritical() << "[SecretChatManager] Failed to init DB at" << dbPath;
        return false;
    }
    auto user_keys = m_dbManager->getIdentity();
    if (!user_keys.has_value()) {
        qDebug() << "[SecretChatManager] New device detected. Generating new "
                    "E2E keys...";

        auto [newPub, newPriv] = crypto::CryptoManager::generateKeyPair();

        if (!m_dbManager->saveIdentity(newPub, newPriv)) {
            qCritical() << "[SecretChatManager] Failed to save newly generated "
                           "keys to DB!";
            return false;
        }

        m_publicKey = std::move(newPub);
        m_privateKey = std::move(newPriv);
    } else {
        m_publicKey = std::move(user_keys->publicKey);
        m_privateKey = std::move(user_keys->privateKey);
    }

    qDebug() << "[SecretChatManager] Session successfully initialized.";
    // TODO maybe emit for qml
    return true;
}

void SecretChatManager::logout() {
    m_publicKey.clear();
    m_privateKey.clear();

    if (m_dbManager) {
        m_dbManager->logout();
    }
    qDebug() << "[SecretChatManager] Logged out safely. Keys wiped from RAM.";
}
}  // namespace client::core