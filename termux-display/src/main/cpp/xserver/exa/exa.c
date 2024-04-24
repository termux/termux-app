/*
 * Copyright © 2001 Keith Packard
 *
 * Partly based on code that is Copyright © The XFree86 Project Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/** @file
 * This file covers the initialization and teardown of EXA, and has various
 * functions not responsible for performing rendering, pixmap migration, or
 * memory management.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "exa_priv.h"
#include "exa.h"

DevPrivateKeyRec exaScreenPrivateKeyRec;

#ifdef MITSHM
static ShmFuncs exaShmFuncs = { NULL, NULL };
#endif

/**
 * exaGetPixmapOffset() returns the offset (in bytes) within the framebuffer of
 * the beginning of the given pixmap.
 *
 * Note that drivers are free to, and often do, munge this offset as necessary
 * for handing to the hardware -- for example, translating it into a different
 * aperture.  This function may need to be extended in the future if we grow
 * support for having multiple card-accessible offscreen, such as an AGP memory
 * pool alongside the framebuffer pool.
 */
unsigned long
exaGetPixmapOffset(PixmapPtr pPix)
{
    ExaScreenPriv(pPix->drawable.pScreen);
    ExaPixmapPriv(pPix);

    return (CARD8 *) pExaPixmap->fb_ptr - pExaScr->info->memoryBase;
}

void *
exaGetPixmapDriverPrivate(PixmapPtr pPix)
{
    ExaPixmapPriv(pPix);

    return pExaPixmap->driverPriv;
}

/**
 * exaGetPixmapPitch() returns the pitch (in bytes) of the given pixmap.
 *
 * This is a helper to make driver code more obvious, due to the rather obscure
 * naming of the pitch field in the pixmap.
 */
unsigned long
exaGetPixmapPitch(PixmapPtr pPix)
{
    return pPix->devKind;
}

/**
 * exaGetPixmapSize() returns the size in bytes of the given pixmap in video
 * memory. Only valid when the pixmap is currently in framebuffer.
 */
unsigned long
exaGetPixmapSize(PixmapPtr pPix)
{
    ExaPixmapPrivPtr pExaPixmap;

    pExaPixmap = ExaGetPixmapPriv(pPix);
    if (pExaPixmap != NULL)
        return pExaPixmap->fb_size;
    return 0;
}

/**
 * exaGetDrawablePixmap() returns a backing pixmap for a given drawable.
 *
 * @param pDrawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.  Note that
 * coordinate translation is needed when drawing to the backing pixmap of a
 * redirected window, and the translation coordinates are provided by calling
 * exaGetOffscreenPixmap() on the drawable.
 */
PixmapPtr
exaGetDrawablePixmap(DrawablePtr pDrawable)
{
    if (pDrawable->type == DRAWABLE_WINDOW)
        return pDrawable->pScreen->GetWindowPixmap((WindowPtr) pDrawable);
    else
        return (PixmapPtr) pDrawable;
}

/**
 * Sets the offsets to add to coordinates to make them address the same bits in
 * the backing drawable. These coordinates are nonzero only for redirected
 * windows.
 */
void
exaGetDrawableDeltas(DrawablePtr pDrawable, PixmapPtr pPixmap, int *xp, int *yp)
{
#ifdef COMPOSITE
    if (pDrawable->type == DRAWABLE_WINDOW) {
        *xp = -pPixmap->screen_x;
        *yp = -pPixmap->screen_y;
        return;
    }
#endif

    *xp = 0;
    *yp = 0;
}

/**
 * exaPixmapDirty() marks a pixmap as dirty, allowing for
 * optimizations in pixmap migration when no changes have occurred.
 */
void
exaPixmapDirty(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    BoxRec box;
    RegionRec region;

    box.x1 = max(x1, 0);
    box.y1 = max(y1, 0);
    box.x2 = min(x2, pPix->drawable.width);
    box.y2 = min(y2, pPix->drawable.height);

    if (box.x1 >= box.x2 || box.y1 >= box.y2)
        return;

    RegionInit(&region, &box, 1);
    DamageDamageRegion(&pPix->drawable, &region);
    RegionUninit(&region);
}

static int
exaLog2(int val)
{
    int bits;

    if (val <= 0)
        return 0;
    for (bits = 0; val != 0; bits++)
        val >>= 1;
    return bits - 1;
}

void
exaSetAccelBlock(ExaScreenPrivPtr pExaScr, ExaPixmapPrivPtr pExaPixmap,
                 int w, int h, int bpp)
{
    pExaPixmap->accel_blocked = 0;

    if (pExaScr->info->maxPitchPixels) {
        int max_pitch = pExaScr->info->maxPitchPixels * bits_to_bytes(bpp);

        if (pExaPixmap->fb_pitch > max_pitch)
            pExaPixmap->accel_blocked |= EXA_RANGE_PITCH;
    }

    if (pExaScr->info->maxPitchBytes &&
        pExaPixmap->fb_pitch > pExaScr->info->maxPitchBytes)
        pExaPixmap->accel_blocked |= EXA_RANGE_PITCH;

    if (w > pExaScr->info->maxX)
        pExaPixmap->accel_blocked |= EXA_RANGE_WIDTH;

    if (h > pExaScr->info->maxY)
        pExaPixmap->accel_blocked |= EXA_RANGE_HEIGHT;
}

