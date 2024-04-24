/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 *		Kensuke Matsuzaki
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * Prototypes for local functions
 */

static int
 winAddRgn(WindowPtr pWindow, void *data);

static
    void
 winUpdateRgnRootless(WindowPtr pWindow);

static
    void
 winReshapeRootless(WindowPtr pWin);

/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbCreateWindow() */

Bool
winCreateWindowRootless(WindowPtr pWin)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winWindowPriv(pWin);
    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winCreateWindowRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(CreateWindow);
    fResult = (*pScreen->CreateWindow) (pWin);
    WIN_WRAP(CreateWindow, winCreateWindowRootless);

    pWinPriv->hRgn = NULL;

    return fResult;
}

/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbDestroyWindow() */

Bool
winDestroyWindowRootless(WindowPtr pWin)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winWindowPriv(pWin);
    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winDestroyWindowRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(DestroyWindow);
    fResult = (*pScreen->DestroyWindow) (pWin);
    WIN_WRAP(DestroyWindow, winDestroyWindowRootless);

    if (pWinPriv->hRgn != NULL) {
        DeleteObject(pWinPriv->hRgn);
        pWinPriv->hRgn = NULL;
    }

    winUpdateRgnRootless(pWin);

    return fResult;
}

/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbPositionWindow() */

Bool
winPositionWindowRootless(WindowPtr pWin, int x, int y)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winPositionWindowRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(PositionWindow);
    fResult = (*pScreen->PositionWindow) (pWin, x, y);
    WIN_WRAP(PositionWindow, winPositionWindowRootless);

    winUpdateRgnRootless(pWin);

    return fResult;
}

/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbChangeWindowAttributes() */

Bool
winChangeWindowAttributesRootless(WindowPtr pWin, unsigned long mask)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winChangeWindowAttributesRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(ChangeWindowAttributes);
    fResult = (*pScreen->ChangeWindowAttributes) (pWin, mask);
    WIN_WRAP(ChangeWindowAttributes, winChangeWindowAttributesRootless);

    winUpdateRgnRootless(pWin);

    return fResult;
}

/* See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowRootless(WindowPtr pWin)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winWindowPriv(pWin);
    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winUnmapWindowRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(UnrealizeWindow);
    fResult = (*pScreen->UnrealizeWindow) (pWin);
    WIN_WRAP(UnrealizeWindow, winUnmapWindowRootless);

    if (pWinPriv->hRgn != NULL) {
        DeleteObject(pWinPriv->hRgn);
        pWinPriv->hRgn = NULL;
    }

    winUpdateRgnRootless(pWin);

    return fResult;
}

/* See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowRootless(WindowPtr pWin)
{
    Bool fResult = FALSE;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winMapWindowRootless (%p)\n", pWin);
#endif

    WIN_UNWRAP(RealizeWindow);
    fResult = (*pScreen->RealizeWindow) (pWin);
    WIN_WRAP(RealizeWindow, winMapWindowRootless);

    winReshapeRootless(pWin);

    winUpdateRgnRootless(pWin);

    return fResult;
}

void
winSetShapeRootless(WindowPtr pWin, int kind)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    winScreenPriv(pScreen);

#if CYGDEBUG
    winTrace("winSetShapeRootless (%p, %i)\n", pWin, kind);
#endif

    WIN_UNWRAP(SetShape);
    (*pScreen->SetShape) (pWin, kind);
    WIN_WRAP(SetShape, winSetShapeRootless);

    winReshapeRootless(pWin);
    winUpdateRgnRootless(pWin);

    return;
}

/*
 * Local function for adding a region to the Windows window region
 */

static
    int
