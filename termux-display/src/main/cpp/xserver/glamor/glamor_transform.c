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


/*
 * Set up rendering to target the specified drawable, computing an
 * appropriate transform for the vertex shader to convert
 * drawable-relative coordinates into pixmap-relative coordinates. If
 * requested, the offset from pixmap origin coordinates back to window
 * system coordinates will be returned in *p_off_x, *p_off_y so that
 * clipping computations can be adjusted as appropriate
 */

Bool
glamor_set_destination_drawable(DrawablePtr     drawable,
                                int             box_index,
                                Bool            do_drawable_translate,
                                Bool            center_offset,
                                GLint           matrix_uniform_location,
                                int             *p_off_x,
                                int             *p_off_y)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    int off_x, off_y;
    BoxPtr box = glamor_pixmap_box_at(pixmap_priv, box_index);
    int w = box->x2 - box->x1;
    int h = box->y2 - box->y1;
    float scale_x = 2.0f / (float) w;
    float scale_y = 2.0f / (float) h;
    float center_adjust = 0.0f;
    glamor_pixmap_fbo *pixmap_fbo;

    pixmap_fbo = glamor_pixmap_fbo_at(pixmap_priv, box_index);
    if (!pixmap_fbo)
        return FALSE;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);

    off_x -= box->x1;
    off_y -= box->y1;

    if (p_off_x) {
        *p_off_x = off_x;
        *p_off_y = off_y;
    }

    /* A tricky computation to find the right value for the two linear functions
     * that transform rendering coordinates to pixmap coordinates
     *
     *  pixmap_x = render_x + drawable->x + off_x
     *  pixmap_y = render_y + drawable->y + off_y
     *
     *  gl_x = pixmap_x * 2 / width - 1
     *  gl_y = pixmap_y * 2 / height - 1
     *
     *  gl_x = (render_x + drawable->x + off_x) * 2 / width - 1
     *
     *  gl_x = (render_x) * 2 / width + (drawable->x + off_x) * 2 / width - 1
     */

    if (do_drawable_translate) {
        off_x += drawable->x;
        off_y += drawable->y;
    }

    /*
     * To get GL_POINTS drawn in the right spot, we need to adjust the
     * coordinates by 1/2 a pixel.
     */
    if (center_offset)
        center_adjust = 0.5f;

    glUniform4f(matrix_uniform_location,
                scale_x, (off_x + center_adjust) * scale_x - 1.0f,
                scale_y, (off_y + center_adjust) * scale_y - 1.0f);

    glamor_set_destination_pixmap_fbo(glamor_priv, pixmap_fbo,
                                      0, 0, w, h);

    return TRUE;
}

/*
 * Set up for solid rendering to the specified pixmap using alu, fg and planemask
 * from the specified GC. Load the target color into the specified uniform
 */

void
glamor_set_color_depth(ScreenPtr      pScreen,
                       int            depth,
                       CARD32         pixel,
                       GLint          uniform)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    float       color[4];

    glamor_get_rgba_from_pixel(pixel,
                               &color[0], &color[1], &color[2], &color[3],
                               glamor_priv->formats[depth].render_format);

    if ((depth <= 8) && glamor_priv->formats[8].format == GL_RED)
      color[0] = color[3];

    glUniform4fv(uniform, 1, color);
}

Bool
glamor_set_solid(PixmapPtr      pixmap,
                 GCPtr          gc,
                 Bool           use_alu,
                 GLint          uniform)
{
    CARD32      pixel;
    int         alu = use_alu ? gc->alu : GXcopy;

    if (!glamor_set_planemask(gc->depth, gc->planemask))
        return FALSE;

    pixel = gc->fgPixel;

    if (!glamor_set_alu(pixmap->drawable.pScreen, alu)) {
        switch (gc->alu) {
        case GXclear:
            pixel = 0;
            break;
        case GXcopyInverted:
            pixel = ~pixel;
            break;
        case GXset:
            pixel = ~0 & gc->planemask;
            break;
        default:
            return FALSE;
        }
    }
    glamor_set_color(pixmap, pixel, uniform);

    return TRUE;
}

