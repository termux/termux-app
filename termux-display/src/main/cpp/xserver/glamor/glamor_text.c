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
#include <dixfontstr.h>
#include "glamor_transform.h"

/*
 * Fill in the array of charinfo pointers for the provided characters. For
 * missing characters, place a NULL in the array so that the charinfo array
 * aligns exactly with chars
 */

static void
glamor_get_glyphs(FontPtr font, glamor_font_t *glamor_font,
                  int count, char *chars, Bool sixteen, CharInfoPtr *charinfo)
{
    unsigned long nglyphs;
    FontEncoding encoding;
    int char_step;
    int c;

    if (sixteen) {
        char_step = 2;
        if (FONTLASTROW(font) == 0)
            encoding = Linear16Bit;
        else
            encoding = TwoD16Bit;
    } else {
        char_step = 1;
        encoding = Linear8Bit;
    }

    /* If the font has a default character, then we shouldn't have to
     * worry about missing glyphs, so just get the whole string all at
     * once. Otherwise, we have to fetch chars one at a time to notice
     * missing ones.
     */
    if (glamor_font->default_char) {
        GetGlyphs(font, (unsigned long) count, (unsigned char *) chars,
                  encoding, &nglyphs, charinfo);

        /* Make sure it worked. There's a bug in libXfont through
         * version 1.4.7 which would cause it to fail when the font is
         * a 2D font without a first row, and the application sends a
         * 1-d request. In this case, libXfont would return zero
         * glyphs, even when the font had a default character.
         *
         * It's easy enough for us to work around that bug here by
         * simply checking the returned nglyphs and falling through to
         * the one-at-a-time code below. Not doing this check would
         * result in uninitialized memory accesses in the rendering code.
         */
        if (nglyphs == count)
            return;
    }

    for (c = 0; c < count; c++) {
        GetGlyphs(font, 1, (unsigned char *) chars,
                  encoding, &nglyphs, &charinfo[c]);
        if (!nglyphs)
            charinfo[c] = NULL;
        chars += char_step;
    }
}

/*
 * Construct quads for the provided list of characters and draw them
 */

static int
glamor_text(DrawablePtr drawable, GCPtr gc,
            glamor_font_t *glamor_font,
            glamor_program *prog,
            int x, int y,
            int count, char *s_chars, CharInfoPtr *charinfo,
            Bool sixteen)
{
    unsigned char *chars = (unsigned char *) s_chars;
    FontPtr font = gc->font;
    int off_x, off_y;
    int c;
    int nglyph;
    GLshort *v;
    char *vbo_offset;
    CharInfoPtr ci;
    int firstRow = font->info.firstRow;
    int firstCol = font->info.firstCol;
    int glyph_spacing_x = glamor_font->glyph_width_bytes * 8;
    int glyph_spacing_y = glamor_font->glyph_height;
    int box_index;
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    /* Set the font as texture 1 */

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glamor_font->texture_id);
    glUniform1i(prog->font_uniform, 1);

    /* Set up the vertex buffers for the font and destination */

    v = glamor_get_vbo_space(drawable->pScreen, count * (6 * sizeof (GLshort)), &vbo_offset);

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glVertexAttribDivisor(GLAMOR_VERTEX_POS, 1);
    glVertexAttribPointer(GLAMOR_VERTEX_POS, 4, GL_SHORT, GL_FALSE,
                          6 * sizeof (GLshort), vbo_offset);

    glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
    glVertexAttribDivisor(GLAMOR_VERTEX_SOURCE, 1);
    glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_SHORT, GL_FALSE,
                          6 * sizeof (GLshort), vbo_offset + 4 * sizeof (GLshort));

    /* Set the vertex coordinates */
    nglyph = 0;

    for (c = 0; c < count; c++) {
        if ((ci = *charinfo++)) {
            int     x1 = x + ci->metrics.leftSideBearing;
            int     y1 = y - ci->metrics.ascent;
            int     width = GLYPHWIDTHPIXELS(ci);
            int     height = GLYPHHEIGHTPIXELS(ci);
            int     tx, ty = 0;
            int     row = 0, col;
            int     second_row = 0;
            x += ci->metrics.characterWidth;

            if (sixteen) {
                if (ci == glamor_font->default_char) {
                    row = glamor_font->default_row;
                    col = glamor_font->default_col;
                } else {
                    row = chars[0];
                    col = chars[1];
                }
                if (FONTLASTROW(font) != 0) {
                    ty = ((row - firstRow) / 2) * glyph_spacing_y;
                    second_row = (row - firstRow) & 1;
                }
                else
                    col += row << 8;
            } else {
                if (ci == glamor_font->default_char)
                    col = glamor_font->default_col;
                else
                    col = chars[0];
            }

            tx = (col - firstCol) * glyph_spacing_x;
            /* adjust for second row layout */
            tx += second_row * glamor_font->row_width * 8;

            v[ 0] = x1;
            v[ 1] = y1;
            v[ 2] = width;
            v[ 3] = height;
            v[ 4] = tx;
            v[ 5] = ty;

            v += 6;
            nglyph++;
        }
        chars += 1 + sixteen;
    }
    glamor_put_vbo_space(drawable->pScreen);

    if (nglyph != 0) {

        glEnable(GL_SCISSOR_TEST);

        glamor_pixmap_loop(pixmap_priv, box_index) {
            BoxPtr box = RegionRects(gc->pCompositeClip);
            int nbox = RegionNumRects(gc->pCompositeClip);

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
                glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nglyph);
            }
        }
        glDisable(GL_SCISSOR_TEST);
    }

    glVertexAttribDivisor(GLAMOR_VERTEX_SOURCE, 0);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
    glVertexAttribDivisor(GLAMOR_VERTEX_POS, 0);
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);

    return x;
}

