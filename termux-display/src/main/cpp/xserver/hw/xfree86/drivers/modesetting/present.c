/*
 * Copyright © 2014 Intel Corporation
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

#ifdef HAVE_DIX_CONFIG_H
#include "dix-config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include <xf86.h>
#include <xf86Crtc.h>
#include <xf86drm.h>
#include <xf86str.h>
#include <present.h>

#include "driver.h"
#include "drmmode_display.h"

#if 0
#define DebugPresent(x) ErrorF x
#else
#define DebugPresent(x)
#endif

struct ms_present_vblank_event {
    uint64_t        event_id;
    Bool            unflip;
};

static RRCrtcPtr
ms_present_get_crtc(WindowPtr window)
{
    return ms_randr_crtc_covering_drawable(&window->drawable);
}

static int
ms_present_get_ust_msc(RRCrtcPtr crtc, CARD64 *ust, CARD64 *msc)
{
    xf86CrtcPtr xf86_crtc = crtc->devPrivate;

    return ms_get_crtc_ust_msc(xf86_crtc, ust, msc);
}

/*
 * Changes the variable refresh state for every CRTC on the screen.
 */
void
ms_present_set_screen_vrr(ScrnInfoPtr scrn, Bool vrr_enabled)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CrtcPtr crtc;
    int i;

    for (i = 0; i < config->num_crtc; i++) {
        crtc = config->crtc[i];
        drmmode_crtc_set_vrr(crtc, vrr_enabled);
    }
}

/*
 * Called when the queued vblank event has occurred
 */
static void
ms_present_vblank_handler(uint64_t msc, uint64_t usec, void *data)
{
    struct ms_present_vblank_event *event = data;

    DebugPresent(("\t\tmh %lld msc %llu\n",
                 (long long) event->event_id, (long long) msc));

    present_event_notify(event->event_id, usec, msc);
    free(event);
}

/*
 * Called when the queued vblank is aborted
 */
static void
ms_present_vblank_abort(void *data)
{
    struct ms_present_vblank_event *event = data;

    DebugPresent(("\t\tma %lld\n", (long long) event->event_id));

    free(event);
}

/*
 * Queue an event to report back to the Present extension when the specified
 * MSC has past
 */
static int
ms_present_queue_vblank(RRCrtcPtr crtc,
                        uint64_t event_id,
                        uint64_t msc)
{
    xf86CrtcPtr xf86_crtc = crtc->devPrivate;
    struct ms_present_vblank_event *event;
    uint32_t seq;

    event = calloc(sizeof(struct ms_present_vblank_event), 1);
    if (!event)
        return BadAlloc;
    event->event_id = event_id;
    seq = ms_drm_queue_alloc(xf86_crtc, event,
                             ms_present_vblank_handler,
                             ms_present_vblank_abort);
    if (!seq) {
        free(event);
        return BadAlloc;
    }

    if (!ms_queue_vblank(xf86_crtc, MS_QUEUE_ABSOLUTE, msc, NULL, seq))
        return BadAlloc;

    DebugPresent(("\t\tmq %lld seq %u msc %llu\n",
                 (long long) event_id, seq, (long long) msc));
    return Success;
}

static Bool
ms_present_event_match(void *data, void *match_data)
{
    struct ms_present_vblank_event *event = data;
    uint64_t *match = match_data;

    return *match == event->event_id;
}

/*
 * Remove a pending vblank event from the DRM queue so that it is not reported
 * to the extension
 */
static void
ms_present_abort_vblank(RRCrtcPtr crtc, uint64_t event_id, uint64_t msc)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);

    ms_drm_abort(scrn, ms_present_event_match, &event_id);
}

/*
 * Flush our batch buffer when requested by the Present extension.
 */
static void
ms_present_flush(WindowPtr window)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr screen = window->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (ms->drmmode.glamor)
        ms->glamor.block_handler(screen);
#endif
}

#ifdef GLAMOR_HAS_GBM

/**
 * Callback for the DRM event queue when a flip has completed on all pipes
 *
 * Notify the extension code
 */
