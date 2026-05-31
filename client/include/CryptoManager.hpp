#pragma once

#include <QByteArray>
#include <QString>

namespace client::crypto {

struct KeyPair {
    QByteArray publicKey;
    QByteArray privateKey;
};

enum class CryptoError {
    InvalidKeySize,
    InvalidPayloadSize,
    DecryptionFailed,
    EncryptionFailed,
    KeyComputationFailed,
    None
};

struct SharedSecretResult {
    bool success;
    QByteArray secret;
    CryptoError error;
};

struct EncryptResult {
    bool success;
    QByteArray envelope;
    CryptoError error;
};

struct DecryptResult {
    bool success;
    QByteArray plainText;
    CryptoError error;
};

class CryptoManager {
public:
    static bool init();
    static KeyPair generateKeyPair();

    static SharedSecretResult computeSharedSecret(
        const QByteArray &my_private_key,
        const QByteArray &other_public_key
    );

    static EncryptResult encryptMessage(
        const QByteArray &plain_text,
        const QByteArray &shared_secret
    );
    static DecryptResult decryptMessage(
        const QByteArray &base64_envelope,
        const QByteArray &shared_secret
    );
};

}  // namespace client::crypto
