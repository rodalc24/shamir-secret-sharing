#include "aes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

int aes_generate_key(uint8_t key[AES_KEY_SIZE]) {
  if (key == NULL)
    return AES_ERR_PARAM;
  if (RAND_bytes(key, AES_KEY_SIZE) != 1)
    return AES_ERR_RANDOM;
  return AES_OK;
}

int aes_encrypt(const uint8_t key[AES_KEY_SIZE], const uint8_t *content,
                size_t content_len, uint8_t **out, size_t *out_len) {
  if (key == NULL || out == NULL || out_len == NULL ||
      (content == NULL && content_len > 0))
    return AES_ERR_PARAM;
  if (content_len > (size_t)INT_MAX - AES_OVERHEAD)
    return AES_ERR_PARAM;

  *out = NULL;
  *out_len = 0;

  size_t total = content_len + AES_OVERHEAD;
  uint8_t *buf = malloc(total);
  if (buf == NULL)
    return AES_ERR_ALLOC;

  uint8_t *nonce = buf;
  uint8_t *ct = buf + AES_NONCE_SIZE;
  uint8_t *tag = ct + content_len;

  int rc = AES_ERR_CRYPTO;
  EVP_CIPHER_CTX *ctx = NULL;

  if (RAND_bytes(nonce, AES_NONCE_SIZE) != 1) {
    rc = AES_ERR_RANDOM;
    goto fail;
  }

  ctx = EVP_CIPHER_CTX_new();
  if (ctx == NULL)
    goto fail;

  int len = 0;
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    goto fail;
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_NONCE_SIZE, NULL) !=
      1)
    goto fail;
  if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce) != 1)
    goto fail;

  if (content_len > 0) {
    if (EVP_EncryptUpdate(ctx, ct, &len, content, (int)content_len) != 1)
      goto fail;
    if ((size_t)len != content_len)
      goto fail;
  }
  if (EVP_EncryptFinal_ex(ctx, ct + content_len, &len) != 1)
    goto fail;
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, tag) != 1)
    goto fail;

  EVP_CIPHER_CTX_free(ctx);
  *out = buf;
  *out_len = total;
  return AES_OK;

fail:
  EVP_CIPHER_CTX_free(ctx);
  free(buf);
  return rc;
}

int aes_decrypt(const uint8_t key[AES_KEY_SIZE], const uint8_t *content,
                size_t content_len, uint8_t **out, size_t *out_len) {
  if (key == NULL || content == NULL || out == NULL || out_len == NULL)
    return AES_ERR_PARAM;
  if (content_len < AES_OVERHEAD || content_len > (size_t)INT_MAX)
    return AES_ERR_PARAM;

  *out = NULL;
  *out_len = 0;

  const uint8_t *nonce = content;
  const uint8_t *ct = content + AES_NONCE_SIZE;
  size_t pt_len = content_len - AES_OVERHEAD;
  const uint8_t *tag = ct + pt_len;

  uint8_t *pt = malloc(pt_len > 0 ? pt_len : 1);
  if (pt == NULL)
    return AES_ERR_ALLOC;

  int rc = AES_ERR_CRYPTO;
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (ctx == NULL)
    goto fail;

  int len = 0;
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    goto fail;
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_NONCE_SIZE, NULL) !=
      1)
    goto fail;
  if (EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce) != 1)
    goto fail;

  if (pt_len > 0) {
    if (EVP_DecryptUpdate(ctx, pt, &len, ct, (int)pt_len) != 1)
      goto fail;
    if ((size_t)len != pt_len)
      goto fail;
  }
  /* The tag must be set before Final; the const cast is required by the API. */
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_SIZE,
                          (void *)tag) != 1)
    goto fail;
  if (EVP_DecryptFinal_ex(ctx, pt + pt_len, &len) != 1) {
    rc = AES_ERR_AUTH;
    goto fail;
  }

  EVP_CIPHER_CTX_free(ctx);
  *out = pt;
  *out_len = pt_len;
  return AES_OK;

fail:
  EVP_CIPHER_CTX_free(ctx);
  OPENSSL_cleanse(
      pt, pt_len > 0 ? pt_len : 1); /* never leak unauthenticated plaintext */
  free(pt);
  return rc;
}
