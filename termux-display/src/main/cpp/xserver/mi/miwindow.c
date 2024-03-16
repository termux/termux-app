
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
#include <X11/extensions/shapeconst.h>
#include "regionstr.h"
#include "region.h"
#include "mi.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "mivalidate.h"
#include "inputstr.h"

void
miClearToBackground(WindowPtr pWin,
                    int x, int y, int w, int h, Bool generateExposures)
{
    BoxRec box;
    RegionRec reg;
    BoxPtr extents;
    int x1, y1, x2, y2;

    /* compute everything using ints to avoid overflow */

    x1 = pWin->drawable.x + x;
    y1 = pWin->drawable.y + y;
    if (w)
        x2 = x1 + (int) w;
    else
        x2 = x1 + (int) pWin->drawable.width - (int) x;
    if (h)
        y2 = y1 + h;
    else
        y2 = y1 + (int) pWin->drawable.height - (int) y;

    extents = &pWin->clipList.extents;

    /* clip the resulting rectangle to the window clipList extents.  This
     * makes sure that the result will fit in a box, given that the
     * screen is < 32768 on a side.
     */

    if (x1 < extents->x1)
        x1 = extents->x1;
    if (x2 > extents->x2)
        x2 = extents->x2;
    if (y1 < extents->y1)
        y1 = extents->y1;
    if (y2 > extents->y2)
        y2 = extents->y2;

    if (x2 <= x1 || y2 <= y1) {
        x2 = x1 = 0;
        y2 = y1 = 0;
    }

    box.x1 = x1;
    box.x2 = x2;
    box.y1 = y1;
    box.y2 = y2;

    RegionInit(&reg, &box, 1);

    RegionIntersect(&reg, &reg, &pWin->clipList);
    if (generateExposures)
        (*pWin->drawable.pScreen->WindowExposures) (pWin, &reg);
    else if (pWin->backgroundState != None)
        pWin->drawable.pScreen->PaintWindow(pWin, &reg, PW_BACKGROUND);
    RegionUninit(&reg);
}

void
miMarkWindow(WindowPtr pWin)
{
    ValidatePtr val;

    if (pWin->valdata)
        return;
    val = (ValidatePtr) xnfalloc(sizeof(ValidateRec));
    val->before.oldAbsCorner.x = pWin->drawable.x;
    val->before.oldAbsCorner.y = pWin->drawable.y;
    val->before.borderVisible = NullRegion;
    val->before.resized = FALSE;
    pWin->valdata = val;
}

Bool
miMarkOverlappedWindows(WindowPtr pWin, WindowPtr pFirst, WindowPtr *ppLayerWin)
{
    BoxPtr box;
    WindowPtr pChild, pLast;
    Bool anyMarked = FALSE;
    MarkWindowProcPtr MarkWindow = pWin->drawable.pScreen->MarkWindow;

    /* single layered systems are easy */
    if (ppLayerWin)
        *ppLayerWin = pWin;

    if (pWin == pFirst) {
        /* Blindly mark pWin and all of its inferiors.   This is a slight
         * overkill if there are mapped windows that outside pWin's border,
         * but it's better than wasting time on RectIn checks.
         */
        pChild = pWin;
        while (1) {
            if (pChild->viewable) {
                if (RegionBroken(&pChild->winSize))
                    SetWinSize(pChild);
                if (RegionBroken(&pChild->borderSize))
                    SetBorderSize(pChild);
                (*MarkWindow) (pChild);
                if (pChild->firstChild) {
                    pChild = pChild->firstChild;
                    continue;
                }
            }
            while (!pChild->nextSib && (pChild != pWin))
                pChild = pChild->parent;
            if (pChild == pWin)
                break;
            pChild = pChild->nextSib;
        }
        anyMarked = TRUE;
        pFirst = pFirst->nextSib;
    }
    if ((pChild = pFirst)) {
        box = RegionExtents(&pWin->borderSize);
        pLast = pChild->parent->lastChild;
        while (1) {
            if (pChild->viewable) {
                if (RegionBroken(&pChild->winSize))
                    SetWinSize(pChild);
                if (RegionBroken(&pChild->borderSize))
                    SetBorderSize(pChild);
                if (RegionContainsRect(&pChild->borderSize, box)) {
                    (*MarkWindow) (pChild);
                    anyMarked = TRUE;
                    if (pChild->firstChild) {
                        pChild = pChild->firstChild;
                        continue;
                    }
                }
            }
            while (!pChild->nextSib && (pChild != pLast))
                pChild = pChild->parent;
            if (pChild == pLast)
                break;
            pChild = pChild->nextSib;
        }
    }
    if (anyMarked)
        (*MarkWindow) (pWin->parent);
    return anyMarked;
}

