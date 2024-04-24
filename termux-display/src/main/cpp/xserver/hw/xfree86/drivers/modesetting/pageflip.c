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

#include <xserver_poll.h>
#include <xf86drm.h>

#include "driver.h"

/*
 * Flush the DRM event queue when full; makes space for new events.
 *
 * Returns a negative value on error, 0 if there was nothing to process,
 * or 1 if we handled any events.
 */
int
ms_flush_drm_events(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    struct pollfd p = { .fd = ms->fd, .events = POLLIN };
    int r;

    do {
            r = xserver_poll(&p, 1, 0);
    } while (r == -1 && (errno == EINTR || errno == EAGAIN));

    /* If there was an error, r will be < 0.  Return that.  If there was
     * nothing to process, r == 0.  Return that.
     */
    if (r <= 0)
        return r;

    /* Try to handle the event.  If there was an error, return it. */
    r = drmHandleEvent(ms->fd, &ms->event_context);
    if (r < 0)
        return r;

    /* Otherwise return 1 to indicate that we handled an event. */
    return 1;
}

#ifdef GLAMOR_HAS_GBM

/*
 * Event data for an in progress flip.
 * This contains a pointer to the vblank event,
 * and information about the flip in progress.
 * a reference to this is stored in the per-crtc
 * flips.
 */
struct ms_flipdata {
    ScreenPtr screen;
    void *event;
    ms_pageflip_handler_proc event_handler;
    ms_pageflip_abort_proc abort_handler;
    /* number of CRTC events referencing this */
    int flip_count;
    uint64_t fe_msc;
    uint64_t fe_usec;
    uint32_t old_fb_id;
};

/*
 * Per crtc pageflipping information,
 * These are submitted to the queuing code
 * one of them per crtc per flip.
 */
struct ms_crtc_pageflip {
    Bool on_reference_crtc;
    /* reference to the ms_flipdata */
    struct ms_flipdata *flipdata;
};

/**
 * Free an ms_crtc_pageflip.
 *
 * Drops the reference count on the flipdata.
 */
static void
ms_pageflip_free(struct ms_crtc_pageflip *flip)
{
    struct ms_flipdata *flipdata = flip->flipdata;

    free(flip);
    if (--flipdata->flip_count > 0)
        return;
    free(flipdata);
}

/**
 * Callback for the DRM event queue when a single flip has completed
 *
 * Once the flip has been completed on all pipes, notify the
 * extension code telling it when that happened
 */
static void
ms_pageflip_handler(uint64_t msc, uint64_t ust, void *data)
{
    struct ms_crtc_pageflip *flip = data;
    struct ms_flipdata *flipdata = flip->flipdata;
    ScreenPtr screen = flipdata->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (flip->on_reference_crtc) {
        flipdata->fe_msc = msc;
        flipdata->fe_usec = ust;
    }

    if (flipdata->flip_count == 1) {
        flipdata->event_handler(ms, flipdata->fe_msc,
                                flipdata->fe_usec,
                                flipdata->event);

        drmModeRmFB(ms->fd, flipdata->old_fb_id);
    }
    ms_pageflip_free(flip);
}

/*
 * Callback for the DRM queue abort code.  A flip has been aborted.
 */
static void
ms_pageflip_abort(void *data)
{
    struct ms_crtc_pageflip *flip = data;
    struct ms_flipdata *flipdata = flip->flipdata;
    ScreenPtr screen = flipdata->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (flipdata->flip_count == 1)
        flipdata->abort_handler(ms, flipdata->event);

    ms_pageflip_free(flip);
}

static Bool
do_queue_flip_on_crtc(modesettingPtr ms, xf86CrtcPtr crtc,
                      uint32_t flags, uint32_t seq)
{
    return drmmode_crtc_flip(crtc, ms->drmmode.fb_id, flags,
                             (void *) (uintptr_t) seq);
}

enum queue_flip_status {
    QUEUE_FLIP_SUCCESS,
    QUEUE_FLIP_ALLOC_FAILED,
    QUEUE_FLIP_QUEUE_ALLOC_FAILED,
    QUEUE_FLIP_DRM_FLUSH_FAILED,
};

