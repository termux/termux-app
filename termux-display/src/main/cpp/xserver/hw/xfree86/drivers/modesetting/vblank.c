/*
 * Copyright © 2013 Keith Packard
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

/** @file vblank.c
 *
 * Support for tracking the DRM's vblank events.
 */

#ifdef HAVE_DIX_CONFIG_H
#include "dix-config.h"
#endif

#include <unistd.h>
#include <xf86.h>
#include <xf86Crtc.h>
#include "driver.h"
#include "drmmode_display.h"

/**
 * Tracking for outstanding events queued to the kernel.
 *
 * Each list entry is a struct ms_drm_queue, which has a uint32_t
 * value generated from drm_seq that identifies the event and a
 * reference back to the crtc/screen associated with the event.  It's
 * done this way rather than in the screen because we want to be able
 * to drain the list of event handlers that should be called at server
 * regen time, even though we don't close the drm fd and have no way
 * to actually drain the kernel events.
 */
static struct xorg_list ms_drm_queue;
static uint32_t ms_drm_seq;

static void box_intersect(BoxPtr dest, BoxPtr a, BoxPtr b)
{
    dest->x1 = a->x1 > b->x1 ? a->x1 : b->x1;
    dest->x2 = a->x2 < b->x2 ? a->x2 : b->x2;
    if (dest->x1 >= dest->x2) {
        dest->x1 = dest->x2 = dest->y1 = dest->y2 = 0;
        return;
    }

    dest->y1 = a->y1 > b->y1 ? a->y1 : b->y1;
    dest->y2 = a->y2 < b->y2 ? a->y2 : b->y2;
    if (dest->y1 >= dest->y2)
        dest->x1 = dest->x2 = dest->y1 = dest->y2 = 0;
}

static void rr_crtc_box(RRCrtcPtr crtc, BoxPtr crtc_box)
{
    if (crtc->mode) {
        crtc_box->x1 = crtc->x;
        crtc_box->y1 = crtc->y;
        switch (crtc->rotation) {
            case RR_Rotate_0:
            case RR_Rotate_180:
            default:
                crtc_box->x2 = crtc->x + crtc->mode->mode.width;
                crtc_box->y2 = crtc->y + crtc->mode->mode.height;
                break;
            case RR_Rotate_90:
            case RR_Rotate_270:
                crtc_box->x2 = crtc->x + crtc->mode->mode.height;
                crtc_box->y2 = crtc->y + crtc->mode->mode.width;
                break;
        }
    } else
        crtc_box->x1 = crtc_box->x2 = crtc_box->y1 = crtc_box->y2 = 0;
}

static int box_area(BoxPtr box)
{
    return (int)(box->x2 - box->x1) * (int)(box->y2 - box->y1);
}

static Bool rr_crtc_on(RRCrtcPtr crtc, Bool crtc_is_xf86_hint)
{
    if (!crtc) {
        return FALSE;
    }
    if (crtc_is_xf86_hint && crtc->devPrivate) {
         return xf86_crtc_on(crtc->devPrivate);
    } else {
        return !!crtc->mode;
    }
}

Bool
xf86_crtc_on(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    return crtc->enabled && drmmode_crtc->dpms_mode == DPMSModeOn;
}


/*
 * Return the crtc covering 'box'. If two crtcs cover a portion of
 * 'box', then prefer the crtc with greater coverage.
 */
static RRCrtcPtr
rr_crtc_covering_box(ScreenPtr pScreen, BoxPtr box, Bool screen_is_xf86_hint)
{
    rrScrPrivPtr pScrPriv;
    RROutputPtr primary_output;
    RRCrtcPtr crtc, best_crtc, primary_crtc;
    int coverage, best_coverage;
    int c;
    BoxRec crtc_box, cover_box;

    best_crtc = NULL;
    best_coverage = 0;

    if (!dixPrivateKeyRegistered(rrPrivKey))
        return NULL;

    pScrPriv = rrGetScrPriv(pScreen);

    if (!pScrPriv)
        return NULL;

    primary_crtc = NULL;
    primary_output = RRFirstOutput(pScreen);
    if (primary_output)
        primary_crtc = primary_output->crtc;

    for (c = 0; c < pScrPriv->numCrtcs; c++) {
        crtc = pScrPriv->crtcs[c];

        /* If the CRTC is off, treat it as not covering */
        if (!rr_crtc_on(crtc, screen_is_xf86_hint))
            continue;

        rr_crtc_box(crtc, &crtc_box);
        box_intersect(&cover_box, &crtc_box, box);
        coverage = box_area(&cover_box);
        if ((coverage > best_coverage) ||
            (coverage == best_coverage && crtc == primary_crtc)) {
            best_crtc = crtc;
            best_coverage = coverage;
        }
    }

    return best_crtc;
}

