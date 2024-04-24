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
#include "glamor_transform.h"
#include "glamor_transfer.h"

glamor_program  fill_spans_progs[4];

static const glamor_facet glamor_facet_fillspans_130 = {
    .name = "fill_spans",
    .version = 130,
    .vs_vars =  "attribute vec3 primitive;\n",
    .vs_exec = ("       vec2 pos = vec2(primitive.z,1) * vec2(gl_VertexID&1, (gl_VertexID&2)>>1);\n"
                GLAMOR_POS(gl_Position, (primitive.xy + pos))),
};

static const glamor_facet glamor_facet_fillspans_120 = {
    .name = "fill_spans",
    .vs_vars =  "attribute vec2 primitive;\n",
    .vs_exec = ("       vec2 pos = vec2(0,0);\n"
                GLAMOR_POS(gl_Position, primitive.xy)),
};

static Bool
glamor_fill_spans_gl(DrawablePtr drawable,
                     GCPtr gc,
                     int n, DDXPointPtr points, int *widths, int sorted)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    glamor_program *prog;
    int off_x, off_y;
    GLshort *v;
    char *vbo_offset;
    int c;
    int box_index;
    Bool ret = FALSE;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    if (glamor_glsl_has_ints(glamor_priv)) {
        prog = glamor_use_program_fill(pixmap, gc, &glamor_priv->fill_spans_program,
                                       &glamor_facet_fillspans_130);

        if (!prog)
            goto bail;

        /* Set up the vertex buffers for the points */

        v = glamor_get_vbo_space(drawable->pScreen, n * (4 * sizeof (GLshort)), &vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
        glVertexAttribDivisor(GLAMOR_VERTEX_POS, 1);
        glVertexAttribPointer(GLAMOR_VERTEX_POS, 3, GL_SHORT, GL_FALSE,
                              4 * sizeof (GLshort), vbo_offset);

        for (c = 0; c < n; c++) {
            v[0] = points->x;
            v[1] = points->y;
            v[2] = *widths++;
            points++;
            v += 4;
        }

        glamor_put_vbo_space(screen);
    } else {
        prog = glamor_use_program_fill(pixmap, gc, &glamor_priv->fill_spans_program,
                                       &glamor_facet_fillspans_120);

        if (!prog)
            goto bail;

        /* Set up the vertex buffers for the points */

        v = glamor_get_vbo_space(drawable->pScreen, n * 8 * sizeof (short), &vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
        glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT, GL_FALSE,
                              2 * sizeof (short), vbo_offset);

        for (c = 0; c < n; c++) {
            v[0] = points->x;           v[1] = points->y;
            v[2] = points->x;           v[3] = points->y + 1;
            v[4] = points->x + *widths; v[5] = points->y + 1;
            v[6] = points->x + *widths; v[7] = points->y;

            widths++;
            points++;
            v += 8;
        }

        glamor_put_vbo_space(screen);
    }

    glEnable(GL_SCISSOR_TEST);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        int nbox = RegionNumRects(gc->pCompositeClip);
        BoxPtr box = RegionRects(gc->pCompositeClip);

        if (!glamor_set_destination_drawable(drawable, box_index, FALSE, FALSE,
                                             prog->matrix_uniform, &off_x, &off_y))
            goto bail;

        while (nbox--) {
            glScissor(box->x1 + off_x,
                      box->y1 + off_y,
                      box->x2 - box->x1,
                      box->y2 - box->y1);
            box++;
            if (glamor_glsl_has_ints(glamor_priv))
                glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, n);
            else {
                glamor_glDrawArrays_GL_QUADS(glamor_priv, n);
            }
        }
    }

    ret = TRUE;

bail:
    glDisable(GL_SCISSOR_TEST);
    if (glamor_glsl_has_ints(glamor_priv))
        glVertexAttribDivisor(GLAMOR_VERTEX_POS, 0);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

static void
glamor_fill_spans_bail(DrawablePtr drawable,
                       GCPtr gc,
                       int n, DDXPointPtr points, int *widths, int sorted)
{
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW) &&
        glamor_prepare_access_gc(gc)) {
        fbFillSpans(drawable, gc, n, points, widths, sorted);
    }
    glamor_finish_access_gc(gc);
    glamor_finish_access(drawable);
}

void
glamor_fill_spans(DrawablePtr drawable,
                  GCPtr gc,
                  int n, DDXPointPtr points, int *widths, int sorted)
{
    if (glamor_fill_spans_gl(drawable, gc, n, points, widths, sorted))
        return;
    glamor_fill_spans_bail(drawable, gc, n, points, widths, sorted);
}

