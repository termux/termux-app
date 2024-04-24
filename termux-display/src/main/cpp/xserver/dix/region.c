/***********************************************************

Copyright 1987, 1988, 1989, 1998  The Open Group

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


Copyright 1987, 1988, 1989 by
Digital Equipment Corporation, Maynard, Massachusetts.

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

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "regionstr.h"
#include <X11/Xprotostr.h>
#include <X11/Xfuncproto.h>
#include "gc.h"
#include <pixman.h>

#undef assert
#ifdef REGION_DEBUG
#define assert(expr) { \
            CARD32 *foo = NULL; \
            if (!(expr)) { \
                ErrorF("Assertion failed file %s, line %d: %s\n", \
                       __FILE__, __LINE__, #expr); \
                *foo = 0xdeadbeef; /* to get a backtrace */ \
            } \
        }
#else
#define assert(expr)
#endif

#define good(reg) assert(RegionIsValid(reg))

/*
 * The functions in this file implement the Region abstraction used extensively
 * throughout the X11 sample server. A Region is simply a set of disjoint
 * (non-overlapping) rectangles, plus an "extent" rectangle which is the
 * smallest single rectangle that contains all the non-overlapping rectangles.
 *
 * A Region is implemented as a "y-x-banded" array of rectangles.  This array
 * imposes two degrees of order.  First, all rectangles are sorted by top side
 * y coordinate first (y1), and then by left side x coordinate (x1).
 *
 * Furthermore, the rectangles are grouped into "bands".  Each rectangle in a
 * band has the same top y coordinate (y1), and each has the same bottom y
 * coordinate (y2).  Thus all rectangles in a band differ only in their left
 * and right side (x1 and x2).  Bands are implicit in the array of rectangles:
 * there is no separate list of band start pointers.
 *
 * The y-x band representation does not minimize rectangles.  In particular,
 * if a rectangle vertically crosses a band (the rectangle has scanlines in
 * the y1 to y2 area spanned by the band), then the rectangle may be broken
 * down into two or more smaller rectangles stacked one atop the other.
 *
 *  -----------				    -----------
 *  |         |				    |         |		    band 0
 *  |         |  --------		    -----------  --------
 *  |         |  |      |  in y-x banded    |         |  |      |   band 1
 *  |         |  |      |  form is	    |         |  |      |
 *  -----------  |      |		    -----------  --------
 *               |      |				 |      |   band 2
 *               --------				 --------
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible: no two rectangles within a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course).
 *
 * Adam de Boor wrote most of the original region code.  Joel McCormack
 * substantially modified or rewrote most of the core arithmetic routines,
 * and added RegionValidate in order to support several speed improvements
 * to miValidateTree.  Bob Scheifler changed the representation to be more
 * compact when empty or a single rectangle, and did a bunch of gratuitous
 * reformatting.
 */

/*  true iff two Boxes overlap */
#define EXTENTCHECK(r1,r2) \
      (!( ((r1)->x2 <= (r2)->x1)  || \
          ((r1)->x1 >= (r2)->x2)  || \
          ((r1)->y2 <= (r2)->y1)  || \
          ((r1)->y1 >= (r2)->y2) ) )

/* true iff (x,y) is in Box */
#define INBOX(r,x,y) \
      ( ((r)->x2 >  x) && \
        ((r)->x1 <= x) && \
        ((r)->y2 >  y) && \
        ((r)->y1 <= y) )

/* true iff Box r1 contains Box r2 */
#define SUBSUMES(r1,r2) \
      ( ((r1)->x1 <= (r2)->x1) && \
        ((r1)->x2 >= (r2)->x2) && \
        ((r1)->y1 <= (r2)->y1) && \
        ((r1)->y2 >= (r2)->y2) )

#define xfreeData(reg) if ((reg)->data && (reg)->data->size) free((reg)->data)

#define RECTALLOC_BAIL(pReg,n,bail) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!RegionRectAlloc(pReg, n)) { goto bail; }

#define RECTALLOC(pReg,n) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!RegionRectAlloc(pReg, n)) { return FALSE; }

#define ADDRECT(pNextRect,nx1,ny1,nx2,ny2)	\
{						\
    pNextRect->x1 = nx1;			\
    pNextRect->y1 = ny1;			\
    pNextRect->x2 = nx2;			\
    pNextRect->y2 = ny2;			\
    pNextRect++;				\
}

#define NEWRECT(pReg,pNextRect,nx1,ny1,nx2,ny2)			\
{									\
    if (!(pReg)->data || ((pReg)->data->numRects == (pReg)->data->size))\
    {									\
	if (!RegionRectAlloc(pReg, 1))					\
	    return FALSE;						\
	pNextRect = RegionTop(pReg);					\
    }									\
    ADDRECT(pNextRect,nx1,ny1,nx2,ny2);					\
    pReg->data->numRects++;						\
    assert(pReg->data->numRects<=pReg->data->size);			\
}

