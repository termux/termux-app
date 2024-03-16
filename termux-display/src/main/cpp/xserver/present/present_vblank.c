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

#include "present_priv.h"

void
present_vblank_notify(present_vblank_ptr vblank, CARD8 kind, CARD8 mode, uint64_t ust, uint64_t crtc_msc)
{
    int n;

    if (vblank->window)
        present_send_complete_notify(vblank->window, kind, mode, vblank->serial, ust, crtc_msc - vblank->msc_offset);
    for (n = 0; n < vblank->num_notifies; n++) {
        WindowPtr   window = vblank->notifies[n].window;
        CARD32      serial = vblank->notifies[n].serial;

        if (window)
            present_send_complete_notify(window, kind, mode, serial, ust, crtc_msc - vblank->msc_offset);
    }
}

/* The memory vblank points to must be 0-initialized before calling this function.
 *
 * If this function returns FALSE, present_vblank_destroy must be called to clean
 * up.
 */
Bool
present_vblank_init(present_vblank_ptr vblank,
                    WindowPtr window,
                    PixmapPtr pixmap,
                    CARD32 serial,
                    RegionPtr valid,
                    RegionPtr update,
                    int16_t x_off,
                    int16_t y_off,
                    RRCrtcPtr target_crtc,
                    SyncFence *wait_fence,
                    SyncFence *idle_fence,
                    uint32_t options,
                    const uint32_t capabilities,
                    present_notify_ptr notifies,
                    int num_notifies,
                    uint64_t target_msc,
                    uint64_t crtc_msc)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_window_priv_ptr     window_priv = present_get_window_priv(window, TRUE);
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    PresentFlipReason           reason = PRESENT_FLIP_REASON_UNKNOWN;

    if (target_crtc) {
        screen_priv = present_screen_priv(target_crtc->pScreen);
    }

    xorg_list_append(&vblank->window_list, &window_priv->vblank);
    xorg_list_init(&vblank->event_queue);

    vblank->screen = screen;
    vblank->window = window;
    vblank->pixmap = pixmap;

    if (pixmap) {
        vblank->kind = PresentCompleteKindPixmap;
        pixmap->refcnt++;
    } else
        vblank->kind = PresentCompleteKindNotifyMSC;

    vblank->serial = serial;

    if (valid) {
        vblank->valid = RegionDuplicate(valid);
        if (!vblank->valid)
            goto no_mem;
    }
    if (update) {
        vblank->update = RegionDuplicate(update);
        if (!vblank->update)
            goto no_mem;
    }

    vblank->x_off = x_off;
    vblank->y_off = y_off;
    vblank->target_msc = target_msc;
    vblank->exec_msc = target_msc;
    vblank->crtc = target_crtc;
    vblank->msc_offset = window_priv->msc_offset;
    vblank->notifies = notifies;
    vblank->num_notifies = num_notifies;
    vblank->has_suboptimal = (options & PresentOptionSuboptimal);

    if (pixmap != NULL &&
        !(options & PresentOptionCopy) &&
        screen_priv->check_flip) {
        if (msc_is_after(target_msc, crtc_msc) &&
            screen_priv->check_flip (target_crtc, window, pixmap, TRUE, valid, x_off, y_off, &reason))
        {
            vblank->flip = TRUE;
            vblank->sync_flip = TRUE;
        } else if ((capabilities & PresentCapabilityAsync) &&
            screen_priv->check_flip (target_crtc, window, pixmap, FALSE, valid, x_off, y_off, &reason))
        {
            vblank->flip = TRUE;
        }
    }
    vblank->reason = reason;

    if (wait_fence) {
        vblank->wait_fence = present_fence_create(wait_fence);
        if (!vblank->wait_fence)
            goto no_mem;
    }

    if (idle_fence) {
        vblank->idle_fence = present_fence_create(idle_fence);
        if (!vblank->idle_fence)
            goto no_mem;
    }

    if (pixmap)
        DebugPresent(("q %" PRIu64 " %p %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 " (crtc %p) flip %d vsync %d serial %d\n",
                      vblank->event_id, vblank, target_msc,
                      vblank->pixmap->drawable.id, vblank->window->drawable.id,
                      target_crtc, vblank->flip, vblank->sync_flip, vblank->serial));
    return TRUE;

no_mem:
    vblank->notifies = NULL;
    return FALSE;
}

present_vblank_ptr
present_vblank_create(WindowPtr window,
                      PixmapPtr pixmap,
                      CARD32 serial,
                      RegionPtr valid,
                      RegionPtr update,
                      int16_t x_off,
                      int16_t y_off,
                      RRCrtcPtr target_crtc,
                      SyncFence *wait_fence,
                      SyncFence *idle_fence,
                      uint32_t options,
                      const uint32_t capabilities,
                      present_notify_ptr notifies,
                      int num_notifies,
                      uint64_t target_msc,
                      uint64_t crtc_msc)
{
    present_vblank_ptr vblank = calloc(1, sizeof(present_vblank_rec));

    if (!vblank)
        return NULL;

    if (present_vblank_init(vblank, window, pixmap, serial, valid, update,
                            x_off, y_off, target_crtc, wait_fence, idle_fence,
                            options, capabilities, notifies, num_notifies,
                            target_msc, crtc_msc))
        return vblank;

    present_vblank_destroy(vblank);
    return NULL;
}

void
present_vblank_scrap(present_vblank_ptr vblank)
{
    DebugPresent(("\tx %" PRIu64 " %p %" PRIu64 " %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 " (crtc %p)\n",
                  vblank->event_id, vblank, vblank->exec_msc, vblank->target_msc,
                  vblank->pixmap->drawable.id, vblank->window->drawable.id,
                  vblank->crtc));

    present_pixmap_idle(vblank->pixmap, vblank->window, vblank->serial, vblank->idle_fence);
    present_fence_destroy(vblank->idle_fence);
    dixDestroyPixmap(vblank->pixmap, vblank->pixmap->drawable.id);

    vblank->pixmap = NULL;
    vblank->idle_fence = NULL;
    vblank->flip = FALSE;
}

void
present_vblank_destroy(present_vblank_ptr vblank)
{
    /* Remove vblank from window and screen lists */
    xorg_list_del(&vblank->window_list);
    /* Also make sure vblank is removed from event queue (wnmd) */
    xorg_list_del(&vblank->event_queue);

    DebugPresent(("\td %" PRIu64 " %p %" PRIu64 " %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                  vblank->event_id, vblank, vblank->exec_msc, vblank->target_msc,
                  vblank->pixmap ? vblank->pixmap->drawable.id : 0,
                  vblank->window ? vblank->window->drawable.id : 0));

    /* Drop pixmap reference */
    if (vblank->pixmap)
        dixDestroyPixmap(vblank->pixmap, vblank->pixmap->drawable.id);

    /* Free regions */
    if (vblank->valid)
        RegionDestroy(vblank->valid);
    if (vblank->update)
        RegionDestroy(vblank->update);

    if (vblank->wait_fence)
        present_fence_destroy(vblank->wait_fence);

    if (vblank->idle_fence)
        present_fence_destroy(vblank->idle_fence);

    if (vblank->notifies)
        present_destroy_notifies(vblank->notifies, vblank->num_notifies);

    free(vblank);
}