static void
ms_present_flip_handler(modesettingPtr ms, uint64_t msc,
                        uint64_t ust, void *data)
{
    struct ms_present_vblank_event *event = data;

    DebugPresent(("\t\tms:fc %lld msc %llu ust %llu\n",
                  (long long) event->event_id,
                  (long long) msc, (long long) ust));

    if (event->unflip)
        ms->drmmode.present_flipping = FALSE;

    ms_present_vblank_handler(msc, ust, event);
}

/*
 * Callback for the DRM queue abort code.  A flip has been aborted.
 */
static void
ms_present_flip_abort(modesettingPtr ms, void *data)
{
    struct ms_present_vblank_event *event = data;

    DebugPresent(("\t\tms:fa %lld\n", (long long) event->event_id));

    free(event);
}

/*
 * Test to see if page flipping is possible on the target crtc
 *
 * We ignore sw-cursors when *disabling* flipping, we may very well be
 * returning to scanning out the normal framebuffer *because* we just
 * switched to sw-cursor mode and check_flip just failed because of that.
 */
static Bool
ms_present_check_unflip(RRCrtcPtr crtc,
                        WindowPtr window,
                        PixmapPtr pixmap,
                        Bool sync_flip,
                        PresentFlipReason *reason)
{
    ScreenPtr screen = window->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    int num_crtcs_on = 0;
    int i;
    struct gbm_bo *gbm;

    if (!ms->drmmode.pageflip)
        return FALSE;

    if (ms->drmmode.dri2_flipping)
        return FALSE;

    if (!scrn->vtSema)
        return FALSE;

    for (i = 0; i < config->num_crtc; i++) {
        drmmode_crtc_private_ptr drmmode_crtc = config->crtc[i]->driver_private;

        /* Don't do pageflipping if CRTCs are rotated. */
        if (drmmode_crtc->rotate_bo.gbm)
            return FALSE;

        if (xf86_crtc_on(config->crtc[i]))
            num_crtcs_on++;
    }

    /* We can't do pageflipping if all the CRTCs are off. */
    if (num_crtcs_on == 0)
        return FALSE;

    /*
     * Check stride, can't change that reliably on flip on some drivers, unless
     * the kms driver is atomic_modeset_capable.
     */
    if (!ms->atomic_modeset_capable &&
        pixmap->devKind != drmmode_bo_get_pitch(&ms->drmmode.front_bo))
        return FALSE;

    if (!ms->drmmode.glamor)
        return FALSE;

#ifdef GBM_BO_WITH_MODIFIERS
    /* Check if buffer format/modifier is supported by all active CRTCs */
    gbm = ms->glamor.gbm_bo_from_pixmap(screen, pixmap);
    if (gbm) {
        uint32_t format;
        uint64_t modifier;

        format = gbm_bo_get_format(gbm);
        modifier = gbm_bo_get_modifier(gbm);
        gbm_bo_destroy(gbm);

        if (!drmmode_is_format_supported(scrn, format, modifier)) {
            if (reason)
                *reason = PRESENT_FLIP_REASON_BUFFER_FORMAT;
            return FALSE;
        }
    }
#endif

    /* Make sure there's a bo we can get to */
    /* XXX: actually do this.  also...is it sufficient?
     * if (!glamor_get_pixmap_private(pixmap))
     *     return FALSE;
     */

    return TRUE;
}

static Bool
ms_present_check_flip(RRCrtcPtr crtc,
                      WindowPtr window,
                      PixmapPtr pixmap,
                      Bool sync_flip,
                      PresentFlipReason *reason)
{
    ScreenPtr screen = window->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (ms->drmmode.sprites_visible > 0)
        return FALSE;

    if(!ms_present_check_unflip(crtc, window, pixmap, sync_flip, reason))
        return FALSE;

    ms->flip_window = window;

    return TRUE;
}

/*
 * Queue a flip on 'crtc' to 'pixmap' at 'target_msc'. If 'sync_flip' is true,
 * then wait for vblank. Otherwise, flip immediately
 */
