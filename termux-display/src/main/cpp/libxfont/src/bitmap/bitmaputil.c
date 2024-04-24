/*

Copyright 1990, 1994, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "src/util/replace.h"

#include <X11/fonts/fntfilst.h>
#include <X11/fonts/bitmap.h>
#include <X11/fonts/bdfint.h>

#ifndef MAXSHORT
#define MAXSHORT    32767
#endif

#ifndef MINSHORT
#define MINSHORT    -32768
#endif

static xCharInfo initMinMetrics = {
MAXSHORT, MAXSHORT, MAXSHORT, MAXSHORT, MAXSHORT, 0xFFFF};
static xCharInfo initMaxMetrics = {
MINSHORT, MINSHORT, MINSHORT, MINSHORT, MINSHORT, 0x0000};

#define MINMAX(field,ci) \
	if (minbounds->field > (ci)->field) \
	     minbounds->field = (ci)->field; \
	if (maxbounds->field < (ci)->field) \
	     maxbounds->field = (ci)->field;

#define COMPUTE_MINMAX(ci) \
    if ((ci)->ascent || (ci)->descent || \
	(ci)->leftSideBearing || (ci)->rightSideBearing || \
	(ci)->characterWidth) \
    { \
	MINMAX(ascent, (ci)); \
	MINMAX(descent, (ci)); \
	MINMAX(leftSideBearing, (ci)); \
	MINMAX(rightSideBearing, (ci)); \
	MINMAX(characterWidth, (ci)); \
    }

void
bitmapComputeFontBounds(FontPtr pFont)
{
    BitmapFontPtr  bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
    int         nchars;
    int         r,
                c;
    CharInfoPtr ci;
    int         maxOverlap;
    int         overlap;
    xCharInfo  *minbounds,
               *maxbounds;
    int         i;
    int		numneg = 0, numpos = 0;

    if (bitmapFont->bitmapExtra) {
	minbounds = &bitmapFont->bitmapExtra->info.minbounds;
	maxbounds = &bitmapFont->bitmapExtra->info.maxbounds;
    } else {
	minbounds = &pFont->info.minbounds;
	maxbounds = &pFont->info.maxbounds;
    }
    *minbounds = initMinMetrics;
    *maxbounds = initMaxMetrics;
    maxOverlap = MINSHORT;
    nchars = bitmapFont->num_chars;
    for (i = 0, ci = bitmapFont->metrics; i < nchars; i++, ci++) {
	COMPUTE_MINMAX(&ci->metrics);
	if (ci->metrics.characterWidth < 0)
	    numneg++;
	else
	    numpos++;
	minbounds->attributes &= ci->metrics.attributes;
	maxbounds->attributes |= ci->metrics.attributes;
	overlap = ci->metrics.rightSideBearing - ci->metrics.characterWidth;
	if (maxOverlap < overlap)
	    maxOverlap = overlap;
    }
    if (bitmapFont->bitmapExtra) {
	if (numneg > numpos)
	    bitmapFont->bitmapExtra->info.drawDirection = RightToLeft;
	else
	    bitmapFont->bitmapExtra->info.drawDirection = LeftToRight;
	bitmapFont->bitmapExtra->info.maxOverlap = maxOverlap;
	minbounds = &pFont->info.minbounds;
	maxbounds = &pFont->info.maxbounds;
	*minbounds = initMinMetrics;
	*maxbounds = initMaxMetrics;
        i = 0;
	maxOverlap = MINSHORT;
	for (r = pFont->info.firstRow; r <= pFont->info.lastRow; r++) {
	    for (c = pFont->info.firstCol; c <= pFont->info.lastCol; c++) {
		ci = ACCESSENCODING(bitmapFont->encoding, i);
		if (ci) {
		    COMPUTE_MINMAX(&ci->metrics);
		    if (ci->metrics.characterWidth < 0)
			numneg++;
		    else
			numpos++;
		    minbounds->attributes &= ci->metrics.attributes;
		    maxbounds->attributes |= ci->metrics.attributes;
		    overlap = ci->metrics.rightSideBearing -
			ci->metrics.characterWidth;
		    if (maxOverlap < overlap)
			maxOverlap = overlap;
		}
                i++;
	    }
	}
    }
    if (numneg > numpos)
	pFont->info.drawDirection = RightToLeft;
    else
	pFont->info.drawDirection = LeftToRight;
    pFont->info.maxOverlap = maxOverlap;
}

void
bitmapComputeFontInkBounds(FontPtr pFont)
{
    BitmapFontPtr  bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
    int         nchars;
    int         r,
                c;
    CharInfoPtr cit;
    xCharInfo  *ci;
    int         offset;
    xCharInfo  *minbounds,
               *maxbounds;
    int         i;

    if (!bitmapFont->ink_metrics) {
	if (bitmapFont->bitmapExtra) {
	    bitmapFont->bitmapExtra->info.ink_minbounds = bitmapFont->bitmapExtra->info.minbounds;
	    bitmapFont->bitmapExtra->info.ink_maxbounds = bitmapFont->bitmapExtra->info.maxbounds;
	}
	pFont->info.ink_minbounds = pFont->info.minbounds;
	pFont->info.ink_maxbounds = pFont->info.maxbounds;
    } else {
	if (bitmapFont->bitmapExtra) {
	    minbounds = &bitmapFont->bitmapExtra->info.ink_minbounds;
	    maxbounds = &bitmapFont->bitmapExtra->info.ink_maxbounds;
	} else {
	    minbounds = &pFont->info.ink_minbounds;
	    maxbounds = &pFont->info.ink_maxbounds;
	}
	*minbounds = initMinMetrics;
	*maxbounds = initMaxMetrics;
	nchars = bitmapFont->num_chars;
	for (i = 0, ci = bitmapFont->ink_metrics; i < nchars; i++, ci++) {
	    COMPUTE_MINMAX(ci);
	    minbounds->attributes &= ci->attributes;
	    maxbounds->attributes |= ci->attributes;
	}
	if (bitmapFont->bitmapExtra) {
	    minbounds = &pFont->info.ink_minbounds;
	    maxbounds = &pFont->info.ink_maxbounds;
	    *minbounds = initMinMetrics;
	    *maxbounds = initMaxMetrics;
            i=0;
	    for (r = pFont->info.firstRow; r <= pFont->info.lastRow; r++) {
		for (c = pFont->info.firstCol; c <= pFont->info.lastCol; c++) {
		    cit = ACCESSENCODING(bitmapFont->encoding, i);
		    if (cit) {
			offset = cit - bitmapFont->metrics;
			ci = &bitmapFont->ink_metrics[offset];
			COMPUTE_MINMAX(ci);
			minbounds->attributes &= ci->attributes;
			maxbounds->attributes |= ci->attributes;
		    }
                    i++;
		}
	    }
	}
    }
}

Bool
bitmapAddInkMetrics(FontPtr pFont)
{
    BitmapFontPtr  bitmapFont;
    int         i;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
    bitmapFont->ink_metrics = mallocarray(bitmapFont->num_chars, sizeof(xCharInfo));
    if (!bitmapFont->ink_metrics) {
      fprintf(stderr, "Error: Couldn't allocate ink_metrics (%d*%ld)\n",
	      bitmapFont->num_chars, (unsigned long)sizeof(xCharInfo));
	return FALSE;
    }
    for (i = 0; i < bitmapFont->num_chars; i++)
	FontCharInkMetrics(pFont, &bitmapFont->metrics[i], &bitmapFont->ink_metrics[i]);
    pFont->info.inkMetrics = TRUE;
    return TRUE;
}

/* ARGSUSED */
int
bitmapComputeWeight(FontPtr pFont)
{
    return 10;
}
