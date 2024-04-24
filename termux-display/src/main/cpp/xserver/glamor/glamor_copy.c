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
#include "glamor_prepare.h"
#include "glamor_transform.h"

struct copy_args {
    PixmapPtr           src_pixmap;
    glamor_pixmap_fbo   *src;
    uint32_t            bitplane;
    int                 dx, dy;
};

static Bool
use_copyarea(PixmapPtr dst, GCPtr gc, glamor_program *prog, void *arg)
{
    struct copy_args *args = arg;
    glamor_pixmap_fbo *src = args->src;

    glamor_bind_texture(glamor_get_screen_private(dst->drawable.pScreen),
                        GL_TEXTURE0, src, TRUE);

    glUniform2f(prog->fill_offset_uniform, args->dx, args->dy);
    glUniform2f(prog->fill_size_inv_uniform, 1.0f/src->width, 1.0f/src->height);

    return TRUE;
}

static const glamor_facet glamor_facet_copyarea = {
    "copy_area",
    .vs_vars = "attribute vec2 primitive;\n",
    .vs_exec = (GLAMOR_POS(gl_Position, primitive.xy)
                "       fill_pos = (fill_offset + primitive.xy) * fill_size_inv;\n"),
    .fs_exec = "       gl_FragColor = texture2D(sampler, fill_pos);\n",
    .locations = glamor_program_location_fillsamp | glamor_program_location_fillpos,
    .use = use_copyarea,
};

/*
 * Configure the copy plane program for the current operation
 */

static Bool
use_copyplane(PixmapPtr dst, GCPtr gc, glamor_program *prog, void *arg)
{
    struct copy_args *args = arg;
    glamor_pixmap_fbo *src = args->src;

    glamor_bind_texture(glamor_get_screen_private(dst->drawable.pScreen),
                        GL_TEXTURE0, src, TRUE);

    glUniform2f(prog->fill_offset_uniform, args->dx, args->dy);
    glUniform2f(prog->fill_size_inv_uniform, 1.0f/src->width, 1.0f/src->height);

    glamor_set_color(dst, gc->fgPixel, prog->fg_uniform);
    glamor_set_color(dst, gc->bgPixel, prog->bg_uniform);

    /* XXX handle 2 10 10 10 and 1555 formats; presumably the pixmap private knows this? */
    switch (args->src_pixmap->drawable.depth) {
    case 30:
        glUniform4ui(prog->bitplane_uniform,
                     (args->bitplane >> 20) & 0x3ff,
                     (args->bitplane >> 10) & 0x3ff,
                     (args->bitplane      ) & 0x3ff,
                     0);

        glUniform4f(prog->bitmul_uniform, 0x3ff, 0x3ff, 0x3ff, 0);
        break;
    case 24:
        glUniform4ui(prog->bitplane_uniform,
                     (args->bitplane >> 16) & 0xff,
                     (args->bitplane >>  8) & 0xff,
                     (args->bitplane      ) & 0xff,
                     0);

        glUniform4f(prog->bitmul_uniform, 0xff, 0xff, 0xff, 0);
        break;
    case 32:
        glUniform4ui(prog->bitplane_uniform,
                     (args->bitplane >> 16) & 0xff,
                     (args->bitplane >>  8) & 0xff,
                     (args->bitplane      ) & 0xff,
                     (args->bitplane >> 24) & 0xff);

        glUniform4f(prog->bitmul_uniform, 0xff, 0xff, 0xff, 0xff);
        break;
    case 16:
        glUniform4ui(prog->bitplane_uniform,
                     (args->bitplane >> 11) & 0x1f,
                     (args->bitplane >>  5) & 0x3f,
                     (args->bitplane      ) & 0x1f,
                     0);

        glUniform4f(prog->bitmul_uniform, 0x1f, 0x3f, 0x1f, 0);
        break;
    case 15:
        glUniform4ui(prog->bitplane_uniform,
                     (args->bitplane >> 10) & 0x1f,
                     (args->bitplane >>  5) & 0x1f,
                     (args->bitplane      ) & 0x1f,
                     0);

        glUniform4f(prog->bitmul_uniform, 0x1f, 0x1f, 0x1f, 0);
        break;
    case 8:
        glUniform4ui(prog->bitplane_uniform,
                     0, 0, 0, args->bitplane);
        glUniform4f(prog->bitmul_uniform, 0, 0, 0, 0xff);
        break;
    case 1:
        glUniform4ui(prog->bitplane_uniform,
                     0, 0, 0, args->bitplane);
        glUniform4f(prog->bitmul_uniform, 0, 0, 0, 0xff);
        break;
    }

    return TRUE;
}

