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

/* This file holds the driver allocated pixmaps + better initial placement code.
 */

static _X_INLINE void *
ExaGetPixmapAddress(PixmapPtr p)
{
    ExaPixmapPriv(p);

    return pExaPixmap->sys_ptr;
}

/**
 * exaCreatePixmap() creates a new pixmap.
 */
PixmapPtr
exaCreatePixmap_mixed(ScreenPtr pScreen, int w, int h, int depth,
                      unsigned usage_hint)
{
    PixmapPtr pPixmap;
    ExaPixmapPrivPtr pExaPixmap;
    int bpp;
    size_t paddedWidth;

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

    paddedWidth = ((w * bpp + FB_MASK) >> FB_SHIFT) * sizeof(FbBits);
    if (paddedWidth / 4 > 32767 || h > 32767)
        return NullPixmap;

    /* We will allocate the system pixmap later if needed. */
    pPixmap->devPrivate.ptr = NULL;
    pExaPixmap->sys_ptr = NULL;
    pExaPixmap->sys_pitch = paddedWidth;

    pExaPixmap->area = NULL;
    pExaPixmap->fb_ptr = NULL;
    pExaPixmap->pDamage = NULL;

    exaSetFbPitch(pExaScr, pExaPixmap, w, h, bpp);
    exaSetAccelBlock(pExaScr, pExaPixmap, w, h, bpp);

    (*pScreen->ModifyPixmapHeader) (pPixmap, w, h, 0, 0, paddedWidth, NULL);

    /* A scratch pixmap will become a driver pixmap right away. */
    if (!w || !h) {
        exaCreateDriverPixmap_mixed(pPixmap);
        pExaPixmap->use_gpu_copy = exaPixmapHasGpuCopy(pPixmap);
    }
    else {
        pExaPixmap->use_gpu_copy = FALSE;

        if (w == 1 && h == 1) {
            pExaPixmap->sys_ptr = malloc(paddedWidth);

            /* Set up damage tracking */
            pExaPixmap->pDamage = DamageCreate(exaDamageReport_mixed, NULL,
                                               DamageReportNonEmpty, TRUE,
                                               pPixmap->drawable.pScreen,
                                               pPixmap);

            if (pExaPixmap->pDamage) {
                DamageRegister(&pPixmap->drawable, pExaPixmap->pDamage);
                /* This ensures that pending damage reflects the current
                 * operation. This is used by exa to optimize migration.
                 */
                DamageSetReportAfterOp(pExaPixmap->pDamage, TRUE);
            }
        }
    }

    /* During a fallback we must prepare access. */
    if (pExaScr->fallback_counter)
        exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_AUX_DEST);

    return pPixmap;
}

