/*
 * Rootless window management
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002-2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>             /* For NULL */
#include <limits.h>             /* For CHAR_BIT */
#include <assert.h>
#include <X11/Xatom.h>
#ifdef __APPLE__
#include <Xplugin.h>
#include "mi.h"
#include "pixmapstr.h"
#include "windowstr.h"
//#include <X11/extensions/applewm.h>
extern int darwinMainScreenX, darwinMainScreenY;
extern Bool no_configure_window;
#endif
#include "fb.h"

#include "rootlessCommon.h"
#include "rootlessWindow.h"

#define SCREEN_TO_GLOBAL_X \
    (pScreen->x + rootlessGlobalOffsetX)
#define SCREEN_TO_GLOBAL_Y \
    (pScreen->y + rootlessGlobalOffsetY)

#define DEFINE_ATOM_HELPER(func,atom_name)                      \
  static Atom func (void) {                                       \
    static unsigned int generation = 0;                             \
    static Atom atom;                                           \
    if (generation != serverGeneration) {                       \
      generation = serverGeneration;                          \
      atom = MakeAtom (atom_name, strlen (atom_name), TRUE);  \
    }                                                           \
    return atom;                                                \
  }

DEFINE_ATOM_HELPER(xa_native_window_id, "_NATIVE_WINDOW_ID")

static Bool windows_hidden;

// TODO - abstract xp functions

#ifdef __APPLE__

// XXX: identical to x_cvt_vptr_to_uint ?
#define MAKE_WINDOW_ID(x)		((xp_window_id)((size_t)(x)))

void
RootlessNativeWindowStateChanged(WindowPtr pWin, unsigned int state)
{
    RootlessWindowRec *winRec;

    if (pWin == NULL)
        return;

    winRec = WINREC(pWin);
    if (winRec == NULL)
        return;

    winRec->is_offscreen = ((state & XP_WINDOW_STATE_OFFSCREEN) != 0);
    winRec->is_obscured = ((state & XP_WINDOW_STATE_OBSCURED) != 0);
    pWin->unhittable = winRec->is_offscreen;
}

void
RootlessNativeWindowMoved(WindowPtr pWin)
{
    xp_box bounds;
    int sx, sy, err;
    XID vlist[2];
    Mask mask;
    ClientPtr pClient;
    RootlessWindowRec *winRec;

    winRec = WINREC(pWin);

    if (xp_get_window_bounds(MAKE_WINDOW_ID(winRec->wid), &bounds) != Success)
        return;

    sx = pWin->drawable.pScreen->x + darwinMainScreenX;
    sy = pWin->drawable.pScreen->y + darwinMainScreenY;

    /* Fake up a ConfigureWindow packet to resize the window to the current bounds. */
    vlist[0] = (INT16) bounds.x1 - sx;
    vlist[1] = (INT16) bounds.y1 - sy;
    mask = CWX | CWY;

    /* pretend we're the owner of the window! */
    err =
        dixLookupClient(&pClient, pWin->drawable.id, serverClient,
                        DixUnknownAccess);
    if (err != Success) {
        ErrorF("RootlessNativeWindowMoved(): Failed to lookup window: 0x%x\n",
               (unsigned int) pWin->drawable.id);
        return;
    }

    /* Don't want to do anything to the physical window (avoids
       notification-response feedback loops) */

    no_configure_window = TRUE;
    ConfigureWindow(pWin, mask, vlist, pClient);
    no_configure_window = FALSE;
}

#endif                          /* __APPLE__ */

/*
 * RootlessCreateWindow
 *  For now, don't create a physical window until either the window is
 *  realized, or we really need it (e.g. to attach VRAM surfaces to).
 *  Do reset the window size so it's not clipped by the root window.
 */
Bool
RootlessCreateWindow(WindowPtr pWin)
{
    Bool result;
    RegionRec saveRoot;

    SETWINREC(pWin, NULL);
    dixSetPrivate(&pWin->devPrivates, rootlessWindowOldPixmapPrivateKey, NULL);

    SCREEN_UNWRAP(pWin->drawable.pScreen, CreateWindow);

    if (!IsRoot(pWin)) {
        /* win/border size set by DIX, not by wrapped CreateWindow, so
           correct it here. Don't HUGE_ROOT when pWin is the root! */

        HUGE_ROOT(pWin);
        SetWinSize(pWin);
        SetBorderSize(pWin);
    }

    result = pWin->drawable.pScreen->CreateWindow(pWin);

    if (pWin->parent) {
        NORMAL_ROOT(pWin);
    }

    SCREEN_WRAP(pWin->drawable.pScreen, CreateWindow);

    return result;
}

/*
 * RootlessDestroyFrame
 *  Destroy the physical window associated with the given window.
 */
static void
RootlessDestroyFrame(WindowPtr pWin, RootlessWindowPtr winRec)
{
    SCREENREC(pWin->drawable.pScreen)->imp->DestroyFrame(winRec->wid);
    free(winRec);
    SETWINREC(pWin, NULL);
}

/*
 * RootlessDestroyWindow
 *  Destroy the physical window associated with the given window.
 */
Bool
RootlessDestroyWindow(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    Bool result;

    if (winRec != NULL) {
        RootlessDestroyFrame(pWin, winRec);
    }

    SCREEN_UNWRAP(pWin->drawable.pScreen, DestroyWindow);
    result = pWin->drawable.pScreen->DestroyWindow(pWin);
    SCREEN_WRAP(pWin->drawable.pScreen, DestroyWindow);

    return result;
}

static Bool
RootlessGetShape(WindowPtr pWin, RegionPtr pShape)
{
    if (wBoundingShape(pWin) == NULL)
        return FALSE;

    /* wBoundingShape is relative to *inner* origin of window.
       Translate by borderWidth to get the outside-relative position. */

    RegionNull(pShape);
    RegionCopy(pShape, wBoundingShape(pWin));
    RegionTranslate(pShape, pWin->borderWidth, pWin->borderWidth);

    return TRUE;
}

/*
 * RootlessReshapeFrame
 *  Set the frame shape.
 */
