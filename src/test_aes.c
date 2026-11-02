#include "aes.h"
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

int main(void) {
  uint8_t key[AES_KEY_SIZE], key2[AES_KEY_SIZE];
  CHECK(aes_generate_key(key) == AES_OK);
  CHECK(aes_generate_key(key2) == AES_OK);
  CHECK(memcmp(key, key2, AES_KEY_SIZE) != 0);

  const char *msg = "Hello, Shamir!";
  size_t msg_len = strlen(msg);
  uint8_t *ct, *pt;
  size_t ct_len, pt_len;

  /* round trip */
  CHECK(aes_encrypt(key, (const uint8_t *)msg, msg_len, &ct, &ct_len) ==
        AES_OK);
  CHECK(ct_len == msg_len + AES_OVERHEAD);
  CHECK(aes_decrypt(key, ct, ct_len, &pt, &pt_len) == AES_OK);
  CHECK(pt_len == msg_len && memcmp(pt, msg, msg_len) == 0);
  free(pt);

  /* wrong key */
  CHECK(aes_decrypt(key2, ct, ct_len, &pt, &pt_len) == AES_ERR_AUTH);

  /* tampered ciphertext */
  ct[AES_NONCE_SIZE] ^= 1;
  CHECK(aes_decrypt(key, ct, ct_len, &pt, &pt_len) == AES_ERR_AUTH);
  free(ct);

  /* empty message */
  CHECK(aes_encrypt(key, NULL, 0, &ct, &ct_len) == AES_OK);
  CHECK(ct_len == AES_OVERHEAD);
  CHECK(aes_decrypt(key, ct, ct_len, &pt, &pt_len) == AES_OK);
  CHECK(pt_len == 0);
  free(pt);
  free(ct);

  /* too-short input */
  uint8_t tiny[5] = {0};
  CHECK(aes_decrypt(key, tiny, sizeof tiny, &pt, &pt_len) == AES_ERR_PARAM);

  printf("All tests passed.\n");
  return 0;
}
