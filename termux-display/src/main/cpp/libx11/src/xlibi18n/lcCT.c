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
 *  2000
 *  Modifier: Ivan Pascal     The XFree86 Project
 *  Modifier: Bruno Haible    The XFree86 Project
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XlcPubI.h"
#include <X11/Xos.h>
#include <stdio.h>


/* ====================== Built-in Character Sets ====================== */

/*
 * Static representation of a character set that can be used in Compound Text.
 */
typedef struct _CTDataRec {
    const char name[19];
    const char ct_sequence[5];	/* Compound Text encoding, ESC sequence */
} CTDataRec, *CTData;

static const CTDataRec default_ct_data[] =
{
    /*                                                                    */
    /* X11 registry name       MIME name         ISO-IR      ESC sequence */
    /*                                                                    */

    /* Registered character sets with one byte per character */
    { "ISO8859-1:GL",       /* US-ASCII              6   */  "\033(B" },
    { "ISO8859-1:GR",       /* ISO-8859-1          100   */  "\033-A" },
    { "ISO8859-2:GR",       /* ISO-8859-2          101   */  "\033-B" },
    { "ISO8859-3:GR",       /* ISO-8859-3          109   */  "\033-C" },
    { "ISO8859-4:GR",       /* ISO-8859-4          110   */  "\033-D" },
    { "ISO8859-5:GR",       /* ISO-8859-5          144   */  "\033-L" },
    { "ISO8859-6:GR",       /* ISO-8859-6          127   */  "\033-G" },
    { "ISO8859-7:GR",       /* ISO-8859-7          126   */  "\033-F" },
    { "ISO8859-8:GR",       /* ISO-8859-8          138   */  "\033-H" },
    { "ISO8859-9:GR",       /* ISO-8859-9          148   */  "\033-M" },
    { "ISO8859-10:GR",      /* ISO-8859-10         157   */  "\033-V" },
    { "ISO8859-11:GR",      /* ISO-8859-11         166   */  "\033-T" },
    { "ISO8859-13:GR",      /* ISO-8859-13         179   */  "\033-Y" },
    { "ISO8859-14:GR",      /* ISO-8859-14         199   */  "\033-_" },
    { "ISO8859-15:GR",      /* ISO-8859-15         203   */  "\033-b" },
    { "ISO8859-16:GR",      /* ISO-8859-16         226   */  "\033-f" },
    { "JISX0201.1976-0:GL", /* ISO-646-JP           14   */  "\033(J" },
    { "JISX0201.1976-0:GR",                                  "\033)I" },
#if 0
    { "TIS620-0:GR",        /* TIS-620             166   */  "\033-T" },
#endif

    /* Registered character sets with two byte per character */
    { "GB2312.1980-0:GL",   /* GB_2312-80           58   */ "\033$(A" },
    { "GB2312.1980-0:GR",   /* GB_2312-80           58   */ "\033$)A" },
    { "JISX0208.1983-0:GL", /* JIS_X0208-1983       87   */ "\033$(B" },
    { "JISX0208.1983-0:GR", /* JIS_X0208-1983       87   */ "\033$)B" },
    { "JISX0208.1990-0:GL", /* JIS_X0208-1990      168   */ "\033$(B" },
    { "JISX0208.1990-0:GR", /* JIS_X0208-1990      168   */ "\033$)B" },
    { "JISX0212.1990-0:GL", /* JIS_X0212-1990      159   */ "\033$(D" },
    { "JISX0212.1990-0:GR", /* JIS_X0212-1990      159   */ "\033$)D" },
    { "KSC5601.1987-0:GL",  /* KS_C_5601-1987      149   */ "\033$(C" },
    { "KSC5601.1987-0:GR",  /* KS_C_5601-1987      149   */ "\033$)C" },
    { "CNS11643.1986-1:GL", /* CNS 11643-1992 pl.1 171   */ "\033$(G" },
    { "CNS11643.1986-1:GR", /* CNS 11643-1992 pl.1 171   */ "\033$)G" },
    { "CNS11643.1986-2:GL", /* CNS 11643-1992 pl.2 172   */ "\033$(H" },
    { "CNS11643.1986-2:GR", /* CNS 11643-1992 pl.2 172   */ "\033$)H" },
    { "CNS11643.1992-3:GL", /* CNS 11643-1992 pl.3 183   */ "\033$(I" },
    { "CNS11643.1992-3:GR", /* CNS 11643-1992 pl.3 183   */ "\033$)I" },
    { "CNS11643.1992-4:GL", /* CNS 11643-1992 pl.4 184   */ "\033$(J" },
    { "CNS11643.1992-4:GR", /* CNS 11643-1992 pl.4 184   */ "\033$)J" },
    { "CNS11643.1992-5:GL", /* CNS 11643-1992 pl.5 185   */ "\033$(K" },
    { "CNS11643.1992-5:GR", /* CNS 11643-1992 pl.5 185   */ "\033$)K" },
    { "CNS11643.1992-6:GL", /* CNS 11643-1992 pl.6 186   */ "\033$(L" },
    { "CNS11643.1992-6:GR", /* CNS 11643-1992 pl.6 186   */ "\033$)L" },
    { "CNS11643.1992-7:GL", /* CNS 11643-1992 pl.7 187   */ "\033$(M" },
    { "CNS11643.1992-7:GR", /* CNS 11643-1992 pl.7 187   */ "\033$)M" },

    /* Registered encodings with a varying number of bytes per character */
    { "ISO10646-1",         /* UTF-8               196   */ "\033%G"  },

    /* Encodings without ISO-IR assigned escape sequence must be
       defined in XLC_LOCALE files, using "\033%/1" or "\033%/2". */

    /* Backward compatibility with XFree86 3.x */
#if 1
    { "ISO8859-14:GR",                                      "\033%/1" },
    { "ISO8859-15:GR",                                      "\033%/1" },
#endif
    /* For use by utf8 -> ctext */
    { "BIG5-0:GLGR", "\033%/2"},
    { "BIG5HKSCS-0:GLGR", "\033%/2"},
    { "GBK-0:GLGR", "\033%/2"},
    /* used by Emacs, but not backed by ISO-IR */
    { "BIG5-E0:GL", "\033$(0" },
    { "BIG5-E0:GR", "\033$)0" },
    { "BIG5-E1:GL", "\033$(1" },
    { "BIG5-E1:GR", "\033$)1" },

};

