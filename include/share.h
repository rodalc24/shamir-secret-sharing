#ifndef SHARE_H
#define SHARE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Text format for one share:
 *
 *     SSS1-<t>-<hex of x|y0|y1|...>-<checksum>
 *
 *   SSS1      magic + format version
 *   <t>       threshold (decimal): how many shares are needed
 *   hex       the raw share bytes from sss_split (hex, case-insensitive)
 *   checksum  first 4 bytes of SHA-256("SSS1" || t || share bytes), as 8 hex
 *             chars. Catches typos and corruption; it is NOT a MAC, so it does
 *             not protect against a deliberately forged share (AES-GCM does
 *             catch a wrong reconstructed key).
 */

#define SHARE_OK 0
#define SHARE_ERR_PARAM -1
#define SHARE_ERR_FORMAT -2   /* not a share / malformed */
#define SHARE_ERR_CHECKSUM -3 /* well-formed but corrupted or mistyped */
#define SHARE_ERR_ALLOC -4
#define SHARE_ERR_CRYPTO -5

#define SHARE_MAGIC "SSS1"

/* Encodes a share. *out is a malloc'd NUL-terminated string; caller frees. */
int share_encode(int threshold, const uint8_t *share, size_t share_len,
                 char **out);

/*
 * Decodes a share string (leading/trailing whitespace is ignored).
 * On success, *share is malloc'd (caller wipes and frees), *share_len its size,
 * *threshold the threshold stored in the share.
 */
int share_decode(const char *text, int *threshold, uint8_t **share,
                 size_t *share_len);

/* Human-readable description of a SHARE_ERR_* code. */
const char *share_strerror(int err);

#endif /* SHARE_H */