static void
RootlessReshapeFrame(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    RegionRec newShape;
    RegionPtr pShape;

    // If the window is not yet framed, do nothing
    if (winRec == NULL)
        return;

    if (IsRoot(pWin))
        return;

    RootlessStopDrawing(pWin, FALSE);

    pShape = RootlessGetShape(pWin, &newShape) ? &newShape : NULL;

#ifdef ROOTLESSDEBUG
    RL_DEBUG_MSG("reshaping...");
    if (pShape != NULL) {
        RL_DEBUG_MSG("numrects %d, extents %d %d %d %d ",
                     RegionNumRects(&newShape),
                     newShape.extents.x1, newShape.extents.y1,
                     newShape.extents.x2, newShape.extents.y2);
    }
    else {
        RL_DEBUG_MSG("no shape ");
    }
#endif

    SCREENREC(pWin->drawable.pScreen)->imp->ReshapeFrame(winRec->wid, pShape);

    if (pShape != NULL)
        RegionUninit(&newShape);
}

/*
 * RootlessSetShape
 *  Shape is usually set before a window is mapped and the window will
 *  not have a frame associated with it. In this case, the frame will be
 *  shaped when the window is framed.
 */
void
RootlessSetShape(WindowPtr pWin, int kind)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    SCREEN_UNWRAP(pScreen, SetShape);
    pScreen->SetShape(pWin, kind);
    SCREEN_WRAP(pScreen, SetShape);

    RootlessReshapeFrame(pWin);
}

/* Disallow ParentRelative background on top-level windows
   because the root window doesn't really have the right background.
 */
Bool
RootlessChangeWindowAttributes(WindowPtr pWin, unsigned long vmask)
{
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("change window attributes start ");

    SCREEN_UNWRAP(pScreen, ChangeWindowAttributes);
    result = pScreen->ChangeWindowAttributes(pWin, vmask);
    SCREEN_WRAP(pScreen, ChangeWindowAttributes);

    if (WINREC(pWin)) {
        // disallow ParentRelative background state
        if (pWin->backgroundState == ParentRelative) {
            XID pixel = 0;

            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
        }
    }

    RL_DEBUG_MSG("change window attributes end\n");
    return result;
}

/*
 * RootlessPositionWindow
 *  This is a hook for when DIX moves or resizes a window.
 *  Update the frame position now although the physical window is moved
 *  in RootlessMoveWindow. (x, y) are *inside* position. After this,
 *  mi and fb are expecting the pixmap to be at the new location.
 */
Bool
RootlessPositionWindow(WindowPtr pWin, int x, int y)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    Bool result;

    RL_DEBUG_MSG("positionwindow start (win 0x%x @ %i, %i)\n", pWin, x, y);

    if (winRec) {
        if (winRec->is_drawing) {
            // Reset frame's pixmap and move it to the new position.
            int bw = wBorderWidth(pWin);

            winRec->pixmap->devPrivate.ptr = winRec->pixelData;
            SetPixmapBaseToScreen(winRec->pixmap, x - bw, y - bw);
        }
    }

    SCREEN_UNWRAP(pScreen, PositionWindow);
    result = pScreen->PositionWindow(pWin, x, y);
    SCREEN_WRAP(pScreen, PositionWindow);

    RL_DEBUG_MSG("positionwindow end\n");
    return result;
}

/*
 * RootlessInitializeFrame
 *  Initialize some basic attributes of the frame. Note that winRec
 *  may already have valid data in it, so don't overwrite anything
 *  valuable.
 */
static void
RootlessInitializeFrame(WindowPtr pWin, RootlessWindowRec * winRec)
{
    DrawablePtr d = &pWin->drawable;
    int bw = wBorderWidth(pWin);

    winRec->win = pWin;

    winRec->x = d->x - bw;
    winRec->y = d->y - bw;
    winRec->width = d->width + 2 * bw;
    winRec->height = d->height + 2 * bw;
    winRec->borderWidth = bw;
}

/*
 * RootlessEnsureFrame
 *  Make sure the given window is framed. If the window doesn't have a
 *  physical window associated with it, attempt to create one. If that
 *  is unsuccessful, return NULL.
 */
static RootlessWindowRec *
RootlessEnsureFrame(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec;
    RegionRec shape;
    RegionPtr pShape = NULL;

    if (WINREC(pWin) != NULL)
        return WINREC(pWin);

    if (!IsTopLevel(pWin) && !IsRoot(pWin))
        return NULL;

    if (pWin->drawable.class != InputOutput)
        return NULL;

    winRec = malloc(sizeof(RootlessWindowRec));

    if (!winRec)
        return NULL;

    RootlessInitializeFrame(pWin, winRec);

    winRec->is_drawing = FALSE;
    winRec->is_reorder_pending = FALSE;
    winRec->pixmap = NULL;
    winRec->wid = NULL;
    winRec->level = 0;

    SETWINREC(pWin, winRec);

    // Set the frame's shape if the window is shaped
    if (RootlessGetShape(pWin, &shape))
        pShape = &shape;

    RL_DEBUG_MSG("creating frame ");

    if (!SCREENREC(pScreen)->imp->CreateFrame(winRec, pScreen,
                                              winRec->x + SCREEN_TO_GLOBAL_X,
                                              winRec->y + SCREEN_TO_GLOBAL_Y,
                                              pShape)) {
        RL_DEBUG_MSG("implementation failed to create frame!\n");
        free(winRec);
        SETWINREC(pWin, NULL);
        return NULL;
    }

    if (pWin->drawable.depth == 8)
        RootlessFlushWindowColormap(pWin);

    if (pShape != NULL)
        RegionUninit(&shape);

    return winRec;
}

/*
 * RootlessRealizeWindow
 *  The frame is usually created here and not in CreateWindow so that
 *  windows do not eat memory until they are realized.
 */
Bool
RootlessRealizeWindow(WindowPtr pWin)
{
    Bool result;
    RegionRec saveRoot;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("realizewindow start (win 0x%x) ", pWin);

    if ((IsTopLevel(pWin) && pWin->drawable.class == InputOutput)) {
        RootlessWindowRec *winRec;

        winRec = RootlessEnsureFrame(pWin);
        if (winRec == NULL)
            return FALSE;

        winRec->is_reorder_pending = TRUE;

        RL_DEBUG_MSG("Top level window ");

        // Disallow ParentRelative background state on top-level windows.
        // This might have been set before the window was mapped.
        if (pWin->backgroundState == ParentRelative) {
            XID pixel = 0;

            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
        }
    }

    if (!IsRoot(pWin))
        HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, RealizeWindow);
    result = pScreen->RealizeWindow(pWin);
    SCREEN_WRAP(pScreen, RealizeWindow);
    if (!IsRoot(pWin))
        NORMAL_ROOT(pWin);

    RL_DEBUG_MSG("realizewindow end\n");
    return result;
}

