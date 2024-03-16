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
/*
 *  mipoly.c
 *
 *  Written by Brian Kelleher; June 1986
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "windowstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "mi.h"
#include "miscanfill.h"
#include "mipoly.h"
#include "regionstr.h"

/*
 * Insert the given edge into the edge table.  First we must find the correct
 * bucket in the Edge table, then find the right slot in the bucket.  Finally,
 * we can insert it.
 */
static Bool
miInsertEdgeInET(EdgeTable * ET, EdgeTableEntry * ETE, int scanline,
                 ScanLineListBlock ** SLLBlock, int *iSLLBlock)
{
    EdgeTableEntry *start, *prev;
    ScanLineList *pSLL, *pPrevSLL;
    ScanLineListBlock *tmpSLLBlock;

    /*
     * find the right bucket to put the edge into
     */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline)) {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /*
     * reassign pSLL (pointer to ScanLineList) if necessary
     */
    if ((!pSLL) || (pSLL->scanline > scanline)) {
        if (*iSLLBlock > SLLSPERBLOCK - 1) {
            tmpSLLBlock = malloc(sizeof(ScanLineListBlock));
            if (!tmpSLLBlock)
                return FALSE;
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = NULL;
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    prev = NULL;
    start = pSLL->edgelist;
    while (start && (start->bres.minor < ETE->bres.minor)) {
        prev = start;
        start = start->next;
    }
    ETE->next = start;

    if (prev)
        prev->next = ETE;
    else
        pSLL->edgelist = ETE;
    return TRUE;
}

static void
miFreeStorage(ScanLineListBlock * pSLLBlock)
{
    ScanLineListBlock *tmpSLLBlock;

    while (pSLLBlock) {
        tmpSLLBlock = pSLLBlock->next;
        free(pSLLBlock);
        pSLLBlock = tmpSLLBlock;
    }
}

/*
 * CreateEdgeTable
 *
 * This routine creates the edge table for scan converting polygons.
 * The Edge Table (ET) looks like:
 *
 * EdgeTable
 *  --------
 * |  ymax  |        ScanLineLists
 * |scanline|-->------------>-------------->...
 *  --------   |scanline|   |scanline|
 *             |edgelist|   |edgelist|
 *             ---------    ---------
 *                 |             |
 *                 |             |
 *                 V             V
 *           list of ETEs   list of ETEs
 *
 * where ETE is an EdgeTableEntry data structure, and there is one ScanLineList
 * per scanline at which an edge is initially entered.
 */

static Bool
miCreateETandAET(int count, DDXPointPtr pts, EdgeTable * ET,
                 EdgeTableEntry * AET, EdgeTableEntry * pETEs,
                 ScanLineListBlock * pSLLBlock)
{
    DDXPointPtr top, bottom;
    DDXPointPtr PrevPt, CurrPt;
    int iSLLBlock = 0;

    int dy;

    if (count < 2)
        return TRUE;

    /*
     *  initialize the Active Edge Table
     */
    AET->next = NULL;
    AET->back = NULL;
    AET->nextWETE = NULL;
    AET->bres.minor = MININT;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = NULL;
    ET->ymax = MININT;
    ET->ymin = MAXINT;
    pSLLBlock->next = NULL;

    PrevPt = &pts[count - 1];

    /*
     *  for each vertex in the array of points.
     *  In this loop we are dealing with two vertices at
     *  a time -- these make up one edge of the polygon.
     */
    while (count--) {
        CurrPt = pts++;

        /*
         *  find out which point is above and which is below.
         */
        if (PrevPt->y > CurrPt->y) {
            bottom = PrevPt, top = CurrPt;
            pETEs->ClockWise = 0;
        }
        else {
            bottom = CurrPt, top = PrevPt;
            pETEs->ClockWise = 1;
        }

        /*
         * don't add horizontal edges to the Edge table.
         */
        if (bottom->y != top->y) {
            pETEs->ymax = bottom->y - 1; /* -1 so we don't get last scanline */

            /*
             *  initialize integer edge algorithm
             */
            dy = bottom->y - top->y;
            BRESINITPGONSTRUCT(dy, top->x, bottom->x, pETEs->bres);

            if (!miInsertEdgeInET(ET, pETEs, top->y, &pSLLBlock, &iSLLBlock)) {
                miFreeStorage(pSLLBlock->next);
                return FALSE;
            }

            ET->ymax = max(ET->ymax, PrevPt->y);
            ET->ymin = min(ET->ymin, PrevPt->y);
            pETEs++;
        }

        PrevPt = CurrPt;
    }
    return TRUE;
}

/*
 * This routine moves EdgeTableEntries from the EdgeTable into the Active Edge
 * Table, leaving them sorted by smaller x coordinate.
 */

static void
miloadAET(EdgeTableEntry * AET, EdgeTableEntry * ETEs)
{
    EdgeTableEntry *pPrevAET;
    EdgeTableEntry *tmp;

    pPrevAET = AET;
    AET = AET->next;
    while (ETEs) {
        while (AET && (AET->bres.minor < ETEs->bres.minor)) {
            pPrevAET = AET;
            AET = AET->next;
        }
        tmp = ETEs->next;
        ETEs->next = AET;
        if (AET)
            AET->back = ETEs;
        ETEs->back = pPrevAET;
        pPrevAET->next = ETEs;
        pPrevAET = ETEs;

        ETEs = tmp;
    }
}

/*
 * computeWAET
 *
 * This routine links the AET by the nextWETE (winding EdgeTableEntry) link for
 * use by the winding number rule.  The final Active Edge Table (AET) might
 * look something like:
 *
 * AET
 * ----------  ---------   ---------
 * |ymax    |  |ymax    |  |ymax    |
 * | ...    |  |...     |  |...     |
 * |next    |->|next    |->|next    |->...
 * |nextWETE|  |nextWETE|  |nextWETE|
 * ---------   ---------   ^--------
 *     |                   |       |
 *     V------------------->       V---> ...
 *
 */
static void
micomputeWAET(EdgeTableEntry * AET)
{
    EdgeTableEntry *pWETE;
    int inside = 1;
    int isInside = 0;

    AET->nextWETE = NULL;
    pWETE = AET;
    AET = AET->next;
    while (AET) {
        if (AET->ClockWise)
            isInside++;
        else
            isInside--;

        if ((!inside && !isInside) || (inside && isInside)) {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }
    pWETE->nextWETE = NULL;
}

/*
 * Just a simple insertion sort using pointers and back pointers to sort the
 * Active Edge Table.
 */

static int
miInsertionSort(EdgeTableEntry * AET)
{
    EdgeTableEntry *pETEchase;
    EdgeTableEntry *pETEinsert;
    EdgeTableEntry *pETEchaseBackTMP;
    int changed = 0;

    AET = AET->next;
    while (AET) {
        pETEinsert = AET;
        pETEchase = AET;
        while (pETEchase->back->bres.minor > AET->bres.minor)
            pETEchase = pETEchase->back;

        AET = AET->next;
        if (pETEchase != pETEinsert) {
            pETEchaseBackTMP = pETEchase->back;
            pETEinsert->back->next = AET;
            if (AET)
                AET->back = pETEinsert->back;
            pETEinsert->next = pETEchase;
            pETEchase->back->next = pETEinsert;
            pETEchase->back = pETEinsert;
            pETEinsert->back = pETEchaseBackTMP;
            changed = 1;
        }
    }
    return changed;
}

/* Find the index of the point with the smallest y */
static int
getPolyYBounds(DDXPointPtr pts, int n, int *by, int *ty)
{
    DDXPointPtr ptMin;
    int ymin, ymax;
    DDXPointPtr ptsStart = pts;

    ptMin = pts;
    ymin = ymax = (pts++)->y;

    while (--n > 0) {
        if (pts->y < ymin) {
            ptMin = pts;
            ymin = pts->y;
        }
        if (pts->y > ymax)
            ymax = pts->y;

        pts++;
    }

    *by = ymin;
    *ty = ymax;
    return ptMin - ptsStart;
}

/*
 * Written by Brian Kelleher; Dec. 1985.
 *
 * Fill a convex polygon.  If the given polygon is not convex, then the result
 * is undefined.  The algorithm is to order the edges from smallest y to
 * largest by partitioning the array into a left edge list and a right edge
 * list.  The algorithm used to traverse each edge is an extension of
 * Bresenham's line algorithm with y as the major axis.  For a derivation of
 * the algorithm, see the author of this code.
 */
static Bool
miFillConvexPoly(DrawablePtr dst, GCPtr pgc, int count, DDXPointPtr ptsIn)
{
    int xl = 0, xr = 0;         /* x vals of left and right edges */
    int dl = 0, dr = 0;         /* decision variables             */
    int ml = 0, m1l = 0;        /* left edge slope and slope+1    */
    int mr = 0, m1r = 0;        /* right edge slope and slope+1   */
    int incr1l = 0, incr2l = 0; /* left edge error increments     */
    int incr1r = 0, incr2r = 0; /* right edge error increments    */
    int dy;                     /* delta y                        */
    int y;                      /* current scanline               */
    int left, right;            /* indices to first endpoints     */
    int i;                      /* loop counter                   */
    int nextleft, nextright;    /* indices to second endpoints    */
    DDXPointPtr ptsOut, FirstPoint;     /* output buffer               */
    int *width, *FirstWidth;    /* output buffer                  */
    int imin;                   /* index of smallest vertex (in y) */
    int ymin;                   /* y-extents of polygon            */
    int ymax;

    /*
     *  find leftx, bottomy, rightx, topy, and the index
     *  of bottomy. Also translate the points.
     */
    imin = getPolyYBounds(ptsIn, count, &ymin, &ymax);

    dy = ymax - ymin + 1;
    if ((count < 3) || (dy < 0))
        return TRUE;
    ptsOut = FirstPoint = xallocarray(dy, sizeof(DDXPointRec));
    width = FirstWidth = xallocarray(dy, sizeof(int));
    if (!FirstPoint || !FirstWidth) {
        free(FirstWidth);
        free(FirstPoint);
        return FALSE;
    }

    nextleft = nextright = imin;
    y = ptsIn[nextleft].y;

    /*
     *  loop through all edges of the polygon
     */
    do {
        /*
         *  add a left edge if we need to
         */
        if (ptsIn[nextleft].y == y) {
            left = nextleft;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextleft++;
            if (nextleft >= count)
                nextleft = 0;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(ptsIn[nextleft].y - ptsIn[left].y,
                         ptsIn[left].x, ptsIn[nextleft].x,
                         xl, dl, ml, m1l, incr1l, incr2l);
        }

        /*
         *  add a right edge if we need to
         */
        if (ptsIn[nextright].y == y) {
            right = nextright;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextright--;
            if (nextright < 0)
                nextright = count - 1;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(ptsIn[nextright].y - ptsIn[right].y,
                         ptsIn[right].x, ptsIn[nextright].x,
                         xr, dr, mr, m1r, incr1r, incr2r);
        }

        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = min(ptsIn[nextleft].y, ptsIn[nextright].y) - y;
        /* in case we're called with non-convex polygon */
        if (i < 0) {
            free(FirstWidth);
            free(FirstPoint);
            return TRUE;
        }
        while (i-- > 0) {
            ptsOut->y = y;

            /*
             *  reverse the edges if necessary
             */
            if (xl < xr) {
                *(width++) = xr - xl;
                (ptsOut++)->x = xl;
            }
            else {
                *(width++) = xl - xr;
                (ptsOut++)->x = xr;
            }
            y++;

            /* increment down the edges */
            BRESINCRPGON(dl, xl, ml, m1l, incr1l, incr2l);
            BRESINCRPGON(dr, xr, mr, m1r, incr1r, incr2r);
        }
    } while (y != ymax);

    /*
     * Finally, fill the <remaining> spans
     */
    (*pgc->ops->FillSpans) (dst, pgc,
                            ptsOut - FirstPoint, FirstPoint, FirstWidth, 1);
    free(FirstWidth);
    free(FirstPoint);
    return TRUE;
}

/*
 * Written by Brian Kelleher;  Oct. 1985
 *
 * Routine to fill a polygon.  Two fill rules are supported: frWINDING and
 * frEVENODD.
 */
static Bool
miFillGeneralPoly(DrawablePtr dst, GCPtr pgc, int count, DDXPointPtr ptsIn)
{
    EdgeTableEntry *pAET;       /* the Active Edge Table   */
    int y;                      /* the current scanline    */
    int nPts = 0;               /* number of pts in buffer */
    EdgeTableEntry *pWETE;      /* Winding Edge Table      */
    ScanLineList *pSLL;         /* Current ScanLineList    */
    DDXPointPtr ptsOut;         /* ptr to output buffers   */
    int *width;
    DDXPointRec FirstPoint[NUMPTSTOBUFFER];     /* the output buffers */
    int FirstWidth[NUMPTSTOBUFFER];
    EdgeTableEntry *pPrevAET;   /* previous AET entry      */
    EdgeTable ET;               /* Edge Table header node  */
    EdgeTableEntry AET;         /* Active ET header node   */
    EdgeTableEntry *pETEs;      /* Edge Table Entries buff */
    ScanLineListBlock SLLBlock; /* header for ScanLineList */
    int fixWAET = 0;

    if (count < 3)
        return TRUE;

    if (!(pETEs = malloc(sizeof(EdgeTableEntry) * count)))
        return FALSE;
    ptsOut = FirstPoint;
    width = FirstWidth;
    if (!miCreateETandAET(count, ptsIn, &ET, &AET, pETEs, &SLLBlock)) {
        free(pETEs);
        return FALSE;
    }
    pSLL = ET.scanlines.next;

    if (pgc->fillRule == EvenOddRule) {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL && y == pSLL->scanline) {
                miloadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;

            /*
             *  for each active edge
             */
            while (pAET) {
                ptsOut->x = pAET->bres.minor;
                ptsOut++->y = y;
                *width++ = pAET->next->bres.minor - pAET->bres.minor;
                nPts++;

                /*
                 *  send out the buffer when its full
                 */
                if (nPts == NUMPTSTOBUFFER) {
                    (*pgc->ops->FillSpans) (dst, pgc,
                                            nPts, FirstPoint, FirstWidth, 1);
                    ptsOut = FirstPoint;
                    width = FirstWidth;
                    nPts = 0;
                }
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
            }
            miInsertionSort(&AET);
        }
    }
    else {                      /* default to WindingNumber */

        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL && y == pSLL->scanline) {
                miloadAET(&AET, pSLL->edgelist);
                micomputeWAET(&AET);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;

            /*
             *  for each active edge
             */
            while (pAET) {
                /*
                 *  if the next edge in the active edge table is
                 *  also the next edge in the winding active edge
                 *  table.
                 */
                if (pWETE == pAET) {
                    ptsOut->x = pAET->bres.minor;
                    ptsOut++->y = y;
                    *width++ = pAET->nextWETE->bres.minor - pAET->bres.minor;
                    nPts++;

                    /*
                     *  send out the buffer
                     */
                    if (nPts == NUMPTSTOBUFFER) {
                        (*pgc->ops->FillSpans) (dst, pgc, nPts, FirstPoint,
                                                FirstWidth, 1);
                        ptsOut = FirstPoint;
                        width = FirstWidth;
                        nPts = 0;
                    }

                    pWETE = pWETE->nextWETE;
                    while (pWETE != pAET)
                        EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
                    pWETE = pWETE->nextWETE;
                }
                EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
            }

            /*
             *  reevaluate the Winding active edge table if we
             *  just had to resort it or if we just exited an edge.
             */
            if (miInsertionSort(&AET) || fixWAET) {
                micomputeWAET(&AET);
                fixWAET = 0;
            }
        }
    }

    /*
     *     Get any spans that we missed by buffering
     */
    (*pgc->ops->FillSpans) (dst, pgc, nPts, FirstPoint, FirstWidth, 1);
    free(pETEs);
    miFreeStorage(SLLBlock.next);
    return TRUE;
}

/*
 *  Draw polygons.  This routine translates the point by the origin if
 *  pGC->miTranslate is non-zero, and calls to the appropriate routine to
 *  actually scan convert the polygon.
 */
void
miFillPolygon(DrawablePtr dst, GCPtr pgc,
              int shape, int mode, int count, DDXPointPtr pPts)
{
    int i;
    int xorg, yorg;
    DDXPointPtr ppt;

    if (count == 0)
        return;

    ppt = pPts;
    if (pgc->miTranslate) {
        xorg = dst->x;
        yorg = dst->y;

        if (mode == CoordModeOrigin) {
            for (i = 0; i < count; i++) {
                ppt->x += xorg;
                ppt++->y += yorg;
            }
        }
        else {
            ppt->x += xorg;
            ppt++->y += yorg;
            for (i = 1; i < count; i++) {
                ppt->x += (ppt - 1)->x;
                ppt->y += (ppt - 1)->y;
                ppt++;
            }
        }
    }
    else {
        if (mode == CoordModePrevious) {
            ppt++;
            for (i = 1; i < count; i++) {
                ppt->x += (ppt - 1)->x;
                ppt->y += (ppt - 1)->y;
                ppt++;
            }
        }
    }
    if (shape == Convex)
        miFillConvexPoly(dst, pgc, count, pPts);
    else
        miFillGeneralPoly(dst, pgc, count, pPts);
}
