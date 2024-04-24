/*
 * UCS-2BE = UCS-2 big endian
 */

static int
ucs2be_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  if (n >= 2) {
    if (s[0] >= 0xd8 && s[0] < 0xe0) {
      return RET_ILSEQ;
    } else {
      *pwc = (s[0] << 8) + s[1];
      return 2;
    }
  }
  return RET_TOOFEW(0);
}

static int
ucs2be_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x10000 && !(wc >= 0xd800 && wc < 0xe000)) {
    if (n >= 2) {
      r[0] = (unsigned char) (wc >> 8);
      r[1] = (unsigned char) wc;
      return 2;
    } else
      return RET_TOOSMALL;
  }
  return RET_ILSEQ;
}
