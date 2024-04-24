/*
 * Copyright © 2001 Keith Packard
 * Copyright © 2008 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

/** @file glamor_core.c
 *
 * This file covers core X rendering in glamor.
 */

#include <stdlib.h>

#include "glamor_priv.h"

Bool
glamor_get_drawable_location(const DrawablePtr drawable)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (pixmap_priv->gl_fbo == GLAMOR_FBO_UNATTACHED)
        return 'm';
    else
        return 'f';
}

GLint
glamor_compile_glsl_prog(GLenum type, const char *source)
{
    GLint ok;
    GLint prog;

    prog = glCreateShader(type);
    glShaderSource(prog, 1, (const GLchar **) &source, NULL);
    glCompileShader(prog);
    glGetShaderiv(prog, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar *info;
        GLint size;

        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &size);
        info = malloc(size);
        if (info) {
            glGetShaderInfoLog(prog, size, NULL, info);
            ErrorF("Failed to compile %s: %s\n",
                   type == GL_FRAGMENT_SHADER ? "FS" : "VS", info);
            ErrorF("Program source:\n%s", source);
            free(info);
        }
        else
            ErrorF("Failed to get shader compilation info.\n");
        FatalError("GLSL compile failure\n");
    }

    return prog;
}

void
glamor_link_glsl_prog(ScreenPtr screen, GLint prog, const char *format, ...)
{
    GLint ok;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    if (glamor_priv->has_khr_debug) {
        char *label;
        va_list va;

        va_start(va, format);
        XNFvasprintf(&label, format, va);
        glObjectLabel(GL_PROGRAM, prog, -1, label);
        free(label);
        va_end(va);
    }

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLchar *info;
        GLint size;

        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &size);
        info = malloc(size);

        glGetProgramInfoLog(prog, size, NULL, info);
        ErrorF("Failed to link: %s\n", info);
        FatalError("GLSL link failure\n");
    }
}


static GCOps glamor_gc_ops = {
    .FillSpans = glamor_fill_spans,
    .SetSpans = glamor_set_spans,
    .PutImage = glamor_put_image,
    .CopyArea = glamor_copy_area,
    .CopyPlane = glamor_copy_plane,
    .PolyPoint = glamor_poly_point,
    .Polylines = glamor_poly_lines,
    .PolySegment = glamor_poly_segment,
    .PolyRectangle = miPolyRectangle,
    .PolyArc = miPolyArc,
    .FillPolygon = miFillPolygon,
    .PolyFillRect = glamor_poly_fill_rect,
    .PolyFillArc = miPolyFillArc,
    .PolyText8 = glamor_poly_text8,
    .PolyText16 = glamor_poly_text16,
    .ImageText8 = glamor_image_text8,
    .ImageText16 = glamor_image_text16,
    .ImageGlyphBlt = miImageGlyphBlt,
    .PolyGlyphBlt = glamor_poly_glyph_blt,
    .PushPixels = glamor_push_pixels,
};

/*
 * When the stipple is changed or drawn to, invalidate any
 * cached copy
 */
static void
glamor_invalidate_stipple(GCPtr gc)
{
    glamor_gc_private *gc_priv = glamor_get_gc_private(gc);

    if (gc_priv->stipple) {
        if (gc_priv->stipple_damage)
            DamageUnregister(gc_priv->stipple_damage);
        glamor_destroy_pixmap(gc_priv->stipple);
        gc_priv->stipple = NULL;
    }
}

static void
glamor_stipple_damage_report(DamagePtr damage, RegionPtr region,
                             void *closure)
{
    GCPtr       gc = closure;

    glamor_invalidate_stipple(gc);
}

static void
glamor_stipple_damage_destroy(DamagePtr damage, void *closure)
{
    GCPtr               gc = closure;
    glamor_gc_private   *gc_priv = glamor_get_gc_private(gc);

    gc_priv->stipple_damage = NULL;
    glamor_invalidate_stipple(gc);
}