/*****
 *  miHandleValidateExposures(pWin)
 *    starting at pWin, draw background in any windows that have exposure
 *    regions, translate the regions, restore any backing store,
 *    and then send any regions still exposed to the client
 *****/
void
miHandleValidateExposures(WindowPtr pWin)
{
    WindowPtr pChild;
    ValidatePtr val;
    WindowExposuresProcPtr WindowExposures;

    pChild = pWin;
    WindowExposures = pChild->drawable.pScreen->WindowExposures;
    while (1) {
        if ((val = pChild->valdata)) {
            if (RegionNotEmpty(&val->after.borderExposed))
                pWin->drawable.pScreen->PaintWindow(pChild,
                                                    &val->after.borderExposed,
                                                    PW_BORDER);
            RegionUninit(&val->after.borderExposed);
            (*WindowExposures) (pChild, &val->after.exposed);
            RegionUninit(&val->after.exposed);
            free(val);
            pChild->valdata = NULL;
            if (pChild->firstChild) {
                pChild = pChild->firstChild;
                continue;
            }
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            break;
        pChild = pChild->nextSib;
    }
}

void
miMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pNextSib, VTKind kind)
{
    WindowPtr pParent;
    Bool WasViewable = (Bool) (pWin->viewable);
    short bw;
    RegionPtr oldRegion = NULL;
    DDXPointRec oldpt;
    Bool anyMarked = FALSE;
    ScreenPtr pScreen;
    WindowPtr windowToValidate;
    WindowPtr pLayerWin;

    /* if this is a root window, can't be moved */
    if (!(pParent = pWin->parent))
        return;
    pScreen = pWin->drawable.pScreen;
    bw = wBorderWidth(pWin);

    oldpt.x = pWin->drawable.x;
    oldpt.y = pWin->drawable.y;
    if (WasViewable) {
        oldRegion = RegionCreate(NullBox, 1);
        RegionCopy(oldRegion, &pWin->borderClip);
        anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pWin, &pLayerWin);
    }
    pWin->origin.x = x + (int) bw;
    pWin->origin.y = y + (int) bw;
    x = pWin->drawable.x = pParent->drawable.x + x + (int) bw;
    y = pWin->drawable.y = pParent->drawable.y + y + (int) bw;

    SetWinSize(pWin);
    SetBorderSize(pWin);

    (*pScreen->PositionWindow) (pWin, x, y);

    windowToValidate = MoveWindowInStack(pWin, pNextSib);

    ResizeChildrenWinSize(pWin, x - oldpt.x, y - oldpt.y, 0, 0);

    if (WasViewable) {
        if (pLayerWin == pWin)
            anyMarked |= (*pScreen->MarkOverlappedWindows)
                (pWin, windowToValidate, NULL);
        else
            anyMarked |= (*pScreen->MarkOverlappedWindows)
                (pWin, pLayerWin, NULL);

        if (anyMarked) {
            (*pScreen->ValidateTree) (pLayerWin->parent, NullWindow, kind);
            (*pWin->drawable.pScreen->CopyWindow) (pWin, oldpt, oldRegion);
            RegionDestroy(oldRegion);
            /* XXX need to retile border if ParentRelative origin */
            (*pScreen->HandleExposures) (pLayerWin->parent);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pLayerWin->parent, NULL, kind);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
}

/*
 * pValid is a region of the screen which has been
 * successfully copied -- recomputed exposed regions for affected windows
 */