void
exaSetFbPitch(ExaScreenPrivPtr pExaScr, ExaPixmapPrivPtr pExaPixmap,
              int w, int h, int bpp)
{
    if (pExaScr->info->flags & EXA_OFFSCREEN_ALIGN_POT && w != 1)
        pExaPixmap->fb_pitch = bits_to_bytes((1 << (exaLog2(w - 1) + 1)) * bpp);
    else
        pExaPixmap->fb_pitch = bits_to_bytes(w * bpp);

    pExaPixmap->fb_pitch = EXA_ALIGN(pExaPixmap->fb_pitch,
                                     pExaScr->info->pixmapPitchAlign);
}

/**
 * Returns TRUE if the pixmap is not movable.  This is the case where it's a
 * pixmap which has no private (almost always bad) or it's a scratch pixmap created by
 * some X Server internal component (the score says it's pinned).
 */
Bool
exaPixmapIsPinned(PixmapPtr pPix)
{
    ExaPixmapPriv(pPix);

    if (pExaPixmap == NULL)
        EXA_FatalErrorDebugWithRet(("EXA bug: exaPixmapIsPinned was called on a non-exa pixmap.\n"), TRUE);

    return pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED;
}

/**
 * exaPixmapHasGpuCopy() is used to determine if a pixmap is in offscreen
 * memory, meaning that acceleration could probably be done to it, and that it
 * will need to be wrapped by PrepareAccess()/FinishAccess() when accessing it
 * with the CPU.
 *
 * Note that except for UploadToScreen()/DownloadFromScreen() (which explicitly
 * deal with moving pixmaps in and out of system memory), EXA will give drivers
 * pixmaps as arguments for which exaPixmapHasGpuCopy() is TRUE.
 *
 * @return TRUE if the given drawable is in framebuffer memory.
 */
Bool
exaPixmapHasGpuCopy(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return FALSE;

    return (*pExaScr->pixmap_has_gpu_copy) (pPixmap);
}

/**
 * exaDrawableIsOffscreen() is a convenience wrapper for exaPixmapHasGpuCopy().
 */
Bool
exaDrawableIsOffscreen(DrawablePtr pDrawable)
{
    return exaPixmapHasGpuCopy(exaGetDrawablePixmap(pDrawable));
}

/**
 * Returns the pixmap which backs a drawable, and the offsets to add to
 * coordinates to make them address the same bits in the backing drawable.
 */
PixmapPtr
exaGetOffscreenPixmap(DrawablePtr pDrawable, int *xp, int *yp)
{
    PixmapPtr pPixmap = exaGetDrawablePixmap(pDrawable);

    exaGetDrawableDeltas(pDrawable, pPixmap, xp, yp);

    if (exaPixmapHasGpuCopy(pPixmap))
        return pPixmap;
    else
        return NULL;
}

/**
 * Returns TRUE if the pixmap GPU copy is being accessed.
 */
Bool
ExaDoPrepareAccess(PixmapPtr pPixmap, int index)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    ExaPixmapPriv(pPixmap);
    Bool has_gpu_copy, ret;
    int i;

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return FALSE;

    if (pExaPixmap == NULL)
        EXA_FatalErrorDebugWithRet(("EXA bug: ExaDoPrepareAccess was called on a non-exa pixmap.\n"), FALSE);

    /* Handle repeated / nested calls. */
    for (i = 0; i < EXA_NUM_PREPARE_INDICES; i++) {
        if (pExaScr->access[i].pixmap == pPixmap) {
            pExaScr->access[i].count++;
            return pExaScr->access[i].retval;
        }
    }

    /* If slot for this index is taken, find an empty slot */
    if (pExaScr->access[index].pixmap) {
        for (index = EXA_NUM_PREPARE_INDICES - 1; index >= 0; index--)
            if (!pExaScr->access[index].pixmap)
                break;
    }

    /* Access to this pixmap hasn't been prepared yet, so data pointer should be NULL. */
    if (pPixmap->devPrivate.ptr != NULL) {
        EXA_FatalErrorDebug(("EXA bug: pPixmap->devPrivate.ptr was %p, but should have been NULL.\n", pPixmap->devPrivate.ptr));
    }

    has_gpu_copy = exaPixmapHasGpuCopy(pPixmap);

    if (has_gpu_copy && pExaPixmap->fb_ptr) {
        pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
        ret = TRUE;
    }
    else {
        pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
        ret = FALSE;
    }

    /* Store so we can handle repeated / nested calls. */
    pExaScr->access[index].pixmap = pPixmap;
    pExaScr->access[index].count = 1;

    if (!has_gpu_copy)
        goto out;

    exaWaitSync(pScreen);

    if (pExaScr->info->PrepareAccess == NULL)
        goto out;

    if (index >= EXA_PREPARE_AUX_DEST &&
        !(pExaScr->info->flags & EXA_SUPPORTS_PREPARE_AUX)) {
        if (pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED)
            FatalError("Unsupported AUX indices used on a pinned pixmap.\n");
        exaMoveOutPixmap(pPixmap);
        ret = FALSE;
        goto out;
    }

    if (!(*pExaScr->info->PrepareAccess) (pPixmap, index)) {
        if (pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED &&
            !(pExaScr->info->flags & EXA_MIXED_PIXMAPS))
            FatalError("Driver failed PrepareAccess on a pinned pixmap.\n");
        exaMoveOutPixmap(pPixmap);
        ret = FALSE;
        goto out;
    }

    ret = TRUE;

 out:
    pExaScr->access[index].retval = ret;
    return ret;
}

