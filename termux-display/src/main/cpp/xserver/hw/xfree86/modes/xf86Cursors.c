/*
 * Copyright © 2007 Keith Packard
 * Copyright © 2010-2011 Aaron Plattner
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

#include <X11/Xarch.h>
#include "xf86.h"
#include "xf86DDC.h"
#include "xf86Crtc.h"
#include "xf86Modes.h"
#include "xf86RandR12.h"
#include "xf86CursorPriv.h"
#include "X11/extensions/render.h"
#include "X11/extensions/dpmsconst.h"
#include "X11/Xatom.h"
#include "picturestr.h"
#include "cursorstr.h"
#include "inputstr.h"

/*
 * Returns the rotation being performed by the server.  If the driver indicates
 * that it's handling the screen transform, then this returns RR_Rotate_0.
 */
static Rotation
xf86_crtc_cursor_rotation(xf86CrtcPtr crtc)
{
    if (crtc->driverIsPerformingTransform & XF86DriverTransformCursorImage)
        return RR_Rotate_0;
    return crtc->rotation;
}

/*
 * Given a screen coordinate, rotate back to a cursor source coordinate
 */
static void
xf86_crtc_rotate_coord(Rotation rotation,
                       int width,
                       int height, int x_dst, int y_dst, int *x_src, int *y_src)
{
    int t;

    switch (rotation & 0xf) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        t = x_dst;
        x_dst = width - y_dst - 1;
        y_dst = t;
        break;
    case RR_Rotate_180:
        x_dst = width - x_dst - 1;
        y_dst = height - y_dst - 1;
        break;
    case RR_Rotate_270:
        t = x_dst;
        x_dst = y_dst;
        y_dst = height - t - 1;
        break;
    }
    if (rotation & RR_Reflect_X)
        x_dst = width - x_dst - 1;
    if (rotation & RR_Reflect_Y)
        y_dst = height - y_dst - 1;
    *x_src = x_dst;
    *y_src = y_dst;
}

/*
 * Given a cursor source  coordinate, rotate to a screen coordinate
 */
static void
xf86_crtc_rotate_coord_back(Rotation rotation,
                            int width,
                            int height,
                            int x_dst, int y_dst, int *x_src, int *y_src)
{
    int t;

    if (rotation & RR_Reflect_X)
        x_dst = width - x_dst - 1;
    if (rotation & RR_Reflect_Y)
        y_dst = height - y_dst - 1;

    switch (rotation & 0xf) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        t = x_dst;
        x_dst = y_dst;
        y_dst = width - t - 1;
        break;
    case RR_Rotate_180:
        x_dst = width - x_dst - 1;
        y_dst = height - y_dst - 1;
        break;
    case RR_Rotate_270:
        t = x_dst;
        x_dst = height - y_dst - 1;
        y_dst = t;
        break;
    }
    *x_src = x_dst;
    *y_src = y_dst;
}

struct cursor_bit {
    CARD8 *byte;
    char bitpos;
};

/*
 * Convert an x coordinate to a position within the cursor bitmap
 */
static struct cursor_bit
cursor_bitpos(CARD8 *image, xf86CursorInfoPtr cursor_info, int x, int y,
              Bool mask)
{
    const int flags = cursor_info->Flags;
    const Bool interleaved =
        ! !(flags & (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1 |
                     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8 |
                     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16 |
                     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32 |
                     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64));
    const int width = cursor_info->MaxWidth;
    const int height = cursor_info->MaxHeight;
    const int stride = interleaved ? width / 4 : width / 8;

    struct cursor_bit ret;

    image += y * stride;

    if (flags & HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK)
        mask = !mask;
    if (flags & HARDWARE_CURSOR_NIBBLE_SWAPPED)
        x = (x & ~3) | (3 - (x & 3));
    if (((flags & HARDWARE_CURSOR_BIT_ORDER_MSBFIRST) == 0) ==
        (X_BYTE_ORDER == X_BIG_ENDIAN))
        x = (x & ~7) | (7 - (x & 7));
    if (flags & HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1)
        x = (x << 1) + mask;
    else if (flags & HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8)
        x = ((x & ~7) << 1) | (mask << 3) | (x & 7);
    else if (flags & HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16)
        x = ((x & ~15) << 1) | (mask << 4) | (x & 15);
    else if (flags & HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32)
        x = ((x & ~31) << 1) | (mask << 5) | (x & 31);
    else if (flags & HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64)
        x = ((x & ~63) << 1) | (mask << 6) | (x & 63);
    else if (mask)
        image += stride * height;

    ret.byte = image + (x / 8);
    ret.bitpos = x & 7;

    return ret;
}

