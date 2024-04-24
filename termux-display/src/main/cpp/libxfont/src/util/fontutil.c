/*

Copyright 1991, 1998  The Open Group

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
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "src/util/replace.h"
#include    <X11/fonts/fontmisc.h>
#include    <X11/fonts/fontstruct.h>
#include    <X11/fonts/FSproto.h>
#include    <X11/fonts/fontutil.h>

/* Define global here...  doesn't hurt the servers, and avoids
   unresolved references in font clients.  */

static int defaultGlyphCachingMode = DEFAULT_GLYPH_CACHING_MODE;
int glyphCachingMode = DEFAULT_GLYPH_CACHING_MODE;

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))

void
xfont2_query_glyph_extents(FontPtr pFont,
			   CharInfoPtr *charinfo,
			   unsigned long count,
			   ExtentInfoRec *info)
{
    register unsigned long i;
    xCharInfo  *pCI;

    info->drawDirection = pFont->info.drawDirection;

    info->fontAscent = pFont->info.fontAscent;
    info->fontDescent = pFont->info.fontDescent;

    if (count != 0) {

	pCI = &((*charinfo)->metrics); charinfo++;
	/* ignore nonexisting characters when calculating text extents */
	if ( !((pCI->characterWidth == 0)
	       && (pCI->rightSideBearing == 0)
	       && (pCI->leftSideBearing == 0)
	       && (pCI->ascent == 0)
	       && (pCI->descent == 0)) ) {
	    info->overallAscent = pCI->ascent;
	    info->overallDescent = pCI->descent;
	    info->overallLeft = pCI->leftSideBearing;
	    info->overallRight = pCI->rightSideBearing;
	    info->overallWidth = pCI->characterWidth;
	}

	if (pFont->info.constantMetrics && pFont->info.noOverlap) {
	    info->overallWidth *= count;
	    info->overallRight += (info->overallWidth -
				   pCI->characterWidth);
	} else {
	    for (i = 1; i < count; i++) {
		pCI = &((*charinfo)->metrics); charinfo++;
		/* ignore nonexisting characters when calculating extents */
		if ( !((pCI->characterWidth == 0)
		       && (pCI->rightSideBearing == 0)
		       && (pCI->leftSideBearing == 0)
		       && (pCI->ascent == 0)
		       && (pCI->descent == 0)) ) {
		    info->overallAscent = MAX(
					      info->overallAscent,
					      pCI->ascent);
		    info->overallDescent = MAX(
					       info->overallDescent,
					       pCI->descent);
		    info->overallLeft = MIN(
					    info->overallLeft,
					    info->overallWidth + pCI->leftSideBearing);
		    info->overallRight = MAX(
					     info->overallRight,
					     info->overallWidth + pCI->rightSideBearing);
		    /*
		     * yes, this order is correct; overallWidth IS incremented
		     * last
		     */
		    info->overallWidth += pCI->characterWidth;
		}
	    }
	}
    } else {
	info->overallAscent = 0;
	info->overallDescent = 0;
	info->overallWidth = 0;
	info->overallLeft = 0;
	info->overallRight = 0;
    }
}

