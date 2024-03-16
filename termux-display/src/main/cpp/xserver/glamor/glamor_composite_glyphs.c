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
#include <stdlib.h>
#include "Xprintf.h"

#include "glamor_priv.h"
#include "glamor_transform.h"
#include "glamor_transfer.h"

#include <mipict.h>

#define DEFAULT_ATLAS_DIM       1024

static DevPrivateKeyRec        glamor_glyph_private_key;

struct glamor_glyph_private {
    int16_t     x;
    int16_t     y;
    uint32_t    serial;
};

struct glamor_glyph_atlas {
    PixmapPtr           atlas;
    PictFormatPtr       format;
    int                 x, y;
    int                 row_height;
    int                 nglyph;
    uint32_t            serial;
};

static inline struct glamor_glyph_private *glamor_get_glyph_private(PixmapPtr pixmap) {
    return dixLookupPrivate(&pixmap->devPrivates, &glamor_glyph_private_key);
}

static inline void
glamor_copy_glyph(PixmapPtr     glyph_pixmap,
                  DrawablePtr   atlas_draw,
                  int16_t x,
                  int16_t y)
{
    DrawablePtr glyph_draw = &glyph_pixmap->drawable;
    BoxRec      box = {
        .x1 = 0,
        .y1 = 0,
        .x2 = glyph_draw->width,
        .y2 = glyph_draw->height,
    };
    PixmapPtr upload_pixmap = glyph_pixmap;

    if (glyph_pixmap->drawable.bitsPerPixel != atlas_draw->bitsPerPixel) {

        /* If we're dealing with 1-bit glyphs, we copy them to a
         * temporary 8-bit pixmap and upload them from there, since
         * that's what GL can handle.
         */
        ScreenPtr       screen = atlas_draw->pScreen;
        GCPtr           scratch_gc;
        ChangeGCVal     changes[2];

        upload_pixmap = glamor_create_pixmap(screen,
                                             glyph_draw->width,
                                             glyph_draw->height,
                                             atlas_draw->depth,
                                             GLAMOR_CREATE_PIXMAP_CPU);
        if (!upload_pixmap)
            return;

        scratch_gc = GetScratchGC(upload_pixmap->drawable.depth, screen);
        if (!scratch_gc) {
            glamor_destroy_pixmap(upload_pixmap);
            return;
        }
        changes[0].val = 0xff;
        changes[1].val = 0x00;
        if (ChangeGC(NullClient, scratch_gc,
                     GCForeground|GCBackground, changes) != Success) {
            glamor_destroy_pixmap(upload_pixmap);
            FreeScratchGC(scratch_gc);
            return;
        }
        ValidateGC(&upload_pixmap->drawable, scratch_gc);

        (*scratch_gc->ops->CopyPlane)(glyph_draw,
                                      &upload_pixmap->drawable,
                                      scratch_gc,
                                      0, 0,
                                      glyph_draw->width,
                                      glyph_draw->height,
                                      0, 0, 0x1);
    }
    glamor_upload_boxes((PixmapPtr) atlas_draw,
                        &box, 1,
                        0, 0,
                        x, y,
                        upload_pixmap->devPrivate.ptr,
                        upload_pixmap->devKind);

    if (upload_pixmap != glyph_pixmap)
        glamor_destroy_pixmap(upload_pixmap);
}

static Bool
glamor_glyph_atlas_init(ScreenPtr screen, struct glamor_glyph_atlas *atlas)
{
    glamor_screen_private       *glamor_priv = glamor_get_screen_private(screen);
    PictFormatPtr               format = atlas->format;

    atlas->atlas = glamor_create_pixmap(screen, glamor_priv->glyph_atlas_dim,
                                        glamor_priv->glyph_atlas_dim, format->depth,
                                        GLAMOR_CREATE_FBO_NO_FBO);
    if (!glamor_pixmap_has_fbo(atlas->atlas)) {
        glamor_destroy_pixmap(atlas->atlas);
        atlas->atlas = NULL;
    }
    atlas->x = 0;
    atlas->y = 0;
    atlas->row_height = 0;
    atlas->serial++;
    atlas->nglyph = 0;
    return TRUE;
}

