/******************************************************************

              Copyright 1993 by SunSoft, Inc.
              Copyright 1999-2000 by Bruno Haible

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the names of SunSoft, Inc. and
Bruno Haible not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.  SunSoft, Inc. and Bruno Haible make no representations
about the suitability of this software for any purpose.  It is
provided "as is" without express or implied warranty.

SunSoft Inc. AND Bruno Haible DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL SunSoft, Inc. OR Bruno Haible BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
 * This file contains:
 *
 * I. Conversion routines CompoundText/CharSet <--> Unicode/UTF-8.
 *
 *    Used for three purposes:
 *      1. The UTF-8 locales, see below.
 *      2. Unicode aware applications for which the use of 8-bit character
 *         sets is an anachronism.
 *      3. For conversion from keysym to locale encoding.
 *
 * II. Conversion files for an UTF-8 locale loader.
 *     Supports: all locales with codeset UTF-8.
 *     How: Provides converters for UTF-8.
 *     Platforms: all systems.
 *
 * The loader itself is located in lcUTF8.c.
 */

/*
 * The conversion from UTF-8 to CompoundText is realized in a very
 * conservative way. Recall that CompoundText data is used for inter-client
 * communication purposes. We distinguish three classes of clients:
 * - Clients which accept only those pieces of CompoundText which belong to
 *   the character set understood by the current locale.
 *   (Example: clients which are linked to an older X11 library.)
 * - Clients which accept CompoundText with multiple character sets and parse
 *   it themselves.
 *   (Example: emacs, xemacs.)
 * - Clients which rely entirely on the X{mb,wc}TextPropertyToTextList
 *   functions for the conversion of CompoundText to their current locale's
 *   multi-byte/wide-character format.
 * For best interoperation, the UTF-8 to CompoundText conversion proceeds as
 * follows. For every character, it first tests whether the character is
 * representable in the current locale's original (non-UTF-8) character set.
 * If not, it goes through the list of predefined character sets for
 * CompoundText and tests if the character is representable in that character
 * set. If so, it encodes the character using its code within that character
 * set. If not, it uses an UTF-8-in-CompoundText encapsulation. Since
 * clients of the first and second kind ignore such encapsulated text,
 * this encapsulation is kept to a minimum and terminated as early as possible.
 *
 * In a distant future, when clients of the first and second kind will have
 * disappeared, we will be able to stuff UTF-8 data directly in CompoundText
 * without first going through the list of predefined character sets.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XlcPubI.h"
#include "XlcGeneric.h"

static XlcConv
create_conv(
    XLCd lcd,
    XlcConvMethods methods)
{
    XlcConv conv;

    conv = Xmalloc(sizeof(XlcConvRec));
    if (conv == (XlcConv) NULL)
	return (XlcConv) NULL;

    conv->methods = methods;
    conv->state = NULL;

    return conv;
}

static void
close_converter(
    XlcConv conv)
{
    Xfree(conv);
}

/* Replacement character for invalid multibyte sequence or wide character. */
#define BAD_WCHAR ((ucs4_t) 0xfffd)
#define BAD_CHAR '?'

/***************************************************************************/
/* Part I: Conversion routines CompoundText/CharSet <--> Unicode/UTF-8.
 *
 * Note that this code works in any locale. We store Unicode values in
 * `ucs4_t' variables, but don't pass them to the user.
 *
 * This code has to support all character sets that are used for CompoundText,
 * nothing more, nothing less. See the table in lcCT.c.
 * Since the conversion _to_ CompoundText is likely to need the tables for all
 * character sets at once, we don't use dynamic loading (of tables or shared
 * libraries through iconv()). Use a fixed set of tables instead.
 *
 * We use statically computed tables, not dynamically allocated arrays,
 * because it's more memory efficient: Different processes using the same
 * libX11 shared library share the "text" and read-only "data" sections.
 */

typedef unsigned int ucs4_t;
#define conv_t XlcConv

typedef struct _Utf8ConvRec {
    const char *name;
    XrmQuark xrm_name;
    int (* cstowc) (XlcConv, ucs4_t *, unsigned char const *, int);
    int (* wctocs) (XlcConv, unsigned char *, ucs4_t, int);
} Utf8ConvRec, *Utf8Conv;

/*
 * int xxx_cstowc (XlcConv conv, ucs4_t *pwc, unsigned char const *s, int n)
 * converts the byte sequence starting at s to a wide character. Up to n bytes
 * are available at s. n is >= 1.
 * Result is number of bytes consumed (if a wide character was read),
 * or 0 if invalid, or -1 if n too small.
 *
 * int xxx_wctocs (XlcConv conv, unsigned char *r, ucs4_t wc, int n)
 * converts the wide character wc to the character set xxx, and stores the
 * result beginning at r. Up to n bytes may be written at r. n is >= 1.
 * Result is number of bytes written, or 0 if invalid, or -1 if n too small.
 */

/* Return code if invalid. (xxx_mbtowc, xxx_wctomb) */
#define RET_ILSEQ      0
/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW(n)  (-1-(n))
/* Return code if output buffer is too small. (xxx_wctomb, xxx_reset) */
#define RET_TOOSMALL   -1