winAddRgn(WindowPtr pWin, void *data)
{
    int iX, iY, iWidth, iHeight, iBorder;
    HRGN hRgn = *(HRGN *) data;
    HRGN hRgnWin;

    winWindowPriv(pWin);

    /* If pWin is not Root */
    if (pWin->parent != NULL) {
#if CYGDEBUG
        winDebug("winAddRgn ()\n");
#endif
        if (pWin->mapped) {
            iBorder = wBorderWidth(pWin);

            iX = pWin->drawable.x - iBorder;
            iY = pWin->drawable.y - iBorder;

            iWidth = pWin->drawable.width + iBorder * 2;
            iHeight = pWin->drawable.height + iBorder * 2;

            hRgnWin = CreateRectRgn(0, 0, iWidth, iHeight);

            if (hRgnWin == NULL) {
                ErrorF("winAddRgn - CreateRectRgn () failed\n");
                ErrorF("  Rect %d %d %d %d\n",
                       iX, iY, iX + iWidth, iY + iHeight);
            }

            if (pWinPriv->hRgn) {
                if (CombineRgn(hRgnWin, hRgnWin, pWinPriv->hRgn, RGN_AND)
                    == ERROR) {
                    ErrorF("winAddRgn - CombineRgn () failed\n");
                }
            }

            OffsetRgn(hRgnWin, iX, iY);

            if (CombineRgn(hRgn, hRgn, hRgnWin, RGN_OR) == ERROR) {
                ErrorF("winAddRgn - CombineRgn () failed\n");
            }

            DeleteObject(hRgnWin);
        }
        return WT_DONTWALKCHILDREN;
    }
    else {
        return WT_WALKCHILDREN;
    }
}

/*
 * Local function to update the Windows window's region
 */

static
    void
winUpdateRgnRootless(WindowPtr pWin)
{
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

    if (hRgn != NULL) {
        WalkTree(pWin->drawable.pScreen, winAddRgn, &hRgn);
        SetWindowRgn(winGetScreenPriv(pWin->drawable.pScreen)->hwndScreen,
                     hRgn, TRUE);
    }
    else {
        ErrorF("winUpdateRgnRootless - CreateRectRgn failed.\n");
    }
}

static
    void
winReshapeRootless(WindowPtr pWin)
{
    int nRects;
    RegionRec rrNewShape;
    BoxPtr pShape, pRects, pEnd;
    HRGN hRgn, hRgnRect;

    winWindowPriv(pWin);

#if CYGDEBUG
    winDebug("winReshapeRootless ()\n");
#endif

    /* Bail if the window is the root window */
    if (pWin->parent == NULL)
        return;

    /* Bail if the window is not top level */
    if (pWin->parent->parent != NULL)
        return;

    /* Free any existing window region stored in the window privates */
    if (pWinPriv->hRgn != NULL) {
        DeleteObject(pWinPriv->hRgn);
        pWinPriv->hRgn = NULL;
    }

    /* Bail if the window has no bounding region defined */
    if (!wBoundingShape(pWin))
        return;

    RegionNull(&rrNewShape);
    RegionCopy(&rrNewShape, wBoundingShape(pWin));
    RegionTranslate(&rrNewShape, pWin->borderWidth, pWin->borderWidth);

    nRects = RegionNumRects(&rrNewShape);
    pShape = RegionRects(&rrNewShape);

    if (nRects > 0) {
        /* Create initial empty Windows region */
        hRgn = CreateRectRgn(0, 0, 0, 0);

        /* Loop through all rectangles in the X region */
        for (pRects = pShape, pEnd = pShape + nRects; pRects < pEnd; pRects++) {
            /* Create a Windows region for the X rectangle */
            hRgnRect = CreateRectRgn(pRects->x1, pRects->y1,
                                     pRects->x2, pRects->y2);
            if (hRgnRect == NULL) {
                ErrorF("winReshapeRootless - CreateRectRgn() failed\n");
            }

            /* Merge the Windows region with the accumulated region */
            if (CombineRgn(hRgn, hRgn, hRgnRect, RGN_OR) == ERROR) {
                ErrorF("winReshapeRootless - CombineRgn() failed\n");
            }

            /* Delete the temporary Windows region */
            DeleteObject(hRgnRect);
        }

        /* Save a handle to the composite region in the window privates */
        pWinPriv->hRgn = hRgn;
    }

    RegionUninit(&rrNewShape);

    return;
}