static int
queue_flip_on_crtc(ScreenPtr screen, xf86CrtcPtr crtc,
                   struct ms_flipdata *flipdata,
                   int ref_crtc_vblank_pipe, uint32_t flags)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    struct ms_crtc_pageflip *flip;
    uint32_t seq;

    flip = calloc(1, sizeof(struct ms_crtc_pageflip));
    if (flip == NULL) {
        return QUEUE_FLIP_ALLOC_FAILED;
    }

    /* Only the reference crtc will finally deliver its page flip
     * completion event. All other crtc's events will be discarded.
     */
    flip->on_reference_crtc = (drmmode_crtc->vblank_pipe == ref_crtc_vblank_pipe);
    flip->flipdata = flipdata;

    seq = ms_drm_queue_alloc(crtc, flip, ms_pageflip_handler, ms_pageflip_abort);
    if (!seq) {
        free(flip);
        return QUEUE_FLIP_QUEUE_ALLOC_FAILED;
    }

    /* take a reference on flipdata for use in flip */
    flipdata->flip_count++;

    while (do_queue_flip_on_crtc(ms, crtc, flags, seq)) {
        /* We may have failed because the event queue was full.  Flush it
         * and retry.  If there was nothing to flush, then we failed for
         * some other reason and should just return an error.
         */
        if (ms_flush_drm_events(screen) <= 0) {
            /* Aborting will also decrement flip_count and free(flip). */
            ms_drm_abort_seq(scrn, seq);
            return QUEUE_FLIP_DRM_FLUSH_FAILED;
        }

        /* We flushed some events, so try again. */
        xf86DrvMsg(scrn->scrnIndex, X_WARNING, "flip queue retry\n");
    }

    /* The page flip succeeded. */
    return QUEUE_FLIP_SUCCESS;
}


#define MS_ASYNC_FLIP_LOG_ENABLE_LOGS_INTERVAL_MS 10000
#define MS_ASYNC_FLIP_LOG_FREQUENT_LOGS_INTERVAL_MS 1000
#define MS_ASYNC_FLIP_FREQUENT_LOG_COUNT 10

static void
ms_print_pageflip_error(int screen_index, const char *log_prefix,
                        int crtc_index, int flags, int err)
{
    /* In certain circumstances we will have a lot of flip errors without a
     * reasonable way to prevent them. In such case we reduce the number of
     * logged messages to at least not fill the error logs.
     *
     * The details are as follows:
     *
     * At least on i915 hardware support for async page flip support depends
     * on the used modifiers which themselves can change dynamically for a
     * screen. This results in the following problems:
     *
     *  - We can't know about whether a particular CRTC will be able to do an
     *    async flip without hardcoding the same logic as the kernel as there's
     *    no interface to query this information.
     *
     *  - There is no way to give this information to an application, because
     *    the protocol of the present extension does not specify anything about
     *    changing of the capabilities on runtime or the need to re-query them.
     *
     * Even if the above was solved, the only benefit would be avoiding a
     * roundtrip to the kernel and reduced amount of error logs. The former
     * does not seem to be a good enough benefit compared to the amount of work
     * that would need to be done. The latter is solved below. */

    static CARD32 error_last_time_ms;
    static int frequent_logs;
    static Bool logs_disabled;

    if (flags & DRM_MODE_PAGE_FLIP_ASYNC) {
        CARD32 curr_time_ms = GetTimeInMillis();
        int clocks_since_last_log = curr_time_ms - error_last_time_ms;

        if (clocks_since_last_log >
                MS_ASYNC_FLIP_LOG_ENABLE_LOGS_INTERVAL_MS) {
            frequent_logs = 0;
            logs_disabled = FALSE;
        }
        if (!logs_disabled) {
            if (clocks_since_last_log <
                    MS_ASYNC_FLIP_LOG_FREQUENT_LOGS_INTERVAL_MS) {
                frequent_logs++;
            }

            if (frequent_logs > MS_ASYNC_FLIP_FREQUENT_LOG_COUNT) {
                xf86DrvMsg(screen_index, X_WARNING,
                           "%s: detected too frequent flip errors, disabling "
                           "logs until frequency is reduced\n", log_prefix);
                logs_disabled = TRUE;
            } else {
                xf86DrvMsg(screen_index, X_WARNING,
                           "%s: queue async flip during flip on CRTC %d failed: %s\n",
                           log_prefix, crtc_index, strerror(err));
            }
        }
        error_last_time_ms = curr_time_ms;
    } else {
        xf86DrvMsg(screen_index, X_WARNING,
                   "%s: queue flip during flip on CRTC %d failed: %s\n",
                   log_prefix, crtc_index, strerror(err));
    }
}


