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

#ifndef _GLAMOR_TRANSFORM_H_
#define _GLAMOR_TRANSFORM_H_

Bool
glamor_set_destination_drawable(DrawablePtr     drawable,
                                int             box_index,
                                Bool            do_drawable_translate,
                                Bool            center_offset,
                                GLint           matrix_uniform_location,
                                int             *p_off_x,
                                int             *p_off_y);

void
glamor_set_color_depth(ScreenPtr      pScreen,
                       int            depth,
                       CARD32         pixel,
                       GLint          uniform);

static inline void
glamor_set_color(PixmapPtr      pixmap,
                 CARD32         pixel,
                 GLint          uniform)
{
    glamor_set_color_depth(pixmap->drawable.pScreen,
                           pixmap->drawable.depth, pixel, uniform);
}

Bool
glamor_set_texture_pixmap(PixmapPtr     texture,
                          Bool          destination_red);

Bool
glamor_set_texture(PixmapPtr    texture,
                   Bool         destination_red,
                   int          off_x,
                   int          off_y,
                   GLint        offset_uniform,
                   GLint        size_uniform);

Bool
glamor_set_solid(PixmapPtr      pixmap,
                 GCPtr          gc,
                 Bool           use_alu,
                 GLint          uniform);

Bool
glamor_set_tiled(PixmapPtr      pixmap,
                 GCPtr          gc,
                 GLint          offset_uniform,
                 GLint          size_uniform);

Bool
glamor_set_stippled(PixmapPtr      pixmap,
                    GCPtr          gc,
                    GLint          fg_uniform,
                    GLint          offset_uniform,
                    GLint          size_uniform);

/*
 * Vertex shader bits that transform X coordinates to pixmap
 * coordinates using the matrix computed above
 */

#define GLAMOR_DECLARE_MATRIX   "uniform vec4 v_matrix;\n"
#define GLAMOR_X_POS(x) #x " *v_matrix.x + v_matrix.y"
#define GLAMOR_Y_POS(y) #y " *v_matrix.z + v_matrix.w"
#if 0
#define GLAMOR_POS(dst,src) \
    "       " #dst ".x = " #src ".x * v_matrix.x + v_matrix.y;\n" \
    "       " #dst ".y = " #src ".y * v_matrix.z + v_matrix.w;\n" \
    "       " #dst ".z = 0.0;\n" \
    "       " #dst ".w = 1.0;\n"
#endif
#define GLAMOR_POS(dst,src) \
    "       " #dst ".xy = " #src ".xy * v_matrix.xz + v_matrix.yw;\n" \
    "       " #dst ".zw = vec2(0.0,1.0);\n"

#endif /* _GLAMOR_TRANSFORM_H_ */