/*
 * RootlessFrameForWindow
 *  Returns the frame ID for the physical window displaying the given window.
 *  If CREATE is true and the window has no frame, attempt to create one.
 */
RootlessFrameID
RootlessFrameForWindow(WindowPtr pWin, Bool create)
{
    WindowPtr pTopWin;
    RootlessWindowRec *winRec;

    pTopWin = TopLevelParent(pWin);
    if (pTopWin == NULL)
        return NULL;

    winRec = WINREC(pTopWin);

    if (winRec == NULL && create && pWin->drawable.class == InputOutput) {
        winRec = RootlessEnsureFrame(pTopWin);
    }

    if (winRec == NULL)
        return NULL;

    return winRec->wid;
}

/*
 * RootlessUnrealizeWindow
 *  Unmap the physical window.
 */
Bool
RootlessUnrealizeWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    Bool result;

    RL_DEBUG_MSG("unrealizewindow start ");

    if (winRec) {
        RootlessStopDrawing(pWin, FALSE);

        SCREENREC(pScreen)->imp->UnmapFrame(winRec->wid);

        winRec->is_reorder_pending = FALSE;
    }

    SCREEN_UNWRAP(pScreen, UnrealizeWindow);
    result = pScreen->UnrealizeWindow(pWin);
    SCREEN_WRAP(pScreen, UnrealizeWindow);

    RL_DEBUG_MSG("unrealizewindow end\n");
    return result;
}

/*
 * RootlessReorderWindow
 *  Reorder the frame associated with the given window so that it's
 *  physically above the window below it in the X stacking order.
 */
void
RootlessReorderWindow(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);

    if (pWin->realized && winRec != NULL && !winRec->is_reorder_pending &&
        !windows_hidden) {
        WindowPtr newPrevW;
        RootlessWindowRec *newPrev;
        RootlessFrameID newPrevID;
        ScreenPtr pScreen = pWin->drawable.pScreen;

        /* Check if the implementation wants the frame to not be reordered
           even though the X11 window is restacked. This can be useful if
           frames are ordered-in with animation so that the reordering is not
           done until the animation is complete. */
        if (SCREENREC(pScreen)->imp->DoReorderWindow) {
            if (!SCREENREC(pScreen)->imp->DoReorderWindow(winRec))
                return;
        }

        RootlessStopDrawing(pWin, FALSE);

        /* Find the next window above this one that has a mapped frame.
         * Only include cases where the windows are in the same category of
         * hittability to ensure offscreen windows don't get restacked
         * relative to onscreen ones (but that the offscreen ones maintain
         * their stacking order if they are explicitly asked to Reorder).
         */

        newPrevW = pWin->prevSib;
        while (newPrevW &&
               (WINREC(newPrevW) == NULL || !newPrevW->realized ||
                newPrevW->unhittable != pWin->unhittable))
            newPrevW = newPrevW->prevSib;

        newPrev = newPrevW != NULL ? WINREC(newPrevW) : NULL;
        newPrevID = newPrev != NULL ? newPrev->wid : 0;

        /* If it exists, reorder the frame above us first. */

        if (newPrev && newPrev->is_reorder_pending) {
            newPrev->is_reorder_pending = FALSE;
            RootlessReorderWindow(newPrevW);
        }

        SCREENREC(pScreen)->imp->RestackFrame(winRec->wid, newPrevID);
    }
}

/*
 * RootlessRestackWindow
 *  This is a hook for when DIX changes the window stacking order.
 *  The window has already been inserted into its new position in the
 *  DIX window stack. We need to change the order of the physical
 *  window to match.
 */
void
RootlessRestackWindow(WindowPtr pWin, WindowPtr pOldNextSib)
{
    RegionRec saveRoot;
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("restackwindow start ");
    if (winRec)
        RL_DEBUG_MSG("restack top level \n");

    HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, RestackWindow);

    if (pScreen->RestackWindow)
        pScreen->RestackWindow(pWin, pOldNextSib);

    SCREEN_WRAP(pScreen, RestackWindow);
    NORMAL_ROOT(pWin);

    if (winRec && pWin->viewable) {
        RootlessReorderWindow(pWin);
    }

    RL_DEBUG_MSG("restackwindow end\n");
}

/*
 * Specialized window copy procedures
 */

// Globals needed during window resize and move.
static void *gResizeDeathBits = NULL;
static int gResizeDeathCount = 0;
static PixmapPtr gResizeDeathPix[2] = { NULL, NULL };

static BoxRec gResizeDeathBounds[2];
static CopyWindowProcPtr gResizeOldCopyWindowProc = NULL;

/*
 * RootlessNoCopyWindow
 *  CopyWindow() that doesn't do anything. For MoveWindow() of
 *  top-level windows.
 */
static void
RootlessNoCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    // some code expects the region to be translated
    int dx = ptOldOrg.x - pWin->drawable.x;
    int dy = ptOldOrg.y - pWin->drawable.y;

    RL_DEBUG_MSG("ROOTLESSNOCOPYWINDOW ");

    RegionTranslate(prgnSrc, -dx, -dy);
}

/*
 * RootlessResizeCopyWindow
 *  CopyWindow used during ResizeWindow for gravity moves. Based on
 *  fbCopyWindow. The original always draws on the root pixmap, which
 *  we don't have. Instead, draw on the parent window's pixmap.
 *  Resize version: the old location's pixels are in gResizeCopyWindowSource.
 */
