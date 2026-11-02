/*
 * shamir: encrypt a file with a random AES-256-GCM key and split that key
 * with Shamir's Secret Sharing.
 *
 *   shamir encrypt <file> -t <threshold> -n <shares> [-o <prefix>]
 *   shamir decrypt <file.enc> <share>... [-o <output>]
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/crypto.h>

#include "aes.h"
#include "share.h"
#include "sss.h"

#define KEY_SHARE_LEN SSS_SHARE_SIZE(AES_KEY_SIZE) /* 33 bytes */

static void usage(void) {
  fprintf(
      stderr,
      "Usage:\n"
      "  shamir encrypt <file> -t <threshold> -n <shares> [-o <prefix>]\n"
      "      Writes <prefix>.enc and <prefix>.share1 ... <prefix>.share<n>\n"
      "      (prefix defaults to <file>). Any <threshold> shares can decrypt.\n"
      "  shamir decrypt <file.enc> <share>... [-o <output>]\n"
      "      Each <share> is a share file, or the share text itself "
      "(SSS1-...).\n"
      "      Output defaults to <file.enc> without .enc; use -o - for "
      "stdout.\n");
}

/* ------------------------------ file helpers ------------------------------ */

static int read_file(const char *path, uint8_t **buf, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "error: cannot open '%s': %s\n", path, strerror(errno));
    return -1;
  }

  size_t cap = 4096, n = 0;
  uint8_t *b = malloc(cap);
  if (b == NULL) {
    fclose(f);
    fprintf(stderr, "error: out of memory\n");
    return -1;
  }

  for (;;) {
    if (n == cap) {
      cap *= 2;
      uint8_t *nb = realloc(b, cap);
      if (nb == NULL) {
        OPENSSL_cleanse(b, n);
        free(b);
        fclose(f);
        fprintf(stderr, "error: out of memory\n");
        return -1;
      }
      b = nb;
    }
    size_t r = fread(b + n, 1, cap - n, f);
    n += r;
    if (r == 0)
      break;
  }
  if (ferror(f)) {
    fprintf(stderr, "error: reading '%s' failed\n", path);
    OPENSSL_cleanse(b, n);
    free(b);
    fclose(f);
    return -1;
  }
  fclose(f);
  *buf = b;
  *len = n;
  return 0;
}

/* Creates a new file (never overwrites). path "-" writes to stdout. */
static int write_new_file(const char *path, const void *data, size_t len,
                          mode_t mode) {
  if (strcmp(path, "-") == 0) {
    if (fwrite(data, 1, len, stdout) != len || fflush(stdout) != 0) {
      fprintf(stderr, "error: writing to stdout failed\n");
      return -1;
    }
    return 0;
  }
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);
  if (fd < 0) {
    if (errno == EEXIST)
      fprintf(stderr, "error: '%s' already exists (refusing to overwrite)\n",
              path);
    else
      fprintf(stderr, "error: cannot create '%s': %s\n", path, strerror(errno));
    return -1;
  }
  const uint8_t *p = data;
  size_t left = len;
  while (left > 0) {
    ssize_t w = write(fd, p, left);
    if (w < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "error: writing '%s' failed: %s\n", path,
              strerror(errno));
      close(fd);
      unlink(path);
      return -1;
    }
    p += w;
    left -= (size_t)w;
  }
  if (close(fd) != 0) {
    fprintf(stderr, "error: closing '%s' failed: %s\n", path, strerror(errno));
    unlink(path);
    return -1;
  }
  return 0;
}

static int parse_int(const char *s) {
  char *end;
  long v = strtol(s, &end, 10);
  if (*s == '\0' || *end != '\0' || v < 0 || v > 100000)
    return -1;
  return (int)v;
}

static char *make_path(const char *prefix, const char *suffix, int num) {
  size_t cap = strlen(prefix) + strlen(suffix) + 16;
  char *p = malloc(cap);
  if (p == NULL)
    return NULL;
  if (num > 0)
    snprintf(p, cap, "%s%s%d", prefix, suffix, num);
  else
    snprintf(p, cap, "%s%s", prefix, suffix);
  return p;
}

/* -------------------------------- encrypt --------------------------------- */