/* We represent UTF-8 as an XlcGLGR charset, not in extended segments. */
#define UTF8_IN_EXTSEQ 0

/* ======================= Parsing ESC Sequences ======================= */

#define XctC0		0x0000
#define XctHT		0x0009
#define XctNL		0x000a
#define XctESC		0x001b
#define XctGL		0x0020
#define XctC1		0x0080
#define XctCSI		0x009b
#define XctGR		0x00a0
#define XctSTX		0x0002

#define XctCntrlFunc	0x0023
#define XctMB		0x0024
#define XctOtherCoding	0x0025
#define XctGL94		0x0028
#define XctGR94		0x0029
#define XctGR96		0x002d
#define XctNonStandard	0x002f
#define XctIgnoreExt	0x0030
#define XctNotIgnoreExt	0x0031
#define XctLeftToRight	0x0031
#define XctRightToLeft	0x0032
#define XctDirection	0x005d
#define XctDirectionEnd	0x005d

#define XctGL94MB	0x2428
#define XctGR94MB	0x2429
#define XctExtSeg	0x252f
#define XctReturn	0x2540

/*
 * Parses the header of a Compound Text segment, i.e. the charset designator.
 * The string starts at *text and has *length bytes.
 * Return value is one of:
 *   0 (no valid charset designator),
 *   XctGL94, XctGR94, XctGR96, XctGL94MB, XctGR94MB,
 *   XctLeftToRight, XctRightToLeft, XctDirectionEnd,
 *   XctExtSeg, XctOtherCoding, XctReturn, XctIgnoreExt, XctNotIgnoreExt.
 * If the return value is not 0, *text is incremented and *length decremented,
 * to point past the charset designator. If the return value is one of
 *   XctGL94, XctGR94, XctGR96, XctGL94MB, XctGR94MB,
 *   XctExtSeg, XctOtherCoding, XctIgnoreExt, XctNotIgnoreExt,
 * *final_byte is set to the "final byte" of the charset designator.
 */
static unsigned int
_XlcParseCT(
    const char **text,
    int *length,
    unsigned char *final_byte)
{
    unsigned int ret = 0;
    unsigned char ch;
    const unsigned char *str = (const unsigned char *) *text;

    *final_byte = 0;

    if (*length < 1)
        return 0;
    switch (ch = *str++) {
        case XctESC:
            if (*length < 2)
                return 0;
            switch (ch = *str++) {
                case XctOtherCoding:             /* % */
                    if (*length < 3)
                        return 0;
                    ch = *str++;
                    if (ch == XctNonStandard) {  /* / */
                        if (*length < 4)
                            return 0;
                        ret = XctExtSeg;
                        ch = *str++;
                    } else if (ch == '@') {
                        ret = XctReturn;
                    } else {
                        ret = XctOtherCoding;
                    }
                    *final_byte = ch;
                    break;

                case XctCntrlFunc:               /* # */
                    if (*length < 4)
                        return 0;
                    *final_byte = *str++;
                    switch (*str++) {
                        case XctIgnoreExt:       /* 0 */
                            ret = XctIgnoreExt;
                            break;
                        case XctNotIgnoreExt:    /* 1 */
                            ret = XctNotIgnoreExt;
                            break;
                        default:
                            ret = 0;
                            break;
                    }
                    break;

                case XctMB:                      /* $ */
                    if (*length < 4)
                        return 0;
                    ch = *str++;
                    switch (ch) {
                        case XctGL94:            /* ( */
                            ret = XctGL94MB;
                            break;
                        case XctGR94:            /* ) */
                            ret = XctGR94MB;
                            break;
                        default:
                            ret = 0;
                            break;
                    }
                    *final_byte = *str++;
                    break;

                case XctGL94:                    /* ( */
                    if (*length < 3)
                        return 0;
                    ret = XctGL94;
                    *final_byte = *str++;
                    break;
                case XctGR94:                    /* ) */
                    if (*length < 3)
                        return 0;
                    ret = XctGR94;
                    *final_byte = *str++;
                    break;
                case XctGR96:                    /* - */
                    if (*length < 3)
                        return 0;
                    ret = XctGR96;
                    *final_byte = *str++;
                    break;
            }
            break;
        case XctCSI:
	    /* direction */
            if (*length < 2)
                return 0;
            switch (*str++) {
                case XctLeftToRight:
                    if (*length < 3)
                        return 0;
                    if (*str++ == XctDirection)
                        ret = XctLeftToRight;
                    break;
                case XctRightToLeft:
                    if (*length < 3)
                        return 0;
                    if (*str++ == XctDirection)
                        ret = XctRightToLeft;
                    break;
                case XctDirectionEnd:
                    ret = XctDirectionEnd;
                    break;
            }
            break;
    }

    if (ret) {
        *length -= (const char *) str - *text;
        *text = (const char *) str;
    }
    return ret;
}

