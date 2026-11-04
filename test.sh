#!/usr/bin/env sh
# End-to-end test of the sss CLI. Usage: ./test.sh [path/to/sss]
BIN=$(cd "$(dirname "${1:-./bin/sss}")" && pwd)/$(basename "${1:-./bin/sss}")
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
cd "$DIR" || exit 1

fail() { echo "FAIL: $1"; exit 1; }
ok()   { echo "ok: $1"; }

# ---- a binary test file (with NUL bytes) and an empty one
head -c 100000 /dev/urandom > data.bin
: > empty.bin

# ---- basic 3-of-5 round trip, using several different subsets of shares
"$BIN" encrypt data.bin -k 3 -n 5 >/dev/null || fail "encrypt"
[ -f data.bin.enc ] && [ -f data.bin.share5 ] || fail "output files missing"
ok "encrypt writes .enc and 5 shares"

for subset in "1 2 3" "3 4 5" "1 3 5" "2 4 5 1"; do
    files=""; for i in $subset; do files="$files data.bin.share$i"; done
    rm -f out.bin
    "$BIN" decrypt data.bin.enc $files -o out.bin 2>/dev/null || fail "decrypt with shares $subset"
    cmp -s data.bin out.bin || fail "content mismatch with shares $subset"
done
ok "any subset of >= 3 shares decrypts correctly"

# ---- too few shares
"$BIN" decrypt data.bin.enc data.bin.share1 data.bin.share2 -o out2.bin 2>err.txt && fail "2 shares should not decrypt"
grep -q "3 shares are needed" err.txt || fail "missing 'need 3 shares' message"
[ ! -e out2.bin ] || fail "no output should be written"
ok "too few shares rejected with a clear message"

# ---- literal share text instead of a file
S1=$(cat data.bin.share1); S2=$(cat data.bin.share2); S3=$(cat data.bin.share3)
"$BIN" decrypt data.bin.enc "$S1" "$S2" "$S3" -o - 2>/dev/null | cmp -s - data.bin || fail "literal shares / stdout"
ok "shares can be passed as text, output to stdout"

# ---- corrupted share (one character changed)
sed 's/^\(SSS1-3-.\{10\}\)./\1z/' data.bin.share2 > bad.share
"$BIN" decrypt data.bin.enc data.bin.share1 bad.share data.bin.share3 -o out3.bin 2>err.txt && fail "corrupt share accepted"
grep -q "share 2" err.txt && grep -qi "checksum" err.txt || fail "corrupt share not identified"
ok "corrupted share identified by position"

# ---- shares from a different encryption
cp data.bin other.bin
"$BIN" encrypt other.bin -k 3 -n 5 >/dev/null || fail "second encrypt"
"$BIN" decrypt data.bin.enc data.bin.share1 data.bin.share2 other.bin.share3 -o out4.bin 2>err.txt && fail "mixed shares accepted"
grep -q "do not belong" err.txt || fail "mixed shares: wrong message"
ok "shares from another file fail authentication"

# ---- different threshold in the set
"$BIN" encrypt other.bin -k 2 -n 3 -o other2 >/dev/null || fail "encrypt t=2"
"$BIN" decrypt data.bin.enc data.bin.share1 other2.share1 data.bin.share3 -o out5.bin 2>err.txt && fail "mixed thresholds accepted"
grep -q "different sets" err.txt || fail "mixed thresholds: wrong message"
ok "mixed thresholds detected"

# ---- duplicate share
"$BIN" decrypt data.bin.enc data.bin.share1 data.bin.share1 data.bin.share2 -o out6.bin 2>err.txt && fail "duplicate accepted"
grep -q "same share" err.txt || fail "duplicate: wrong message"
ok "duplicate share detected"

# ---- tampered ciphertext
cp data.bin.enc tampered.enc
printf '\377' | dd of=tampered.enc bs=1 seek=100 conv=notrunc 2>/dev/null
"$BIN" decrypt tampered.enc data.bin.share1 data.bin.share2 data.bin.share3 -o out7.bin 2>/dev/null && fail "tampered file accepted"
[ ! -e out7.bin ] || fail "plaintext written for tampered file"
ok "tampered ciphertext rejected, no output"

# ---- refuses to overwrite
"$BIN" encrypt data.bin -k 3 -n 5 2>err.txt && fail "overwrite allowed (encrypt)"
grep -q "already exists" err.txt || fail "no overwrite message"
"$BIN" decrypt data.bin.enc data.bin.share1 data.bin.share2 data.bin.share3 -o out.bin 2>err.txt && fail "overwrite allowed (decrypt)"
ok "never overwrites existing files"

# ---- default output name, empty file, bad parameters
mv data.bin data.orig
"$BIN" decrypt data.bin.enc data.bin.share2 data.bin.share4 data.bin.share5 2>/dev/null || fail "default output name"
cmp -s data.orig data.bin || fail "default output content"
ok "default output name strips .enc"

"$BIN" encrypt empty.bin -k 2 -n 2 >/dev/null || fail "encrypt empty"
"$BIN" decrypt empty.bin.enc empty.bin.share1 empty.bin.share2 -o empty.out 2>/dev/null || fail "decrypt empty"
[ -f empty.out ] && [ ! -s empty.out ] || fail "empty file round trip"
ok "empty file round trip"

"$BIN" encrypt data.orig -k 1 -n 3 -o p1 2>/dev/null && fail "t=1 accepted"
"$BIN" encrypt data.orig -k 4 -n 3 -o p2 2>/dev/null && fail "t>n accepted"
"$BIN" encrypt data.orig -k 2 -n 300 -o p3 2>/dev/null && fail "n>255 accepted"
"$BIN" encrypt missing.file -k 2 -n 3 -o p4 2>/dev/null && fail "missing input accepted"
ls p1* p2* p3* p4* >/dev/null 2>&1 && fail "failed encrypt left files behind"
ok "bad parameters rejected without leaving files"

echo "All end-to-end tests passed."
