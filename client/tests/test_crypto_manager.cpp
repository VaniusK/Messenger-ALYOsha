#include <gtest/gtest.h>
#include <QByteArray>
#include <QString>
#include "../include/CryptoManager.hpp"

using namespace client::crypto;

class CryptoManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(CryptoManager::init());
    }
};

TEST_F(CryptoManagerTest, GeneratesCorrectKeySizes) {
    KeyPair keys = CryptoManager::generateKeyPair();

    EXPECT_EQ(keys.publicKey.size(), 32);
    EXPECT_EQ(keys.privateKey.size(), 32);
}

TEST_F(CryptoManagerTest, SharedSecretMatchesForAliceAndBob) {
    KeyPair alice = CryptoManager::generateKeyPair();
    KeyPair bob = CryptoManager::generateKeyPair();

    auto aliceResult =
        CryptoManager::computeSharedSecret(alice.privateKey, bob.publicKey);
    auto bobResult =
        CryptoManager::computeSharedSecret(bob.privateKey, alice.publicKey);

    ASSERT_TRUE(aliceResult.success);
    ASSERT_TRUE(bobResult.success);

    EXPECT_EQ(aliceResult.secret, bobResult.secret);
}

TEST_F(CryptoManagerTest, EncryptDecryptSymmetry) {
    KeyPair alice = CryptoManager::generateKeyPair();
    KeyPair bob = CryptoManager::generateKeyPair();
    QByteArray sharedSecret =
        CryptoManager::computeSharedSecret(alice.privateKey, bob.publicKey)
            .secret;

    QByteArray originalMessage = "Hello, Bob! Transfer 100$";

    auto encryptResult =
        CryptoManager::encryptMessage(originalMessage, sharedSecret);
    ASSERT_TRUE(encryptResult.success);
    EXPECT_FALSE(encryptResult.envelope.isEmpty());

    auto decryptResult =
        CryptoManager::decryptMessage(encryptResult.envelope, sharedSecret);
    ASSERT_TRUE(decryptResult.success);

    EXPECT_EQ(decryptResult.plainText, originalMessage);
}

TEST_F(CryptoManagerTest, RejectsTamperedMessage) {
    KeyPair alice = CryptoManager::generateKeyPair();
    KeyPair bob = CryptoManager::generateKeyPair();
    QByteArray sharedSecret =
        CryptoManager::computeSharedSecret(alice.privateKey, bob.publicKey)
            .secret;

    QByteArray originalMessage = "I am transferring you 100$";
    auto encryptResult =
        CryptoManager::encryptMessage(originalMessage, sharedSecret);
    ASSERT_TRUE(encryptResult.success);

    QByteArray tamperedEnvelope =
        QByteArray::fromBase64(encryptResult.envelope);

    tamperedEnvelope[tamperedEnvelope.size() - 1] =
        tamperedEnvelope[tamperedEnvelope.size() - 1] ^
        0xFF;  // Переворачиваем биты

    QByteArray hackedBase64 = tamperedEnvelope.toBase64();

    auto decryptResult =
        CryptoManager::decryptMessage(hackedBase64, sharedSecret);

    EXPECT_FALSE(decryptResult.success);
    EXPECT_EQ(decryptResult.error, CryptoError::DecryptionFailed);
}

TEST_F(CryptoManagerTest, RejectsTruncatedPayload) {
    KeyPair alice = CryptoManager::generateKeyPair();
    KeyPair bob = CryptoManager::generateKeyPair();
    QByteArray sharedSecret =
        CryptoManager::computeSharedSecret(alice.privateKey, bob.publicKey)
            .secret;

    QByteArray shortGarbage = "TooShort";
    QByteArray base64Garbage = shortGarbage.toBase64();

    auto decryptResult =
        CryptoManager::decryptMessage(base64Garbage, sharedSecret);

    EXPECT_FALSE(decryptResult.success);
    EXPECT_EQ(decryptResult.error, CryptoError::InvalidPayloadSize);
}