/*
 * Fills into a freshly created XlcCharSet the fields that can be inferred
 * from the ESC sequence. These are side, char_size, set_size.
 * Returns True if the charset can be used with Compound Text.
 *
 * Used by _XlcCreateDefaultCharSet.
 */
Bool
_XlcParseCharSet(
    XlcCharSet charset)
{
    unsigned int type;
    unsigned char final_byte;
    const char *ptr = charset->ct_sequence;
    int length;
    int char_size;

    if (*ptr == '\0')
    	return False;

    length = (int) strlen(ptr);

    type = _XlcParseCT(&ptr, &length, &final_byte);

    /* Check for validity and determine char_size.
       char_size = 0 means varying number of bytes per character. */
    switch (type) {
        case XctGL94:
        case XctGR94:
        case XctGR96:
            char_size = 1;
            break;
        case XctGL94MB:
        case XctGR94MB:
            char_size = (final_byte < 0x60 ? 2 : final_byte < 0x70 ? 3 : 4);
            break;
        case XctExtSeg:
            char_size = final_byte - '0';
            if (!(char_size >= 0 && char_size <= 4))
                return False;
            break;
        case XctOtherCoding:
            char_size = 0;
            break;
        default:
            return False;
    }

    charset->char_size = char_size;

    /* Fill in other values. */
    switch (type) {
        case XctGL94:
        case XctGL94MB:
            charset->side = XlcGL;
            charset->set_size = 94;
            break;
        case XctGR94:
        case XctGR94MB:
            charset->side = XlcGR;
            charset->set_size = 94;
            break;
        case XctGR96:
            charset->side = XlcGR;
            charset->set_size = 96;
            break;
        case XctExtSeg:
        case XctOtherCoding:
            charset->side = XlcGLGR;
            charset->set_size = 0;
            break;
    }
    return True;
}


/* =============== Management of the List of Character Sets =============== */

/*
 * Representation of a character set that can be used for Compound Text,
 * at run time.
 * Note: This information is not contained in the XlcCharSet, because
 * multiple ESC sequences may be used for the same XlcCharSet.
 */
typedef struct _CTInfoRec {
    XlcCharSet charset;
    const char *ct_sequence;	/* Compound Text ESC sequence */
    unsigned int type;
    unsigned char final_byte;
				/* If type == XctExtSeg: */
    const char *ext_segment;	/* extended segment name, then '\002' */
    int ext_segment_len;	/* length of above, including final '\002' */

    struct _CTInfoRec *next;
} CTInfoRec, *CTInfo;

/*
 * List of character sets that can be used for Compound Text,
 * Includes all that are listed in default_ct_data, but more can be added
 * at runtime through _XlcAddCT.
 */
static CTInfo ct_list = NULL;
static CTInfo ct_list_end = NULL;

/*
 * Returns a Compound Text info record for an ESC sequence.
 * The first part of the ESC sequence has already been parsed into 'type'
 * and 'final_byte'. The remainder starts at 'text', at least 'text_len'
 * bytes (only used if type == XctExtSeg).
 */
static CTInfo
_XlcGetCTInfo(
    unsigned int type,
    unsigned char final_byte,
    const char *text,
    int text_len)
{
    CTInfo ct_info;

    for (ct_info = ct_list; ct_info; ct_info = ct_info->next)
        if (ct_info->type == type
            && ct_info->final_byte == final_byte
            && (type != XctExtSeg
                || (text_len >= ct_info->ext_segment_len
                    && memcmp(text, ct_info->ext_segment,
                              (size_t) ct_info->ext_segment_len) == 0)))
            return ct_info;

    return (CTInfo) NULL;
}