Bool
xfont2_query_text_extents(FontPtr pFont,
			  unsigned long count,
			  unsigned char *chars,
			  ExtentInfoRec *info)
{
    xCharInfo     **charinfo;
    unsigned long   n;
    FontEncoding    encoding;
    int             cm;
    int             i;
    unsigned long   t;
    xCharInfo      *defaultChar = 0;
    unsigned char   defc[2];
    int             firstReal;

    charinfo = mallocarray(count, sizeof(xCharInfo *));
    if (!charinfo)
	return FALSE;
    encoding = TwoD16Bit;
    if (pFont->info.lastRow == 0)
	encoding = Linear16Bit;
    (*pFont->get_metrics) (pFont, count, chars, encoding, &n, charinfo);

    /* Do default character substitution as get_metrics doesn't */

#define IsNonExistentChar(ci) (!(ci) || \
			       ((ci)->ascent == 0 && \
			       (ci)->descent == 0 && \
			       (ci)->leftSideBearing == 0 && \
			       (ci)->rightSideBearing == 0 && \
			       (ci)->characterWidth == 0))

    firstReal = n;
    defc[0] = pFont->info.defaultCh >> 8;
    defc[1] = pFont->info.defaultCh;
    (*pFont->get_metrics) (pFont, 1, defc, encoding, &t, &defaultChar);
    if ((IsNonExistentChar (defaultChar)))
	defaultChar = 0;
    for (i = 0; i < n; i++)
    {
	if ((IsNonExistentChar (charinfo[i])))
	{
	    if (!defaultChar)
		continue;
	    charinfo[i] = defaultChar;
	}
	if (firstReal == n)
	    firstReal = i;
    }
    cm = pFont->info.constantMetrics;
    pFont->info.constantMetrics = FALSE;
    xfont2_query_glyph_extents(pFont, (CharInfoPtr*) charinfo + firstReal,
			       n - firstReal, info);
    pFont->info.constantMetrics = cm;
    free(charinfo);
    return TRUE;
}

Bool
xfont2_parse_glyph_caching_mode(char *str)
{
    if (!strcmp(str, "none")) defaultGlyphCachingMode = CACHING_OFF;
    else if (!strcmp(str, "all")) defaultGlyphCachingMode = CACHE_ALL_GLYPHS;
    else if (!strcmp(str, "16")) defaultGlyphCachingMode = CACHE_16_BIT_GLYPHS;
    else return FALSE;
    return TRUE;
}

void
xfont2_init_glyph_caching(void)
{
    /* Set glyphCachingMode to the mode the server hopes to
       support.  DDX drivers that do not support the requested level
       of glyph caching can call SetGlyphCachingMode to lower the
       level of support.
     */

    glyphCachingMode = defaultGlyphCachingMode;
}

/* ddxen can call SetGlyphCachingMode to inform us of what level of glyph
 * caching they can support.
 */
void
xfont2_set_glyph_caching_mode(int newmode)
{
    if ( (glyphCachingMode > newmode) && (newmode >= 0) )
	glyphCachingMode = newmode;
}

#define range_alloc_granularity 16
#define mincharp(p) ((p)->min_char_low + ((p)->min_char_high << 8))
#define maxcharp(p) ((p)->max_char_low + ((p)->max_char_high << 8))

