/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#include "glamor_priv.h"
#include <dixfontstr.h>
#include "glamor_transform.h"

static const glamor_facet glamor_facet_poly_glyph_blt = {
    .name = "poly_glyph_blt",
    .vs_vars = "attribute vec2 primitive;\n",
    .vs_exec = ("       vec2 pos = vec2(0,0);\n"
                GLAMOR_POS(gl_Position, primitive)),
};

static Bool
glamor_poly_glyph_blt_gl(DrawablePtr drawable, GCPtr gc,
                         int start_x, int y, unsigned int nglyph,
                         CharInfoPtr *ppci, void *pglyph_base)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    glamor_program *prog;
    RegionPtr clip = gc->pCompositeClip;
    int box_index;
    Bool ret = FALSE;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    prog = glamor_use_program_fill(pixmap, gc,
                                   &glamor_priv->poly_glyph_blt_progs,
                                   &glamor_facet_poly_glyph_blt);
    if (!prog)
        goto bail;

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

    start_x += drawable->x;
    y += drawable->y;

    glamor_pixmap_loop(pixmap_priv, box_index) {
        int x;
        int n;
        int num_points, max_points;
        INT16 *points = NULL;
        int off_x, off_y;
        char *vbo_offset;

        if (!glamor_set_destination_drawable(drawable, box_index, FALSE, TRUE,
                                              prog->matrix_uniform, &off_x, &off_y))
            goto bail;

        max_points = 500;
        num_points = 0;
        x = start_x;
        for (n = 0; n < nglyph; n++) {
            CharInfoPtr charinfo = ppci[n];
            int w = GLYPHWIDTHPIXELS(charinfo);
            int h = GLYPHHEIGHTPIXELS(charinfo);
            uint8_t *glyphbits = FONTGLYPHBITS(NULL, charinfo);

            if (w && h) {
                int glyph_x = x + charinfo->metrics.leftSideBearing;
                int glyph_y = y - charinfo->metrics.ascent;
                int glyph_stride = GLYPHWIDTHBYTESPADDED(charinfo);
                int xx, yy;

                for (yy = 0; yy < h; yy++) {
                    uint8_t *glyph = glyphbits;
                    for (xx = 0; xx < w; glyph += ((xx&7) == 7), xx++) {
                        int pt_x_i = glyph_x + xx;
                        int pt_y_i = glyph_y + yy;

                        if (!(*glyph & (1 << (xx & 7))))
                            continue;

                        if (!RegionContainsPoint(clip, pt_x_i, pt_y_i, NULL))
                            continue;

                        if (!num_points) {
                            points = glamor_get_vbo_space(screen,
                                                          max_points *
                                                          (2 * sizeof (INT16)),
                                                          &vbo_offset);

                            glVertexAttribPointer(GLAMOR_VERTEX_POS,
                                                  2, GL_SHORT,
                                                  GL_FALSE, 0, vbo_offset);
                        }

                        *points++ = pt_x_i;
                        *points++ = pt_y_i;
                        num_points++;

                        if (num_points == max_points) {
                            glamor_put_vbo_space(screen);
                            glDrawArrays(GL_POINTS, 0, num_points);
                            num_points = 0;
                        }
                    }
                    glyphbits += glyph_stride;
                }
            }
            x += charinfo->metrics.characterWidth;
        }

        if (num_points) {
            glamor_put_vbo_space(screen);
            glDrawArrays(GL_POINTS, 0, num_points);
        }
    }

    ret = TRUE;

bail:
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

void
glamor_poly_glyph_blt(DrawablePtr drawable, GCPtr gc,
                      int start_x, int y, unsigned int nglyph,
                      CharInfoPtr *ppci, void *pglyph_base)
{
    if (glamor_poly_glyph_blt_gl(drawable, gc, start_x, y, nglyph, ppci,
                                 pglyph_base))
        return;
    miPolyGlyphBlt(drawable, gc, start_x, y, nglyph,
                   ppci, pglyph_base);
}

static Bool
glamor_push_pixels_gl(GCPtr gc, PixmapPtr bitmap,
                      DrawablePtr drawable, int w, int h, int x, int y)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    uint8_t *bitmap_data = bitmap->devPrivate.ptr;
    int bitmap_stride = bitmap->devKind;
    glamor_program *prog;
    RegionPtr clip = gc->pCompositeClip;
    int box_index;
    int yy, xx;
    int num_points;
    INT16 *points = NULL;
    char *vbo_offset;
    Bool ret = FALSE;

    if (w * h > MAXINT / (2 * sizeof(float)))
        goto bail;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    prog = glamor_use_program_fill(pixmap, gc,
                                   &glamor_priv->poly_glyph_blt_progs,
                                   &glamor_facet_poly_glyph_blt);
    if (!prog)
        goto bail;

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

    points = glamor_get_vbo_space(screen, w * h * sizeof(INT16) * 2,
                                  &vbo_offset);
    num_points = 0;

    /* Note that because fb sets miTranslate in the GC, our incoming X
     * and Y are in screen coordinate space (same for spans, but not
     * other operations).
     */

    for (yy = 0; yy < h; yy++) {
        uint8_t *bitmap_row = bitmap_data + yy * bitmap_stride;
        for (xx = 0; xx < w; xx++) {
            if (bitmap_row[xx / 8] & (1 << xx % 8) &&
                RegionContainsPoint(clip,
                                    x + xx,
                                    y + yy,
                                    NULL)) {
                *points++ = x + xx;
                *points++ = y + yy;
                num_points++;
            }
        }
    }
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT,
                          GL_FALSE, 0, vbo_offset);

    glamor_put_vbo_space(screen);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        if (!glamor_set_destination_drawable(drawable, box_index, FALSE, TRUE,
                                             prog->matrix_uniform, NULL, NULL))
            goto bail;

        glDrawArrays(GL_POINTS, 0, num_points);
    }

    ret = TRUE;

bail:
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

void
glamor_push_pixels(GCPtr pGC, PixmapPtr pBitmap,
                   DrawablePtr pDrawable, int w, int h, int x, int y)
{
    if (glamor_push_pixels_gl(pGC, pBitmap, pDrawable, w, h, x, y))
        return;

    miPushPixels(pGC, pBitmap, pDrawable, w, h, x, y);
}
