#include "include/CryptoManager.hpp"
#include <sodium.h>
#include <sodium/crypto_box.h>
#include <sodium/randombytes.h>
#include <QDebug>
#include <QtGlobal>

namespace client::crypto {
bool CryptoManager::init() {
    return sodium_init() >= 0;
}

KeyPair CryptoManager::generateKeyPair() {
    KeyPair key_pair;
    key_pair.publicKey.resize(crypto_box_PUBLICKEYBYTES);
    key_pair.privateKey.resize(crypto_box_SECRETKEYBYTES);

    crypto_box_keypair(
        reinterpret_cast<unsigned char *>(key_pair.publicKey.data()),
        reinterpret_cast<unsigned char *>(key_pair.privateKey.data())
    );
    return key_pair;
}

SharedSecretResult CryptoManager::computeSharedSecret(
    const QByteArray &my_private_key,
    const QByteArray &other_public_key
) {
    if (my_private_key.size() != crypto_box_SECRETKEYBYTES ||
        other_public_key.size() != crypto_box_PUBLICKEYBYTES) {
        qWarning() << "[Crypto] Shared Secret Failed: Invalid input key sizes";
        return {false, {}, CryptoError::InvalidKeySize};
    }

    QByteArray shared_secret;
    shared_secret.resize(crypto_box_BEFORENMBYTES);
    if (crypto_box_beforenm(
            reinterpret_cast<unsigned char *>(shared_secret.data()),
            reinterpret_cast<const unsigned char *>(other_public_key.constData()
            ),
            reinterpret_cast<const unsigned char *>(my_private_key.constData())
        ) != 0) {
        qWarning() << "[Crypto] Shared Secret failed: Internal math error";
        return {false, {}, CryptoError::KeyComputationFailed};
    }
    return {true, shared_secret, CryptoError::None};
}

EncryptResult CryptoManager::encryptMessage(
    const QByteArray &plain_text,
    const QByteArray &shared_secret
) {
    if (shared_secret.size() != crypto_box_BEFORENMBYTES) {
        qWarning() << "[Crypto] Encrypt failed: Invalid Shared Secret size:"
                   << shared_secret.size();
        return {false, {}, CryptoError::InvalidKeySize};
    }

    QByteArray envelope;
    envelope.resize(
        crypto_box_NONCEBYTES + plain_text.size() + crypto_box_MACBYTES
    );

    unsigned char *noncePtr =
        reinterpret_cast<unsigned char *>(envelope.data());
    randombytes_buf(noncePtr, crypto_box_NONCEBYTES);

    unsigned char *cipherTextPtr = noncePtr + crypto_box_NONCEBYTES;
    if (crypto_box_easy_afternm(
            cipherTextPtr,
            reinterpret_cast<const unsigned char *>(plain_text.constData()),
            plain_text.size(), noncePtr,
            reinterpret_cast<const unsigned char *>(shared_secret.constData())
        ) != 0) {
        qWarning(
        ) << "[Crypto] Encrypt failed: libsodium rejected the operation";
        return {false, {}, CryptoError::EncryptionFailed};
    }

    return {true, envelope.toBase64(), CryptoError::None};
}

DecryptResult CryptoManager::decryptMessage(
    const QByteArray &base64_envelope,
    const QByteArray &shared_secret
) {
    if (shared_secret.size() != crypto_box_BEFORENMBYTES) {
        qWarning() << "[Crypto] Decrypt failed: Invalid Shared Secret size:"
                   << shared_secret.size();
        return {false, {}, CryptoError::InvalidKeySize};
    }

    QByteArray envelope = QByteArray::fromBase64(base64_envelope);
    if (envelope.size() < crypto_box_NONCEBYTES + crypto_box_MACBYTES) {
        qWarning() << "[Crypto] Decrypt failed: Payload too small:"
                   << envelope.size();
        return {false, {}, CryptoError::InvalidPayloadSize};
    }

    QByteArray plain_text;
    plain_text.resize(
        envelope.size() - crypto_box_MACBYTES - crypto_box_NONCEBYTES
    );

    const unsigned char *nonce_ptr =
        reinterpret_cast<const unsigned char *>(envelope.constData());
    const unsigned char *cipher_text_ptr = nonce_ptr + crypto_box_NONCEBYTES;
    uint64_t cipher_text_len = envelope.size() - crypto_box_NONCEBYTES;

    if (crypto_box_open_easy_afternm(
            reinterpret_cast<unsigned char *>(plain_text.data()),
            cipher_text_ptr, cipher_text_len, nonce_ptr,
            reinterpret_cast<const unsigned char *>(shared_secret.constData())
        ) != 0) {
        qWarning() << "[Crypto] Decrypt failed: MAC mismatch or wrong key "
                      "(Attack or desync)";
        return {false, {}, CryptoError::DecryptionFailed};
    }

    return {true, plain_text, CryptoError::None};
}

}  // namespace client::crypto