static const char vs_vars_text[] =
    "attribute vec4 primitive;\n"
    "attribute vec2 source;\n"
    "varying vec2 glyph_pos;\n";

static const char vs_exec_text[] =
    "       vec2 pos = primitive.zw * vec2(gl_VertexID&1, (gl_VertexID&2)>>1);\n"
    GLAMOR_POS(gl_Position, (primitive.xy + pos))
    "       glyph_pos = source + pos;\n";

static const char fs_vars_text[] =
    "varying vec2 glyph_pos;\n";

static const char fs_exec_text[] =
    "       ivec2 itile_texture = ivec2(glyph_pos);\n"
    "       uint x = uint(itile_texture.x & 7);\n"
    "       itile_texture.x >>= 3;\n"
    "       uint texel = texelFetch(font, itile_texture, 0).x;\n"
    "       uint bit = (texel >> x) & uint(1);\n"
    "       if (bit == uint(0))\n"
    "               discard;\n";

static const char fs_exec_te[] =
    "       ivec2 itile_texture = ivec2(glyph_pos);\n"
    "       uint x = uint(itile_texture.x & 7);\n"
    "       itile_texture.x >>= 3;\n"
    "       uint texel = texelFetch(font, itile_texture, 0).x;\n"
    "       uint bit = (texel >> x) & uint(1);\n"
    "       if (bit == uint(0))\n"
    "               gl_FragColor = bg;\n"
    "       else\n"
    "               gl_FragColor = fg;\n";

static const glamor_facet glamor_facet_poly_text = {
    .name = "poly_text",
    .version = 130,
    .vs_vars = vs_vars_text,
    .vs_exec = vs_exec_text,
    .fs_vars = fs_vars_text,
    .fs_exec = fs_exec_text,
    .source_name = "source",
    .locations = glamor_program_location_font,
};

static Bool
glamor_poly_text(DrawablePtr drawable, GCPtr gc,
                 int x, int y, int count, char *chars, Bool sixteen, int *final_pos)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_program *prog;
    glamor_pixmap_private *pixmap_priv;
    glamor_font_t *glamor_font;
    CharInfoPtr charinfo[255];  /* encoding only has 1 byte for count */

    glamor_font = glamor_font_get(drawable->pScreen, gc->font);
    if (!glamor_font)
        goto bail;

    glamor_get_glyphs(gc->font, glamor_font, count, chars, sixteen, charinfo);

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        goto bail;

    glamor_make_current(glamor_priv);

    prog = glamor_use_program_fill(pixmap, gc, &glamor_priv->poly_text_progs, &glamor_facet_poly_text);

    if (!prog)
        goto bail;

    x = glamor_text(drawable, gc, glamor_font, prog,
                    x, y, count, chars, charinfo, sixteen);

    *final_pos = x;
    return TRUE;

bail:
    return FALSE;
}

int
glamor_poly_text8(DrawablePtr drawable, GCPtr gc,
                   int x, int y, int count, char *chars)
{
    int final_pos;

    if (glamor_poly_text(drawable, gc, x, y, count, chars, FALSE, &final_pos))
        return final_pos;
    return miPolyText8(drawable, gc, x, y, count, chars);
}

int
glamor_poly_text16(DrawablePtr drawable, GCPtr gc,
                    int x, int y, int count, unsigned short *chars)
{
    int final_pos;

    if (glamor_poly_text(drawable, gc, x, y, count, (char *) chars, TRUE, &final_pos))
        return final_pos;
    return miPolyText16(drawable, gc, x, y, count, chars);
}

/*
 * Draw image text, which is always solid in copy mode and has the
 * background cleared while painting the text. For fonts which have
 * their bitmap metrics exactly equal to the area to clear, we can use
 * the accelerated version which paints both fg and bg at the same
 * time. Otherwise, clear the whole area and then paint the glyphs on
 * top
 */

