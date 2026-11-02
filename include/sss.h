#ifndef SSS_H
#define SSS_H

#include <stddef.h>
#include <stdint.h>

/* Return codes */
#define SSS_OK 0
#define SSS_ERR_PARAM                                                          \
  -1 /* bad argument (NULL, bad threshold, duplicate x, ...) */
#define SSS_ERR_RANDOM -2 /* CSPRNG failure */
#define SSS_ERR_ALLOC -3  /* out of memory */

#define SSS_MAX_SHARES 255

/* Size in bytes of one share for a secret of `len` bytes: [x | y0 ... y(len-1)]
 */
#define SSS_SHARE_SIZE(len) ((len) + 1)

/*
 * Splits `secret` (len bytes) into `shares_count` shares, any `threshold` of
 * which reconstruct it. Works byte by byte in GF(2^8), so
 *     2 <= threshold <= shares_count <= 255.
 *
 * On success, *shares_out points to a malloc'd buffer of
 * shares_count * SSS_SHARE_SIZE(len) bytes: share i starts at offset
 * i * SSS_SHARE_SIZE(len), its first byte is its x coordinate
 * (1..shares_count). The caller must wipe (OPENSSL_cleanse) and free it.
 */
int sss_split(const uint8_t *secret, size_t len, int threshold,
              int shares_count, uint8_t **shares_out);

/*
 * Reconstructs a secret of `len` bytes from `k` shares laid out contiguously
 * (each SSS_SHARE_SIZE(len) bytes). Share x values must be nonzero and
 * distinct.
 *
 * IMPORTANT: Shamir cannot tell whether k >= threshold, or whether a share is
 * corrupted; it silently returns garbage in those cases. Detect that one level
 * up (e.g. the AES-GCM tag, or a checksum on each share).
 */
int sss_combine(const uint8_t *shares, int k, size_t len, uint8_t *secret_out);

/* GF(2^8) primitives (AES polynomial 0x11B), exposed for testing. */
uint8_t sss_gf_mul(uint8_t a, uint8_t b);
uint8_t sss_gf_inv(uint8_t a); /* inverse of a != 0; returns 0 for 0 */

#endif /* SSS_H */
