#ifndef AES_H
#define AES_H

#include <stddef.h>
#include <stdint.h>

#define AES_KEY_SIZE 32   /* AES-256 */
#define AES_NONCE_SIZE 12 /* recommended GCM nonce size */
#define AES_TAG_SIZE 16   /* GCM authentication tag */
#define AES_OVERHEAD (AES_NONCE_SIZE + AES_TAG_SIZE)

/* Return codes */
#define AES_OK 0
#define AES_ERR_PARAM -1  /* bad argument (NULL, too large, input too short) */
#define AES_ERR_RANDOM -2 /* CSPRNG failure */
#define AES_ERR_ALLOC -3  /* out of memory */
#define AES_ERR_CRYPTO -4 /* OpenSSL internal error */
#define AES_ERR_AUTH -5   /* authentication failed: wrong key, corrupted data   */

/*
 * Fills `key` with AES_KEY_SIZE random bytes from a CSPRNG.
 * Returns AES_OK or a negative error code.
 */
int aes_generate_key(uint8_t key[AES_KEY_SIZE]);

/*
 * Encrypts `content` (content_len bytes) with AES-256-GCM.
 *
 * On success, *out points to a malloc'd buffer with the layout
 *     [ nonce (12) | ciphertext (content_len) | tag (16) ]
 * and *out_len = content_len + AES_OVERHEAD. The caller must free(*out).
 * A fresh random nonce is generated for every call.
 */
int aes_encrypt(const uint8_t key[AES_KEY_SIZE], const uint8_t *content,
                size_t content_len, uint8_t **out, size_t *out_len);

/*
 * Decrypts a buffer produced by aes_encrypt().
 *
 * On success, *out points to a malloc'd buffer holding the plaintext and
 * *out_len is its length (the buffer is always at least 1 byte, so it is
 * safe to free even for empty plaintext). The caller must free(*out).
 * Returns AES_ERR_AUTH if the key is wrong or the data was modified; in that
 * case no plaintext is returned.
 */
int aes_decrypt(const uint8_t key[AES_KEY_SIZE], const uint8_t *content,
                size_t content_len, uint8_t **out, size_t *out_len);

#endif /* AES_H */