static void
RootlessResizeCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                         RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RegionRec rgnDst;
    int dx, dy;

    RL_DEBUG_MSG("resizecopywindowFB start (win 0x%x) ", pWin);

    /* Don't unwrap pScreen->CopyWindow.
       The bogus rewrap with RootlessCopyWindow causes a crash if
       CopyWindow is called again during the same resize. */

    if (gResizeDeathCount == 0)
        return;

    RootlessStartDrawing(pWin);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    RegionTranslate(prgnSrc, -dx, -dy);
    RegionNull(&rgnDst);
    RegionIntersect(&rgnDst, &pWin->borderClip, prgnSrc);

    if (gResizeDeathCount == 1) {
        /* Simple case, we only have a single source pixmap. */

        miCopyRegion(&gResizeDeathPix[0]->drawable,
                     &pScreen->GetWindowPixmap(pWin)->drawable, 0,
                     &rgnDst, dx, dy, fbCopyWindowProc, 0, 0);
    }
    else {
        int i;
        RegionRec clip, clipped;

        /* More complex case, N source pixmaps (usually two). So we
           intersect the destination with each source and copy those bits. */

        for (i = 0; i < gResizeDeathCount; i++) {
            RegionInit(&clip, gResizeDeathBounds + 0, 1);
            RegionNull(&clipped);
            RegionIntersect(&rgnDst, &clip, &clipped);

            miCopyRegion(&gResizeDeathPix[i]->drawable,
                         &pScreen->GetWindowPixmap(pWin)->drawable, 0,
                         &clipped, dx, dy, fbCopyWindowProc, 0, 0);

            RegionUninit(&clipped);
            RegionUninit(&clip);
        }
    }

    /* Don't update - resize will update everything */
    RegionUninit(&rgnDst);

    fbValidateDrawable(&pWin->drawable);

    RL_DEBUG_MSG("resizecopywindowFB end\n");
}

/*
 * RootlessCopyWindow
 *  Update *new* location of window. Old location is redrawn with
 *  PaintWindow. Cloned from fbCopyWindow.
 *  The original always draws on the root pixmap, which we don't have.
 *  Instead, draw on the parent window's pixmap.
 */
void
RootlessCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RegionRec rgnDst;
    int dx, dy;
    BoxPtr extents;
    int area;

    RL_DEBUG_MSG("copywindowFB start (win 0x%x) ", pWin);

    SCREEN_UNWRAP(pScreen, CopyWindow);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    RegionTranslate(prgnSrc, -dx, -dy);

    RegionNull(&rgnDst);
    RegionIntersect(&rgnDst, &pWin->borderClip, prgnSrc);

    extents = RegionExtents(&rgnDst);
    area = (extents->x2 - extents->x1) * (extents->y2 - extents->y1);

    /* If the area exceeds threshold, use the implementation's
       accelerated version. */
    if (area > rootless_CopyWindow_threshold &&
        SCREENREC(pScreen)->imp->CopyWindow) {
        RootlessWindowRec *winRec;
        WindowPtr top;

        top = TopLevelParent(pWin);
        if (top == NULL) {
            RL_DEBUG_MSG("no parent\n");
            goto out;
        }

        winRec = WINREC(top);
        if (winRec == NULL) {
            RL_DEBUG_MSG("not framed\n");
            goto out;
        }

        /* Move region to window local coords */
        RegionTranslate(&rgnDst, -winRec->x, -winRec->y);

        RootlessStopDrawing(pWin, FALSE);

        SCREENREC(pScreen)->imp->CopyWindow(winRec->wid,
                                            RegionNumRects(&rgnDst),
                                            RegionRects(&rgnDst), dx, dy);
    }
    else {
        RootlessStartDrawing(pWin);

        miCopyRegion((DrawablePtr) pWin, (DrawablePtr) pWin,
                     0, &rgnDst, dx, dy, fbCopyWindowProc, 0, 0);

        /* prgnSrc has been translated to dst position */
        RootlessDamageRegion(pWin, prgnSrc);
    }

 out:
    RegionUninit(&rgnDst);
    fbValidateDrawable(&pWin->drawable);

    SCREEN_WRAP(pScreen, CopyWindow);

    RL_DEBUG_MSG("copywindowFB end\n");
}

void
RootlessPaintWindow(WindowPtr pWin, RegionPtr prgn, int what)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (IsFramedWindow(pWin)) {
        RootlessStartDrawing(pWin);
        RootlessDamageRegion(pWin, prgn);

        if (pWin->backgroundState == ParentRelative) {
            if ((what == PW_BACKGROUND) ||
                (what == PW_BORDER && !pWin->borderIsPixel))
                RootlessSetPixmapOfAncestors(pWin);
        }
    }

    SCREEN_UNWRAP(pScreen, PaintWindow);
    pScreen->PaintWindow(pWin, prgn, what);
    SCREEN_WRAP(pScreen, PaintWindow);
}

/*
 * Window resize procedures
 */

enum {
    WIDTH_SMALLER = 1,
    HEIGHT_SMALLER = 2,
};

/*
 * ResizeWeighting
 *  Choose gravity to avoid local copies. Do that by looking for
 *  a corner that doesn't move _relative to the screen_.
 */
static inline unsigned int
ResizeWeighting(int oldX1, int oldY1, int oldX2, int oldY2, int oldBW,
                int newX1, int newY1, int newX2, int newY2, int newBW)
{
#ifdef ROOTLESS_RESIZE_GRAVITY
    if (newBW != oldBW)
        return RL_GRAVITY_NONE;

    if (newX1 == oldX1 && newY1 == oldY1)
        return RL_GRAVITY_NORTH_WEST;
    else if (newX1 == oldX1 && newY2 == oldY2)
        return RL_GRAVITY_SOUTH_WEST;
    else if (newX2 == oldX2 && newY2 == oldY2)
        return RL_GRAVITY_SOUTH_EAST;
    else if (newX2 == oldX2 && newY1 == oldY1)
        return RL_GRAVITY_NORTH_EAST;
    else
        return RL_GRAVITY_NONE;
#else
    return RL_GRAVITY_NONE;
#endif
}

/*
 * StartFrameResize
 *  Prepare to resize a top-level window. The old window's pixels are
 *  saved and the implementation is told to change the window size.
 *  (x,y,w,h) is outer frame of window (outside border)
 */
