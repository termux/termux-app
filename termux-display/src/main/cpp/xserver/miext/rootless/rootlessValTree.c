/*
 * Calculate window clip lists for rootless mode
 *
 * This file is very closely based on mivaltree.c.
 */

/*
 * mivaltree.c --
 *	Functions for recalculating window clip lists. Main function
 *	is miValidateTree.
 *

Copyright 1987, 1988, 1989, 1998  The Open Group

All Rights Reserved.

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

 *
 * Copyright 1987, 1988, 1989 by
 * Digital Equipment Corporation, Maynard, Massachusetts,
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
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
 /*
  * Aug '86: Susan Angebranndt -- original code
  * July '87: Adam de Boor -- substantially modified and commented
  * Summer '89: Joel McCormack -- so fast you wouldn't believe it possible.
  *             In particular, much improved code for window mapping and
  *             circulating.
  *             Bob Scheifler -- avoid miComputeClips for unmapped windows,
  *                              valdata changes
  */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>             /* For NULL */
#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "validate.h"
#include    "windowstr.h"
#include    "mi.h"
#include    "regionstr.h"
#include    "mivalidate.h"

#include    "globals.h"

int RootlessMiValidateTree(WindowPtr pRoot, WindowPtr pChild, VTKind kind);

#define HasParentRelativeBorder(w) (!(w)->borderIsPixel && \
				    HasBorder(w) && \
				    (w)->backgroundState == ParentRelative)

/*
 *-----------------------------------------------------------------------
 * RootlessComputeClips --
 *	Recompute the clipList, borderClip, exposed and borderExposed
 *	regions for pParent and its children. Only viewable windows are
 *	taken into account.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	clipList, borderClip, exposed and borderExposed are altered.
 *	A VisibilityNotify event may be generated on the parent window.
 *
 *-----------------------------------------------------------------------
 */
