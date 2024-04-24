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
#include <misync.h>
#include <misyncstr.h>
#ifdef MONOTONIC_CLOCK
#include <time.h>
#endif

/*
 * Screen flip mode
 *
 * Provides the default mode for drivers, that do not
 * support flips and the full screen flip mode.
 *
 */

static uint64_t present_scmd_event_id;

static struct xorg_list present_exec_queue;
static struct xorg_list present_flip_queue;

static void
present_execute(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc);

static inline PixmapPtr
present_flip_pending_pixmap(ScreenPtr screen)
{
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    if (!screen_priv)
        return NULL;

    if (!screen_priv->flip_pending)
        return NULL;

    return screen_priv->flip_pending->pixmap;
}

static Bool
present_check_flip(RRCrtcPtr            crtc,
                   WindowPtr            window,
                   PixmapPtr            pixmap,
                   Bool                 sync_flip,
                   RegionPtr            valid,
                   int16_t              x_off,
                   int16_t              y_off,
                   PresentFlipReason   *reason)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    PixmapPtr                   window_pixmap;
    WindowPtr                   root = screen->root;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    if (crtc) {
       screen_priv = present_screen_priv(crtc->pScreen);
    }
    if (reason)
        *reason = PRESENT_FLIP_REASON_UNKNOWN;

    if (!screen_priv)
        return FALSE;

    if (!screen_priv->info)
        return FALSE;

    if (!crtc)
        return FALSE;

    /* Check to see if the driver supports flips at all */
    if (!screen_priv->info->flip)
        return FALSE;

    /* Make sure the window hasn't been redirected with Composite */
    window_pixmap = screen->GetWindowPixmap(window);
    if (window_pixmap != screen->GetScreenPixmap(screen) &&
        window_pixmap != screen_priv->flip_pixmap &&
        window_pixmap != present_flip_pending_pixmap(screen))
        return FALSE;

    /* Check for full-screen window */
    if (!RegionEqual(&window->clipList, &root->winSize)) {
        return FALSE;
    }

    /* Source pixmap must align with window exactly */
    if (x_off || y_off) {
        return FALSE;
    }

    /* Make sure the area marked as valid fills the screen */
    if (valid && !RegionEqual(valid, &root->winSize)) {
        return FALSE;
    }

    /* Does the window match the pixmap exactly? */
    if (window->drawable.x != 0 || window->drawable.y != 0 ||
#ifdef COMPOSITE
        window->drawable.x != pixmap->screen_x || window->drawable.y != pixmap->screen_y ||
#endif
        window->drawable.width != pixmap->drawable.width ||
        window->drawable.height != pixmap->drawable.height) {
        return FALSE;
    }

    /* Ask the driver for permission */
    if (screen_priv->info->version >= 1 && screen_priv->info->check_flip2) {
        if (!(*screen_priv->info->check_flip2) (crtc, window, pixmap, sync_flip, reason)) {
            DebugPresent(("\td %08" PRIx32 " -> %08" PRIx32 "\n", window->drawable.id, pixmap ? pixmap->drawable.id : 0));
            return FALSE;
        }
    } else if (screen_priv->info->check_flip) {
        if (!(*screen_priv->info->check_flip) (crtc, window, pixmap, sync_flip)) {
            DebugPresent(("\td %08" PRIx32 " -> %08" PRIx32 "\n", window->drawable.id, pixmap ? pixmap->drawable.id : 0));
            return FALSE;
        }
    }

    return TRUE;
}

static Bool
present_flip(RRCrtcPtr crtc,
             uint64_t event_id,
             uint64_t target_msc,
             PixmapPtr pixmap,
             Bool sync_flip)
{
    ScreenPtr                   screen = crtc->pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    return (*screen_priv->info->flip) (crtc, event_id, target_msc, pixmap, sync_flip);
}

static RRCrtcPtr
present_scmd_get_crtc(present_screen_priv_ptr screen_priv, WindowPtr window)
{
    if (!screen_priv->info)
        return NULL;

    if (!screen_priv->info->get_crtc)
        return NULL;

    return (*screen_priv->info->get_crtc)(window);
}

static uint32_t
present_scmd_query_capabilities(present_screen_priv_ptr screen_priv)
{
    if (!screen_priv->info)
        return 0;

    return screen_priv->info->capabilities;
}