/**
 * exaPrepareAccess() is EXA's wrapper for the driver's PrepareAccess() handler.
 *
 * It deals with waiting for synchronization with the card, determining if
 * PrepareAccess() is necessary, and working around PrepareAccess() failure.
 */
void
exaPrepareAccess(DrawablePtr pDrawable, int index)
{
    PixmapPtr pPixmap = exaGetDrawablePixmap(pDrawable);

    ExaScreenPriv(pDrawable->pScreen);

    if (pExaScr->prepare_access_reg)
        pExaScr->prepare_access_reg(pPixmap, index, NULL);
    else
        (void) ExaDoPrepareAccess(pPixmap, index);
}

/**
 * exaFinishAccess() is EXA's wrapper for the driver's FinishAccess() handler.
 *
 * It deals with calling the driver's FinishAccess() only if necessary.
 */
void
exaFinishAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    ExaScreenPriv(pScreen);
    PixmapPtr pPixmap = exaGetDrawablePixmap(pDrawable);

    ExaPixmapPriv(pPixmap);
    int i;

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return;

    if (pExaPixmap == NULL)
        EXA_FatalErrorDebugWithRet(("EXA bug: exaFinishAccesss was called on a non-exa pixmap.\n"),);

    /* Handle repeated / nested calls. */
    for (i = 0; i < EXA_NUM_PREPARE_INDICES; i++) {
        if (pExaScr->access[i].pixmap == pPixmap) {
            if (--pExaScr->access[i].count > 0)
                return;
            break;
        }
    }

    /* Catch unbalanced Prepare/FinishAccess calls. */
    if (i == EXA_NUM_PREPARE_INDICES)
        EXA_FatalErrorDebugWithRet(("EXA bug: FinishAccess called without PrepareAccess for pixmap 0x%p.\n", pPixmap),);

    pExaScr->access[i].pixmap = NULL;

    /* We always hide the devPrivate.ptr. */
    pPixmap->devPrivate.ptr = NULL;

    /* Only call FinishAccess if PrepareAccess was called and succeeded. */
    if (!pExaScr->info->FinishAccess || !pExaScr->access[i].retval)
        return;

    if (i >= EXA_PREPARE_AUX_DEST &&
        !(pExaScr->info->flags & EXA_SUPPORTS_PREPARE_AUX)) {
        ErrorF("EXA bug: Trying to call driver FinishAccess hook with "
               "unsupported index EXA_PREPARE_AUX*\n");
        return;
    }

    (*pExaScr->info->FinishAccess) (pPixmap, i);
}

/**
 * Helper for things common to all schemes when a pixmap is destroyed
 */
void
exaDestroyPixmap(PixmapPtr pPixmap)
{
    ExaScreenPriv(pPixmap->drawable.pScreen);
    int i;

    /* Finish access if it was prepared (e.g. pixmap created during
     * software fallback)
     */
    for (i = 0; i < EXA_NUM_PREPARE_INDICES; i++) {
        if (pExaScr->access[i].pixmap == pPixmap) {
            exaFinishAccess(&pPixmap->drawable, i);
            pExaScr->access[i].pixmap = NULL;
            break;
        }
    }
}

/**
 * Here begins EXA's GC code.
 * Do not ever access the fb/mi layer directly.
 */

static void
 exaValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);

static void
 exaDestroyGC(GCPtr pGC);

static void
 exaChangeGC(GCPtr pGC, unsigned long mask);

static void
 exaCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);

static void
 exaChangeClip(GCPtr pGC, int type, void *pvalue, int nrects);

static void
 exaCopyClip(GCPtr pGCDst, GCPtr pGCSrc);

static void
 exaDestroyClip(GCPtr pGC);