static const glamor_facet glamor_facet_copyplane = {
    "copy_plane",
    .version = 130,
    .vs_vars = "attribute vec2 primitive;\n",
    .vs_exec = (GLAMOR_POS(gl_Position, (primitive.xy))
                "       fill_pos = (fill_offset + primitive.xy) * fill_size_inv;\n"),
    .fs_exec = ("       uvec4 bits = uvec4(round(texture2D(sampler, fill_pos) * bitmul));\n"
                "       if ((bits & bitplane) != uvec4(0,0,0,0))\n"
                "               gl_FragColor = fg;\n"
                "       else\n"
                "               gl_FragColor = bg;\n"),
    .locations = glamor_program_location_fillsamp|glamor_program_location_fillpos|glamor_program_location_fg|glamor_program_location_bg|glamor_program_location_bitplane,
    .use = use_copyplane,
};

/*
 * When all else fails, pull the bits out of the GPU and do the
 * operation with fb
 */

static void
glamor_copy_bail(DrawablePtr src,
                 DrawablePtr dst,
                 GCPtr gc,
                 BoxPtr box,
                 int nbox,
                 int dx,
                 int dy,
                 Bool reverse,
                 Bool upsidedown,
                 Pixel bitplane,
                 void *closure)
{
    if (glamor_prepare_access(dst, GLAMOR_ACCESS_RW) && glamor_prepare_access(src, GLAMOR_ACCESS_RO)) {
        if (bitplane) {
            if (src->bitsPerPixel > 1)
                fbCopyNto1(src, dst, gc, box, nbox, dx, dy,
                           reverse, upsidedown, bitplane, closure);
            else
                fbCopy1toN(src, dst, gc, box, nbox, dx, dy,
                           reverse, upsidedown, bitplane, closure);
        } else {
            fbCopyNtoN(src, dst, gc, box, nbox, dx, dy,
                       reverse, upsidedown, bitplane, closure);
        }
    }
    glamor_finish_access(dst);
    glamor_finish_access(src);
}

/**
 * Implements CopyPlane and CopyArea from the CPU to the GPU by using
 * the source as a texture and painting that into the destination.
 *
 * This requires that source and dest are different textures, or that
 * (if the copy area doesn't overlap), GL_NV_texture_barrier is used
 * to ensure that the caches are flushed at the right times.
 */
static Bool
glamor_copy_cpu_fbo(DrawablePtr src,
                    DrawablePtr dst,
                    GCPtr gc,
                    BoxPtr box,
                    int nbox,
                    int dx,
                    int dy,
                    Bool reverse,
                    Bool upsidedown,
                    Pixel bitplane,
                    void *closure)
{
    ScreenPtr screen = dst->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    int dst_xoff, dst_yoff;

    if (gc && gc->alu != GXcopy)
        goto bail;

    if (gc && !glamor_pm_is_solid(gc->depth, gc->planemask))
        goto bail;

    glamor_make_current(glamor_priv);

    if (!glamor_prepare_access(src, GLAMOR_ACCESS_RO))
        goto bail;

    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_xoff, &dst_yoff);

    if (bitplane) {
        FbBits *tmp_bits;
        FbStride tmp_stride;
        int tmp_bpp;
        int tmp_xoff, tmp_yoff;

        PixmapPtr tmp_pix = fbCreatePixmap(screen, dst_pixmap->drawable.width,
                                           dst_pixmap->drawable.height,
                                           dst->depth, 0);

        if (!tmp_pix) {
            glamor_finish_access(src);
            goto bail;
        }

        tmp_pix->drawable.x = dst_xoff;
        tmp_pix->drawable.y = dst_yoff;

        fbGetDrawable(&tmp_pix->drawable, tmp_bits, tmp_stride, tmp_bpp, tmp_xoff,
                      tmp_yoff);

        if (src->bitsPerPixel > 1)
            fbCopyNto1(src, &tmp_pix->drawable, gc, box, nbox, dx, dy,
                       reverse, upsidedown, bitplane, closure);
        else
            fbCopy1toN(src, &tmp_pix->drawable, gc, box, nbox, dx, dy,
                       reverse, upsidedown, bitplane, closure);

        glamor_upload_boxes(dst_pixmap, box, nbox, tmp_xoff, tmp_yoff,
                            dst_xoff, dst_yoff, (uint8_t *) tmp_bits,
                            tmp_stride * sizeof(FbBits));
        fbDestroyPixmap(tmp_pix);
    } else {
        FbBits *src_bits;
        FbStride src_stride;
        int src_bpp;
        int src_xoff, src_yoff;

        fbGetDrawable(src, src_bits, src_stride, src_bpp, src_xoff, src_yoff);
        glamor_upload_boxes(dst_pixmap, box, nbox, src_xoff + dx, src_yoff + dy,
                            dst_xoff, dst_yoff,
                            (uint8_t *) src_bits, src_stride * sizeof (FbBits));
    }
    glamor_finish_access(src);

    return TRUE;

