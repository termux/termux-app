/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*

Copyright 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifndef SNFSTR_H
#define SNFSTR_H 1

#include <X11/fonts/fntfilio.h>

/*-
 * This file describes the Server Natural Font format.
 * SNF fonts are both CPU-dependent and frame buffer bit order dependent.
 * This file is used by:
 *	1)  the server, to hold font information read out of font files.
 *	2)  font converters
 *
 * Each font file contains the following
 * data structures, with no padding in-between.
 *
 *	1)  The XFONTINFO structure
 *		hand-padded to a two-short boundary.
 *		maxbounds.byteoffset is the total number of bytes in the
 *			glyph array
 *		maxbounds.bitOffset is thetotal width of the unpadded font
 *
 *	2)  The XCHARINFO array
 *		indexed directly with character codes, both on disk
 *		and in memory.
 *
 *	3)  Character glyphs
 *		padded in the server-natural way, and
 *		ordered in the device-natural way.
 *		End of glyphs padded to 32-bit boundary.
 *
 *	4)  nProps font properties
 *
 *	5)  a sequence of null-terminated strings, for font properties
 */

#define FONT_FILE_VERSION	4

typedef struct _snfFontProp {
    CARD32      name;		/* offset of string */
    INT32       value;		/* number or offset of string */
    Bool        indirect;	/* value is a string offset */
}           snfFontPropRec;

/*
 * the following macro definitions describe a font file image in memory
 */
#define ADDRCharInfoRec( pfi)	\
	((snfCharInfoRec *) &(pfi)[1])

#define ADDRCHARGLYPHS( pfi)	\
	(((char *) &(pfi)[1]) + BYTESOFCHARINFO(pfi))

/*
 * pad out glyphs to a CARD32 boundary
 */
#define ADDRXFONTPROPS( pfi)  \
	((snfFontPropRec *) ((char *)ADDRCHARGLYPHS( pfi) + BYTESOFGLYPHINFO(pfi)))

#define ADDRSTRINGTAB( pfi)  \
	((char *)ADDRXFONTPROPS( pfi) + BYTESOFPROPINFO(pfi))

#define n2dChars(pfi)	(((pfi)->lastRow - (pfi)->firstRow + 1) * \
			 ((pfi)->lastCol - (pfi)->firstCol + 1))
#define	BYTESOFFONTINFO(pfi)	(sizeof(snfFontInfoRec))
#define BYTESOFCHARINFO(pfi)	(sizeof(snfCharInfoRec) * n2dChars(pfi))
#define	BYTESOFPROPINFO(pfi)	(sizeof(snfFontPropRec) * (pfi)->nProps)
#define	BYTESOFSTRINGINFO(pfi)	((pfi)->lenStrings)
#define	BYTESOFGLYPHINFO(pfi)	(((pfi)->maxbounds.byteOffset+3) & ~0x3)
#define BYTESOFINKINFO(pfi)	(sizeof(snfCharInfoRec) * n2dChars(pfi))

typedef struct _snfFontProp *snfFontPropPtr;
typedef struct _snfCharInfo *snfCharInfoPtr;
typedef struct _snfFontInfo *snfFontInfoPtr;

typedef struct _snfCharInfo {
    xCharInfo   metrics;	/* info preformatted for Queries */
    unsigned    byteOffset:24;	/* byte offset of the raster from pGlyphs */
    unsigned    exists:1;	/* true iff glyph exists for this char */
    unsigned    pad:7;		/* must be zero for now */
}           snfCharInfoRec;

typedef struct _snfFontInfo {
    unsigned int version1;	/* version stamp */
    unsigned int allExist;
    unsigned int drawDirection;
    unsigned int noOverlap;	/* true if:
				 * max(rightSideBearing-characterWidth) <=
				 * minbounds->metrics.leftSideBearing */
    unsigned int constantMetrics;
    unsigned int terminalFont;	/* Should be deprecated!  true if: constant
				 * metrics && leftSideBearing == 0 &&
				 * rightSideBearing == characterWidth &&
				 * ascent == fontAscent && descent ==
				 * fontDescent */
    unsigned int linear:1;	/* true if firstRow == lastRow */
    unsigned int constantWidth:1;	/* true if
					 * minbounds->metrics.characterWidth
					 * ==
					 * maxbounds->metrics.characterWidth */
    unsigned int inkInside:1;	/* true if for all defined glyphs:
				 * leftSideBearing >= 0 && rightSideBearing <=
				 * characterWidth && -fontDescent <= ascent <=
				 * fontAscent && -fontAscent <= descent <=
				 * fontDescent */
    unsigned int inkMetrics:1;	/* ink metrics != bitmap metrics */
    /* used with terminalFont */
    /* see font's pInk{CI,Min,Max} */
    unsigned int padding:28;
    unsigned int firstCol;
    unsigned int lastCol;
    unsigned int firstRow;
    unsigned int lastRow;
    unsigned int nProps;
    unsigned int lenStrings;	/* length in bytes of string table */
    unsigned int chDefault;	/* default character */
    int         fontDescent;	/* minimum for quality typography */
    int         fontAscent;	/* minimum for quality typography */
    snfCharInfoRec minbounds;	/* MIN of glyph metrics over all chars */
    snfCharInfoRec maxbounds;	/* MAX of glyph metrics over all chars */
    unsigned int pixDepth;	/* intensity bits per pixel */
    unsigned int glyphSets;	/* number of sets of glyphs, for sub-pixel
				 * positioning */
    unsigned int version2;	/* version stamp double-check */
}           snfFontInfoRec;

extern void SnfSetFormat ( int bit, int byte, int glyph, int scan );
extern int snfReadFont ( FontPtr pFont, FontFilePtr file,
			 int bit, int byte, int glyph, int scan );
extern int snfReadFontInfo ( FontInfoPtr pFontInfo, FontFilePtr file );

#endif				/* SNFSTR_H */