Bool
exaModifyPixmapHeader_mixed(PixmapPtr pPixmap, int width, int height, int depth,
                            int bitsPerPixel, int devKind, void *pPixData)
{
    ScreenPtr pScreen;
    ExaScreenPrivPtr pExaScr;
    ExaPixmapPrivPtr pExaPixmap;
    Bool ret, has_gpu_copy;

    if (!pPixmap)
        return FALSE;

    pScreen = pPixmap->drawable.pScreen;
    pExaScr = ExaGetScreenPriv(pScreen);
    pExaPixmap = ExaGetPixmapPriv(pPixmap);

    if (pPixData) {
        if (pExaPixmap->driverPriv) {
            if (pExaPixmap->pDamage) {
                DamageDestroy(pExaPixmap->pDamage);
                pExaPixmap->pDamage = NULL;
            }

            pExaScr->info->DestroyPixmap(pScreen, pExaPixmap->driverPriv);
            pExaPixmap->driverPriv = NULL;
        }

        pExaPixmap->use_gpu_copy = FALSE;
        pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    }

    has_gpu_copy = exaPixmapHasGpuCopy(pPixmap);

    if (width <= 0)
        width = pPixmap->drawable.width;

    if (height <= 0)
        height = pPixmap->drawable.height;

    if (bitsPerPixel <= 0) {
        if (depth <= 0)
            bitsPerPixel = pPixmap->drawable.bitsPerPixel;
        else
            bitsPerPixel = BitsPerPixel(depth);
    }

    if (depth <= 0)
        depth = pPixmap->drawable.depth;

    if (width != pPixmap->drawable.width ||
        height != pPixmap->drawable.height ||
        depth != pPixmap->drawable.depth ||
        bitsPerPixel != pPixmap->drawable.bitsPerPixel) {
        if (pExaPixmap->driverPriv) {
            if (devKind > 0)
                pExaPixmap->fb_pitch = devKind;
            else
                exaSetFbPitch(pExaScr, pExaPixmap, width, height, bitsPerPixel);

            exaSetAccelBlock(pExaScr, pExaPixmap, width, height, bitsPerPixel);
            RegionEmpty(&pExaPixmap->validFB);
        }

        /* Need to re-create system copy if there's also a GPU copy */
        if (has_gpu_copy) {
            if (pExaPixmap->sys_ptr) {
                free(pExaPixmap->sys_ptr);
                pExaPixmap->sys_ptr = NULL;
                DamageDestroy(pExaPixmap->pDamage);
                pExaPixmap->pDamage = NULL;
                RegionEmpty(&pExaPixmap->validSys);

                if (pExaScr->deferred_mixed_pixmap == pPixmap)
                    pExaScr->deferred_mixed_pixmap = NULL;
            }

            pExaPixmap->sys_pitch = PixmapBytePad(width, depth);
        }
    }

    if (has_gpu_copy) {
        pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
        pPixmap->devKind = pExaPixmap->fb_pitch;
    }
    else {
        pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
        pPixmap->devKind = pExaPixmap->sys_pitch;
    }

    /* Only pass driver pixmaps to the driver. */
    if (pExaScr->info->ModifyPixmapHeader && pExaPixmap->driverPriv) {
        ret = pExaScr->info->ModifyPixmapHeader(pPixmap, width, height, depth,
                                                bitsPerPixel, devKind,
                                                pPixData);
        if (ret == TRUE)
            goto out;
    }

    swap(pExaScr, pScreen, ModifyPixmapHeader);
    ret = pScreen->ModifyPixmapHeader(pPixmap, width, height, depth,
                                      bitsPerPixel, devKind, pPixData);
    swap(pExaScr, pScreen, ModifyPixmapHeader);

 out:
    if (has_gpu_copy) {
        pExaPixmap->fb_ptr = pPixmap->devPrivate.ptr;
        pExaPixmap->fb_pitch = pPixmap->devKind;
    }
    else {
        pExaPixmap->sys_ptr = pPixmap->devPrivate.ptr;
        pExaPixmap->sys_pitch = pPixmap->devKind;
    }
    /* Always NULL this, we don't want lingering pointers. */
    pPixmap->devPrivate.ptr = NULL;

    return ret;
}

Bool
exaDestroyPixmap_mixed(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    Bool ret;

    if (pPixmap->refcnt == 1) {
        ExaPixmapPriv(pPixmap);

        exaDestroyPixmap(pPixmap);

        if (pExaScr->deferred_mixed_pixmap == pPixmap)
            pExaScr->deferred_mixed_pixmap = NULL;

        if (pExaPixmap->driverPriv)
            pExaScr->info->DestroyPixmap(pScreen, pExaPixmap->driverPriv);
        pExaPixmap->driverPriv = NULL;

        if (pExaPixmap->pDamage) {
            free(pExaPixmap->sys_ptr);
            pExaPixmap->sys_ptr = NULL;
            pExaPixmap->pDamage = NULL;
        }
    }

    swap(pExaScr, pScreen, DestroyPixmap);
    ret = pScreen->DestroyPixmap(pPixmap);
    swap(pExaScr, pScreen, DestroyPixmap);

    return ret;
}

Bool
exaPixmapHasGpuCopy_mixed(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    ExaPixmapPriv(pPixmap);
    void *saved_ptr;
    Bool ret;

    if (!pExaPixmap->driverPriv)
        return FALSE;

    saved_ptr = pPixmap->devPrivate.ptr;
    pPixmap->devPrivate.ptr = ExaGetPixmapAddress(pPixmap);
    ret = pExaScr->info->PixmapIsOffscreen(pPixmap);
    pPixmap->devPrivate.ptr = saved_ptr;

    return ret;
}

Bool
exaSharePixmapBacking_mixed(PixmapPtr pPixmap, ScreenPtr secondary, void **handle_p)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv(pScreen);
    Bool ret = FALSE;

    exaMoveInPixmap(pPixmap);
    /* get the driver to give us a handle */
    if (pExaScr->info->SharePixmapBacking)
        ret = pExaScr->info->SharePixmapBacking(pPixmap, secondary, handle_p);

    return ret;
}

Bool
exaSetSharedPixmapBacking_mixed(PixmapPtr pPixmap, void *handle)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv(pScreen);
    Bool ret = FALSE;

    if (pExaScr->info->SetSharedPixmapBacking)
        ret = pExaScr->info->SetSharedPixmapBacking(pPixmap, handle);

    if (ret == TRUE)
        exaMoveInPixmap(pPixmap);

    return ret;
}