const GCFuncs exaGCFuncs = {
    exaValidateGC,
    exaChangeGC,
    exaCopyGC,
    exaDestroyGC,
    exaChangeClip,
    exaDestroyClip,
    exaCopyClip
};

static void
exaValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    /* fbValidateGC will do direct access to pixmaps if the tiling has changed.
     * Do a few smart things so fbValidateGC can do its work.
     */

    ScreenPtr pScreen = pDrawable->pScreen;

    ExaScreenPriv(pScreen);
    ExaGCPriv(pGC);
    PixmapPtr pTile = NULL;

    /* Either of these conditions is enough to trigger access to a tile pixmap.
     * With pGC->tileIsPixel == 1, you run the risk of dereferencing an invalid
     * tile pixmap pointer.
     */
    if (pGC->fillStyle == FillTiled ||
        ((changes & GCTile) && !pGC->tileIsPixel)) {
        pTile = pGC->tile.pixmap;
    }

    if (pGC->stipple)
        exaPrepareAccess(&pGC->stipple->drawable, EXA_PREPARE_MASK);
    if (pTile)
        exaPrepareAccess(&pTile->drawable, EXA_PREPARE_SRC);

    /* Calls to Create/DestroyPixmap have to be identified as special. */
    pExaScr->fallback_counter++;
    swap(pExaGC, pGC, funcs);
    (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
    swap(pExaGC, pGC, funcs);
    pExaScr->fallback_counter--;

    if (pTile)
        exaFinishAccess(&pTile->drawable, EXA_PREPARE_SRC);
    if (pGC->stipple)
        exaFinishAccess(&pGC->stipple->drawable, EXA_PREPARE_MASK);
}

/* Is exaPrepareAccessGC() needed? */
static void
exaDestroyGC(GCPtr pGC)
{
    ExaGCPriv(pGC);
    swap(pExaGC, pGC, funcs);
    (*pGC->funcs->DestroyGC) (pGC);
    swap(pExaGC, pGC, funcs);
}

static void
exaChangeGC(GCPtr pGC, unsigned long mask)
{
    ExaGCPriv(pGC);
    swap(pExaGC, pGC, funcs);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    swap(pExaGC, pGC, funcs);
}

static void
exaCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    ExaGCPriv(pGCDst);
    swap(pExaGC, pGCDst, funcs);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    swap(pExaGC, pGCDst, funcs);
}

static void
exaChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    ExaGCPriv(pGC);
    swap(pExaGC, pGC, funcs);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    swap(pExaGC, pGC, funcs);
}

static void
exaCopyClip(GCPtr pGCDst, GCPtr pGCSrc)
{
    ExaGCPriv(pGCDst);
    swap(pExaGC, pGCDst, funcs);
    (*pGCDst->funcs->CopyClip) (pGCDst, pGCSrc);
    swap(pExaGC, pGCDst, funcs);
}

static void
exaDestroyClip(GCPtr pGC)
{
    ExaGCPriv(pGC);
    swap(pExaGC, pGC, funcs);
    (*pGC->funcs->DestroyClip) (pGC);
    swap(pExaGC, pGC, funcs);
}

/**
 * exaCreateGC makes a new GC and hooks up its funcs handler, so that
 * exaValidateGC() will get called.
 */
static int
exaCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;

    ExaScreenPriv(pScreen);
    ExaGCPriv(pGC);
    Bool ret;

    swap(pExaScr, pScreen, CreateGC);
    if ((ret = (*pScreen->CreateGC) (pGC))) {
        wrap(pExaGC, pGC, funcs, &exaGCFuncs);
        wrap(pExaGC, pGC, ops, &exaOps);
    }
    swap(pExaScr, pScreen, CreateGC);

    return ret;
}

static Bool
exaChangeWindowAttributes(WindowPtr pWin, unsigned long mask)
{
    Bool ret;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    ExaScreenPriv(pScreen);

    if ((mask & CWBackPixmap) && pWin->backgroundState == BackgroundPixmap)
        exaPrepareAccess(&pWin->background.pixmap->drawable, EXA_PREPARE_SRC);

    if ((mask & CWBorderPixmap) && pWin->borderIsPixel == FALSE)
        exaPrepareAccess(&pWin->border.pixmap->drawable, EXA_PREPARE_MASK);

    pExaScr->fallback_counter++;
    swap(pExaScr, pScreen, ChangeWindowAttributes);
    ret = pScreen->ChangeWindowAttributes(pWin, mask);
    swap(pExaScr, pScreen, ChangeWindowAttributes);
    pExaScr->fallback_counter--;

    if ((mask & CWBackPixmap) && pWin->backgroundState == BackgroundPixmap)
        exaFinishAccess(&pWin->background.pixmap->drawable, EXA_PREPARE_SRC);
    if ((mask & CWBorderPixmap) && pWin->borderIsPixel == FALSE)
        exaFinishAccess(&pWin->border.pixmap->drawable, EXA_PREPARE_MASK);

    return ret;
}

