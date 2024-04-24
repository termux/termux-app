/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 *
 * Modifier: Takanori Tateno   FUJITSU LIMITED
 *
 */
/*
 * Most of this API is documented in i18n/Framework.PS
 */

#ifndef _XLCPUBLIC_H_
#define _XLCPUBLIC_H_

#include "Xlcint.h"


/*
 * Character sets.
 */

/* Every character set has a "side". It denotes the range of byte values for
   which the character set is responsible. This means that the character
   set's encoded characters will only assumes bytes within the range, and
   that the character set can be used simultaneously with another character
   set responsible for a disjoint range. */
typedef enum {
    XlcUnknown,
    XlcC0,		/* responsible for values 0x00..0x1F */
    XlcGL,		/* responsible for values 0x00..0x7F or 0x20..0x7F */
    XlcC1,		/* responsible for values 0x80..0x9F */
    XlcGR,		/* responsible for values 0x80..0xFF or 0xA0..0xFF */
    XlcGLGR,		/* responsible for values 0x00..0xFF */
    XlcOther,		/* unused */
    XlcNONE
} XlcSide;

/* Data read from XLC_LOCALE files.
   XXX Apparently superseded by _XUDCGlyphRegion. */
typedef struct _UDCArea {
    unsigned long	start;
    unsigned long	end;
} UDCAreaRec, *UDCArea;

/* Where the character set comes from. */
typedef enum {
    CSsrcUndef,		/* unused */
    CSsrcStd,		/* defined in libX11 */
    CSsrcXLC		/* defined in an XLC_LOCALE file */
} CSSrc;

/* These are the supported properties of XlcCharSet. */
#define XlcNCharSize 		"charSize"
#define XlcNControlSequence 	"controlSequence"
#define XlcNEncodingName 	"encodingName"
#define XlcNName 		"name"
#define XlcNSetSize 		"setSize"
#define XlcNSide 		"side"

/* This is the structure of an XlcCharSet.
   Once allocated, they are never freed. */
typedef struct _XlcCharSetRec {
    /* Character set name, including side suffix */
    const char 		*name;
    XrmQuark 		xrm_name;

    /* XLFD encoding name, no side suffix */
    const char 		*encoding_name;
    XrmQuark 		xrm_encoding_name;

    /* Range for which the charset is responsible: XlcGL, XlcGR or XlcGLGR */
    XlcSide 		side;

    /* Number of bytes per character. 0 means a varying number (e.g. UTF-8) */
    int 		char_size;
    /* Classification of the character set according to ISO-2022 */
    int 		set_size;	/* e.g. 94 or 96 */
    const char 		*ct_sequence;	/* control sequence of CT */
					/* (normally at most 4 bytes) */

    /* for UDC */
    Bool        	string_encoding;
    UDCArea 		udc_area;
    int     		udc_area_num;

    /* Description source */
    CSSrc		source;
} XlcCharSetRec, *XlcCharSet;

_XFUNCPROTOBEGIN

/* Returns the charset with the given name (including side suffix).
   Returns NULL if not found. */
extern XlcCharSet _XlcGetCharSet(
    const char*		name
);

/* Returns the charset with the given encoding (no side suffix) and
   responsible for at least the given side (XlcGL or XlcGR).
   Returns NULL if not found. */
extern XlcCharSet _XlcGetCharSetWithSide(
    const char*		encoding_name,
    XlcSide		side
);

/* Registers an XlcCharSet in the list of character sets.
   Returns True if successful. */
extern Bool _XlcAddCharSet(
    XlcCharSet		charset
);

/* Retrieves a number of attributes of an XlcCharSet.
   Return NULL if successful, otherwise the name of the first argument
   specifying a nonexistent attribute. */
extern char *_XlcGetCSValues(
    XlcCharSet		charset,
    ...
);

_XFUNCPROTOEND


#define XlcNCodeset 		"codeset"
#define XlcNDefaultString 	"defaultString"
#define XlcNLanguage 		"language"
#define XlcNMbCurMax 		"mbCurMax"
#define XlcNStateDependentEncoding "stateDependentEncoding"
#define XlcNTerritory 		"territory"

typedef struct _FontScope {
        unsigned long   start;
        unsigned long   end;
        unsigned long   shift;
        unsigned long   shift_direction;
} FontScopeRec, *FontScope;