#define DOWNSIZE(reg,numRects)						 \
if (((numRects) < ((reg)->data->size >> 1)) && ((reg)->data->size > 50)) \
{									 \
    size_t NewSize = RegionSizeof(numRects);				 \
    RegDataPtr NewData =						 \
        (NewSize > 0) ? realloc((reg)->data, NewSize) : NULL ;		 \
    if (NewData)							 \
    {									 \
	NewData->size = (numRects);					 \
	(reg)->data = NewData;						 \
    }									 \
}

BoxRec RegionEmptyBox = { 0, 0, 0, 0 };
RegDataRec RegionEmptyData = { 0, 0 };

RegDataRec RegionBrokenData = { 0, 0 };
static RegionRec RegionBrokenRegion = { {0, 0, 0, 0}, &RegionBrokenData };

void
InitRegions(void)
{
    pixman_region_set_static_pointers(&RegionEmptyBox, &RegionEmptyData,
                                      &RegionBrokenData);
}

/*****************************************************************
 *   RegionCreate(rect, size)
 *     This routine does a simple malloc to make a structure of
 *     REGION of "size" number of rectangles.
 *****************************************************************/

RegionPtr
RegionCreate(BoxPtr rect, int size)
{
    RegionPtr pReg;

    pReg = (RegionPtr) malloc(sizeof(RegionRec));
    if (!pReg)
        return &RegionBrokenRegion;

    RegionInit(pReg, rect, size);

    return pReg;
}

void
RegionDestroy(RegionPtr pReg)
{
    pixman_region_fini(pReg);
    if (pReg != &RegionBrokenRegion)
        free(pReg);
}

RegionPtr
RegionDuplicate(RegionPtr pOld)
{
    RegionPtr   pNew;

    pNew = RegionCreate(&pOld->extents, 0);
    if (!pNew)
        return NULL;
    if (!RegionCopy(pNew, pOld)) {
        RegionDestroy(pNew);
        return NULL;
    }
    return pNew;
}

void
RegionPrint(RegionPtr rgn)
{
    int num, size;
    int i;
    BoxPtr rects;

    num = RegionNumRects(rgn);
    size = RegionSize(rgn);
    rects = RegionRects(rgn);
    ErrorF("[mi] num: %d size: %d\n", num, size);
    ErrorF("[mi] extents: %d %d %d %d\n",
           rgn->extents.x1, rgn->extents.y1, rgn->extents.x2, rgn->extents.y2);
    for (i = 0; i < num; i++)
        ErrorF("[mi] %d %d %d %d \n",
               rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    ErrorF("[mi] \n");
}

#ifdef DEBUG
Bool
RegionIsValid(RegionPtr reg)
{
    int i, numRects;

    if ((reg->extents.x1 > reg->extents.x2) ||
        (reg->extents.y1 > reg->extents.y2))
        return FALSE;
    numRects = RegionNumRects(reg);
    if (!numRects)
        return ((reg->extents.x1 == reg->extents.x2) &&
                (reg->extents.y1 == reg->extents.y2) &&
                (reg->data->size || (reg->data == &RegionEmptyData)));
    else if (numRects == 1)
        return !reg->data;
    else {
        BoxPtr pboxP, pboxN;
        BoxRec box;

        pboxP = RegionRects(reg);
        box = *pboxP;
        box.y2 = pboxP[numRects - 1].y2;
        pboxN = pboxP + 1;
        for (i = numRects; --i > 0; pboxP++, pboxN++) {
            if ((pboxN->x1 >= pboxN->x2) || (pboxN->y1 >= pboxN->y2))
                return FALSE;
            if (pboxN->x1 < box.x1)
                box.x1 = pboxN->x1;
            if (pboxN->x2 > box.x2)
                box.x2 = pboxN->x2;
            if ((pboxN->y1 < pboxP->y1) ||
                ((pboxN->y1 == pboxP->y1) &&
                 ((pboxN->x1 < pboxP->x2) || (pboxN->y2 != pboxP->y2))))
                return FALSE;
        }
        return ((box.x1 == reg->extents.x1) &&
                (box.x2 == reg->extents.x2) &&
                (box.y1 == reg->extents.y1) && (box.y2 == reg->extents.y2));
    }
}
#endif                          /* DEBUG */

Bool
RegionBreak(RegionPtr pReg)
{
    xfreeData(pReg);
    pReg->extents = RegionEmptyBox;
    pReg->data = &RegionBrokenData;
    return FALSE;
}

Bool
RegionRectAlloc(RegionPtr pRgn, int n)
{
    RegDataPtr data;
    size_t rgnSize;

    if (!pRgn->data) {
        n++;
        rgnSize = RegionSizeof(n);
        pRgn->data = (rgnSize > 0) ? malloc(rgnSize) : NULL;
        if (!pRgn->data)
            return RegionBreak(pRgn);
        pRgn->data->numRects = 1;
        *RegionBoxptr(pRgn) = pRgn->extents;
    }
    else if (!pRgn->data->size) {
        rgnSize = RegionSizeof(n);
        pRgn->data = (rgnSize > 0) ? malloc(rgnSize) : NULL;
        if (!pRgn->data)
            return RegionBreak(pRgn);
        pRgn->data->numRects = 0;
    }
    else {
        if (n == 1) {
            n = pRgn->data->numRects;
            if (n > 500)        /* XXX pick numbers out of a hat */
                n = 250;
        }
        n += pRgn->data->numRects;
        rgnSize = RegionSizeof(n);
        data = (rgnSize > 0) ? realloc(pRgn->data, rgnSize) : NULL;
        if (!data)
            return RegionBreak(pRgn);
        pRgn->data = data;
    }
    pRgn->data->size = n;
    return TRUE;
}

/*======================================================================
 *	    Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * RegionCoalesce --
 *	Attempt to merge the boxes in the current band with those in the
 *	previous one.  We are guaranteed that the current band extends to
 *      the end of the rects array.  Used only by RegionOp.
 *
 * Results:
 *	The new index for the previous band.
 *
 * Side Effects:
 *	If coalescing takes place:
 *	    - rectangles in the previous band will have their y2 fields
 *	      altered.
 *	    - pReg->data->numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
_X_INLINE static int
RegionCoalesce(RegionPtr pReg,  /* Region to coalesce                */
               int prevStart,   /* Index of start of previous band   */
               int curStart)
{                               /* Index of start of current band    */
    BoxPtr pPrevBox;            /* Current box in previous band      */
    BoxPtr pCurBox;             /* Current box in current band       */
    int numRects;               /* Number rectangles in both bands   */
    int y2;                     /* Bottom of current band            */

    /*
     * Figure out how many rectangles are in the band.
     */
    numRects = curStart - prevStart;
    assert(numRects == pReg->data->numRects - curStart);

    if (!numRects)
        return curStart;

    /*
     * The bands may only be coalesced if the bottom of the previous
     * matches the top scanline of the current.
     */
    pPrevBox = RegionBox(pReg, prevStart);
    pCurBox = RegionBox(pReg, curStart);
    if (pPrevBox->y2 != pCurBox->y1)
        return curStart;

    /*
     * Make sure the bands have boxes in the same places. This
     * assumes that boxes have been added in such a way that they
     * cover the most area possible. I.e. two boxes in a band must
     * have some horizontal space between them.
     */
    y2 = pCurBox->y2;

    do {
        if ((pPrevBox->x1 != pCurBox->x1) || (pPrevBox->x2 != pCurBox->x2)) {
            return curStart;
        }
        pPrevBox++;
        pCurBox++;
        numRects--;
    } while (numRects);

    /*
     * The bands may be merged, so set the bottom y of each box
     * in the previous band to the bottom y of the current band.
     */
    numRects = curStart - prevStart;
    pReg->data->numRects -= numRects;
    do {
        pPrevBox--;
        pPrevBox->y2 = y2;
        numRects--;
    } while (numRects);
    return prevStart;
}