/*
 * The tables below are bijective. It would be possible to extend the
 * xxx_wctocs tables to do some transliteration (e.g. U+201C,U+201D -> 0x22)
 * but *only* with characters not contained in any other table, and *only*
 * when the current locale is not an UTF-8 locale.
 */

#include "lcUniConv/utf8.h"
#include "lcUniConv/ucs2be.h"
#ifdef notused
#include "lcUniConv/ascii.h"
#endif
#include "lcUniConv/iso8859_1.h"
#include "lcUniConv/iso8859_2.h"
#include "lcUniConv/iso8859_3.h"
#include "lcUniConv/iso8859_4.h"
#include "lcUniConv/iso8859_5.h"
#include "lcUniConv/iso8859_6.h"
#include "lcUniConv/iso8859_7.h"
#include "lcUniConv/iso8859_8.h"
#include "lcUniConv/iso8859_9.h"
#include "lcUniConv/iso8859_10.h"
#include "lcUniConv/iso8859_11.h"
#include "lcUniConv/iso8859_13.h"
#include "lcUniConv/iso8859_14.h"
#include "lcUniConv/iso8859_15.h"
#include "lcUniConv/iso8859_16.h"
#include "lcUniConv/iso8859_9e.h"
#include "lcUniConv/jisx0201.h"
#include "lcUniConv/tis620.h"
#include "lcUniConv/koi8_r.h"
#include "lcUniConv/koi8_u.h"
#include "lcUniConv/koi8_c.h"
#include "lcUniConv/armscii_8.h"
#include "lcUniConv/cp1133.h"
#include "lcUniConv/mulelao.h"
#include "lcUniConv/viscii.h"
#include "lcUniConv/tcvn.h"
#include "lcUniConv/georgian_academy.h"
#include "lcUniConv/georgian_ps.h"
#include "lcUniConv/cp1251.h"
#include "lcUniConv/cp1255.h"
#include "lcUniConv/cp1256.h"
#include "lcUniConv/tatar_cyr.h"

typedef struct {
    unsigned short indx; /* index into big table */
    unsigned short used; /* bitmask of used entries */
} Summary16;

#include "lcUniConv/gb2312.h"
#include "lcUniConv/jisx0208.h"
#include "lcUniConv/jisx0212.h"
#include "lcUniConv/ksc5601.h"
#include "lcUniConv/big5.h"
#include "lcUniConv/big5_emacs.h"
#include "lcUniConv/big5hkscs.h"
#include "lcUniConv/gbk.h"

static Utf8ConvRec all_charsets[] = {
    /* The ISO10646-1/UTF-8 entry occurs twice, once at the beginning
       (for lookup speed), once at the end (as a fallback).  */
    { "ISO10646-1", NULLQUARK,
	utf8_mbtowc, utf8_wctomb
    },

    { "ISO8859-1", NULLQUARK,
	iso8859_1_mbtowc, iso8859_1_wctomb
    },
    { "ISO8859-2", NULLQUARK,
	iso8859_2_mbtowc, iso8859_2_wctomb
    },
    { "ISO8859-3", NULLQUARK,
	iso8859_3_mbtowc, iso8859_3_wctomb
    },
    { "ISO8859-4", NULLQUARK,
	iso8859_4_mbtowc, iso8859_4_wctomb
    },
    { "ISO8859-5", NULLQUARK,
	iso8859_5_mbtowc, iso8859_5_wctomb
    },
    { "ISO8859-6", NULLQUARK,
	iso8859_6_mbtowc, iso8859_6_wctomb
    },
    { "ISO8859-7", NULLQUARK,
	iso8859_7_mbtowc, iso8859_7_wctomb
    },
    { "ISO8859-8", NULLQUARK,
	iso8859_8_mbtowc, iso8859_8_wctomb
    },
    { "ISO8859-9", NULLQUARK,
	iso8859_9_mbtowc, iso8859_9_wctomb
    },
    { "ISO8859-10", NULLQUARK,
	iso8859_10_mbtowc, iso8859_10_wctomb
    },
    { "ISO8859-11", NULLQUARK,
	iso8859_11_mbtowc, iso8859_11_wctomb
    },
    { "ISO8859-13", NULLQUARK,
	iso8859_13_mbtowc, iso8859_13_wctomb
    },
    { "ISO8859-14", NULLQUARK,
	iso8859_14_mbtowc, iso8859_14_wctomb
    },
    { "ISO8859-15", NULLQUARK,
	iso8859_15_mbtowc, iso8859_15_wctomb
    },
    { "ISO8859-16", NULLQUARK,
	iso8859_16_mbtowc, iso8859_16_wctomb
    },
    { "JISX0201.1976-0", NULLQUARK,
	jisx0201_mbtowc, jisx0201_wctomb
    },
    { "TIS620-0", NULLQUARK,
	tis620_mbtowc, tis620_wctomb
    },
    { "GB2312.1980-0", NULLQUARK,
	gb2312_mbtowc, gb2312_wctomb
    },
    { "JISX0208.1983-0", NULLQUARK,
	jisx0208_mbtowc, jisx0208_wctomb
    },
    { "JISX0208.1990-0", NULLQUARK,
	jisx0208_mbtowc, jisx0208_wctomb
    },
    { "JISX0212.1990-0", NULLQUARK,
	jisx0212_mbtowc, jisx0212_wctomb
    },
    { "KSC5601.1987-0", NULLQUARK,
	ksc5601_mbtowc, ksc5601_wctomb
    },
    { "KOI8-R", NULLQUARK,
	koi8_r_mbtowc, koi8_r_wctomb
    },
    { "KOI8-U", NULLQUARK,
	koi8_u_mbtowc, koi8_u_wctomb
    },
    { "KOI8-C", NULLQUARK,
	koi8_c_mbtowc, koi8_c_wctomb
    },
    { "TATAR-CYR", NULLQUARK,
	tatar_cyr_mbtowc, tatar_cyr_wctomb
    },
    { "ARMSCII-8", NULLQUARK,
	armscii_8_mbtowc, armscii_8_wctomb
    },
    { "IBM-CP1133", NULLQUARK,
	cp1133_mbtowc, cp1133_wctomb
    },
    { "MULELAO-1", NULLQUARK,
	mulelao_mbtowc, mulelao_wctomb
    },
    { "VISCII1.1-1", NULLQUARK,
	viscii_mbtowc, viscii_wctomb
    },
    { "TCVN-5712", NULLQUARK,
	tcvn_mbtowc, tcvn_wctomb
    },
    { "GEORGIAN-ACADEMY", NULLQUARK,
	georgian_academy_mbtowc, georgian_academy_wctomb
    },
    { "GEORGIAN-PS", NULLQUARK,
	georgian_ps_mbtowc, georgian_ps_wctomb
    },
    { "ISO8859-9E", NULLQUARK,
	iso8859_9e_mbtowc, iso8859_9e_wctomb
    },
    { "MICROSOFT-CP1251", NULLQUARK,
	cp1251_mbtowc, cp1251_wctomb
    },
    { "MICROSOFT-CP1255", NULLQUARK,
	cp1255_mbtowc, cp1255_wctomb
    },
    { "MICROSOFT-CP1256", NULLQUARK,
	cp1256_mbtowc, cp1256_wctomb
    },
    { "BIG5-0", NULLQUARK,
	big5_mbtowc, big5_wctomb
    },
    { "BIG5-E0", NULLQUARK,
	big5_0_mbtowc, big5_0_wctomb
    },
    { "BIG5-E1", NULLQUARK,
	big5_1_mbtowc, big5_1_wctomb
    },
    { "GBK-0", NULLQUARK,
	gbk_mbtowc, gbk_wctomb
    },
    { "BIG5HKSCS-0", NULLQUARK,
	big5hkscs_mbtowc, big5hkscs_wctomb
    },

    /* The ISO10646-1/UTF-8 entry occurs twice, once at the beginning
       (for lookup speed), once at the end (as a fallback).  */
    { "ISO10646-1", NULLQUARK,
	utf8_mbtowc, utf8_wctomb
    },

    /* Encoding ISO10646-1 for fonts means UCS2-like encoding
       so for conversion to FontCharSet we need this record */
    { "ISO10646-1", NULLQUARK,
	ucs2be_mbtowc, ucs2be_wctomb
    }
};

