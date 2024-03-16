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
#include "glamor_transfer.h"
#include "glamor_prepare.h"

static const char dash_vs_vars[] =
    "attribute vec3 primitive;\n"
    "varying float dash_offset;\n";

static const char dash_vs_exec[] =
    "       dash_offset = primitive.z / dash_length;\n"
    "       vec2 pos = vec2(0,0);\n"
    GLAMOR_POS(gl_Position, primitive.xy);

static const char dash_fs_vars[] =
    "varying float dash_offset;\n";

static const char on_off_fs_exec[] =
    "       float pattern = texture2D(dash, vec2(dash_offset, 0.5)).w;\n"
    "       if (pattern == 0.0)\n"
    "               discard;\n";

/* XXX deal with stippled double dashed lines once we have stippling support */
static const char double_fs_exec[] =
    "       float pattern = texture2D(dash, vec2(dash_offset, 0.5)).w;\n"
    "       if (pattern == 0.0)\n"
    "               gl_FragColor = bg;\n"
    "       else\n"
    "               gl_FragColor = fg;\n";


static const glamor_facet glamor_facet_on_off_dash_lines = {
    .version = 130,
    .name = "poly_lines_on_off_dash",
    .vs_vars = dash_vs_vars,
    .vs_exec = dash_vs_exec,
    .fs_vars = dash_fs_vars,
    .fs_exec = on_off_fs_exec,
    .locations = glamor_program_location_dash,
};

static const glamor_facet glamor_facet_double_dash_lines = {
    .version = 130,
    .name = "poly_lines_double_dash",
    .vs_vars = dash_vs_vars,
    .vs_exec = dash_vs_exec,
    .fs_vars = dash_fs_vars,
    .fs_exec = double_fs_exec,
    .locations = (glamor_program_location_dash|
                  glamor_program_location_fg|
                  glamor_program_location_bg),
};

static PixmapPtr
glamor_get_dash_pixmap(GCPtr gc)
{
    glamor_gc_private *gc_priv = glamor_get_gc_private(gc);
    ScreenPtr   screen = gc->pScreen;
    PixmapPtr   pixmap;
    int         offset;
    int         d;
    uint32_t    pixel;
    GCPtr       scratch_gc;

    if (gc_priv->dash)
        return gc_priv->dash;

    offset = 0;
    for (d = 0; d < gc->numInDashList; d++)
        offset += gc->dash[d];

    pixmap = glamor_create_pixmap(screen, offset, 1, 8, 0);
    if (!pixmap)
        goto bail;

    scratch_gc = GetScratchGC(8, screen);
    if (!scratch_gc)
        goto bail_pixmap;

    pixel = 0xffffffff;
    offset = 0;
    for (d = 0; d < gc->numInDashList; d++) {
        xRectangle      rect;
        ChangeGCVal     changes;

        changes.val = pixel;
        (void) ChangeGC(NullClient, scratch_gc,
                        GCForeground, &changes);
        ValidateGC(&pixmap->drawable, scratch_gc);
        rect.x = offset;
        rect.y = 0;
        rect.width = gc->dash[d];
        rect.height = 1;
        scratch_gc->ops->PolyFillRect (&pixmap->drawable, scratch_gc, 1, &rect);
        offset += gc->dash[d];
        pixel = ~pixel;
    }
    FreeScratchGC(scratch_gc);

    gc_priv->dash = pixmap;
    return pixmap;

bail_pixmap:
    glamor_destroy_pixmap(pixmap);
bail:
    return NULL;
}

static glamor_program *
glamor_dash_setup(DrawablePtr drawable, GCPtr gc)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    PixmapPtr dash_pixmap;
    glamor_pixmap_private *dash_priv;
    glamor_program *prog;

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    if (gc->lineWidth != 0)
        goto bail;

    dash_pixmap = glamor_get_dash_pixmap(gc);
    dash_priv = glamor_get_pixmap_private(dash_pixmap);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dash_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    switch (gc->lineStyle) {
    case LineOnOffDash:
        prog = glamor_use_program_fill(pixmap, gc,
                                       &glamor_priv->on_off_dash_line_progs,
                                       &glamor_facet_on_off_dash_lines);
        if (!prog)
            goto bail;
        break;
    case LineDoubleDash:
        if (gc->fillStyle != FillSolid)
            goto bail;

        prog = &glamor_priv->double_dash_line_prog;

        if (!prog->prog) {
            if (!glamor_build_program(screen, prog,
                                      &glamor_facet_double_dash_lines,
                                      NULL, NULL, NULL))
                goto bail;
        }

        if (!glamor_use_program(pixmap, gc, prog, NULL))
            goto bail;

        glamor_set_color(pixmap, gc->fgPixel, prog->fg_uniform);
        glamor_set_color(pixmap, gc->bgPixel, prog->bg_uniform);
        break;

    default:
        goto bail;
    }


    /* Set the dash pattern as texture 1 */

    glamor_bind_texture(glamor_priv, GL_TEXTURE1, dash_priv->fbo, FALSE);
    glUniform1i(prog->dash_uniform, 1);
    glUniform1f(prog->dash_length_uniform, dash_pixmap->drawable.width);

    return prog;