/* Quicky macro to avoid trivial reject procedure calls to RegionCoalesce */

#define Coalesce(newReg, prevBand, curBand)				\
    if (curBand - prevBand == newReg->data->numRects - curBand) {	\
	prevBand = RegionCoalesce(newReg, prevBand, curBand);		\
    } else {								\
	prevBand = curBand;						\
    }

/*-
 *-----------------------------------------------------------------------
 * RegionAppendNonO --
 *	Handle a non-overlapping band for the union and subtract operations.
 *      Just adds the (top/bottom-clipped) rectangles into the region.
 *      Doesn't have to check for subsumption or anything.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg->data->numRects is incremented and the rectangles overwritten
 *	with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */

_X_INLINE static Bool
RegionAppendNonO(RegionPtr pReg, BoxPtr r, BoxPtr rEnd, int y1, int y2)
{
    BoxPtr pNextRect;
    int newRects;

    newRects = rEnd - r;

    assert(y1 < y2);
    assert(newRects != 0);

    /* Make sure we have enough space for all rectangles to be added */
    RECTALLOC(pReg, newRects);
    pNextRect = RegionTop(pReg);
    pReg->data->numRects += newRects;
    do {
        assert(r->x1 < r->x2);
        ADDRECT(pNextRect, r->x1, y1, r->x2, y2);
        r++;
    } while (r != rEnd);

    return TRUE;
}

#define FindBand(r, rBandEnd, rEnd, ry1)		    \
{							    \
    ry1 = r->y1;					    \
    rBandEnd = r+1;					    \
    while ((rBandEnd != rEnd) && (rBandEnd->y1 == ry1)) {   \
	rBandEnd++;					    \
    }							    \
}

#define	AppendRegions(newReg, r, rEnd)					\
{									\
    int newRects;							\
    if ((newRects = rEnd - r)) {					\
	RECTALLOC(newReg, newRects);					\
	memmove((char *)RegionTop(newReg),(char *)r, 			\
              newRects * sizeof(BoxRec));				\
	newReg->data->numRects += newRects;				\
    }									\
}