static int
present_get_ust_msc(ScreenPtr screen, RRCrtcPtr crtc, uint64_t *ust, uint64_t *msc)
{
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    present_screen_priv_ptr     crtc_screen_priv = screen_priv;
    if (crtc)
        crtc_screen_priv = present_screen_priv(crtc->pScreen);

    if (crtc == NULL)
        return present_fake_get_ust_msc(screen, ust, msc);
    else
        return (*crtc_screen_priv->info->get_ust_msc)(crtc, ust, msc);
}

static void
present_flush(WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    if (!screen_priv)
        return;

    if (!screen_priv->info)
        return;

    if (!screen_priv->info->flush)
        return;

    (*screen_priv->info->flush) (window);
}

static int
present_queue_vblank(ScreenPtr screen,
                     WindowPtr window,
                     RRCrtcPtr crtc,
                     uint64_t event_id,
                     uint64_t msc)
{
    Bool                        ret;

    if (crtc == NULL)
        ret = present_fake_queue_vblank(screen, event_id, msc);
    else
    {
        present_screen_priv_ptr     screen_priv = present_screen_priv(crtc->pScreen);
        ret = (*screen_priv->info->queue_vblank) (crtc, event_id, msc);
    }
    return ret;
}

/*
 * When the wait fence or previous flip is completed, it's time
 * to re-try the request
 */
static void
present_re_execute(present_vblank_ptr vblank)
{
    uint64_t            ust = 0, crtc_msc = 0;

    if (vblank->crtc)
        (void) present_get_ust_msc(vblank->screen, vblank->crtc, &ust, &crtc_msc);

    present_execute(vblank, ust, crtc_msc);
}

static void
present_flip_try_ready(ScreenPtr screen)
{
    present_vblank_ptr  vblank;

    xorg_list_for_each_entry(vblank, &present_flip_queue, event_queue) {
        if (vblank->queued) {
            present_re_execute(vblank);
            return;
        }
    }
}

static void
present_flip_idle(ScreenPtr screen)
{
    present_screen_priv_ptr screen_priv = present_screen_priv(screen);

    if (screen_priv->flip_pixmap) {
        present_pixmap_idle(screen_priv->flip_pixmap, screen_priv->flip_window,
                            screen_priv->flip_serial, screen_priv->flip_idle_fence);
        if (screen_priv->flip_idle_fence)
            present_fence_destroy(screen_priv->flip_idle_fence);
        dixDestroyPixmap(screen_priv->flip_pixmap, screen_priv->flip_pixmap->drawable.id);
        screen_priv->flip_crtc = NULL;
        screen_priv->flip_window = NULL;
        screen_priv->flip_serial = 0;
        screen_priv->flip_pixmap = NULL;
        screen_priv->flip_idle_fence = NULL;
    }
}

void
present_restore_screen_pixmap(ScreenPtr screen)
{
    present_screen_priv_ptr screen_priv = present_screen_priv(screen);
    PixmapPtr screen_pixmap = (*screen->GetScreenPixmap)(screen);
    PixmapPtr flip_pixmap;
    WindowPtr flip_window;

    if (screen_priv->flip_pending) {
        flip_window = screen_priv->flip_pending->window;
        flip_pixmap = screen_priv->flip_pending->pixmap;
    } else {
        flip_window = screen_priv->flip_window;
        flip_pixmap = screen_priv->flip_pixmap;
    }

    assert (flip_pixmap);

    /* Update the screen pixmap with the current flip pixmap contents
     * Only do this the first time for a particular unflip operation, or
     * we'll probably scribble over other windows
     */
    if (screen->root && screen->GetWindowPixmap(screen->root) == flip_pixmap)
        present_copy_region(&screen_pixmap->drawable, flip_pixmap, NULL, 0, 0);

    /* Switch back to using the screen pixmap now to avoid
     * 2D applications drawing to the wrong pixmap.
     */
    if (flip_window)
        present_set_tree_pixmap(flip_window, flip_pixmap, screen_pixmap);
    if (screen->root)
        present_set_tree_pixmap(screen->root, NULL, screen_pixmap);
}

