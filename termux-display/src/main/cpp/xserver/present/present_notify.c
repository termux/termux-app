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

/*
 * Mark all pending notifies for 'window' as invalid when
 * the window is destroyed
 */

void
present_clear_window_notifies(WindowPtr window)
{
    present_notify_ptr          notify;
    present_window_priv_ptr     window_priv = present_window_priv(window);

    if (!window_priv)
        return;

    xorg_list_for_each_entry(notify, &window_priv->notifies, window_list) {
        notify->window = NULL;
    }
}

/*
 * 'notify' is being freed; remove it from the window's notify list
 */

void
present_free_window_notify(present_notify_ptr notify)
{
    xorg_list_del(&notify->window_list);
}

/*
 * 'notify' is new; add it to the specified window
 */

int
present_add_window_notify(present_notify_ptr notify)
{
    WindowPtr                   window = notify->window;
    present_window_priv_ptr     window_priv = present_get_window_priv(window, TRUE);

    if (!window_priv)
        return BadAlloc;

    xorg_list_add(&notify->window_list, &window_priv->notifies);
    return Success;
}

int
present_create_notifies(ClientPtr client, int num_notifies, xPresentNotify *x_notifies, present_notify_ptr *p_notifies)
{
    present_notify_ptr  notifies;
    int                 i;
    int                 added = 0;
    int                 status;

    notifies = calloc (num_notifies, sizeof (present_notify_rec));
    if (!notifies)
        return BadAlloc;

    for (i = 0; i < num_notifies; i++) {
        status = dixLookupWindow(&notifies[i].window, x_notifies[i].window, client, DixGetAttrAccess);
        if (status != Success)
            goto bail;

        notifies[i].serial = x_notifies[i].serial;
        status = present_add_window_notify(&notifies[i]);
        if (status != Success)
            goto bail;

        added = i;
    }
    return Success;

bail:
    present_destroy_notifies(notifies, added);
    return status;
}

void
present_destroy_notifies(present_notify_ptr notifies, int num_notifies)
{
    int i;
    for (i = 0; i < num_notifies; i++)
        present_free_window_notify(&notifies[i]);

    free(notifies);
}