/*-
 *-----------------------------------------------------------------------
 * RegionOp --
 *	Apply an operation to two regions. Called by RegionUnion, RegionInverse,
 *	RegionSubtract, RegionIntersect....  Both regions MUST have at least one
 *      rectangle, and cannot be the same object.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	The new region is overwritten.
 *	pOverlap set to TRUE if overlapFunc ever returns TRUE.
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

typedef Bool (*OverlapProcPtr) (RegionPtr pReg,
                                BoxPtr r1,
                                BoxPtr r1End,
                                BoxPtr r2,
                                BoxPtr r2End,
                                short y1, short y2, Bool *pOverlap);

static Bool
RegionOp(RegionPtr newReg,      /* Place to store result         */
         RegionPtr reg1,        /* First region in operation     */
         RegionPtr reg2,        /* 2d region in operation        */
         OverlapProcPtr overlapFunc,    /* Function to call for over-
                                         * lapping bands                 */
         Bool appendNon1,       /* Append non-overlapping bands  */
         /* in region 1 ? */
         Bool appendNon2,       /* Append non-overlapping bands  */
         /* in region 2 ? */
         Bool *pOverlap)
{
    BoxPtr r1;                  /* Pointer into first region     */
    BoxPtr r2;                  /* Pointer into 2d region        */
    BoxPtr r1End;               /* End of 1st region             */
    BoxPtr r2End;               /* End of 2d region              */
    short ybot;                 /* Bottom of intersection        */
    short ytop;                 /* Top of intersection           */
    RegDataPtr oldData;         /* Old data for newReg           */
    int prevBand;               /* Index of start of
                                 * previous band in newReg       */
    int curBand;                /* Index of start of current
                                 * band in newReg                */
    BoxPtr r1BandEnd;           /* End of current band in r1     */
    BoxPtr r2BandEnd;           /* End of current band in r2     */
    short top;                  /* Top of non-overlapping band   */
    short bot;                  /* Bottom of non-overlapping band */
    int r1y1;                   /* Temps for r1->y1 and r2->y1   */
    int r2y1;
    int newSize;
    int numRects;

    /*
     * Break any region computed from a broken region
     */
    if (RegionNar(reg1) || RegionNar(reg2))
        return RegionBreak(newReg);

    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, save the rectangles
     * of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */

    r1 = RegionRects(reg1);
    newSize = RegionNumRects(reg1);
    r1End = r1 + newSize;
    numRects = RegionNumRects(reg2);
    r2 = RegionRects(reg2);
    r2End = r2 + numRects;
    assert(r1 != r1End);
    assert(r2 != r2End);

    oldData = NULL;
    if (((newReg == reg1) && (newSize > 1)) ||
        ((newReg == reg2) && (numRects > 1))) {
        oldData = newReg->data;
        newReg->data = &RegionEmptyData;
    }
    /* guess at new size */
    if (numRects > newSize)
        newSize = numRects;
    newSize <<= 1;
    if (!newReg->data)
        newReg->data = &RegionEmptyData;
    else if (newReg->data->size)
        newReg->data->numRects = 0;
    if (newSize > newReg->data->size)
        if (!RegionRectAlloc(newReg, newSize))
            return FALSE;

    /*
     * Initialize ybot.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */

    ybot = min(r1->y1, r2->y1);

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. RegionCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do {
        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
        assert(r1 != r1End);
        assert(r2 != r2End);

        FindBand(r1, r1BandEnd, r1End, r1y1);
        FindBand(r2, r2BandEnd, r2End, r2y1);

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1y1 < r2y1) {
            if (appendNon1) {
                top = max(r1y1, ybot);
                bot = min(r1->y2, r2y1);
                if (top != bot) {
                    curBand = newReg->data->numRects;
                    RegionAppendNonO(newReg, r1, r1BandEnd, top, bot);
                    Coalesce(newReg, prevBand, curBand);
                }
            }
            ytop = r2y1;
        }
        else if (r2y1 < r1y1) {
            if (appendNon2) {
                top = max(r2y1, ybot);
                bot = min(r2->y2, r1y1);
                if (top != bot) {
                    curBand = newReg->data->numRects;
                    RegionAppendNonO(newReg, r2, r2BandEnd, top, bot);
                    Coalesce(newReg, prevBand, curBand);
                }
            }
            ytop = r1y1;
        }
        else {
            ytop = r1y1;
        }

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot > ytop
         */
        ybot = min(r1->y2, r2->y2);
        if (ybot > ytop) {
            curBand = newReg->data->numRects;
            (*overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot,
                            pOverlap);
            Coalesce(newReg, prevBand, curBand);
        }

        /*
         * If we've finished with a band (y2 == ybot) we skip forward
         * in the region to the next band.
         */
        if (r1->y2 == ybot)
            r1 = r1BandEnd;
        if (r2->y2 == ybot)
            r2 = r2BandEnd;

    } while (r1 != r1End && r2 != r2End);

    /*
     * Deal with whichever region (if any) still has rectangles left.
     *
     * We only need to worry about banding and coalescing for the very first
     * band left.  After that, we can just group all remaining boxes,
     * regardless of how many bands, into one final append to the list.
     */

    if ((r1 != r1End) && appendNon1) {
        /* Do first nonOverlap1Func call, which may be able to coalesce */
        FindBand(r1, r1BandEnd, r1End, r1y1);
        curBand = newReg->data->numRects;
        RegionAppendNonO(newReg, r1, r1BandEnd, max(r1y1, ybot), r1->y2);
        Coalesce(newReg, prevBand, curBand);
        /* Just append the rest of the boxes  */
        AppendRegions(newReg, r1BandEnd, r1End);

    }
    else if ((r2 != r2End) && appendNon2) {
        /* Do first nonOverlap2Func call, which may be able to coalesce */
        FindBand(r2, r2BandEnd, r2End, r2y1);
        curBand = newReg->data->numRects;
        RegionAppendNonO(newReg, r2, r2BandEnd, max(r2y1, ybot), r2->y2);
        Coalesce(newReg, prevBand, curBand);
        /* Append rest of boxes */
        AppendRegions(newReg, r2BandEnd, r2End);
    }

    free(oldData);

    if (!(numRects = newReg->data->numRects)) {
        xfreeData(newReg);
        newReg->data = &RegionEmptyData;
    }
    else if (numRects == 1) {
        newReg->extents = *RegionBoxptr(newReg);
        xfreeData(newReg);
        newReg->data = NULL;
    }
    else {
        DOWNSIZE(newReg, numRects);
    }

    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * RegionSetExtents --
 *	Reset the extents of a region to what they should be. Called by
 *	Subtract and Intersect as they can't figure it out along the
 *	way or do so easily, as Union can.
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
RegionSetExtents(RegionPtr pReg)
{
    BoxPtr pBox, pBoxEnd;

    if (!pReg->data)
        return;
    if (!pReg->data->size) {
        pReg->extents.x2 = pReg->extents.x1;
        pReg->extents.y2 = pReg->extents.y1;
        return;
    }

    pBox = RegionBoxptr(pReg);
    pBoxEnd = RegionEnd(pReg);

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest y1 and since pBoxEnd is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pReg->extents.x1 = pBox->x1;
    pReg->extents.y1 = pBox->y1;
    pReg->extents.x2 = pBoxEnd->x2;
    pReg->extents.y2 = pBoxEnd->y2;

    assert(pReg->extents.y1 < pReg->extents.y2);
    while (pBox <= pBoxEnd) {
        if (pBox->x1 < pReg->extents.x1)
            pReg->extents.x1 = pBox->x1;
        if (pBox->x2 > pReg->extents.x2)
            pReg->extents.x2 = pBox->x2;
        pBox++;
    };

    assert(pReg->extents.x1 < pReg->extents.x2);
}