void
present_set_abort_flip(ScreenPtr screen)
{
    present_screen_priv_ptr screen_priv = present_screen_priv(screen);

    if (!screen_priv->flip_pending->abort_flip) {
        present_restore_screen_pixmap(screen);
        screen_priv->flip_pending->abort_flip = TRUE;
    }
}

static void
present_unflip(ScreenPtr screen)
{
    present_screen_priv_ptr screen_priv = present_screen_priv(screen);

    assert (!screen_priv->unflip_event_id);
    assert (!screen_priv->flip_pending);

    present_restore_screen_pixmap(screen);

    screen_priv->unflip_event_id = ++present_scmd_event_id;
    DebugPresent(("u %" PRIu64 "\n", screen_priv->unflip_event_id));
    (*screen_priv->info->unflip) (screen, screen_priv->unflip_event_id);
}

static void
present_flip_notify(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc)
{
    ScreenPtr                   screen = vblank->screen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    DebugPresent(("\tn %" PRIu64 " %p %" PRIu64 " %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                  vblank->event_id, vblank, vblank->exec_msc, vblank->target_msc,
                  vblank->pixmap ? vblank->pixmap->drawable.id : 0,
                  vblank->window ? vblank->window->drawable.id : 0));

    assert (vblank == screen_priv->flip_pending);

    present_flip_idle(screen);

    xorg_list_del(&vblank->event_queue);

    /* Transfer reference for pixmap and fence from vblank to screen_priv */
    screen_priv->flip_crtc = vblank->crtc;
    screen_priv->flip_window = vblank->window;
    screen_priv->flip_serial = vblank->serial;
    screen_priv->flip_pixmap = vblank->pixmap;
    screen_priv->flip_sync = vblank->sync_flip;
    screen_priv->flip_idle_fence = vblank->idle_fence;

    vblank->pixmap = NULL;
    vblank->idle_fence = NULL;

    screen_priv->flip_pending = NULL;

    if (vblank->abort_flip)
        present_unflip(screen);

    present_vblank_notify(vblank, PresentCompleteKindPixmap, PresentCompleteModeFlip, ust, crtc_msc);
    present_vblank_destroy(vblank);

    present_flip_try_ready(screen);
}

void
present_event_notify(uint64_t event_id, uint64_t ust, uint64_t msc)
{
    present_vblank_ptr  vblank;
    int                 s;

    if (!event_id)
        return;
    DebugPresent(("\te %" PRIu64 " ust %" PRIu64 " msc %" PRIu64 "\n", event_id, ust, msc));
    xorg_list_for_each_entry(vblank, &present_exec_queue, event_queue) {
        int64_t match = event_id - vblank->event_id;
        if (match == 0) {
            present_execute(vblank, ust, msc);
            return;
        }
    }
    xorg_list_for_each_entry(vblank, &present_flip_queue, event_queue) {
        if (vblank->event_id == event_id) {
            if (vblank->queued)
                present_execute(vblank, ust, msc);
            else
                present_flip_notify(vblank, ust, msc);
            return;
        }
    }

    for (s = 0; s < screenInfo.numScreens; s++) {
        ScreenPtr               screen = screenInfo.screens[s];
        present_screen_priv_ptr screen_priv = present_screen_priv(screen);

        if (event_id == screen_priv->unflip_event_id) {
            DebugPresent(("\tun %" PRIu64 "\n", event_id));
            screen_priv->unflip_event_id = 0;
            present_flip_idle(screen);
            present_flip_try_ready(screen);
            return;
        }
    }
}

/*
 * 'window' is being reconfigured. Check to see if it is involved
 * in flipping and clean up as necessary
 */