bail:
    return FALSE;
}

/**
 * Implements CopyArea from the GPU to the CPU using glReadPixels from the
 * source FBO.
 */
static Bool
glamor_copy_fbo_cpu(DrawablePtr src,
                    DrawablePtr dst,
                    GCPtr gc,
                    BoxPtr box,
                    int nbox,
                    int dx,
                    int dy,
                    Bool reverse,
                    Bool upsidedown,
                    Pixel bitplane,
                    void *closure)
{
    ScreenPtr screen = dst->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    FbBits *dst_bits;
    FbStride dst_stride;
    int dst_bpp;
    int src_xoff, src_yoff;
    int dst_xoff, dst_yoff;

    if (gc && gc->alu != GXcopy)
        goto bail;

    if (gc && !glamor_pm_is_solid(gc->depth, gc->planemask))
        goto bail;

    glamor_make_current(glamor_priv);

    if (!glamor_prepare_access(dst, GLAMOR_ACCESS_RW))
        goto bail;

    glamor_get_drawable_deltas(src, src_pixmap, &src_xoff, &src_yoff);

    fbGetDrawable(dst, dst_bits, dst_stride, dst_bpp, dst_xoff, dst_yoff);

    glamor_download_boxes(src_pixmap, box, nbox, src_xoff + dx, src_yoff + dy,
                          dst_xoff, dst_yoff,
                          (uint8_t *) dst_bits, dst_stride * sizeof (FbBits));
    glamor_finish_access(dst);

    return TRUE;

bail:
    return FALSE;
}

/* Include the enums here for the moment, to keep from needing to bump epoxy. */
#ifndef GL_TILE_RASTER_ORDER_FIXED_MESA
#define GL_TILE_RASTER_ORDER_FIXED_MESA          0x8BB8
#define GL_TILE_RASTER_ORDER_INCREASING_X_MESA   0x8BB9
#define GL_TILE_RASTER_ORDER_INCREASING_Y_MESA   0x8BBA
#endif

/*
 * Copy from GPU to GPU by using the source
 * as a texture and painting that into the destination
 */