static Bool
StartFrameResize(WindowPtr pWin, Bool gravity,
                 int oldX, int oldY, int oldW, int oldH, int oldBW,
                 int newX, int newY, int newW, int newH, int newBW)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    Bool need_window_source = FALSE, resize_after = FALSE;

    BoxRec rect;
    int oldX2, newX2;
    int oldY2, newY2;
    unsigned int weight;

    oldX2 = oldX + oldW, newX2 = newX + newW;
    oldY2 = oldY + oldH, newY2 = newY + newH;

    /* Decide which resize weighting to use */
    weight = ResizeWeighting(oldX, oldY, oldW, oldH, oldBW,
                             newX, newY, newW, newH, newBW);

    /* Compute intersection between old and new rects */
    rect.x1 = max(oldX, newX);
    rect.y1 = max(oldY, newY);
    rect.x2 = min(oldX2, newX2);
    rect.y2 = min(oldY2, newY2);

    RL_DEBUG_MSG("RESIZE TOPLEVEL WINDOW with gravity %i ", gravity);
    RL_DEBUG_MSG("%d %d %d %d %d   %d %d %d %d %d\n",
                 oldX, oldY, oldW, oldH, oldBW, newX, newY, newW, newH, newBW);

    RootlessRedisplay(pWin);

    /* If gravity is true, then we need to have a way of recovering all
       the original bits in the window for when X rearranges the contents
       based on the various gravity settings. The obvious way is to just
       snapshot the entire backing store before resizing it, but that
       it slow on large windows.

       So the optimization here is to use the implementation's resize
       weighting options (if available) to allow us to reason about what
       is left in the backing store after the resize. We can then only
       copy what won't be there after the resize, and do a two-stage copy
       operation.

       Most of these optimizations are only applied when the top-left
       corner of the window is fixed, since that's the common case. They
       could probably be extended with some thought. */

    gResizeDeathCount = 0;

    if (gravity && weight == RL_GRAVITY_NORTH_WEST) {
        unsigned int code = 0;

        /* Top left corner is anchored. We never need to copy the
           entire window. */

        need_window_source = TRUE;

        /* These comparisons were chosen to avoid setting bits when the sizes
           are the same. (So the fastest case automatically gets taken when
           dimensions are unchanging.) */

        if (newW < oldW)
            code |= WIDTH_SMALLER;
        if (newH < oldH)
            code |= HEIGHT_SMALLER;

        if (((code ^ (code >> 1)) & 1) == 0) {
            /* Both dimensions are either getting larger, or both
               are getting smaller. No need to copy anything. */

            if (code == (WIDTH_SMALLER | HEIGHT_SMALLER)) {
                /* Since the window is getting smaller, we can do gravity
                   repair on it with its current size, then resize it
                   afterwards. */

                resize_after = TRUE;
            }

            gResizeDeathCount = 1;
        }
        else {
            unsigned int copy_rowbytes, Bpp;
            unsigned int copy_rect_width, copy_rect_height;
            BoxRec copy_rect;

            /* We can get away with a partial copy. 'rect' is the
               intersection between old and new bounds, so copy
               everything to the right of or below the intersection. */

            RootlessStartDrawing(pWin);

            if (code == WIDTH_SMALLER) {
                copy_rect.x1 = rect.x2;
                copy_rect.y1 = rect.y1;
                copy_rect.x2 = oldX2;
                copy_rect.y2 = oldY2;
            }
            else if (code == HEIGHT_SMALLER) {
                copy_rect.x1 = rect.x1;
                copy_rect.y1 = rect.y2;
                copy_rect.x2 = oldX2;
                copy_rect.y2 = oldY2;
            }
            else
                OsAbort();

            Bpp = winRec->win->drawable.bitsPerPixel / 8;
            copy_rect_width = copy_rect.x2 - copy_rect.x1;
            copy_rect_height = copy_rect.y2 - copy_rect.y1;
            copy_rowbytes = ((copy_rect_width * Bpp) + 31) & ~31;
            gResizeDeathBits = xallocarray(copy_rowbytes, copy_rect_height);

            if (copy_rect_width * copy_rect_height >
                rootless_CopyBytes_threshold &&
                SCREENREC(pScreen)->imp->CopyBytes) {
                SCREENREC(pScreen)->imp->CopyBytes(copy_rect_width * Bpp,
                                                   copy_rect_height,
                                                   ((char *) winRec->pixelData)
                                                   +
                                                   ((copy_rect.y1 -
                                                     oldY) *
                                                    winRec->bytesPerRow)
                                                   + (copy_rect.x1 -
                                                      oldX) * Bpp,
                                                   winRec->bytesPerRow,
                                                   gResizeDeathBits,
                                                   copy_rowbytes);
            }
            else {
                fbBlt((FbBits *) (winRec->pixelData
                                  +
                                  ((copy_rect.y1 - oldY) * winRec->bytesPerRow)
                                  + (copy_rect.x1 - oldX) * Bpp),
                      winRec->bytesPerRow / sizeof(FbBits), 0,
                      (FbBits *) gResizeDeathBits,
                      copy_rowbytes / sizeof(FbBits), 0, copy_rect_width * Bpp,
                      copy_rect_height, GXcopy, FB_ALLONES, Bpp, 0, 0);
            }

            gResizeDeathBounds[1] = copy_rect;
            gResizeDeathPix[1]
                = GetScratchPixmapHeader(pScreen, copy_rect_width,
                                         copy_rect_height,
                                         winRec->win->drawable.depth,
                                         winRec->win->drawable.bitsPerPixel,
                                         winRec->bytesPerRow,
                                         (void *) gResizeDeathBits);

            SetPixmapBaseToScreen(gResizeDeathPix[1],
                                  copy_rect.x1, copy_rect.y1);

            gResizeDeathCount = 2;
        }
    }
    else if (gravity) {
        /* The general case. Just copy everything. */

        RootlessStartDrawing(pWin);

        gResizeDeathBits = xallocarray(winRec->bytesPerRow, winRec->height);

        memcpy(gResizeDeathBits, winRec->pixelData,
               winRec->bytesPerRow * winRec->height);

        gResizeDeathBounds[0] = (BoxRec) {
        oldX, oldY, oldX2, oldY2};
        gResizeDeathPix[0]
            = GetScratchPixmapHeader(pScreen, winRec->width,
                                     winRec->height,
                                     winRec->win->drawable.depth,
                                     winRec->win->drawable.bitsPerPixel,
                                     winRec->bytesPerRow,
                                     (void *) gResizeDeathBits);

        SetPixmapBaseToScreen(gResizeDeathPix[0], oldX, oldY);
        gResizeDeathCount = 1;
    }

    RootlessStopDrawing(pWin, FALSE);

    winRec->x = newX;
    winRec->y = newY;
    winRec->width = newW;
    winRec->height = newH;
    winRec->borderWidth = newBW;

    /* Unless both dimensions are getting smaller, Resize the frame
       before doing gravity repair */

    if (!resize_after) {
        SCREENREC(pScreen)->imp->ResizeFrame(winRec->wid, pScreen,
                                             newX + SCREEN_TO_GLOBAL_X,
                                             newY + SCREEN_TO_GLOBAL_Y,
                                             newW, newH, weight);
    }

    RootlessStartDrawing(pWin);

    /* If necessary, create a source pixmap pointing at the current
       window bits. */

    if (need_window_source) {
        gResizeDeathBounds[0] = (BoxRec) {
        oldX, oldY, oldX2, oldY2};
        gResizeDeathPix[0]
            = GetScratchPixmapHeader(pScreen, oldW, oldH,
                                     winRec->win->drawable.depth,
                                     winRec->win->drawable.bitsPerPixel,
                                     winRec->bytesPerRow, winRec->pixelData);

        SetPixmapBaseToScreen(gResizeDeathPix[0], oldX, oldY);
    }

    /* Use custom CopyWindow when moving gravity bits around
       ResizeWindow assumes the old window contents are in the same
       pixmap, but here they're in deathPix instead. */

    if (gravity) {
        gResizeOldCopyWindowProc = pScreen->CopyWindow;
        pScreen->CopyWindow = RootlessResizeCopyWindow;
    }

    /* If we can't rely on the window server preserving the bits we
       need in the position we need, copy the pixels in the
       intersection from src to dst. ResizeWindow assumes these pixels
       are already present when making gravity adjustments. pWin
       currently has new-sized pixmap but is in old position.

       FIXME: border width change! (?) */

    if (gravity && weight == RL_GRAVITY_NONE) {
        PixmapPtr src, dst;

        assert(gResizeDeathCount == 1);

        src = gResizeDeathPix[0];
        dst = pScreen->GetWindowPixmap(pWin);

        RL_DEBUG_MSG("Resize copy rect %d %d %d %d\n",
                     rect.x1, rect.y1, rect.x2, rect.y2);

        /* rect is the intersection of the old location and new location */
        if (BOX_NOT_EMPTY(rect) && src != NULL && dst != NULL) {
            /* The window drawable still has the old frame position, which
               means that DST doesn't actually point at the origin of our
               physical backing store when adjusted by the drawable.x,y
               position. So sneakily adjust it temporarily while copying.. */

            ((PixmapPtr) dst)->devPrivate.ptr = winRec->pixelData;
            SetPixmapBaseToScreen(dst, newX, newY);

            fbCopyWindowProc(&src->drawable, &dst->drawable, NULL,
                             &rect, 1, 0, 0, FALSE, FALSE, 0, 0);

            ((PixmapPtr) dst)->devPrivate.ptr = winRec->pixelData;
            SetPixmapBaseToScreen(dst, oldX, oldY);
        }
    }

    return resize_after;
}