#define charsets_table_size (sizeof(all_charsets)/sizeof(all_charsets[0]))
#define all_charsets_count  (charsets_table_size - 1)
#define ucs2_conv_index     (charsets_table_size - 1)

static void
init_all_charsets (void)
{
    Utf8Conv convptr;
    int i;

    for (convptr = all_charsets, i = charsets_table_size; i > 0; convptr++, i--)
	convptr->xrm_name = XrmStringToQuark(convptr->name);
}

#define lazy_init_all_charsets()					\
    do {								\
	if (all_charsets[0].xrm_name == NULLQUARK)			\
	    init_all_charsets();					\
    } while (0)

/* from XlcNCharSet to XlcNUtf8String */

static int
cstoutf8(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    XlcCharSet charset;
    const char *name;
    Utf8Conv convptr;
    int i;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args < 1)
	return -1;

    charset = (XlcCharSet) args[0];
    name = charset->encoding_name;
    /* not charset->name because the latter has a ":GL"/":GR" suffix */

    for (convptr = all_charsets, i = all_charsets_count-1; i > 0; convptr++, i--)
	if (!strcmp(convptr->name, name))
	    break;
    if (i == 0)
	return -1;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	ucs4_t wc;
	int consumed;
	int count;

	consumed = convptr->cstowc(conv, &wc, src, srcend-src);
	if (consumed == RET_ILSEQ)
	    return -1;
	if (consumed == RET_TOOFEW(0))
	    break;

	count = utf8_wctomb(NULL, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    count = utf8_wctomb(NULL, dst, BAD_WCHAR, dstend-dst);
	    if (count == RET_TOOSMALL)
		break;
	    unconv_num++;
	}
	src += consumed;
	dst += count;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_cstoutf8 = {
    close_converter,
    cstoutf8,
    NULL
};

static XlcConv
open_cstoutf8(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    lazy_init_all_charsets();
    return create_conv(from_lcd, &methods_cstoutf8);
}

/* from XlcNUtf8String to XlcNCharSet */