/*======================================================================
 *	    Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * RegionIntersectO --
 *	Handle an overlapping band for RegionIntersect.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
 /*ARGSUSED*/
#define MERGERECT(r)						\
{								\
    if (r->x1 <= x2) {						\
	/* Merge with current rectangle */			\
	if (r->x1 < x2) *pOverlap = TRUE;				\
	if (x2 < r->x2) x2 = r->x2;				\
    } else {							\
	/* Add current rectangle, start new one */		\
	NEWRECT(pReg, pNextRect, x1, y1, x2, y2);		\
	x1 = r->x1;						\
	x2 = r->x2;						\
    }								\
    r++;							\
}
/*======================================================================
 *	    Region Union
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * RegionUnionO --
 *	Handle an overlapping band for the union operation. Picks the
 *	left-most rectangle each time and merges it into the region.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	pReg is overwritten.
 *	pOverlap is set to TRUE if any boxes overlap.
 *
 *-----------------------------------------------------------------------
 */
    static Bool
RegionUnionO(RegionPtr pReg,
             BoxPtr r1,
             BoxPtr r1End,
             BoxPtr r2, BoxPtr r2End, short y1, short y2, Bool *pOverlap)
{
    BoxPtr pNextRect;
    int x1;                     /* left and right side of current union */
    int x2;

    assert(y1 < y2);
    assert(r1 != r1End);
    assert(r2 != r2End);

    pNextRect = RegionTop(pReg);

    /* Start off current rectangle */
    if (r1->x1 < r2->x1) {
        x1 = r1->x1;
        x2 = r1->x2;
        r1++;
    }
    else {
        x1 = r2->x1;
        x2 = r2->x2;
        r2++;
    }
    while (r1 != r1End && r2 != r2End) {
        if (r1->x1 < r2->x1)
            MERGERECT(r1)
            else
            MERGERECT(r2);
    }

    /* Finish off whoever (if any) is left */
    if (r1 != r1End) {
        do {
            MERGERECT(r1);
        } while (r1 != r1End);
    }
    else if (r2 != r2End) {
        do {
            MERGERECT(r2);
        } while (r2 != r2End);
    }

    /* Add current rectangle */
    NEWRECT(pReg, pNextRect, x1, y1, x2, y2);

    return TRUE;
}

