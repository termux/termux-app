/*
 * Copyright © 2014 Keith Packard
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

#include "glamor_priv.h"
#include "glamor_transfer.h"
#include "glamor_transform.h"

/*
 * PutImage. Only does ZPixmap right now as other formats are quite a bit harder
 */

static Bool
glamor_put_image_gl(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
                    int w, int h, int leftPad, int format, char *bits)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    uint32_t    byte_stride = PixmapBytePad(w, drawable->depth);
    RegionRec   region;
    BoxRec      box;
    int         off_x, off_y;

    pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        return FALSE;

    if (gc->alu != GXcopy)
        goto bail;

    if (!glamor_pm_is_solid(gc->depth, gc->planemask))
        goto bail;

    if (format == XYPixmap && drawable->depth == 1 && leftPad == 0)
        format = ZPixmap;

    if (format != ZPixmap)
        goto bail;

    x += drawable->x;
    y += drawable->y;
    box.x1 = x;
    box.y1 = y;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;
    RegionInit(&region, &box, 1);
    RegionIntersect(&region, &region, gc->pCompositeClip);

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);
    if (off_x || off_y) {
        x += off_x;
        y += off_y;
        RegionTranslate(&region, off_x, off_y);
    }

    glamor_make_current(glamor_priv);

    glamor_upload_region(pixmap, &region, x, y, (uint8_t *) bits, byte_stride);

    RegionUninit(&region);
    return TRUE;
bail:
    return FALSE;
}

static void
glamor_put_image_bail(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
                      int w, int h, int leftPad, int format, char *bits)
{
    if (glamor_prepare_access_box(drawable, GLAMOR_ACCESS_RW, x, y, w, h))
        fbPutImage(drawable, gc, depth, x, y, w, h, leftPad, format, bits);
    glamor_finish_access(drawable);
}

void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
                 int w, int h, int leftPad, int format, char *bits)
{
    if (glamor_put_image_gl(drawable, gc, depth, x, y, w, h, leftPad, format, bits))
        return;
    glamor_put_image_bail(drawable, gc, depth, x, y, w, h, leftPad, format, bits);
}

static Bool
glamor_get_image_gl(DrawablePtr drawable, int x, int y, int w, int h,
                    unsigned int format, unsigned long plane_mask, char *d)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    uint32_t    byte_stride = PixmapBytePad(w, drawable->depth);
    BoxRec      box;
    int         off_x, off_y;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    if (format != ZPixmap)
        goto bail;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);
    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;
    glamor_download_boxes(pixmap, &box, 1,
                          drawable->x + off_x, drawable->y + off_y,
                          -x, -y,
                          (uint8_t *) d, byte_stride);

    if (!glamor_pm_is_solid(drawable->depth, plane_mask)) {
        FbStip pm = fbReplicatePixel(plane_mask, drawable->bitsPerPixel);
        FbStip *dst = (void *)d;
        uint32_t dstStride = byte_stride / sizeof(FbStip);

        for (int i = 0; i < dstStride * h; i++)
            dst[i] &= pm;
    }

    return TRUE;
bail:
    return FALSE;
}

static void
glamor_get_image_bail(DrawablePtr drawable, int x, int y, int w, int h,
                      unsigned int format, unsigned long plane_mask, char *d)
{
    if (glamor_prepare_access_box(drawable, GLAMOR_ACCESS_RO, x, y, w, h))
        fbGetImage(drawable, x, y, w, h, format, plane_mask, d);
    glamor_finish_access(drawable);
}

void
glamor_get_image(DrawablePtr drawable, int x, int y, int w, int h,
                 unsigned int format, unsigned long plane_mask, char *d)
{
    if (glamor_get_image_gl(drawable, x, y, w, h, format, plane_mask, d))
        return;
    glamor_get_image_bail(drawable, x, y, w, h, format, plane_mask, d);
}