Bool
glamor_set_texture_pixmap(PixmapPtr texture, Bool destination_red)
{
    glamor_pixmap_private *texture_priv;

    texture_priv = glamor_get_pixmap_private(texture);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(texture_priv))
        return FALSE;

    if (glamor_pixmap_priv_is_large(texture_priv))
        return FALSE;

    glamor_bind_texture(glamor_get_screen_private(texture->drawable.pScreen),
                        GL_TEXTURE0,
                        texture_priv->fbo, destination_red);

    /* we're not setting the sampler uniform here as we always use
     * GL_TEXTURE0, and the default value for uniforms is zero. So,
     * save a bit of CPU time by taking advantage of that.
     */
    return TRUE;
}

Bool
glamor_set_texture(PixmapPtr    texture,
                   Bool         destination_red,
                   int          off_x,
                   int          off_y,
                   GLint        offset_uniform,
                   GLint        size_inv_uniform)
{
    if (!glamor_set_texture_pixmap(texture, destination_red))
        return FALSE;

    glUniform2f(offset_uniform, off_x, off_y);
    glUniform2f(size_inv_uniform, 1.0f/texture->drawable.width, 1.0f/texture->drawable.height);
    return TRUE;
}

Bool
glamor_set_tiled(PixmapPtr      pixmap,
                 GCPtr          gc,
                 GLint          offset_uniform,
                 GLint          size_inv_uniform)
{
    if (!glamor_set_alu(pixmap->drawable.pScreen, gc->alu))
        return FALSE;

    if (!glamor_set_planemask(gc->depth, gc->planemask))
        return FALSE;

    return glamor_set_texture(gc->tile.pixmap,
                              TRUE,
                              -gc->patOrg.x,
                              -gc->patOrg.y,
                              offset_uniform,
                              size_inv_uniform);
}

static PixmapPtr
glamor_get_stipple_pixmap(GCPtr gc)
{
    glamor_gc_private *gc_priv = glamor_get_gc_private(gc);
    ScreenPtr   screen = gc->pScreen;
    PixmapPtr   bitmap;
    PixmapPtr   pixmap;
    GCPtr       scratch_gc;
    ChangeGCVal changes[2];

    if (gc_priv->stipple)
        return gc_priv->stipple;

    bitmap = gc->stipple;
    if (!bitmap)
        goto bail;

    pixmap = glamor_create_pixmap(screen,
                                  bitmap->drawable.width,
                                  bitmap->drawable.height,
                                  8, GLAMOR_CREATE_NO_LARGE);
    if (!pixmap)
        goto bail;

    scratch_gc = GetScratchGC(8, screen);
    if (!scratch_gc)
        goto bail_pixmap;

    changes[0].val = 0xff;
    changes[1].val = 0x00;
    if (ChangeGC(NullClient, scratch_gc,
                 GCForeground|GCBackground, changes) != Success)
        goto bail_gc;
    ValidateGC(&pixmap->drawable, scratch_gc);

    (*scratch_gc->ops->CopyPlane)(&bitmap->drawable,
                                  &pixmap->drawable,
                                  scratch_gc,
                                  0, 0,
                                  bitmap->drawable.width,
                                  bitmap->drawable.height,
                                  0, 0, 0x1);

    FreeScratchGC(scratch_gc);
    gc_priv->stipple = pixmap;

    glamor_track_stipple(gc);

    return pixmap;

bail_gc:
    FreeScratchGC(scratch_gc);
bail_pixmap:
    glamor_destroy_pixmap(pixmap);
bail:
    return NULL;
}

Bool
glamor_set_stippled(PixmapPtr      pixmap,
                    GCPtr          gc,
                    GLint          fg_uniform,
                    GLint          offset_uniform,
                    GLint          size_uniform)
{
    PixmapPtr   stipple;

    stipple = glamor_get_stipple_pixmap(gc);
    if (!stipple)
        return FALSE;

    if (!glamor_set_solid(pixmap, gc, TRUE, fg_uniform))
        return FALSE;

    return glamor_set_texture(stipple,
                              FALSE,
                              -gc->patOrg.x,
                              -gc->patOrg.y,
                              offset_uniform,
                              size_uniform);
}