static void
RootlessComputeClips(WindowPtr pParent, ScreenPtr pScreen,
                     RegionPtr universe, VTKind kind, RegionPtr exposed)
{
    int dx, dy;
    RegionRec childUniverse;
    register WindowPtr pChild;
    int oldVis, newVis;
    BoxRec borderSize;
    RegionRec childUnion;
    Bool overlap;
    RegionPtr borderVisible;

    /*
     * Figure out the new visibility of this window.
     * The extent of the universe should be the same as the extent of
     * the borderSize region. If the window is unobscured, this rectangle
     * will be completely inside the universe (the universe will cover it
     * completely). If the window is completely obscured, none of the
     * universe will cover the rectangle.
     */
    borderSize.x1 = pParent->drawable.x - wBorderWidth(pParent);
    borderSize.y1 = pParent->drawable.y - wBorderWidth(pParent);
    dx = (int) pParent->drawable.x + (int) pParent->drawable.width +
        wBorderWidth(pParent);
    if (dx > 32767)
        dx = 32767;
    borderSize.x2 = dx;
    dy = (int) pParent->drawable.y + (int) pParent->drawable.height +
        wBorderWidth(pParent);
    if (dy > 32767)
        dy = 32767;
    borderSize.y2 = dy;

    oldVis = pParent->visibility;
    switch (RegionContainsRect(universe, &borderSize)) {
    case rgnIN:
        newVis = VisibilityUnobscured;
        break;
    case rgnPART:
        newVis = VisibilityPartiallyObscured;
        {
            RegionPtr pBounding;

            if ((pBounding = wBoundingShape(pParent))) {
                switch (miShapedWindowIn(universe, pBounding, &borderSize,
                                         pParent->drawable.x,
                                         pParent->drawable.y)) {
                case rgnIN:
                    newVis = VisibilityUnobscured;
                    break;
                case rgnOUT:
                    newVis = VisibilityFullyObscured;
                    break;
                }
            }
        }
        break;
    default:
        newVis = VisibilityFullyObscured;
        break;
    }

    pParent->visibility = newVis;
    if (oldVis != newVis &&
        ((pParent->
          eventMask | wOtherEventMasks(pParent)) & VisibilityChangeMask))
        SendVisibilityNotify(pParent);

    dx = pParent->drawable.x - pParent->valdata->before.oldAbsCorner.x;
    dy = pParent->drawable.y - pParent->valdata->before.oldAbsCorner.y;

    /*
     * avoid computations when dealing with simple operations
     */

    switch (kind) {
    case VTMap:
    case VTStack:
    case VTUnmap:
        break;
    case VTMove:
        if ((oldVis == newVis) &&
            ((oldVis == VisibilityFullyObscured) ||
             (oldVis == VisibilityUnobscured))) {
            pChild = pParent;
            while (1) {
                if (pChild->viewable) {
                    if (pChild->visibility != VisibilityFullyObscured) {
                        RegionTranslate(&pChild->borderClip, dx, dy);
                        RegionTranslate(&pChild->clipList, dx, dy);
                        pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
                        if (pScreen->ClipNotify)
                            (*pScreen->ClipNotify) (pChild, dx, dy);

                    }
                    if (pChild->valdata) {
                        RegionNull(&pChild->valdata->after.borderExposed);
                        if (HasParentRelativeBorder(pChild)) {
                            RegionSubtract(&pChild->valdata->after.
                                           borderExposed, &pChild->borderClip,
                                           &pChild->winSize);
                        }
                        RegionNull(&pChild->valdata->after.exposed);
                    }
                    if (pChild->firstChild) {
                        pChild = pChild->firstChild;
                        continue;
                    }
                }
                while (!pChild->nextSib && (pChild != pParent))
                    pChild = pChild->parent;
                if (pChild == pParent)
                    break;
                pChild = pChild->nextSib;
            }
            return;
        }
        /* fall through */
    default:
        /*
         * To calculate exposures correctly, we have to translate the old
         * borderClip and clipList regions to the window's new location so there
         * is a correspondence between pieces of the new and old clipping regions.
         */
        if (dx || dy) {
            /*
             * We translate the old clipList because that will be exposed or copied
             * if gravity is right.
             */
            RegionTranslate(&pParent->borderClip, dx, dy);
            RegionTranslate(&pParent->clipList, dx, dy);
        }
        break;
    case VTBroken:
        RegionEmpty(&pParent->borderClip);
        RegionEmpty(&pParent->clipList);
        break;
    }

    borderVisible = pParent->valdata->before.borderVisible;
    RegionNull(&pParent->valdata->after.borderExposed);
    RegionNull(&pParent->valdata->after.exposed);

    /*
     * Since the borderClip must not be clipped by the children, we do
     * the border exposure first...
     *
     * 'universe' is the window's borderClip. To figure the exposures, remove
     * the area that used to be exposed from the new.
     * This leaves a region of pieces that weren't exposed before.
     */

    if (HasBorder(pParent)) {
        if (borderVisible) {
            /*
             * when the border changes shape, the old visible portions
             * of the border will be saved by DIX in borderVisible --
             * use that region and destroy it
             */
            RegionSubtract(exposed, universe, borderVisible);
            RegionDestroy(borderVisible);
        }
        else {
            RegionSubtract(exposed, universe, &pParent->borderClip);
        }
        if (HasParentRelativeBorder(pParent) && (dx || dy)) {
            RegionSubtract(&pParent->valdata->after.borderExposed,
                           universe, &pParent->winSize);
        }
        else {
            RegionSubtract(&pParent->valdata->after.borderExposed,
                           exposed, &pParent->winSize);
        }

        RegionCopy(&pParent->borderClip, universe);

        /*
         * To get the right clipList for the parent, and to make doubly sure
         * that no child overlaps the parent's border, we remove the parent's
         * border from the universe before proceeding.
         */

        RegionIntersect(universe, universe, &pParent->winSize);
    }
    else
        RegionCopy(&pParent->borderClip, universe);

    if ((pChild = pParent->firstChild) && pParent->mapped) {
        RegionNull(&childUniverse);
        RegionNull(&childUnion);
        if ((pChild->drawable.y < pParent->lastChild->drawable.y) ||
            ((pChild->drawable.y == pParent->lastChild->drawable.y) &&
             (pChild->drawable.x < pParent->lastChild->drawable.x))) {
            for (; pChild; pChild = pChild->nextSib) {
                if (pChild->viewable)
                    RegionAppend(&childUnion, &pChild->borderSize);
            }
        }
        else {
            for (pChild = pParent->lastChild; pChild; pChild = pChild->prevSib) {
                if (pChild->viewable)
                    RegionAppend(&childUnion, &pChild->borderSize);
            }
        }
        RegionValidate(&childUnion, &overlap);

        for (pChild = pParent->firstChild; pChild; pChild = pChild->nextSib) {
            if (pChild->viewable) {
                /*
                 * If the child is viewable, we want to remove its extents
                 * from the current universe, but we only re-clip it if
                 * it's been marked.
                 */
                if (pChild->valdata) {
                    /*
                     * Figure out the new universe from the child's
                     * perspective and recurse.
                     */
                    RegionIntersect(&childUniverse,
                                    universe, &pChild->borderSize);
                    RootlessComputeClips(pChild, pScreen, &childUniverse,
                                         kind, exposed);
                }
                /*
                 * Once the child has been processed, we remove its extents
                 * from the current universe, thus denying its space to any
                 * other sibling.
                 */
                if (overlap)
                    RegionSubtract(universe, universe, &pChild->borderSize);
            }
        }
        if (!overlap)
            RegionSubtract(universe, universe, &childUnion);
        RegionUninit(&childUnion);
        RegionUninit(&childUniverse);
    }                           /* if any children */

    /*
     * 'universe' now contains the new clipList for the parent window.
     *
     * To figure the exposure of the window we subtract the old clip from the
     * new, just as for the border.
     */

    if (oldVis == VisibilityFullyObscured || oldVis == VisibilityNotViewable) {
        RegionCopy(&pParent->valdata->after.exposed, universe);
    }
    else if (newVis != VisibilityFullyObscured &&
             newVis != VisibilityNotViewable) {
        RegionSubtract(&pParent->valdata->after.exposed,
                       universe, &pParent->clipList);
    }

    /* HACK ALERT - copying contents of regions, instead of regions */
    {
        RegionRec tmp;

        tmp = pParent->clipList;
        pParent->clipList = *universe;
        *universe = tmp;
    }

#ifdef NOTDEF
    RegionCopy(&pParent->clipList, universe);
#endif

    pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    if (pScreen->ClipNotify)
        (*pScreen->ClipNotify) (pParent, dx, dy);
}

