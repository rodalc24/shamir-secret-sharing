#include "sss.h"

#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>

/* ---------- GF(2^8) arithmetic, AES polynomial x^8+x^4+x^3+x+1 ---------- */

/* Branch-free multiplication so secrets don't leak through timing. */
uint8_t sss_gf_mul(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  for (int i = 0; i < 8; i++) {
    p ^= (uint8_t)(-(b & 1)) & a;
    uint8_t carry = (uint8_t)(-(a >> 7));
    a = (uint8_t)(a << 1) ^ (carry & 0x1B);
    b >>= 1;
  }
  return p;
}

/* a^254 = a^-1 for a != 0 (and gives 0 for a == 0). */
uint8_t sss_gf_inv(uint8_t a) {
  uint8_t r = 1;
  for (int i = 0; i < 254; i++)
    r = sss_gf_mul(r, a);
  return r;
}

/* --------------------------------- split --------------------------------- */

int sss_split(const uint8_t *secret, size_t len, int threshold,
              int shares_count, uint8_t **shares_out) {
  if (secret == NULL || shares_out == NULL || len == 0)
    return SSS_ERR_PARAM;
  if (threshold < 2 || threshold > shares_count ||
      shares_count > SSS_MAX_SHARES)
    return SSS_ERR_PARAM;

  *shares_out = NULL;

  size_t share_size = SSS_SHARE_SIZE(len);
  if (share_size > SIZE_MAX / (size_t)shares_count)
    return SSS_ERR_PARAM;

  uint8_t *shares = malloc(share_size * (size_t)shares_count);
  if (shares == NULL)
    return SSS_ERR_ALLOC;

  for (int i = 0; i < shares_count; i++)
    shares[(size_t)i * share_size] = (uint8_t)(i + 1);

  uint8_t coef[SSS_MAX_SHARES]; /* coef[0] = secret byte, coef[1..t-1] random */
  int rc = SSS_OK;

  for (size_t j = 0; j < len; j++) {
    coef[0] = secret[j];
    if (RAND_bytes(coef + 1, threshold - 1) != 1) {
      rc = SSS_ERR_RANDOM;
      goto out;
    }

    for (int i = 0; i < shares_count; i++) {
      uint8_t x = (uint8_t)(i + 1), y = 0;
      for (int k = threshold - 1; k >= 0; k--) /* Horner's method */
        y = sss_gf_mul(y, x) ^ coef[k];
      shares[(size_t)i * share_size + 1 + j] = y;
    }
  }

out:
  OPENSSL_cleanse(coef, sizeof coef);
  if (rc != SSS_OK) {
    OPENSSL_cleanse(shares, share_size * (size_t)shares_count);
    free(shares);
    return rc;
  }
  *shares_out = shares;
  return SSS_OK;
}

/* -------------------------------- combine --------------------------------- */

int sss_combine(const uint8_t *shares, int k, size_t len, uint8_t *secret_out) {
  if (shares == NULL || secret_out == NULL || len == 0)
    return SSS_ERR_PARAM;
  if (k < 2 || k > SSS_MAX_SHARES)
    return SSS_ERR_PARAM;

  size_t share_size = SSS_SHARE_SIZE(len);

  /* x values must be nonzero and distinct */
  for (int i = 0; i < k; i++) {
    uint8_t xi = shares[(size_t)i * share_size];
    if (xi == 0)
      return SSS_ERR_PARAM;
    for (int m = i + 1; m < k; m++)
      if (shares[(size_t)m * share_size] == xi)
        return SSS_ERR_PARAM;
  }

  /* Lagrange coefficients at x = 0 depend only on the x values, so compute
     them once. In GF(2^8), subtraction is XOR. */
  uint8_t li[SSS_MAX_SHARES];
  for (int i = 0; i < k; i++) {
    uint8_t xi = shares[(size_t)i * share_size];
    uint8_t num = 1, den = 1;
    for (int m = 0; m < k; m++) {
      if (m == i)
        continue;
      uint8_t xm = shares[(size_t)m * share_size];
      num = sss_gf_mul(num, xm);
      den = sss_gf_mul(den, xi ^ xm);
    }
    li[i] = sss_gf_mul(num, sss_gf_inv(den));
  }

  for (size_t j = 0; j < len; j++) {
    uint8_t s = 0;
    for (int i = 0; i < k; i++)
      s ^= sss_gf_mul(shares[(size_t)i * share_size + 1 + j], li[i]);
    secret_out[j] = s;
  }
  return SSS_OK;
}
