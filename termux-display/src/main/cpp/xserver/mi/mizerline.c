/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

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
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>

#include "misc.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "mi.h"
#include "miline.h"

/* Draw lineSolid, fillStyle-independent zero width lines.
 *
 * Must keep X and Y coordinates in "ints" at least until after they're
 * translated and clipped to accommodate CoordModePrevious lines with very
 * large coordinates.
 *
 * Draws the same pixels regardless of sign(dx) or sign(dy).
 *
 * Ken Whaley
 *
 */

/* largest positive value that can fit into a component of a point.
 * Assumes that the point structure is {type x, y;} where type is
 * a signed type.
 */
#define MAX_COORDINATE ((1 << (((sizeof(DDXPointRec) >> 1) << 3) - 1)) - 1)

#define MI_OUTPUT_POINT(xx, yy)\
{\
    if ( !new_span && yy == current_y)\
    {\
        if (xx < spans->x)\
	    spans->x = xx;\
	++*widths;\
    }\
    else\
    {\
        ++Nspans;\
	++spans;\
	++widths;\
	spans->x = xx;\
	spans->y = yy;\
	*widths = 1;\
	current_y = yy;\
        new_span = FALSE;\
    }\
}

void
miZeroLine(DrawablePtr pDraw, GCPtr pGC, int mode,      /* Origin or Previous */
           int npt,             /* number of points */
           DDXPointPtr pptInit)
{
    int Nspans, current_y = 0;
    DDXPointPtr ppt;
    DDXPointPtr pspanInit, spans;
    int *pwidthInit, *widths, list_len;
    int xleft, ytop, xright, ybottom;
    int new_x1, new_y1, new_x2, new_y2;
    int x = 0, y = 0, x1, y1, x2, y2, xstart, ystart;
    int oc1, oc2;
    int result;
    int pt1_clipped, pt2_clipped = 0;
    Bool new_span;
    int signdx, signdy;
    int clipdx, clipdy;
    int width, height;
    int adx, ady;
    int octant;
    unsigned int bias = miGetZeroLineBias(pDraw->pScreen);
    int e, e1, e2, e3;          /* Bresenham error terms */
    int length;                 /* length of lines == # of pixels on major axis */

    xleft = pDraw->x;
    ytop = pDraw->y;
    xright = pDraw->x + pDraw->width - 1;
    ybottom = pDraw->y + pDraw->height - 1;

    if (!pGC->miTranslate) {
        /* do everything in drawable-relative coordinates */
        xleft = 0;
        ytop = 0;
        xright -= pDraw->x;
        ybottom -= pDraw->y;
    }

    /* it doesn't matter whether we're in drawable or screen coordinates,
     * FillSpans simply cannot take starting coordinates outside of the
     * range of a DDXPointRec component.
     */
    if (xright > MAX_COORDINATE)
        xright = MAX_COORDINATE;
    if (ybottom > MAX_COORDINATE)
        ybottom = MAX_COORDINATE;

    /* since we're clipping to the drawable's boundaries & coordinate
     * space boundaries, we're guaranteed that the larger of width/height
     * is the longest span we'll need to output
     */
    width = xright - xleft + 1;
    height = ybottom - ytop + 1;
    list_len = (height >= width) ? height : width;
    pspanInit = xallocarray(list_len, sizeof(DDXPointRec));
    pwidthInit = xallocarray(list_len, sizeof(int));
    if (!pspanInit || !pwidthInit) {
        free(pspanInit);
        free(pwidthInit);
        return;
    }
    Nspans = 0;
    new_span = TRUE;
    spans = pspanInit - 1;
    widths = pwidthInit - 1;
    ppt = pptInit;

    xstart = ppt->x;
    ystart = ppt->y;
    if (pGC->miTranslate) {
        xstart += pDraw->x;
        ystart += pDraw->y;
    }

    /* x2, y2, oc2 copied to x1, y1, oc1 at top of loop to simplify
     * iteration logic
     */
    x2 = xstart;
    y2 = ystart;
    oc2 = 0;
    MIOUTCODES(oc2, x2, y2, xleft, ytop, xright, ybottom);

    while (--npt > 0) {
        x1 = x2;
        y1 = y2;
        oc1 = oc2;
        ++ppt;

        x2 = ppt->x;
        y2 = ppt->y;
        if (pGC->miTranslate && (mode != CoordModePrevious)) {
            x2 += pDraw->x;
            y2 += pDraw->y;
        }
        else if (mode == CoordModePrevious) {
            x2 += x1;
            y2 += y1;
        }

        oc2 = 0;
        MIOUTCODES(oc2, x2, y2, xleft, ytop, xright, ybottom);

        CalcLineDeltas(x1, y1, x2, y2, adx, ady, signdx, signdy, 1, 1, octant);

        if (ady + 1 > (list_len - Nspans)) {
            (*pGC->ops->FillSpans) (pDraw, pGC, Nspans, pspanInit,
                                    pwidthInit, FALSE);
            Nspans = 0;
            spans = pspanInit - 1;
            widths = pwidthInit - 1;
        }
        new_span = TRUE;
        if (adx > ady) {
            e1 = ady << 1;
            e2 = e1 - (adx << 1);
            e = e1 - adx;
            length = adx;       /* don't draw endpoint in main loop */

            FIXUP_ERROR(e, octant, bias);

            new_x1 = x1;
            new_y1 = y1;
            new_x2 = x2;
            new_y2 = y2;
            pt1_clipped = 0;
            pt2_clipped = 0;

            if ((oc1 | oc2) != 0) {
                result = miZeroClipLine(xleft, ytop, xright, ybottom,
                                        &new_x1, &new_y1, &new_x2, &new_y2,
                                        adx, ady,
                                        &pt1_clipped, &pt2_clipped,
                                        octant, bias, oc1, oc2);
                if (result == -1)
                    continue;

                length = abs(new_x2 - new_x1);

                /* if we've clipped the endpoint, always draw the full length
                 * of the segment, because then the capstyle doesn't matter
                 */
                if (pt2_clipped)
                    length++;

                if (pt1_clipped) {
                    /* must calculate new error terms */
                    clipdx = abs(new_x1 - x1);
                    clipdy = abs(new_y1 - y1);
                    e += (clipdy * e2) + ((clipdx - clipdy) * e1);
                }
            }

            /* draw the segment */

            x = new_x1;
            y = new_y1;

            e3 = e2 - e1;
            e = e - e1;

            while (length--) {
                MI_OUTPUT_POINT(x, y);
                e += e1;
                if (e >= 0) {
                    y += signdy;
                    e += e3;
                }
                x += signdx;
            }
        }
        else {                  /* Y major line */

            e1 = adx << 1;
            e2 = e1 - (ady << 1);
            e = e1 - ady;
            length = ady;       /* don't draw endpoint in main loop */

            SetYMajorOctant(octant);
            FIXUP_ERROR(e, octant, bias);

            new_x1 = x1;
            new_y1 = y1;
            new_x2 = x2;
            new_y2 = y2;
            pt1_clipped = 0;
            pt2_clipped = 0;

            if ((oc1 | oc2) != 0) {
                result = miZeroClipLine(xleft, ytop, xright, ybottom,
                                        &new_x1, &new_y1, &new_x2, &new_y2,
                                        adx, ady,
                                        &pt1_clipped, &pt2_clipped,
                                        octant, bias, oc1, oc2);
                if (result == -1)
                    continue;

                length = abs(new_y2 - new_y1);

                /* if we've clipped the endpoint, always draw the full length
                 * of the segment, because then the capstyle doesn't matter
                 */
                if (pt2_clipped)
                    length++;

                if (pt1_clipped) {
                    /* must calculate new error terms */
                    clipdx = abs(new_x1 - x1);
                    clipdy = abs(new_y1 - y1);
                    e += (clipdx * e2) + ((clipdy - clipdx) * e1);
                }
            }

            /* draw the segment */

            x = new_x1;
            y = new_y1;

            e3 = e2 - e1;
            e = e - e1;

            while (length--) {
                MI_OUTPUT_POINT(x, y);
                e += e1;
                if (e >= 0) {
                    x += signdx;
                    e += e3;
                }
                y += signdy;
            }
        }
    }

    /* only do the capnotlast check on the last segment
     * and only if the endpoint wasn't clipped.  And then, if the last
     * point is the same as the first point, do not draw it, unless the
     * line is degenerate
     */
    if ((!pt2_clipped) && (pGC->capStyle != CapNotLast) &&
        (((xstart != x2) || (ystart != y2)) || (ppt == pptInit + 1))) {
        MI_OUTPUT_POINT(x, y);
    }

    if (Nspans > 0)
        (*pGC->ops->FillSpans) (pDraw, pGC, Nspans, pspanInit,
                                pwidthInit, FALSE);

    free(pwidthInit);
    free(pspanInit);
}

void
miZeroDashLine(DrawablePtr dst, GCPtr pgc, int mode, int nptInit,       /* number of points in polyline */
               DDXPointRec * pptInit    /* points in the polyline */
    )
{
    /* XXX kludge until real zero-width dash code is written */
    pgc->lineWidth = 1;
    miWideDash(dst, pgc, mode, nptInit, pptInit);
    pgc->lineWidth = 0;
}