static void
FinishFrameResize(WindowPtr pWin, Bool gravity, int oldX, int oldY,
                  unsigned int oldW, unsigned int oldH, unsigned int oldBW,
                  int newX, int newY, unsigned int newW, unsigned int newH,
                  unsigned int newBW, Bool resize_now)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    int i;

    RootlessStopDrawing(pWin, FALSE);

    if (resize_now) {
        unsigned int weight;

        /* We didn't resize anything earlier, so do it now, now that
           we've finished gravitating the bits. */

        weight = ResizeWeighting(oldX, oldY, oldW, oldH, oldBW,
                                 newX, newY, newW, newH, newBW);

        SCREENREC(pScreen)->imp->ResizeFrame(winRec->wid, pScreen,
                                             newX + SCREEN_TO_GLOBAL_X,
                                             newY + SCREEN_TO_GLOBAL_Y,
                                             newW, newH, weight);
    }

    /* Redraw everything. FIXME: there must be times when we don't need
       to do this. Perhaps when top-left weighting and no gravity? */

    RootlessDamageRect(pWin, -newBW, -newBW, newW, newH);

    for (i = 0; i < 2; i++) {
        if (gResizeDeathPix[i] != NULL) {
            FreeScratchPixmapHeader(gResizeDeathPix[i]);
            gResizeDeathPix[i] = NULL;
        }
    }

    free(gResizeDeathBits);
    gResizeDeathBits = NULL;

    if (gravity) {
        pScreen->CopyWindow = gResizeOldCopyWindowProc;
    }
}

/*
 * RootlessMoveWindow
 *  If kind==VTOther, window border is resizing (and borderWidth is
 *  already changed!!@#$)  This case works like window resize, not move.
 */
void
RootlessMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pSib, VTKind kind)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    CopyWindowProcPtr oldCopyWindowProc = NULL;
    int oldX = 0, oldY = 0, newX = 0, newY = 0;
    unsigned int oldW = 0, oldH = 0, oldBW = 0;
    unsigned int newW = 0, newH = 0, newBW = 0;
    Bool resize_after = FALSE;
    RegionRec saveRoot;

    RL_DEBUG_MSG("movewindow start \n");

    if (winRec) {
        if (kind == VTMove) {
            oldX = winRec->x;
            oldY = winRec->y;
            RootlessRedisplay(pWin);
            RootlessStartDrawing(pWin);
        }
        else {
            RL_DEBUG_MSG("movewindow border resizing ");

            oldBW = winRec->borderWidth;
            oldX = winRec->x;
            oldY = winRec->y;
            oldW = winRec->width;
            oldH = winRec->height;

            newBW = wBorderWidth(pWin);
            newX = x;
            newY = y;
            newW = pWin->drawable.width + 2 * newBW;
            newH = pWin->drawable.height + 2 * newBW;

            resize_after = StartFrameResize(pWin, FALSE,
                                            oldX, oldY, oldW, oldH, oldBW,
                                            newX, newY, newW, newH, newBW);
        }
    }

    HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, MoveWindow);

    if (winRec) {
        oldCopyWindowProc = pScreen->CopyWindow;
        pScreen->CopyWindow = RootlessNoCopyWindow;
    }
    pScreen->MoveWindow(pWin, x, y, pSib, kind);
    if (winRec) {
        pScreen->CopyWindow = oldCopyWindowProc;
    }

    NORMAL_ROOT(pWin);
    SCREEN_WRAP(pScreen, MoveWindow);

    if (winRec) {
        if (kind == VTMove) {
            winRec->x = x;
            winRec->y = y;
            RootlessStopDrawing(pWin, FALSE);
            SCREENREC(pScreen)->imp->MoveFrame(winRec->wid, pScreen,
                                               x + SCREEN_TO_GLOBAL_X,
                                               y + SCREEN_TO_GLOBAL_Y);
        }
        else {
            FinishFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW, resize_after);
        }
    }

    RL_DEBUG_MSG("movewindow end\n");
}

