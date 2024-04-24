/*
 * Screen routines for generic rootless X server
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
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

#include "mi.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "mivalidate.h"
#include "picturestr.h"
#include "colormapst.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "rootlessCommon.h"
#include "rootlessWindow.h"

extern int RootlessMiValidateTree(WindowPtr pRoot, WindowPtr pChild,
                                  VTKind kind);
extern Bool RootlessCreateGC(GCPtr pGC);

// Initialize globals
DevPrivateKeyRec rootlessGCPrivateKeyRec;
DevPrivateKeyRec rootlessScreenPrivateKeyRec;
DevPrivateKeyRec rootlessWindowPrivateKeyRec;
DevPrivateKeyRec rootlessWindowOldPixmapPrivateKeyRec;

/*
 * RootlessUpdateScreenPixmap
 *  miCreateScreenResources does not like a null framebuffer pointer,
 *  it leaves the screen pixmap with an uninitialized data pointer.
 *  Thus, rootless implementations typically set the framebuffer width
 *  to zero so that miCreateScreenResources does not allocate a screen
 *  pixmap for us. We allocate our own screen pixmap here since we need
 *  the screen pixmap to be valid (e.g. CopyArea from the root window).
 */
void
RootlessUpdateScreenPixmap(ScreenPtr pScreen)
{
    RootlessScreenRec *s = SCREENREC(pScreen);
    PixmapPtr pPix;
    unsigned int rowbytes;

    pPix = (*pScreen->GetScreenPixmap) (pScreen);
    if (pPix == NULL) {
        pPix = (*pScreen->CreatePixmap) (pScreen, 0, 0, pScreen->rootDepth, 0);
        (*pScreen->SetScreenPixmap) (pPix);
    }

    rowbytes = PixmapBytePad(pScreen->width, pScreen->rootDepth);

    if (s->pixmap_data_size < rowbytes) {
        free(s->pixmap_data);

        s->pixmap_data_size = rowbytes;
        s->pixmap_data = malloc(s->pixmap_data_size);
        if (s->pixmap_data == NULL)
            return;

        memset(s->pixmap_data, 0xFF, s->pixmap_data_size);

        pScreen->ModifyPixmapHeader(pPix, pScreen->width, pScreen->height,
                                    pScreen->rootDepth,
                                    BitsPerPixel(pScreen->rootDepth),
                                    0, s->pixmap_data);
        /* ModifyPixmapHeader ignores zero arguments, so install rowbytes
           by hand. */
        pPix->devKind = 0;
    }
}

/*
 * RootlessCreateScreenResources
 *  Rootless implementations typically set a null framebuffer pointer, which
 *  causes problems with miCreateScreenResources. We fix things up here.
 */
static Bool
RootlessCreateScreenResources(ScreenPtr pScreen)
{
    Bool ret = TRUE;

    SCREEN_UNWRAP(pScreen, CreateScreenResources);

    if (pScreen->CreateScreenResources != NULL)
        ret = (*pScreen->CreateScreenResources) (pScreen);

    SCREEN_WRAP(pScreen, CreateScreenResources);

    if (!ret)
        return ret;

    /* Make sure we have a valid screen pixmap. */

    RootlessUpdateScreenPixmap(pScreen);

    return ret;
}

static Bool
RootlessCloseScreen(ScreenPtr pScreen)
{
    RootlessScreenRec *s;

    s = SCREENREC(pScreen);

    // fixme unwrap everything that was wrapped?
    pScreen->CloseScreen = s->CloseScreen;

    if (s->pixmap_data != NULL) {
        free(s->pixmap_data);
        s->pixmap_data = NULL;
        s->pixmap_data_size = 0;
    }

    free(s);
    return pScreen->CloseScreen(pScreen);
}