/* Returns the Compound Text info for a given XlcCharSet.
   Returns NULL if none is found. */
static CTInfo
_XlcGetCTInfoFromCharSet(
    XlcCharSet charset)
{
    CTInfo ct_info;

    for (ct_info = ct_list; ct_info; ct_info = ct_info->next)
	if (ct_info->charset == charset)
	    return ct_info;

    return (CTInfo) NULL;
}

/* Creates a new XlcCharSet, given its name (including side suffix) and
   Compound Text ESC sequence (normally at most 4 bytes), and makes it
   eligible for Compound Text processing. */
XlcCharSet
_XlcAddCT(
    const char *name,
    const char *ct_sequence)
{
    CTInfo ct_info, existing_info;
    XlcCharSet charset;
    const char *ct_ptr;
    int length;
    unsigned int type;
    unsigned char final_byte;

    charset = _XlcGetCharSet(name);
    if (charset != NULL) {
        /* Even if the charset already exists, it is OK to register a second
           Compound Text sequence for it. */
    } else {
        /* Attempt to create the charset. */
        charset = _XlcCreateDefaultCharSet(name, ct_sequence);
        if (charset == NULL)
	    return (XlcCharSet) NULL;
        _XlcAddCharSet(charset);
    }

    /* Allocate a CTinfo record. */
    length = (int) strlen(ct_sequence);
    ct_info = Xmalloc(sizeof(CTInfoRec) + length+1);
    if (ct_info == NULL)
	return charset;

    ct_info->charset = charset;
    ct_info->ct_sequence = strcpy((char *) (ct_info + 1), ct_sequence);

    /* Parse the Compound Text sequence. */
    ct_ptr = ct_sequence;
    type = _XlcParseCT(&ct_ptr, &length, &final_byte);

    ct_info->type = type;
    ct_info->final_byte = final_byte;

    switch (type) {
	case XctGL94:
	case XctGR94:
	case XctGR96:
	case XctGL94MB:
	case XctGR94MB:
	case XctOtherCoding:
            ct_info->ext_segment = NULL;
            ct_info->ext_segment_len = 0;
            break;
	case XctExtSeg: {
            /* By convention, the extended segment name is the encoding_name
               in lowercase. */
            const char *q = charset->encoding_name;
            int n = (int) strlen(q);
            char *p;

            /* Ensure ct_info->ext_segment_len <= 0x3fff - 6. */
            if (n > 0x3fff - 6 - 1) {
                Xfree(ct_info);
                return charset;
            }
            p = Xmalloc(n+1);
            if (p == NULL) {
                Xfree(ct_info);
                return charset;
            }
            ct_info->ext_segment = p;
            ct_info->ext_segment_len = n+1;
            for ( ; n > 0; p++, q++, n--)
                *p = (*q >= 'A' && *q <= 'Z' ? *q - 'A' + 'a' : *q);
            *p = XctSTX;
            break;
        }
	default:
            Xfree(ct_info);
            return (XlcCharSet) NULL;
    }

    /* Insert it into the list, if not already present. */
    existing_info =
        _XlcGetCTInfo(type, ct_info->final_byte,
                      ct_info->ext_segment, ct_info->ext_segment_len);
    if (existing_info == NULL) {
        /* Insert it at the end. If there are duplicates CTinfo entries
           for the same XlcCharSet, we want the first (standard) one to
           override the second (user defined) one. */
	ct_info->next = NULL;
	if (ct_list_end)
	    ct_list_end->next = ct_info;
	else
	    ct_list = ct_info;
	ct_list_end = ct_info;
    } else {
        if (existing_info->charset != charset
            /* We have a conflict, with one exception: JISX0208.1983-0 and
               JISX0208.1990-0 are the same for all practical purposes. */
            && !(strncmp(existing_info->charset->name, "JISX0208", 8) == 0
                 && strncmp(charset->name, "JISX0208", 8) == 0)) {
            fprintf(stderr,
                    "Xlib: charsets %s and %s have the same CT sequence\n",
                    charset->name, existing_info->charset->name);
            if (strcmp(charset->ct_sequence, ct_sequence) == 0)
                charset->ct_sequence = "";
        }
        Xfree(ct_info);
    }

    return charset;
}


/* ========== Converters String <--> CharSet <--> Compound Text ========== */

/*
 * Structure representing the parse state of a Compound Text string.
 */
typedef struct _StateRec {
    XlcCharSet charset;		/* The charset of the current segment */
    XlcCharSet GL_charset;	/* The charset responsible for 0x00..0x7F */
    XlcCharSet GR_charset;	/* The charset responsible for 0x80..0xFF */
    XlcCharSet Other_charset;	/* != NULL if currently in an other segment */
    int ext_seg_left;		/* > 0 if currently in an extended segment */
} StateRec, *State;


/* Subroutine for parsing an ESC sequence. */