/*
 * RootlessResizeWindow
 *  Note: (x, y, w, h) as passed to this procedure don't match the frame
 *  definition. (x,y) is corner of very outer edge, *outside* border.
 *  w,h is width and height *inside* border, *ignoring* border width.
 *  The rect (x, y, w, h) doesn't mean anything. (x, y, w+2*bw, h+2*bw)
 *  is total rect and (x+bw, y+bw, w, h) is inner rect.
 */
void
RootlessResizeWindow(WindowPtr pWin, int x, int y,
                     unsigned int w, unsigned int h, WindowPtr pSib)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    int oldX = 0, oldY = 0, newX = 0, newY = 0;
    unsigned int oldW = 0, oldH = 0, oldBW = 0, newW = 0, newH = 0, newBW = 0;
    Bool resize_after = FALSE;
    RegionRec saveRoot;

    RL_DEBUG_MSG("resizewindow start (win 0x%x) ", pWin);

    if (pWin->parent) {
        if (winRec) {
            oldBW = winRec->borderWidth;
            oldX = winRec->x;
            oldY = winRec->y;
            oldW = winRec->width;
            oldH = winRec->height;

            newBW = oldBW;
            newX = x;
            newY = y;
            newW = w + 2 * newBW;
            newH = h + 2 * newBW;

            resize_after = StartFrameResize(pWin, TRUE,
                                            oldX, oldY, oldW, oldH, oldBW,
                                            newX, newY, newW, newH, newBW);
        }

        HUGE_ROOT(pWin);
        SCREEN_UNWRAP(pScreen, ResizeWindow);
        pScreen->ResizeWindow(pWin, x, y, w, h, pSib);
        SCREEN_WRAP(pScreen, ResizeWindow);
        NORMAL_ROOT(pWin);

        if (winRec) {
            FinishFrameResize(pWin, TRUE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW, resize_after);
        }
    }
    else {
        /* Special case for resizing the root window */
        BoxRec box;

        pWin->drawable.x = x;
        pWin->drawable.y = y;
        pWin->drawable.width = w;
        pWin->drawable.height = h;

        box.x1 = x;
        box.y1 = y;
        box.x2 = x + w;
        box.y2 = y + h;
        RegionUninit(&pWin->winSize);
        RegionInit(&pWin->winSize, &box, 1);
        RegionCopy(&pWin->borderSize, &pWin->winSize);
        RegionCopy(&pWin->clipList, &pWin->winSize);
        RegionCopy(&pWin->borderClip, &pWin->winSize);

        if (winRec) {
            SCREENREC(pScreen)->imp->ResizeFrame(winRec->wid, pScreen,
                                                 x + SCREEN_TO_GLOBAL_X,
                                                 y + SCREEN_TO_GLOBAL_Y,
                                                 w, h, RL_GRAVITY_NONE);
        }

        miSendExposures(pWin, &pWin->borderClip,
                        pWin->drawable.x, pWin->drawable.y);
    }

    RL_DEBUG_MSG("resizewindow end\n");
}

/*
 * RootlessRepositionWindow
 *  Called by the implementation when a window needs to be repositioned to
 *  its correct location on the screen. This routine is typically needed
 *  due to changes in the underlying window system, such as a screen layout
 *  change.
 */
void
RootlessRepositionWindow(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (winRec == NULL)
        return;

    RootlessStopDrawing(pWin, FALSE);
    SCREENREC(pScreen)->imp->MoveFrame(winRec->wid, pScreen,
                                       winRec->x + SCREEN_TO_GLOBAL_X,
                                       winRec->y + SCREEN_TO_GLOBAL_Y);

    RootlessReorderWindow(pWin);
}

/*
 * RootlessReparentWindow
 *  Called after a window has been reparented. Generally windows are not
 *  framed until they are mapped. However, a window may be framed early by the
 *  implementation calling RootlessFrameForWindow. (e.g. this could be needed
 *  to attach a VRAM surface to it.) If the window is subsequently reparented
 *  by the window manager before being mapped, we need to give the frame to
 *  the new top-level window.
 */
void
RootlessReparentWindow(WindowPtr pWin, WindowPtr pPriorParent)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    WindowPtr pTopWin;

    /* Check that window is not top-level now, but used to be. */
    if (IsRoot(pWin) || IsRoot(pWin->parent)
        || IsTopLevel(pWin) || winRec == NULL) {
        goto out;
    }

    /* If the formerly top-level window has a frame, we want to give the
       frame to its new top-level parent. If we can't do that, we'll just
       have to jettison it... */

    pTopWin = TopLevelParent(pWin);
    assert(pTopWin != pWin);

    pWin->unhittable = FALSE;

    DeleteProperty(serverClient, pWin, xa_native_window_id());

    if (WINREC(pTopWin) != NULL) {
        /* We're screwed. */
        RootlessDestroyFrame(pWin, winRec);
    }
    else {
        if (!pTopWin->realized && pWin->realized) {
            SCREENREC(pScreen)->imp->UnmapFrame(winRec->wid);
        }

        /* Switch the frame record from one to the other. */

        SETWINREC(pWin, NULL);
        SETWINREC(pTopWin, winRec);

        RootlessInitializeFrame(pTopWin, winRec);
        RootlessReshapeFrame(pTopWin);

        SCREENREC(pScreen)->imp->ResizeFrame(winRec->wid, pScreen,
                                             winRec->x + SCREEN_TO_GLOBAL_X,
                                             winRec->y + SCREEN_TO_GLOBAL_Y,
                                             winRec->width, winRec->height,
                                             RL_GRAVITY_NONE);

        if (SCREENREC(pScreen)->imp->SwitchWindow) {
            SCREENREC(pScreen)->imp->SwitchWindow(winRec, pWin);
        }

        if (pTopWin->realized && !pWin->realized)
            winRec->is_reorder_pending = TRUE;
    }

 out:
    if (SCREENREC(pScreen)->ReparentWindow) {
        SCREEN_UNWRAP(pScreen, ReparentWindow);
        pScreen->ReparentWindow(pWin, pPriorParent);
        SCREEN_WRAP(pScreen, ReparentWindow);
    }
}

