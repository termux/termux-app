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

void
exaCreateDriverPixmap_mixed(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    ExaScreenPriv(pScreen);
    ExaPixmapPriv(pPixmap);
    int w = pPixmap->drawable.width, h = pPixmap->drawable.height;
    int depth = pPixmap->drawable.depth, bpp = pPixmap->drawable.bitsPerPixel;
    int usage_hint = pPixmap->usage_hint;
    int paddedWidth = pExaPixmap->sys_pitch;

    /* Already done. */
    if (pExaPixmap->driverPriv)
        return;

    if (exaPixmapIsPinned(pPixmap))
        return;

    /* Can't accel 1/4 bpp. */
    if (pExaPixmap->accel_blocked || bpp < 8)
        return;

    if (pExaScr->info->CreatePixmap2) {
        int new_pitch = 0;

        pExaPixmap->driverPriv =
            pExaScr->info->CreatePixmap2(pScreen, w, h, depth, usage_hint, bpp,
                                         &new_pitch);
        paddedWidth = pExaPixmap->fb_pitch = new_pitch;
    }
    else {
        if (paddedWidth < pExaPixmap->fb_pitch)
            paddedWidth = pExaPixmap->fb_pitch;
        pExaPixmap->driverPriv =
            pExaScr->info->CreatePixmap(pScreen, paddedWidth * h, 0);
    }

    if (!pExaPixmap->driverPriv)
        return;

    (*pScreen->ModifyPixmapHeader) (pPixmap, w, h, 0, 0, paddedWidth, NULL);
}

void
exaDoMigration_mixed(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel)
{
    int i;

    /* If anything is pinned in system memory, we won't be able to
     * accelerate.
     */
    for (i = 0; i < npixmaps; i++) {
        if (exaPixmapIsPinned(pixmaps[i].pPix) &&
            !exaPixmapHasGpuCopy(pixmaps[i].pPix)) {
            can_accel = FALSE;
            break;
        }
    }

    /* We can do nothing. */
    if (!can_accel)
        return;

    for (i = 0; i < npixmaps; i++) {
        PixmapPtr pPixmap = pixmaps[i].pPix;

        ExaPixmapPriv(pPixmap);

        if (!pExaPixmap->driverPriv)
            exaCreateDriverPixmap_mixed(pPixmap);

        if (pExaPixmap->pDamage && exaPixmapHasGpuCopy(pPixmap)) {
            ExaScreenPriv(pPixmap->drawable.pScreen);

            /* This pitch is needed for proper acceleration. For some reason
             * there are pixmaps without pDamage and a bad fb_pitch value.
             * So setting devKind when only exaPixmapHasGpuCopy() is true
             * causes corruption. Pixmaps without pDamage are not migrated
             * and should have a valid devKind at all times, so that's why this
             * isn't causing problems. Pixmaps have their gpu pitch set the
             * first time in the MPH call from exaCreateDriverPixmap_mixed().
             */
            pPixmap->devKind = pExaPixmap->fb_pitch;
            exaCopyDirtyToFb(pixmaps + i);

            if (pExaScr->deferred_mixed_pixmap == pPixmap &&
                !pixmaps[i].as_dst && !pixmaps[i].pReg)
                pExaScr->deferred_mixed_pixmap = NULL;
        }

        pExaPixmap->use_gpu_copy = exaPixmapHasGpuCopy(pPixmap);
    }
}

void
exaMoveInPixmap_mixed(PixmapPtr pPixmap)
{
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = FALSE;
    pixmaps[0].as_src = TRUE;
    pixmaps[0].pPix = pPixmap;
    pixmaps[0].pReg = NULL;

    exaDoMigration(pixmaps, 1, TRUE);
}

