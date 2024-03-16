/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2011 Aaron Plattner
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "mi.h"
#include "xf86.h"
#include "xf86DDC.h"
#include "windowstr.h"
#include "xf86Crtc.h"
#include "xf86Modes.h"
#include "xf86RandR12.h"
#include "X11/extensions/render.h"
#include "X11/extensions/dpmsconst.h"
#include "X11/Xatom.h"

static void
xf86RotateCrtcRedisplay(xf86CrtcPtr crtc, RegionPtr region)
{
    ScrnInfoPtr scrn = crtc->scrn;
    ScreenPtr screen = scrn->pScreen;
    WindowPtr root = screen->root;
    PixmapPtr dst_pixmap = crtc->rotatedPixmap;
    PictFormatPtr format = PictureWindowFormat(screen->root);
    int error;
    PicturePtr src, dst;
    int n = RegionNumRects(region);
    BoxPtr b = RegionRects(region);
    XID include_inferiors = IncludeInferiors;

    if (crtc->driverIsPerformingTransform & XF86DriverTransformOutput)
        return;

    src = CreatePicture(None,
                        &root->drawable,
                        format,
                        CPSubwindowMode,
                        &include_inferiors, serverClient, &error);
    if (!src)
        return;

    dst = CreatePicture(None,
                        &dst_pixmap->drawable,
                        format, 0L, NULL, serverClient, &error);
    if (!dst)
        return;

    error = SetPictureTransform(src, &crtc->crtc_to_framebuffer);
    if (error)
        return;
    if (crtc->transform_in_use && crtc->filter)
        SetPicturePictFilter(src, crtc->filter, crtc->params, crtc->nparams);

    if (crtc->shadowClear) {
        CompositePicture(PictOpSrc,
                         src, NULL, dst,
                         0, 0, 0, 0, 0, 0,
                         crtc->mode.HDisplay, crtc->mode.VDisplay);
        crtc->shadowClear = FALSE;
    }
    else {
        while (n--) {
            BoxRec dst_box;

            dst_box = *b;
            dst_box.x1 -= crtc->filter_width >> 1;
            dst_box.x2 += crtc->filter_width >> 1;
            dst_box.y1 -= crtc->filter_height >> 1;
            dst_box.y2 += crtc->filter_height >> 1;
            pixman_f_transform_bounds(&crtc->f_framebuffer_to_crtc, &dst_box);
            CompositePicture(PictOpSrc,
                             src, NULL, dst,
                             dst_box.x1, dst_box.y1, 0, 0, dst_box.x1,
                             dst_box.y1, dst_box.x2 - dst_box.x1,
                             dst_box.y2 - dst_box.y1);
            b++;
        }
    }
    FreePicture(src, None);
    FreePicture(dst, None);
}

static void
xf86CrtcDamageShadow(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    BoxRec damage_box;
    RegionRec damage_region;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);

    damage_box.x1 = 0;
    damage_box.x2 = crtc->mode.HDisplay;
    damage_box.y1 = 0;
    damage_box.y2 = crtc->mode.VDisplay;
    if (!pixman_transform_bounds(&crtc->crtc_to_framebuffer, &damage_box)) {
        damage_box.x1 = 0;
        damage_box.y1 = 0;
        damage_box.x2 = pScreen->width;
        damage_box.y2 = pScreen->height;
    }
    if (damage_box.x1 < 0)
        damage_box.x1 = 0;
    if (damage_box.y1 < 0)
        damage_box.y1 = 0;
    if (damage_box.x2 > pScreen->width)
        damage_box.x2 = pScreen->width;
    if (damage_box.y2 > pScreen->height)
        damage_box.y2 = pScreen->height;
    RegionInit(&damage_region, &damage_box, 1);
    DamageDamageRegion(&(*pScreen->GetScreenPixmap) (pScreen)->drawable,
                       &damage_region);
    RegionUninit(&damage_region);
    crtc->shadowClear = TRUE;
}

static void
xf86RotatePrepare(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->rotatedData && !crtc->rotatedPixmap) {
            crtc->rotatedPixmap = crtc->funcs->shadow_create(crtc,
                                                             crtc->rotatedData,
                                                             crtc->mode.
                                                             HDisplay,
                                                             crtc->mode.
                                                             VDisplay);
            if (!xf86_config->rotation_damage_registered) {
                /* Hook damage to screen pixmap */
                DamageRegister(&pScreen->root->drawable,
                               xf86_config->rotation_damage);
                xf86_config->rotation_damage_registered = TRUE;
                EnableLimitedSchedulingLatency();
            }

            xf86CrtcDamageShadow(crtc);
        }
    }
}