static RegionPtr
exaBitmapToRegion(PixmapPtr pPix)
{
    RegionPtr ret;
    ScreenPtr pScreen = pPix->drawable.pScreen;

    ExaScreenPriv(pScreen);

    exaPrepareAccess(&pPix->drawable, EXA_PREPARE_SRC);
    swap(pExaScr, pScreen, BitmapToRegion);
    ret = (*pScreen->BitmapToRegion) (pPix);
    swap(pExaScr, pScreen, BitmapToRegion);
    exaFinishAccess(&pPix->drawable, EXA_PREPARE_SRC);

    return ret;
}

static Bool
exaCreateScreenResources(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    PixmapPtr pScreenPixmap;
    Bool b;

    swap(pExaScr, pScreen, CreateScreenResources);
    b = pScreen->CreateScreenResources(pScreen);
    swap(pExaScr, pScreen, CreateScreenResources);

    if (!b)
        return FALSE;

    pScreenPixmap = pScreen->GetScreenPixmap(pScreen);

    if (pScreenPixmap) {
        ExaPixmapPriv(pScreenPixmap);

        exaSetAccelBlock(pExaScr, pExaPixmap,
                         pScreenPixmap->drawable.width,
                         pScreenPixmap->drawable.height,
                         pScreenPixmap->drawable.bitsPerPixel);
    }

    return TRUE;
}

static void
ExaBlockHandler(ScreenPtr pScreen, void *pTimeout)
{
    ExaScreenPriv(pScreen);

    /* Move any deferred results from a software fallback to the driver pixmap */
    if (pExaScr->deferred_mixed_pixmap)
        exaMoveInPixmap_mixed(pExaScr->deferred_mixed_pixmap);

    unwrap(pExaScr, pScreen, BlockHandler);
    (*pScreen->BlockHandler) (pScreen, pTimeout);
    wrap(pExaScr, pScreen, BlockHandler, ExaBlockHandler);

    /* The rest only applies to classic EXA */
    if (pExaScr->info->flags & EXA_HANDLES_PIXMAPS)
        return;

    /* Try and keep the offscreen memory area tidy every now and then (at most
     * once per second) when the server has been idle for at least 100ms.
     */
    if (pExaScr->numOffscreenAvailable > 1) {
        CARD32 now = GetTimeInMillis();

        pExaScr->nextDefragment = now +
            max(100, (INT32) (pExaScr->lastDefragment + 1000 - now));
        AdjustWaitForDelay(pTimeout, pExaScr->nextDefragment - now);
    }
}

static void
ExaWakeupHandler(ScreenPtr pScreen, int result)
{
    ExaScreenPriv(pScreen);

    unwrap(pExaScr, pScreen, WakeupHandler);
    (*pScreen->WakeupHandler) (pScreen, result);
    wrap(pExaScr, pScreen, WakeupHandler, ExaWakeupHandler);

    if (result == 0 && pExaScr->numOffscreenAvailable > 1) {
        CARD32 now = GetTimeInMillis();

        if ((int) (now - pExaScr->nextDefragment) > 0) {
            ExaOffscreenDefragment(pScreen);
            pExaScr->lastDefragment = now;
        }
    }
}

/**
 * exaCloseScreen() unwraps its wrapped screen functions and tears down EXA's
 * screen private, before calling down to the next CloseSccreen.
 */
static Bool
exaCloseScreen(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    PictureScreenPtr ps = GetPictureScreenIfSet(pScreen);

    if (ps->Glyphs == exaGlyphs)
        exaGlyphsFini(pScreen);

    if (pScreen->BlockHandler == ExaBlockHandler)
        unwrap(pExaScr, pScreen, BlockHandler);
    if (pScreen->WakeupHandler == ExaWakeupHandler)
        unwrap(pExaScr, pScreen, WakeupHandler);
    unwrap(pExaScr, pScreen, CreateGC);
    unwrap(pExaScr, pScreen, CloseScreen);
    unwrap(pExaScr, pScreen, GetImage);
    unwrap(pExaScr, pScreen, GetSpans);
    if (pExaScr->SavedCreatePixmap)
        unwrap(pExaScr, pScreen, CreatePixmap);
    if (pExaScr->SavedDestroyPixmap)
        unwrap(pExaScr, pScreen, DestroyPixmap);
    if (pExaScr->SavedModifyPixmapHeader)
        unwrap(pExaScr, pScreen, ModifyPixmapHeader);
    unwrap(pExaScr, pScreen, CopyWindow);
    unwrap(pExaScr, pScreen, ChangeWindowAttributes);
    unwrap(pExaScr, pScreen, BitmapToRegion);
    unwrap(pExaScr, pScreen, CreateScreenResources);
    if (pExaScr->SavedSharePixmapBacking)
        unwrap(pExaScr, pScreen, SharePixmapBacking);
    if (pExaScr->SavedSetSharedPixmapBacking)
        unwrap(pExaScr, pScreen, SetSharedPixmapBacking);
    unwrap(pExaScr, ps, Composite);
    if (pExaScr->SavedGlyphs)
        unwrap(pExaScr, ps, Glyphs);
    unwrap(pExaScr, ps, Trapezoids);
    unwrap(pExaScr, ps, Triangles);
    unwrap(pExaScr, ps, AddTraps);

    free(pExaScr);

    return (*pScreen->CloseScreen) (pScreen);
}