static const glamor_facet glamor_facet_image_text = {
    .name = "image_text",
    .version = 130,
    .vs_vars = vs_vars_text,
    .vs_exec = vs_exec_text,
    .fs_vars = fs_vars_text,
    .fs_exec = fs_exec_text,
    .source_name = "source",
    .locations = glamor_program_location_font,
};

static Bool
use_image_solid(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    return glamor_set_solid(pixmap, gc, FALSE, prog->fg_uniform);
}

static const glamor_facet glamor_facet_image_fill = {
    .name = "solid",
    .fs_exec = "       gl_FragColor = fg;\n",
    .locations = glamor_program_location_fg,
    .use = use_image_solid,
};

static Bool
glamor_te_text_use(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    if (!glamor_set_solid(pixmap, gc, FALSE, prog->fg_uniform))
        return FALSE;
    glamor_set_color(pixmap, gc->bgPixel, prog->bg_uniform);
    return TRUE;
}

static const glamor_facet glamor_facet_te_text = {
    .name = "te_text",
    .version = 130,
    .vs_vars = vs_vars_text,
    .vs_exec = vs_exec_text,
    .fs_vars = fs_vars_text,
    .fs_exec = fs_exec_te,
    .locations = glamor_program_location_fg | glamor_program_location_bg | glamor_program_location_font,
    .source_name = "source",
    .use = glamor_te_text_use,
};

static Bool
glamor_image_text(DrawablePtr drawable, GCPtr gc,
                  int x, int y, int count, char *chars,
                  Bool sixteen)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_program *prog;
    glamor_pixmap_private *pixmap_priv;
    glamor_font_t *glamor_font;
    const glamor_facet *prim_facet;
    const glamor_facet *fill_facet;
    CharInfoPtr charinfo[255];  /* encoding only has 1 byte for count */

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        return FALSE;

    glamor_font = glamor_font_get(drawable->pScreen, gc->font);
    if (!glamor_font)
        return FALSE;

    glamor_get_glyphs(gc->font, glamor_font, count, chars, sixteen, charinfo);

    glamor_make_current(glamor_priv);

    if (TERMINALFONT(gc->font))
        prog = &glamor_priv->te_text_prog;
    else
        prog = &glamor_priv->image_text_prog;

    if (prog->failed)
        goto bail;

    if (!prog->prog) {
        if (TERMINALFONT(gc->font)) {
            prim_facet = &glamor_facet_te_text;
            fill_facet = NULL;
        } else {
            prim_facet = &glamor_facet_image_text;
            fill_facet = &glamor_facet_image_fill;
        }

        if (!glamor_build_program(screen, prog, prim_facet, fill_facet, NULL, NULL))
            goto bail;
    }

    if (!TERMINALFONT(gc->font)) {
        int width = 0;
        int c;
        RegionRec region;
        BoxRec box;
        int off_x, off_y;

        /* Check planemask before drawing background to
         * bail early if it's not OK
         */
        if (!glamor_set_planemask(gc->depth, gc->planemask))
            goto bail;
        for (c = 0; c < count; c++)
            if (charinfo[c])
                width += charinfo[c]->metrics.characterWidth;

        glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);

        if (width >= 0) {
            box.x1 = drawable->x + x;
            box.x2 = drawable->x + x + width;
        } else {
            box.x1 = drawable->x + x + width;
            box.x2 = drawable->x + x;
        }
        box.y1 = drawable->y + y - gc->font->info.fontAscent;
        box.y2 = drawable->y + y + gc->font->info.fontDescent;
        RegionInit(&region, &box, 1);
        RegionIntersect(&region, &region, gc->pCompositeClip);
        RegionTranslate(&region, off_x, off_y);
        glamor_solid_boxes(pixmap, RegionRects(&region), RegionNumRects(&region), gc->bgPixel);
        RegionUninit(&region);
    }

    if (!glamor_use_program(pixmap, gc, prog, NULL))
        goto bail;

    (void) glamor_text(drawable, gc, glamor_font, prog,
                       x, y, count, chars, charinfo, sixteen);

    return TRUE;

bail:
    return FALSE;
}

void
glamor_image_text8(DrawablePtr drawable, GCPtr gc,
                   int x, int y, int count, char *chars)
{
    if (!glamor_image_text(drawable, gc, x, y, count, chars, FALSE))
        miImageText8(drawable, gc, x, y, count, chars);
}

void
glamor_image_text16(DrawablePtr drawable, GCPtr gc,
                    int x, int y, int count, unsigned short *chars)
{
    if (!glamor_image_text(drawable, gc, x, y, count, (char *) chars, TRUE))
        miImageText16(drawable, gc, x, y, count, chars);
}