void
exaDamageReport_mixed(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    PixmapPtr pPixmap = closure;

    ExaPixmapPriv(pPixmap);

    /* Move back results of software rendering on system memory copy of mixed driver
     * pixmap (see exaPrepareAccessReg_mixed).
     *
     * Defer moving the destination back into the driver pixmap, to try and save
     * overhead on multiple subsequent software fallbacks.
     */
    if (!pExaPixmap->use_gpu_copy && exaPixmapHasGpuCopy(pPixmap)) {
        ExaScreenPriv(pPixmap->drawable.pScreen);

        if (pExaScr->deferred_mixed_pixmap &&
            pExaScr->deferred_mixed_pixmap != pPixmap)
            exaMoveInPixmap_mixed(pExaScr->deferred_mixed_pixmap);
        pExaScr->deferred_mixed_pixmap = pPixmap;
    }
}

/* With mixed pixmaps, if we fail to get direct access to the driver pixmap, we
 * use the DownloadFromScreen hook to retrieve contents to a copy in system
 * memory, perform software rendering on that and move back the results with the
 * UploadToScreen hook (see exaDamageReport_mixed).
 */
void
exaPrepareAccessReg_mixed(PixmapPtr pPixmap, int index, RegionPtr pReg)
{
    ExaPixmapPriv(pPixmap);
    Bool has_gpu_copy = exaPixmapHasGpuCopy(pPixmap);
    Bool success;

    success = ExaDoPrepareAccess(pPixmap, index);

    if (success && has_gpu_copy && pExaPixmap->pDamage) {
        /* You cannot do accelerated operations while a buffer is mapped. */
        exaFinishAccess(&pPixmap->drawable, index);
        /* Update the gpu view of both deferred destination pixmaps and of
         * source pixmaps that were migrated with a bounding region.
         */
        exaMoveInPixmap_mixed(pPixmap);
        success = ExaDoPrepareAccess(pPixmap, index);

        if (success) {
            /* We have a gpu pixmap that can be accessed, we don't need the cpu
             * copy anymore. Drivers that prefer DFS, should fail prepare
             * access.
             */
            DamageDestroy(pExaPixmap->pDamage);
            pExaPixmap->pDamage = NULL;

            free(pExaPixmap->sys_ptr);
            pExaPixmap->sys_ptr = NULL;

            return;
        }
    }

    if (!success) {
        ExaMigrationRec pixmaps[1];

        /* Do we need to allocate our system buffer? */
        if (!pExaPixmap->sys_ptr) {
            pExaPixmap->sys_ptr = xallocarray(pExaPixmap->sys_pitch,
                                              pPixmap->drawable.height);
            if (!pExaPixmap->sys_ptr)
                FatalError("EXA: malloc failed for size %d bytes\n",
                           pExaPixmap->sys_pitch * pPixmap->drawable.height);
        }

        if (index == EXA_PREPARE_DEST || index == EXA_PREPARE_AUX_DEST) {
            pixmaps[0].as_dst = TRUE;
            pixmaps[0].as_src = FALSE;
        }
        else {
            pixmaps[0].as_dst = FALSE;
            pixmaps[0].as_src = TRUE;
        }
        pixmaps[0].pPix = pPixmap;
        pixmaps[0].pReg = pReg;

        if (!pExaPixmap->pDamage &&
            (has_gpu_copy || !exaPixmapIsPinned(pPixmap))) {
            Bool as_dst = pixmaps[0].as_dst;

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

            if (has_gpu_copy) {
                exaPixmapDirty(pPixmap, 0, 0, pPixmap->drawable.width,
                               pPixmap->drawable.height);

                /* We don't know which region of the destination will be damaged,
                 * have to assume all of it
                 */
                if (as_dst) {
                    pixmaps[0].as_dst = FALSE;
                    pixmaps[0].as_src = TRUE;
                    pixmaps[0].pReg = NULL;
                }
                exaCopyDirtyToSys(pixmaps);
            }

            if (as_dst)
                exaPixmapDirty(pPixmap, 0, 0, pPixmap->drawable.width,
                               pPixmap->drawable.height);
        }
        else if (has_gpu_copy)
            exaCopyDirtyToSys(pixmaps);

        pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
        pPixmap->devKind = pExaPixmap->sys_pitch;
        pExaPixmap->use_gpu_copy = FALSE;
    }
}
