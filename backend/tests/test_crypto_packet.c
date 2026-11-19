#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "packet.h"

#define BUFFER_SIZE 128

int main(void)
{
    const unsigned char plaintext[] = "halo";

    unsigned char key[KEY_SIZE] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f
    };

    unsigned char nonce[NONCE_SIZE] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    };

    unsigned char ciphertext[BUFFER_SIZE];
    unsigned char tag[TAG_SIZE];
    unsigned char decrypted[BUFFER_SIZE];
    unsigned char tampered_decrypted[BUFFER_SIZE];

    int ciphertext_len = crypto_encrypt(
        plaintext,
        (int)strlen((const char *)plaintext),
        key,
        nonce,
        ciphertext,
        tag
    );

    assert(ciphertext_len > 0);

    size_t packet_len;

    unsigned char *packet = packet_create(
        1,
        nonce,
        ciphertext,
        (size_t)ciphertext_len,
        tag,
        &packet_len
    );

    assert(packet != NULL);

    uint32_t parsed_sequence;
    const unsigned char *parsed_nonce;
    const unsigned char *parsed_ciphertext;
    size_t parsed_ciphertext_len;
    const unsigned char *parsed_tag;

    int parse_result = packet_parse(
        packet,
        packet_len,
        &parsed_sequence,
        &parsed_nonce,
        &parsed_ciphertext,
        &parsed_ciphertext_len,
        &parsed_tag
    );

    assert(parse_result == 0);
    assert(parsed_sequence == 1);

    int decrypted_len = crypto_decrypt(
        parsed_ciphertext,
        (int)parsed_ciphertext_len,
        key,
        parsed_nonce,
        parsed_tag,
        decrypted
    );

    assert(decrypted_len > 0);
    assert(
        decrypted_len ==
        (int)strlen((const char *)plaintext)
    );

    assert(
        memcmp(
            decrypted,
            plaintext,
            (size_t)decrypted_len
        ) == 0
    );

    packet[SEQUENCE_SIZE + NONCE_SIZE] ^= 0x01;

    // test ubah
    // Pelajaran pentingnya: jika crypto_decrypt() menghasilkan < 0, isi output harus langsung diabaikan karena autentikasi tag gagal.
    int tampered_result = crypto_decrypt(
        parsed_ciphertext,
        (int)parsed_ciphertext_len,
        key,
        parsed_nonce,
        parsed_tag,
        tampered_decrypted
    );

    assert(tampered_result < 0);

    printf("[OK] packet yang diubah berhasil ditolak\n");

    free(packet);

    printf("[OK] crypto dan packet berhasil terhubung\n");
    printf("Hasil decrypt: %.*s\n", decrypted_len, decrypted);

    return 0;
}