static void
present_check_flip_window (WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    present_window_priv_ptr     window_priv = present_window_priv(window);
    present_vblank_ptr          flip_pending = screen_priv->flip_pending;
    present_vblank_ptr          vblank;
    PresentFlipReason           reason;

    /* If this window hasn't ever been used with Present, it can't be
     * flipping
     */
    if (!window_priv)
        return;

    if (screen_priv->unflip_event_id)
        return;

    if (flip_pending) {
        /*
         * Check pending flip
         */
        if (flip_pending->window == window) {
            if (!present_check_flip(flip_pending->crtc, window, flip_pending->pixmap,
                                    flip_pending->sync_flip, NULL, 0, 0, NULL))
                present_set_abort_flip(screen);
        }
    } else {
        /*
         * Check current flip
         */
        if (window == screen_priv->flip_window) {
            if (!present_check_flip(screen_priv->flip_crtc, window, screen_priv->flip_pixmap, screen_priv->flip_sync, NULL, 0, 0, NULL))
                present_unflip(screen);
        }
    }

    /* Now check any queued vblanks */
    xorg_list_for_each_entry(vblank, &window_priv->vblank, window_list) {
        if (vblank->queued && vblank->flip && !present_check_flip(vblank->crtc, window, vblank->pixmap, vblank->sync_flip, NULL, 0, 0, &reason)) {
            vblank->flip = FALSE;
            vblank->reason = reason;
            if (vblank->sync_flip)
                vblank->exec_msc = vblank->target_msc;
        }
    }
}

static Bool
present_scmd_can_window_flip(WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    PixmapPtr                   window_pixmap;
    WindowPtr                   root = screen->root;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    if (!screen_priv)
        return FALSE;

    if (!screen_priv->info)
        return FALSE;

    /* Check to see if the driver supports flips at all */
    if (!screen_priv->info->flip)
        return FALSE;

    /* Make sure the window hasn't been redirected with Composite */
    window_pixmap = screen->GetWindowPixmap(window);
    if (window_pixmap != screen->GetScreenPixmap(screen) &&
        window_pixmap != screen_priv->flip_pixmap &&
        window_pixmap != present_flip_pending_pixmap(screen))
        return FALSE;

    /* Check for full-screen window */
    if (!RegionEqual(&window->clipList, &root->winSize)) {
        return FALSE;
    }

    /* Does the window match the pixmap exactly? */
    if (window->drawable.x != 0 || window->drawable.y != 0) {
        return FALSE;
    }

    return TRUE;
}

/*
 * Clean up any pending or current flips for this window
 */
static void
present_scmd_clear_window_flip(WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    present_vblank_ptr          flip_pending = screen_priv->flip_pending;

    if (flip_pending && flip_pending->window == window) {
        present_set_abort_flip(screen);
        flip_pending->window = NULL;
    }
    if (screen_priv->flip_window == window) {
        present_restore_screen_pixmap(screen);
        screen_priv->flip_window = NULL;
    }
}

/*
 * Once the required MSC has been reached, execute the pending request.
 *
 * For requests to actually present something, either blt contents to
 * the screen or queue a frame buffer swap.
 *
 * For requests to just get the current MSC/UST combo, skip that part and
 * go straight to event delivery
 */

