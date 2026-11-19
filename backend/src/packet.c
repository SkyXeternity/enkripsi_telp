#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "packet.h"

unsigned char *packet_create(
  uint32_t sequence,
  const unsigned char *nonce,
  const unsigned char *ciphertext,
  size_t ciphertext_len,
  const unsigned char *tag,
  size_t *packet_len
) {
  if (
    nonce == NULL ||
    tag == NULL ||
    packet_len == NULL ||
    (ciphertext == NULL && ciphertext_len > 0)
  ) {
    return NULL;
  }
  if (ciphertext_len > SIZE_MAX - PACKET_FIXED_SIZE) {
    return NULL;
  }

  size_t total_len = 
    PACKET_FIXED_SIZE + ciphertext_len;

    unsigned char *packet = malloc(total_len);

  if (packet == NULL) {
    return NULL;
  }

  packet[0] = (unsigned char)(sequence >> 24);
  packet[1] = (unsigned char)(sequence >> 16);
  packet[2] = (unsigned char)(sequence >> 8);
  packet[3] = (unsigned char)sequence;

  memcpy(
    packet + SEQUENCE_SIZE,
    nonce,
    NONCE_SIZE
  );

  if (ciphertext_len > 0){
    memcpy(
      packet + SEQUENCE_SIZE + NONCE_SIZE,
      ciphertext,
      ciphertext_len
    );
  }

  memcpy(
    packet + SEQUENCE_SIZE + NONCE_SIZE + ciphertext_len,
    tag,
    TAG_SIZE
  );

  *packet_len = total_len;

  return packet;

}


int packet_parse(
  const unsigned char *packet,
  size_t packet_len,
  uint32_t *sequence,
  const unsigned char **nonce,
  const unsigned char **ciphertext,
  size_t *ciphertext_len,
  const unsigned char **tag
) {
  if (
      packet == NULL ||
      sequence == NULL ||
      nonce == NULL ||
      ciphertext == NULL ||
      ciphertext_len == NULL ||
      tag == NULL
  ) {
      return -1;
  }

  if (packet_len < PACKET_FIXED_SIZE) {
      return -1;
  }

  *sequence =
      ((uint32_t)packet[0] << 24) |
      ((uint32_t)packet[1] << 16) |
      ((uint32_t)packet[2] << 8) |
      (uint32_t)packet[3];

  *nonce =
      packet + SEQUENCE_SIZE;

  *ciphertext_len =
      packet_len - PACKET_FIXED_SIZE;

  *ciphertext =
      packet + SEQUENCE_SIZE + NONCE_SIZE;

  *tag =
      packet + packet_len - TAG_SIZE;

  return 0;
}