/* add_range(): Add range to a list of ranges, with coalescence */
int
add_range(fsRange *newrange,
	  int *nranges,
	  fsRange **range,
	  Bool charset_subset)
{
    int first, last, middle;
    unsigned long keymin, keymax;
    unsigned long ptrmin = 0, ptrmax = 0;
    fsRange *ptr = NULL, *ptr1, *ptr2, *endptr;

    /* There are two different ways to treat ranges:

       1) Charset subsetting (support of the HP XLFD enhancements), in
	  which a range of 0x1234,0x3456 means all numbers between
	  0x1234 and 0x3456, and in which min and max might be swapped.

       2) Row/column ranges, in which a range of 0x1234,0x3456 means the
	  ranges 0x1234-0x1256, 0x1334-0x1356, ...  , 0x3434-0x3456.
	  This is for support of glyph caching.

       The choice of treatment is selected with the "charset_subset"
       flag */

    /* If newrange covers multiple rows; break up the rows */
    if (!charset_subset && newrange->min_char_high != newrange->max_char_high)
    {
	int i, err = 0;
	fsRange temprange;
	for (i = newrange->min_char_high;
	     i <= newrange->max_char_high;
	     i++)
	{
	    temprange.min_char_low = newrange->min_char_low;
	    temprange.max_char_low = newrange->max_char_low;
	    temprange.min_char_high = temprange.max_char_high = i;
	    err = add_range(&temprange, nranges, range, charset_subset);
	    if (err != Successful) break;
	}
	return err;
    }

    keymin = mincharp(newrange);
    keymax = maxcharp(newrange);

    if (charset_subset && keymin > keymax)
    {
	unsigned long temp = keymin;
	keymin = keymax;
	keymax = temp;
    }

    /* add_range() maintains a sorted list; this makes possible coalescence
       and binary searches */

    /* Binary search for a range with which the new range can merge */

    first = middle = 0;
    last = *nranges - 1;
    while (last >= first)
    {
	middle = (first + last) / 2;
	ptr = (*range) + middle;
	ptrmin = mincharp(ptr);
	ptrmax = maxcharp(ptr);

	if (ptrmin > 0 && keymax < ptrmin - 1) last = middle - 1;
	else if (keymin > ptrmax + 1) first = middle + 1;
	else if (!charset_subset)
	{
	    /* We might have a range with which to merge... IF the
	       result doesn't cross rows */
	    if (newrange->min_char_high != ptr->min_char_high)
		last = first - 1;	/* Force adding a new range */
	    break;
	}
	else break;	/* We have at least one range with which we can merge */
    }

    if (last < first)
    {
	/* Search failed; we need to add a new range to the list. */

	/* Grow the list if necessary */
	if (*nranges == 0 || *range == (fsRange *)0)
	{
	    *range = mallocarray(range_alloc_granularity, SIZEOF(fsRange));
	    *nranges = 0;
	}
	else if (!(*nranges % range_alloc_granularity))
	{
	    *range = reallocarray(*range, (*nranges + range_alloc_granularity),
				  SIZEOF(fsRange));
	}

	/* If alloc failed, just return a null list */
	if (*range == (fsRange *)0)
	{
	    *nranges = 0;
	    return AllocError;
	}

	/* Should new entry go *at* or *after* ptr? */
	ptr = (*range) + middle;
	if (middle < *nranges && keymin > ptrmin) ptr++;	/* after */

	/* Open up a space for our new range */
	memmove((char *)(ptr + 1),
		(char *)ptr,
		(char *)(*range + *nranges) - (char *)ptr);

	/* Insert the new range */
	ptr->min_char_low = keymin & 0xff;
	ptr->min_char_high = keymin >> 8;
	ptr->max_char_low = keymax & 0xff;
	ptr->max_char_high = keymax >> 8;

	/* Update range count */
	(*nranges)++;

	/* Done */
	return Successful;
    }

    /* Join our new range to that pointed to by "ptr" */
    if (keymin < ptrmin)
    {
	ptr->min_char_low = keymin & 0xff;
	ptr->min_char_high = keymin >> 8;
    }
    if (keymax > ptrmax)
    {
	ptr->max_char_low = keymax & 0xff;
	ptr->max_char_high = keymax >> 8;
    }

    ptrmin = mincharp(ptr);
    ptrmax = maxcharp(ptr);

    endptr = *range + *nranges;

    for (ptr1 = ptr; ptr1 >= *range; ptr1--)
    {
	if (ptrmin <= maxcharp(ptr1) + 1)
	{
	    if (!charset_subset && ptr->min_char_high != ptr1->min_char_high)
		break;
	    if (ptrmin >= mincharp(ptr1))
		ptrmin = mincharp(ptr1);
	}
	else break;
    }
    for (ptr2 = ptr; ptr2 < endptr; ptr2++)
    {
	if ((ptr2->min_char_low == 0 && ptr2->min_char_high == 0) ||
	    ptrmax >= mincharp(ptr2) - 1)
	{
	    if (!charset_subset && ptr->min_char_high != ptr2->min_char_high)
		break;
	    if (ptrmax <= maxcharp(ptr2))
		ptrmax = maxcharp(ptr2);
	}
	else break;
    }

    /* We need to coalesce ranges between ptr1 and ptr2 exclusive */
    ptr1++;
    ptr2--;
    if (ptr1 != ptr2)
    {
	memmove(ptr1, ptr2, (char *)endptr - (char *)ptr2);
	*nranges -= (ptr2 - ptr1);
    }

    /* Write the new range into the range list */
    ptr1->min_char_low = ptrmin & 0xff;
    ptr1->min_char_high = ptrmin >> 8;
    ptr1->max_char_low = ptrmax & 0xff;
    ptr1->max_char_high = ptrmax >> 8;

    return Successful;
}
