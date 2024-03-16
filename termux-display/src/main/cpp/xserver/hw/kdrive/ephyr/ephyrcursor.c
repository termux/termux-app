/*
 * Copyright © 2014 Red Hat, Inc.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *      Adam Jackson <ajax@redhat.com>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "ephyr.h"
#include "ephyrlog.h"
#include "hostx.h"
#include "cursorstr.h"
#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

static DevPrivateKeyRec ephyrCursorPrivateKey;

typedef struct _ephyrCursor {
    xcb_cursor_t cursor;
} ephyrCursorRec, *ephyrCursorPtr;

static ephyrCursorPtr
ephyrGetCursor(CursorPtr cursor)
{
    return dixGetPrivateAddr(&cursor->devPrivates, &ephyrCursorPrivateKey);
}

static void
ephyrRealizeCoreCursor(EphyrScrPriv *scr, CursorPtr cursor)
{
    ephyrCursorPtr hw = ephyrGetCursor(cursor);
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_pixmap_t source, mask;
    xcb_image_t *image;
    xcb_gcontext_t gc;
    int w = cursor->bits->width, h = cursor->bits->height;
    uint32_t gcmask = XCB_GC_FUNCTION |
                      XCB_GC_PLANE_MASK |
                      XCB_GC_FOREGROUND |
                      XCB_GC_BACKGROUND |
                      XCB_GC_CLIP_MASK;
    uint32_t val[] = {
        XCB_GX_COPY,    /* function */
        ~0,             /* planemask */
        1L,             /* foreground */
        0L,             /* background */
        None,           /* clipmask */
    };

    source = xcb_generate_id(conn);
    mask = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 1, source, scr->win, w, h);
    xcb_create_pixmap(conn, 1, mask, scr->win, w, h);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, source, gcmask, val);

    image = xcb_image_create_native(conn, w, h, XCB_IMAGE_FORMAT_XY_BITMAP,
                                    1, NULL, ~0, NULL);
    image->data = cursor->bits->source;
    xcb_image_put(conn, source, gc, image, 0, 0, 0);
    xcb_image_destroy(image);

    image = xcb_image_create_native(conn, w, h, XCB_IMAGE_FORMAT_XY_BITMAP,
                                    1, NULL, ~0, NULL);
    image->data = cursor->bits->mask;
    xcb_image_put(conn, mask, gc, image, 0, 0, 0);
    xcb_image_destroy(image);

    xcb_free_gc(conn, gc);

    hw->cursor = xcb_generate_id(conn);
    xcb_create_cursor(conn, hw->cursor, source, mask,
                      cursor->foreRed, cursor->foreGreen, cursor->foreBlue,
                      cursor->backRed, cursor->backGreen, cursor->backBlue,
                      cursor->bits->xhot, cursor->bits->yhot);

    xcb_free_pixmap(conn, source);
    xcb_free_pixmap(conn, mask);
}

static xcb_render_pictformat_t
get_argb_format(void)
{
    static xcb_render_pictformat_t format;
    if (format == None) {
        xcb_connection_t *conn = hostx_get_xcbconn();
        xcb_render_query_pict_formats_cookie_t cookie;
        xcb_render_query_pict_formats_reply_t *formats;

        cookie = xcb_render_query_pict_formats(conn);
        formats =
            xcb_render_query_pict_formats_reply(conn, cookie, NULL);

        format =
            xcb_render_util_find_standard_format(formats,
                                                 XCB_PICT_STANDARD_ARGB_32)->id;

        free(formats);
    }

    return format;
}

static void
ephyrRealizeARGBCursor(EphyrScrPriv *scr, CursorPtr cursor)
{
    ephyrCursorPtr hw = ephyrGetCursor(cursor);
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    xcb_pixmap_t source;
    xcb_render_picture_t picture;
    xcb_image_t *image;
    int w = cursor->bits->width, h = cursor->bits->height;

    /* dix' storage is PICT_a8r8g8b8 */
    source = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 32, source, scr->win, w, h);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, source, 0, NULL);
    image = xcb_image_create_native(conn, w, h, XCB_IMAGE_FORMAT_Z_PIXMAP,
                                    32, NULL, ~0, NULL);
    image->data = (void *)cursor->bits->argb;
    xcb_image_put(conn, source, gc, image, 0, 0, 0);
    xcb_free_gc(conn, gc);
    xcb_image_destroy(image);

    picture = xcb_generate_id(conn);
    xcb_render_create_picture(conn, picture, source, get_argb_format(),
                              0, NULL);
    xcb_free_pixmap(conn, source);

    hw->cursor = xcb_generate_id(conn);
    xcb_render_create_cursor(conn, hw->cursor, picture,
                             cursor->bits->xhot, cursor->bits->yhot);

    xcb_render_free_picture(conn, picture);
}

static Bool
can_argb_cursor(void)
{
    static const xcb_render_query_version_reply_t *v;

    if (!v)
        v = xcb_render_util_query_version(hostx_get_xcbconn());

    return v->major_version == 0 && v->minor_version >= 5;
}

static Bool
ephyrRealizeCursor(DeviceIntPtr dev, ScreenPtr screen, CursorPtr cursor)
{
    KdScreenPriv(screen);
    KdScreenInfo *kscr = pScreenPriv->screen;
    EphyrScrPriv *scr = kscr->driver;

    if (cursor->bits->argb && can_argb_cursor())
        ephyrRealizeARGBCursor(scr, cursor);
    else
    {
        ephyrRealizeCoreCursor(scr, cursor);
    }
    return TRUE;
}

static Bool
ephyrUnrealizeCursor(DeviceIntPtr dev, ScreenPtr screen, CursorPtr cursor)
{
    ephyrCursorPtr hw = ephyrGetCursor(cursor);

    if (hw->cursor) {
        xcb_free_cursor(hostx_get_xcbconn(), hw->cursor);
        hw->cursor = None;
    }

    return TRUE;
}

static void
ephyrSetCursor(DeviceIntPtr dev, ScreenPtr screen, CursorPtr cursor, int x,
               int y)
{
    KdScreenPriv(screen);
    KdScreenInfo *kscr = pScreenPriv->screen;
    EphyrScrPriv *scr = kscr->driver;
    uint32_t attr = None;

    if (cursor)
        attr = ephyrGetCursor(cursor)->cursor;
    else
        attr = hostx_get_empty_cursor();

    xcb_change_window_attributes(hostx_get_xcbconn(), scr->win,
                                 XCB_CW_CURSOR, &attr);
    xcb_flush(hostx_get_xcbconn());
}

static void
ephyrMoveCursor(DeviceIntPtr dev, ScreenPtr screen, int x, int y)
{
}

static Bool
ephyrDeviceCursorInitialize(DeviceIntPtr dev, ScreenPtr screen)
{
    return TRUE;
}

static void
ephyrDeviceCursorCleanup(DeviceIntPtr dev, ScreenPtr screen)
{
}

miPointerSpriteFuncRec EphyrPointerSpriteFuncs = {
    ephyrRealizeCursor,
    ephyrUnrealizeCursor,
    ephyrSetCursor,
    ephyrMoveCursor,
    ephyrDeviceCursorInitialize,
    ephyrDeviceCursorCleanup
};

Bool
ephyrCursorInit(ScreenPtr screen)
{
    if (!dixRegisterPrivateKey(&ephyrCursorPrivateKey, PRIVATE_CURSOR_BITS,
                               sizeof(ephyrCursorRec)))
        return FALSE;

    miPointerInitialize(screen,
                        &EphyrPointerSpriteFuncs,
                        &ephyrPointerScreenFuncs, FALSE);

    return TRUE;
}