static RRCrtcPtr
rr_crtc_covering_box_on_secondary(ScreenPtr pScreen, BoxPtr box)
{
    if (!pScreen->isGPU) {
        ScreenPtr secondary;
        RRCrtcPtr crtc = NULL;

        xorg_list_for_each_entry(secondary, &pScreen->secondary_list, secondary_head) {
            if (!secondary->is_output_secondary)
                continue;

            crtc = rr_crtc_covering_box(secondary, box, FALSE);
            if (crtc)
                return crtc;
        }
    }

    return NULL;
}

xf86CrtcPtr
ms_dri2_crtc_covering_drawable(DrawablePtr pDraw)
{
    ScreenPtr pScreen = pDraw->pScreen;
    RRCrtcPtr crtc = NULL;
    BoxRec box;

    box.x1 = pDraw->x;
    box.y1 = pDraw->y;
    box.x2 = box.x1 + pDraw->width;
    box.y2 = box.y1 + pDraw->height;

    crtc = rr_crtc_covering_box(pScreen, &box, TRUE);
    if (crtc) {
        return crtc->devPrivate;
    }
    return NULL;
}

RRCrtcPtr
ms_randr_crtc_covering_drawable(DrawablePtr pDraw)
{
    ScreenPtr pScreen = pDraw->pScreen;
    RRCrtcPtr crtc = NULL;
    BoxRec box;

    box.x1 = pDraw->x;
    box.y1 = pDraw->y;
    box.x2 = box.x1 + pDraw->width;
    box.y2 = box.y1 + pDraw->height;

    crtc = rr_crtc_covering_box(pScreen, &box, TRUE);
    if (!crtc) {
        crtc = rr_crtc_covering_box_on_secondary(pScreen, &box);
    }
    return crtc;
}

static Bool
ms_get_kernel_ust_msc(xf86CrtcPtr crtc,
                      uint64_t *msc, uint64_t *ust)
{
    ScreenPtr screen = crtc->randr_crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmVBlank vbl;
    int ret;

    if (ms->has_queue_sequence || !ms->tried_queue_sequence) {
        uint64_t ns;
        ms->tried_queue_sequence = TRUE;

        ret = drmCrtcGetSequence(ms->fd, drmmode_crtc->mode_crtc->crtc_id,
                                 msc, &ns);
        if (ret != -1 || (errno != ENOTTY && errno != EINVAL)) {
            ms->has_queue_sequence = TRUE;
            if (ret == 0)
                *ust = ns / 1000;
            return ret == 0;
        }
    }
    /* Get current count */
    vbl.request.type = DRM_VBLANK_RELATIVE | drmmode_crtc->vblank_pipe;
    vbl.request.sequence = 0;
    vbl.request.signal = 0;
    ret = drmWaitVBlank(ms->fd, &vbl);
    if (ret) {
        *msc = 0;
        *ust = 0;
        return FALSE;
    } else {
        *msc = vbl.reply.sequence;
        *ust = (CARD64) vbl.reply.tval_sec * 1000000 + vbl.reply.tval_usec;
        return TRUE;
    }
}