static XlcConv
create_tocs_conv(
    XLCd lcd,
    XlcConvMethods methods)
{
    XlcConv conv;
    CodeSet *codeset_list;
    int codeset_num;
    int charset_num;
    int i, j, k;
    Utf8Conv *preferred;

    lazy_init_all_charsets();

    codeset_list = XLC_GENERIC(lcd, codeset_list);
    codeset_num = XLC_GENERIC(lcd, codeset_num);

    charset_num = 0;
    for (i = 0; i < codeset_num; i++)
	charset_num += codeset_list[i]->num_charsets;
    if (charset_num > all_charsets_count-1)
	charset_num = all_charsets_count-1;

    conv = Xmalloc(sizeof(XlcConvRec)
			     + (charset_num + 1) * sizeof(Utf8Conv));
    if (conv == (XlcConv) NULL)
	return (XlcConv) NULL;
    preferred = (Utf8Conv *) ((char *) conv + sizeof(XlcConvRec));

    /* Loop through all codesets mentioned in the locale. */
    charset_num = 0;
    for (i = 0; i < codeset_num; i++) {
	XlcCharSet *charsets = codeset_list[i]->charset_list;
	int num_charsets = codeset_list[i]->num_charsets;
	for (j = 0; j < num_charsets; j++) {
	    const char *name = charsets[j]->encoding_name;
	    /* If it wasn't already encountered... */
	    for (k = charset_num-1; k >= 0; k--)
		if (!strcmp(preferred[k]->name, name))
		    break;
	    if (k < 0) {
		/* Look it up in all_charsets[]. */
		for (k = 0; k < all_charsets_count-1; k++)
		    if (!strcmp(all_charsets[k].name, name)) {
			/* Add it to the preferred set. */
			preferred[charset_num++] = &all_charsets[k];
			break;
		    }
	    }
	}
    }
    preferred[charset_num] = (Utf8Conv) NULL;

    conv->methods = methods;
    conv->state = (XPointer) preferred;

    return conv;
}

static void
close_tocs_converter(
    XlcConv conv)
{
    /* conv->state is allocated together with conv, free both at once.  */
    Xfree(conv);
}

/*
 * Converts a Unicode character to an appropriate character set. The NULL
 * terminated array of preferred character sets is passed as first argument.
 * If successful, *charsetp is set to the character set that was used, and
 * *sidep is set to the character set side (XlcGL or XlcGR).
 */
static int
charset_wctocs(
    Utf8Conv *preferred,
    Utf8Conv *charsetp,
    XlcSide *sidep,
    XlcConv conv,
    unsigned char *r,
    ucs4_t wc,
    int n)
{
    int count;
    Utf8Conv convptr;
    int i;

    for (; *preferred != (Utf8Conv) NULL; preferred++) {
	convptr = *preferred;
	count = convptr->wctocs(conv, r, wc, n);
	if (count == RET_TOOSMALL)
	    return RET_TOOSMALL;
	if (count != RET_ILSEQ) {
	    *charsetp = convptr;
	    *sidep = (*r < 0x80 ? XlcGL : XlcGR);
	    return count;
	}
    }
    for (convptr = all_charsets+1, i = all_charsets_count-1; i > 0; convptr++, i--) {
	count = convptr->wctocs(conv, r, wc, n);
	if (count == RET_TOOSMALL)
	    return RET_TOOSMALL;
	if (count != RET_ILSEQ) {
	    *charsetp = convptr;
	    *sidep = (*r < 0x80 ? XlcGL : XlcGR);
	    return count;
	}
    }
    return RET_ILSEQ;
}

static int
utf8tocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	ucs4_t wc;
	int consumed;
	int count;

	consumed = utf8_mbtowc(NULL, &wc, src, srcend-src);
	if (consumed == RET_TOOFEW(0))
	    break;
	if (consumed == RET_ILSEQ) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src += consumed;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src += consumed;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tocs = {
    close_tocs_converter,
    utf8tocs,
    NULL
};

static XlcConv
open_utf8tocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tocs_conv(from_lcd, &methods_utf8tocs);
}

/* from XlcNUtf8String to XlcNChar */

static int
utf8tocs1(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	ucs4_t wc;
	int consumed;
	int count;

	consumed = utf8_mbtowc(NULL, &wc, src, srcend-src);
	if (consumed == RET_TOOFEW(0))
	    break;
	if (consumed == RET_ILSEQ) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	last_charset = _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);

	if (last_charset == NULL) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	src += consumed;
	dst += count;
	break;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tocs1 = {
    close_tocs_converter,
    utf8tocs1,
    NULL
};

static XlcConv
open_utf8tocs1(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tocs_conv(from_lcd, &methods_utf8tocs1);
}

/* from XlcNUtf8String to XlcNString */

static int
utf8tostr(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	unsigned char c;
	ucs4_t wc;
	int consumed;

	consumed = utf8_mbtowc(NULL, &wc, src, srcend-src);
	if (consumed == RET_TOOFEW(0))
	    break;
	if (dst == dstend)
	    break;
	if (consumed == RET_ILSEQ) {
	    consumed = 1;
	    c = BAD_CHAR;
	    unconv_num++;
	} else {
	    if ((wc & ~(ucs4_t)0xff) != 0) {
		c = BAD_CHAR;
		unconv_num++;
	    } else
		c = (unsigned char) wc;
	}
	*dst++ = c;
	src += consumed;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tostr = {
    close_converter,
    utf8tostr,
    NULL
};

static XlcConv
open_utf8tostr(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_utf8tostr);
}

/* from XlcNString to XlcNUtf8String */

