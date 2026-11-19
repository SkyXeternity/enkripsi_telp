
#ifndef CRYPTO_H
#define CRYPTO_H

#define KEY_SIZE 32
#define NONCE_SIZE 12
#define TAG_SIZE 16

int crypto_encrypt(
    const unsigned char *plaintext,
    int plaintext_len,
    const unsigned char *key,
    const unsigned char *nonce,
    unsigned char *ciphertext,
    unsigned char *tag
);

int crypto_decrypt(
    const unsigned char *ciphertext,
    int ciphertext_len,
    const unsigned char *key,
    const unsigned char *nonce,
    const unsigned char *tag,
    unsigned char *plaintext
);

#endif
