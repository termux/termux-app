/*

Copyright 1990, 1998  The Open Group

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
#include    <X11/fonts/fontmisc.h>
#include    <X11/fonts/fontstruct.h>
#include    <X11/fonts/fontutil.h>

void
FontComputeInfoAccelerators(FontInfoPtr pFontInfo)
{
    pFontInfo->noOverlap = FALSE;
    if (pFontInfo->maxOverlap <= pFontInfo->minbounds.leftSideBearing)
	pFontInfo->noOverlap = TRUE;

    if ((pFontInfo->minbounds.ascent == pFontInfo->maxbounds.ascent) &&
	    (pFontInfo->minbounds.descent == pFontInfo->maxbounds.descent) &&
	    (pFontInfo->minbounds.leftSideBearing ==
	     pFontInfo->maxbounds.leftSideBearing) &&
	    (pFontInfo->minbounds.rightSideBearing ==
	     pFontInfo->maxbounds.rightSideBearing) &&
	    (pFontInfo->minbounds.characterWidth ==
	     pFontInfo->maxbounds.characterWidth) &&
      (pFontInfo->minbounds.attributes == pFontInfo->maxbounds.attributes)) {
	pFontInfo->constantMetrics = TRUE;
	if ((pFontInfo->maxbounds.leftSideBearing == 0) &&
		(pFontInfo->maxbounds.rightSideBearing ==
		 pFontInfo->maxbounds.characterWidth) &&
		(pFontInfo->maxbounds.ascent == pFontInfo->fontAscent) &&
		(pFontInfo->maxbounds.descent == pFontInfo->fontDescent))
	    pFontInfo->terminalFont = TRUE;
	else
	    pFontInfo->terminalFont = FALSE;
    } else {
	pFontInfo->constantMetrics = FALSE;
	pFontInfo->terminalFont = FALSE;
    }
    if (pFontInfo->minbounds.characterWidth == pFontInfo->maxbounds.characterWidth)
	pFontInfo->constantWidth = TRUE;
    else
	pFontInfo->constantWidth = FALSE;

    if ((pFontInfo->minbounds.leftSideBearing >= 0) &&
	    (pFontInfo->maxOverlap <= 0) &&
	    (pFontInfo->minbounds.ascent >= -pFontInfo->fontDescent) &&
	    (pFontInfo->maxbounds.ascent <= pFontInfo->fontAscent) &&
	    (-pFontInfo->minbounds.descent <= pFontInfo->fontAscent) &&
	    (pFontInfo->maxbounds.descent <= pFontInfo->fontDescent))
	pFontInfo->inkInside = TRUE;
    else
	pFontInfo->inkInside = FALSE;
}

int
FontCouldBeTerminal(FontInfoPtr pFontInfo)
{
    if ((pFontInfo->minbounds.leftSideBearing >= 0) &&
	    (pFontInfo->maxbounds.rightSideBearing <= pFontInfo->maxbounds.characterWidth) &&
	    (pFontInfo->minbounds.characterWidth == pFontInfo->maxbounds.characterWidth) &&
	    (pFontInfo->maxbounds.ascent <= pFontInfo->fontAscent) &&
	    (pFontInfo->maxbounds.descent <= pFontInfo->fontDescent) &&
	    (pFontInfo->maxbounds.leftSideBearing != 0 ||
	     pFontInfo->minbounds.rightSideBearing != pFontInfo->minbounds.characterWidth ||
	     pFontInfo->minbounds.ascent != pFontInfo->fontAscent ||
	     pFontInfo->minbounds.descent != pFontInfo->fontDescent)) {
	/* blow off font with nothing but a SPACE */
	if (pFontInfo->maxbounds.ascent == 0 &&
	    pFontInfo->maxbounds.descent == 0)
		return FALSE;
	return TRUE;
    }
    return FALSE;
}
