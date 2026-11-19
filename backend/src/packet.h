#ifndef PACKET_H
#define PACKET_H

#include <stddef.h>
#include <stdint.h>

#include "crypto.h"

#define SEQUENCE_SIZE 4

#define PACKET_FIXED_SIZE \
  (SEQUENCE_SIZE + NONCE_SIZE + TAG_SIZE)

unsigned char *packet_create(
  uint32_t sequence,
  const unsigned char *nonce,
  const unsigned char *ciphertext,
  size_t ciphertext_len,
  const unsigned char *tag,
  size_t *packet_len
);

int packet_parse(
  const unsigned char *packet,
  size_t packet_len,
  uint32_t *sequence,
  const unsigned char **nonce,
  const unsigned char **ciphertext,
  size_t *ciphertext_len,
  const unsigned char **tag
);

#endif  