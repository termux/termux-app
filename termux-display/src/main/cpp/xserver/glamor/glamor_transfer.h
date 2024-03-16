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

#ifndef _GLAMOR_TRANSFER_H_
#define _GLAMOR_TRANSFER_H_

void
glamor_upload_boxes(PixmapPtr pixmap, BoxPtr in_boxes, int in_nbox,
                    int dx_src, int dy_src,
                    int dx_dst, int dy_dst,
                    uint8_t *bits, uint32_t byte_stride);

void
glamor_upload_region(PixmapPtr pixmap, RegionPtr region,
                     int region_x, int region_y,
                     uint8_t *bits, uint32_t byte_stride);

void
glamor_upload_pixmap(PixmapPtr pixmap);

void
glamor_download_boxes(PixmapPtr pixmap, BoxPtr in_boxes, int in_nbox,
                      int dx_src, int dy_src,
                      int dx_dst, int dy_dst,
                      uint8_t *bits, uint32_t byte_stride);

void
glamor_download_rect(PixmapPtr pixmap, int x, int y, int w, int h, uint8_t *bits);

void
glamor_download_pixmap(PixmapPtr pixmap);

#endif /* _GLAMOR_TRANSFER_H_ */