static Bool
ms_present_flip(RRCrtcPtr crtc,
                uint64_t event_id,
                uint64_t target_msc,
                PixmapPtr pixmap,
                Bool sync_flip)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcPtr xf86_crtc = crtc->devPrivate;
    drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;
    Bool ret;
    struct ms_present_vblank_event *event;

    if (!ms_present_check_flip(crtc, ms->flip_window, pixmap, sync_flip, NULL))
        return FALSE;

    event = calloc(1, sizeof(struct ms_present_vblank_event));
    if (!event)
        return FALSE;

    DebugPresent(("\t\tms:pf %lld msc %llu\n",
                  (long long) event_id, (long long) target_msc));

    event->event_id = event_id;
    event->unflip = FALSE;

    /* A window can only flip if it covers the entire X screen.
     * Only one window can flip at a time.
     *
     * If the window also has the variable refresh property then
     * variable refresh supported can be enabled on every CRTC.
     */
    if (ms->vrr_support && ms->is_connector_vrr_capable &&
          ms_window_has_variable_refresh(ms, ms->flip_window)) {
        ms_present_set_screen_vrr(scrn, TRUE);
    }

    ret = ms_do_pageflip(screen, pixmap, event, drmmode_crtc->vblank_pipe, !sync_flip,
                         ms_present_flip_handler, ms_present_flip_abort,
                         "Present-flip");
    if (ret)
        ms->drmmode.present_flipping = TRUE;

    return ret;
}

/*
 * Queue a flip back to the normal frame buffer
 */
static void
ms_present_unflip(ScreenPtr screen, uint64_t event_id)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    PixmapPtr pixmap = screen->GetScreenPixmap(screen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    int i;
    struct ms_present_vblank_event *event;

    ms_present_set_screen_vrr(scrn, FALSE);

    event = calloc(1, sizeof(struct ms_present_vblank_event));
    if (!event)
        return;

    event->event_id = event_id;
    event->unflip = TRUE;

    if (ms_present_check_unflip(NULL, screen->root, pixmap, TRUE, NULL) &&
        ms_do_pageflip(screen, pixmap, event, -1, FALSE,
                       ms_present_flip_handler, ms_present_flip_abort,
                       "Present-unflip")) {
        return;
    }

    for (i = 0; i < config->num_crtc; i++) {
        xf86CrtcPtr crtc = config->crtc[i];
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	if (!crtc->enabled)
	    continue;

	/* info->drmmode.fb_id still points to the FB for the last flipped BO.
	 * Clear it, drmmode_set_mode_major will re-create it
	 */
	if (drmmode_crtc->drmmode->fb_id) {
		drmModeRmFB(drmmode_crtc->drmmode->fd,
			    drmmode_crtc->drmmode->fb_id);
		drmmode_crtc->drmmode->fb_id = 0;
	}

	if (drmmode_crtc->dpms_mode == DPMSModeOn)
	    crtc->funcs->set_mode_major(crtc, &crtc->mode, crtc->rotation,
					crtc->x, crtc->y);
	else
	    drmmode_crtc->need_modeset = TRUE;
    }

    present_event_notify(event_id, 0, 0);
    ms->drmmode.present_flipping = FALSE;
}
#endif

static present_screen_info_rec ms_present_screen_info = {
    .version = PRESENT_SCREEN_INFO_VERSION,

    .get_crtc = ms_present_get_crtc,
    .get_ust_msc = ms_present_get_ust_msc,
    .queue_vblank = ms_present_queue_vblank,
    .abort_vblank = ms_present_abort_vblank,
    .flush = ms_present_flush,

    .capabilities = PresentCapabilityNone,
#ifdef GLAMOR_HAS_GBM
    .check_flip = NULL,
    .check_flip2 = ms_present_check_flip,
    .flip = ms_present_flip,
    .unflip = ms_present_unflip,
#endif
};

Bool
ms_present_screen_init(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    uint64_t value;
    int ret;

    ret = drmGetCap(ms->fd, DRM_CAP_ASYNC_PAGE_FLIP, &value);
    if (ret == 0 && value == 1) {
        ms_present_screen_info.capabilities |= PresentCapabilityAsync;
        ms->drmmode.can_async_flip = TRUE;
    }

    return present_screen_init(screen, &ms_present_screen_info);
}
