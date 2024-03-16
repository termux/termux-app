/************************************************************************

Copyright 1987, 1988, 1998  The Open Group

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


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

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

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xutil.h"
#include <X11/Xregion.h>
#include "poly.h"
#include "reallocarray.h"

#ifdef DEBUG
#include <stdio.h>
#define assert(expr) {if (!(expr)) fprintf(stderr,\
"Assertion failed file %s, line %d: expr\n", __FILE__, __LINE__); }
#else
#define assert(expr)
#endif

typedef int (*overlapProcp)(
        register Region     pReg,
        register BoxPtr     r1,
        BoxPtr              r1End,
        register BoxPtr     r2,
        BoxPtr              r2End,
        short               y1,
        short               y2);

typedef int (*nonOverlapProcp)(
    register Region     pReg,
    register BoxPtr     r,
    BoxPtr              rEnd,
    register short      y1,
    register short      y2);

static void miRegionOp(
    register Region 	newReg,	    	    	/* Place to store result */
    Region	  	reg1,	    	    	/* First region in operation */
    Region	  	reg2,	    	    	/* 2d region in operation */
    int    	  	(*overlapFunc)(
        register Region     pReg,
        register BoxPtr     r1,
        BoxPtr              r1End,
        register BoxPtr     r2,
        BoxPtr              r2End,
        short               y1,
        short               y2),                /* Function to call for over-
						 * lapping bands */
    int    	  	(*nonOverlap1Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register short      y1,
        register short      y2),                /* Function to call for non-
						 * overlapping bands in region
						 * 1 */
    int    	  	(*nonOverlap2Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register short      y1,
        register short      y2));               /* Function to call for non-
						 * overlapping bands in region
						 * 2 */


/*	Create a new empty region	*/
Region
XCreateRegion(void)
{
    Region temp;

    if (! (temp = Xmalloc(sizeof( REGION ))))
	return (Region) NULL;
    if (! (temp->rects = Xmalloc(sizeof( BOX )))) {
	Xfree(temp);
	return (Region) NULL;
    }
    temp->numRects = 0;
    temp->extents.x1 = 0;
    temp->extents.y1 = 0;
    temp->extents.x2 = 0;
    temp->extents.y2 = 0;
    temp->size = 1;
    return( temp );
}

int
XClipBox(
    Region r,
    XRectangle *rect)
{
    rect->x = r->extents.x1;
    rect->y = r->extents.y1;
    rect->width = r->extents.x2 - r->extents.x1;
    rect->height = r->extents.y2 - r->extents.y1;
    return 1;
}

int
XUnionRectWithRegion(
    register XRectangle *rect,
    Region source, Region dest)
{
    REGION region;

    if (!rect->width || !rect->height)
	return 0;
    region.rects = &region.extents;
    region.numRects = 1;
    region.extents.x1 = rect->x;
    region.extents.y1 = rect->y;
    region.extents.x2 = rect->x + rect->width;
    region.extents.y2 = rect->y + rect->height;
    region.size = 1;

    return XUnionRegion(&region, source, dest);
}

/*-
 *-----------------------------------------------------------------------
 * miSetExtents --
 *	Reset the extents of a region to what they should be. Called by
 *	miSubtract and miIntersect b/c they can't figure it out along the
 *	way or do so easily, as miUnion can.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The region's 'extents' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static void
miSetExtents (
    Region	  	pReg)
{
    register BoxPtr	pBox,
			pBoxEnd,
			pExtents;

    if (pReg->numRects == 0)
    {
	pReg->extents.x1 = 0;
	pReg->extents.y1 = 0;
	pReg->extents.x2 = 0;
	pReg->extents.y2 = 0;
	return;
    }

    pExtents = &pReg->extents;
    pBox = pReg->rects;
    pBoxEnd = &pBox[pReg->numRects - 1];

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest y1 and since pBoxEnd is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->x1 = pBox->x1;
    pExtents->y1 = pBox->y1;
    pExtents->x2 = pBoxEnd->x2;
    pExtents->y2 = pBoxEnd->y2;

    assert(pExtents->y1 < pExtents->y2);
    while (pBox <= pBoxEnd)
    {
	if (pBox->x1 < pExtents->x1)
	{
	    pExtents->x1 = pBox->x1;
	}
	if (pBox->x2 > pExtents->x2)
	{
	    pExtents->x2 = pBox->x2;
	}
	pBox++;
    }
    assert(pExtents->x1 < pExtents->x2);
}

int
XSetRegion(
    Display *dpy,
    GC gc,
    register Region r)
{
    register int i;
    register XRectangle *xr, *pr;
    register BOX *pb;
    unsigned long total;

    LockDisplay (dpy);
    total = r->numRects * sizeof (XRectangle);
    if ((xr = (XRectangle *) _XAllocTemp(dpy, total))) {
	for (pr = xr, pb = r->rects, i = r->numRects; --i >= 0; pr++, pb++) {
	    pr->x = pb->x1;
	    pr->y = pb->y1;
	    pr->width = pb->x2 - pb->x1;
	    pr->height = pb->y2 - pb->y1;
	}
    }
    if (xr || !r->numRects)
	_XSetClipRectangles(dpy, gc, 0, 0, xr, r->numRects, YXBanded);
    if (xr)
	_XFreeTemp(dpy, (char *)xr, total);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

int
XDestroyRegion(
    Region r)
{
    Xfree( (char *) r->rects );
    Xfree( (char *) r );
    return 1;
}


/* TranslateRegion(pRegion, x, y)
   translates in place
   added by raymond
*/