typedef enum {
    resOK,		/* Charset saved in 'state', sequence skipped */
    resNotInList,	/* Charset not found, sequence skipped */
    resNotCTSeq		/* EscSeq not recognized, pointers not changed */
} CheckResult;

static CheckResult
_XlcCheckCTSequence(
    State state,
    const char **ctext,
    int *ctext_len)
{
    XlcCharSet charset;
    CTInfo ct_info;
    const char *tmp_ctext = *ctext;
    int tmp_ctext_len = *ctext_len;
    unsigned int type;
    unsigned char final_byte;
    int ext_seg_left = 0;

    /* Check for validity. */
    type = _XlcParseCT(&tmp_ctext, &tmp_ctext_len, &final_byte);

    switch (type) {
	case XctGL94:
	case XctGR94:
	case XctGR96:
	case XctGL94MB:
	case XctGR94MB:
	case XctOtherCoding:
            *ctext = tmp_ctext;
            *ctext_len = tmp_ctext_len;
            break;
        case XctReturn:
            *ctext = tmp_ctext;
            *ctext_len = tmp_ctext_len;
            state->Other_charset = NULL;
            return resOK;
        case XctExtSeg:
            if (tmp_ctext_len > 2
                && (tmp_ctext[0] & 0x80) && (tmp_ctext[1] & 0x80)) {
                unsigned int msb = tmp_ctext[0] & 0x7f;
                unsigned int lsb = tmp_ctext[1] & 0x7f;
                ext_seg_left = (msb << 7) + lsb;
                if (ext_seg_left <= tmp_ctext_len - 2) {
                    *ctext = tmp_ctext + 2;
                    *ctext_len = tmp_ctext_len - 2;
                    break;
                }
            }
            return resNotCTSeq;
        default:
            return resNotCTSeq;
    }

    ct_info = _XlcGetCTInfo(type, final_byte, *ctext, ext_seg_left);

    if (ct_info) {
        charset = ct_info->charset;
        state->ext_seg_left = ext_seg_left;
        if (type == XctExtSeg) {
            state->charset = charset;
            /* Skip past the extended segment name and the separator. */
            *ctext += ct_info->ext_segment_len;
            *ctext_len -= ct_info->ext_segment_len;
            state->ext_seg_left -= ct_info->ext_segment_len;
        } else if (type == XctOtherCoding) {
            state->Other_charset = charset;
        } else {
            if (charset->side == XlcGL) {
                state->GL_charset = charset;
            } else if (charset->side == XlcGR) {
                state->GR_charset = charset;
            } else {
                state->GL_charset = charset;
                state->GR_charset = charset;
            }
        }
        return resOK;
    } else {
        state->ext_seg_left = 0;
        if (type == XctExtSeg) {
            /* Skip the entire extended segment. */
            *ctext += ext_seg_left;
            *ctext_len -= ext_seg_left;
        }
        return resNotInList;
    }
}

static void
init_state(
    XlcConv conv)
{
    State state = (State) conv->state;
    static XlcCharSet default_GL_charset = NULL;
    static XlcCharSet default_GR_charset = NULL;

    if (default_GL_charset == NULL) {
	default_GL_charset = _XlcGetCharSet("ISO8859-1:GL");
	default_GR_charset = _XlcGetCharSet("ISO8859-1:GR");
    }

    /* The initial state is ISO-8859-1 on both sides. */
    state->GL_charset = state->charset = default_GL_charset;
    state->GR_charset = default_GR_charset;

    state->Other_charset = NULL;

    state->ext_seg_left = 0;
}

/* from XlcNCompoundText to XlcNCharSet */

