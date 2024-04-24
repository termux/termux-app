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

#include <stdlib.h>

#include "glamor_priv.h"

void
glamor_destroy_fbo(glamor_screen_private *glamor_priv,
                   glamor_pixmap_fbo *fbo)
{
    glamor_make_current(glamor_priv);

    if (fbo->fb)
        glDeleteFramebuffers(1, &fbo->fb);
    if (fbo->tex)
        glDeleteTextures(1, &fbo->tex);

    free(fbo);
}

static int
glamor_pixmap_ensure_fb(glamor_screen_private *glamor_priv,
                        glamor_pixmap_fbo *fbo)
{
    int status, err = 0;

    glamor_make_current(glamor_priv);

    if (fbo->fb == 0)
        glGenFramebuffers(1, &fbo->fb);
    assert(fbo->tex != 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, fbo->tex, 0);
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char *str;

        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            str = "incomplete attachment";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            str = "incomplete/missing attachment";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            str = "incomplete draw buffer";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            str = "incomplete read buffer";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            str = "unsupported";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            str = "incomplete multiple";
            break;
        default:
            str = "unknown error";
            break;
        }

        glamor_fallback("glamor: Failed to create fbo, %s\n", str);
        err = -1;
    }

    return err;
}

glamor_pixmap_fbo *
glamor_create_fbo_from_tex(glamor_screen_private *glamor_priv,
                           PixmapPtr pixmap, int w, int h, GLint tex, int flag)
{
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);
    glamor_pixmap_fbo *fbo;

    fbo = calloc(1, sizeof(*fbo));
    if (fbo == NULL)
        return NULL;

    fbo->tex = tex;
    fbo->width = w;
    fbo->height = h;
    fbo->is_red = f->format == GL_RED;

    if (flag != GLAMOR_CREATE_FBO_NO_FBO) {
        if (glamor_pixmap_ensure_fb(glamor_priv, fbo) != 0) {
            glamor_destroy_fbo(glamor_priv, fbo);
            fbo = NULL;
        }
    }

    return fbo;
}

static int
_glamor_create_tex(glamor_screen_private *glamor_priv,
                   PixmapPtr pixmap, int w, int h)
{
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);
    unsigned int tex;

    glamor_make_current(glamor_priv);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (f->format == GL_RED)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glamor_priv->suppress_gl_out_of_memory_logging = true;
    glTexImage2D(GL_TEXTURE_2D, 0, f->internalformat, w, h, 0,
                 f->format, f->type, NULL);
    glamor_priv->suppress_gl_out_of_memory_logging = false;

    if (glGetError() == GL_OUT_OF_MEMORY) {
        if (!glamor_priv->logged_any_fbo_allocation_failure) {
            LogMessageVerb(X_WARNING, 0, "glamor: Failed to allocate %dx%d "
                           "FBO due to GL_OUT_OF_MEMORY.\n", w, h);
            LogMessageVerb(X_WARNING, 0,
                           "glamor: Expect reduced performance.\n");
            glamor_priv->logged_any_fbo_allocation_failure = true;
        }
        glDeleteTextures(1, &tex);
        return 0;
    }

    return tex;
}

glamor_pixmap_fbo *
glamor_create_fbo(glamor_screen_private *glamor_priv,
                  PixmapPtr pixmap, int w, int h, int flag)
{
    GLint tex = _glamor_create_tex(glamor_priv, pixmap, w, h);

    if (!tex) /* Texture creation failed due to GL_OUT_OF_MEMORY */
        return NULL;

    return glamor_create_fbo_from_tex(glamor_priv, pixmap, w, h,
                                      tex, flag);
}

/**
 * Create storage for the w * h region, using FBOs of the GL's maximum
 * supported size.
 */
glamor_pixmap_fbo *
glamor_create_fbo_array(glamor_screen_private *glamor_priv,
                        PixmapPtr pixmap, int flag,
                         int block_w, int block_h,
                         glamor_pixmap_private *priv)
{
    int w = pixmap->drawable.width;
    int h = pixmap->drawable.height;
    int block_wcnt;
    int block_hcnt;
    glamor_pixmap_fbo **fbo_array;
    BoxPtr box_array;
    int i, j;

    priv->block_w = block_w;
    priv->block_h = block_h;

    block_wcnt = (w + block_w - 1) / block_w;
    block_hcnt = (h + block_h - 1) / block_h;

    box_array = calloc(block_wcnt * block_hcnt, sizeof(box_array[0]));
    if (box_array == NULL)
        return NULL;

    fbo_array = calloc(block_wcnt * block_hcnt, sizeof(glamor_pixmap_fbo *));
    if (fbo_array == NULL) {
        free(box_array);
        return FALSE;
    }
    for (i = 0; i < block_hcnt; i++) {
        int block_y1, block_y2;
        int fbo_w, fbo_h;

        block_y1 = i * block_h;
        block_y2 = (block_y1 + block_h) > h ? h : (block_y1 + block_h);
        fbo_h = block_y2 - block_y1;

        for (j = 0; j < block_wcnt; j++) {
            box_array[i * block_wcnt + j].x1 = j * block_w;
            box_array[i * block_wcnt + j].y1 = block_y1;
            box_array[i * block_wcnt + j].x2 =
                (j + 1) * block_w > w ? w : (j + 1) * block_w;
            box_array[i * block_wcnt + j].y2 = block_y2;
            fbo_w =
                box_array[i * block_wcnt + j].x2 - box_array[i * block_wcnt +
                                                             j].x1;
            fbo_array[i * block_wcnt + j] = glamor_create_fbo(glamor_priv,
                                                              pixmap,
                                                              fbo_w, fbo_h,
                                                              GLAMOR_CREATE_PIXMAP_FIXUP);
            if (fbo_array[i * block_wcnt + j] == NULL)
                goto cleanup;
        }
    }

    priv->box = box_array[0];
    priv->box_array = box_array;
    priv->fbo_array = fbo_array;
    priv->block_wcnt = block_wcnt;
    priv->block_hcnt = block_hcnt;
    return fbo_array[0];

 cleanup:
    for (i = 0; i < block_wcnt * block_hcnt; i++)
        if (fbo_array[i])
            glamor_destroy_fbo(glamor_priv, fbo_array[i]);
    free(box_array);
    free(fbo_array);
    return NULL;
}

