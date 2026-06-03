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
    FileOpenError,
    StreamInitFailed,
    StreamCorrupted,
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

struct EncryptFileResult {
    bool success;
    CryptoError error;
};

struct DecryptFileResult {
    bool success;
    CryptoError error;
};

class CryptoManager {
public:
    static bool init();
    static KeyPair generateKeyPair();
    static QByteArray generateFileKey();

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

    static EncryptFileResult encryptFile(
        const QString &input_path,
        const QString &output_path,
        const QByteArray &file_key
    );
    static DecryptFileResult decryptFile(
        const QString &input_path,
        const QString &output_path,
        const QByteArray &file_key
    );
};

}  // namespace client::crypto
