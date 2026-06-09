#include "include/CryptoManager.hpp"
#include <sodium.h>
#include <sodium/crypto_box.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <sodium/randombytes.h>
#include <QDebug>
#include <QFile>
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

const qint64 CHUNK_SIZE = 65536;

QByteArray CryptoManager::generateFileKey() {
    QByteArray key;
    key.resize(crypto_secretstream_xchacha20poly1305_KEYBYTES);
    crypto_secretstream_xchacha20poly1305_keygen(
        reinterpret_cast<unsigned char *>(key.data())
    );
    return key;
}

EncryptFileResult CryptoManager::encryptFile(
    const QString &input_path,
    const QString &output_path,
    const QByteArray &file_key
) {
    if (file_key.size() != crypto_secretstream_xchacha20poly1305_KEYBYTES) {
        qWarning() << "[Crypto] File Encrypt failed: Invalid file key size";
        return {false, CryptoError::InvalidKeySize};
    }

    QFile in_file(input_path);
    QFile out_file(output_path);
    if (!in_file.open(QIODevice::ReadOnly) ||
        !out_file.open(QIODevice::WriteOnly)) {
        qWarning() << "[Crypto] File Encrypt failed: Cannot open I/O files";
        return {false, CryptoError::FileOpenError};
    }

    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;

    if (crypto_secretstream_xchacha20poly1305_init_push(
            &st, header,
            reinterpret_cast<const unsigned char *>(file_key.constData())
        ) != 0) {
        qWarning() << "[Crypto] File Encrypt failed: Stream init error";
        return {false, CryptoError::StreamInitFailed};
    }

    out_file.write(reinterpret_cast<char *>(header), sizeof(header));

    while (!in_file.atEnd()) {
        QByteArray chunk = in_file.read(CHUNK_SIZE);
        unsigned char tag =
            in_file.atEnd() ? crypto_secretstream_xchacha20poly1305_TAG_FINAL
                            : 0;

        QByteArray cipher_chunk;
        cipher_chunk.resize(
            chunk.size() + crypto_secretstream_xchacha20poly1305_ABYTES
        );

        unsigned long long out_len;
        if (crypto_secretstream_xchacha20poly1305_push(
                &st, reinterpret_cast<unsigned char *>(cipher_chunk.data()),
                &out_len,
                reinterpret_cast<const unsigned char *>(chunk.constData()),
                chunk.size(), nullptr, 0, tag
            ) != 0) {
            qWarning() << "[Crypto] File Encrypt failed: Error pushing chunk";
            return {false, CryptoError::EncryptionFailed};
        }

        out_file.write(cipher_chunk.constData(), out_len);
    }

    return {true, CryptoError::None};
}

DecryptFileResult CryptoManager::decryptFile(
    const QString &input_path,
    const QString &output_path,
    const QByteArray &file_key
) {
    if (file_key.size() != crypto_secretstream_xchacha20poly1305_KEYBYTES) {
        qWarning() << "[Crypto] File Decrypt failed: Invalid file key size";
        return {false, CryptoError::InvalidKeySize};
    }

    QFile in_file(input_path);
    QFile out_file(output_path);

    if (!in_file.open(QIODevice::ReadOnly) ||
        !out_file.open(QIODevice::WriteOnly)) {
        qWarning() << "[Crypto] File Decrypt failed: Cannot open I/O files";
        return {false, CryptoError::FileOpenError};
    }

    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    if (in_file.read(reinterpret_cast<char *>(header), sizeof(header)) !=
        sizeof(header)) {
        qWarning() << "[Crypto] File Decrypt failed: File too short for header";
        return {false, CryptoError::InvalidPayloadSize};
    }

    crypto_secretstream_xchacha20poly1305_state st;
    if (crypto_secretstream_xchacha20poly1305_init_pull(
            &st, header,
            reinterpret_cast<const unsigned char *>(file_key.constData())
        ) != 0) {
        qWarning() << "[Crypto] File Decrypt failed: Invalid header or key";
        return {false, CryptoError::StreamInitFailed};
    }

    const qint64 ENCRYPTED_CHUNK_SIZE =
        CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES;

    while (!in_file.atEnd()) {
        QByteArray cipher_chunk = in_file.read(ENCRYPTED_CHUNK_SIZE);
        QByteArray plain_chunk;
        plain_chunk.resize(
            cipher_chunk.size() - crypto_secretstream_xchacha20poly1305_ABYTES
        );

        unsigned long long out_len;
        unsigned char tag;

        if (crypto_secretstream_xchacha20poly1305_pull(
                &st, reinterpret_cast<unsigned char *>(plain_chunk.data()),
                &out_len, &tag,
                reinterpret_cast<const unsigned char *>(cipher_chunk.constData()
                ),
                cipher_chunk.size(), nullptr, 0
            ) != 0) {
            qWarning() << "[Crypto] File Decrypt failed: MAC mismatch on chunk "
                          "(Corrupted)";
            out_file.close();
            out_file.remove();
            return {false, CryptoError::StreamCorrupted};
        }

        out_file.write(plain_chunk.constData(), out_len);

        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            break;
        }
    }

    return {true, CryptoError::None};
}

}  // namespace client::crypto