void
glamor_track_stipple(GCPtr gc)
{
    if (gc->stipple) {
        glamor_gc_private *gc_priv = glamor_get_gc_private(gc);

        if (!gc_priv->stipple_damage)
            gc_priv->stipple_damage = DamageCreate(glamor_stipple_damage_report,
                                                   glamor_stipple_damage_destroy,
                                                   DamageReportNonEmpty,
                                                   TRUE, gc->pScreen, gc);
        if (gc_priv->stipple_damage)
            DamageRegister(&gc->stipple->drawable, gc_priv->stipple_damage);
    }
}

/**
 * uxa_validate_gc() sets the ops to glamor's implementations, which may be
 * accelerated or may sync the card and fall back to fb.
 */
void
glamor_validate_gc(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
    /* fbValidateGC will do direct access to pixmaps if the tiling has changed.
     * Preempt fbValidateGC by doing its work and masking the change out, so
     * that we can do the Prepare/finish_access.
     */
    if (changes & GCTile) {
        if (!gc->tileIsPixel) {
            glamor_pixmap_private *pixmap_priv =
                glamor_get_pixmap_private(gc->tile.pixmap);
            if ((!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
                && FbEvenTile(gc->tile.pixmap->drawable.width *
                              drawable->bitsPerPixel)) {
                glamor_fallback
                    ("GC %p tile changed %p.\n", gc, gc->tile.pixmap);
                if (glamor_prepare_access
                    (&gc->tile.pixmap->drawable, GLAMOR_ACCESS_RW)) {
                    fbPadPixmap(gc->tile.pixmap);
                    glamor_finish_access(&gc->tile.pixmap->drawable);
                }
            }
        }
        /* Mask out the GCTile change notification, now that we've done FB's
         * job for it.
         */
        changes &= ~GCTile;
    }

    if (changes & GCStipple)
        glamor_invalidate_stipple(gc);

    if (changes & GCStipple && gc->stipple) {
        /* We can't inline stipple handling like we do for GCTile because
         * it sets fbgc privates.
         */
        if (glamor_prepare_access(&gc->stipple->drawable, GLAMOR_ACCESS_RW)) {
            fbValidateGC(gc, changes, drawable);
            glamor_finish_access(&gc->stipple->drawable);
        }
    }
    else {
        fbValidateGC(gc, changes, drawable);
    }

    if (changes & GCDashList) {
        glamor_gc_private *gc_priv = glamor_get_gc_private(gc);

        if (gc_priv->dash) {
            glamor_destroy_pixmap(gc_priv->dash);
            gc_priv->dash = NULL;
        }
    }

    gc->ops = &glamor_gc_ops;
}

void
glamor_destroy_gc(GCPtr gc)
{
    glamor_gc_private *gc_priv = glamor_get_gc_private(gc);

    if (gc_priv->dash) {
        glamor_destroy_pixmap(gc_priv->dash);
        gc_priv->dash = NULL;
    }
    glamor_invalidate_stipple(gc);
    if (gc_priv->stipple_damage)
        DamageDestroy(gc_priv->stipple_damage);
    miDestroyGC(gc);
}

static GCFuncs glamor_gc_funcs = {
    glamor_validate_gc,
    miChangeGC,
    miCopyGC,
    glamor_destroy_gc,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

/**
 * exaCreateGC makes a new GC and hooks up its funcs handler, so that
 * exaValidateGC() will get called.
 */
int
glamor_create_gc(GCPtr gc)
{
    glamor_gc_private *gc_priv = glamor_get_gc_private(gc);

    gc_priv->dash = NULL;
    gc_priv->stipple = NULL;
    if (!fbCreateGC(gc))
        return FALSE;

    gc->funcs = &glamor_gc_funcs;

    return TRUE;
}

RegionPtr
glamor_bitmap_to_region(PixmapPtr pixmap)
{
    RegionPtr ret;

    glamor_fallback("pixmap %p \n", pixmap);
    if (!glamor_prepare_access(&pixmap->drawable, GLAMOR_ACCESS_RO))
        return NULL;
    ret = fbPixmapToRegion(pixmap);
    glamor_finish_access(&pixmap->drawable);
    return ret;
}