/**
 * This function allocates a driver structure for EXA drivers to fill in.  By
 * having EXA allocate the structure, the driver structure can be extended
 * without breaking ABI between EXA and the drivers.  The driver's
 * responsibility is to check beforehand that the EXA module has a matching
 * major number and sufficient minor.  Drivers are responsible for freeing the
 * driver structure using free().
 *
 * @return a newly allocated, zero-filled driver structure
 */
ExaDriverPtr
exaDriverAlloc(void)
{
    return calloc(1, sizeof(ExaDriverRec));
}

/**
 * @param pScreen screen being initialized
 * @param pScreenInfo EXA driver record
 *
 * exaDriverInit sets up EXA given a driver record filled in by the driver.
 * pScreenInfo should have been allocated by exaDriverAlloc().  See the
 * comments in _ExaDriver for what must be filled in and what is optional.
 *
 * @return TRUE if EXA was successfully initialized.
 */
Bool
exaDriverInit(ScreenPtr pScreen, ExaDriverPtr pScreenInfo)
{
    ExaScreenPrivPtr pExaScr;
    PictureScreenPtr ps;

    if (!pScreenInfo)
        return FALSE;

    if (pScreenInfo->exa_major != EXA_VERSION_MAJOR ||
        pScreenInfo->exa_minor > EXA_VERSION_MINOR) {
        LogMessage(X_ERROR, "EXA(%d): driver's EXA version requirements "
                   "(%d.%d) are incompatible with EXA version (%d.%d)\n",
                   pScreen->myNum,
                   pScreenInfo->exa_major, pScreenInfo->exa_minor,
                   EXA_VERSION_MAJOR, EXA_VERSION_MINOR);
        return FALSE;
    }

    if (!pScreenInfo->CreatePixmap && !pScreenInfo->CreatePixmap2) {
        if (!pScreenInfo->memoryBase) {
            LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::memoryBase "
                       "must be non-zero\n", pScreen->myNum);
            return FALSE;
        }

        if (!pScreenInfo->memorySize) {
            LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::memorySize must be "
                       "non-zero\n", pScreen->myNum);
            return FALSE;
        }

        if (pScreenInfo->offScreenBase > pScreenInfo->memorySize) {
            LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::offScreenBase must "
                       "be <= ExaDriverRec::memorySize\n", pScreen->myNum);
            return FALSE;
        }
    }

    if (!pScreenInfo->PrepareSolid) {
        LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::PrepareSolid must be "
                   "non-NULL\n", pScreen->myNum);
        return FALSE;
    }

    if (!pScreenInfo->PrepareCopy) {
        LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::PrepareCopy must be "
                   "non-NULL\n", pScreen->myNum);
        return FALSE;
    }

    if (!pScreenInfo->WaitMarker) {
        LogMessage(X_ERROR, "EXA(%d): ExaDriverRec::WaitMarker must be "
                   "non-NULL\n", pScreen->myNum);
        return FALSE;
    }

    /* If the driver doesn't set any max pitch values, we'll just assume
     * that there's a limitation by pixels, and that it's the same as
     * maxX.
     *
     * We want maxPitchPixels or maxPitchBytes to be set so we can check
     * pixmaps against the max pitch in exaCreatePixmap() -- it matters
     * whether a pixmap is rejected because of its pitch or
     * because of its width.
     */
    if (!pScreenInfo->maxPitchPixels && !pScreenInfo->maxPitchBytes) {
        pScreenInfo->maxPitchPixels = pScreenInfo->maxX;
    }

    ps = GetPictureScreenIfSet(pScreen);

    if (!dixRegisterPrivateKey(&exaScreenPrivateKeyRec, PRIVATE_SCREEN, 0)) {
        LogMessage(X_WARNING, "EXA(%d): Failed to register screen private\n",
                   pScreen->myNum);
        return FALSE;
    }

    pExaScr = calloc(sizeof(ExaScreenPrivRec), 1);
    if (!pExaScr) {
        LogMessage(X_WARNING, "EXA(%d): Failed to allocate screen private\n",
                   pScreen->myNum);
        return FALSE;
    }

    pExaScr->info = pScreenInfo;

    dixSetPrivate(&pScreen->devPrivates, exaScreenPrivateKey, pExaScr);

    pExaScr->migration = ExaMigrationAlways;

    exaDDXDriverInit(pScreen);

    if (!dixRegisterScreenSpecificPrivateKey
        (pScreen, &pExaScr->gcPrivateKeyRec, PRIVATE_GC, sizeof(ExaGCPrivRec))) {
        LogMessage(X_WARNING, "EXA(%d): Failed to allocate GC private\n",
                   pScreen->myNum);
        return FALSE;
    }

    /*
     * Replace various fb screen functions
     */
    if ((pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS) &&
        (!(pExaScr->info->flags & EXA_HANDLES_PIXMAPS) ||
         (pExaScr->info->flags & EXA_MIXED_PIXMAPS)))
        wrap(pExaScr, pScreen, BlockHandler, ExaBlockHandler);
    if ((pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS) &&
        !(pExaScr->info->flags & EXA_HANDLES_PIXMAPS))
        wrap(pExaScr, pScreen, WakeupHandler, ExaWakeupHandler);
    wrap(pExaScr, pScreen, CreateGC, exaCreateGC);
    wrap(pExaScr, pScreen, CloseScreen, exaCloseScreen);
    wrap(pExaScr, pScreen, GetImage, exaGetImage);
    wrap(pExaScr, pScreen, GetSpans, ExaCheckGetSpans);
    wrap(pExaScr, pScreen, CopyWindow, exaCopyWindow);
    wrap(pExaScr, pScreen, ChangeWindowAttributes, exaChangeWindowAttributes);
    wrap(pExaScr, pScreen, BitmapToRegion, exaBitmapToRegion);
    wrap(pExaScr, pScreen, CreateScreenResources, exaCreateScreenResources);

    if (ps) {
        wrap(pExaScr, ps, Composite, exaComposite);
        if (pScreenInfo->PrepareComposite) {
            wrap(pExaScr, ps, Glyphs, exaGlyphs);
        }
        else {
            wrap(pExaScr, ps, Glyphs, ExaCheckGlyphs);
        }
        wrap(pExaScr, ps, Trapezoids, exaTrapezoids);
        wrap(pExaScr, ps, Triangles, exaTriangles);
        wrap(pExaScr, ps, AddTraps, ExaCheckAddTraps);
    }