static int
cttocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    State state = (State) conv->state;
    XlcCharSet charset = NULL;
    const char *ctptr;
    char *bufptr;
    int ctext_len, buf_len;
    int unconv_num = 0;

    ctptr = (const char *) *from;
    bufptr = (char *) *to;
    ctext_len = *from_left;
    buf_len = *to_left;

    while (ctext_len > 0 && buf_len > 0) {
        if (state->ext_seg_left == 0) {
            /* Not in the middle of an extended segment; look at next byte. */
            unsigned char ch = *ctptr;
            XlcCharSet ch_charset;

            if (ch == XctESC) {
                CheckResult ret =
                    _XlcCheckCTSequence(state, &ctptr, &ctext_len);
                if (ret == resOK)
                    /* state has been modified. */
                    continue;
                if (ret == resNotInList) {
                    /* XXX Just continue with previous charset. */
                    unconv_num++;
                    continue;
                }
            } else if (ch == XctCSI) {
                /* XXX Simply ignore the XctLeftToRight, XctRightToLeft,
                   XctDirectionEnd sequences for the moment. */
                unsigned char dummy;
                if (_XlcParseCT(&ctptr, &ctext_len, &dummy)) {
                    unconv_num++;
                    continue;
                }
            }

            /* Find the charset which is responsible for this byte. */
            ch_charset = (state->Other_charset != NULL ? state->Other_charset :
                          (ch & 0x80 ? state->GR_charset : state->GL_charset));

            /* Set the charset of this run, or continue the current run,
               or stop the current run. */
            if (charset) {
                if (charset != ch_charset)
                    break;
            } else {
                state->charset = charset = ch_charset;
            }

            /* We don't want to split a character into multiple pieces. */
            if (buf_len < 6) {
                if (charset->char_size > 0) {
                    if (buf_len < charset->char_size)
                        break;
                } else {
                    /* char_size == 0 is tricky. The code here is good only
                       for valid UTF-8 input. */
                    if (charset->ct_sequence[0] == XctESC
                        && charset->ct_sequence[1] == XctOtherCoding
                        && charset->ct_sequence[2] == 'G') {
                        int char_size = (ch < 0xc0 ? 1 :
                                         ch < 0xe0 ? 2 :
                                         ch < 0xf0 ? 3 :
                                         ch < 0xf8 ? 4 :
                                         ch < 0xfc ? 5 :
                                                     6);
                        if (buf_len < char_size)
                            break;
                    }
                }
            }

            *bufptr++ = *ctptr++;
            ctext_len--;
            buf_len--;
        } else {
            /* Copy as much as possible from the current extended segment
               to the buffer. */
            int char_size;

            /* Set the charset of this run, or continue the current run,
               or stop the current run. */
            if (charset) {
                if (charset != state->charset)
                    break;
            } else {
                charset = state->charset;
            }

            char_size = charset->char_size;

            if (state->ext_seg_left <= buf_len || char_size > 0) {
                int n = (state->ext_seg_left <= buf_len
                         ? state->ext_seg_left
                         : (buf_len / char_size) * char_size);
                memcpy(bufptr, ctptr, (size_t) n);
                ctptr += n; ctext_len -= n;
                bufptr += n; buf_len -= n;
                state->ext_seg_left -= n;
            } else {
#if UTF8_IN_EXTSEQ
                /* char_size == 0 is tricky. The code here is good only
                   for valid UTF-8 input. */
                if (strcmp(charset->name, "ISO10646-1") == 0) {
                    unsigned char ch = *ctptr;
                    int char_size = (ch < 0xc0 ? 1 :
                                     ch < 0xe0 ? 2 :
                                     ch < 0xf0 ? 3 :
                                     ch < 0xf8 ? 4 :
                                     ch < 0xfc ? 5 :
                                                 6);
                    int i;
                    if (buf_len < char_size)
                        break;
                    /* A small loop is faster than calling memcpy. */
                    for (i = char_size; i > 0; i--)
                        *bufptr++ = *ctptr++;
                    ctext_len -= char_size;
                    buf_len -= char_size;
                    state->ext_seg_left -= char_size;
                } else
#endif
                {
                    /* Here ctext_len >= state->ext_seg_left > buf_len.
                       We may be splitting a character into multiple pieces.
                       Oh well. */
                    int n = buf_len;
                    memcpy(bufptr, ctptr, (size_t) n);
                    ctptr += n; ctext_len -= n;
                    bufptr += n; buf_len -= n;
                    state->ext_seg_left -= n;
                }
            }
        }
    }

    /* 'charset' is the charset for the current run. In some cases,
       'state->charset' contains the charset for the next run. Therefore,
       return 'charset'.
       'charset' may still be NULL only if no output was produced. */
    if (num_args > 0)
	*((XlcCharSet *) args[0]) = charset;

    *from_left -= ctptr - *((const char **) from);
    *from = (XPointer) ctptr;

    *to_left -= bufptr - *((char **) to);
    *to = (XPointer) bufptr;

    return unconv_num;
}

/* from XlcNCharSet to XlcNCompoundText */