static int
miRecomputeExposures(WindowPtr pWin, void *value)
{                               /* must conform to VisitWindowProcPtr */
    RegionPtr pValid = (RegionPtr) value;

    if (pWin->valdata) {
#ifdef COMPOSITE
        /*
         * Redirected windows are not affected by parent window
         * gravity manipulations, so don't recompute their
         * exposed areas here.
         */
        if (pWin->redirectDraw != RedirectDrawNone)
            return WT_DONTWALKCHILDREN;
#endif
        /*
         * compute exposed regions of this window
         */
        RegionSubtract(&pWin->valdata->after.exposed, &pWin->clipList, pValid);
        /*
         * compute exposed regions of the border
         */
        RegionSubtract(&pWin->valdata->after.borderExposed,
                       &pWin->borderClip, &pWin->winSize);
        RegionSubtract(&pWin->valdata->after.borderExposed,
                       &pWin->valdata->after.borderExposed, pValid);
        return WT_WALKCHILDREN;
    }
    return WT_NOMATCH;
}

void
miResizeWindow(WindowPtr pWin, int x, int y, unsigned int w, unsigned int h,
               WindowPtr pSib)
{
    WindowPtr pParent;
    Bool WasViewable = (Bool) (pWin->viewable);
    unsigned short width = pWin->drawable.width, height = pWin->drawable.height;
    short oldx = pWin->drawable.x, oldy = pWin->drawable.y;
    int bw = wBorderWidth(pWin);
    short dw, dh;
    DDXPointRec oldpt;
    RegionPtr oldRegion = NULL;
    Bool anyMarked = FALSE;
    ScreenPtr pScreen;
    WindowPtr pFirstChange;
    WindowPtr pChild;
    RegionPtr gravitate[StaticGravity + 1];
    unsigned g;
    int nx, ny;                 /* destination x,y */
    int newx, newy;             /* new inner window position */
    RegionPtr pRegion = NULL;
    RegionPtr destClip;         /* portions of destination already written */
    RegionPtr oldWinClip = NULL;        /* old clip list for window */
    RegionPtr borderVisible = NullRegion;       /* visible area of the border */
    Bool shrunk = FALSE;        /* shrunk in an inner dimension */
    Bool moved = FALSE;         /* window position changed */
    WindowPtr pLayerWin;

    /* if this is a root window, can't be resized */
    if (!(pParent = pWin->parent))
        return;

    pScreen = pWin->drawable.pScreen;
    newx = pParent->drawable.x + x + bw;
    newy = pParent->drawable.y + y + bw;
    if (WasViewable) {
        anyMarked = FALSE;
        /*
         * save the visible region of the window
         */
        oldRegion = RegionCreate(NullBox, 1);
        RegionCopy(oldRegion, &pWin->winSize);

        /*
         * categorize child windows into regions to be moved
         */
        for (g = 0; g <= StaticGravity; g++)
            gravitate[g] = (RegionPtr) NULL;
        for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
            g = pChild->winGravity;
            if (g != UnmapGravity) {
                if (!gravitate[g])
                    gravitate[g] = RegionCreate(NullBox, 1);
                RegionUnion(gravitate[g], gravitate[g], &pChild->borderClip);
            }
            else {
                UnmapWindow(pChild, TRUE);
                anyMarked = TRUE;
            }
        }
        anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin, pWin, &pLayerWin);

        oldWinClip = NULL;
        if (pWin->bitGravity != ForgetGravity) {
            oldWinClip = RegionCreate(NullBox, 1);
            RegionCopy(oldWinClip, &pWin->clipList);
        }
        /*
         * if the window is changing size, borderExposed
         * can't be computed correctly without some help.
         */
        if (pWin->drawable.height > h || pWin->drawable.width > w)
            shrunk = TRUE;

        if (newx != oldx || newy != oldy)
            moved = TRUE;

        if ((pWin->drawable.height != h || pWin->drawable.width != w) &&
            HasBorder(pWin)) {
            borderVisible = RegionCreate(NullBox, 1);
            /* for tiled borders, we punt and draw the whole thing */
            if (pWin->borderIsPixel || !moved) {
                if (shrunk || moved)
                    RegionSubtract(borderVisible,
                                   &pWin->borderClip, &pWin->winSize);
                else
                    RegionCopy(borderVisible, &pWin->borderClip);
            }
        }
    }
    pWin->origin.x = x + bw;
    pWin->origin.y = y + bw;
    pWin->drawable.height = h;
    pWin->drawable.width = w;

    x = pWin->drawable.x = newx;
    y = pWin->drawable.y = newy;

    SetWinSize(pWin);
    SetBorderSize(pWin);

    dw = (int) w - (int) width;
    dh = (int) h - (int) height;
    ResizeChildrenWinSize(pWin, x - oldx, y - oldy, dw, dh);

    /* let the hardware adjust background and border pixmaps, if any */
    (*pScreen->PositionWindow) (pWin, x, y);

    pFirstChange = MoveWindowInStack(pWin, pSib);

    if (WasViewable) {
        pRegion = RegionCreate(NullBox, 1);

        if (pLayerWin == pWin)
            anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin, pFirstChange,
                                                            NULL);
        else
            anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin, pLayerWin,
                                                            NULL);

        if (pWin->valdata) {
            pWin->valdata->before.resized = TRUE;
            pWin->valdata->before.borderVisible = borderVisible;
        }

        if (anyMarked)
            (*pScreen->ValidateTree) (pLayerWin->parent, pFirstChange, VTOther);
        /*
         * the entire window is trashed unless bitGravity
         * recovers portions of it
         */
        RegionCopy(&pWin->valdata->after.exposed, &pWin->clipList);
    }

    GravityTranslate(x, y, oldx, oldy, dw, dh, pWin->bitGravity, &nx, &ny);

    if (WasViewable) {
        /* avoid the border */
        if (HasBorder(pWin)) {
            int offx, offy, dx, dy;

            /* kruft to avoid double translates for each gravity */
            offx = 0;
            offy = 0;
            for (g = 0; g <= StaticGravity; g++) {
                if (!gravitate[g])
                    continue;

                /* align winSize to gravitate[g].
                 * winSize is in new coordinates,
                 * gravitate[g] is still in old coordinates */
                GravityTranslate(x, y, oldx, oldy, dw, dh, g, &nx, &ny);

                dx = (oldx - nx) - offx;
                dy = (oldy - ny) - offy;
                if (dx || dy) {
                    RegionTranslate(&pWin->winSize, dx, dy);
                    offx += dx;
                    offy += dy;
                }
                RegionIntersect(gravitate[g], gravitate[g], &pWin->winSize);
            }
            /* get winSize back where it belongs */
            if (offx || offy)
                RegionTranslate(&pWin->winSize, -offx, -offy);
        }
        /*
         * add screen bits to the appropriate bucket
         */

        if (oldWinClip) {
            /*
             * clip to new clipList
             */
            RegionCopy(pRegion, oldWinClip);
            RegionTranslate(pRegion, nx - oldx, ny - oldy);
            RegionIntersect(oldWinClip, pRegion, &pWin->clipList);
            /*
             * don't step on any gravity bits which will be copied after this
             * region.  Note -- this assumes that the regions will be copied
             * in gravity order.
             */
            for (g = pWin->bitGravity + 1; g <= StaticGravity; g++) {
                if (gravitate[g])
                    RegionSubtract(oldWinClip, oldWinClip, gravitate[g]);
            }
            RegionTranslate(oldWinClip, oldx - nx, oldy - ny);
            g = pWin->bitGravity;
            if (!gravitate[g])
                gravitate[g] = oldWinClip;
            else {
                RegionUnion(gravitate[g], gravitate[g], oldWinClip);
                RegionDestroy(oldWinClip);
            }
        }

        /*
         * move the bits on the screen
         */

        destClip = NULL;

        for (g = 0; g <= StaticGravity; g++) {
            if (!gravitate[g])
                continue;

            GravityTranslate(x, y, oldx, oldy, dw, dh, g, &nx, &ny);

            oldpt.x = oldx + (x - nx);
            oldpt.y = oldy + (y - ny);

            /* Note that gravitate[g] is *translated* by CopyWindow */

            /* only copy the remaining useful bits */

            RegionIntersect(gravitate[g], gravitate[g], oldRegion);

            /* clip to not overwrite already copied areas */

            if (destClip) {
                RegionTranslate(destClip, oldpt.x - x, oldpt.y - y);
                RegionSubtract(gravitate[g], gravitate[g], destClip);
                RegionTranslate(destClip, x - oldpt.x, y - oldpt.y);
            }

            /* and move those bits */

            if (oldpt.x != x || oldpt.y != y
#ifdef COMPOSITE
                || pWin->redirectDraw
#endif
                ) {
                (*pWin->drawable.pScreen->CopyWindow) (pWin, oldpt,
                                                       gravitate[g]);
            }

            /* remove any overwritten bits from the remaining useful bits */

            RegionSubtract(oldRegion, oldRegion, gravitate[g]);

            /*
             * recompute exposed regions of child windows
             */

            for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
                if (pChild->winGravity != g)
                    continue;
                RegionIntersect(pRegion, &pChild->borderClip, gravitate[g]);
                TraverseTree(pChild, miRecomputeExposures, (void *) pRegion);
            }

            /*
             * remove the successfully copied regions of the
             * window from its exposed region
             */

            if (g == pWin->bitGravity)
                RegionSubtract(&pWin->valdata->after.exposed,
                               &pWin->valdata->after.exposed, gravitate[g]);
            if (!destClip)
                destClip = gravitate[g];
            else {
                RegionUnion(destClip, destClip, gravitate[g]);
                RegionDestroy(gravitate[g]);
            }
        }

        RegionDestroy(oldRegion);
        RegionDestroy(pRegion);
        if (destClip)
            RegionDestroy(destClip);
        if (anyMarked) {
            (*pScreen->HandleExposures) (pLayerWin->parent);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pLayerWin->parent, pFirstChange,
                                              VTOther);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
}