static int cmd_encrypt(int argc, char **argv) {
  const char *input = NULL, *prefix = NULL;
  int t = -1, n = -1;

  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-t") && i + 1 < argc)
      t = parse_int(argv[++i]);
    else if (!strcmp(argv[i], "-n") && i + 1 < argc)
      n = parse_int(argv[++i]);
    else if (!strcmp(argv[i], "-o") && i + 1 < argc)
      prefix = argv[++i];
    else if (argv[i][0] != '-' && input == NULL)
      input = argv[i];
    else {
      usage();
      return 1;
    }
  }
  if (input == NULL || t < 0 || n < 0) {
    usage();
    return 1;
  }
  if (t < 2 || n < t || n > SSS_MAX_SHARES) {
    fprintf(stderr, "error: need 2 <= threshold <= shares <= %d\n",
            SSS_MAX_SHARES);
    return 1;
  }
  if (prefix == NULL)
    prefix = input;

  /* Work out all output paths and make sure none exists before doing anything.
   */
  char **paths =
      calloc((size_t)n + 1, sizeof *paths); /* [0] = .enc, [i] = share i */
  if (paths == NULL) {
    fprintf(stderr, "error: out of memory\n");
    return 1;
  }
  int rc = 1;
  uint8_t *data = NULL, *enc = NULL, *shares = NULL;
  size_t data_len = 0, enc_len = 0;
  uint8_t key[AES_KEY_SIZE];
  int created = 0;

  paths[0] = make_path(prefix, ".enc", 0);
  for (int i = 1; i <= n; i++)
    paths[i] = make_path(prefix, ".share", i);
  for (int i = 0; i <= n; i++) {
    if (paths[i] == NULL) {
      fprintf(stderr, "error: out of memory\n");
      goto done;
    }
    if (access(paths[i], F_OK) == 0) {
      fprintf(stderr, "error: '%s' already exists (refusing to overwrite)\n",
              paths[i]);
      goto done;
    }
  }

  if (read_file(input, &data, &data_len) != 0)
    goto done;

  int e;
  if ((e = aes_generate_key(key)) != AES_OK) {
    fprintf(stderr, "error: key generation failed (%d)\n", e);
    goto done;
  }
  if ((e = aes_encrypt(key, data, data_len, &enc, &enc_len)) != AES_OK) {
    if (e == AES_ERR_PARAM)
      fprintf(stderr, "error: file too large (limit is about 2 GB)\n");
    else
      fprintf(stderr, "error: encryption failed (%d)\n", e);
    goto done;
  }
  if ((e = sss_split(key, AES_KEY_SIZE, t, n, &shares)) != SSS_OK) {
    fprintf(stderr, "error: splitting key failed (%d)\n", e);
    goto done;
  }

  /* Encode everything first, so a failure leaves no partial output. */
  char **lines = calloc((size_t)n, sizeof *lines);
  if (lines == NULL) {
    fprintf(stderr, "error: out of memory\n");
    goto done;
  }
  for (int i = 0; i < n; i++) {
    e = share_encode(t, shares + (size_t)i * KEY_SHARE_LEN, KEY_SHARE_LEN,
                     &lines[i]);
    if (e != SHARE_OK) {
      fprintf(stderr, "error: %s\n", share_strerror(e));
      goto free_lines;
    }
  }

  if (write_new_file(paths[0], enc, enc_len, 0644) != 0)
    goto free_lines;
  created = 1;
  for (int i = 1; i <= n; i++) {
    char *line = lines[i - 1];
    size_t ll = strlen(line);
    line[ll] = '\n'; /* capacity includes the NUL slot */
    int w = write_new_file(paths[i], line, ll + 1, 0600);
    line[ll] = '\0';
    if (w != 0)
      goto free_lines;
    created = i + 1;
  }

  printf("Encrypted '%s' -> '%s'\n", input, paths[0]);
  printf("Wrote %d shares (%s ... %s); any %d of them can decrypt.\n", n,
         paths[1], paths[n], t);
  rc = 0;

free_lines:
  if (rc != 0)
    for (int i = 0; i < created; i++)
      unlink(paths[i]); /* roll back partial output */
  for (int i = 0; i < n; i++) {
    if (lines[i]) {
      OPENSSL_cleanse(lines[i], strlen(lines[i]));
      free(lines[i]);
    }
  }
  free(lines);
done:
  OPENSSL_cleanse(key, sizeof key);
  if (data) {
    OPENSSL_cleanse(data, data_len);
    free(data);
  }
  if (shares) {
    OPENSSL_cleanse(shares, (size_t)n * KEY_SHARE_LEN);
    free(shares);
  }
  free(enc);
  for (int i = 0; i <= n; i++)
    free(paths[i]);
  free(paths);
  return rc;
}

/* -------------------------------- decrypt --------------------------------- */