static int
cstoct(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    State state = (State) conv->state;
    XlcSide side;
    unsigned char min_ch = 0, max_ch = 0;
    int length, unconv_num;
    CTInfo ct_info;
    XlcCharSet charset;
    const char *csptr;
    char *ctptr;
    int csstr_len, ct_len;
    char *ext_segment_start;
    int char_size;

    /* One argument is required, of type XlcCharSet. */
    if (num_args < 1)
	return -1;

    csptr = *((const char **) from);
    ctptr = *((char **) to);
    csstr_len = *from_left;
    ct_len = *to_left;

    charset = (XlcCharSet) args[0];

    ct_info = _XlcGetCTInfoFromCharSet(charset);
    if (ct_info == NULL)
	return -1;

    side = charset->side;
    length = (int) strlen(ct_info->ct_sequence);

    ext_segment_start = NULL;

    if (ct_info->type == XctOtherCoding) {
        /* Output the Escape sequence for switching to the charset, and
           reserve room now for the XctReturn sequence at the end. */
        if (ct_len < length + 3)
            return -1;

        memcpy(ctptr, ct_info->ct_sequence, (size_t) length);
        ctptr += length;
        ct_len -= length + 3;
    } else
    /* Test whether the charset is already active. */
    if (((side == XlcGR || side == XlcGLGR)
	 && charset != state->GR_charset)
	|| ((side == XlcGL || side == XlcGLGR)
	    && charset != state->GL_charset)) {

        /* Output the Escape sequence for switching to the charset. */
        if (ct_info->type == XctExtSeg) {
            if (ct_len < length + 2 + ct_info->ext_segment_len)
                return -1;

            memcpy(ctptr, ct_info->ct_sequence, (size_t) length);
            ctptr += length;
            ct_len -= length;

            ctptr += 2;
            ct_len -= 2;
            ext_segment_start = ctptr;

            /* The size of an extended segment must fit in 14 bits. */
            if (ct_len > 0x3fff)
                ct_len = 0x3fff;

            memcpy(ctptr, ct_info->ext_segment, (size_t) ct_info->ext_segment_len);
            ctptr += ct_info->ext_segment_len;
            ct_len -= ct_info->ext_segment_len;
        } else {
            if (ct_len < length)
                return -1;

            memcpy(ctptr, ct_info->ct_sequence, (size_t) length);
            ctptr += length;
            ct_len -= length;
        }
    }

    /* If the charset has side GL or GR, prepare remapping the characters
       to the correct side. */
    if (charset->set_size) {
        min_ch = 0x20;
        max_ch = 0x7f;
        if (charset->set_size == 94) {
            max_ch--;
	    if (charset->char_size > 1 || side == XlcGR)
		min_ch++;
        }
    }

    /* Actually copy the contents. */
    unconv_num = 0;
    char_size = charset->char_size;
    if (char_size == 1) {
	while (csstr_len > 0 && ct_len > 0) {
	    if (charset->set_size) {
		/* The CompoundText specification says that the only
		   control characters allowed are 0x09, 0x0a, 0x1b, 0x9b.
		   Therefore here we eliminate other control characters. */
		unsigned char ch = *((const unsigned char *) csptr) & 0x7f;
		if (!((ch >= min_ch && ch <= max_ch)
		      || (side == XlcGL
			  && (ch == 0x00 || ch == 0x09 || ch == 0x0a))
		      || ((side == XlcGL || side == XlcGR)
			  && (ch == 0x1b)))) {
                    csptr++;
                    csstr_len--;
		    unconv_num++;
                    continue;
 		}
	    }

	    if (side == XlcGL)
		*ctptr++ = *csptr++ & 0x7f;
	    else if (side == XlcGR)
		*ctptr++ = *csptr++ | 0x80;
	    else
		*ctptr++ = *csptr++;
	    csstr_len--;
	    ct_len--;
	}
    } else if (char_size > 1) {
	while (csstr_len >= char_size && ct_len >= char_size) {
	    if (side == XlcGL) {
		int i;
		for (i = char_size; i > 0; i--)
		    *ctptr++ = *csptr++ & 0x7f;
	    } else if (side == XlcGR) {
		int i;
		for (i = char_size; i > 0; i--)
		    *ctptr++ = *csptr++ | 0x80;
	    } else {
		int i;
		for (i = char_size; i > 0; i--)
		    *ctptr++ = *csptr++;
	    }
	    csstr_len -= char_size;
	    ct_len -= char_size;
	}
    } else {
        /* char_size = 0. The code here is good only for valid UTF-8 input. */
        if ((charset->ct_sequence[0] == XctESC
             && charset->ct_sequence[1] == XctOtherCoding
             && charset->ct_sequence[2] == 'G')
#if UTF8_IN_EXTSEQ
            || strcmp(charset->name, "ISO10646-1") == 0
#endif
           ) {
            while (csstr_len > 0 && ct_len > 0) {
                unsigned char ch = * (const unsigned char *) csptr;
                int ch_size = (ch < 0xc0 ? 1 :
                                 ch < 0xe0 ? 2 :
                                 ch < 0xf0 ? 3 :
                                 ch < 0xf8 ? 4 :
                                 ch < 0xfc ? 5 :
                                             6);
                int i;
                if (!(csstr_len >= ch_size && ct_len >= ch_size))
                    break;
                for (i = ch_size; i > 0; i--)
                    *ctptr++ = *csptr++;
                csstr_len -= ch_size;
                ct_len -= ch_size;
            }
        } else {
            while (csstr_len > 0 && ct_len > 0) {
                *ctptr++ = *csptr++;
                csstr_len--;
                ct_len--;
            }
        }
    }

    if (ct_info->type == XctOtherCoding) {
        /* Terminate with an XctReturn sequence. */
        ctptr[0] = XctESC;
        ctptr[1] = XctOtherCoding;
        ctptr[2] = '@';
        ctptr += 3;
    } else if (ext_segment_start != NULL) {
        /* Backpatch the extended segment's length. */
        int ext_segment_length = ctptr - ext_segment_start;
        *(ext_segment_start - 2) = (ext_segment_length >> 7) | 0x80;
        *(ext_segment_start - 1) = (ext_segment_length & 0x7f) | 0x80;
    } else {
        if (side == XlcGR || side == XlcGLGR)
            state->GR_charset = charset;
        if (side == XlcGL || side == XlcGLGR)
            state->GL_charset = charset;
    }

    *from_left -= csptr - *((const char **) from);
    *from = (XPointer) csptr;

    *to_left -= ctptr - *((char **) to);
    *to = (XPointer) ctptr;

    return 0;
}

