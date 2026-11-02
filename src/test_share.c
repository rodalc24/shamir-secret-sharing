#include "share.h"
#include <ctype.h>
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
  uint8_t raw[33];
  for (int i = 0; i < 33; i++)
    raw[i] = (uint8_t)(i * 7 + 3);

  char *s;
  CHECK(share_encode(3, raw, sizeof raw, &s) == SHARE_OK);
  CHECK(strncmp(s, "SSS1-3-", 7) == 0);
  CHECK(strlen(s) == 7 + 66 + 1 + 8);
  printf("example: %s\n", s);

  /* round trip */
  int t;
  uint8_t *out;
  size_t olen;
  CHECK(share_decode(s, &t, &out, &olen) == SHARE_OK);
  CHECK(t == 3 && olen == sizeof raw && memcmp(out, raw, olen) == 0);
  free(out);

  /* whitespace and uppercase are tolerated */
  size_t n = strlen(s);
  char *w = malloc(n + 8);
  snprintf(w, n + 8, "  \n");
  for (size_t i = 0; i < n; i++)
    w[3 + i] = (i >= 5) ? (char)toupper((unsigned char)s[i]) : s[i];
  strcpy(w + 3 + n, "\r\n");
  CHECK(share_decode(w, &t, &out, &olen) == SHARE_OK);
  CHECK(olen == sizeof raw && memcmp(out, raw, olen) == 0);
  free(out);
  free(w);

  /* any single-character change in the payload or checksum is caught */
  for (size_t i = 7; i < n; i++) {
    if (s[i] == '-')
      continue;
    char *c = strdup(s);
    c[i] = (c[i] == '0') ? '1' : '0';
    int e = share_decode(c, &t, &out, &olen);
    CHECK(e == SHARE_ERR_CHECKSUM);
    free(c);
  }

  /* changed threshold is caught too */
  char *c = strdup(s);
  c[5] = '4';
  CHECK(share_decode(c, &t, &out, &olen) == SHARE_ERR_CHECKSUM);
  free(c);

  /* malformed input */
  CHECK(share_decode("", &t, &out, &olen) == SHARE_ERR_FORMAT);
  CHECK(share_decode("hello world", &t, &out, &olen) == SHARE_ERR_FORMAT);
  CHECK(share_decode("SSS2-3-0102-deadbeef", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* version */
  CHECK(share_decode("SSS1-1-0102-deadbeef", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* t < 2 */
  CHECK(share_decode("SSS1-x-0102-deadbeef", &t, &out, &olen) ==
        SHARE_ERR_FORMAT);
  CHECK(share_decode("SSS1-3-010-deadbeef", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* odd hex */
  CHECK(share_decode("SSS1-3-01zz-deadbeef", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* non-hex */
  CHECK(share_decode("SSS1-3-0102-dead", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* short checksum */
  CHECK(share_decode("SSS1-3-0102", &t, &out, &olen) ==
        SHARE_ERR_FORMAT); /* no checksum */
  CHECK(share_decode(NULL, &t, &out, &olen) == SHARE_ERR_PARAM);

  /* bad encode params */
  char *bad;
  CHECK(share_encode(1, raw, sizeof raw, &bad) == SHARE_ERR_PARAM);
  CHECK(share_encode(256, raw, sizeof raw, &bad) == SHARE_ERR_PARAM);
  CHECK(share_encode(3, raw, 1, &bad) == SHARE_ERR_PARAM);

  free(s);
  printf("All tests passed.\n");
  return 0;
}