/*
 * Fetch one bit from a cursor bitmap
 */
static CARD8
get_bit(CARD8 *image, xf86CursorInfoPtr cursor_info, int x, int y, Bool mask)
{
    struct cursor_bit bit = cursor_bitpos(image, cursor_info, x, y, mask);

    return (*bit.byte >> bit.bitpos) & 1;
}

/*
 * Set one bit in a cursor bitmap
 */
static void
set_bit(CARD8 *image, xf86CursorInfoPtr cursor_info, int x, int y, Bool mask)
{
    struct cursor_bit bit = cursor_bitpos(image, cursor_info, x, y, mask);

    *bit.byte |= 1 << bit.bitpos;
}

/*
 * Wrappers to deal with API compatibility with drivers that don't expose
 * *_cursor_*_check
 */
static inline Bool
xf86_driver_has_show_cursor(xf86CrtcPtr crtc)
{
    return crtc->funcs->show_cursor_check || crtc->funcs->show_cursor;
}

static inline Bool
xf86_driver_has_load_cursor_image(xf86CrtcPtr crtc)
{
    return crtc->funcs->load_cursor_image_check || crtc->funcs->load_cursor_image;
}

static inline Bool
xf86_driver_has_load_cursor_argb(xf86CrtcPtr crtc)
{
    return crtc->funcs->load_cursor_argb_check || crtc->funcs->load_cursor_argb;
}

static inline Bool
xf86_driver_show_cursor(xf86CrtcPtr crtc)
{
    if (crtc->funcs->show_cursor_check)
        return crtc->funcs->show_cursor_check(crtc);
    crtc->funcs->show_cursor(crtc);
    return TRUE;
}

static inline Bool
xf86_driver_load_cursor_image(xf86CrtcPtr crtc, CARD8 *cursor_image)
{
    if (crtc->funcs->load_cursor_image_check)
        return crtc->funcs->load_cursor_image_check(crtc, cursor_image);
    crtc->funcs->load_cursor_image(crtc, cursor_image);
    return TRUE;
}

static inline Bool
xf86_driver_load_cursor_argb(xf86CrtcPtr crtc, CARD32 *cursor_argb)
{
    if (crtc->funcs->load_cursor_argb_check)
        return crtc->funcs->load_cursor_argb_check(crtc, cursor_argb);
    crtc->funcs->load_cursor_argb(crtc, cursor_argb);
    return TRUE;
}

/*
 * Load a two color cursor into a driver that supports only ARGB cursors
 */
static Bool
xf86_crtc_convert_cursor_to_argb(xf86CrtcPtr crtc, unsigned char *src)
{
    ScrnInfoPtr scrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    CARD32 *cursor_image = (CARD32 *) xf86_config->cursor_image;
    int x, y;
    int xin, yin;
    int flags = cursor_info->Flags;
    CARD32 bits;
    const Rotation rotation = xf86_crtc_cursor_rotation(crtc);

    crtc->cursor_argb = FALSE;

    for (y = 0; y < cursor_info->MaxHeight; y++)
        for (x = 0; x < cursor_info->MaxWidth; x++) {
            xf86_crtc_rotate_coord(rotation,
                                   cursor_info->MaxWidth,
                                   cursor_info->MaxHeight, x, y, &xin, &yin);
            if (get_bit(src, cursor_info, xin, yin, TRUE) ==
                ((flags & HARDWARE_CURSOR_INVERT_MASK) == 0)) {
                if (get_bit(src, cursor_info, xin, yin, FALSE))
                    bits = xf86_config->cursor_fg;
                else
                    bits = xf86_config->cursor_bg;
            }
            else
                bits = 0;
            cursor_image[y * cursor_info->MaxWidth + x] = bits;
        }
    return xf86_driver_load_cursor_argb(crtc, cursor_image);
}