Bool
ms_queue_vblank(xf86CrtcPtr crtc, ms_queue_flag flags,
                uint64_t msc, uint64_t *msc_queued, uint32_t seq)
{
    ScreenPtr screen = crtc->randr_crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmVBlank vbl;
    int ret;

    for (;;) {
        /* Queue an event at the specified sequence */
        if (ms->has_queue_sequence || !ms->tried_queue_sequence) {
            uint32_t drm_flags = 0;
            uint64_t kernel_queued;

            ms->tried_queue_sequence = TRUE;

            if (flags & MS_QUEUE_RELATIVE)
                drm_flags |= DRM_CRTC_SEQUENCE_RELATIVE;
            if (flags & MS_QUEUE_NEXT_ON_MISS)
                drm_flags |= DRM_CRTC_SEQUENCE_NEXT_ON_MISS;

            ret = drmCrtcQueueSequence(ms->fd, drmmode_crtc->mode_crtc->crtc_id,
                                       drm_flags, msc, &kernel_queued, seq);
            if (ret == 0) {
                if (msc_queued)
                    *msc_queued = ms_kernel_msc_to_crtc_msc(crtc, kernel_queued, TRUE);
                ms->has_queue_sequence = TRUE;
                return TRUE;
            }

            if (ret != -1 || (errno != ENOTTY && errno != EINVAL)) {
                ms->has_queue_sequence = TRUE;
                goto check;
            }
        }
        vbl.request.type = DRM_VBLANK_EVENT | drmmode_crtc->vblank_pipe;
        if (flags & MS_QUEUE_RELATIVE)
            vbl.request.type |= DRM_VBLANK_RELATIVE;
        else
            vbl.request.type |= DRM_VBLANK_ABSOLUTE;
        if (flags & MS_QUEUE_NEXT_ON_MISS)
            vbl.request.type |= DRM_VBLANK_NEXTONMISS;

        vbl.request.sequence = msc;
        vbl.request.signal = seq;
        ret = drmWaitVBlank(ms->fd, &vbl);
        if (ret == 0) {
            if (msc_queued)
                *msc_queued = ms_kernel_msc_to_crtc_msc(crtc, vbl.reply.sequence, FALSE);
            return TRUE;
        }
    check:
        if (errno != EBUSY) {
            ms_drm_abort_seq(scrn, seq);
            return FALSE;
        }
        ms_flush_drm_events(screen);
    }
}

/**
 * Convert a 32-bit or 64-bit kernel MSC sequence number to a 64-bit local
 * sequence number, adding in the high 32 bits, and dealing with 32-bit
 * wrapping if needed.
 */
uint64_t
ms_kernel_msc_to_crtc_msc(xf86CrtcPtr crtc, uint64_t sequence, Bool is64bit)
{
    drmmode_crtc_private_rec *drmmode_crtc = crtc->driver_private;

    if (!is64bit) {
        /* sequence is provided as a 32 bit value from one of the 32 bit apis,
         * e.g., drmWaitVBlank(), classic vblank events, or pageflip events.
         *
         * Track and handle 32-Bit wrapping, somewhat robust against occasional
         * out-of-order not always monotonically increasing sequence values.
         */
        if ((int64_t) sequence < ((int64_t) drmmode_crtc->msc_prev - 0x40000000))
            drmmode_crtc->msc_high += 0x100000000L;

        if ((int64_t) sequence > ((int64_t) drmmode_crtc->msc_prev + 0x40000000))
            drmmode_crtc->msc_high -= 0x100000000L;

        drmmode_crtc->msc_prev = sequence;

        return drmmode_crtc->msc_high + sequence;
    }

    /* True 64-Bit sequence from Linux 4.15+ 64-Bit drmCrtcGetSequence /
     * drmCrtcQueueSequence apis and events. Pass through sequence unmodified,
     * but update the 32-bit tracking variables with reliable ground truth.
     *
     * With 64-Bit api in use, the only !is64bit input is from pageflip events,
     * and any pageflip event is usually preceded by some is64bit input from
     * swap scheduling, so this should provide reliable mapping for pageflip
     * events based on true 64-bit input as baseline as well.
     */
    drmmode_crtc->msc_prev = sequence;
    drmmode_crtc->msc_high = sequence & 0xffffffff00000000;

    return sequence;
}

int
ms_get_crtc_ust_msc(xf86CrtcPtr crtc, CARD64 *ust, CARD64 *msc)
{
    ScreenPtr screen = crtc->randr_crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    uint64_t kernel_msc;

    if (!ms_get_kernel_ust_msc(crtc, &kernel_msc, ust))
        return BadMatch;
    *msc = ms_kernel_msc_to_crtc_msc(crtc, kernel_msc, ms->has_queue_sequence);

    return Success;
}

/**
 * Check for pending DRM events and process them.
 */
static void
ms_drm_socket_handler(int fd, int ready, void *data)
{
    ScreenPtr screen = data;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (data == NULL)
        return;

    drmHandleEvent(fd, &ms->event_context);
}

/*
 * Enqueue a potential drm response; when the associated response
 * appears, we've got data to pass to the handler from here
 */
uint32_t
ms_drm_queue_alloc(xf86CrtcPtr crtc,
                   void *data,
                   ms_drm_handler_proc handler,
                   ms_drm_abort_proc abort)
{
    ScreenPtr screen = crtc->randr_crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    struct ms_drm_queue *q;

    q = calloc(1, sizeof(struct ms_drm_queue));

    if (!q)
        return 0;
    if (!ms_drm_seq)
        ++ms_drm_seq;
    q->seq = ms_drm_seq++;
    q->scrn = scrn;
    q->crtc = crtc;
    q->data = data;
    q->handler = handler;
    q->abort = abort;

    xorg_list_add(&q->list, &ms_drm_queue);

    return q->seq;
}

