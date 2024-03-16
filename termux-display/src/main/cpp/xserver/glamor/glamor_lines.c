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
#include "glamor_program.h"
#include "glamor_transform.h"
#include "glamor_prepare.h"

static const glamor_facet glamor_facet_poly_lines = {
    .name = "poly_lines",
    .vs_vars = "attribute vec2 primitive;\n",
    .vs_exec = ("       vec2 pos = vec2(0.0,0.0);\n"
                GLAMOR_POS(gl_Position, primitive.xy)),
};

static Bool
glamor_poly_lines_solid_gl(DrawablePtr drawable, GCPtr gc,
                           int mode, int n, DDXPointPtr points)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv;
    glamor_program *prog;
    int off_x, off_y;
    DDXPointPtr v;
    char *vbo_offset;
    int box_index;
    int add_last;
    Bool ret = FALSE;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    add_last = 0;
    if (gc->capStyle != CapNotLast)
        add_last = 1;

    if (n < 2)
        return TRUE;

    glamor_make_current(glamor_priv);

    prog = glamor_use_program_fill(pixmap, gc,
                                   &glamor_priv->poly_line_program,
                                   &glamor_facet_poly_lines);

    if (!prog)
        goto bail;

    /* Set up the vertex buffers for the points */

    v = glamor_get_vbo_space(drawable->pScreen,
                             (n + add_last) * sizeof (DDXPointRec),
                             &vbo_offset);

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT, GL_FALSE,
                          sizeof (DDXPointRec), vbo_offset);

    if (mode == CoordModePrevious) {
        int i;
        DDXPointRec here = { 0, 0 };

        for (i = 0; i < n; i++) {
            here.x += points[i].x;
            here.y += points[i].y;
            v[i] = here;
        }
    } else {
        memcpy(v, points, n * sizeof (DDXPointRec));
    }

    if (add_last) {
        v[n].x = v[n-1].x + 1;
        v[n].y = v[n-1].y;
    }

    glamor_put_vbo_space(screen);

    glEnable(GL_SCISSOR_TEST);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        int nbox = RegionNumRects(gc->pCompositeClip);
        BoxPtr box = RegionRects(gc->pCompositeClip);

        if (!glamor_set_destination_drawable(drawable, box_index, TRUE, TRUE,
                                             prog->matrix_uniform, &off_x, &off_y))
            goto bail;

        while (nbox--) {
            glScissor(box->x1 + off_x,
                      box->y1 + off_y,
                      box->x2 - box->x1,
                      box->y2 - box->y1);
            box++;
            glDrawArrays(GL_LINE_STRIP, 0, n + add_last);
        }
    }

    ret = TRUE;

bail:
    glDisable(GL_SCISSOR_TEST);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

static Bool
glamor_poly_lines_gl(DrawablePtr drawable, GCPtr gc,
                     int mode, int n, DDXPointPtr points)
{
    if (gc->lineWidth != 0)
        return FALSE;

    switch (gc->lineStyle) {
    case LineSolid:
        return glamor_poly_lines_solid_gl(drawable, gc, mode, n, points);
    case LineOnOffDash:
        return glamor_poly_lines_dash_gl(drawable, gc, mode, n, points);
    case LineDoubleDash:
        if (gc->fillStyle == FillTiled)
            return glamor_poly_lines_solid_gl(drawable, gc, mode, n, points);
        else
            return glamor_poly_lines_dash_gl(drawable, gc, mode, n, points);
    default:
        return FALSE;
    }
}

static void
glamor_poly_lines_bail(DrawablePtr drawable, GCPtr gc,
                       int mode, int n, DDXPointPtr points)
{
    glamor_fallback("to %p (%c)\n", drawable,
                    glamor_get_drawable_location(drawable));

    miPolylines(drawable, gc, mode, n, points);
}

void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc,
                  int mode, int n, DDXPointPtr points)
{
    if (glamor_poly_lines_gl(drawable, gc, mode, n, points))
        return;
    glamor_poly_lines_bail(drawable, gc, mode, n, points);
}