static Bool
glamor_glyph_can_add(struct glamor_glyph_atlas *atlas, int dim, DrawablePtr glyph_draw)
{
    /* Step down */
    if (atlas->x + glyph_draw->width > dim) {
        atlas->x = 0;
        atlas->y += atlas->row_height;
        atlas->row_height = 0;
    }

    /* Check for overfull */
    if (atlas->y + glyph_draw->height > dim)
        return FALSE;

    return TRUE;
}

static Bool
glamor_glyph_add(struct glamor_glyph_atlas *atlas, DrawablePtr glyph_draw)
{
    PixmapPtr                   glyph_pixmap = (PixmapPtr) glyph_draw;
    struct glamor_glyph_private *glyph_priv = glamor_get_glyph_private(glyph_pixmap);

    glamor_copy_glyph(glyph_pixmap, &atlas->atlas->drawable, atlas->x, atlas->y);

    glyph_priv->x = atlas->x;
    glyph_priv->y = atlas->y;
    glyph_priv->serial = atlas->serial;

    atlas->x += glyph_draw->width;
    if (atlas->row_height < glyph_draw->height)
        atlas->row_height = glyph_draw->height;

    atlas->nglyph++;

    return TRUE;
}

static const glamor_facet glamor_facet_composite_glyphs_130 = {
    .name = "composite_glyphs",
    .version = 130,
    .vs_vars = ("attribute vec4 primitive;\n"
                "attribute vec2 source;\n"
                "varying vec2 glyph_pos;\n"),
    .vs_exec = ("       vec2 pos = primitive.zw * vec2(gl_VertexID&1, (gl_VertexID&2)>>1);\n"
                GLAMOR_POS(gl_Position, (primitive.xy + pos))
                "       glyph_pos = (source + pos) * ATLAS_DIM_INV;\n"),
    .fs_vars = ("varying vec2 glyph_pos;\n"
                "out vec4 color0;\n"
                "out vec4 color1;\n"),
    .fs_exec = ("       vec4 mask = texture2D(atlas, glyph_pos);\n"),
    .source_name = "source",
    .locations = glamor_program_location_atlas,
};

static const glamor_facet glamor_facet_composite_glyphs_120 = {
    .name = "composite_glyphs",
    .vs_vars = ("attribute vec2 primitive;\n"
                "attribute vec2 source;\n"
                "varying vec2 glyph_pos;\n"),
    .vs_exec = ("       vec2 pos = vec2(0,0);\n"
                GLAMOR_POS(gl_Position, primitive.xy)
                "       glyph_pos = source.xy * ATLAS_DIM_INV;\n"),
    .fs_vars = ("varying vec2 glyph_pos;\n"),
    .fs_exec = ("       vec4 mask = texture2D(atlas, glyph_pos);\n"),
    .source_name = "source",
    .locations = glamor_program_location_atlas,
};

static Bool
glamor_glyphs_init_facet(ScreenPtr screen)
{
    glamor_screen_private       *glamor_priv = glamor_get_screen_private(screen);

    return asprintf(&glamor_priv->glyph_defines, "#define ATLAS_DIM_INV %20.18f\n", 1.0/glamor_priv->glyph_atlas_dim) > 0;
}

static void
glamor_glyphs_fini_facet(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    free(glamor_priv->glyph_defines);
}

static void
glamor_glyphs_flush(CARD8 op, PicturePtr src, PicturePtr dst,
                   glamor_program *prog,
                   struct glamor_glyph_atlas *atlas, int nglyph)
{
    DrawablePtr drawable = dst->pDrawable;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(drawable->pScreen);
    PixmapPtr atlas_pixmap = atlas->atlas;
    glamor_pixmap_private *atlas_priv = glamor_get_pixmap_private(atlas_pixmap);
    glamor_pixmap_fbo *atlas_fbo = glamor_pixmap_fbo_at(atlas_priv, 0);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    int box_index;
    int off_x, off_y;