WindowPtr
miGetLayerWindow(WindowPtr pWin)
{
    return pWin->firstChild;
}

/******
 *
 * miSetShape
 *    The border/window shape has changed.  Recompute winSize/borderSize
 *    and send appropriate exposure events
 */

void
miSetShape(WindowPtr pWin, int kind)
{
    Bool WasViewable = (Bool) (pWin->viewable);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    Bool anyMarked = FALSE;
    WindowPtr pLayerWin;

    if (kind != ShapeInput) {
        if (WasViewable) {
            anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pWin,
                                                           &pLayerWin);
            if (pWin->valdata) {
                if (HasBorder(pWin)) {
                    RegionPtr borderVisible;

                    borderVisible = RegionCreate(NullBox, 1);
                    RegionSubtract(borderVisible,
                                   &pWin->borderClip, &pWin->winSize);
                    pWin->valdata->before.borderVisible = borderVisible;
                }
                pWin->valdata->before.resized = TRUE;
            }
        }

        SetWinSize(pWin);
        SetBorderSize(pWin);

        ResizeChildrenWinSize(pWin, 0, 0, 0, 0);

        if (WasViewable) {
            anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);

            if (anyMarked) {
                (*pScreen->ValidateTree) (pLayerWin->parent, NullWindow,
                                          VTOther);
                (*pScreen->HandleExposures) (pLayerWin->parent);
                if (pScreen->PostValidateTree)
                    (*pScreen->PostValidateTree) (pLayerWin->parent, NULL,
                                                  VTOther);
            }
        }
    }
    if (pWin->realized)
        WindowsRestructured();
    CheckCursorConfinement(pWin);
}