void
glamor_pixmap_clear_fbo(glamor_screen_private *glamor_priv, glamor_pixmap_fbo *fbo,
                        const struct glamor_format *pixmap_format)
{
    glamor_make_current(glamor_priv);

    assert(fbo->fb != 0 && fbo->tex != 0);

    if (glamor_priv->has_clear_texture) {
        glClearTexImage(fbo->tex, 0, pixmap_format->format, pixmap_format->type, NULL);
    }
    else {
        glamor_set_destination_pixmap_fbo(glamor_priv, fbo, 0, 0, fbo->width, fbo->height);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

glamor_pixmap_fbo *
glamor_pixmap_detach_fbo(glamor_pixmap_private *pixmap_priv)
{
    glamor_pixmap_fbo *fbo;

    if (pixmap_priv == NULL)
        return NULL;

    fbo = pixmap_priv->fbo;
    if (fbo == NULL)
        return NULL;

    pixmap_priv->fbo = NULL;
    return fbo;
}

/* The pixmap must not be attached to another fbo. */
void
glamor_pixmap_attach_fbo(PixmapPtr pixmap, glamor_pixmap_fbo *fbo)
{
    glamor_pixmap_private *pixmap_priv;

    pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (pixmap_priv->fbo)
        return;

    pixmap_priv->fbo = fbo;

    switch (pixmap_priv->type) {
    case GLAMOR_TEXTURE_ONLY:
    case GLAMOR_TEXTURE_DRM:
        pixmap_priv->gl_fbo = GLAMOR_FBO_NORMAL;
        pixmap->devPrivate.ptr = NULL;
    default:
        break;
    }
}

void
glamor_pixmap_destroy_fbo(PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);
    glamor_pixmap_fbo *fbo;

    if (glamor_pixmap_priv_is_large(priv)) {
        int i;

        for (i = 0; i < priv->block_wcnt * priv->block_hcnt; i++)
            glamor_destroy_fbo(glamor_priv, priv->fbo_array[i]);
        free(priv->fbo_array);
        priv->fbo_array = NULL;
    }
    else {
        fbo = glamor_pixmap_detach_fbo(priv);
        if (fbo)
            glamor_destroy_fbo(glamor_priv, fbo);
    }
}

Bool
glamor_pixmap_ensure_fbo(PixmapPtr pixmap, int flag)
{
    glamor_screen_private *glamor_priv;
    glamor_pixmap_private *pixmap_priv;
    glamor_pixmap_fbo *fbo;

    glamor_priv = glamor_get_screen_private(pixmap->drawable.pScreen);
    pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (pixmap_priv->fbo == NULL) {

        fbo = glamor_create_fbo(glamor_priv, pixmap, pixmap->drawable.width,
                                pixmap->drawable.height, flag);
        if (fbo == NULL)
            return FALSE;

        glamor_pixmap_attach_fbo(pixmap, fbo);
    }
    else {
        /* We do have a fbo, but it may lack of fb or tex. */
        if (!pixmap_priv->fbo->tex)
            pixmap_priv->fbo->tex =
                _glamor_create_tex(glamor_priv, pixmap, pixmap->drawable.width,
                                   pixmap->drawable.height);

        if (flag != GLAMOR_CREATE_FBO_NO_FBO && pixmap_priv->fbo->fb == 0)
            if (glamor_pixmap_ensure_fb(glamor_priv, pixmap_priv->fbo) != 0)
                return FALSE;
    }

    return TRUE;
}

_X_EXPORT void
glamor_pixmap_exchange_fbos(PixmapPtr front, PixmapPtr back)
{
    glamor_pixmap_private *front_priv, *back_priv;
    glamor_pixmap_fbo *temp_fbo;

    front_priv = glamor_get_pixmap_private(front);
    back_priv = glamor_get_pixmap_private(back);
    temp_fbo = front_priv->fbo;
    front_priv->fbo = back_priv->fbo;
    back_priv->fbo = temp_fbo;
}
