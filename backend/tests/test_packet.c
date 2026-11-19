
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packet.h"

static void test_create_and_parse_packet(void)
{
    uint32_t original_sequence = 42;

    unsigned char original_nonce[NONCE_SIZE] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    };

    unsigned char original_ciphertext[] = {
        0xaa, 0xbb, 0xcc, 0xdd
    };

    unsigned char original_tag[TAG_SIZE] = {
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f
    };

    size_t packet_len;

    unsigned char *packet = packet_create(
        original_sequence,
        original_nonce,
        original_ciphertext,
        sizeof(original_ciphertext),
        original_tag,
        &packet_len
    );

    assert(packet != NULL);

    assert(
        packet_len ==
        PACKET_FIXED_SIZE + sizeof(original_ciphertext)
    );

    uint32_t parsed_sequence;
    const unsigned char *parsed_nonce;
    const unsigned char *parsed_ciphertext;
    size_t parsed_ciphertext_len;
    const unsigned char *parsed_tag;

    int result = packet_parse(
        packet,
        packet_len,
        &parsed_sequence,
        &parsed_nonce,
        &parsed_ciphertext,
        &parsed_ciphertext_len,
        &parsed_tag
    );

    assert(result == 0);
    assert(parsed_sequence == original_sequence);

    assert(
        parsed_ciphertext_len ==
        sizeof(original_ciphertext)
    );

    assert(
        memcmp(
            parsed_nonce,
            original_nonce,
            NONCE_SIZE
        ) == 0
    );

    assert(
        memcmp(
            parsed_ciphertext,
            original_ciphertext,
            sizeof(original_ciphertext)
        ) == 0
    );

    assert(
        memcmp(
            parsed_tag,
            original_tag,
            TAG_SIZE
        ) == 0
    );

    free(packet);

    printf("[OK] create dan parse packet\n");
}

static void test_packet_too_short(void)
{
    unsigned char invalid_packet[4] = {0};

    uint32_t sequence;
    const unsigned char *nonce;
    const unsigned char *ciphertext;
    size_t ciphertext_len;
    const unsigned char *tag;

    int result = packet_parse(
        invalid_packet,
        sizeof(invalid_packet),
        &sequence,
        &nonce,
        &ciphertext,
        &ciphertext_len,
        &tag
    );

    assert(result < 0);

    printf("[OK] packet terlalu pendek ditolak\n");
}

int main(void)
{
    test_create_and_parse_packet();
    test_packet_too_short();

    printf("Semua tes packet berhasil\n");
    return 0;
}