static int
strtoutf8(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;

    while (src < srcend) {
	int count = utf8_wctomb(NULL, dst, *src, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	dst += count;
	src++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_strtoutf8 = {
    close_converter,
    strtoutf8,
    NULL
};

static XlcConv
open_strtoutf8(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_strtoutf8);
}

/* Support for the input methods. */

XPointer
_Utf8GetConvByName(
    const char *name)
{
    XrmQuark xrm_name;
    Utf8Conv convptr;
    int i;

    if (name == NULL)
        return (XPointer) NULL;

    lazy_init_all_charsets();
    xrm_name = XrmStringToQuark(name);

    for (convptr = all_charsets, i = all_charsets_count-1; i > 0; convptr++, i--)
	if (convptr->xrm_name == xrm_name)
	    return (XPointer) convptr->wctocs;
    return (XPointer) NULL;
}

/* from XlcNUcsChar to XlcNChar, needed for input methods */

static XlcConv
create_ucstocs_conv(
    XLCd lcd,
    XlcConvMethods methods)
{

    if (XLC_PUBLIC_PART(lcd)->codeset
	&& _XlcCompareISOLatin1(XLC_PUBLIC_PART(lcd)->codeset, "UTF-8") == 0) {
	XlcConv conv;
	Utf8Conv *preferred;

	lazy_init_all_charsets();

	conv = Xmalloc(sizeof(XlcConvRec) + 2 * sizeof(Utf8Conv));
	if (conv == (XlcConv) NULL)
	    return (XlcConv) NULL;
	preferred = (Utf8Conv *) ((char *) conv + sizeof(XlcConvRec));

	preferred[0] = &all_charsets[0]; /* ISO10646 */
	preferred[1] = (Utf8Conv) NULL;

	conv->methods = methods;
	conv->state = (XPointer) preferred;

	return conv;
    } else {
	return create_tocs_conv(lcd, methods);
    }
}

static int
charset_wctocs_exactly(
    Utf8Conv *preferred,
    Utf8Conv *charsetp,
    XlcSide *sidep,
    XlcConv conv,
    unsigned char *r,
    ucs4_t wc,
    int n)
{
    int count;
    Utf8Conv convptr;

    for (; *preferred != (Utf8Conv) NULL; preferred++) {
	convptr = *preferred;
	count = convptr->wctocs(conv, r, wc, n);
	if (count == RET_TOOSMALL)
	    return RET_TOOSMALL;
	if (count != RET_ILSEQ) {
	    *charsetp = convptr;
	    *sidep = (*r < 0x80 ? XlcGL : XlcGR);
	    return count;
	}
    }
    return RET_ILSEQ;
}

static int
ucstocs1(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    ucs4_t const *src;
    unsigned char *dst = (unsigned char *) *to;
    int unconv_num = 0;
    Utf8Conv *preferred_charsets = (Utf8Conv *) conv->state;
    Utf8Conv chosen_charset = NULL;
    XlcSide chosen_side = XlcNONE;
    XlcCharSet charset = NULL;
    int count;

    if (from == NULL || *from == NULL)
	return 0;

    src = (ucs4_t const *) *from;

    count = charset_wctocs_exactly(preferred_charsets, &chosen_charset,
                                   &chosen_side, conv, dst, *src, *to_left);
    if (count < 1) {
        unconv_num++;
        count = 0;
    } else {
        charset = _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
    }
    if (charset == NULL)
	return -1;

    *from = (XPointer) ++src;
    (*from_left)--;
    *to = (XPointer) dst;
    *to_left -= count;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_ucstocs1 = {
    close_tocs_converter,
    ucstocs1,
    NULL
};

static XlcConv
open_ucstocs1(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_ucstocs_conv(from_lcd, &methods_ucstocs1);
}

/* from XlcNUcsChar to XlcNUtf8String, needed for input methods */

static int
ucstoutf8(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const ucs4_t *src;
    const ucs4_t *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const ucs4_t *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	int count = utf8_wctomb(NULL, dst, *src, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ)
	    unconv_num++;
	src++;
	dst += count;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_ucstoutf8 = {
    close_converter,
    ucstoutf8,
    NULL
};

static XlcConv
open_ucstoutf8(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_ucstoutf8);
}

/* Registers UTF-8 converters for a non-UTF-8 locale. */
void
_XlcAddUtf8Converters(
    XLCd lcd)
{
    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNUtf8String, open_cstoutf8);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNCharSet, open_utf8tocs);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNChar, open_utf8tocs1);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNUtf8String, open_strtoutf8);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNString, open_utf8tostr);
    _XlcSetConverter(lcd, XlcNUcsChar,    lcd, XlcNChar, open_ucstocs1);
    _XlcSetConverter(lcd, XlcNUcsChar,    lcd, XlcNUtf8String, open_ucstoutf8);
}

/***************************************************************************/
/* Part II: UTF-8 locale loader conversion files
 *
 * Here we can assume that "multi-byte" is UTF-8 and that `wchar_t' is Unicode.
 */

/* from XlcNMultiByte to XlcNWideChar */

static int
utf8towcs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	ucs4_t wc;
	int consumed = utf8_mbtowc(NULL, &wc, src, srcend-src);
	if (consumed == RET_TOOFEW(0))
	    break;
	if (consumed == RET_ILSEQ) {
	    src++;
	    *dst = BAD_WCHAR;
	    unconv_num++;
	} else {
	    src += consumed;
	    *dst = wc;
	}
	dst++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8towcs = {
    close_converter,
    utf8towcs,
    NULL
};