bail:
    return NULL;
}

static void
glamor_dash_loop(DrawablePtr drawable, GCPtr gc, glamor_program *prog,
                 int n, GLenum mode)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    int box_index;
    int off_x, off_y;

    glEnable(GL_SCISSOR_TEST);

    glamor_pixmap_loop(pixmap_priv, box_index) {
        int nbox = RegionNumRects(gc->pCompositeClip);
        BoxPtr box = RegionRects(gc->pCompositeClip);

        glamor_set_destination_drawable(drawable, box_index, TRUE, TRUE,
                                        prog->matrix_uniform, &off_x, &off_y);

        while (nbox--) {
            glScissor(box->x1 + off_x,
                      box->y1 + off_y,
                      box->x2 - box->x1,
                      box->y2 - box->y1);
            box++;
            glDrawArrays(mode, 0, n);
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
}

static int
glamor_line_length(short x1, short y1, short x2, short y2)
{
    return max(abs(x2 - x1), abs(y2 - y1));
}

Bool
glamor_poly_lines_dash_gl(DrawablePtr drawable, GCPtr gc,
                          int mode, int n, DDXPointPtr points)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_program *prog;
    short *v;
    char *vbo_offset;
    int add_last;
    int dash_pos;
    int prev_x, prev_y;
    int i;

    if (n < 2)
        return TRUE;

    if (!(prog = glamor_dash_setup(drawable, gc)))
        return FALSE;

    add_last = 0;
    if (gc->capStyle != CapNotLast)
        add_last = 1;

    /* Set up the vertex buffers for the points */

    v = glamor_get_vbo_space(drawable->pScreen,
                             (n + add_last) * 3 * sizeof (short),
                             &vbo_offset);

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 3, GL_SHORT, GL_FALSE,
                          3 * sizeof (short), vbo_offset);

    dash_pos = gc->dashOffset;
    prev_x = prev_y = 0;
    for (i = 0; i < n; i++) {
        int this_x = points[i].x;
        int this_y = points[i].y;
        if (i) {
            if (mode == CoordModePrevious) {
                this_x += prev_x;
                this_y += prev_y;
            }
            dash_pos += glamor_line_length(prev_x, prev_y,
                                           this_x, this_y);
        }
        v[0] = prev_x = this_x;
        v[1] = prev_y = this_y;
        v[2] = dash_pos;
        v += 3;
    }

    if (add_last) {
        v[0] = prev_x + 1;
        v[1] = prev_y;
        v[2] = dash_pos + 1;
    }

    glamor_put_vbo_space(screen);

    glamor_dash_loop(drawable, gc, prog, n + add_last, GL_LINE_STRIP);

    return TRUE;
}

static short *
glamor_add_segment(short *v, short x1, short y1, short x2, short y2,
                   int dash_start, int dash_end)
{
    v[0] = x1;
    v[1] = y1;
    v[2] = dash_start;

    v[3] = x2;
    v[4] = y2;
    v[5] = dash_end;
    return v + 6;
}

Bool
glamor_poly_segment_dash_gl(DrawablePtr drawable, GCPtr gc,
                            int nseg, xSegment *segs)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_program *prog;
    short *v;
    char *vbo_offset;
    int dash_start = gc->dashOffset;
    int add_last;
    int i;

    if (!(prog = glamor_dash_setup(drawable, gc)))
        return FALSE;

    add_last = 0;
    if (gc->capStyle != CapNotLast)
        add_last = 1;

    /* Set up the vertex buffers for the points */

    v = glamor_get_vbo_space(drawable->pScreen,
                             (nseg<<add_last) * 6 * sizeof (short),
                             &vbo_offset);

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 3, GL_SHORT, GL_FALSE,
                          3 * sizeof (short), vbo_offset);

    for (i = 0; i < nseg; i++) {
        int dash_end = dash_start + glamor_line_length(segs[i].x1, segs[i].y1,
                                                       segs[i].x2, segs[i].y2);
        v = glamor_add_segment(v,
                               segs[i].x1, segs[i].y1,
                               segs[i].x2, segs[i].y2,
                               dash_start, dash_end);
        if (add_last)
            v = glamor_add_segment(v,
                                   segs[i].x2, segs[i].y2,
                                   segs[i].x2 + 1, segs[i].y2,
                                   dash_end, dash_end + 1);
    }

    glamor_put_vbo_space(screen);

    glamor_dash_loop(drawable, gc, prog, nseg << (1 + add_last), GL_LINES);

    return TRUE;
}