/* Keeps the same inside(!) origin */

void
miChangeBorderWidth(WindowPtr pWin, unsigned int width)
{
    int oldwidth;
    Bool anyMarked = FALSE;
    ScreenPtr pScreen;
    Bool WasViewable = (Bool) (pWin->viewable);
    Bool HadBorder;
    WindowPtr pLayerWin;

    oldwidth = wBorderWidth(pWin);
    if (oldwidth == width)
        return;
    HadBorder = HasBorder(pWin);
    pScreen = pWin->drawable.pScreen;
    if (WasViewable && width < oldwidth)
        anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pWin, &pLayerWin);

    pWin->borderWidth = width;
    SetBorderSize(pWin);

    if (WasViewable) {
        if (width > oldwidth) {
            anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pWin,
                                                           &pLayerWin);
            /*
             * save the old border visible region to correctly compute
             * borderExposed.
             */
            if (pWin->valdata && HadBorder) {
                RegionPtr borderVisible;

                borderVisible = RegionCreate(NULL, 1);
                RegionSubtract(borderVisible,
                               &pWin->borderClip, &pWin->winSize);
                pWin->valdata->before.borderVisible = borderVisible;
            }
        }

        if (anyMarked) {
            (*pScreen->ValidateTree) (pLayerWin->parent, pLayerWin, VTOther);
            (*pScreen->HandleExposures) (pLayerWin->parent);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pLayerWin->parent, pLayerWin,
                                              VTOther);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
}