static Bool
xf86RotateRedisplay(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    DamagePtr damage = xf86_config->rotation_damage;
    RegionPtr region;

    if (!damage || !pScreen->root)
        return FALSE;
    xf86RotatePrepare(pScreen);
    region = DamageRegion(damage);
    if (RegionNotEmpty(region)) {
        int c;
        SourceValidateProcPtr SourceValidate;

        /*
         * SourceValidate is used by the software cursor code
         * to pull the cursor off of the screen when reading
         * bits from the frame buffer. Bypassing this function
         * leaves the software cursor in place
         */
        SourceValidate = pScreen->SourceValidate;
        pScreen->SourceValidate = miSourceValidate;

        for (c = 0; c < xf86_config->num_crtc; c++) {
            xf86CrtcPtr crtc = xf86_config->crtc[c];

            if (crtc->transform_in_use && crtc->enabled) {
                RegionRec crtc_damage;

                /* compute portion of damage that overlaps crtc */
                RegionInit(&crtc_damage, &crtc->bounds, 1);
                RegionIntersect(&crtc_damage, &crtc_damage, region);

                /* update damaged region */
                if (RegionNotEmpty(&crtc_damage))
                    xf86RotateCrtcRedisplay(crtc, &crtc_damage);

                RegionUninit(&crtc_damage);
            }
        }
        pScreen->SourceValidate = SourceValidate;
        DamageEmpty(damage);
    }
    return TRUE;
}

static void
xf86RotateBlockHandler(ScreenPtr pScreen, void *pTimeout)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

    /* Unwrap before redisplay in case the software
     * cursor layer wants to add its block handler to the
     * chain
     */
    pScreen->BlockHandler = xf86_config->BlockHandler;

    xf86RotateRedisplay(pScreen);

    (*pScreen->BlockHandler) (pScreen, pTimeout);

    /* Re-wrap if we still need this hook */
    if (xf86_config->rotation_damage != NULL) {
        xf86_config->BlockHandler = pScreen->BlockHandler;
        pScreen->BlockHandler = xf86RotateBlockHandler;
    } else
        xf86_config->BlockHandler = NULL;
}

void
xf86RotateDestroy(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;

    /* Free memory from rotation */
    if (crtc->rotatedPixmap || crtc->rotatedData) {
        crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
                                    crtc->rotatedData);
        crtc->rotatedPixmap = NULL;
        crtc->rotatedData = NULL;
    }

    for (c = 0; c < xf86_config->num_crtc; c++)
        if (xf86_config->crtc[c]->rotatedData)
            return;

    /*
     * Clean up damage structures when no crtcs are rotated
     */
    if (xf86_config->rotation_damage) {
        /* Free damage structure */
        if (xf86_config->rotation_damage_registered) {
            xf86_config->rotation_damage_registered = FALSE;
            DisableLimitedSchedulingLatency();
        }
        DamageDestroy(xf86_config->rotation_damage);
        xf86_config->rotation_damage = NULL;
    }
}

void
xf86RotateFreeShadow(ScrnInfoPtr pScrn)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;

    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

        if (crtc->rotatedPixmap || crtc->rotatedData) {
            crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
                                        crtc->rotatedData);
            crtc->rotatedPixmap = NULL;
            crtc->rotatedData = NULL;
        }
    }
}

void
xf86RotateCloseScreen(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    /* This has already been destroyed when the root window was destroyed */
    xf86_config->rotation_damage = NULL;
    for (c = 0; c < xf86_config->num_crtc; c++)
        xf86RotateDestroy(xf86_config->crtc[c]);
}

static Bool
xf86CrtcFitsScreen(xf86CrtcPtr crtc, struct pict_f_transform *crtc_to_fb)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    BoxRec b;

    /* When called before PreInit, the driver is
     * presumably doing load detect
     */
    if (pScrn->is_gpu) {
	ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
	if (pScreen->current_primary)
	    pScrn = xf86ScreenToScrn(pScreen->current_primary);
    }

    if (pScrn->virtualX == 0 || pScrn->virtualY == 0)
        return TRUE;

    b.x1 = 0;
    b.y1 = 0;
    b.x2 = crtc->mode.HDisplay;
    b.y2 = crtc->mode.VDisplay;
    if (crtc_to_fb)
        pixman_f_transform_bounds(crtc_to_fb, &b);
    else {
        b.x1 += crtc->x;
        b.y1 += crtc->y;
        b.x2 += crtc->x;
        b.y2 += crtc->y;
    }

    return (0 <= b.x1 && b.x2 <= pScrn->virtualX &&
            0 <= b.y1 && b.y2 <= pScrn->virtualY);
}