int
XOffsetRegion(
    register Region pRegion,
    register int x,
    register int y)
{
    register int nbox;
    register BOX *pbox;

    pbox = pRegion->rects;
    nbox = pRegion->numRects;

    while(nbox--)
    {
	pbox->x1 += x;
	pbox->x2 += x;
	pbox->y1 += y;
	pbox->y2 += y;
	pbox++;
    }
    pRegion->extents.x1 += x;
    pRegion->extents.x2 += x;
    pRegion->extents.y1 += y;
    pRegion->extents.y2 += y;
    return 1;
}

/*
   Utility procedure Compress:
   Replace r by the region r', where
     p in r' iff (Quantifer m <= dx) (p + m in r), and
     Quantifier is Exists if grow is TRUE, For all if grow is FALSE, and
     (x,y) + m = (x+m,y) if xdir is TRUE; (x,y+m) if xdir is FALSE.

   Thus, if xdir is TRUE and grow is FALSE, r is replaced by the region
   of all points p such that p and the next dx points on the same
   horizontal scan line are all in r.  We do this using by noting
   that p is the head of a run of length 2^i + k iff p is the head
   of a run of length 2^i and p+2^i is the head of a run of length
   k. Thus, the loop invariant: s contains the region corresponding
   to the runs of length shift.  r contains the region corresponding
   to the runs of length 1 + dxo & (shift-1), where dxo is the original
   value of dx.  dx = dxo & ~(shift-1).  As parameters, s and t are
   scratch regions, so that we don't have to allocate them on every
   call.
*/

#define ZOpRegion(a,b,c) if (grow) XUnionRegion(a,b,c); \
			 else XIntersectRegion(a,b,c)
#define ZShiftRegion(a,b) if (xdir) XOffsetRegion(a,b,0); \
			  else XOffsetRegion(a,0,b)
#define ZCopyRegion(a,b) XUnionRegion(a,a,b)

static void
Compress(
    Region r, Region s, Region t,
    register unsigned dx,
    register int xdir, register int grow)
{
    register unsigned shift = 1;

    ZCopyRegion(r, s);
    while (dx) {
        if (dx & shift) {
            ZShiftRegion(r, -(int)shift);
            ZOpRegion(r, s, r);
            dx -= shift;
            if (!dx) break;
        }
        ZCopyRegion(s, t);
        ZShiftRegion(s, -(int)shift);
        ZOpRegion(s, t, s);
        shift <<= 1;
    }
}

#undef ZOpRegion
#undef ZShiftRegion
#undef ZCopyRegion

int
XShrinkRegion(
    Region r,
    int dx, int dy)
{
    Region s, t;
    int grow;

    if (!dx && !dy) return 0;
    if (! (s = XCreateRegion()) )
	return 0;
    if (! (t = XCreateRegion()) ) {
	XDestroyRegion(s);
	return 0;
    }
    if ((grow = (dx < 0))) dx = -dx;
    if (dx) Compress(r, s, t, (unsigned) 2*dx, TRUE, grow);
    if ((grow = (dy < 0))) dy = -dy;
    if (dy) Compress(r, s, t, (unsigned) 2*dy, FALSE, grow);
    XOffsetRegion(r, dx, dy);
    XDestroyRegion(s);
    XDestroyRegion(t);
    return 0;
}