void
miMarkUnrealizedWindow(WindowPtr pChild, WindowPtr pWin, Bool fromConfigure)
{
    if ((pChild != pWin) || fromConfigure) {
        RegionEmpty(&pChild->clipList);
        if (pChild->drawable.pScreen->ClipNotify)
            (*pChild->drawable.pScreen->ClipNotify) (pChild, 0, 0);
        RegionEmpty(&pChild->borderClip);
    }
}

WindowPtr
miSpriteTrace(SpritePtr pSprite, int x, int y)
{
    WindowPtr pWin;
    BoxRec box;

    pWin = DeepestSpriteWin(pSprite)->firstChild;
    while (pWin) {
        if ((pWin->mapped) &&
            (x >= pWin->drawable.x - wBorderWidth(pWin)) &&
            (x < pWin->drawable.x + (int) pWin->drawable.width +
             wBorderWidth(pWin)) &&
            (y >= pWin->drawable.y - wBorderWidth(pWin)) &&
            (y < pWin->drawable.y + (int) pWin->drawable.height +
             wBorderWidth(pWin))
            /* When a window is shaped, a further check
             * is made to see if the point is inside
             * borderSize
             */
            && (!wBoundingShape(pWin) || PointInBorderSize(pWin, x, y))
            && (!wInputShape(pWin) ||
                RegionContainsPoint(wInputShape(pWin),
                                    x - pWin->drawable.x,
                                    y - pWin->drawable.y, &box))
            /* In rootless mode windows may be offscreen, even when
             * they're in X's stack. (E.g. if the native window system
             * implements some form of virtual desktop system).
             */
            && !pWin->unhittable) {
            if (pSprite->spriteTraceGood >= pSprite->spriteTraceSize) {
                pSprite->spriteTraceSize += 10;
                pSprite->spriteTrace = reallocarray(pSprite->spriteTrace,
                                                    pSprite->spriteTraceSize,
                                                    sizeof(WindowPtr));
            }
            pSprite->spriteTrace[pSprite->spriteTraceGood++] = pWin;
            pWin = pWin->firstChild;
        }
        else
            pWin = pWin->nextSib;
    }
    return DeepestSpriteWin(pSprite);
}

/**
 * Traversed from the root window to the window at the position x/y. While
 * traversing, it sets up the traversal history in the spriteTrace array.
 * After completing, the spriteTrace history is set in the following way:
 *   spriteTrace[0] ... root window
 *   spriteTrace[1] ... top level window that encloses x/y
 *       ...
 *   spriteTrace[spriteTraceGood - 1] ... window at x/y
 *
 * @returns the window at the given coordinates.
 */
WindowPtr
miXYToWindow(ScreenPtr pScreen, SpritePtr pSprite, int x, int y)
{
    pSprite->spriteTraceGood = 1;       /* root window still there */
    return miSpriteTrace(pSprite, x, y);
}
