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

/* This file holds the classic exa specific implementation. */

static _X_INLINE void *
ExaGetPixmapAddress(PixmapPtr p)
{
    ExaPixmapPriv(p);

    if (pExaPixmap->use_gpu_copy && pExaPixmap->fb_ptr)
        return pExaPixmap->fb_ptr;
    else
        return pExaPixmap->sys_ptr;
}

/**
 * exaCreatePixmap() creates a new pixmap.
 *
 * If width and height are 0, this won't be a full-fledged pixmap and it will
 * get ModifyPixmapHeader() called on it later.  So, we mark it as pinned, because
 * ModifyPixmapHeader() would break migration.  These types of pixmaps are used
 * for scratch pixmaps, or to represent the visible screen.
 */
PixmapPtr
exaCreatePixmap_classic(ScreenPtr pScreen, int w, int h, int depth,
                        unsigned usage_hint)
{
    PixmapPtr pPixmap;
    ExaPixmapPrivPtr pExaPixmap;
    BoxRec box;
    int bpp;

    ExaScreenPriv(pScreen);

    if (w > 32767 || h > 32767)
        return NullPixmap;

    swap(pExaScr, pScreen, CreatePixmap);
    pPixmap = pScreen->CreatePixmap(pScreen, w, h, depth, usage_hint);
    swap(pExaScr, pScreen, CreatePixmap);

    if (!pPixmap)
        return NULL;

    pExaPixmap = ExaGetPixmapPriv(pPixmap);
    pExaPixmap->driverPriv = NULL;

    bpp = pPixmap->drawable.bitsPerPixel;

    pExaPixmap->driverPriv = NULL;
    /* Scratch pixmaps may have w/h equal to zero, and may not be
     * migrated.
     */
    if (!w || !h)
        pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    else
        pExaPixmap->score = EXA_PIXMAP_SCORE_INIT;

    pExaPixmap->sys_ptr = pPixmap->devPrivate.ptr;
    pExaPixmap->sys_pitch = pPixmap->devKind;

    pPixmap->devPrivate.ptr = NULL;
    pExaPixmap->use_gpu_copy = FALSE;

    pExaPixmap->fb_ptr = NULL;
    exaSetFbPitch(pExaScr, pExaPixmap, w, h, bpp);
    pExaPixmap->fb_size = pExaPixmap->fb_pitch * h;

    if (pExaPixmap->fb_pitch > 131071) {
        swap(pExaScr, pScreen, DestroyPixmap);
        pScreen->DestroyPixmap(pPixmap);
        swap(pExaScr, pScreen, DestroyPixmap);
        return NULL;
    }

    /* Set up damage tracking */
    pExaPixmap->pDamage = DamageCreate(NULL, NULL,
                                       DamageReportNone, TRUE,
                                       pScreen, pPixmap);

    if (pExaPixmap->pDamage == NULL) {
        swap(pExaScr, pScreen, DestroyPixmap);
        pScreen->DestroyPixmap(pPixmap);
        swap(pExaScr, pScreen, DestroyPixmap);
        return NULL;
    }

    DamageRegister(&pPixmap->drawable, pExaPixmap->pDamage);
    /* This ensures that pending damage reflects the current operation. */
    /* This is used by exa to optimize migration. */
    DamageSetReportAfterOp(pExaPixmap->pDamage, TRUE);

    pExaPixmap->area = NULL;

    /* We set the initial pixmap as completely valid for a simple reason.
     * Imagine a 1000x1000 pixmap, it has 1 million pixels, 250000 of which
     * could form single pixel rects as part of a region. Setting the complete region
     * as valid is a natural defragmentation of the region.
     */
    box.x1 = 0;
    box.y1 = 0;
    box.x2 = w;
    box.y2 = h;
    RegionInit(&pExaPixmap->validSys, &box, 0);
    RegionInit(&pExaPixmap->validFB, &box, 0);

    exaSetAccelBlock(pExaScr, pExaPixmap, w, h, bpp);

    /* During a fallback we must prepare access. */
    if (pExaScr->fallback_counter)
        exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_AUX_DEST);

    return pPixmap;
}

Bool
exaModifyPixmapHeader_classic(PixmapPtr pPixmap, int width, int height,
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

        /* Classic EXA:
         * - Framebuffer.
         * - Scratch pixmap with gpu memory.
         */
        if (pExaScr->info->memoryBase && pPixData) {
            if ((CARD8 *) pPixData >= pExaScr->info->memoryBase &&
                ((CARD8 *) pPixData - pExaScr->info->memoryBase) <
                pExaScr->info->memorySize) {
                pExaPixmap->fb_ptr = pPixData;
                pExaPixmap->fb_pitch = devKind;
                pExaPixmap->use_gpu_copy = TRUE;
            }
        }

        if (width > 0 && height > 0 && bitsPerPixel > 0) {
            exaSetFbPitch(pExaScr, pExaPixmap, width, height, bitsPerPixel);

            exaSetAccelBlock(pExaScr, pExaPixmap, width, height, bitsPerPixel);
        }

        /* Pixmaps subject to ModifyPixmapHeader will be pinned to system or
         * gpu memory, so there's no need to track damage.
         */
        if (pExaPixmap->pDamage) {
            DamageDestroy(pExaPixmap->pDamage);
            pExaPixmap->pDamage = NULL;
        }
    }

    swap(pExaScr, pScreen, ModifyPixmapHeader);
    ret = pScreen->ModifyPixmapHeader(pPixmap, width, height, depth,
                                      bitsPerPixel, devKind, pPixData);
    swap(pExaScr, pScreen, ModifyPixmapHeader);

    /* Always NULL this, we don't want lingering pointers. */
    pPixmap->devPrivate.ptr = NULL;

    return ret;
}

Bool
exaDestroyPixmap_classic(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    Bool ret;

    if (pPixmap->refcnt == 1) {
        ExaPixmapPriv(pPixmap);

        exaDestroyPixmap(pPixmap);

        if (pExaPixmap->area) {
            DBG_PIXMAP(("-- 0x%p (0x%x) (%dx%d)\n",
                        (void *) pPixmap->drawable.id,
                        ExaGetPixmapPriv(pPixmap)->area->offset,
                        pPixmap->drawable.width, pPixmap->drawable.height));
            /* Free the offscreen area */
            exaOffscreenFree(pPixmap->drawable.pScreen, pExaPixmap->area);
            pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
            pPixmap->devKind = pExaPixmap->sys_pitch;
        }
        RegionUninit(&pExaPixmap->validSys);
        RegionUninit(&pExaPixmap->validFB);
    }

    swap(pExaScr, pScreen, DestroyPixmap);
    ret = pScreen->DestroyPixmap(pPixmap);
    swap(pExaScr, pScreen, DestroyPixmap);

    return ret;
}

Bool
exaPixmapHasGpuCopy_classic(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    ExaPixmapPriv(pPixmap);
    Bool ret;

    if (pExaScr->info->PixmapIsOffscreen) {
        void *old_ptr = pPixmap->devPrivate.ptr;

        pPixmap->devPrivate.ptr = ExaGetPixmapAddress(pPixmap);
        ret = pExaScr->info->PixmapIsOffscreen(pPixmap);
        pPixmap->devPrivate.ptr = old_ptr;
    }
    else
        ret = (pExaPixmap->use_gpu_copy && pExaPixmap->fb_ptr);

    return ret;
}