static void
RootlessTreeObscured(WindowPtr pParent)
{
    register WindowPtr pChild;
    register int oldVis;

    pChild = pParent;
    while (1) {
        if (pChild->viewable) {
            oldVis = pChild->visibility;
            if (oldVis != (pChild->visibility = VisibilityFullyObscured) &&
                ((pChild->
                  eventMask | wOtherEventMasks(pChild)) & VisibilityChangeMask))
                SendVisibilityNotify(pChild);
            if (pChild->firstChild) {
                pChild = pChild->firstChild;
                continue;
            }
        }
        while (!pChild->nextSib && (pChild != pParent))
            pChild = pChild->parent;
        if (pChild == pParent)
            break;
        pChild = pChild->nextSib;
    }
}

/*
 *-----------------------------------------------------------------------
 * RootlessMiValidateTree --
 *	Recomputes the clip list for pParent and all its inferiors.
 *
 * Results:
 *	Always returns 1.
 *
 * Side Effects:
 *	The clipList, borderClip, exposed, and borderExposed regions for
 *	each marked window are altered.
 *
 * Notes:
 *	This routine assumes that all affected windows have been marked
 *	(valdata created) and their winSize and borderSize regions
 *	adjusted to correspond to their new positions. The borderClip and
 *	clipList regions should not have been touched.
 *
 *	The top-most level is treated differently from all lower levels
 *	because pParent is unchanged. For the top level, we merge the
 *	regions taken up by the marked children back into the clipList
 *	for pParent, thus forming a region from which the marked children
 *	can claim their areas. For lower levels, where the old clipList
 *	and borderClip are invalid, we can't do this and have to do the
 *	extra operations done in miComputeClips, but this is much faster
 *	e.g. when only one child has moved...
 *
 *-----------------------------------------------------------------------
 */
/*
   Quartz version: used for validate from root in rootless mode.
   We need to make sure top-level windows don't clip each other,
   and that top-level windows aren't clipped to the root window.
*/
 /*ARGSUSED*/
// fixme this is ugly
// Xprint/ValTree.c doesn't work, but maybe that method can?
    int
RootlessMiValidateTree(WindowPtr pRoot, /* Parent to validate */
                       WindowPtr pChild,        /* First child of pRoot that was
                                                 * affected */
                       VTKind kind /* What kind of configuration caused call */
                       )
{
    RegionRec childClip;        /* The new borderClip for the current
                                 * child */
    RegionRec exposed;          /* For intermediate calculations */
    register ScreenPtr pScreen;
    register WindowPtr pWin;

    pScreen = pRoot->drawable.pScreen;
    if (pChild == NullWindow)
        pChild = pRoot->firstChild;

    RegionNull(&childClip);
    RegionNull(&exposed);

    if (RegionBroken(&pRoot->clipList) && !RegionBroken(&pRoot->borderClip)) {
        // fixme this might not work, but hopefully doesn't happen anyway.
        kind = VTBroken;
        RegionNull(&pRoot->clipList);
        ErrorF("ValidateTree: BUSTED!\n");
    }

    /*
     * Recursively compute the clips for all children of the root.
     * They don't clip against each other or the root itself, so
     * childClip is always reset to that child's size.
     */

    for (pWin = pChild; pWin != NullWindow; pWin = pWin->nextSib) {
        if (pWin->viewable) {
            if (pWin->valdata) {
                RegionCopy(&childClip, &pWin->borderSize);
                RootlessComputeClips(pWin, pScreen, &childClip, kind, &exposed);
            }
            else if (pWin->visibility == VisibilityNotViewable) {
                RootlessTreeObscured(pWin);
            }
        }
        else {
            if (pWin->valdata) {
                RegionEmpty(&pWin->clipList);
                if (pScreen->ClipNotify)
                    (*pScreen->ClipNotify) (pWin, 0, 0);
                RegionEmpty(&pWin->borderClip);
                pWin->valdata = NULL;
            }
        }
    }

    RegionUninit(&childClip);

    /* The root is never clipped by its children, so nothing on the root
       is ever exposed by moving or mapping its children. */
    RegionNull(&pRoot->valdata->after.exposed);
    RegionNull(&pRoot->valdata->after.borderExposed);

    return 1;
}
