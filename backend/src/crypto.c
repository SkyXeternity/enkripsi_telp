 #include <stdio.h>
 #include <openssl/err.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>
 #include <openssl/core_names.h>
 #include "crypto.h"
 
 
 int crypto_encrypt(
  const unsigned char *plaintext,
  int plaintext_len,
  const unsigned char *key,
  const unsigned char *nonce,
  unsigned char *ciphertext,
  unsigned char *tag
) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  int len;
  int ciphertext_len;

  if (ctx == NULL)
      return -1;

  if (EVP_EncryptInit_ex(
          ctx, EVP_aes_256_gcm(), NULL, key, nonce
      ) != 1) {
      EVP_CIPHER_CTX_free(ctx);
      return -1;
  }

  if (EVP_EncryptUpdate(
          ctx,
          ciphertext,
          &len,
          plaintext,
          plaintext_len
      ) != 1) {
      EVP_CIPHER_CTX_free(ctx);
      return -1;
  }

  ciphertext_len = len;

  if (EVP_EncryptFinal_ex(
          ctx,
          ciphertext + ciphertext_len,
          &len
      ) != 1) {
      EVP_CIPHER_CTX_free(ctx);
      return -1;
  }

  ciphertext_len += len;

  if (EVP_CIPHER_CTX_ctrl(
          ctx,
          EVP_CTRL_GCM_GET_TAG,
          TAG_SIZE,
          tag
      ) != 1) {
      EVP_CIPHER_CTX_free(ctx);
      return -1;
  }

  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}
 
int crypto_decrypt(
    const unsigned char *ciphertext,
    int ciphertext_len,
    const unsigned char *key,
    const unsigned char *nonce,
    const unsigned char *tag,
    unsigned char *plaintext
) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int plaintext_len;
    int result;

    if (ctx == NULL)
        return -1;

    if (EVP_DecryptInit_ex(
            ctx, EVP_aes_256_gcm(), NULL, key, nonce
        ) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptUpdate(
            ctx,
            plaintext,
            &len,
            ciphertext,
            ciphertext_len
        ) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    plaintext_len = len;

    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_TAG,
            TAG_SIZE,
            (void *)tag
        ) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    result = EVP_DecryptFinal_ex(
        ctx,
        plaintext + plaintext_len,
        &len
    );

    EVP_CIPHER_CTX_free(ctx);

    if (result <= 0)
        return -1; /* Tag tidak valid */

    plaintext_len += len;
    return plaintext_len;
}

 
//  int crypto(int argc, char **argv)
//  {
//      if (!aes_gcm_encrypt())
//          return EXIT_FAILURE;
 
//      if (!aes_gcm_decrypt())
//          return EXIT_FAILURE;
 
//      return EXIT_SUCCESS;
//  }