Bool
ms_do_pageflip(ScreenPtr screen,
               PixmapPtr new_front,
               void *event,
               int ref_crtc_vblank_pipe,
               Bool async,
               ms_pageflip_handler_proc pageflip_handler,
               ms_pageflip_abort_proc pageflip_abort,
               const char *log_prefix)
{
#ifndef GLAMOR_HAS_GBM
    return FALSE;
#else
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    drmmode_bo new_front_bo;
    uint32_t flags;
    int i;
    struct ms_flipdata *flipdata;
    ms->glamor.block_handler(screen);

    new_front_bo.gbm = ms->glamor.gbm_bo_from_pixmap(screen, new_front);
    new_front_bo.dumb = NULL;

    if (!new_front_bo.gbm) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "%s: Failed to get GBM BO for flip to new front.\n",
                   log_prefix);
        return FALSE;
    }

    flipdata = calloc(1, sizeof(struct ms_flipdata));
    if (!flipdata) {
        drmmode_bo_destroy(&ms->drmmode, &new_front_bo);
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "%s: Failed to allocate flipdata.\n", log_prefix);
        return FALSE;
    }

    flipdata->event = event;
    flipdata->screen = screen;
    flipdata->event_handler = pageflip_handler;
    flipdata->abort_handler = pageflip_abort;

    /*
     * Take a local reference on flipdata.
     * if the first flip fails, the sequence abort
     * code will free the crtc flip data, and drop
     * its reference which would cause this to be
     * freed when we still required it.
     */
    flipdata->flip_count++;

    /* Create a new handle for the back buffer */
    flipdata->old_fb_id = ms->drmmode.fb_id;

    new_front_bo.width = new_front->drawable.width;
    new_front_bo.height = new_front->drawable.height;
    if (drmmode_bo_import(&ms->drmmode, &new_front_bo,
                          &ms->drmmode.fb_id)) {
        if (!ms->drmmode.flip_bo_import_failed) {
            xf86DrvMsg(scrn->scrnIndex, X_WARNING, "%s: Import BO failed: %s\n",
                       log_prefix, strerror(errno));
            ms->drmmode.flip_bo_import_failed = TRUE;
        }
        goto error_out;
    } else {
        if (ms->drmmode.flip_bo_import_failed &&
            new_front != screen->GetScreenPixmap(screen))
            ms->drmmode.flip_bo_import_failed = FALSE;
    }

    /* Queue flips on all enabled CRTCs.
     *
     * Note that if/when we get per-CRTC buffers, we'll have to update this.
     * Right now it assumes a single shared fb across all CRTCs, with the
     * kernel fixing up the offset of each CRTC as necessary.
     *
     * Also, flips queued on disabled or incorrectly configured displays
     * may never complete; this is a configuration error.
     */
    for (i = 0; i < config->num_crtc; i++) {
        enum queue_flip_status flip_status;
        xf86CrtcPtr crtc = config->crtc[i];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        if (!xf86_crtc_on(crtc))
            continue;

        flags = DRM_MODE_PAGE_FLIP_EVENT;
        if (ms->drmmode.can_async_flip && async)
            flags |= DRM_MODE_PAGE_FLIP_ASYNC;

        /*
         * If this is not the reference crtc used for flip timing and flip event
         * delivery and timestamping, ie. not the one whose presentation timing
         * we do really care about, and async flips are possible, and requested
         * by an xorg.conf option, then we flip this "secondary" crtc without
         * sync to vblank. This may cause tearing on such "secondary" outputs,
         * but it will prevent throttling of multi-display flips to the refresh
         * cycle of any of the secondary crtcs, avoiding periodic slowdowns and
         * judder caused by unsynchronized outputs. This is especially useful for
         * outputs in a "clone-mode" or "mirror-mode" configuration.
         */
        if (ms->drmmode.can_async_flip && ms->drmmode.async_flip_secondaries &&
            (drmmode_crtc->vblank_pipe != ref_crtc_vblank_pipe) &&
            (ref_crtc_vblank_pipe >= 0))
            flags |= DRM_MODE_PAGE_FLIP_ASYNC;

        flip_status = queue_flip_on_crtc(screen, crtc, flipdata,
                                         ref_crtc_vblank_pipe,
                                         flags);

        switch (flip_status) {
            case QUEUE_FLIP_ALLOC_FAILED:
                xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                           "%s: carrier alloc for queue flip on CRTC %d failed.\n",
                           log_prefix, i);
                goto error_undo;
            case QUEUE_FLIP_QUEUE_ALLOC_FAILED:
                xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                           "%s: entry alloc for queue flip on CRTC %d failed.\n",
                           log_prefix, i);
                goto error_undo;
            case QUEUE_FLIP_DRM_FLUSH_FAILED:
                ms_print_pageflip_error(scrn->scrnIndex, log_prefix, i, flags, errno);
                goto error_undo;
            case QUEUE_FLIP_SUCCESS:
                break;
        }
    }

    drmmode_bo_destroy(&ms->drmmode, &new_front_bo);

    /*
     * Do we have more than our local reference,
     * if so and no errors, then drop our local
     * reference and return now.
     */
    if (flipdata->flip_count > 1) {
        flipdata->flip_count--;
        return TRUE;
    }

error_undo:

    /*
     * Have we just got the local reference?
     * free the framebuffer if so since nobody successfully
     * submitted anything
     */
    if (flipdata->flip_count == 1) {
        drmModeRmFB(ms->fd, ms->drmmode.fb_id);
        ms->drmmode.fb_id = flipdata->old_fb_id;
    }

error_out:
    drmmode_bo_destroy(&ms->drmmode, &new_front_bo);
    /* if only the local reference - free the structure,
     * else drop the local reference and return */
    if (flipdata->flip_count == 1)
        free(flipdata);
    else
        flipdata->flip_count--;

    return FALSE;
#endif /* GLAMOR_HAS_GBM */
}

#endif