/**
 * Abort one queued DRM entry, removing it
 * from the list, calling the abort function and
 * freeing the memory
 */
static void
ms_drm_abort_one(struct ms_drm_queue *q)
{
        xorg_list_del(&q->list);
        q->abort(q->data);
        free(q);
}

/**
 * Abort all queued entries on a specific scrn, used
 * when resetting the X server
 */
static void
ms_drm_abort_scrn(ScrnInfoPtr scrn)
{
    struct ms_drm_queue *q, *tmp;

    xorg_list_for_each_entry_safe(q, tmp, &ms_drm_queue, list) {
        if (q->scrn == scrn)
            ms_drm_abort_one(q);
    }
}

/**
 * Abort by drm queue sequence number.
 */
void
ms_drm_abort_seq(ScrnInfoPtr scrn, uint32_t seq)
{
    struct ms_drm_queue *q, *tmp;

    xorg_list_for_each_entry_safe(q, tmp, &ms_drm_queue, list) {
        if (q->seq == seq) {
            ms_drm_abort_one(q);
            break;
        }
    }
}

/*
 * Externally usable abort function that uses a callback to match a single
 * queued entry to abort
 */
void
ms_drm_abort(ScrnInfoPtr scrn, Bool (*match)(void *data, void *match_data),
             void *match_data)
{
    struct ms_drm_queue *q;

    xorg_list_for_each_entry(q, &ms_drm_queue, list) {
        if (match(q->data, match_data)) {
            ms_drm_abort_one(q);
            break;
        }
    }
}

/*
 * General DRM kernel handler. Looks for the matching sequence number in the
 * drm event queue and calls the handler for it.
 */
static void
ms_drm_sequence_handler(int fd, uint64_t frame, uint64_t ns, Bool is64bit, uint64_t user_data)
{
    struct ms_drm_queue *q, *tmp;
    uint32_t seq = (uint32_t) user_data;

    xorg_list_for_each_entry_safe(q, tmp, &ms_drm_queue, list) {
        if (q->seq == seq) {
            uint64_t msc;

            msc = ms_kernel_msc_to_crtc_msc(q->crtc, frame, is64bit);
            xorg_list_del(&q->list);
            q->handler(msc, ns / 1000, q->data);
            free(q);
            break;
        }
    }
}

static void
ms_drm_sequence_handler_64bit(int fd, uint64_t frame, uint64_t ns, uint64_t user_data)
{
    /* frame is true 64 bit wrapped into 64 bit */
    ms_drm_sequence_handler(fd, frame, ns, TRUE, user_data);
}

static void
ms_drm_handler(int fd, uint32_t frame, uint32_t sec, uint32_t usec,
               void *user_ptr)
{
    /* frame is 32 bit wrapped into 64 bit */
    ms_drm_sequence_handler(fd, frame, ((uint64_t) sec * 1000000 + usec) * 1000,
                            FALSE, (uint32_t) (uintptr_t) user_ptr);
}

Bool
ms_vblank_screen_init(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    modesettingEntPtr ms_ent = ms_ent_priv(scrn);
    xorg_list_init(&ms_drm_queue);

    ms->event_context.version = 4;
    ms->event_context.vblank_handler = ms_drm_handler;
    ms->event_context.page_flip_handler = ms_drm_handler;
    ms->event_context.sequence_handler = ms_drm_sequence_handler_64bit;

    /* We need to re-register the DRM fd for the synchronisation
     * feedback on every server generation, so perform the
     * registration within ScreenInit and not PreInit.
     */
    if (ms_ent->fd_wakeup_registered != serverGeneration) {
        SetNotifyFd(ms->fd, ms_drm_socket_handler, X_NOTIFY_READ, screen);
        ms_ent->fd_wakeup_registered = serverGeneration;
        ms_ent->fd_wakeup_ref = 1;
    } else
        ms_ent->fd_wakeup_ref++;

    return TRUE;
}

void
ms_vblank_close_screen(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    modesettingEntPtr ms_ent = ms_ent_priv(scrn);

    ms_drm_abort_scrn(scrn);

    if (ms_ent->fd_wakeup_registered == serverGeneration &&
        !--ms_ent->fd_wakeup_ref) {
        RemoveNotifyFd(ms->fd);
    }
}
