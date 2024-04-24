/*

Copyright 1989, 1998  The Open Group

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
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

#define min_byte2 min_char_or_byte2
#define max_byte2 max_char_or_byte2

/*
 * XTextExtents16 - compute the extents of string given as a sequence of
 * XChar2bs.
 */
int
XTextExtents16 (
    XFontStruct *fs,
    _Xconst XChar2b *string,
    int nchars,
    int *dir,           /* RETURN font information */
    int *font_ascent,   /* RETURN font information */
    int *font_descent,  /* RETURN font information */
    register XCharStruct *overall)	/* RETURN character information */
{
    int i;				/* iterator */
    Bool singlerow = (fs->max_byte1 == 0);  /* optimization */
    int nfound = 0;			/* number of characters found */
    XCharStruct *def;			/* info about default char */

    if (singlerow) {
	CI_GET_DEFAULT_INFO_1D (fs, def);
    } else {
	CI_GET_DEFAULT_INFO_2D (fs, def);
    }

    *dir = fs->direction;
    *font_ascent = fs->ascent;
    *font_descent = fs->descent;

    /*
     * Iterate over the input string getting the appropriate * char struct.
     * The default (which may be null if there is no def_char) will be returned
     * if the character doesn't exist.  On the first time * through the loop,
     * assign the values to overall; otherwise, compute * the new values.
     */

    for (i = 0; i < nchars; i++, string++) {
	register XCharStruct *cs;
	unsigned int r = (unsigned int) string->byte1;	/* watch for macros */
	unsigned int c = (unsigned int) string->byte2;	/* watch for macros */

	if (singlerow) {
	    unsigned int ind = ((r << 8) | c);		/* watch for macros */
	    CI_GET_CHAR_INFO_1D (fs, ind, def, cs);
	} else {
	    CI_GET_CHAR_INFO_2D (fs, r, c, def, cs);
	}

	if (cs) {
	    if (nfound++ == 0) {
		*overall = *cs;
	    } else {
		overall->ascent = max (overall->ascent, cs->ascent);
		overall->descent = max (overall->descent, cs->descent);
		overall->lbearing = min (overall->lbearing,
					 overall->width + cs->lbearing);
		overall->rbearing = max (overall->rbearing,
					 overall->width + cs->rbearing);
		overall->width += cs->width;
	    }
	}
    }

    /*
     * if there were no characters, then set everything to 0
     */
    if (nfound == 0) {
	overall->width = overall->ascent = overall->descent =
	  overall->lbearing = overall->rbearing = 0;
    }

    return 0;
}


/*
 * XTextWidth16 - compute the width of sequence of XChar2bs.  This is a
 * subset of XTextExtents16.
 */
int
XTextWidth16 (
    XFontStruct *fs,
    _Xconst XChar2b *string,
    int count)
{
    int i;				/* iterator */
    Bool singlerow = (fs->max_byte1 == 0);  /* optimization */
    XCharStruct *def;			/* info about default char */
    int width = 0;			/* RETURN value */

    if (singlerow) {
	CI_GET_DEFAULT_INFO_1D (fs, def);
    } else {
	CI_GET_DEFAULT_INFO_2D (fs, def);
    }

    if (def && fs->min_bounds.width == fs->max_bounds.width)
	return (fs->min_bounds.width * count);

    /*
     * Iterate over all character in the input string; only consider characters
     * that exist.
     */
    for (i = 0; i < count; i++, string++) {
	register XCharStruct *cs;
	unsigned int r = (unsigned int) string->byte1;	/* watch for macros */
	unsigned int c = (unsigned int) string->byte2;	/* watch for macros */

	if (singlerow) {
	    unsigned int ind = ((r << 8) | c);		/* watch for macros */
	    CI_GET_CHAR_INFO_1D (fs, ind, def, cs);
	} else {
	    CI_GET_CHAR_INFO_2D (fs, r, c, def, cs);
	}

	if (cs) width += cs->width;
    }

    return width;
}


/*
 * _XTextHeight16 - compute the height of sequence of XChar2bs.
 */
int
_XTextHeight16 (
    XFontStruct *fs,
    _Xconst XChar2b *string,
    int count)
{
    int i;				/* iterator */
    Bool singlerow = (fs->max_byte1 == 0);  /* optimization */
    XCharStruct *def;			/* info about default char */
    int height = 0;			/* RETURN value */

    if (singlerow) {
	CI_GET_DEFAULT_INFO_1D (fs, def);
    } else {
	CI_GET_DEFAULT_INFO_2D (fs, def);
    }

    if (def && (fs->min_bounds.ascent == fs->max_bounds.ascent)
	    && (fs->min_bounds.descent == fs->max_bounds.descent))
	return ((fs->min_bounds.ascent + fs->min_bounds.descent) * count);

    /*
     * Iterate over all character in the input string; only consider characters
     * that exist.
     */
    for (i = 0; i < count; i++, string++) {
	register XCharStruct *cs;
	unsigned int r = (unsigned int) string->byte1;	/* watch for macros */
	unsigned int c = (unsigned int) string->byte2;	/* watch for macros */

	if (singlerow) {
	    unsigned int ind = ((r << 8) | c);		/* watch for macros */
	    CI_GET_CHAR_INFO_1D (fs, ind, def, cs);
	} else {
	    CI_GET_CHAR_INFO_2D (fs, r, c, def, cs);
	}

	if (cs) height += (cs->ascent + cs->descent);
    }

    return height;
}

