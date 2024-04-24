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
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#include "glamor_priv.h"
#include "glamor_transform.h"

static const glamor_facet glamor_facet_point = {
    .name = "poly_point",
    .vs_vars = "attribute vec2 primitive;\n",
    .vs_exec = GLAMOR_POS(gl_Position, primitive),
};

static Bool
glamor_poly_point_gl(DrawablePtr drawable, GCPtr gc, int mode, int npt, DDXPointPtr ppt)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_program *prog = &glamor_priv->point_prog;
    glamor_pixmap_private *pixmap_priv;
    int off_x, off_y;
    GLshort *vbo_ppt;
    char *vbo_offset;
    int box_index;
    Bool ret = FALSE;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    if (prog->failed)
        goto bail;

    if (!prog->prog) {
        if (!glamor_build_program(screen, prog,
                                  &glamor_facet_point,
                                  &glamor_fill_solid,
                                  NULL, NULL))
            goto bail;
    }

    if (!glamor_use_program(pixmap, gc, prog, NULL))
        goto bail;

    vbo_ppt = glamor_get_vbo_space(screen, npt * (2 * sizeof (INT16)), &vbo_offset);
    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT, GL_FALSE, 0, vbo_offset);
    if (mode == CoordModePrevious) {
        int n = npt;
        INT16 x = 0, y = 0;
        while (n--) {
            vbo_ppt[0] = (x += ppt->x);
            vbo_ppt[1] = (y += ppt->y);
            vbo_ppt += 2;
            ppt++;
        }
    } else
        memcpy(vbo_ppt, ppt, npt * (2 * sizeof (INT16)));
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
            glDrawArrays(GL_POINTS, 0, npt);
        }
    }

    ret = TRUE;

bail:
    glDisable(GL_SCISSOR_TEST);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

void
glamor_poly_point(DrawablePtr drawable, GCPtr gc, int mode, int npt,
                  DDXPointPtr ppt)
{
    if (glamor_poly_point_gl(drawable, gc, mode, npt, ppt))
        return;
    miPolyPoint(drawable, gc, mode, npt, ppt);
}