/*======================================================================
 *	    Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *	Handle an overlapping band for miIntersect.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
static int
miIntersectO (
    register Region	pReg,
    register BoxPtr	r1,
    BoxPtr  	  	r1End,
    register BoxPtr	r2,
    BoxPtr  	  	r2End,
    short    	  	y1,
    short    	  	y2)
{
    register short  	x1;
    register short  	x2;
    register BoxPtr	pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	x1 = max(r1->x1,r2->x1);
	x2 = min(r1->x2,r2->x2);

	/*
	 * If there's any overlap between the two rectangles, add that
	 * overlap to the new region.
	 * There's no need to check for subsumption because the only way
	 * such a need could arise is if some region has two rectangles
	 * right next to each other. Since that should never happen...
	 */
	if (x1 < x2)
	{
	    assert(y1<y2);

	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->x1 = x1;
	    pNextRect->y1 = y1;
	    pNextRect->x2 = x2;
	    pNextRect->y2 = y2;
	    pReg->numRects += 1;
	    pNextRect++;
	    assert(pReg->numRects <= pReg->size);
	}

	/*
	 * Need to advance the pointers. Shift the one that extends
	 * to the right the least, since the other still has a chance to
	 * overlap with that region's next rectangle, if you see what I mean.
	 */
	if (r1->x2 < r2->x2)
	{
	    r1++;
	}
	else if (r2->x2 < r1->x2)
	{
	    r2++;
	}
	else
	{
	    r1++;
	    r2++;
	}
    }
    return 0;	/* lint */
}

int
XIntersectRegion(
    Region 	  	reg1,
    Region	  	reg2,          /* source regions     */
    register Region 	newReg)               /* destination Region */
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
	(!EXTENTCHECK(&reg1->extents, &reg2->extents)))
        newReg->numRects = 0;
    else
	miRegionOp (newReg, reg1, reg2,
    		miIntersectO, NULL, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(newReg);
    return 1;
}

static int
miRegionCopy(
    register Region dstrgn,
    register Region rgn)

{
    if (dstrgn != rgn) /*  don't want to copy to itself */
    {
        if (dstrgn->size < rgn->numRects)
        {
            if (dstrgn->rects)
            {
		BOX *prevRects = dstrgn->rects;

		dstrgn->rects = Xreallocarray(dstrgn->rects,
                                              rgn->numRects, sizeof(BOX));
		if (! dstrgn->rects) {
		    Xfree(prevRects);
		    dstrgn->size = 0;
		    return 0;
		}
            }
            dstrgn->size = rgn->numRects;
	}
        dstrgn->numRects = rgn->numRects;
        dstrgn->extents.x1 = rgn->extents.x1;
        dstrgn->extents.y1 = rgn->extents.y1;
        dstrgn->extents.x2 = rgn->extents.x2;
        dstrgn->extents.y2 = rgn->extents.y2;

	memcpy((char *) dstrgn->rects, (char *) rgn->rects,
	       (int) (rgn->numRects * sizeof(BOX)));
    }
    return 1;
}