    glamor_put_vbo_space(drawable->pScreen);

    glEnable(GL_SCISSOR_TEST);
    glamor_bind_texture(glamor_priv, GL_TEXTURE1, atlas_fbo, FALSE);

    for (;;) {
        if (!glamor_use_program_render(prog, op, src, dst))
            break;

        glUniform1i(prog->atlas_uniform, 1);

        glamor_pixmap_loop(pixmap_priv, box_index) {
            BoxPtr box = RegionRects(dst->pCompositeClip);
            int nbox = RegionNumRects(dst->pCompositeClip);

            glamor_set_destination_drawable(drawable, box_index, TRUE, FALSE,
                                            prog->matrix_uniform,
                                            &off_x, &off_y);

            /* Run over the clip list, drawing the glyphs
             * in each box
             */

            while (nbox--) {
                glScissor(box->x1 + off_x,
                          box->y1 + off_y,
                          box->x2 - box->x1,
                          box->y2 - box->y1);
                box++;

                if (glamor_glsl_has_ints(glamor_priv))
                    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nglyph);
                else
                    glamor_glDrawArrays_GL_QUADS(glamor_priv, nglyph);
            }
        }
        if (prog->alpha != glamor_program_alpha_ca_first)
            break;
        prog++;
    }

    glDisable(GL_SCISSOR_TEST);

    if (glamor_glsl_has_ints(glamor_priv)) {
        glVertexAttribDivisor(GLAMOR_VERTEX_SOURCE, 0);
        glVertexAttribDivisor(GLAMOR_VERTEX_POS, 0);
    }
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisable(GL_BLEND);
}

static GLshort *
glamor_glyph_start(ScreenPtr screen, int count)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    GLshort *v;
    char *vbo_offset;

    /* Set up the vertex buffers for the font and destination */

    if (glamor_glsl_has_ints(glamor_priv)) {
        v = glamor_get_vbo_space(screen, count * (6 * sizeof (GLshort)), &vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
        glVertexAttribDivisor(GLAMOR_VERTEX_POS, 1);
        glVertexAttribPointer(GLAMOR_VERTEX_POS, 4, GL_SHORT, GL_FALSE,
                              6 * sizeof (GLshort), vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
        glVertexAttribDivisor(GLAMOR_VERTEX_SOURCE, 1);
        glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_SHORT, GL_FALSE,
                              6 * sizeof (GLshort), vbo_offset + 4 * sizeof (GLshort));
    } else {
        v = glamor_get_vbo_space(screen, count * (16 * sizeof (GLshort)), &vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
        glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_SHORT, GL_FALSE,
                              4 * sizeof (GLshort), vbo_offset);

        glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
        glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_SHORT, GL_FALSE,
                              4 * sizeof (GLshort), vbo_offset + 2 * sizeof (GLshort));
    }
    return v;
}

static inline struct glamor_glyph_atlas *
glamor_atlas_for_glyph(glamor_screen_private *glamor_priv, DrawablePtr drawable)
{
    if (drawable->depth == 32)
        return glamor_priv->glyph_atlas_argb;
    else
        return glamor_priv->glyph_atlas_a;
}