/*
 * Set the colors for a two-color cursor (ignore for ARGB cursors)
 */
static void
xf86_set_cursor_colors(ScrnInfoPtr scrn, int bg, int fg)
{
    ScreenPtr screen = scrn->pScreen;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    CursorPtr cursor = xf86CurrentCursor(screen);
    int c;
    CARD8 *bits = cursor ?
        dixLookupScreenPrivate(&cursor->devPrivates, CursorScreenKey, screen)
        : NULL;

    /* Save ARGB versions of these colors */
    xf86_config->cursor_fg = (CARD32) fg | 0xff000000;
    xf86_config->cursor_bg = (CARD32) bg | 0xff000000;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled && !crtc->cursor_argb) {
            if (xf86_driver_has_load_cursor_image(crtc))
                crtc->funcs->set_cursor_colors(crtc, bg, fg);
            else if (bits)
                xf86_crtc_convert_cursor_to_argb(crtc, bits);
        }
    }
}

void
xf86_crtc_hide_cursor(xf86CrtcPtr crtc)
{
    if (crtc->cursor_shown) {
        crtc->funcs->hide_cursor(crtc);
        crtc->cursor_shown = FALSE;
    }
}

void
xf86_hide_cursors(ScrnInfoPtr scrn)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    xf86_config->cursor_on = FALSE;
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled)
            xf86_crtc_hide_cursor(crtc);
    }
}

Bool
xf86_crtc_show_cursor(xf86CrtcPtr crtc)
{
    if (!crtc->cursor_in_range) {
        crtc->funcs->hide_cursor(crtc);
        return TRUE;
    }

    if (!crtc->cursor_shown)
        crtc->cursor_shown = xf86_driver_show_cursor(crtc);

    return crtc->cursor_shown;
}

Bool
xf86_show_cursors(ScrnInfoPtr scrn)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    xf86_config->cursor_on = TRUE;
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled && !xf86_crtc_show_cursor(crtc))
            return FALSE;
    }

    return TRUE;
}

static void
xf86_crtc_transform_cursor_position(xf86CrtcPtr crtc, int *x, int *y)
{
    ScrnInfoPtr scrn = crtc->scrn;
    ScreenPtr screen = scrn->pScreen;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&screen->devPrivates,
                                               xf86CursorScreenKey);
    int dx, dy, t;
    Bool swap_reflection = FALSE;

    *x = *x - crtc->x + ScreenPriv->HotX;
    *y = *y - crtc->y + ScreenPriv->HotY;

    switch (crtc->rotation & 0xf) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        t = *x;
        *x = *y;
        *y = crtc->mode.VDisplay - t - 1;
        swap_reflection = TRUE;
        break;
    case RR_Rotate_180:
        *x = crtc->mode.HDisplay - *x - 1;
        *y = crtc->mode.VDisplay - *y - 1;
        break;
    case RR_Rotate_270:
        t = *x;
        *x = crtc->mode.HDisplay - *y - 1;
        *y = t;
        swap_reflection = TRUE;
        break;
    }

    if (swap_reflection) {
        if (crtc->rotation & RR_Reflect_Y)
            *x = crtc->mode.HDisplay - *x - 1;
        if (crtc->rotation & RR_Reflect_X)
            *y = crtc->mode.VDisplay - *y - 1;
    } else {
        if (crtc->rotation & RR_Reflect_X)
            *x = crtc->mode.HDisplay - *x - 1;
        if (crtc->rotation & RR_Reflect_Y)
            *y = crtc->mode.VDisplay - *y - 1;
    }

    /*
     * Transform position of cursor upper left corner
     */
    xf86_crtc_rotate_coord_back(crtc->rotation, cursor_info->MaxWidth,
                                cursor_info->MaxHeight, ScreenPriv->HotX,
                                ScreenPriv->HotY, &dx, &dy);
    *x -= dx;
    *y -= dy;
}