/*======================================================================
 *	    Batch Rectangle Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * RegionAppend --
 *
 *      "Append" the rgn rectangles onto the end of dstrgn, maintaining
 *      knowledge of YX-banding when it's easy.  Otherwise, dstrgn just
 *      becomes a non-y-x-banded random collection of rectangles, and not
 *      yet a true region.  After a sequence of appends, the caller must
 *      call RegionValidate to ensure that a valid region is constructed.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *      dstrgn is modified if rgn has rectangles.
 *
 */
Bool
RegionAppend(RegionPtr dstrgn, RegionPtr rgn)
{
    int numRects, dnumRects, size;
    BoxPtr new, old;
    Bool prepend;

    if (RegionNar(rgn))
        return RegionBreak(dstrgn);

    if (!rgn->data && (dstrgn->data == &RegionEmptyData)) {
        dstrgn->extents = rgn->extents;
        dstrgn->data = NULL;
        return TRUE;
    }

    numRects = RegionNumRects(rgn);
    if (!numRects)
        return TRUE;
    prepend = FALSE;
    size = numRects;
    dnumRects = RegionNumRects(dstrgn);
    if (!dnumRects && (size < 200))
        size = 200;             /* XXX pick numbers out of a hat */
    RECTALLOC(dstrgn, size);
    old = RegionRects(rgn);
    if (!dnumRects)
        dstrgn->extents = rgn->extents;
    else if (dstrgn->extents.x2 > dstrgn->extents.x1) {
        BoxPtr first, last;

        first = old;
        last = RegionBoxptr(dstrgn) + (dnumRects - 1);
        if ((first->y1 > last->y2) ||
            ((first->y1 == last->y1) && (first->y2 == last->y2) &&
             (first->x1 > last->x2))) {
            if (rgn->extents.x1 < dstrgn->extents.x1)
                dstrgn->extents.x1 = rgn->extents.x1;
            if (rgn->extents.x2 > dstrgn->extents.x2)
                dstrgn->extents.x2 = rgn->extents.x2;
            dstrgn->extents.y2 = rgn->extents.y2;
        }
        else {
            first = RegionBoxptr(dstrgn);
            last = old + (numRects - 1);
            if ((first->y1 > last->y2) ||
                ((first->y1 == last->y1) && (first->y2 == last->y2) &&
                 (first->x1 > last->x2))) {
                prepend = TRUE;
                if (rgn->extents.x1 < dstrgn->extents.x1)
                    dstrgn->extents.x1 = rgn->extents.x1;
                if (rgn->extents.x2 > dstrgn->extents.x2)
                    dstrgn->extents.x2 = rgn->extents.x2;
                dstrgn->extents.y1 = rgn->extents.y1;
            }
            else
                dstrgn->extents.x2 = dstrgn->extents.x1;
        }
    }
    if (prepend) {
        new = RegionBox(dstrgn, numRects);
        if (dnumRects == 1)
            *new = *RegionBoxptr(dstrgn);
        else
            memmove((char *) new, (char *) RegionBoxptr(dstrgn),
                    dnumRects * sizeof(BoxRec));
        new = RegionBoxptr(dstrgn);
    }
    else
        new = RegionBoxptr(dstrgn) + dnumRects;
    if (numRects == 1)
        *new = *old;
    else
        memmove((char *) new, (char *) old, numRects * sizeof(BoxRec));
    dstrgn->data->numRects += numRects;
    return TRUE;
}

#define ExchangeRects(a, b) \
{			    \
    BoxRec     t;	    \
    t = rects[a];	    \
    rects[a] = rects[b];    \
    rects[b] = t;	    \
}

static void
QuickSortRects(BoxRec rects[], int numRects)
{
    int y1;
    int x1;
    int i, j;
    BoxPtr r;

    /* Always called with numRects > 1 */

    do {
        if (numRects == 2) {
            if (rects[0].y1 > rects[1].y1 ||
                (rects[0].y1 == rects[1].y1 && rects[0].x1 > rects[1].x1))
                ExchangeRects(0, 1);
            return;
        }

        /* Choose partition element, stick in location 0 */
        ExchangeRects(0, numRects >> 1);
        y1 = rects[0].y1;
        x1 = rects[0].x1;

        /* Partition array */
        i = 0;
        j = numRects;
        do {
            r = &(rects[i]);
            do {
                r++;
                i++;
            } while (i != numRects &&
                     (r->y1 < y1 || (r->y1 == y1 && r->x1 < x1)));
            r = &(rects[j]);
            do {
                r--;
                j--;
            } while (y1 < r->y1 || (y1 == r->y1 && x1 < r->x1));
            if (i < j)
                ExchangeRects(i, j);
        } while (i < j);

        /* Move partition element back to middle */
        ExchangeRects(0, j);

        /* Recurse */
        if (numRects - j - 1 > 1)
            QuickSortRects(&rects[j + 1], numRects - j - 1);
        numRects = j;
    } while (numRects > 1);
}