static XlcConv
open_utf8towcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_utf8towcs);
}

/* from XlcNWideChar to XlcNMultiByte */

static int
wcstoutf8(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	int count = utf8_wctomb(NULL, dst, *src, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    count = utf8_wctomb(NULL, dst, BAD_WCHAR, dstend-dst);
	    if (count == RET_TOOSMALL)
		break;
	    unconv_num++;
	}
	dst += count;
	src++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstoutf8 = {
    close_converter,
    wcstoutf8,
    NULL
};

static XlcConv
open_wcstoutf8(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_wcstoutf8);
}

/* from XlcNString to XlcNWideChar */

static int
our_strtowcs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;

    while (src < srcend && dst < dstend)
	*dst++ = (wchar_t) *src++;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_strtowcs = {
    close_converter,
    our_strtowcs,
    NULL
};

static XlcConv
open_strtowcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_strtowcs);
}

/* from XlcNWideChar to XlcNString */

static int
our_wcstostr(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	unsigned int wc = *src++;
	if (wc < 0x80)
	    *dst = wc;
	else {
	    *dst = BAD_CHAR;
	    unconv_num++;
	}
	dst++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstostr = {
    close_converter,
    our_wcstostr,
    NULL
};

static XlcConv
open_wcstostr(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_wcstostr);
}

/* from XlcNCharSet to XlcNWideChar */

static int
cstowcs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    XlcCharSet charset;
    const char *name;
    Utf8Conv convptr;
    int i;
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args < 1)
	return -1;

    charset = (XlcCharSet) args[0];
    name = charset->encoding_name;
    /* not charset->name because the latter has a ":GL"/":GR" suffix */

    for (convptr = all_charsets, i = all_charsets_count-1; i > 0; convptr++, i--)
	if (!strcmp(convptr->name, name))
	    break;
    if (i == 0)
	return -1;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	unsigned int wc;
	int consumed;

	consumed = convptr->cstowc(conv, &wc, src, srcend-src);
	if (consumed == RET_ILSEQ)
	    return -1;
	if (consumed == RET_TOOFEW(0))
	    break;

	*dst++ = wc;
	src += consumed;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_cstowcs = {
    close_converter,
    cstowcs,
    NULL
};

static XlcConv
open_cstowcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    lazy_init_all_charsets();
    return create_conv(from_lcd, &methods_cstowcs);
}

/* from XlcNWideChar to XlcNCharSet */

static int
wcstocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc = *src;
	int count;

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src++;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src++;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src++;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstocs = {
    close_tocs_converter,
    wcstocs,
    NULL
};

static XlcConv
open_wcstocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tocs_conv(from_lcd, &methods_wcstocs);
}

/* from XlcNWideChar to XlcNChar */

static int
wcstocs1(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc = *src;
	int count;

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src++;
	    unconv_num++;
	    continue;
	}

	last_charset = _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);

	if (last_charset == NULL) {
	    src++;
	    unconv_num++;
	    continue;
	}

	src++;
	dst += count;
	break;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstocs1 = {
    close_tocs_converter,
    wcstocs1,
    NULL
};

static XlcConv
open_wcstocs1(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tocs_conv(from_lcd, &methods_wcstocs1);
}

/* trivial, no conversion */

static int
identity(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;

    while (src < srcend && dst < dstend)
	*dst++ = *src++;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_identity = {
    close_converter,
    identity,
    NULL
};

static XlcConv
open_identity(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &methods_identity);
}

/* from MultiByte/WideChar to FontCharSet. */
/* They really use converters to CharSet
 * but with different create_conv procedure. */

static XlcConv
create_tofontcs_conv(
    XLCd lcd,
    XlcConvMethods methods)
{
    XlcConv conv;
    int i, num, k, count;
    char **value, buf[32];
    Utf8Conv *preferred;

    lazy_init_all_charsets();

    for (i = 0, num = 0;; i++) {
	snprintf(buf, sizeof(buf), "fs%d.charset.name", i);
	_XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
	if (count < 1) {
	    snprintf(buf, sizeof(buf), "fs%d.charset", i);
	    _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
	    if (count < 1)
		break;
	}
	num += count;
    }

    conv = Xmalloc(sizeof(XlcConvRec) + (num + 1) * sizeof(Utf8Conv));
    if (conv == (XlcConv) NULL)
	return (XlcConv) NULL;
    preferred = (Utf8Conv *) ((char *) conv + sizeof(XlcConvRec));

    /* Loop through all fontsets mentioned in the locale. */
    for (i = 0, num = 0;; i++) {
        snprintf(buf, sizeof(buf), "fs%d.charset.name", i);
        _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
        if (count < 1) {
            snprintf(buf, sizeof(buf), "fs%d.charset", i);
            _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
            if (count < 1)
                break;
        }
	while (count-- > 0) {
	    XlcCharSet charset = _XlcGetCharSet(*value++);
	    const char *name;

	    if (charset == (XlcCharSet) NULL)
		continue;

	    name = charset->encoding_name;
	    /* If it wasn't already encountered... */
	    for (k = num - 1; k >= 0; k--)
		if (!strcmp(preferred[k]->name, name))
		    break;
	    if (k < 0) {
                /* For fonts "ISO10646-1" means ucs2, not utf8.*/
                if (!strcmp("ISO10646-1", name)) {
                    preferred[num++] = &all_charsets[ucs2_conv_index];
                    continue;
                }
		/* Look it up in all_charsets[]. */
		for (k = 0; k < all_charsets_count-1; k++)
		    if (!strcmp(all_charsets[k].name, name)) {
			/* Add it to the preferred set. */
			preferred[num++] = &all_charsets[k];
			break;
		    }
	    }
        }
    }
    preferred[num] = (Utf8Conv) NULL;

    conv->methods = methods;
    conv->state = (XPointer) preferred;

    return conv;
}