static int cmd_decrypt(int argc, char **argv) {
  const char *enc_path = NULL, *out_path = NULL;
  const char **share_args = calloc((size_t)argc + 1, sizeof *share_args);
  int nshare_args = 0;
  if (share_args == NULL) {
    fprintf(stderr, "error: out of memory\n");
    return 1;
  }

  int rc = 1;
  uint8_t *all = NULL, *enc = NULL, *pt = NULL;
  size_t enc_len = 0, pt_len = 0;
  uint8_t key[AES_KEY_SIZE];
  char *default_out = NULL;
  int count = 0, t = 0;

  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-o") && i + 1 < argc)
      out_path = argv[++i];
    else if (!strcmp(argv[i], "-") || argv[i][0] != '-') {
      if (enc_path == NULL)
        enc_path = argv[i];
      else
        share_args[nshare_args++] = argv[i];
    } else {
      usage();
      goto done;
    }
  }
  if (enc_path == NULL || nshare_args == 0) {
    usage();
    goto done;
  }

  /* Load and validate shares */
  all = malloc((size_t)nshare_args * KEY_SHARE_LEN);
  if (all == NULL) {
    fprintf(stderr, "error: out of memory\n");
    goto done;
  }

  for (int i = 0; i < nshare_args; i++) {
    const char *arg = share_args[i];
    char *text = NULL;
    uint8_t *fbuf = NULL;
    size_t flen = 0;

    if (strncmp(arg, SHARE_MAGIC "-", strlen(SHARE_MAGIC) + 1) == 0) {
      text = strdup(arg);
    } else {
      if (read_file(arg, &fbuf, &flen) != 0)
        goto done;
      text = malloc(flen + 1);
      if (text) {
        memcpy(text, fbuf, flen);
        text[flen] = '\0';
      }
      OPENSSL_cleanse(fbuf, flen);
      free(fbuf);
    }
    if (text == NULL) {
      fprintf(stderr, "error: out of memory\n");
      goto done;
    }

    int st;
    uint8_t *sh;
    size_t sl;
    int e = share_decode(text, &st, &sh, &sl);
    OPENSSL_cleanse(text, strlen(text));
    free(text);
    if (e != SHARE_OK) {
      fprintf(stderr, "error: share %d (%.40s): %s\n", i + 1, arg,
              share_strerror(e));
      goto done;
    }
    if (sl != KEY_SHARE_LEN) {
      fprintf(stderr,
              "error: share %d (%.40s): wrong size, this is not a share of an "
              "AES key\n",
              i + 1, arg);
      OPENSSL_cleanse(sh, sl);
      free(sh);
      goto done;
    }
    if (count == 0)
      t = st;
    else if (st != t) {
      fprintf(stderr,
              "error: share %d (%.40s) has threshold %d but earlier shares "
              "have %d; "
              "they belong to different sets\n",
              i + 1, arg, st, t);
      OPENSSL_cleanse(sh, sl);
      free(sh);
      goto done;
    }
    for (int j = 0; j < count; j++) {
      if (all[(size_t)j * KEY_SHARE_LEN] == sh[0]) {
        fprintf(stderr, "error: shares %d and %d are the same share (x = %d)\n",
                j + 1, i + 1, sh[0]);
        OPENSSL_cleanse(sh, sl);
        free(sh);
        goto done;
      }
    }
    memcpy(all + (size_t)count * KEY_SHARE_LEN, sh, KEY_SHARE_LEN);
    OPENSSL_cleanse(sh, sl);
    free(sh);
    count++;
  }

  if (count < t) {
    fprintf(stderr,
            "error: %d shares are needed to decrypt, but only %d given\n", t,
            count);
    goto done;
  }

  int e = sss_combine(all, t, AES_KEY_SIZE, key); /* uses the first t shares */
  if (e != SSS_OK) {
    fprintf(stderr, "error: combining shares failed (%d)\n", e);
    goto done;
  }

  size_t enc_size;
  if (read_file(enc_path, &enc, &enc_size) != 0)
    goto done;
  enc_len = enc_size;

  e = aes_decrypt(key, enc, enc_len, &pt, &pt_len);
  if (e == AES_ERR_AUTH) {
    fprintf(stderr,
            "error: decryption failed. The shares do not belong to this file, "
            "a share was\n"
            "altered, or '%s' is corrupted.\n",
            enc_path);
    goto done;
  } else if (e == AES_ERR_PARAM) {
    fprintf(stderr, "error: '%s' is not a valid encrypted file (too short)\n",
            enc_path);
    goto done;
  } else if (e != AES_OK) {
    fprintf(stderr, "error: decryption failed (%d)\n", e);
    goto done;
  }

  if (out_path == NULL) {
    size_t l = strlen(enc_path);
    if (l > 4 && strcmp(enc_path + l - 4, ".enc") == 0) {
      default_out = strndup(enc_path, l - 4);
    } else {
      default_out = make_path(enc_path, ".dec", 0);
    }
    if (default_out == NULL) {
      fprintf(stderr, "error: out of memory\n");
      goto done;
    }
    out_path = default_out;
  }
  if (strcmp(out_path, "-") != 0 && access(out_path, F_OK) == 0) {
    fprintf(stderr,
            "error: '%s' already exists (refusing to overwrite); use -o "
            "<output> to choose another name\n",
            out_path);
    goto done;
  }
  if (write_new_file(out_path, pt, pt_len, 0600) != 0)
    goto done;
  if (strcmp(out_path, "-") != 0)
    fprintf(stderr, "Decrypted '%s' -> '%s' (used %d of %d given shares)\n",
            enc_path, out_path, t, count);
  rc = 0;

done:
  OPENSSL_cleanse(key, sizeof key);
  if (all) {
    OPENSSL_cleanse(all, (size_t)nshare_args * KEY_SHARE_LEN);
    free(all);
  }
  if (pt) {
    OPENSSL_cleanse(pt, pt_len);
    free(pt);
  }
  free(enc);
  free(default_out);
  free(share_args);
  return rc;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }
  if (!strcmp(argv[1], "encrypt"))
    return cmd_encrypt(argc - 2, argv + 2);
  if (!strcmp(argv[1], "decrypt"))
    return cmd_decrypt(argc - 2, argv + 2);
  usage();
  return 1;
}
