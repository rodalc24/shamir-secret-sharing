#include "sss.h"
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("FAIL line %d: %s\n", __LINE__, #cond);                           \
      return 1;                                                                \
    }                                                                          \
  } while (0)

/* Copies the shares selected by `mask` (bit i = share i) into a contiguous
 * buffer. */
static int pick(const uint8_t *shares, size_t len, int n, unsigned mask,
                uint8_t *dst) {
  size_t ss = SSS_SHARE_SIZE(len);
  int k = 0;
  for (int i = 0; i < n; i++)
    if (mask & (1u << i))
      memcpy(dst + (size_t)k++ * ss, shares + (size_t)i * ss, ss);
  return k;
}

static int popcount(unsigned v) {
  int c = 0;
  while (v) {
    c += v & 1;
    v >>= 1;
  }
  return c;
}

/* For every subset of shares: size >= t must reconstruct, size == t-1 must not.
 */
static int test_all_subsets(int t, int n, size_t len) {
  uint8_t secret[64], out[64];
  uint8_t *shares, *subset = malloc((size_t)n * SSS_SHARE_SIZE(len));
  RAND_bytes(secret, (int)len);
  CHECK(sss_split(secret, len, t, n, &shares) == SSS_OK);

  for (unsigned mask = 1; mask < (1u << n); mask++) {
    int k = popcount(mask);
    if (k < 2)
      continue;
    pick(shares, len, n, mask, subset);
    CHECK(sss_combine(subset, k, len, out) == SSS_OK);
    if (k >= t)
      CHECK(memcmp(out, secret, len) == 0);
    else if (k == t - 1 && len >= 16)
      CHECK(memcmp(out, secret, len) != 0);
  }
  free(shares);
  free(subset);
  return 0;
}

int main(void) {
  /* GF(256) primitives */
  CHECK(sss_gf_mul(0x57, 0x83) == 0xC1); /* FIPS-197 example */
  CHECK(sss_gf_mul(0x00, 0xAB) == 0 && sss_gf_mul(0x01, 0xAB) == 0xAB);
  for (int a = 1; a < 256; a++)
    CHECK(sss_gf_mul((uint8_t)a, sss_gf_inv((uint8_t)a)) == 1);

  /* Every subset, several (t, n) combinations */
  CHECK(test_all_subsets(2, 2, 32) == 0);
  CHECK(test_all_subsets(2, 5, 32) == 0);
  CHECK(test_all_subsets(3, 5, 32) == 0);
  CHECK(test_all_subsets(5, 5, 32) == 0);
  CHECK(test_all_subsets(4, 9, 32) == 0);
  CHECK(test_all_subsets(3, 6, 1) == 0); /* 1-byte secret */

  /* Maximum: 255 of 255 */
  {
    uint8_t secret[32], out[32], *shares;
    RAND_bytes(secret, sizeof secret);
    CHECK(sss_split(secret, sizeof secret, 255, 255, &shares) == SSS_OK);
    CHECK(sss_combine(shares, 255, sizeof secret, out) == SSS_OK);
    CHECK(memcmp(out, secret, sizeof secret) == 0);
    free(shares);
  }

  /* Invalid parameters */
  {
    uint8_t secret[32] = {0}, out[32], *shares;
    CHECK(sss_split(secret, 32, 1, 5, &shares) == SSS_ERR_PARAM); /* t < 2 */
    CHECK(sss_split(secret, 32, 6, 5, &shares) == SSS_ERR_PARAM); /* t > n */
    CHECK(sss_split(secret, 32, 3, 256, &shares) ==
          SSS_ERR_PARAM);                                        /* n > 255 */
    CHECK(sss_split(secret, 0, 3, 5, &shares) == SSS_ERR_PARAM); /* empty */
    CHECK(sss_split(NULL, 32, 3, 5, &shares) == SSS_ERR_PARAM);

    CHECK(sss_split(secret, 32, 2, 3, &shares) == SSS_OK);
    size_t ss = SSS_SHARE_SIZE(32);
    uint8_t two[2 * 33];
    memcpy(two, shares, ss);
    memcpy(two + ss, shares, ss); /* duplicate x */
    CHECK(sss_combine(two, 2, 32, out) == SSS_ERR_PARAM);
    memcpy(two + ss, shares + ss, ss);
    two[0] = 0; /* x = 0 */
    CHECK(sss_combine(two, 2, 32, out) == SSS_ERR_PARAM);
    CHECK(sss_combine(shares, 1, 32, out) == SSS_ERR_PARAM); /* k < 2 */
    free(shares);
  }

  /* A corrupted share yields a wrong secret (Shamir can't detect it itself) */
  {
    uint8_t secret[32], out[32], *shares;
    RAND_bytes(secret, sizeof secret);
    CHECK(sss_split(secret, 32, 3, 5, &shares) == SSS_OK);
    shares[5] ^= 1;
    CHECK(sss_combine(shares, 3, 32, out) == SSS_OK);
    CHECK(memcmp(out, secret, 32) != 0);
    free(shares);
  }

  printf("All tests passed.\n");
  return 0;
}