static XlcConv
open_wcstofcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tofontcs_conv(from_lcd, &methods_wcstocs);
}

static XlcConv
open_utf8tofcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tofontcs_conv(from_lcd, &methods_utf8tocs);
}

/* ========================== iconv Stuff ================================ */

/* from XlcNCharSet to XlcNMultiByte */

static int
iconv_cstombs(XlcConv conv, XPointer *from, int *from_left,
	      XPointer *to, int *to_left, XPointer *args, int num_args)
{
    XlcCharSet charset;
    char const *name;
    Utf8Conv convptr;
    int i;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args < 1)
	return -1;

    charset = (XlcCharSet) args[0];
    name = charset->encoding_name;
    /* not charset->name because the latter has a ":GL"/":GR" suffix */

    for (convptr = all_charsets, i = all_charsets_count-1; i > 0; convptr++, i--)
	if (!strcmp(convptr->name, name))
	    break;
    if (i == 0)
	return -1;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	ucs4_t wc;
	int consumed;
	int count;

	consumed = convptr->cstowc(conv, &wc, src, srcend-src);
	if (consumed == RET_ILSEQ)
	    return -1;
	if (consumed == RET_TOOFEW(0))
	    break;

    /* Use stdc iconv to convert widechar -> multibyte */

	count = wctomb((char *)dst, wc);
	if (count == 0)
	    break;
	if (count == -1) {
	    count = wctomb((char *)dst, BAD_WCHAR);
	    if (count == 0)
		break;
	    unconv_num++;
	}
	src += consumed;
	dst += count;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;

}

static XlcConvMethodsRec iconv_cstombs_methods = {
    close_converter,
    iconv_cstombs,
    NULL
};

static XlcConv
open_iconv_cstombs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    lazy_init_all_charsets();
    return create_conv(from_lcd, &iconv_cstombs_methods);
}

static int
iconv_mbstocs(XlcConv conv, XPointer *from, int *from_left,
	      XPointer *to, int *to_left, XPointer *args, int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc;
	int consumed;
	int count;

    /* Uses stdc iconv to convert multibyte -> widechar */

	consumed = mbtowc(&wc, (const char *)src, (size_t) (srcend - src));
	if (consumed == 0)
	    break;
	if (consumed == -1) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);

	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src += consumed;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src += consumed;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec iconv_mbstocs_methods = {
    close_tocs_converter,
    iconv_mbstocs,
    NULL
};

static XlcConv
open_iconv_mbstocs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_tocs_conv(from_lcd, &iconv_mbstocs_methods);
}

/* from XlcNMultiByte to XlcNChar */

static int
iconv_mbtocs(XlcConv conv, XPointer *from, int *from_left,
	     XPointer *to, int *to_left, XPointer *args, int num_args)
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc;
	int consumed;
	int count;

    /* Uses stdc iconv to convert multibyte -> widechar */

	consumed = mbtowc(&wc, (const char *)src, (size_t) (srcend - src));
	if (consumed == 0)
	    break;
	if (consumed == -1) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == RET_TOOSMALL)
	    break;
	if (count == RET_ILSEQ) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
		_XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src += consumed;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
		  && (last_charset->side == XlcGLGR
		      || last_charset->side == chosen_side)))
		break;
	}
	src += consumed;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec iconv_mbtocs_methods = {
    close_tocs_converter,
    iconv_mbtocs,
    NULL
};

static XlcConv
open_iconv_mbtocs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_tocs_conv(from_lcd, &iconv_mbtocs_methods );
}

/* from XlcNMultiByte to XlcNString */