/*-
 *-----------------------------------------------------------------------
 * RegionValidate --
 *
 *      Take a ``region'' which is a non-y-x-banded random collection of
 *      rectangles, and compute a nice region which is the union of all the
 *      rectangles.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *      The passed-in ``region'' may be modified.
 *	pOverlap set to TRUE if any rectangles overlapped, else FALSE;
 *
 * Strategy:
 *      Step 1. Sort the rectangles into ascending order with primary key y1
 *		and secondary key x1.
 *
 *      Step 2. Split the rectangles into the minimum number of proper y-x
 *		banded regions.  This may require horizontally merging
 *		rectangles, and vertically coalescing bands.  With any luck,
 *		this step in an identity transformation (ala the Box widget),
 *		or a coalescing into 1 box (ala Menus).
 *
 *	Step 3. Merge the separate regions down to a single region by calling
 *		Union.  Maximize the work each Union call does by using
 *		a binary merge.
 *
 *-----------------------------------------------------------------------
 */

Bool
RegionValidate(RegionPtr badreg, Bool *pOverlap)
{
    /* Descriptor for regions under construction  in Step 2. */
    typedef struct {
        RegionRec reg;
        int prevBand;
        int curBand;
    } RegionInfo;

    int numRects;               /* Original numRects for badreg         */
    RegionInfo *ri;             /* Array of current regions             */
    int numRI;                  /* Number of entries used in ri         */
    int sizeRI;                 /* Number of entries available in ri    */
    int i;                      /* Index into rects                     */
    int j;                      /* Index into ri                        */
    RegionInfo *rit;            /* &ri[j]                                */
    RegionPtr reg;              /* ri[j].reg                     */
    BoxPtr box;                 /* Current box in rects                 */
    BoxPtr riBox;               /* Last box in ri[j].reg                */
    RegionPtr hreg;             /* ri[j_half].reg                        */
    Bool ret = TRUE;

    *pOverlap = FALSE;
    if (!badreg->data) {
        good(badreg);
        return TRUE;
    }
    numRects = badreg->data->numRects;
    if (!numRects) {
        if (RegionNar(badreg))
            return FALSE;
        good(badreg);
        return TRUE;
    }
    if (badreg->extents.x1 < badreg->extents.x2) {
        if ((numRects) == 1) {
            xfreeData(badreg);
            badreg->data = (RegDataPtr) NULL;
        }
        else {
            DOWNSIZE(badreg, numRects);
        }
        good(badreg);
        return TRUE;
    }

    /* Step 1: Sort the rects array into ascending (y1, x1) order */
    QuickSortRects(RegionBoxptr(badreg), numRects);

    /* Step 2: Scatter the sorted array into the minimum number of regions */

    /* Set up the first region to be the first rectangle in badreg */
    /* Note that step 2 code will never overflow the ri[0].reg rects array */
    ri = (RegionInfo *) malloc(4 * sizeof(RegionInfo));
    if (!ri)
        return RegionBreak(badreg);
    sizeRI = 4;
    numRI = 1;
    ri[0].prevBand = 0;
    ri[0].curBand = 0;
    ri[0].reg = *badreg;
    box = RegionBoxptr(&ri[0].reg);
    ri[0].reg.extents = *box;
    ri[0].reg.data->numRects = 1;

    /* Now scatter rectangles into the minimum set of valid regions.  If the
       next rectangle to be added to a region would force an existing rectangle
       in the region to be split up in order to maintain y-x banding, just
       forget it.  Try the next region.  If it doesn't fit cleanly into any
       region, make a new one. */

    for (i = numRects; --i > 0;) {
        box++;
        /* Look for a region to append box to */
        for (j = numRI, rit = ri; --j >= 0; rit++) {
            reg = &rit->reg;
            riBox = RegionEnd(reg);

            if (box->y1 == riBox->y1 && box->y2 == riBox->y2) {
                /* box is in same band as riBox.  Merge or append it */
                if (box->x1 <= riBox->x2) {
                    /* Merge it with riBox */
                    if (box->x1 < riBox->x2)
                        *pOverlap = TRUE;
                    if (box->x2 > riBox->x2)
                        riBox->x2 = box->x2;
                }
                else {
                    RECTALLOC_BAIL(reg, 1, bail);
                    *RegionTop(reg) = *box;
                    reg->data->numRects++;
                }
                goto NextRect;  /* So sue me */
            }
            else if (box->y1 >= riBox->y2) {
                /* Put box into new band */
                if (reg->extents.x2 < riBox->x2)
                    reg->extents.x2 = riBox->x2;
                if (reg->extents.x1 > box->x1)
                    reg->extents.x1 = box->x1;
                Coalesce(reg, rit->prevBand, rit->curBand);
                rit->curBand = reg->data->numRects;
                RECTALLOC_BAIL(reg, 1, bail);
                *RegionTop(reg) = *box;
                reg->data->numRects++;
                goto NextRect;
            }
            /* Well, this region was inappropriate.  Try the next one. */
        }                       /* for j */

        /* Uh-oh.  No regions were appropriate.  Create a new one. */
        if (sizeRI == numRI) {
            /* Oops, allocate space for new region information */
            sizeRI <<= 1;
            rit = (RegionInfo *) reallocarray(ri, sizeRI, sizeof(RegionInfo));
            if (!rit)
                goto bail;
            ri = rit;
            rit = &ri[numRI];
        }
        numRI++;
        rit->prevBand = 0;
        rit->curBand = 0;
        rit->reg.extents = *box;
        rit->reg.data = NULL;
        if (!RegionRectAlloc(&rit->reg, (i + numRI) / numRI))   /* MUST force allocation */
            goto bail;
 NextRect:;
    }                           /* for i */

    /* Make a final pass over each region in order to Coalesce and set
       extents.x2 and extents.y2 */

    for (j = numRI, rit = ri; --j >= 0; rit++) {
        reg = &rit->reg;
        riBox = RegionEnd(reg);
        reg->extents.y2 = riBox->y2;
        if (reg->extents.x2 < riBox->x2)
            reg->extents.x2 = riBox->x2;
        Coalesce(reg, rit->prevBand, rit->curBand);
        if (reg->data->numRects == 1) { /* keep unions happy below */
            xfreeData(reg);
            reg->data = NULL;
        }
    }

    /* Step 3: Union all regions into a single region */
    while (numRI > 1) {
        int half = numRI / 2;

        for (j = numRI & 1; j < (half + (numRI & 1)); j++) {
            reg = &ri[j].reg;
            hreg = &ri[j + half].reg;
            if (!RegionOp(reg, reg, hreg, RegionUnionO, TRUE, TRUE, pOverlap))
                ret = FALSE;
            if (hreg->extents.x1 < reg->extents.x1)
                reg->extents.x1 = hreg->extents.x1;
            if (hreg->extents.y1 < reg->extents.y1)
                reg->extents.y1 = hreg->extents.y1;
            if (hreg->extents.x2 > reg->extents.x2)
                reg->extents.x2 = hreg->extents.x2;
            if (hreg->extents.y2 > reg->extents.y2)
                reg->extents.y2 = hreg->extents.y2;
            xfreeData(hreg);
        }
        numRI -= half;
    }
    *badreg = ri[0].reg;
    free(ri);
    good(badreg);
    return ret;
 bail:
    for (i = 0; i < numRI; i++)
        xfreeData(&ri[i].reg);
    free(ri);
    return RegionBreak(badreg);
}