static void
RootlessGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h,
                 unsigned int format, unsigned long planeMask, char *pdstLine)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    SCREEN_UNWRAP(pScreen, GetImage);

    if (pDrawable->type == DRAWABLE_WINDOW) {
        int x0, y0, x1, y1;
        RootlessWindowRec *winRec;

        // Many apps use GetImage to sync with the visible frame buffer
        // FIXME: entire screen or just window or all screens?
        RootlessRedisplayScreen(pScreen);

        // RedisplayScreen stops drawing, so we need to start it again
        RootlessStartDrawing((WindowPtr) pDrawable);

        /* Check that we have some place to read from. */
        winRec = WINREC(TopLevelParent((WindowPtr) pDrawable));
        if (winRec == NULL)
            goto out;

        /* Clip to top-level window bounds. */
        /* FIXME: fbGetImage uses the width parameter to calculate the
           stride of the destination pixmap. If w is clipped, the data
           returned will be garbage, although we will not crash. */

        x0 = pDrawable->x + sx;
        y0 = pDrawable->y + sy;
        x1 = x0 + w;
        y1 = y0 + h;

        x0 = max(x0, winRec->x);
        y0 = max(y0, winRec->y);
        x1 = min(x1, winRec->x + winRec->width);
        y1 = min(y1, winRec->y + winRec->height);

        sx = x0 - pDrawable->x;
        sy = y0 - pDrawable->y;
        w = x1 - x0;
        h = y1 - y0;

        if (w <= 0 || h <= 0)
            goto out;
    }

    pScreen->GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);

 out:
    SCREEN_WRAP(pScreen, GetImage);
}

/*
 * RootlessSourceValidate
 *  CopyArea and CopyPlane use a GC tied to the destination drawable.
 *  StartDrawing/StopDrawing wrappers won't be called if source is
 *  a visible window but the destination isn't. So, we call StartDrawing
 *  here and leave StopDrawing for the block handler.
 */
static void
RootlessSourceValidate(DrawablePtr pDrawable, int x, int y, int w, int h,
                       unsigned int subWindowMode)
{
    SCREEN_UNWRAP(pDrawable->pScreen, SourceValidate);
    if (pDrawable->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr) pDrawable;

        RootlessStartDrawing(pWin);
    }
    pDrawable->pScreen->SourceValidate(pDrawable, x, y, w, h,
                                       subWindowMode);
    SCREEN_WRAP(pDrawable->pScreen, SourceValidate);
}

static void
RootlessComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                  INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask,
                  INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    WindowPtr srcWin, dstWin, maskWin = NULL;

    if (pMask) {                // pMask can be NULL
        maskWin = (pMask->pDrawable &&
                   pMask->pDrawable->type ==
                   DRAWABLE_WINDOW) ? (WindowPtr) pMask->pDrawable : NULL;
    }
    srcWin = (pSrc->pDrawable && pSrc->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr) pSrc->pDrawable : NULL;
    dstWin = (pDst->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr) pDst->pDrawable : NULL;

    // SCREEN_UNWRAP(ps, Composite);
    ps->Composite = SCREENREC(pScreen)->Composite;

    if (srcWin && IsFramedWindow(srcWin))
        RootlessStartDrawing(srcWin);
    if (maskWin && IsFramedWindow(maskWin))
        RootlessStartDrawing(maskWin);
    if (dstWin && IsFramedWindow(dstWin))
        RootlessStartDrawing(dstWin);

    ps->Composite(op, pSrc, pMask, pDst,
                  xSrc, ySrc, xMask, yMask, xDst, yDst, width, height);

    if (dstWin && IsFramedWindow(dstWin)) {
        RootlessDamageRect(dstWin, xDst, yDst, width, height);
    }

    ps->Composite = RootlessComposite;
    // SCREEN_WRAP(ps, Composite);
}