void
RootlessFlushWindowColormap(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (winRec == NULL)
        return;

    RootlessStopDrawing(pWin, FALSE);

    if (SCREENREC(pScreen)->imp->UpdateColormap)
        SCREENREC(pScreen)->imp->UpdateColormap(winRec->wid, pScreen);
}

/*
 * RootlessChangeBorderWidth
 *  FIXME: untested!
 *  pWin inside corner stays the same; pWin->drawable.[xy] stays the same
 *  Frame moves and resizes.
 */
void
RootlessChangeBorderWidth(WindowPtr pWin, unsigned int width)
{
    RegionRec saveRoot;
    Bool resize_after = FALSE;

    RL_DEBUG_MSG("change border width ");

    if (width != wBorderWidth(pWin)) {
        RootlessWindowRec *winRec = WINREC(pWin);
        int oldX = 0, oldY = 0, newX = 0, newY = 0;
        unsigned int oldW = 0, oldH = 0, oldBW = 0;
        unsigned int newW = 0, newH = 0, newBW = 0;

        if (winRec) {
            oldBW = winRec->borderWidth;
            oldX = winRec->x;
            oldY = winRec->y;
            oldW = winRec->width;
            oldH = winRec->height;

            newBW = width;
            newX = pWin->drawable.x - newBW;
            newY = pWin->drawable.y - newBW;
            newW = pWin->drawable.width + 2 * newBW;
            newH = pWin->drawable.height + 2 * newBW;

            resize_after = StartFrameResize(pWin, FALSE,
                                            oldX, oldY, oldW, oldH, oldBW,
                                            newX, newY, newW, newH, newBW);
        }

        HUGE_ROOT(pWin);
        SCREEN_UNWRAP(pWin->drawable.pScreen, ChangeBorderWidth);
        pWin->drawable.pScreen->ChangeBorderWidth(pWin, width);
        SCREEN_WRAP(pWin->drawable.pScreen, ChangeBorderWidth);
        NORMAL_ROOT(pWin);

        if (winRec) {
            FinishFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW, resize_after);
        }
    }

    RL_DEBUG_MSG("change border width end\n");
}

/*
 * RootlessOrderAllWindows
 * Brings all X11 windows to the top of the window stack
 * (i.e in front of Aqua windows) -- called when X11.app is given focus
 */
void
RootlessOrderAllWindows(Bool include_unhitable)
{
    int i;
    WindowPtr pWin;

    if (windows_hidden)
        return;

    RL_DEBUG_MSG("RootlessOrderAllWindows() ");
    for (i = 0; i < screenInfo.numScreens; i++) {
        if (screenInfo.screens[i] == NULL)
            continue;
        pWin = screenInfo.screens[i]->root;
        if (pWin == NULL)
            continue;

        for (pWin = pWin->firstChild; pWin != NULL; pWin = pWin->nextSib) {
            if (!pWin->realized)
                continue;
            if (RootlessEnsureFrame(pWin) == NULL)
                continue;
            if (!include_unhitable && pWin->unhittable)
                continue;
            RootlessReorderWindow(pWin);
        }
    }
    RL_DEBUG_MSG("RootlessOrderAllWindows() done");
}

void
RootlessEnableRoot(ScreenPtr pScreen)
{
    WindowPtr pRoot;

    pRoot = pScreen->root;

    RootlessEnsureFrame(pRoot);
    (*pScreen->ClearToBackground) (pRoot, 0, 0, 0, 0, TRUE);
    RootlessReorderWindow(pRoot);
}

void
RootlessDisableRoot(ScreenPtr pScreen)
{
    WindowPtr pRoot;
    RootlessWindowRec *winRec;

    pRoot = pScreen->root;
    winRec = WINREC(pRoot);

    if (NULL == winRec)
        return;

    RootlessDestroyFrame(pRoot, winRec);
    DeleteProperty(serverClient, pRoot, xa_native_window_id());
}

void
RootlessHideAllWindows(void)
{
    int i;
    ScreenPtr pScreen;
    WindowPtr pWin;
    RootlessWindowRec *winRec;

    if (windows_hidden)
        return;

    windows_hidden = TRUE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];
        if (pScreen == NULL)
            continue;
        pWin = pScreen->root;
        if (pWin == NULL)
            continue;

        for (pWin = pWin->firstChild; pWin != NULL; pWin = pWin->nextSib) {
            if (!pWin->realized)
                continue;

            RootlessStopDrawing(pWin, FALSE);

            winRec = WINREC(pWin);
            if (winRec != NULL) {
                if (SCREENREC(pScreen)->imp->HideWindow)
                    SCREENREC(pScreen)->imp->HideWindow(winRec->wid);
            }
        }
    }
}

void
RootlessShowAllWindows(void)
{
    int i;
    ScreenPtr pScreen;
    WindowPtr pWin;
    RootlessWindowRec *winRec;

    if (!windows_hidden)
        return;

    windows_hidden = FALSE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];
        if (pScreen == NULL)
            continue;
        pWin = pScreen->root;
        if (pWin == NULL)
            continue;

        for (pWin = pWin->firstChild; pWin != NULL; pWin = pWin->nextSib) {
            if (!pWin->realized)
                continue;

            winRec = RootlessEnsureFrame(pWin);
            if (winRec == NULL)
                continue;

            RootlessReorderWindow(pWin);
        }

        RootlessScreenExpose(pScreen);
    }
}

/*
 * SetPixmapOfAncestors
 *  Set the Pixmaps on all ParentRelative windows up the ancestor chain.
 */
void
RootlessSetPixmapOfAncestors(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr topWin = TopLevelParent(pWin);
    RootlessWindowRec *topWinRec = WINREC(topWin);

    while (pWin->backgroundState == ParentRelative) {
        if (pWin == topWin) {
            // disallow ParentRelative background state on top level
            XID pixel = 0;

            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
            RL_DEBUG_MSG("Cleared ParentRelative on 0x%x.\n", pWin);
            break;
        }

        pWin = pWin->parent;
        pScreen->SetWindowPixmap(pWin, topWinRec->pixmap);
    }
}