void
glamor_composite_glyphs(CARD8 op,
                        PicturePtr src,
                        PicturePtr dst,
                        PictFormatPtr glyph_format,
                        INT16 x_src,
                        INT16 y_src, int nlist, GlyphListPtr list,
                        GlyphPtr *glyphs)
{
    int glyphs_queued;
    GLshort *v = NULL;
    DrawablePtr drawable = dst->pDrawable;
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_program *prog = NULL;
    glamor_program_render       *glyphs_program = &glamor_priv->glyphs_program;
    struct glamor_glyph_atlas    *glyph_atlas = NULL;
    int x = 0, y = 0;
    int n;
    int glyph_atlas_dim = glamor_priv->glyph_atlas_dim;
    int glyph_max_dim = glamor_priv->glyph_max_dim;
    int nglyph = 0;
    int screen_num = screen->myNum;

    for (n = 0; n < nlist; n++)
        nglyph += list[n].len;

    glamor_make_current(glamor_priv);

    glyphs_queued = 0;

    while (nlist--) {
        x += list->xOff;
        y += list->yOff;
        n = list->len;
        list++;
        while (n--) {
            GlyphPtr glyph = *glyphs++;

            /* Glyph not empty?
             */
            if (glyph->info.width && glyph->info.height) {
                PicturePtr glyph_pict = GlyphPicture(glyph)[screen_num];
                DrawablePtr glyph_draw = glyph_pict->pDrawable;

                /* Need to draw with slow path?
                 */
                if (_X_UNLIKELY(glyph_draw->width > glyph_max_dim ||
                                glyph_draw->height > glyph_max_dim ||
                                !glamor_pixmap_is_memory((PixmapPtr)glyph_draw)))
                {
                    if (glyphs_queued) {
                        glamor_glyphs_flush(op, src, dst, prog, glyph_atlas, glyphs_queued);
                        glyphs_queued = 0;
                    }
                bail_one:
                    glamor_composite(op, src, glyph_pict, dst,
                                     x_src + (x - glyph->info.x), (y - glyph->info.y),
                                     0, 0,
                                     x - glyph->info.x, y - glyph->info.y,
                                     glyph_draw->width, glyph_draw->height);
                } else {
                    struct glamor_glyph_private *glyph_priv = glamor_get_glyph_private((PixmapPtr)(glyph_draw));
                    struct glamor_glyph_atlas *next_atlas = glamor_atlas_for_glyph(glamor_priv, glyph_draw);

                    /* Switching source glyph format?
                     */
                    if (_X_UNLIKELY(next_atlas != glyph_atlas)) {
                        if (glyphs_queued) {
                            glamor_glyphs_flush(op, src, dst, prog, glyph_atlas, glyphs_queued);
                            glyphs_queued = 0;
                        }
                        glyph_atlas = next_atlas;
                    }

                    /* Glyph not cached in current atlas?
                     */
                    if (_X_UNLIKELY(glyph_priv->serial != glyph_atlas->serial)) {
                        if (!glamor_glyph_can_add(glyph_atlas, glyph_atlas_dim, glyph_draw)) {
                            if (glyphs_queued) {
                                glamor_glyphs_flush(op, src, dst, prog, glyph_atlas, glyphs_queued);
                                glyphs_queued = 0;
                            }
                            if (glyph_atlas->atlas) {
                                (*screen->DestroyPixmap)(glyph_atlas->atlas);
                                glyph_atlas->atlas = NULL;
                            }
                        }
                        if (!glyph_atlas->atlas) {
                            glamor_glyph_atlas_init(screen, glyph_atlas);
                            if (!glyph_atlas->atlas)
                                goto bail_one;
                        }
                        glamor_glyph_add(glyph_atlas, glyph_draw);
                    }

                    /* First glyph in the current atlas?
                     */
                    if (_X_UNLIKELY(glyphs_queued == 0)) {
                        if (glamor_glsl_has_ints(glamor_priv))
                            prog = glamor_setup_program_render(op, src, glyph_pict, dst,
                                                               glyphs_program,
                                                               &glamor_facet_composite_glyphs_130,
                                                               glamor_priv->glyph_defines);
                        else
                            prog = glamor_setup_program_render(op, src, glyph_pict, dst,
                                                               glyphs_program,
                                                               &glamor_facet_composite_glyphs_120,
                                                               glamor_priv->glyph_defines);
                        if (!prog)
                            goto bail_one;
                        v = glamor_glyph_start(screen, nglyph);
                    }

                    /* Add the glyph
                     */

                    glyphs_queued++;
                    if (_X_LIKELY(glamor_glsl_has_ints(glamor_priv))) {
                        v[0] = x - glyph->info.x;
                        v[1] = y - glyph->info.y;
                        v[2] = glyph_draw->width;
                        v[3] = glyph_draw->height;
                        v[4] = glyph_priv->x;
                        v[5] = glyph_priv->y;
                        v += 6;
                    } else {
                        v[0] = x - glyph->info.x;
                        v[1] = y - glyph->info.y;
                        v[2] = glyph_priv->x;
                        v[3] = glyph_priv->y;
                        v += 4;

                        v[0] = x - glyph->info.x + glyph_draw->width;
                        v[1] = y - glyph->info.y;
                        v[2] = glyph_priv->x + glyph_draw->width;
                        v[3] = glyph_priv->y;
                        v += 4;

                        v[0] = x - glyph->info.x + glyph_draw->width;
                        v[1] = y - glyph->info.y + glyph_draw->height;
                        v[2] = glyph_priv->x + glyph_draw->width;
                        v[3] = glyph_priv->y + glyph_draw->height;
                        v += 4;

                        v[0] = x - glyph->info.x;
                        v[1] = y - glyph->info.y + glyph_draw->height;
                        v[2] = glyph_priv->x;
                        v[3] = glyph_priv->y + glyph_draw->height;
                        v += 4;
                    }
                }
            }
            x += glyph->info.xOff;
            y += glyph->info.yOff;
            nglyph--;
        }
    }

    if (glyphs_queued)
        glamor_glyphs_flush(op, src, dst, prog, glyph_atlas, glyphs_queued);

    return;
}