static int
iconv_mbstostr(XlcConv conv, XPointer *from, int *from_left,
	       XPointer *to, int *to_left, XPointer *args, int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	unsigned char c;
	wchar_t wc;
	int consumed;

    /* Uses stdc iconv to convert multibyte -> widechar */

	consumed = mbtowc(&wc, (const char *)src, (size_t) (srcend - src));
	if (consumed == 0)
	    break;
	if (dst == dstend)
	    break;
	if (consumed == -1) {
	    consumed = 1;
	    c = BAD_CHAR;
	    unconv_num++;
	} else {
	    if ((wc & ~(wchar_t)0xff) != 0) {
		c = BAD_CHAR;
		unconv_num++;
	    } else
		c = (unsigned char) wc;
	}
	*dst++ = c;
	src += consumed;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec iconv_mbstostr_methods = {
    close_converter,
    iconv_mbstostr,
    NULL
};

static XlcConv
open_iconv_mbstostr(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_conv(from_lcd, &iconv_mbstostr_methods);
}

/* from XlcNString to XlcNMultiByte */
static int
iconv_strtombs(XlcConv conv, XPointer *from, int *from_left,
	       XPointer *to, int *to_left, XPointer *args, int num_args)
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;

    while (src < srcend) {
	int count = wctomb((char *)dst, *src);
	if (count < 0)
	    break;
	dst += count;
	src++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec iconv_strtombs_methods= {
    close_converter,
    iconv_strtombs,
    NULL
};

static XlcConv
open_iconv_strtombs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_conv(from_lcd, &iconv_strtombs_methods);
}

/***************************************************************************/
/* Part II: An iconv locale loader.
 *
 *Here we can assume that "multi-byte" is iconv and that `wchar_t' is Unicode.
 */

/* from XlcNMultiByte to XlcNWideChar */
static int
iconv_mbstowcs(XlcConv conv, XPointer *from, int *from_left,
	       XPointer *to, int *to_left, XPointer *args,  int num_args)
{
    char *src = *((char **) from);
    wchar_t *dst = *((wchar_t **) to);
    int src_left = *from_left;
    int dst_left = *to_left;
    int length, unconv_num = 0;

    while (src_left > 0 && dst_left > 0) {
	length = mbtowc(dst, src, (size_t) src_left);

	if (length > 0) {
	    src += length;
	    src_left -= length;
	    if (dst)
	        dst++;
	    dst_left--;
	} else if (length < 0) {
	    src++;
	    src_left--;
	    unconv_num++;
        } else {
            /* null ? */
            src++;
            src_left--;
            if (dst) 
                *dst++ = L'\0';
            dst_left--;
        }
    }

    *from = (XPointer) src;
    if (dst)
	*to = (XPointer) dst;
    *from_left = src_left;
    *to_left = dst_left;

    return unconv_num;
}

static XlcConvMethodsRec iconv_mbstowcs_methods = {
    close_converter,
    iconv_mbstowcs,
    NULL
} ;

static XlcConv
open_iconv_mbstowcs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_conv(from_lcd, &iconv_mbstowcs_methods);
}

static int
iconv_wcstombs(XlcConv conv, XPointer *from, int *from_left,
	       XPointer *to, int *to_left, XPointer *args, int num_args)
{
    wchar_t *src = *((wchar_t **) from);
    char *dst = *((char **) to);
    int src_left = *from_left;
    int dst_left = *to_left;
    int length, unconv_num = 0;

    while (src_left > 0 && dst_left >= MB_CUR_MAX) { 
	length = wctomb(dst, *src);		/* XXX */

        if (length > 0) {
	    src++;
	    src_left--;
	    if (dst) 
		dst += length;
	    dst_left -= length;
	} else if (length < 0) {
	    src++;
	    src_left--;
	    unconv_num++;
	} 
    }

    *from = (XPointer) src;
    if (dst)
      *to = (XPointer) dst;
    *from_left = src_left;
    *to_left = dst_left;

    return unconv_num;
}

static XlcConvMethodsRec iconv_wcstombs_methods = {
    close_converter,
    iconv_wcstombs,
    NULL
} ;

static XlcConv
open_iconv_wcstombs(XLCd from_lcd, const char *from_type, XLCd to_lcd, const char *to_type)
{
    return create_conv(from_lcd, &iconv_wcstombs_methods);
}

static XlcConv
open_iconv_mbstofcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_tofontcs_conv(from_lcd, &iconv_mbstocs_methods);
}

/* Registers UTF-8 converters for a UTF-8 locale. */

void
_XlcAddUtf8LocaleConverters(
    XLCd lcd)
{
    /* Register elementary converters. */

    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar, open_utf8towcs);

    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte, open_wcstoutf8);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNString, open_wcstostr);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNWideChar, open_strtowcs);

    /* Register converters for XlcNCharSet. This implicitly provides
     * converters from and to XlcNCompoundText. */

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNMultiByte, open_cstoutf8);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet, open_utf8tocs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar, open_utf8tocs1);

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNWideChar, open_cstowcs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCharSet, open_wcstocs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNChar, open_wcstocs1);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNMultiByte, open_strtoutf8);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString, open_utf8tostr);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNMultiByte, open_identity);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNUtf8String, open_identity);

    /* Register converters for XlcNFontCharSet */
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNFontCharSet, open_utf8tofcs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNFontCharSet, open_wcstofcs);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNFontCharSet, open_utf8tofcs);
}

void
_XlcAddGB18030LocaleConverters(
    XLCd lcd)
{

    /* Register elementary converters. */
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar, open_iconv_mbstowcs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte, open_iconv_wcstombs);

    /* Register converters for XlcNCharSet. This implicitly provides
     * converters from and to XlcNCompoundText. */

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNMultiByte, open_iconv_cstombs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet, open_iconv_mbstocs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar, open_iconv_mbtocs);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNMultiByte, open_iconv_strtombs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString, open_iconv_mbstostr);

    /* Register converters for XlcNFontCharSet */
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNFontCharSet, open_iconv_mbstofcs);

    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNString, open_wcstostr);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNWideChar, open_strtowcs);
    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNWideChar, open_cstowcs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCharSet, open_wcstocs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNChar, open_wcstocs1);

    /* Register converters for XlcNFontCharSet */
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNFontCharSet, open_wcstofcs);
}