static Bool
glamor_copy_fbo_fbo_draw(DrawablePtr src,
                         DrawablePtr dst,
                         GCPtr gc,
                         BoxPtr box,
                         int nbox,
                         int dx,
                         int dy,
                         Bool reverse,
                         Bool upsidedown,
                         Pixel bitplane,
                         void *closure)
{
    ScreenPtr screen = dst->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    glamor_pixmap_private *src_priv = glamor_get_pixmap_private(src_pixmap);
    glamor_pixmap_private *dst_priv = glamor_get_pixmap_private(dst_pixmap);
    int src_box_index, dst_box_index;
    int dst_off_x, dst_off_y;
    int src_off_x, src_off_y;
    GLshort *v;
    char *vbo_offset;
    struct copy_args args;
    glamor_program *prog;
    const glamor_facet *copy_facet;
    int n;
    Bool ret = FALSE;
    BoxRec bounds = glamor_no_rendering_bounds();

    glamor_make_current(glamor_priv);

    if (gc && !glamor_set_planemask(gc->depth, gc->planemask))
        goto bail_ctx;

    if (!glamor_set_alu(screen, gc ? gc->alu : GXcopy))
        goto bail_ctx;

    if (bitplane && !glamor_priv->can_copyplane)
        goto bail_ctx;

    if (bitplane) {
        prog = &glamor_priv->copy_plane_prog;
        copy_facet = &glamor_facet_copyplane;
    } else {
        prog = &glamor_priv->copy_area_prog;
        copy_facet = &glamor_facet_copyarea;
    }

    if (prog->failed)
        goto bail_ctx;

    if (!prog->prog) {
        if (!glamor_build_program(screen, prog,
                                  copy_facet, NULL, NULL, NULL))
            goto bail_ctx;
    }

    args.src_pixmap = src_pixmap;
    args.bitplane = bitplane;

    /* Set up the vertex buffers for the points */

    v = glamor_get_vbo_space(dst->pScreen, nbox * 8 * sizeof (int16_t), &vbo_offset);

    if (src_pixmap == dst_pixmap && glamor_priv->has_mesa_tile_raster_order) {
        glEnable(GL_TILE_RASTER_ORDER_FIXED_MESA);
        if (dx >= 0)
            glEnable(GL_TILE_RASTER_ORDER_INCREASING_X_MESA);
        else
            glDisable(GL_TILE_RASTER_ORDER_INCREASING_X_MESA);
        if (dy >= 0)
            glEnable(GL_TILE_RASTER_ORDER_INCREASING_Y_MESA);
        else
            glDisable(GL_TILE_RASTER_ORDER_INCREASING_Y_MESA);
    }

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT, GL_FALSE,
                          2 * sizeof (GLshort), vbo_offset);

    if (nbox < 100) {
        bounds = glamor_start_rendering_bounds();
        for (int i = 0; i < nbox; i++)
            glamor_bounds_union_box(&bounds, &box[i]);
    }

    for (n = 0; n < nbox; n++) {
        v[0] = box->x1; v[1] = box->y1;
        v[2] = box->x1; v[3] = box->y2;
        v[4] = box->x2; v[5] = box->y2;
        v[6] = box->x2; v[7] = box->y1;

        v += 8;
        box++;
    }

    glamor_put_vbo_space(screen);

    glamor_get_drawable_deltas(src, src_pixmap, &src_off_x, &src_off_y);

    glEnable(GL_SCISSOR_TEST);

    glamor_pixmap_loop(src_priv, src_box_index) {
        BoxPtr src_box = glamor_pixmap_box_at(src_priv, src_box_index);

        args.dx = dx + src_off_x - src_box->x1;
        args.dy = dy + src_off_y - src_box->y1;
        args.src = glamor_pixmap_fbo_at(src_priv, src_box_index);

        if (!glamor_use_program(dst_pixmap, gc, prog, &args))
            goto bail_ctx;

        glamor_pixmap_loop(dst_priv, dst_box_index) {
            BoxRec scissor = {
                .x1 = max(-args.dx, bounds.x1),
                .y1 = max(-args.dy, bounds.y1),
                .x2 = min(-args.dx + src_box->x2 - src_box->x1, bounds.x2),
                .y2 = min(-args.dy + src_box->y2 - src_box->y1, bounds.y2),
            };
            if (scissor.x1 >= scissor.x2 || scissor.y1 >= scissor.y2)
                continue;

            if (!glamor_set_destination_drawable(dst, dst_box_index, FALSE, FALSE,
                                                 prog->matrix_uniform,
                                                 &dst_off_x, &dst_off_y))
                goto bail_ctx;

            glScissor(scissor.x1 + dst_off_x,
                      scissor.y1 + dst_off_y,
                      scissor.x2 - scissor.x1,
                      scissor.y2 - scissor.y1);

            glamor_glDrawArrays_GL_QUADS(glamor_priv, nbox);
        }
    }

    ret = TRUE;

bail_ctx:
    if (src_pixmap == dst_pixmap && glamor_priv->has_mesa_tile_raster_order) {
        glDisable(GL_TILE_RASTER_ORDER_FIXED_MESA);
    }
    glDisable(GL_SCISSOR_TEST);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return ret;
}

/**
 * Copies from the GPU to the GPU using a temporary pixmap in between,
 * to correctly handle overlapping copies.
 */