static void
RootlessGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
               PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
               int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    int x, y;
    int n;
    GlyphPtr glyph;
    WindowPtr srcWin, dstWin;

    srcWin = (pSrc->pDrawable && pSrc->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr) pSrc->pDrawable : NULL;
    dstWin = (pDst->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr) pDst->pDrawable : NULL;

    if (srcWin && IsFramedWindow(srcWin))
        RootlessStartDrawing(srcWin);
    if (dstWin && IsFramedWindow(dstWin))
        RootlessStartDrawing(dstWin);

    //SCREEN_UNWRAP(ps, Glyphs);
    ps->Glyphs = SCREENREC(pScreen)->Glyphs;
    ps->Glyphs(op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
    ps->Glyphs = RootlessGlyphs;
    //SCREEN_WRAP(ps, Glyphs);

    if (dstWin && IsFramedWindow(dstWin)) {
        x = xSrc;
        y = ySrc;

        while (nlist--) {
            x += list->xOff;
            y += list->yOff;
            n = list->len;

            /* Calling DamageRect for the bounding box of each glyph is
               inefficient. So compute the union of all glyphs in a list
               and damage that. */

            if (n > 0) {
                BoxRec box;

                glyph = *glyphs++;

                box.x1 = x - glyph->info.x;
                box.y1 = y - glyph->info.y;
                box.x2 = box.x1 + glyph->info.width;
                box.y2 = box.y1 + glyph->info.height;

                x += glyph->info.xOff;
                y += glyph->info.yOff;

                while (--n > 0) {
                    short x1, y1, x2, y2;

                    glyph = *glyphs++;

                    x1 = x - glyph->info.x;
                    y1 = y - glyph->info.y;
                    x2 = x1 + glyph->info.width;
                    y2 = y1 + glyph->info.height;

                    box.x1 = max(box.x1, x1);
                    box.y1 = max(box.y1, y1);
                    box.x2 = max(box.x2, x2);
                    box.y2 = max(box.y2, y2);

                    x += glyph->info.xOff;
                    y += glyph->info.yOff;
                }

                RootlessDamageBox(dstWin, &box);
            }
            list++;
        }
    }
}

/*
 * RootlessValidateTree
 *  ValidateTree is modified in two ways:
 *   - top-level windows don't clip each other
 *   - windows aren't clipped against root.
 *  These only matter when validating from the root.
 */
static int
RootlessValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
    int result;
    RegionRec saveRoot;
    ScreenPtr pScreen = pParent->drawable.pScreen;

    SCREEN_UNWRAP(pScreen, ValidateTree);
    RL_DEBUG_MSG("VALIDATETREE start ");

    // Use our custom version to validate from root
    if (IsRoot(pParent)) {
        RL_DEBUG_MSG("custom ");
        result = RootlessMiValidateTree(pParent, pChild, kind);
    }
    else {
        HUGE_ROOT(pParent);
        result = pScreen->ValidateTree(pParent, pChild, kind);
        NORMAL_ROOT(pParent);
    }

    SCREEN_WRAP(pScreen, ValidateTree);
    RL_DEBUG_MSG("VALIDATETREE end\n");

    return result;
}

/*
 * RootlessMarkOverlappedWindows
 *  MarkOverlappedWindows is modified to ignore overlapping
 *  top-level windows.
 */
static Bool
RootlessMarkOverlappedWindows(WindowPtr pWin, WindowPtr pFirst,
                              WindowPtr *ppLayerWin)
{
    RegionRec saveRoot;
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    SCREEN_UNWRAP(pScreen, MarkOverlappedWindows);
    RL_DEBUG_MSG("MARKOVERLAPPEDWINDOWS start ");

    HUGE_ROOT(pWin);
    if (IsRoot(pWin)) {
        // root - mark nothing
        RL_DEBUG_MSG("is root not marking ");
        result = FALSE;
    }
    else if (!IsTopLevel(pWin)) {
        // not top-level window - mark normally
        result = pScreen->MarkOverlappedWindows(pWin, pFirst, ppLayerWin);
    }
    else {
        //top-level window - mark children ONLY - NO overlaps with sibs (?)
        // This code copied from miMarkOverlappedWindows()

        register WindowPtr pChild;
        Bool anyMarked = FALSE;
        MarkWindowProcPtr MarkWindow = pScreen->MarkWindow;

        RL_DEBUG_MSG("is top level! ");
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
        }
        if (anyMarked)
            (*MarkWindow) (pWin->parent);
        result = anyMarked;
    }
    NORMAL_ROOT(pWin);
    SCREEN_WRAP(pScreen, MarkOverlappedWindows);
    RL_DEBUG_MSG("MARKOVERLAPPEDWINDOWS end\n");

    return result;
}

