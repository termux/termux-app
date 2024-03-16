/*
 * Copyright © 2009 Maarten Maathuis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "exa_priv.h"
#include "exa.h"

/* This file holds the driver allocated pixmaps specific implementation. */

static _X_INLINE void *
ExaGetPixmapAddress(PixmapPtr p)
{
    ExaPixmapPriv(p);

    return pExaPixmap->sys_ptr;
}

/**
 * exaCreatePixmap() creates a new pixmap.
 *
 * Pixmaps are always marked as pinned, because exa has no control over them.
 */
PixmapPtr
exaCreatePixmap_driver(ScreenPtr pScreen, int w, int h, int depth,
                       unsigned usage_hint)
{
    PixmapPtr pPixmap;
    ExaPixmapPrivPtr pExaPixmap;
    int bpp;
    size_t paddedWidth, datasize;

    ExaScreenPriv(pScreen);

    if (w > 32767 || h > 32767)
        return NullPixmap;

    swap(pExaScr, pScreen, CreatePixmap);
    pPixmap = pScreen->CreatePixmap(pScreen, 0, 0, depth, usage_hint);
    swap(pExaScr, pScreen, CreatePixmap);

    if (!pPixmap)
        return NULL;

    pExaPixmap = ExaGetPixmapPriv(pPixmap);
    pExaPixmap->driverPriv = NULL;

    bpp = pPixmap->drawable.bitsPerPixel;

    /* Set this before driver hooks, to allow for driver pixmaps without gpu
     * memory to back it. These pixmaps have a valid pointer at all times.
     */
    pPixmap->devPrivate.ptr = NULL;

    if (pExaScr->info->CreatePixmap2) {
        int new_pitch = 0;

        pExaPixmap->driverPriv =
            pExaScr->info->CreatePixmap2(pScreen, w, h, depth, usage_hint, bpp,
                                         &new_pitch);
        paddedWidth = pExaPixmap->fb_pitch = new_pitch;
    }
    else {
        paddedWidth = ((w * bpp + FB_MASK) >> FB_SHIFT) * sizeof(FbBits);
        if (paddedWidth / 4 > 32767 || h > 32767)
            return NullPixmap;

        exaSetFbPitch(pExaScr, pExaPixmap, w, h, bpp);

        if (paddedWidth < pExaPixmap->fb_pitch)
            paddedWidth = pExaPixmap->fb_pitch;
        datasize = h * paddedWidth;
        pExaPixmap->driverPriv =
            pExaScr->info->CreatePixmap(pScreen, datasize, 0);
    }

    if (!pExaPixmap->driverPriv) {
        swap(pExaScr, pScreen, DestroyPixmap);
        pScreen->DestroyPixmap(pPixmap);
        swap(pExaScr, pScreen, DestroyPixmap);
        return NULL;
    }

    /* Allow ModifyPixmapHeader to set sys_ptr appropriately. */
    pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    pExaPixmap->fb_ptr = NULL;
    pExaPixmap->pDamage = NULL;
    pExaPixmap->sys_ptr = NULL;

    (*pScreen->ModifyPixmapHeader) (pPixmap, w, h, 0, 0, paddedWidth, NULL);

    pExaPixmap->area = NULL;

    exaSetAccelBlock(pExaScr, pExaPixmap, w, h, bpp);

    pExaPixmap->use_gpu_copy = exaPixmapHasGpuCopy(pPixmap);

    /* During a fallback we must prepare access. */
    if (pExaScr->fallback_counter)
        exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_AUX_DEST);

    return pPixmap;
}

Bool
exaModifyPixmapHeader_driver(PixmapPtr pPixmap, int width, int height,
                             int depth, int bitsPerPixel, int devKind,
                             void *pPixData)
{
    ScreenPtr pScreen;
    ExaScreenPrivPtr pExaScr;
    ExaPixmapPrivPtr pExaPixmap;
    Bool ret;

    if (!pPixmap)
        return FALSE;

    pScreen = pPixmap->drawable.pScreen;
    pExaScr = ExaGetScreenPriv(pScreen);
    pExaPixmap = ExaGetPixmapPriv(pPixmap);

    if (pExaPixmap) {
        if (pPixData)
            pExaPixmap->sys_ptr = pPixData;

        if (devKind > 0)
            pExaPixmap->sys_pitch = devKind;

        if (width > 0 && height > 0 && bitsPerPixel > 0) {
            exaSetFbPitch(pExaScr, pExaPixmap, width, height, bitsPerPixel);

            exaSetAccelBlock(pExaScr, pExaPixmap, width, height, bitsPerPixel);
        }
    }

    if (pExaScr->info->ModifyPixmapHeader) {
        ret = pExaScr->info->ModifyPixmapHeader(pPixmap, width, height, depth,
                                                bitsPerPixel, devKind,
                                                pPixData);
        /* For EXA_HANDLES_PIXMAPS, we set pPixData to NULL.
         * If pPixmap->devPrivate.ptr is non-NULL, then we've got a
         * !has_gpu_copy pixmap. We need to store the pointer,
         * because PrepareAccess won't be called.
         */
        if (!pPixData && pPixmap->devPrivate.ptr && pPixmap->devKind) {
            pExaPixmap->sys_ptr = pPixmap->devPrivate.ptr;
            pExaPixmap->sys_pitch = pPixmap->devKind;
        }
        if (ret == TRUE)
            goto out;
    }

    swap(pExaScr, pScreen, ModifyPixmapHeader);
    ret = pScreen->ModifyPixmapHeader(pPixmap, width, height, depth,
                                      bitsPerPixel, devKind, pPixData);
    swap(pExaScr, pScreen, ModifyPixmapHeader);

 out:
    /* Always NULL this, we don't want lingering pointers. */
    pPixmap->devPrivate.ptr = NULL;

    return ret;
}

Bool
exaDestroyPixmap_driver(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    Bool ret;

    if (pPixmap->refcnt == 1) {
        ExaPixmapPriv(pPixmap);

        exaDestroyPixmap(pPixmap);

        if (pExaPixmap->driverPriv)
            pExaScr->info->DestroyPixmap(pScreen, pExaPixmap->driverPriv);
        pExaPixmap->driverPriv = NULL;
    }

    swap(pExaScr, pScreen, DestroyPixmap);
    ret = pScreen->DestroyPixmap(pPixmap);
    swap(pExaScr, pScreen, DestroyPixmap);

    return ret;
}

Bool
exaPixmapHasGpuCopy_driver(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    void *saved_ptr;
    Bool ret;

    saved_ptr = pPixmap->devPrivate.ptr;
    pPixmap->devPrivate.ptr = ExaGetPixmapAddress(pPixmap);
    ret = pExaScr->info->PixmapIsOffscreen(pPixmap);
    pPixmap->devPrivate.ptr = saved_ptr;

    return ret;
}