static void
xf86_crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
    ScrnInfoPtr scrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    DisplayModePtr mode = &crtc->mode;
    int crtc_x = x, crtc_y = y;

    /*
     * Transform position of cursor on screen
     */
    if (crtc->rotation != RR_Rotate_0)
        xf86_crtc_transform_cursor_position(crtc, &crtc_x, &crtc_y);
    else {
        crtc_x -= crtc->x;
        crtc_y -= crtc->y;
    }

    /*
     * Disable the cursor when it is outside the viewport
     */
    if (crtc_x >= mode->HDisplay || crtc_y >= mode->VDisplay ||
        crtc_x <= -cursor_info->MaxWidth || crtc_y <= -cursor_info->MaxHeight) {
        crtc->cursor_in_range = FALSE;
        xf86_crtc_hide_cursor(crtc);
    } else {
        crtc->cursor_in_range = TRUE;
        if (crtc->driverIsPerformingTransform & XF86DriverTransformCursorPosition)
            crtc->funcs->set_cursor_position(crtc, x, y);
        else
            crtc->funcs->set_cursor_position(crtc, crtc_x, crtc_y);
        xf86_crtc_show_cursor(crtc);
    }
}

static void
xf86_set_cursor_position(ScrnInfoPtr scrn, int x, int y)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    /* undo what xf86HWCurs did to the coordinates */
    x += scrn->frameX0;
    y += scrn->frameY0;
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled)
            xf86_crtc_set_cursor_position(crtc, x, y);
    }
}

/*
 * Load a two-color cursor into a crtc, performing rotation as needed
 */
static Bool
xf86_crtc_load_cursor_image(xf86CrtcPtr crtc, CARD8 *src)
{
    ScrnInfoPtr scrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    CARD8 *cursor_image;
    const Rotation rotation = xf86_crtc_cursor_rotation(crtc);

    crtc->cursor_argb = FALSE;

    if (rotation == RR_Rotate_0)
        cursor_image = src;
    else {
        int x, y;
        int xin, yin;
        int stride = cursor_info->MaxWidth >> 2;

        cursor_image = xf86_config->cursor_image;
        memset(cursor_image, 0, cursor_info->MaxHeight * stride);

        for (y = 0; y < cursor_info->MaxHeight; y++)
            for (x = 0; x < cursor_info->MaxWidth; x++) {
                xf86_crtc_rotate_coord(rotation,
                                       cursor_info->MaxWidth,
                                       cursor_info->MaxHeight,
                                       x, y, &xin, &yin);
                if (get_bit(src, cursor_info, xin, yin, FALSE))
                    set_bit(cursor_image, cursor_info, x, y, FALSE);
                if (get_bit(src, cursor_info, xin, yin, TRUE))
                    set_bit(cursor_image, cursor_info, x, y, TRUE);
            }
    }
    return xf86_driver_load_cursor_image(crtc, cursor_image);
}

/*
 * Load a cursor image into all active CRTCs
 */
static Bool
xf86_load_cursor_image(ScrnInfoPtr scrn, unsigned char *src)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    xf86_config->cursor = xf86CurrentCursor(scrn->pScreen);
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled) {
            if (xf86_driver_has_load_cursor_image(crtc)) {
                if (!xf86_crtc_load_cursor_image(crtc, src))
                    return FALSE;
            } else if (xf86_driver_has_load_cursor_argb(crtc)) {
                if (!xf86_crtc_convert_cursor_to_argb(crtc, src))
                    return FALSE;
            } else
                return FALSE;
        }
    }
    return TRUE;
}