/*
 * conversion methods
 */

typedef struct _XlcConvRec *XlcConv;

typedef XlcConv (*XlcOpenConverterProc)(
    XLCd		from_lcd,
    const char*		from_type,
    XLCd		to_lcd,
    const char*		to_type
);

typedef void (*XlcCloseConverterProc)(
    XlcConv		/* conv */
);

typedef int (*XlcConvertProc)(
    XlcConv		/* conv */,
    XPointer*		/* from */,
    int*		/* from_left */,
    XPointer*		/* to */,
    int*		/* to_left */,
    XPointer*		/* args */,
    int			/* num_args */
);

typedef void (*XlcResetConverterProc)(
    XlcConv		/* conv */
);

typedef struct _XlcConvMethodsRec{
    XlcCloseConverterProc 	close;
    XlcConvertProc 		convert;
    XlcResetConverterProc 	reset;
} XlcConvMethodsRec, *XlcConvMethods;

/*
 * conversion data
 */

#define XlcNMultiByte 		"multiByte"
#define XlcNWideChar 		"wideChar"
#define XlcNCompoundText 	"compoundText"
#define XlcNString 		"string"
#define XlcNUtf8String 		"utf8String"
#define XlcNCharSet 		"charSet"
#define XlcNCTCharSet 		"CTcharSet"
#define XlcNFontCharSet		"FontCharSet"
#define XlcNChar 		"char"
#define XlcNUcsChar 		"UCSchar"

typedef struct _XlcConvRec {
    XlcConvMethods 		methods;
    XPointer 			state;
} XlcConvRec;


_XFUNCPROTOBEGIN

extern Bool _XInitOM(
    XLCd		/* lcd */
);

extern Bool _XInitIM(
    XLCd		/* lcd */
);

extern XIM _XimOpenIM(
    XLCd		/* lcd */,
    Display *		/* dpy */,
    XrmDatabase		/* rdb */,
    char *		/* res_name */,
    char *		/* res_class */
);

extern char *_XGetLCValues(
    XLCd		/* lcd */,
    ...
);

extern XlcConv _XlcOpenConverter(
    XLCd		from_lcd,
    const char*		from_type,
    XLCd		to_lcd,
    const char*		to_type
);

extern void _XlcCloseConverter(
    XlcConv		conv
);

extern int _XlcConvert(
    XlcConv		conv,
    XPointer*		from,
    int*		from_left,
    XPointer*		to,
    int*		to_left,
    XPointer*		args,
    int			num_args
);

extern void _XlcResetConverter(
    XlcConv		conv
);

extern Bool _XlcSetConverter(
    XLCd			from_lcd,
    const char*			from_type,
    XLCd			to_lcd,
    const char*			to_type,
    XlcOpenConverterProc	open_converter
);

extern void _XlcGetResource(
    XLCd		lcd,
    const char*		category,
    const char*		_class,
    char***		value,
    int*		count
);

extern char *_XlcFileName(
    XLCd		lcd,
    const char*		category
);

extern int _Xwcslen(
    wchar_t*		/* wstr */
);

extern wchar_t *_Xwcscpy(
    wchar_t*		/* wstr1 */,
    wchar_t*		/* wstr2 */
);

extern wchar_t *_Xwcsncpy(wchar_t *wstr1, wchar_t *wstr2, int len);
extern int _Xwcscmp(wchar_t *wstr1, wchar_t *wstr2);
extern int _Xwcsncmp(wchar_t *wstr1, wchar_t *wstr2, int len);

/* Compares two ISO 8859-1 strings, ignoring case of ASCII letters.
   Like strcasecmp in an ASCII locale. */
extern int _XlcCompareISOLatin1(
    const char*		str1,
    const char*		str2
);

/* Compares two ISO 8859-1 strings, at most len bytes of each, ignoring
   case of ASCII letters. Like strncasecmp in an ASCII locale. */
extern int _XlcNCompareISOLatin1(
    const char*		str1,
    const char*		str2,
    int			len
);

extern XOM
_XDefaultOpenOM(
    XLCd lcd, Display *dpy, XrmDatabase rdb,
    _Xconst char *res_name, _Xconst char *res_class);

_XFUNCPROTOEND

#endif  /* _XLCPUBLIC_H_ */