Bool
xf86CrtcRotate(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    PictTransform crtc_to_fb;
    struct pict_f_transform f_crtc_to_fb, f_fb_to_crtc;
    xFixed *new_params = NULL;
    int new_nparams = 0;
    PictFilterPtr new_filter = NULL;
    int new_width = 0;
    int new_height = 0;
    RRTransformPtr transform = NULL;
    Bool damage = FALSE;

    if (pScreen->isGPU)
        return TRUE;
    if (crtc->transformPresent)
        transform = &crtc->transform;

    if (!RRTransformCompute(crtc->x, crtc->y,
                            crtc->mode.HDisplay, crtc->mode.VDisplay,
                            crtc->rotation,
                            transform,
                            &crtc_to_fb,
                            &f_crtc_to_fb,
                            &f_fb_to_crtc) &&
        xf86CrtcFitsScreen(crtc, &f_crtc_to_fb)) {
        /*
         * If the untranslated transformation is the identity,
         * disable the shadow buffer
         */
        xf86RotateDestroy(crtc);
        crtc->transform_in_use = FALSE;
        free(new_params);
        new_params = NULL;
        new_nparams = 0;
        new_filter = NULL;
        new_width = 0;
        new_height = 0;
    }
    else {
        if (crtc->driverIsPerformingTransform & XF86DriverTransformOutput) {
            xf86RotateDestroy(crtc);
        }
        else {
            /*
             * these are the size of the shadow pixmap, which
             * matches the mode, not the pre-rotated copy in the
             * frame buffer
             */
            int width = crtc->mode.HDisplay;
            int height = crtc->mode.VDisplay;
            void *shadowData = crtc->rotatedData;
            PixmapPtr shadow = crtc->rotatedPixmap;
            int old_width = shadow ? shadow->drawable.width : 0;
            int old_height = shadow ? shadow->drawable.height : 0;

            /* Allocate memory for rotation */
            if (old_width != width || old_height != height) {
                if (shadow || shadowData) {
                    crtc->funcs->shadow_destroy(crtc, shadow, shadowData);
                    crtc->rotatedPixmap = NULL;
                    crtc->rotatedData = NULL;
                }
                shadowData = crtc->funcs->shadow_allocate(crtc, width, height);
                if (!shadowData)
                    goto bail1;
                crtc->rotatedData = shadowData;
                /* shadow will be damaged in xf86RotatePrepare */
            }
            else {
                /* mark shadowed area as damaged so it will be repainted */
                damage = TRUE;
            }

            if (!xf86_config->rotation_damage) {
                /* Create damage structure */
                xf86_config->rotation_damage = DamageCreate(NULL, NULL,
                                                            DamageReportNone,
                                                            TRUE, pScreen,
                                                            pScreen);
                if (!xf86_config->rotation_damage)
                    goto bail2;

                /* Wrap block handler */
                if (!xf86_config->BlockHandler) {
                    xf86_config->BlockHandler = pScreen->BlockHandler;
                    pScreen->BlockHandler = xf86RotateBlockHandler;
                }
            }

            if (0) {
 bail2:
                if (shadow || shadowData) {
                    crtc->funcs->shadow_destroy(crtc, shadow, shadowData);
                    crtc->rotatedPixmap = NULL;
                    crtc->rotatedData = NULL;
                }
 bail1:
                if (old_width && old_height)
                    crtc->rotatedPixmap =
                        crtc->funcs->shadow_create(crtc, NULL, old_width,
                                                   old_height);
                return FALSE;
            }
        }
#ifdef RANDR_12_INTERFACE
        if (transform) {
            if (transform->nparams) {
                new_params = malloc(transform->nparams * sizeof(xFixed));
                if (new_params) {
                    memcpy(new_params, transform->params,
                           transform->nparams * sizeof(xFixed));
                    new_nparams = transform->nparams;
                    new_filter = transform->filter;
                }
            }
            else
                new_filter = transform->filter;
            if (new_filter) {
                new_width = new_filter->width;
                new_height = new_filter->height;
            }
        }
#endif
        crtc->transform_in_use = TRUE;
    }
    crtc->crtc_to_framebuffer = crtc_to_fb;
    crtc->f_crtc_to_framebuffer = f_crtc_to_fb;
    crtc->f_framebuffer_to_crtc = f_fb_to_crtc;
    free(crtc->params);
    crtc->params = new_params;
    crtc->nparams = new_nparams;
    crtc->filter = new_filter;
    crtc->filter_width = new_width;
    crtc->filter_height = new_height;
    crtc->bounds.x1 = 0;
    crtc->bounds.x2 = crtc->mode.HDisplay;
    crtc->bounds.y1 = 0;
    crtc->bounds.y2 = crtc->mode.VDisplay;
    pixman_f_transform_bounds(&f_crtc_to_fb, &crtc->bounds);

    if (damage)
        xf86CrtcDamageShadow(crtc);
    else if (crtc->rotatedData && !crtc->rotatedPixmap)
        /* Make sure the new rotate buffer has valid transformed contents */
        xf86RotateRedisplay(pScreen);

    /* All done */
    return TRUE;
}