static struct glamor_glyph_atlas *
glamor_alloc_glyph_atlas(ScreenPtr screen, int depth, CARD32 f)
{
    PictFormatPtr               format;
    struct glamor_glyph_atlas    *glyph_atlas;

    format = PictureMatchFormat(screen, depth, f);
    if (!format)
        return NULL;
    glyph_atlas = calloc (1, sizeof (struct glamor_glyph_atlas));
    if (!glyph_atlas)
        return NULL;
    glyph_atlas->format = format;
    glyph_atlas->serial = 1;

    return glyph_atlas;
}

Bool
glamor_composite_glyphs_init(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    if (!dixRegisterPrivateKey(&glamor_glyph_private_key, PRIVATE_PIXMAP, sizeof (struct glamor_glyph_private)))
        return FALSE;

    /* Make glyph atlases of a reasonable size, but no larger than the maximum
     * supported by the hardware
     */
    glamor_priv->glyph_atlas_dim = MIN(DEFAULT_ATLAS_DIM, glamor_priv->max_fbo_size);

    /* Don't stick huge glyphs in the atlases */
    glamor_priv->glyph_max_dim = glamor_priv->glyph_atlas_dim / 8;

    glamor_priv->glyph_atlas_a = glamor_alloc_glyph_atlas(screen, 8, PICT_a8);
    if (!glamor_priv->glyph_atlas_a)
        return FALSE;
    glamor_priv->glyph_atlas_argb = glamor_alloc_glyph_atlas(screen, 32, PICT_a8r8g8b8);
    if (!glamor_priv->glyph_atlas_argb) {
        free (glamor_priv->glyph_atlas_a);
        return FALSE;
    }
    if (!glamor_glyphs_init_facet(screen))
        return FALSE;
    return TRUE;
}

static void
glamor_free_glyph_atlas(struct glamor_glyph_atlas *atlas)
{
    if (!atlas)
        return;
    if (atlas->atlas)
        (*atlas->atlas->drawable.pScreen->DestroyPixmap)(atlas->atlas);
    free (atlas);
}

void
glamor_composite_glyphs_fini(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_glyphs_fini_facet(screen);
    glamor_free_glyph_atlas(glamor_priv->glyph_atlas_a);
    glamor_free_glyph_atlas(glamor_priv->glyph_atlas_argb);
}