/* from XlcNString to XlcNCharSet */

static int
strtocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    State state = (State) conv->state;
    const char *src;
    char *dst;
    unsigned char side;
    int length;

    src = (const char *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);
    side = *((const unsigned char *) src) & 0x80;

    while (side == (*((const unsigned char *) src) & 0x80) && length-- > 0)
	*dst++ = *src++;

    *from_left -= src - (const char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0)
	*((XlcCharSet *)args[0]) = (side ? state->GR_charset : state->GL_charset);

    return 0;
}

/* from XlcNCharSet to XlcNString */

static int
cstostr(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    State state = (State) conv->state;
    const char *csptr;
    char *string_ptr;
    int csstr_len, str_len;
    unsigned char ch;
    int unconv_num = 0;

    /* This converter can only convert from ISO8859-1:GL and ISO8859-1:GR. */
    if (num_args < 1
	|| !((XlcCharSet) args[0] == state->GL_charset
	     || (XlcCharSet) args[0] == state->GR_charset))
	return -1;

    csptr = *((const char **) from);
    string_ptr = *((char **) to);
    csstr_len = *from_left;
    str_len = *to_left;

    while (csstr_len > 0 && str_len > 0) {
	ch = *((const unsigned char *) csptr++);
	csstr_len--;
	/* Citing ICCCM: "STRING as a type specifies the ISO Latin-1 character
	   set plus the control characters TAB and NEWLINE." */
	if ((ch < 0x20 && ch != 0x00 && ch != 0x09 && ch != 0x0a)
	    || (ch >= 0x7f && ch < 0xa0)) {
	    unconv_num++;
	    continue;
	}
	*((unsigned char *) string_ptr++) = ch;
	str_len--;
    }

    *from_left -= csptr - *((const char **) from);
    *from = (XPointer) csptr;

    *to_left -= string_ptr - *((char **) to);
    *to = (XPointer) string_ptr;

    return unconv_num;
}


static XlcConv
create_conv(
    XlcConvMethods methods)
{
    XlcConv conv;

    conv = Xmalloc(sizeof(XlcConvRec) + sizeof(StateRec));
    if (conv == NULL)
	return (XlcConv) NULL;

    conv->state = (XPointer) &conv[1];

    conv->methods = methods;

    init_state(conv);

    return conv;
}

static void
close_converter(
    XlcConv conv)
{
    /* conv->state is allocated together with conv, free both at once.  */
    Xfree(conv);
}


static XlcConvMethodsRec cttocs_methods = {
    close_converter,
    cttocs,
    init_state
};

static XlcConv
open_cttocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(&cttocs_methods);
}


static XlcConvMethodsRec cstoct_methods = {
    close_converter,
    cstoct,
    init_state
};

static XlcConv
open_cstoct(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(&cstoct_methods);
}


static XlcConvMethodsRec strtocs_methods = {
    close_converter,
    strtocs,
    init_state
};

static XlcConv
open_strtocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(&strtocs_methods);
}


static XlcConvMethodsRec cstostr_methods = {
    close_converter,
    cstostr,
    init_state
};

static XlcConv
open_cstostr(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(&cstostr_methods);
}


/* =========================== Initialization =========================== */

Bool
_XlcInitCTInfo(void)
{
    if (ct_list == NULL) {
        const CTDataRec *ct_data;
        int num;
        XlcCharSet charset;

        /* Initialize ct_list.  */

	num = sizeof(default_ct_data) / sizeof(CTDataRec);
	for (ct_data = default_ct_data; num > 0; ct_data++, num--) {
	    charset = _XlcAddCT(ct_data->name, ct_data->ct_sequence);
            if (charset == NULL)
                continue;
			if (strncmp(charset->ct_sequence, "\x1b\x25\x2f", 3) != 0)
				charset->source = CSsrcStd;
			else
				charset->source = CSsrcXLC;
	}

        /* Register CompoundText and CharSet converters.  */

        _XlcSetConverter((XLCd) NULL, XlcNCompoundText,
                         (XLCd) NULL, XlcNCharSet,
                         open_cttocs);
        _XlcSetConverter((XLCd) NULL, XlcNString,
                         (XLCd) NULL, XlcNCharSet,
                         open_strtocs);

        _XlcSetConverter((XLCd) NULL, XlcNCharSet,
                         (XLCd) NULL, XlcNCompoundText,
                         open_cstoct);
        _XlcSetConverter((XLCd) NULL, XlcNCharSet,
                         (XLCd) NULL, XlcNString,
                         open_cstostr);
    }

    return True;
}