static void
expose_1(WindowPtr pWin)
{
    WindowPtr pChild;

    if (!pWin->realized)
        return;

    pWin->drawable.pScreen->PaintWindow(pWin, &pWin->borderClip, PW_BACKGROUND);

    /* FIXME: comments in windowstr.h indicate that borderClip doesn't
       include subwindow visibility. But I'm not so sure.. so we may
       be exposing too much.. */

    miSendExposures(pWin, &pWin->borderClip,
                    pWin->drawable.x, pWin->drawable.y);

    for (pChild = pWin->firstChild; pChild != NULL; pChild = pChild->nextSib)
        expose_1(pChild);
}

void
RootlessScreenExpose(ScreenPtr pScreen)
{
    expose_1(pScreen->root);
}

ColormapPtr
RootlessGetColormap(ScreenPtr pScreen)
{
    RootlessScreenRec *s = SCREENREC(pScreen);

    return s->colormap;
}

static void
RootlessInstallColormap(ColormapPtr pMap)
{
    ScreenPtr pScreen = pMap->pScreen;
    RootlessScreenRec *s = SCREENREC(pScreen);

    SCREEN_UNWRAP(pScreen, InstallColormap);

    if (s->colormap != pMap) {
        s->colormap = pMap;
        s->colormap_changed = TRUE;
        RootlessQueueRedisplay(pScreen);
    }

    pScreen->InstallColormap(pMap);

    SCREEN_WRAP(pScreen, InstallColormap);
}

static void
RootlessUninstallColormap(ColormapPtr pMap)
{
    ScreenPtr pScreen = pMap->pScreen;
    RootlessScreenRec *s = SCREENREC(pScreen);

    SCREEN_UNWRAP(pScreen, UninstallColormap);

    if (s->colormap == pMap)
        s->colormap = NULL;

    pScreen->UninstallColormap(pMap);

    SCREEN_WRAP(pScreen, UninstallColormap);
}

static void
RootlessStoreColors(ColormapPtr pMap, int ndef, xColorItem * pdef)
{
    ScreenPtr pScreen = pMap->pScreen;
    RootlessScreenRec *s = SCREENREC(pScreen);

    SCREEN_UNWRAP(pScreen, StoreColors);

    if (s->colormap == pMap && ndef > 0) {
        s->colormap_changed = TRUE;
        RootlessQueueRedisplay(pScreen);
    }

    pScreen->StoreColors(pMap, ndef, pdef);

    SCREEN_WRAP(pScreen, StoreColors);
}

static CARD32
RootlessRedisplayCallback(OsTimerPtr timer, CARD32 time, void *arg)
{
    RootlessScreenRec *screenRec = arg;

    if (!screenRec->redisplay_queued) {
        /* No update needed. Stop the timer. */

        screenRec->redisplay_timer_set = FALSE;
        return 0;
    }

    screenRec->redisplay_queued = FALSE;

    /* Mark that we should redisplay before waiting for I/O next time */
    screenRec->redisplay_expired = TRUE;

    /* Reinstall the timer immediately, so we get as close to our
       redisplay interval as possible. */

    return ROOTLESS_REDISPLAY_DELAY;
}

/*
 * RootlessQueueRedisplay
 *  Queue a redisplay after a timer delay to ensure we do not redisplay
 *  too frequently.
 */
void
RootlessQueueRedisplay(ScreenPtr pScreen)
{
    RootlessScreenRec *screenRec = SCREENREC(pScreen);

    screenRec->redisplay_queued = TRUE;

    if (screenRec->redisplay_timer_set)
        return;

    screenRec->redisplay_timer = TimerSet(screenRec->redisplay_timer,
                                          0, ROOTLESS_REDISPLAY_DELAY,
                                          RootlessRedisplayCallback, screenRec);
    screenRec->redisplay_timer_set = TRUE;
}

/*
 * RootlessBlockHandler
 *  If the redisplay timer has expired, flush drawing before blocking
 *  on select().
 */