#ifdef MITSHM
    /*
     * Don't allow shared pixmaps.
     */
    ShmRegisterFuncs(pScreen, &exaShmFuncs);
#endif
    /*
     * Hookup offscreen pixmaps
     */
    if (pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS) {
        if (!dixRegisterScreenSpecificPrivateKey
            (pScreen, &pExaScr->pixmapPrivateKeyRec, PRIVATE_PIXMAP,
             sizeof(ExaPixmapPrivRec))) {
            LogMessage(X_WARNING,
                       "EXA(%d): Failed to allocate pixmap private\n",
                       pScreen->myNum);
            return FALSE;
        }
        if (pExaScr->info->flags & EXA_HANDLES_PIXMAPS) {
            if (pExaScr->info->flags & EXA_MIXED_PIXMAPS) {
                wrap(pExaScr, pScreen, CreatePixmap, exaCreatePixmap_mixed);
                wrap(pExaScr, pScreen, DestroyPixmap, exaDestroyPixmap_mixed);
                wrap(pExaScr, pScreen, ModifyPixmapHeader,
                     exaModifyPixmapHeader_mixed);
                wrap(pExaScr, pScreen, SharePixmapBacking, exaSharePixmapBacking_mixed);
                wrap(pExaScr, pScreen, SetSharedPixmapBacking, exaSetSharedPixmapBacking_mixed);

                pExaScr->do_migration = exaDoMigration_mixed;
                pExaScr->pixmap_has_gpu_copy = exaPixmapHasGpuCopy_mixed;
                pExaScr->do_move_in_pixmap = exaMoveInPixmap_mixed;
                pExaScr->do_move_out_pixmap = NULL;
                pExaScr->prepare_access_reg = exaPrepareAccessReg_mixed;
            }
            else {
                wrap(pExaScr, pScreen, CreatePixmap, exaCreatePixmap_driver);
                wrap(pExaScr, pScreen, DestroyPixmap, exaDestroyPixmap_driver);
                wrap(pExaScr, pScreen, ModifyPixmapHeader,
                     exaModifyPixmapHeader_driver);
                pExaScr->do_migration = NULL;
                pExaScr->pixmap_has_gpu_copy = exaPixmapHasGpuCopy_driver;
                pExaScr->do_move_in_pixmap = NULL;
                pExaScr->do_move_out_pixmap = NULL;
                pExaScr->prepare_access_reg = NULL;
            }
        }
        else {
            wrap(pExaScr, pScreen, CreatePixmap, exaCreatePixmap_classic);
            wrap(pExaScr, pScreen, DestroyPixmap, exaDestroyPixmap_classic);
            wrap(pExaScr, pScreen, ModifyPixmapHeader,
                 exaModifyPixmapHeader_classic);
            pExaScr->do_migration = exaDoMigration_classic;
            pExaScr->pixmap_has_gpu_copy = exaPixmapHasGpuCopy_classic;
            pExaScr->do_move_in_pixmap = exaMoveInPixmap_classic;
            pExaScr->do_move_out_pixmap = exaMoveOutPixmap_classic;
            pExaScr->prepare_access_reg = exaPrepareAccessReg_classic;
        }
        if (!(pExaScr->info->flags & EXA_HANDLES_PIXMAPS)) {
            LogMessage(X_INFO, "EXA(%d): Offscreen pixmap area of %lu bytes\n",
                       pScreen->myNum,
                       pExaScr->info->memorySize -
                       pExaScr->info->offScreenBase);
        }
        else {
            LogMessage(X_INFO, "EXA(%d): Driver allocated offscreen pixmaps\n",
                       pScreen->myNum);

        }
    }
    else
        LogMessage(X_INFO, "EXA(%d): No offscreen pixmaps\n", pScreen->myNum);

    if (!(pExaScr->info->flags & EXA_HANDLES_PIXMAPS)) {
        DBG_PIXMAP(("============== %ld < %ld\n", pExaScr->info->offScreenBase,
                    pExaScr->info->memorySize));
        if (pExaScr->info->offScreenBase < pExaScr->info->memorySize) {
            if (!exaOffscreenInit(pScreen)) {
                LogMessage(X_WARNING,
                           "EXA(%d): Offscreen pixmap setup failed\n",
                           pScreen->myNum);
                return FALSE;
            }
        }
    }

    if (ps->Glyphs == exaGlyphs)
        exaGlyphsInit(pScreen);

    LogMessage(X_INFO, "EXA(%d): Driver registered support for the following"
               " operations:\n", pScreen->myNum);
    assert(pScreenInfo->PrepareSolid != NULL);
    LogMessage(X_INFO, "        Solid\n");
    assert(pScreenInfo->PrepareCopy != NULL);
    LogMessage(X_INFO, "        Copy\n");
    if (pScreenInfo->PrepareComposite != NULL) {
        LogMessage(X_INFO, "        Composite (RENDER acceleration)\n");
    }
    if (pScreenInfo->UploadToScreen != NULL) {
        LogMessage(X_INFO, "        UploadToScreen\n");
    }
    if (pScreenInfo->DownloadFromScreen != NULL) {
        LogMessage(X_INFO, "        DownloadFromScreen\n");
    }

    return TRUE;
}