static Bool
glamor_copy_fbo_fbo_temp(DrawablePtr src,
                         DrawablePtr dst,
                         GCPtr gc,
                         BoxPtr box,
                         int nbox,
                         int dx,
                         int dy,
                         Bool reverse,
                         Bool upsidedown,
                         Pixel bitplane,
                         void *closure)
{
    ScreenPtr screen = dst->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr tmp_pixmap;
    BoxRec bounds;
    int n;
    BoxPtr tmp_box;

    if (nbox == 0)
        return TRUE;

    /* Sanity check state to avoid getting halfway through and bailing
     * at the last second. Might be nice to have checks that didn't
     * involve setting state.
     */
    glamor_make_current(glamor_priv);

    if (gc && !glamor_set_planemask(gc->depth, gc->planemask))
        goto bail_ctx;

    if (!glamor_set_alu(screen, gc ? gc->alu : GXcopy))
        goto bail_ctx;

    /* Find the size of the area to copy
     */
    bounds = box[0];
    for (n = 1; n < nbox; n++) {
        bounds.x1 = min(bounds.x1, box[n].x1);
        bounds.x2 = max(bounds.x2, box[n].x2);
        bounds.y1 = min(bounds.y1, box[n].y1);
        bounds.y2 = max(bounds.y2, box[n].y2);
    }

    /* Allocate a suitable temporary pixmap
     */
    tmp_pixmap = glamor_create_pixmap(screen,
                                      bounds.x2 - bounds.x1,
                                      bounds.y2 - bounds.y1,
                                      src->depth, 0);
    if (!tmp_pixmap)
        goto bail;

    tmp_box = calloc(nbox, sizeof (BoxRec));
    if (!tmp_box)
        goto bail_pixmap;

    /* Convert destination boxes into tmp pixmap boxes
     */
    for (n = 0; n < nbox; n++) {
        tmp_box[n].x1 = box[n].x1 - bounds.x1;
        tmp_box[n].x2 = box[n].x2 - bounds.x1;
        tmp_box[n].y1 = box[n].y1 - bounds.y1;
        tmp_box[n].y2 = box[n].y2 - bounds.y1;
    }

    if (!glamor_copy_fbo_fbo_draw(src,
                                  &tmp_pixmap->drawable,
                                  NULL,
                                  tmp_box,
                                  nbox,
                                  dx + bounds.x1,
                                  dy + bounds.y1,
                                  FALSE, FALSE,
                                  0, NULL))
        goto bail_box;

    if (!glamor_copy_fbo_fbo_draw(&tmp_pixmap->drawable,
                                  dst,
                                  gc,
                                  box,
                                  nbox,
                                  -bounds.x1,
                                  -bounds.y1,
                                  FALSE, FALSE,
                                  bitplane, closure))
        goto bail_box;

    free(tmp_box);

    glamor_destroy_pixmap(tmp_pixmap);

    return TRUE;
bail_box:
    free(tmp_box);
bail_pixmap:
    glamor_destroy_pixmap(tmp_pixmap);
bail:
    return FALSE;

bail_ctx:
    return FALSE;
}

/**
 * Returns TRUE if the copy has to be implemented with
 * glamor_copy_fbo_fbo_temp() instead of glamor_copy_fbo_fbo().
 *
 * If the src and dst are in the same pixmap, then glamor_copy_fbo_fbo()'s
 * sampling would give undefined results (since the same texture would be
 * bound as an FBO destination and as a texture source).  However, if we
 * have GL_NV_texture_barrier, we can take advantage of the exception it
 * added:
 *
 *    "- If a texel has been written, then in order to safely read the result
 *       a texel fetch must be in a subsequent Draw separated by the command
 *
 *       void TextureBarrierNV(void);
 *
 *    TextureBarrierNV() will guarantee that writes have completed and caches
 *    have been invalidated before subsequent Draws are executed."
 */
static Bool
glamor_copy_needs_temp(DrawablePtr src,
                       DrawablePtr dst,
                       BoxPtr box,
                       int nbox,
                       int dx,
                       int dy)
{
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    ScreenPtr screen = dst->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    int n;
    int dst_off_x, dst_off_y;
    int src_off_x, src_off_y;
    BoxRec bounds;

    if (src_pixmap != dst_pixmap)
        return FALSE;

    if (nbox == 0)
        return FALSE;

    if (!glamor_priv->has_nv_texture_barrier)
        return TRUE;

    if (!glamor_priv->has_mesa_tile_raster_order) {
        glamor_get_drawable_deltas(src, src_pixmap, &src_off_x, &src_off_y);
        glamor_get_drawable_deltas(dst, dst_pixmap, &dst_off_x, &dst_off_y);

        bounds = box[0];
        for (n = 1; n < nbox; n++) {
            bounds.x1 = min(bounds.x1, box[n].x1);
            bounds.y1 = min(bounds.y1, box[n].y1);

            bounds.x2 = max(bounds.x2, box[n].x2);
            bounds.y2 = max(bounds.y2, box[n].y2);
        }

        /* Check to see if the pixmap-relative boxes overlap in both X and Y,
         * in which case we can't rely on NV_texture_barrier and must
         * make a temporary copy
         *
         *  dst.x1                     < src.x2 &&
         *  src.x1                     < dst.x2 &&
         *
         *  dst.y1                     < src.y2 &&
         *  src.y1                     < dst.y2
         */
        if (bounds.x1 + dst_off_x      < bounds.x2 + dx + src_off_x &&
            bounds.x1 + dx + src_off_x < bounds.x2 + dst_off_x &&

            bounds.y1 + dst_off_y      < bounds.y2 + dy + src_off_y &&
            bounds.y1 + dy + src_off_y < bounds.y2 + dst_off_y) {
            return TRUE;
        }
    }

    glTextureBarrierNV();

    return FALSE;
}