static void
RootlessBlockHandler(void *pbdata, void *ptimeout)
{
    ScreenPtr pScreen = pbdata;
    RootlessScreenRec *screenRec = SCREENREC(pScreen);

    if (screenRec->redisplay_expired) {
        screenRec->redisplay_expired = FALSE;

        RootlessRedisplayScreen(pScreen);
    }
}

static void
RootlessWakeupHandler(void *data, int result)
{
    // nothing here
}

static Bool
RootlessAllocatePrivates(ScreenPtr pScreen)
{
    RootlessScreenRec *s;

    if (!dixRegisterPrivateKey
        (&rootlessGCPrivateKeyRec, PRIVATE_GC, sizeof(RootlessGCRec)))
        return FALSE;
    if (!dixRegisterPrivateKey(&rootlessScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&rootlessWindowPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;
    if (!dixRegisterPrivateKey
        (&rootlessWindowOldPixmapPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;

    s = malloc(sizeof(RootlessScreenRec));
    if (!s)
        return FALSE;
    SETSCREENREC(pScreen, s);

    s->pixmap_data = NULL;
    s->pixmap_data_size = 0;

    s->redisplay_timer = NULL;
    s->redisplay_timer_set = FALSE;

    return TRUE;
}

static void
RootlessWrap(ScreenPtr pScreen)
{
    RootlessScreenRec *s = SCREENREC(pScreen);

#define WRAP(a) \
    if (pScreen->a) { \
        s->a = pScreen->a; \
    } else { \
        RL_DEBUG_MSG("null screen fn " #a "\n"); \
        s->a = NULL; \
    } \
    pScreen->a = Rootless##a

    WRAP(CreateScreenResources);
    WRAP(CloseScreen);
    WRAP(CreateGC);
    WRAP(CopyWindow);
    WRAP(PaintWindow);
    WRAP(GetImage);
    WRAP(SourceValidate);
    WRAP(CreateWindow);
    WRAP(DestroyWindow);
    WRAP(RealizeWindow);
    WRAP(UnrealizeWindow);
    WRAP(MoveWindow);
    WRAP(PositionWindow);
    WRAP(ResizeWindow);
    WRAP(RestackWindow);
    WRAP(ReparentWindow);
    WRAP(ChangeBorderWidth);
    WRAP(MarkOverlappedWindows);
    WRAP(ValidateTree);
    WRAP(ChangeWindowAttributes);
    WRAP(InstallColormap);
    WRAP(UninstallColormap);
    WRAP(StoreColors);

    WRAP(SetShape);

    {
        // Composite and Glyphs don't use normal screen wrapping
        PictureScreenPtr ps = GetPictureScreen(pScreen);

        s->Composite = ps->Composite;
        ps->Composite = RootlessComposite;
        s->Glyphs = ps->Glyphs;
        ps->Glyphs = RootlessGlyphs;
    }

    // WRAP(ClearToBackground); fixme put this back? useful for shaped wins?

#undef WRAP
}

/*
 * RootlessInit
 *  Called by the rootless implementation to initialize the rootless layer.
 *  Rootless wraps lots of stuff and needs a bunch of devPrivates.
 */
Bool
RootlessInit(ScreenPtr pScreen, RootlessFrameProcsPtr procs)
{
    RootlessScreenRec *s;

    if (!RootlessAllocatePrivates(pScreen))
        return FALSE;

    s = SCREENREC(pScreen);

    s->imp = procs;
    s->colormap = NULL;
    s->redisplay_expired = FALSE;

    RootlessWrap(pScreen);

    if (!RegisterBlockAndWakeupHandlers(RootlessBlockHandler,
                                        RootlessWakeupHandler,
                                        (void *) pScreen)) {
        return FALSE;
    }

    return TRUE;
}

void
RootlessUpdateRooted(Bool state)
{
    int i;

    if (!state) {
        for (i = 0; i < screenInfo.numScreens; i++)
            RootlessDisableRoot(screenInfo.screens[i]);
    }
    else {
        for (i = 0; i < screenInfo.numScreens; i++)
            RootlessEnableRoot(screenInfo.screens[i]);
    }
}