/**
 * exaDriverFini tears down EXA on a given screen.
 *
 * @param pScreen screen being torn down.
 */
void
exaDriverFini(ScreenPtr pScreen)
{
    /*right now does nothing */
}

/**
 * exaMarkSync() should be called after any asynchronous drawing by the hardware.
 *
 * @param pScreen screen which drawing occurred on
 *
 * exaMarkSync() sets a flag to indicate that some asynchronous drawing has
 * happened and a WaitSync() will be necessary before relying on the contents of
 * offscreen memory from the CPU's perspective.  It also calls an optional
 * driver MarkSync() callback, the return value of which may be used to do partial
 * synchronization with the hardware in the future.
 */
void
exaMarkSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    pExaScr->info->needsSync = TRUE;
    if (pExaScr->info->MarkSync != NULL) {
        pExaScr->info->lastMarker = (*pExaScr->info->MarkSync) (pScreen);
    }
}

/**
 * exaWaitSync() ensures that all drawing has been completed.
 *
 * @param pScreen screen being synchronized.
 *
 * Calls down into the driver to ensure that all previous drawing has completed.
 * It should always be called before relying on the framebuffer contents
 * reflecting previous drawing, from a CPU perspective.
 */
void
exaWaitSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    if (pExaScr->info->needsSync && !pExaScr->swappedOut) {
        (*pExaScr->info->WaitMarker) (pScreen, pExaScr->info->lastMarker);
        pExaScr->info->needsSync = FALSE;
    }
}

/**
 * Performs migration of the pixmaps according to the operation information
 * provided in pixmaps and can_accel and the migration scheme chosen in the
 * config file.
 */
void
exaDoMigration(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel)
{
    ScreenPtr pScreen = pixmaps[0].pPix->drawable.pScreen;

    ExaScreenPriv(pScreen);

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return;

    if (pExaScr->do_migration)
        (*pExaScr->do_migration) (pixmaps, npixmaps, can_accel);
}

void
exaMoveInPixmap(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return;

    if (pExaScr->do_move_in_pixmap)
        (*pExaScr->do_move_in_pixmap) (pPixmap);
}

void
exaMoveOutPixmap(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);

    if (!(pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS))
        return;

    if (pExaScr->do_move_out_pixmap)
        (*pExaScr->do_move_out_pixmap) (pPixmap);
}
