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

#ifndef _XLCGENERIC_H_
#define _XLCGENERIC_H_

#include "XlcPubI.h"

typedef struct _ByteInfo {
     unsigned char start,end;
} ByteInfoRec, *ByteInfo;

typedef struct _ByteInfoList {
    int 	M;                          /* 1 <= M <= length */
    int 	byteinfo_num;
    ByteInfo 	byteinfo;
} ByteInfoListRec, *ByteInfoList;

/* conversion_type values */
#define LOCALCONV      1
#define FILECONV       2
#define FUNCTIONCONV   4

typedef struct _Conversion {
    unsigned long 	conversion_type;
    int       		conv_num;
    FontScope 		convlist;
    char      		*cnv_file;
    XlcConv    		cnvfunc;
} ConversionRec, *Conversion;

typedef struct _ExtdSegment {
    char    	*name;
    XlcSide 	side;
    FontScope 	area;
    int 	area_num;
    XlcCharSet  charset;
} ExtdSegmentRec, *ExtdSegment;

typedef struct _SegConvRec {
    int         	length;
    char        	*source_encoding;
    XlcCharSet		source;
    char        	*destination_encoding;
    XlcCharSet		dest;
    FontScopeRec   	range;
    int         	conv_num;
    FontScope   	conv;
} SegConvRec, *SegConv;

typedef struct _ParseInfoRec *ParseInfo;

typedef struct _CodeSetRec {
    XlcCharSet 		*charset_list;
    int 		num_charsets;
    int 		cs_num;
    XlcSide 		side;
    int 		length;
    ByteInfoList 	byteM;
    Conversion 		mbconv;
    Conversion 		ctconv;
    ExtdSegment 	ctextseg;
    ParseInfo 		parse_info;
    unsigned long 	wc_encoding;
    Bool		string_encoding;
} CodeSetRec, *CodeSet;

typedef enum {
    E_GL,			/* GL encoding */
    E_GR,			/* GR encoding */
    E_SS,			/* single shift */
    E_LSL,			/* locking shift left */
    E_LSR,			/* locking shift right */
    E_LAST
} EncodingType;

typedef struct _ParseInfoRec {
    EncodingType 	type;
    char 		*encoding;
    CodeSet 		codeset;
} ParseInfoRec;

/*
 * XLCd private data
 */

#define XLC_GENERIC(lcd, x)	(((XLCdGeneric) lcd->core)->gen.x)
#define XLC_GENERIC_PART(lcd)	(&(((XLCdGeneric) lcd->core)->gen))

typedef struct _XLCdGenericPart {
    int 		codeset_num;
    CodeSet 		*codeset_list;
    unsigned char 	*mb_parse_table;
    int 		mb_parse_list_num;
    ParseInfo 		*mb_parse_list;
    unsigned long 	wc_encode_mask;
    unsigned long 	wc_shift_bits;
    CodeSet 		initial_state_GL;
    CodeSet 		initial_state_GR;
    int  		segment_conv_num;  /* UDC */
    SegConv 		segment_conv;      /* UDC */
    Bool 		use_stdc_env;
    Bool 		force_convert_to_mb;
} XLCdGenericPart;

typedef struct _XLCdGenericRec {
    XLCdCoreRec 	core;
    XLCdPublicPart 	pub;
    XLCdGenericPart 	gen;
} XLCdGenericRec, *XLCdGeneric;

extern XLCdMethods _XlcGenericMethods;

extern FontScope
_XlcParse_scopemaps(
    const char *str,
    int *size);
extern void
_XlcDbg_printValue(
    const char *str,
    char **value,
    int num);

extern XIM
_XDefaultOpenIM(
    XLCd                lcd,
    Display             *dpy,
    XrmDatabase         rdb,
    char                *res_name,
    char                *res_class);

#endif  /* _XLCGENERIC_H_ */