static Bool
glamor_get_spans_gl(DrawablePtr drawable, int wmax,
                    DDXPointPtr points, int *widths, int count, char *dst)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    int box_index;
    int n;
    char *d;
    int off_x, off_y;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);

    glamor_make_current(glamor_priv);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        BoxPtr                  box = glamor_pixmap_box_at(pixmap_priv, box_index);
        glamor_pixmap_fbo       *fbo = glamor_pixmap_fbo_at(pixmap_priv, box_index);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

        d = dst;
        for (n = 0; n < count; n++) {
            int x1 = points[n].x + off_x;
            int y = points[n].y + off_y;
            int w = widths[n];
            int x2 = x1 + w;
            char *l;

            l = d;
            d += PixmapBytePad(w, drawable->depth);

            /* clip */
            if (x1 < box->x1) {
                l += (box->x1 - x1) * (drawable->bitsPerPixel >> 3);
                x1 = box->x1;
            }
            if (x2 > box->x2)
                x2 = box->x2;

            if (x1 >= x2)
                continue;
            if (y < box->y1)
                continue;
            if (y >= box->y2)
                continue;

            glReadPixels(x1 - box->x1, y - box->y1, x2 - x1, 1,
                         f->format, f->type, l);
        }
    }

    return TRUE;
bail:
    return FALSE;
}

static void
glamor_get_spans_bail(DrawablePtr drawable, int wmax,
                 DDXPointPtr points, int *widths, int count, char *dst)
{
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RO))
        fbGetSpans(drawable, wmax, points, widths, count, dst);
    glamor_finish_access(drawable);
}

void
glamor_get_spans(DrawablePtr drawable, int wmax,
                 DDXPointPtr points, int *widths, int count, char *dst)
{
    if (glamor_get_spans_gl(drawable, wmax, points, widths, count, dst))
        return;
    glamor_get_spans_bail(drawable, wmax, points, widths, count, dst);
}

static Bool
glamor_set_spans_gl(DrawablePtr drawable, GCPtr gc, char *src,
                    DDXPointPtr points, int *widths, int numPoints, int sorted)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);
    int box_index;
    int n;
    char *s;
    int off_x, off_y;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    if (gc->alu != GXcopy)
        goto bail;

    if (!glamor_pm_is_solid(gc->depth, gc->planemask))
        goto bail;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);

    glamor_make_current(glamor_priv);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        BoxPtr              box = glamor_pixmap_box_at(pixmap_priv, box_index);
        glamor_pixmap_fbo  *fbo = glamor_pixmap_fbo_at(pixmap_priv, box_index);

        glamor_bind_texture(glamor_priv, GL_TEXTURE0, fbo, TRUE);

        s = src;
        for (n = 0; n < numPoints; n++) {

            BoxPtr      clip_box = RegionRects(gc->pCompositeClip);
            int         nclip_box = RegionNumRects(gc->pCompositeClip);
            int         w = widths[n];
            int         y = points[n].y;
            int         x = points[n].x;

            while (nclip_box--) {
                int x1 = x;
                int x2 = x + w;
                int y1 = y;
                char *l = s;

                /* clip to composite clip */
                if (x1 < clip_box->x1) {
                    l += (clip_box->x1 - x1) * (drawable->bitsPerPixel >> 3);
                    x1 = clip_box->x1;
                }
                if (x2 > clip_box->x2)
                    x2 = clip_box->x2;

                if (y < clip_box->y1)
                    continue;
                if (y >= clip_box->y2)
                    continue;

                /* adjust to pixmap coordinates */
                x1 += off_x;
                x2 += off_x;
                y1 += off_y;

                if (x1 < box->x1) {
                    l += (box->x1 - x1) * (drawable->bitsPerPixel >> 3);
                    x1 = box->x1;
                }
                if (x2 > box->x2)
                    x2 = box->x2;

                if (x1 >= x2)
                    continue;
                if (y1 < box->y1)
                    continue;
                if (y1 >= box->y2)
                    continue;

                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x1 - box->x1, y1 - box->y1, x2 - x1, 1,
                                f->format, f->type,
                                l);
            }
            s += PixmapBytePad(w, drawable->depth);
        }
    }

    return TRUE;

bail:
    return FALSE;
}

static void
glamor_set_spans_bail(DrawablePtr drawable, GCPtr gc, char *src,
                      DDXPointPtr points, int *widths, int numPoints, int sorted)
{
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW) && glamor_prepare_access_gc(gc))
        fbSetSpans(drawable, gc, src, points, widths, numPoints, sorted);
    glamor_finish_access_gc(gc);
    glamor_finish_access(drawable);
}

void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
                 DDXPointPtr points, int *widths, int numPoints, int sorted)
{
    if (glamor_set_spans_gl(drawable, gc, src, points, widths, numPoints, sorted))
        return;
    glamor_set_spans_bail(drawable, gc, src, points, widths, numPoints, sorted);
}
