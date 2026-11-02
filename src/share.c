#include "share.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>

#define CHECKSUM_BYTES 4
#define MAGIC_LEN 4

static const char HEX[] = "0123456789abcdef";

static void to_hex(const uint8_t *in, size_t n, char *out) {
  for (size_t i = 0; i < n; i++) {
    out[2 * i] = HEX[in[i] >> 4];
    out[2 * i + 1] = HEX[in[i] & 15];
  }
}

static int hex_val(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

/* Decodes exactly `nchars` hex characters (nchars even). Returns 0 or -1. */
static int from_hex(const char *in, size_t nchars, uint8_t *out) {
  for (size_t i = 0; i < nchars; i += 2) {
    int hi = hex_val(in[i]), lo = hex_val(in[i + 1]);
    if (hi < 0 || lo < 0)
      return -1;
    out[i / 2] = (uint8_t)((hi << 4) | lo);
  }
  return 0;
}

/* checksum = SHA-256("SSS1" || t || share)[0..3]. Hashing bytes rather than
   the text makes it independent of hex case. */
static int checksum(int t, const uint8_t *share, size_t len,
                    uint8_t out[CHECKSUM_BYTES]) {
  uint8_t md[EVP_MAX_MD_SIZE];
  unsigned mdlen = 0;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (ctx == NULL)
    return -1;
  int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1 &&
           EVP_DigestUpdate(ctx, SHARE_MAGIC, MAGIC_LEN) == 1 &&
           EVP_DigestUpdate(ctx, &(uint8_t){(uint8_t)t}, 1) == 1 &&
           EVP_DigestUpdate(ctx, share, len) == 1 &&
           EVP_DigestFinal_ex(ctx, md, &mdlen) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok)
    return -1;
  memcpy(out, md, CHECKSUM_BYTES);
  return 0;
}

int share_encode(int threshold, const uint8_t *share, size_t share_len,
                 char **out) {
  if (share == NULL || out == NULL || share_len < 2 || share_len > 100000 ||
      threshold < 2 || threshold > 255)
    return SHARE_ERR_PARAM;
  *out = NULL;

  uint8_t ck[CHECKSUM_BYTES];
  if (checksum(threshold, share, share_len, ck) != 0)
    return SHARE_ERR_CRYPTO;

  /* "SSS1-" + up to 3 digits + "-" + hex + "-" + 8 hex + NUL */
  size_t cap =
      MAGIC_LEN + 1 + 3 + 1 + 2 * share_len + 1 + 2 * CHECKSUM_BYTES + 1;
  char *s = malloc(cap);
  if (s == NULL)
    return SHARE_ERR_ALLOC;

  int pre = snprintf(s, cap, "%s-%d-", SHARE_MAGIC, threshold);
  to_hex(share, share_len, s + pre);
  char *p = s + pre + 2 * share_len;
  *p++ = '-';
  to_hex(ck, CHECKSUM_BYTES, p);
  p[2 * CHECKSUM_BYTES] = '\0';

  *out = s;
  return SHARE_OK;
}

int share_decode(const char *text, int *threshold, uint8_t **share,
                 size_t *share_len) {
  if (text == NULL || threshold == NULL || share == NULL || share_len == NULL)
    return SHARE_ERR_PARAM;
  *share = NULL;
  *share_len = 0;

  /* trim whitespace */
  while (isspace((unsigned char)*text))
    text++;
  size_t n = strlen(text);
  while (n > 0 && isspace((unsigned char)text[n - 1]))
    n--;

  /* magic */
  if (n < MAGIC_LEN + 1 || strncmp(text, SHARE_MAGIC "-", MAGIC_LEN + 1) != 0)
    return SHARE_ERR_FORMAT;
  const char *p = text + MAGIC_LEN + 1, *end = text + n;

  /* threshold */
  int t = 0, digits = 0;
  while (p < end && *p >= '0' && *p <= '9' && digits < 4) {
    t = t * 10 + (*p - '0');
    p++;
    digits++;
  }
  if (digits == 0 || p >= end || *p != '-' || t < 2 || t > 255)
    return SHARE_ERR_FORMAT;
  p++;

  /* payload up to the last '-' ; the checksum is the 8 chars after it */
  const char *last = NULL;
  for (const char *q = p; q < end; q++)
    if (*q == '-')
      last = q;
  if (last == NULL || (size_t)(end - last - 1) != 2 * CHECKSUM_BYTES)
    return SHARE_ERR_FORMAT;

  size_t hexlen = (size_t)(last - p);
  if (hexlen < 4 || hexlen % 2 != 0)
    return SHARE_ERR_FORMAT;
  size_t len = hexlen / 2;

  uint8_t *buf = malloc(len);
  if (buf == NULL)
    return SHARE_ERR_ALLOC;
  uint8_t given[CHECKSUM_BYTES], expect[CHECKSUM_BYTES];

  if (from_hex(p, hexlen, buf) != 0 ||
      from_hex(last + 1, 2 * CHECKSUM_BYTES, given) != 0) {
    OPENSSL_cleanse(buf, len);
    free(buf);
    return SHARE_ERR_FORMAT;
  }
  if (checksum(t, buf, len, expect) != 0) {
    OPENSSL_cleanse(buf, len);
    free(buf);
    return SHARE_ERR_CRYPTO;
  }
  if (CRYPTO_memcmp(given, expect, CHECKSUM_BYTES) != 0) {
    OPENSSL_cleanse(buf, len);
    free(buf);
    return SHARE_ERR_CHECKSUM;
  }

  *threshold = t;
  *share = buf;
  *share_len = len;
  return SHARE_OK;
}

const char *share_strerror(int err) {
  switch (err) {
  case SHARE_OK:
    return "ok";
  case SHARE_ERR_PARAM:
    return "invalid argument";
  case SHARE_ERR_FORMAT:
    return "not a valid share (expected SSS1-<t>-<hex>-<checksum>)";
  case SHARE_ERR_CHECKSUM:
    return "checksum mismatch (share is corrupted or mistyped)";
  case SHARE_ERR_ALLOC:
    return "out of memory";
  case SHARE_ERR_CRYPTO:
    return "internal crypto error";
  default:
    return "unknown error";
  }
}