static Bool
glamor_copy_gl(DrawablePtr src,
               DrawablePtr dst,
               GCPtr gc,
               BoxPtr box,
               int nbox,
               int dx,
               int dy,
               Bool reverse,
               Bool upsidedown,
               Pixel bitplane,
               void *closure)
{
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    glamor_pixmap_private *src_priv = glamor_get_pixmap_private(src_pixmap);
    glamor_pixmap_private *dst_priv = glamor_get_pixmap_private(dst_pixmap);

    if (GLAMOR_PIXMAP_PRIV_HAS_FBO(dst_priv)) {
        if (GLAMOR_PIXMAP_PRIV_HAS_FBO(src_priv)) {
            if (glamor_copy_needs_temp(src, dst, box, nbox, dx, dy))
                return glamor_copy_fbo_fbo_temp(src, dst, gc, box, nbox, dx, dy,
                                                reverse, upsidedown, bitplane, closure);
            else
                return glamor_copy_fbo_fbo_draw(src, dst, gc, box, nbox, dx, dy,
                                                reverse, upsidedown, bitplane, closure);
        }

        return glamor_copy_cpu_fbo(src, dst, gc, box, nbox, dx, dy,
                                   reverse, upsidedown, bitplane, closure);
    } else if (GLAMOR_PIXMAP_PRIV_HAS_FBO(src_priv) &&
               dst_priv->type != GLAMOR_DRM_ONLY &&
               bitplane == 0) {
            return glamor_copy_fbo_cpu(src, dst, gc, box, nbox, dx, dy,
                                       reverse, upsidedown, bitplane, closure);
    }
    return FALSE;
}

void
glamor_copy(DrawablePtr src,
            DrawablePtr dst,
            GCPtr gc,
            BoxPtr box,
            int nbox,
            int dx,
            int dy,
            Bool reverse,
            Bool upsidedown,
            Pixel bitplane,
            void *closure)
{
    if (nbox == 0)
	return;

    if (glamor_copy_gl(src, dst, gc, box, nbox, dx, dy, reverse, upsidedown, bitplane, closure))
        return;
    glamor_copy_bail(src, dst, gc, box, nbox, dx, dy, reverse, upsidedown, bitplane, closure);
}

RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
                 int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    return miDoCopy(src, dst, gc,
                    srcx, srcy, width, height,
                    dstx, dsty, glamor_copy, 0, NULL);
}

RegionPtr
glamor_copy_plane(DrawablePtr src, DrawablePtr dst, GCPtr gc,
                  int srcx, int srcy, int width, int height, int dstx, int dsty,
                  unsigned long bitplane)
{
    if ((bitplane & FbFullMask(src->depth)) == 0)
        return miHandleExposures(src, dst, gc,
                                 srcx, srcy, width, height, dstx, dsty);
    return miDoCopy(src, dst, gc,
                    srcx, srcy, width, height,
                    dstx, dsty, glamor_copy, bitplane, NULL);
}

void
glamor_copy_window(WindowPtr window, DDXPointRec old_origin, RegionPtr src_region)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(&window->drawable);
    DrawablePtr drawable = &pixmap->drawable;
    RegionRec dst_region;
    int dx, dy;

    dx = old_origin.x - window->drawable.x;
    dy = old_origin.y - window->drawable.y;
    RegionTranslate(src_region, -dx, -dy);

    RegionNull(&dst_region);

    RegionIntersect(&dst_region, &window->borderClip, src_region);

#ifdef COMPOSITE
    if (pixmap->screen_x || pixmap->screen_y)
        RegionTranslate(&dst_region, -pixmap->screen_x, -pixmap->screen_y);
#endif

    miCopyRegion(drawable, drawable,
                 0, &dst_region, dx, dy, glamor_copy, 0, 0);

    RegionUninit(&dst_region);
}