static Bool
xf86_use_hw_cursor(ScreenPtr screen, CursorPtr cursor)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    int c;

    if (cursor->bits->width > cursor_info->MaxWidth ||
        cursor->bits->height > cursor_info->MaxHeight)
        return FALSE;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (!crtc->enabled)
            continue;

        if (crtc->transformPresent)
            return FALSE;
    }

    return TRUE;
}

static Bool
xf86_use_hw_cursor_argb(ScreenPtr screen, CursorPtr cursor)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;

    if (!xf86_use_hw_cursor(screen, cursor))
        return FALSE;

    /* Make sure ARGB support is available */
    if ((cursor_info->Flags & HARDWARE_CURSOR_ARGB) == 0)
        return FALSE;

    return TRUE;
}

static Bool
xf86_crtc_load_cursor_argb(xf86CrtcPtr crtc, CursorPtr cursor)
{
    ScrnInfoPtr scrn = crtc->scrn;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;
    CARD32 *cursor_image = (CARD32 *) xf86_config->cursor_image;
    CARD32 *cursor_source = (CARD32 *) cursor->bits->argb;
    int x, y;
    int xin, yin;
    CARD32 bits;
    int source_width = cursor->bits->width;
    int source_height = cursor->bits->height;
    int image_width = cursor_info->MaxWidth;
    int image_height = cursor_info->MaxHeight;
    const Rotation rotation = xf86_crtc_cursor_rotation(crtc);

    for (y = 0; y < image_height; y++)
        for (x = 0; x < image_width; x++) {
            xf86_crtc_rotate_coord(rotation, image_width, image_height, x, y,
                                   &xin, &yin);
            if (xin < source_width && yin < source_height)
                bits = cursor_source[yin * source_width + xin];
            else
                bits = 0;
            cursor_image[y * image_width + x] = bits;
        }

    return xf86_driver_load_cursor_argb(crtc, cursor_image);
}

static Bool
xf86_load_cursor_argb(ScrnInfoPtr scrn, CursorPtr cursor)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    xf86_config->cursor = cursor;
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (crtc->enabled)
            if (!xf86_crtc_load_cursor_argb(crtc, cursor))
                return FALSE;
    }
    return TRUE;
}

Bool
xf86_cursors_init(ScreenPtr screen, int max_width, int max_height, int flags)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    xf86CursorInfoPtr cursor_info;

    cursor_info = xf86CreateCursorInfoRec();
    if (!cursor_info)
        return FALSE;

    xf86_config->cursor_image = malloc(max_width * max_height * 4);

    if (!xf86_config->cursor_image) {
        xf86DestroyCursorInfoRec(cursor_info);
        return FALSE;
    }

    xf86_config->cursor_info = cursor_info;

    cursor_info->MaxWidth = max_width;
    cursor_info->MaxHeight = max_height;
    cursor_info->Flags = flags;

    cursor_info->SetCursorColors = xf86_set_cursor_colors;
    cursor_info->SetCursorPosition = xf86_set_cursor_position;
    cursor_info->LoadCursorImageCheck = xf86_load_cursor_image;
    cursor_info->HideCursor = xf86_hide_cursors;
    cursor_info->ShowCursorCheck = xf86_show_cursors;
    cursor_info->UseHWCursor = xf86_use_hw_cursor;
    if (flags & HARDWARE_CURSOR_ARGB) {
        cursor_info->UseHWCursorARGB = xf86_use_hw_cursor_argb;
        cursor_info->LoadCursorARGBCheck = xf86_load_cursor_argb;
    }

    xf86_hide_cursors(scrn);

    return xf86InitCursor(screen, cursor_info);
}

/**
 * Clean up CRTC-based cursor code
 */
void
xf86_cursors_fini(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    if (xf86_config->cursor_info) {
        xf86DestroyCursorInfoRec(xf86_config->cursor_info);
        xf86_config->cursor_info = NULL;
    }
    free(xf86_config->cursor_image);
    xf86_config->cursor_image = NULL;
    xf86_config->cursor = NULL;
}