RegionPtr
RegionFromRects(int nrects, xRectangle *prect, int ctype)
{

    RegionPtr pRgn;
    size_t rgnSize;
    RegDataPtr pData;
    BoxPtr pBox;
    int i;
    int x1, y1, x2, y2;

    pRgn = RegionCreate(NullBox, 0);
    if (RegionNar(pRgn))
        return pRgn;
    if (!nrects)
        return pRgn;
    if (nrects == 1) {
        x1 = prect->x;
        y1 = prect->y;
        if ((x2 = x1 + (int) prect->width) > MAXSHORT)
            x2 = MAXSHORT;
        if ((y2 = y1 + (int) prect->height) > MAXSHORT)
            y2 = MAXSHORT;
        if (x1 != x2 && y1 != y2) {
            pRgn->extents.x1 = x1;
            pRgn->extents.y1 = y1;
            pRgn->extents.x2 = x2;
            pRgn->extents.y2 = y2;
            pRgn->data = NULL;
        }
        return pRgn;
    }
    rgnSize = RegionSizeof(nrects);
    pData = (rgnSize > 0) ? malloc(rgnSize) : NULL;
    if (!pData) {
        RegionBreak(pRgn);
        return pRgn;
    }
    pBox = (BoxPtr) (pData + 1);
    for (i = nrects; --i >= 0; prect++) {
        x1 = prect->x;
        y1 = prect->y;
        if ((x2 = x1 + (int) prect->width) > MAXSHORT)
            x2 = MAXSHORT;
        if ((y2 = y1 + (int) prect->height) > MAXSHORT)
            y2 = MAXSHORT;
        if (x1 != x2 && y1 != y2) {
            pBox->x1 = x1;
            pBox->y1 = y1;
            pBox->x2 = x2;
            pBox->y2 = y2;
            pBox++;
        }
    }
    if (pBox != (BoxPtr) (pData + 1)) {
        pData->size = nrects;
        pData->numRects = pBox - (BoxPtr) (pData + 1);
        pRgn->data = pData;
        if (ctype != CT_YXBANDED) {
            Bool overlap;       /* result ignored */

            pRgn->extents.x1 = pRgn->extents.x2 = 0;
            RegionValidate(pRgn, &overlap);
        }
        else
            RegionSetExtents(pRgn);
        good(pRgn);
    }
    else {
        free(pData);
    }
    return pRgn;
}
