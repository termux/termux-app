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

/*
 * Write a region of bits into a pixmap
 */
void
glamor_upload_boxes(PixmapPtr pixmap, BoxPtr in_boxes, int in_nbox,
                    int dx_src, int dy_src,
                    int dx_dst, int dy_dst,
                    uint8_t *bits, uint32_t byte_stride)
{
    ScreenPtr                   screen = pixmap->drawable.pScreen;
    glamor_screen_private       *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private       *priv = glamor_get_pixmap_private(pixmap);
    int                         box_index;
    int                         bytes_per_pixel = pixmap->drawable.bitsPerPixel >> 3;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);

    glamor_make_current(glamor_priv);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (glamor_priv->has_unpack_subimage)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, byte_stride / bytes_per_pixel);

    glamor_pixmap_loop(priv, box_index) {
        BoxPtr                  box = glamor_pixmap_box_at(priv, box_index);
        glamor_pixmap_fbo       *fbo = glamor_pixmap_fbo_at(priv, box_index);
        BoxPtr                  boxes = in_boxes;
        int                     nbox = in_nbox;

        glamor_bind_texture(glamor_priv, GL_TEXTURE0, fbo, TRUE);

        while (nbox--) {

            /* compute drawable coordinates */
            int x1 = MAX(boxes->x1 + dx_dst, box->x1);
            int x2 = MIN(boxes->x2 + dx_dst, box->x2);
            int y1 = MAX(boxes->y1 + dy_dst, box->y1);
            int y2 = MIN(boxes->y2 + dy_dst, box->y2);

            size_t ofs = (y1 - dy_dst + dy_src) * byte_stride;
            ofs += (x1 - dx_dst + dx_src) * bytes_per_pixel;

            boxes++;

            if (x2 <= x1 || y2 <= y1)
                continue;

            if (glamor_priv->has_unpack_subimage ||
                x2 - x1 == byte_stride / bytes_per_pixel) {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x1 - box->x1, y1 - box->y1,
                                x2 - x1, y2 - y1,
                                f->format, f->type,
                                bits + ofs);
            } else {
                for (; y1 < y2; y1++, ofs += byte_stride)
                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                                    x1 - box->x1, y1 - box->y1,
                                    x2 - x1, 1,
                                    f->format, f->type,
                                    bits + ofs);
            }
        }
    }

    if (glamor_priv->has_unpack_subimage)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

/*
 * Upload a region of data
 */

void
glamor_upload_region(PixmapPtr pixmap, RegionPtr region,
                     int region_x, int region_y,
                     uint8_t *bits, uint32_t byte_stride)
{
    glamor_upload_boxes(pixmap, RegionRects(region), RegionNumRects(region),
                        -region_x, -region_y,
                        0, 0,
                        bits, byte_stride);
}

/*
 * Take the data in the pixmap and stuff it back into the FBO
 */
void
glamor_upload_pixmap(PixmapPtr pixmap)
{
    BoxRec box;

    box.x1 = 0;
    box.x2 = pixmap->drawable.width;
    box.y1 = 0;
    box.y2 = pixmap->drawable.height;
    glamor_upload_boxes(pixmap, &box, 1, 0, 0, 0, 0,
                        pixmap->devPrivate.ptr, pixmap->devKind);
}

/*
 * Read stuff from the pixmap FBOs and write to memory
 */
void
glamor_download_boxes(PixmapPtr pixmap, BoxPtr in_boxes, int in_nbox,
                      int dx_src, int dy_src,
                      int dx_dst, int dy_dst,
                      uint8_t *bits, uint32_t byte_stride)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);
    int box_index;
    int bytes_per_pixel = pixmap->drawable.bitsPerPixel >> 3;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);

    glamor_make_current(glamor_priv);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (glamor_priv->has_pack_subimage)
        glPixelStorei(GL_PACK_ROW_LENGTH, byte_stride / bytes_per_pixel);

    glamor_pixmap_loop(priv, box_index) {
        BoxPtr                  box = glamor_pixmap_box_at(priv, box_index);
        glamor_pixmap_fbo       *fbo = glamor_pixmap_fbo_at(priv, box_index);
        BoxPtr                  boxes = in_boxes;
        int                     nbox = in_nbox;

        /* This should not be called on GLAMOR_FBO_NO_FBO-allocated pixmaps. */
        assert(fbo->fb);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);

        while (nbox--) {

            /* compute drawable coordinates */
            int                     x1 = MAX(boxes->x1 + dx_src, box->x1);
            int                     x2 = MIN(boxes->x2 + dx_src, box->x2);
            int                     y1 = MAX(boxes->y1 + dy_src, box->y1);
            int                     y2 = MIN(boxes->y2 + dy_src, box->y2);
            size_t ofs = (y1 - dy_src + dy_dst) * byte_stride;
            ofs += (x1 - dx_src + dx_dst) * bytes_per_pixel;

            boxes++;

            if (x2 <= x1 || y2 <= y1)
                continue;

            if (glamor_priv->has_pack_subimage ||
                x2 - x1 == byte_stride / bytes_per_pixel) {
                glReadPixels(x1 - box->x1, y1 - box->y1, x2 - x1, y2 - y1, f->format, f->type, bits + ofs);
            } else {
                for (; y1 < y2; y1++, ofs += byte_stride)
                    glReadPixels(x1 - box->x1, y1 - box->y1, x2 - x1, 1, f->format, f->type, bits + ofs);
            }
        }
    }
    if (glamor_priv->has_pack_subimage)
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
}

/*
 * Read data from the pixmap FBO
 */
void
glamor_download_rect(PixmapPtr pixmap, int x, int y, int w, int h, uint8_t *bits)
{
    BoxRec      box;

    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;

    glamor_download_boxes(pixmap, &box, 1, 0, 0, -x, -y,
                          bits, PixmapBytePad(w, pixmap->drawable.depth));
}

/*
 * Pull the data from the FBO down to the pixmap
 */
void
glamor_download_pixmap(PixmapPtr pixmap)
{
    BoxRec      box;

    box.x1 = 0;
    box.x2 = pixmap->drawable.width;
    box.y1 = 0;
    box.y2 = pixmap->drawable.height;

    glamor_download_boxes(pixmap, &box, 1, 0, 0, 0, 0,
                          pixmap->devPrivate.ptr, pixmap->devKind);
}
