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
#include <gcstruct.h>

uint32_t
present_query_capabilities(RRCrtcPtr crtc)
{
    present_screen_priv_ptr screen_priv;

    if (!crtc)
        return 0;

    screen_priv = present_screen_priv(crtc->pScreen);

    if (!screen_priv)
        return 0;

    return screen_priv->query_capabilities(screen_priv);
}

RRCrtcPtr
present_get_crtc(WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    RRCrtcPtr                   crtc = NULL;

    if (!screen_priv)
        return NULL;

    crtc = screen_priv->get_crtc(screen_priv, window);
    if (crtc && !present_screen_priv(crtc->pScreen)) {
        crtc = RRFirstEnabledCrtc(screen);
    }
    if (crtc && !present_screen_priv(crtc->pScreen)) {
        crtc = NULL;
    }
    return crtc;
}

/*
 * Copies the update region from a pixmap to the target drawable
 */
void
present_copy_region(DrawablePtr drawable,
                    PixmapPtr pixmap,
                    RegionPtr update,
                    int16_t x_off,
                    int16_t y_off)
{
    ScreenPtr   screen = drawable->pScreen;
    GCPtr       gc;

    gc = GetScratchGC(drawable->depth, screen);
    if (update) {
        ChangeGCVal     changes[2];

        changes[0].val = x_off;
        changes[1].val = y_off;
        ChangeGC(serverClient, gc,
                 GCClipXOrigin|GCClipYOrigin,
                 changes);
        (*gc->funcs->ChangeClip)(gc, CT_REGION, update, 0);
    }
    ValidateGC(drawable, gc);
    (*gc->ops->CopyArea)(&pixmap->drawable,
                         drawable,
                         gc,
                         0, 0,
                         pixmap->drawable.width, pixmap->drawable.height,
                         x_off, y_off);
    if (update)
        (*gc->funcs->ChangeClip)(gc, CT_NONE, NULL, 0);
    FreeScratchGC(gc);
}

void
present_pixmap_idle(PixmapPtr pixmap, WindowPtr window, CARD32 serial, struct present_fence *present_fence)
{
    if (present_fence)
        present_fence_set_triggered(present_fence);
    if (window) {
        DebugPresent(("\ti %08" PRIx32 "\n", pixmap ? pixmap->drawable.id : 0));
        present_send_idle_notify(window, serial, pixmap, present_fence);
    }
}

struct pixmap_visit {
    PixmapPtr   old;
    PixmapPtr   new;
};

static int
present_set_tree_pixmap_visit(WindowPtr window, void *data)
{
    struct pixmap_visit *visit = data;
    ScreenPtr           screen = window->drawable.pScreen;

    if ((*screen->GetWindowPixmap)(window) != visit->old)
        return WT_DONTWALKCHILDREN;
    (*screen->SetWindowPixmap)(window, visit->new);
    return WT_WALKCHILDREN;
}

void
present_set_tree_pixmap(WindowPtr window,
                        PixmapPtr expected,
                        PixmapPtr pixmap)
{
    struct pixmap_visit visit;
    ScreenPtr           screen = window->drawable.pScreen;

    visit.old = (*screen->GetWindowPixmap)(window);
    if (expected && visit.old != expected)
        return;

    visit.new = pixmap;
    if (visit.old == visit.new)
        return;
    TraverseTree(window, present_set_tree_pixmap_visit, &visit);
}

Bool
present_can_window_flip(WindowPtr window)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    return screen_priv->can_window_flip(window);
}

uint64_t
present_get_target_msc(uint64_t target_msc_arg,
                       uint64_t crtc_msc,
                       uint64_t divisor,
                       uint64_t remainder,
                       uint32_t options)
{
    const Bool  synced_flip = !(options & PresentOptionAsync);
    uint64_t    target_msc;

    /* If the specified target-msc lies in the future, then this
     * defines the target-msc according to Present protocol.
     */
    if (msc_is_after(target_msc_arg, crtc_msc))
        return target_msc_arg;

    /* If no divisor is specified, the modulo is undefined
     * and we do present instead asap.
     */
    if (divisor == 0) {
        target_msc = crtc_msc;

        /* When no async presentation is forced, by default we sync the
         * presentation with vblank. But in this case we can't target
         * the current crtc-msc, which already has begun, but must aim
         * for the upcoming one.
         */
        if (synced_flip)
            target_msc++;

        return target_msc;
    }

    /* Calculate target-msc by the specified modulo parameters. According
     * to Present protocol this is after the next field with:
     *
     *      field-msc % divisor == remainder.
     *
     * The following formula calculates a target_msc solving above equation
     * and with |target_msc - crtc_msc| < divisor.
     *
     * Example with crtc_msc = 10, divisor = 4 and remainder = 3, 2, 1, 0:
     *      11 = 10 - 2 + 3 = 10 - (10 % 4) + 3,
     *      10 = 10 - 2 + 2 = 10 - (10 % 4) + 2,
     *       9 = 10 - 2 + 1 = 10 - (10 % 4) + 1,
     *       8 = 10 - 2 + 0 = 10 - (10 % 4) + 0.
     */
    target_msc = crtc_msc - (crtc_msc % divisor) + remainder;

    /* Here we already found the correct field-msc. */
    if (msc_is_after(target_msc, crtc_msc))
        return target_msc;
    /*
     * Here either:
     * a) target_msc == crtc_msc, i.e. crtc_msc actually solved
     * above equation with crtc_msc % divisor == remainder.
     *
     * => This means we want to present at target_msc + divisor for a synced
     *    flip or directly now for an async flip.
     *
     * b) target_msc < crtc_msc with target_msc + divisor > crtc_msc.
     *
     * => This means in any case we want to present at target_msc + divisor.
     */
    if (synced_flip || msc_is_after(crtc_msc, target_msc))
        target_msc += divisor;
    return target_msc;
}

int
present_pixmap(WindowPtr window,
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
               uint64_t window_msc,
               uint64_t divisor,
               uint64_t remainder,
               present_notify_ptr notifies,
               int num_notifies)
{
    ScreenPtr                   screen = window->drawable.pScreen;
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);

    return screen_priv->present_pixmap(window,
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
                                       window_msc,
                                       divisor,
                                       remainder,
                                       notifies,
                                       num_notifies);
}

int
present_notify_msc(WindowPtr window,
                   CARD32 serial,
                   uint64_t target_msc,
                   uint64_t divisor,
                   uint64_t remainder)
{
    return present_pixmap(window,
                          NULL,
                          serial,
                          NULL, NULL,
                          0, 0,
                          NULL,
                          NULL, NULL,
                          divisor == 0 ? PresentOptionAsync : 0,
                          target_msc, divisor, remainder, NULL, 0);
}