static void
present_execute(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc)
{
    WindowPtr                   window = vblank->window;
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    if (vblank && vblank->crtc) {
        screen_priv=present_screen_priv(vblank->crtc->pScreen);
    }

    if (present_execute_wait(vblank, crtc_msc))
        return;

    if (vblank->flip && vblank->pixmap && vblank->window) {
        if (screen_priv->flip_pending || screen_priv->unflip_event_id) {
            DebugPresent(("\tr %" PRIu64 " %p (pending %p unflip %" PRIu64 ")\n",
                          vblank->event_id, vblank,
                          screen_priv->flip_pending, screen_priv->unflip_event_id));
            xorg_list_del(&vblank->event_queue);
            xorg_list_append(&vblank->event_queue, &present_flip_queue);
            vblank->flip_ready = TRUE;
            return;
        }
    }

    xorg_list_del(&vblank->event_queue);
    xorg_list_del(&vblank->window_list);
    vblank->queued = FALSE;

    if (vblank->pixmap && vblank->window) {

        if (vblank->flip) {

            DebugPresent(("\tf %" PRIu64 " %p %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                          vblank->event_id, vblank, crtc_msc,
                          vblank->pixmap->drawable.id, vblank->window->drawable.id));

            /* Prepare to flip by placing it in the flip queue and
             * and sticking it into the flip_pending field
             */
            screen_priv->flip_pending = vblank;

            xorg_list_add(&vblank->event_queue, &present_flip_queue);
            /* Try to flip
             */
            if (present_flip(vblank->crtc, vblank->event_id, vblank->target_msc, vblank->pixmap, vblank->sync_flip)) {
                RegionPtr damage;

                /* Fix window pixmaps:
                 *  1) Restore previous flip window pixmap
                 *  2) Set current flip window pixmap to the new pixmap
                 */
                if (screen_priv->flip_window && screen_priv->flip_window != window)
                    present_set_tree_pixmap(screen_priv->flip_window,
                                            screen_priv->flip_pixmap,
                                            (*screen->GetScreenPixmap)(screen));
                present_set_tree_pixmap(vblank->window, NULL, vblank->pixmap);
                present_set_tree_pixmap(screen->root, NULL, vblank->pixmap);

                /* Report update region as damaged
                 */
                if (vblank->update) {
                    damage = vblank->update;
                    RegionIntersect(damage, damage, &window->clipList);
                } else
                    damage = &window->clipList;

                DamageDamageRegion(&vblank->window->drawable, damage);
                return;
            }

            xorg_list_del(&vblank->event_queue);
            /* Oops, flip failed. Clear the flip_pending field
              */
            screen_priv->flip_pending = NULL;
            vblank->flip = FALSE;
            vblank->exec_msc = vblank->target_msc;
        }
        DebugPresent(("\tc %p %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                      vblank, crtc_msc, vblank->pixmap->drawable.id, vblank->window->drawable.id));
        if (screen_priv->flip_pending) {

            /* Check pending flip
             */
            if (window == screen_priv->flip_pending->window)
                present_set_abort_flip(screen);
        } else if (!screen_priv->unflip_event_id) {

            /* Check current flip
             */
            if (window == screen_priv->flip_window)
                present_unflip(screen);
        }

        present_execute_copy(vblank, crtc_msc);

        if (vblank->queued) {
            xorg_list_add(&vblank->event_queue, &present_exec_queue);
            xorg_list_append(&vblank->window_list,
                             &present_get_window_priv(window, TRUE)->vblank);
            return;
        }
    }

    present_execute_post(vblank, ust, crtc_msc);
}

static void
present_scmd_update_window_crtc(WindowPtr window, RRCrtcPtr crtc, uint64_t new_msc)
{
    present_window_priv_ptr window_priv = present_get_window_priv(window, TRUE);
    uint64_t                old_ust, old_msc;

    /* Crtc unchanged, no offset. */
    if (crtc == window_priv->crtc)
        return;

    /* No crtc earlier to offset against, just set the crtc. */
    if (window_priv->crtc == PresentCrtcNeverSet) {
        window_priv->crtc = crtc;
        return;
    }

    /* Crtc may have been turned off or be destroyed, just use whatever previous MSC we'd seen from this CRTC. */
    if (!RRCrtcExists(window->drawable.pScreen, window_priv->crtc) ||
        present_get_ust_msc(window->drawable.pScreen, window_priv->crtc, &old_ust, &old_msc) != Success)
        old_msc = window_priv->msc;

    window_priv->msc_offset += new_msc - old_msc;
    window_priv->crtc = crtc;
}

static int
present_scmd_pixmap(WindowPtr window,
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
                    uint64_t target_window_msc,
                    uint64_t divisor,
                    uint64_t remainder,
                    present_notify_ptr notifies,
                    int num_notifies)
{
    uint64_t                    ust = 0;
    uint64_t                    target_msc;
    uint64_t                    crtc_msc = 0;
    int                         ret;
    present_vblank_ptr          vblank, tmp;
    ScreenPtr                   screen = window->drawable.pScreen;
    present_window_priv_ptr     window_priv = present_get_window_priv(window, TRUE);
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    if (!window_priv)
        return BadAlloc;

    if (!screen_priv || !screen_priv->info)
        target_crtc = NULL;
    else if (!target_crtc) {
        /* Update the CRTC if we have a pixmap or we don't have a CRTC
         */
        if (!pixmap)
            target_crtc = window_priv->crtc;

        if (!target_crtc || target_crtc == PresentCrtcNeverSet)
            target_crtc = present_get_crtc(window);
    }

    ret = present_get_ust_msc(screen, target_crtc, &ust, &crtc_msc);

    present_scmd_update_window_crtc(window, target_crtc, crtc_msc);

    if (ret == Success) {
        /* Stash the current MSC away in case we need it later
         */
        window_priv->msc = crtc_msc;
    }

    target_msc = present_get_target_msc(target_window_msc + window_priv->msc_offset,
                                        crtc_msc,
                                        divisor,
                                        remainder,
                                        options);

    /*
     * Look for a matching presentation already on the list and
     * don't bother doing the previous one if this one will overwrite it
     * in the same frame
     */

    if (!update && pixmap) {
        xorg_list_for_each_entry_safe(vblank, tmp, &window_priv->vblank, window_list) {

            if (!vblank->pixmap)
                continue;

            if (!vblank->queued)
                continue;

            if (vblank->crtc != target_crtc || vblank->target_msc != target_msc)
                continue;

            present_vblank_scrap(vblank);
            if (vblank->flip_ready)
                present_re_execute(vblank);
        }
    }

    vblank = present_vblank_create(window,
                                   pixmap,
                                   serial,
                                   valid,
                                   update,
                                   x_off,
                                   y_off,
                                   target_crtc,
                                   wait_fence,
                                   idle_fence,
                                   options,
                                   screen_priv->info ? screen_priv->info->capabilities : 0,
                                   notifies,
                                   num_notifies,
                                   target_msc,
                                   crtc_msc);

    if (!vblank)
        return BadAlloc;

    vblank->event_id = ++present_scmd_event_id;

    if (vblank->flip && vblank->sync_flip)
        vblank->exec_msc--;

    xorg_list_append(&vblank->event_queue, &present_exec_queue);
    vblank->queued = TRUE;
    if (msc_is_after(vblank->exec_msc, crtc_msc)) {
        ret = present_queue_vblank(screen, window, target_crtc, vblank->event_id, vblank->exec_msc);
        if (ret == Success)
            return Success;

        DebugPresent(("present_queue_vblank failed\n"));
    }

    present_execute(vblank, ust, crtc_msc);

    return Success;
}

static void
present_scmd_abort_vblank(ScreenPtr screen, WindowPtr window, RRCrtcPtr crtc, uint64_t event_id, uint64_t msc)
{
    present_vblank_ptr  vblank;

    if (crtc == NULL)
        present_fake_abort_vblank(screen, event_id, msc);
    else
    {
        present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

        (*screen_priv->info->abort_vblank) (crtc, event_id, msc);
    }

    xorg_list_for_each_entry(vblank, &present_exec_queue, event_queue) {
        int64_t match = event_id - vblank->event_id;
        if (match == 0) {
            xorg_list_del(&vblank->event_queue);
            vblank->queued = FALSE;
            return;
        }
    }
    xorg_list_for_each_entry(vblank, &present_flip_queue, event_queue) {
        if (vblank->event_id == event_id) {
            xorg_list_del(&vblank->event_queue);
            vblank->queued = FALSE;
            return;
        }
    }
}

static void
present_scmd_flip_destroy(ScreenPtr screen)
{
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    /* Reset window pixmaps back to the screen pixmap */
    if (screen_priv->flip_pending)
        present_set_abort_flip(screen);

    /* Drop reference to any pending flip or unflip pixmaps. */
    present_flip_idle(screen);
}

void
present_scmd_init_mode_hooks(present_screen_priv_ptr screen_priv)
{
    screen_priv->query_capabilities =   &present_scmd_query_capabilities;
    screen_priv->get_crtc           =   &present_scmd_get_crtc;

    screen_priv->check_flip         =   &present_check_flip;
    screen_priv->check_flip_window  =   &present_check_flip_window;
    screen_priv->can_window_flip    =   &present_scmd_can_window_flip;
    screen_priv->clear_window_flip  =   &present_scmd_clear_window_flip;

    screen_priv->present_pixmap     =   &present_scmd_pixmap;

    screen_priv->queue_vblank       =   &present_queue_vblank;
    screen_priv->flush              =   &present_flush;
    screen_priv->re_execute         =   &present_re_execute;

    screen_priv->abort_vblank       =   &present_scmd_abort_vblank;
    screen_priv->flip_destroy       =   &present_scmd_flip_destroy;
}

Bool
present_init(void)
{
    xorg_list_init(&present_exec_queue);
    xorg_list_init(&present_flip_queue);
    present_fake_queue_init();
    return TRUE;
}
