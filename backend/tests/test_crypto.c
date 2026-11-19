#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "crypto.h"

#define TEST_BUFFER_SIZE 128

static const unsigned char test_key[KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

static const unsigned char test_nonce[NONCE_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
};

static void test_encrypt_and_decrypt(void)
{
    const unsigned char plaintext[] = "halo";
    unsigned char ciphertext[TEST_BUFFER_SIZE];
    unsigned char decrypted[TEST_BUFFER_SIZE];
    unsigned char tag[TAG_SIZE];

    int ciphertext_len = crypto_encrypt(
        plaintext,
        (int)strlen((const char *)plaintext),
        test_key,
        test_nonce,
        ciphertext,
        tag
    );

    assert(ciphertext_len > 0);

    int decrypted_len = crypto_decrypt(
        ciphertext,
        ciphertext_len,
        test_key,
        test_nonce,
        tag,
        decrypted
    );

    assert(decrypted_len == (int)strlen((const char *)plaintext));
    assert(memcmp(decrypted, plaintext, (size_t)decrypted_len) == 0);

    printf("[OK] encrypt dan decrypt\n");
}

static void test_modified_ciphertext_is_rejected(void)
{
    const unsigned char plaintext[] = "halo";
    unsigned char ciphertext[TEST_BUFFER_SIZE];
    unsigned char decrypted[TEST_BUFFER_SIZE];
    unsigned char tag[TAG_SIZE];

    int ciphertext_len = crypto_encrypt(
        plaintext,
        (int)strlen((const char *)plaintext),
        test_key,
        test_nonce,
        ciphertext,
        tag
    );

    assert(ciphertext_len > 0);

    ciphertext[0] ^= 0x01;

    int decrypted_len = crypto_decrypt(
        ciphertext,
        ciphertext_len,
        test_key,
        test_nonce,
        tag,
        decrypted
    );

    assert(decrypted_len < 0);

    printf("[OK] ciphertext yang diubah berhasil ditolak\n");
}

int main(void)
{
    test_encrypt_and_decrypt();
    test_modified_ciphertext_is_rejected();

    printf("Semua tes crypto berhasil\n");
    return 0;
}