/*======================================================================
 *	    Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *	Attempt to merge the boxes in the current band with those in the
 *	previous one. Used only by miRegionOp.
 *
 * Results:
 *	The new index for the previous band.
 *
 * Side Effects:
 *	If coalescing takes place:
 *	    - rectangles in the previous band will have their y2 fields
 *	      altered.
 *	    - pReg->numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
/* static int*/
static int
miCoalesce(
    register Region	pReg,	    	/* Region to coalesce */
    int	    	  	prevStart,  	/* Index of start of previous band */
    int	    	  	curStart)   	/* Index of start of current band */
{
    register BoxPtr	pPrevBox;   	/* Current box in previous band */
    register BoxPtr	pCurBox;    	/* Current box in current band */
    register BoxPtr	pRegEnd;    	/* End of region */
    int	    	  	curNumRects;	/* Number of rectangles in current
					 * band */
    int	    	  	prevNumRects;	/* Number of rectangles in previous
					 * band */
    int	    	  	bandY1;	    	/* Y1 coordinate for current band */

    pRegEnd = &pReg->rects[pReg->numRects];

    pPrevBox = &pReg->rects[prevStart];
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in miRegionOp
     * at the end when one region has been exhausted.
     */
    pCurBox = &pReg->rects[curStart];
    bandY1 = pCurBox->y1;
    for (curNumRects = 0;
	 (pCurBox != pRegEnd) && (pCurBox->y1 == bandY1);
	 curNumRects++)
    {
	pCurBox++;
    }

    if (pCurBox != pRegEnd)
    {
	/*
	 * If more than one band was added, we have to find the start
	 * of the last band added so the next coalescing job can start
	 * at the right place... (given when multiple bands are added,
	 * this may be pointless -- see above).
	 */
	pRegEnd--;
	while (pRegEnd[-1].y1 == pRegEnd->y1)
	{
	    pRegEnd--;
	}
	curStart = pRegEnd - pReg->rects;
	pRegEnd = pReg->rects + pReg->numRects;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
	pCurBox -= curNumRects;
	/*
	 * The bands may only be coalesced if the bottom of the previous
	 * matches the top scanline of the current.
	 */
	if (pPrevBox->y2 == pCurBox->y1)
	{
	    /*
	     * Make sure the bands have boxes in the same places. This
	     * assumes that boxes have been added in such a way that they
	     * cover the most area possible. I.e. two boxes in a band must
	     * have some horizontal space between them.
	     */
	    do
	    {
		if ((pPrevBox->x1 != pCurBox->x1) ||
		    (pPrevBox->x2 != pCurBox->x2))
		{
		    /*
		     * The bands don't line up so they can't be coalesced.
		     */
		    return (curStart);
		}
		pPrevBox++;
		pCurBox++;
		prevNumRects -= 1;
	    } while (prevNumRects != 0);

	    pReg->numRects -= curNumRects;
	    pCurBox -= curNumRects;
	    pPrevBox -= curNumRects;

	    /*
	     * The bands may be merged, so set the bottom y of each box
	     * in the previous band to that of the corresponding box in
	     * the current band.
	     */
	    do
	    {
		pPrevBox->y2 = pCurBox->y2;
		pPrevBox++;
		pCurBox++;
		curNumRects -= 1;
	    } while (curNumRects != 0);

	    /*
	     * If only one band was added to the region, we have to backup
	     * curStart to the start of the previous band.
	     *
	     * If more than one band was added to the region, copy the
	     * other bands down. The assumption here is that the other bands
	     * came from the same region as the current one and no further
	     * coalescing can be done on them since it's all been done
	     * already... curStart is already in the right place.
	     */
	    if (pCurBox == pRegEnd)
	    {
		curStart = prevStart;
	    }
	    else
	    {
		do
		{
		    *pPrevBox++ = *pCurBox++;
		} while (pCurBox != pRegEnd);
	    }

	}
    }
    return (curStart);
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *	Apply an operation to two regions. Called by miUnion, miInverse,
 *	miSubtract, miIntersect...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The new region is overwritten.
 *
 * Notes:
 *	The idea behind this function is to view the two regions as sets.
 *	Together they cover a rectangle of area that this function divides
 *	into horizontal bands where points are covered only by one region
 *	or by both. For the first case, the nonOverlapFunc is called with
 *	each the band and the band's upper and lower extents. For the
 *	second, the overlapFunc is called to process the entire band. It
 *	is responsible for clipping the rectangles in the band, though
 *	this function provides the boundaries.
 *	At the end of each band, the new region is coalesced, if possible,
 *	to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
static void
miRegionOp(
    register Region 	newReg,	    	    	/* Place to store result */
    Region	  	reg1,	    	    	/* First region in operation */
    Region	  	reg2,	    	    	/* 2d region in operation */
    int    	  	(*overlapFunc)(
        register Region     pReg,
        register BoxPtr     r1,
        BoxPtr              r1End,
        register BoxPtr     r2,
        BoxPtr              r2End,
        short               y1,
        short               y2),                /* Function to call for over-
						 * lapping bands */
    int    	  	(*nonOverlap1Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register short      y1,
        register short      y2),                /* Function to call for non-
						 * overlapping bands in region
						 * 1 */
    int    	  	(*nonOverlap2Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register short      y1,
        register short      y2))                /* Function to call for non-
						 * overlapping bands in region
						 * 2 */
{
    register BoxPtr	r1; 	    	    	/* Pointer into first region */
    register BoxPtr	r2; 	    	    	/* Pointer into 2d region */
    BoxPtr  	  	r1End;	    	    	/* End of 1st region */
    BoxPtr  	  	r2End;	    	    	/* End of 2d region */
    register short  	ybot;	    	    	/* Bottom of intersection */
    register short  	ytop;	    	    	/* Top of intersection */
    BoxPtr  	  	oldRects;   	    	/* Old rects for newReg */
    int	    	  	prevBand;   	    	/* Index of start of
						 * previous band in newReg */
    int	    	  	curBand;    	    	/* Index of start of current
						 * band in newReg */
    register BoxPtr 	r1BandEnd;  	    	/* End of current band in r1 */
    register BoxPtr 	r2BandEnd;  	    	/* End of current band in r2 */
    short     	  	top;	    	    	/* Top of non-overlapping
						 * band */
    short     	  	bot;	    	    	/* Bottom of non-overlapping
						 * band */

    /*
     * Initialization:
     *	set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->rects;
    r2 = reg2->rects;
    r1End = r1 + reg1->numRects;
    r2End = r2 + reg2->numRects;

    oldRects = newReg->rects;

    EMPTY_REGION(newReg);

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    newReg->size = max(reg1->numRects,reg2->numRects) * 2;

    if (! (newReg->rects = Xmallocarray (newReg->size, sizeof(BoxRec)))) {
	newReg->size = 0;
	return;
    }

    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     * 	In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *	For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->extents.y1 < reg2->extents.y1)
	ybot = reg1->extents.y1;
    else
	ybot = reg2->extents.y1;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do
    {
	curBand = newReg->numRects;

	/*
	 * This algorithm proceeds one source-band (as opposed to a
	 * destination band, which is determined by where the two regions
	 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
	 * rectangle after the last one in the current band for their
	 * respective regions.
	 */
	r1BandEnd = r1;
	while ((r1BandEnd != r1End) && (r1BandEnd->y1 == r1->y1))
	{
	    r1BandEnd++;
	}

	r2BandEnd = r2;
	while ((r2BandEnd != r2End) && (r2BandEnd->y1 == r2->y1))
	{
	    r2BandEnd++;
	}

	/*
	 * First handle the band that doesn't intersect, if any.
	 *
	 * Note that attention is restricted to one band in the
	 * non-intersecting region at once, so if a region has n
	 * bands between the current position and the next place it overlaps
	 * the other, this entire loop will be passed through n times.
	 */
	if (r1->y1 < r2->y1)
	{
	    top = max(r1->y1,ybot);
	    bot = min(r1->y2,r2->y1);

	    if ((top != bot) && (nonOverlap1Func != NULL))
	    {
		(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
	    }

	    ytop = r2->y1;
	}
	else if (r2->y1 < r1->y1)
	{
	    top = max(r2->y1,ybot);
	    bot = min(r2->y2,r1->y1);

	    if ((top != bot) && (nonOverlap2Func != NULL))
	    {
		(* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
	    }

	    ytop = r1->y1;
	}
	else
	{
	    ytop = r1->y1;
	}

	/*
	 * If any rectangles got added to the region, try and coalesce them
	 * with rectangles from the previous band. Note we could just do
	 * this test in miCoalesce, but some machines incur a not
	 * inconsiderable cost for function calls, so...
	 */
	if (newReg->numRects != curBand)
	{
	    prevBand = miCoalesce (newReg, prevBand, curBand);
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = min(r1->y2, r2->y2);
	curBand = newReg->numRects;
	if (ybot > ytop)
	{
	    (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

	}

	if (newReg->numRects != curBand)
	{
	    prevBand = miCoalesce (newReg, prevBand, curBand);
	}

	/*
	 * If we've finished with a band (y2 == ybot) we skip forward
	 * in the region to the next band.
	 */
	if (r1->y2 == ybot)
	{
	    r1 = r1BandEnd;
	}
	if (r2->y2 == ybot)
	{
	    r2 = r2BandEnd;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = newReg->numRects;
    if (r1 != r1End)
    {
	if (nonOverlap1Func != NULL)
	{
	    do
	    {
		r1BandEnd = r1;
		while ((r1BandEnd < r1End) && (r1BandEnd->y1 == r1->y1))
		{
		    r1BandEnd++;
		}
		(* nonOverlap1Func) (newReg, r1, r1BandEnd,
				     max(r1->y1,ybot), r1->y2);
		r1 = r1BandEnd;
	    } while (r1 != r1End);
	}
    }
    else if ((r2 != r2End) && (nonOverlap2Func != NULL))
    {
	do
	{
	    r2BandEnd = r2;
	    while ((r2BandEnd < r2End) && (r2BandEnd->y1 == r2->y1))
	    {
		 r2BandEnd++;
	    }
	    (* nonOverlap2Func) (newReg, r2, r2BandEnd,
				max(r2->y1,ybot), r2->y2);
	    r2 = r2BandEnd;
	} while (r2 != r2End);
    }

    if (newReg->numRects != curBand)
    {
	(void) miCoalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
    if (newReg->numRects < (newReg->size >> 1))
    {
	if (REGION_NOT_EMPTY(newReg))
	{
	    BoxPtr prev_rects = newReg->rects;
	    newReg->rects = Xreallocarray (newReg->rects,
                                           newReg->numRects, sizeof(BoxRec));
	    if (! newReg->rects)
		newReg->rects = prev_rects;
	    else
		newReg->size = newReg->numRects;
	}
	else
	{
	    /*
	     * No point in doing the extra work involved in an Xrealloc if
	     * the region is empty
	     */
	    newReg->size = 1;
	    Xfree(newReg->rects);
	    newReg->rects = Xmalloc(sizeof(BoxRec));
	}
    }
    Xfree (oldRects);
    return;
}


/*======================================================================
 *	    Region Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miUnionNonO --
 *	Handle a non-overlapping band for the union operation. Just
 *	Adds the rectangles into the region. Doesn't have to check for
 *	subsumption or anything.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg->numRects is incremented and the final rectangles overwritten
 *	with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
static int
miUnionNonO (
    register Region	pReg,
    register BoxPtr	r,
    BoxPtr  	  	rEnd,
    register short  	y1,
    register short  	y2)
{
    register BoxPtr	pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    assert(y1 < y2);

    while (r != rEnd)
    {
	assert(r->x1 < r->x2);
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->x1 = r->x1;
	pNextRect->y1 = y1;
	pNextRect->x2 = r->x2;
	pNextRect->y2 = y2;
	pReg->numRects += 1;
	pNextRect++;

	assert(pReg->numRects<=pReg->size);
	r++;
    }
    return 0;	/* lint */
}


/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *	Handle an overlapping band for the union operation. Picks the
 *	left-most rectangle each time and merges it into the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles are overwritten in pReg->rects and pReg->numRects will
 *	be changed.
 *
 *-----------------------------------------------------------------------
 */

/* static void*/
static int
miUnionO (
    register Region	pReg,
    register BoxPtr	r1,
    BoxPtr  	  	r1End,
    register BoxPtr	r2,
    BoxPtr  	  	r2End,
    register short	y1,
    register short	y2)
{
    register BoxPtr	pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

#define MERGERECT(r) \
    if ((pReg->numRects != 0) &&  \
	(pNextRect[-1].y1 == y1) &&  \
	(pNextRect[-1].y2 == y2) &&  \
	(pNextRect[-1].x2 >= r->x1))  \
    {  \
	if (pNextRect[-1].x2 < r->x2)  \
	{  \
	    pNextRect[-1].x2 = r->x2;  \
	    assert(pNextRect[-1].x1<pNextRect[-1].x2); \
	}  \
    }  \
    else  \
    {  \
	MEMCHECK(pReg, pNextRect, pReg->rects);  \
	pNextRect->y1 = y1;  \
	pNextRect->y2 = y2;  \
	pNextRect->x1 = r->x1;  \
	pNextRect->x2 = r->x2;  \
	pReg->numRects += 1;  \
        pNextRect += 1;  \
    }  \
    assert(pReg->numRects<=pReg->size);\
    r++;

    assert (y1<y2);
    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r1->x1 < r2->x1)
	{
	    MERGERECT(r1);
	}
	else
	{
	    MERGERECT(r2);
	}
    }

    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
	MERGERECT(r2);
    }
    return 0;	/* lint */
}

int
XUnionRegion(
    Region 	  reg1,
    Region	  reg2,             /* source regions     */
    Region 	  newReg)                  /* destination Region */
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
        if (newReg != reg2)
            return miRegionCopy(newReg, reg2);
        return 1;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
        if (newReg != reg1)
            return miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->numRects == 1) &&
	(reg1->extents.x1 <= reg2->extents.x1) &&
	(reg1->extents.y1 <= reg2->extents.y1) &&
	(reg1->extents.x2 >= reg2->extents.x2) &&
	(reg1->extents.y2 >= reg2->extents.y2))
    {
        if (newReg != reg1)
            return miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->numRects == 1) &&
	(reg2->extents.x1 <= reg1->extents.x1) &&
	(reg2->extents.y1 <= reg1->extents.y1) &&
	(reg2->extents.x2 >= reg1->extents.x2) &&
	(reg2->extents.y2 >= reg1->extents.y2))
    {
        if (newReg != reg2)
            return miRegionCopy(newReg, reg2);
        return 1;
    }

    miRegionOp (newReg, reg1, reg2, miUnionO,
    		miUnionNonO, miUnionNonO);

    newReg->extents.x1 = min(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = min(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = max(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = max(reg1->extents.y2, reg2->extents.y2);

    return 1;
}


/*======================================================================
 * 	    	  Region Subtraction
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miSubtractNonO --
 *	Deal with non-overlapping band for subtraction. Any parts from
 *	region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg may be affected.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
static int
miSubtractNonO1 (
    register Region	pReg,
    register BoxPtr	r,
    BoxPtr  	  	rEnd,
    register short  	y1,
    register short   	y2)
{
    register BoxPtr	pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    assert(y1<y2);

    while (r != rEnd)
    {
	assert(r->x1<r->x2);
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->x1 = r->x1;
	pNextRect->y1 = y1;
	pNextRect->x2 = r->x2;
	pNextRect->y2 = y2;
	pReg->numRects += 1;
	pNextRect++;

	assert(pReg->numRects <= pReg->size);

	r++;
    }
    return 0;	/* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *	Overlapping band subtraction. x1 is the left-most point not yet
 *	checked.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
static int
miSubtractO (
    register Region	pReg,
    register BoxPtr	r1,
    BoxPtr  	  	r1End,
    register BoxPtr	r2,
    BoxPtr  	  	r2End,
    register short  	y1,
    register short  	y2)
{
    register BoxPtr	pNextRect;
    register int  	x1;

    x1 = r1->x1;

    assert(y1<y2);
    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r2->x2 <= x1)
	{
	    /*
	     * Subtrahend missed the boat: go to next subtrahend.
	     */
	    r2++;
	}
	else if (r2->x1 <= x1)
	{
	    /*
	     * Subtrahend precedes minuend: nuke left edge of minuend.
	     */
	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		/*
		 * Minuend completely covered: advance to next minuend and
		 * reset left fence to edge of new minuend.
		 */
		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		/*
		 * Subtrahend now used up since it doesn't extend beyond
		 * minuend
		 */
		r2++;
	    }
	}
	else if (r2->x1 < r1->x2)
	{
	    /*
	     * Left part of subtrahend covers part of minuend: add uncovered
	     * part of minuend to region and skip to next subtrahend.
	     */
	    assert(x1<r2->x1);
	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->x1 = x1;
	    pNextRect->y1 = y1;
	    pNextRect->x2 = r2->x1;
	    pNextRect->y2 = y2;
	    pReg->numRects += 1;
	    pNextRect++;

	    assert(pReg->numRects<=pReg->size);

	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		/*
		 * Minuend used up: advance to new...
		 */
		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		/*
		 * Subtrahend used up
		 */
		r2++;
	    }
	}
	else
	{
	    /*
	     * Minuend used up: add any remaining piece before advancing.
	     */
	    if (r1->x2 > x1)
	    {
		MEMCHECK(pReg, pNextRect, pReg->rects);
		pNextRect->x1 = x1;
		pNextRect->y1 = y1;
		pNextRect->x2 = r1->x2;
		pNextRect->y2 = y2;
		pReg->numRects += 1;
		pNextRect++;
		assert(pReg->numRects<=pReg->size);
	    }
	    r1++;
	    if (r1 != r1End)
		x1 = r1->x1;
	}
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
	assert(x1<r1->x2);
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->x1 = x1;
	pNextRect->y1 = y1;
	pNextRect->x2 = r1->x2;
	pNextRect->y2 = y2;
	pReg->numRects += 1;
	pNextRect++;

	assert(pReg->numRects<=pReg->size);

	r1++;
	if (r1 != r1End)
	{
	    x1 = r1->x1;
	}
    }
    return 0;	/* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *	Subtract regS from regM and leave the result in regD.
 *	S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */

int
XSubtractRegion(
    Region 	  	regM,
    Region	  	regS,
    register Region	regD)
{
   /* check for trivial reject */
    if ( (!(regM->numRects)) || (!(regS->numRects))  ||
	(!EXTENTCHECK(&regM->extents, &regS->extents)) )
    {
	return miRegionCopy(regD, regM);
    }

    miRegionOp (regD, regM, regS, miSubtractO,
    		miSubtractNonO1, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents (regD);
    return 1;
}

int
XXorRegion(Region sra, Region srb, Region dr)
{
    Region tra, trb;

    if (! (tra = XCreateRegion()) )
	return 0;
    if (! (trb = XCreateRegion()) ) {
	XDestroyRegion(tra);
	return 0;
    }
    (void) XSubtractRegion(sra,srb,tra);
    (void) XSubtractRegion(srb,sra,trb);
    (void) XUnionRegion(tra,trb,dr);
    XDestroyRegion(tra);
    XDestroyRegion(trb);
    return 0;
}

/*
 * Check to see if the region is empty.  Assumes a region is passed
 * as a parameter
 */
int
XEmptyRegion(
    Region r)
{
    if( r->numRects == 0 ) return TRUE;
    else  return FALSE;
}

/*
 *	Check to see if two regions are equal
 */
int
XEqualRegion(Region r1, Region r2)
{
    int i;

    if( r1->numRects != r2->numRects ) return FALSE;
    else if( r1->numRects == 0 ) return TRUE;
    else if ( r1->extents.x1 != r2->extents.x1 ) return FALSE;
    else if ( r1->extents.x2 != r2->extents.x2 ) return FALSE;
    else if ( r1->extents.y1 != r2->extents.y1 ) return FALSE;
    else if ( r1->extents.y2 != r2->extents.y2 ) return FALSE;
    else for( i=0; i < r1->numRects; i++ ) {
    	if ( r1->rects[i].x1 != r2->rects[i].x1 ) return FALSE;
    	else if ( r1->rects[i].x2 != r2->rects[i].x2 ) return FALSE;
    	else if ( r1->rects[i].y1 != r2->rects[i].y1 ) return FALSE;
    	else if ( r1->rects[i].y2 != r2->rects[i].y2 ) return FALSE;
    }
    return TRUE;
}

int
XPointInRegion(
    Region pRegion,
    int x, int y)
{
    int i;

    if (pRegion->numRects == 0)
        return FALSE;
    if (!INBOX(pRegion->extents, x, y))
        return FALSE;
    for (i=0; i<pRegion->numRects; i++)
    {
        if (INBOX (pRegion->rects[i], x, y))
	    return TRUE;
    }
    return FALSE;
}

int
XRectInRegion(
    register Region	region,
    int rx, int ry,
    unsigned int rwidth, unsigned int rheight)
{
    register BoxPtr pbox;
    register BoxPtr pboxEnd;
    Box rect;
    register BoxPtr prect = &rect;
    int      partIn, partOut;

    prect->x1 = rx;
    prect->y1 = ry;
    prect->x2 = rwidth + rx;
    prect->y2 = rheight + ry;

    /* this is (just) a useful optimization */
    if ((region->numRects == 0) || !EXTENTCHECK(&region->extents, prect))
        return(RectangleOut);

    partOut = FALSE;
    partIn = FALSE;

    /* can stop when both partOut and partIn are TRUE, or we reach prect->y2 */
    for (pbox = region->rects, pboxEnd = pbox + region->numRects;
	 pbox < pboxEnd;
	 pbox++)
    {

	if (pbox->y2 <= ry)
	   continue;	/* getting up to speed or skipping remainder of band */

	if (pbox->y1 > ry)
	{
	   partOut = TRUE;	/* missed part of rectangle above */
	   if (partIn || (pbox->y1 >= prect->y2))
	      break;
	   ry = pbox->y1;	/* x guaranteed to be == prect->x1 */
	}

	if (pbox->x2 <= rx)
	   continue;		/* not far enough over yet */

	if (pbox->x1 > rx)
	{
	   partOut = TRUE;	/* missed part of rectangle to left */
	   if (partIn)
	      break;
	}

	if (pbox->x1 < prect->x2)
	{
	    partIn = TRUE;	/* definitely overlap */
	    if (partOut)
	       break;
	}

	if (pbox->x2 >= prect->x2)
	{
	   ry = pbox->y2;	/* finished with this band */
	   if (ry >= prect->y2)
	      break;
	   rx = prect->x1;	/* reset x out to left again */
	} else
	{
	    /*
	     * Because boxes in a band are maximal width, if the first box
	     * to overlap the rectangle doesn't completely cover it in that
	     * band, the rectangle must be partially out, since some of it
	     * will be uncovered in that band. partIn will have been set true
	     * by now...
	     */
	    break;
	}

    }

    return(partIn ? ((ry < prect->y2) ? RectanglePart : RectangleIn) :
		RectangleOut);
}
