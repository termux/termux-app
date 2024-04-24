
/*
 * BIG5-0 and BIG5-1
 */

/*
   BIG5 with its 13494 characters doesn't fit in a single 94x94 or 96x96
   block. Therefore Emacs/Mule developers, in a typically Japanese way of
   thinking, have developed an alternative encoding of BIG5 in two 94x94
   planes, very similar to the SHIFT_JIS encoding for JISX0208.

   Conversion between BIG5 codes (s1,s2) and BIG5-0 codes (c1,c2):
   Example. (s1,s2) = 0xA140, (c1,c2) = 0x2121.
   0xA1 <= s1 <= 0xC7, 0x40 <= s2 <= 0x7E || 0xA1 <= s2 <= 0xFE,
   0x21 <= c1 <= 0x62, 0x21 <= c2 <= 0x7E.
   Invariant:
     157*(s1-0xA1) + (s2 < 0x80 ? s2-0x40 : s2-0x62)
     = 94*(c1-0x21)+(c2-0x21)
   Conversion (s1,s2) -> (c1,c2):
     t := 157*(s1-0xA1) + (s2 < 0x80 ? s2-0x40 : s2-0x62)
     c1 := (t div 94) + 0x21
     c2 := (t mod 94) + 0x21
   Conversion (c1,c2) -> (s1,s2):
     t := 94*(c1-0x21)+(c2-0x21)
     t2 := t mod 157
     s1 := (t div 157) + 0xA1
     s2 := (t2 < 0x3F ? t2+0x40 : t2+0x62)

   Conversion between BIG5 codes (s1,s2) and BIG5-1 codes (c1,c2):
   Example. (s1,s2) = 0xC940, (c1,c2) = 0x2121.
   0xC9 <= s1 <= 0xF9, 0x40 <= s2 <= 0x7E || 0xA1 <= s2 <= 0xFE,
   0x21 <= c1 <= 0x72, 0x21 <= c2 <= 0x7E.
   Invariant:
     157*(s1-0xC9) + (s2 < 0x80 ? s2-0x40 : s2-0x62)
     = 94*(c1-0x21)+(c2-0x21)
   Conversion (s1,s2) -> (c1,c2):
     t := 157*(s1-0xC9) + (s2 < 0x80 ? s2-0x40 : s2-0x62)
     c1 := (t div 94) + 0x21
     c2 := (t mod 94) + 0x21
   Conversion (c1,c2) -> (s1,s2):
     t := 94*(c1-0x21)+(c2-0x21)
     t2 := t mod 157
     s1 := (t div 157) + 0xC9
     s2 := (t2 < 0x3F ? t2+0x40 : t2+0x62)
 */

static int
big5_0_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c1 = s[0];
  if (c1 >= 0x21 && c1 <= 0x62) {
    if (n >= 2) {
      unsigned char c2 = s[1];
      if (c2 >= 0x21 && c2 <= 0x7e) {
        unsigned int i = 94 * (c1 - 0x21) + (c2 - 0x21);
        if (0) {
          /* Unoptimized. */
          unsigned char buf[2];
          buf[0] = (i / 157) + 0xa1;
          i = i % 157;
          buf[1] = i + (i < 0x3f ? 0x40 : 0x62);
          return big5_mbtowc(conv,pwc,buf,2);
        } else {
          /* Inline the implementation of big5_mbtowc. */
          if (i < 6121) {
            unsigned short wc = big5_2uni_pagea1[i];
            if (wc != 0xfffd) {
              *pwc = (ucs4_t) wc;
              return 2;
            }
          }
        }
      }
      return RET_ILSEQ;
    }
    return RET_TOOFEW(0);
  }
  return RET_ILSEQ;
}

static int
big5_1_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c1 = s[0];
  if (c1 >= 0x21 && c1 <= 0x72) {
    if (n >= 2) {
      unsigned char c2 = s[1];
      if (c2 >= 0x21 && c2 <= 0x7e) {
        unsigned int i = 94 * (c1 - 0x21) + (c2 - 0x21);
        if (0) {
          /* Unoptimized. */
          unsigned char buf[2];
          buf[0] = (i / 157) + 0xc9;
          i = i % 157;
          buf[1] = i + (i < 0x3f ? 0x40 : 0x62);
          return big5_mbtowc(conv,pwc,buf,2);
        } else {
          /* Inline the implementation of big5_mbtowc. */
          if (i < 7652) {
            unsigned short wc = big5_2uni_pagec9[i];
            if (wc != 0xfffd) {
              *pwc = (ucs4_t) wc;
              return 2;
            }
          }
        }
      }
      return RET_ILSEQ;
    }
    return RET_TOOFEW(0);
  }
  return RET_ILSEQ;
}

static int
big5_0_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (n >= 2) {
    unsigned char buf[2];
    int ret = big5_wctomb(conv,buf,wc,2);
    if (ret != RET_ILSEQ) {
      unsigned char s1, s2;
      if (ret != 2) abort();
      s1 = buf[0];
      s2 = buf[1];
      if (!(s1 >= 0xa1)) abort();
      if (!((s2 >= 0x40 && s2 <= 0x7e) || (s2 >= 0xa1 && s2 <= 0xfe))) abort();
      if (s1 < 0xc9) {
        unsigned int t = 157 * (s1 - 0xa1) + s2 - (s2 < 0x80 ? 0x40 : 0x62);
        r[0] = (t / 94) + 0x21;
        r[1] = (t % 94) + 0x21;
        return 2;
      }
    }
    return RET_ILSEQ;
  }
  return RET_TOOSMALL;
}

static int
big5_1_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (n >= 2) {
    unsigned char buf[2];
    int ret = big5_wctomb(conv,buf,wc,2);
    if (ret != RET_ILSEQ) {
      unsigned char s1, s2;
      if (ret != 2) abort();
      s1 = buf[0];
      s2 = buf[1];
      if (!(s1 <= 0xf9)) abort();
      if (!((s2 >= 0x40 && s2 <= 0x7e) || (s2 >= 0xa1 && s2 <= 0xfe))) abort();
      if (s1 >= 0xc9) {
        unsigned int t = 157 * (s1 - 0xc9) + s2 - (s2 < 0x80 ? 0x40 : 0x62);
        r[0] = (t / 94) + 0x21;
        r[1] = (t % 94) + 0x21;
        return 2;
      }
    }
    return RET_ILSEQ;
  }
  return RET_TOOSMALL;
}
