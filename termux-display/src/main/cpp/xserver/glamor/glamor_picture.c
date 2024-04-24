/*
 * Copyright © 2016 Broadcom
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
 */

/**
 * @file glamor_picture.c
 *
 * Implements temporary uploads of GL_MEMORY Pixmaps to a texture that
 * is swizzled appropriately for a given Render picture format.
 * laid *
 *
 * This is important because GTK likes to use SHM Pixmaps for Render
 * blending operations, and we don't want a blend operation to fall
 * back to software (readback is more expensive than the upload we do
 * here, and you'd have to re-upload the fallback output anyway).
 */

#include <stdlib.h>

#include "glamor_priv.h"
#include "mipict.h"

static void byte_swap_swizzle(GLenum *swizzle)
{
    GLenum temp;

    temp = swizzle[0];
    swizzle[0] = swizzle[3];
    swizzle[3] = temp;

    temp = swizzle[1];
    swizzle[1] = swizzle[2];
    swizzle[2] = temp;
}

/**
 * Returns the GL format and type for uploading our bits to a given PictFormat.
 *
 * We may need to tell the caller to translate the bits to another
 * format, as in PICT_a1 (which GL doesn't support).  We may also need
 * to tell the GL to swizzle the texture on sampling, because GLES3
 * doesn't support the GL_UNSIGNED_INT_8_8_8_8{,_REV} types, so we
 * don't have enough channel reordering options at upload time without
 * it.
 */
static Bool
glamor_get_tex_format_type_from_pictformat(ScreenPtr pScreen,
                                           PictFormatShort format,
                                           PictFormatShort *temp_format,
                                           GLenum *tex_format,
                                           GLenum *tex_type,
                                           GLenum *swizzle)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    Bool is_little_endian = IMAGE_BYTE_ORDER == LSBFirst;

    *temp_format = format;
    swizzle[0] = GL_RED;
    swizzle[1] = GL_GREEN;
    swizzle[2] = GL_BLUE;
    swizzle[3] = GL_ALPHA;

    switch (format) {
    case PICT_a1:
        *tex_format = glamor_priv->formats[1].format;
        *tex_type = GL_UNSIGNED_BYTE;
        *temp_format = PICT_a8;
        break;

    case PICT_b8g8r8x8:
    case PICT_b8g8r8a8:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_8_8_8_8;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_BYTE;

            swizzle[0] = GL_GREEN;
            swizzle[1] = GL_BLUE;
            swizzle[2] = GL_ALPHA;
            swizzle[3] = GL_RED;

            if (!is_little_endian)
                byte_swap_swizzle(swizzle);
        }
        break;

    case PICT_x8r8g8b8:
    case PICT_a8r8g8b8:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_BYTE;

            swizzle[0] = GL_BLUE;
            swizzle[2] = GL_RED;

            if (!is_little_endian)
                byte_swap_swizzle(swizzle);
            break;
        }
        break;

    case PICT_x8b8g8r8:
    case PICT_a8b8g8r8:
        *tex_format = GL_RGBA;
        if (!glamor_priv->is_gles) {
            *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_BYTE;

            if (!is_little_endian)
                byte_swap_swizzle(swizzle);
        }
        break;

    case PICT_x2r10g10b10:
    case PICT_a2r10g10b10:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_x2b10g10r10:
    case PICT_a2b10g10r10:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_r5g6b5:
        *tex_format = GL_RGB;
        *tex_type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case PICT_b5g6r5:
        *tex_format = GL_RGB;
        if (!glamor_priv->is_gles) {
            *tex_type = GL_UNSIGNED_SHORT_5_6_5_REV;
        } else {
            *tex_type = GL_UNSIGNED_SHORT_5_6_5;
            swizzle[0] = GL_BLUE;
            swizzle[2] = GL_RED;
        }
        break;

    case PICT_x1b5g5r5:
    case PICT_a1b5g5r5:
        *tex_format = GL_RGBA;
        if (!glamor_priv->is_gles) {
            *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_x1r5g5b5:
    case PICT_a1r5g5b5:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_a8:
        *tex_format = glamor_priv->formats[8].format;
        *tex_type = GL_UNSIGNED_BYTE;
        break;

    case PICT_x4r4g4b4:
    case PICT_a4r4g4b4:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        } else {
            /* XXX */
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4;
        }
        break;

    case PICT_x4b4g4r4:
    case PICT_a4b4g4r4:
        if (!glamor_priv->is_gles) {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        } else {
            /* XXX */
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4;
        }
        break;

    default:
        return FALSE;
    }

    if (!PICT_FORMAT_A(format))
        swizzle[3] = GL_ONE;

    return TRUE;
}

/**
 * Takes a set of source bits with a given format and returns an
 * in-memory pixman image of those bits in a destination format.
 */
static pixman_image_t *
glamor_get_converted_image(PictFormatShort dst_format,
                           PictFormatShort src_format,
                           void *src_bits,
                           int src_stride,
                           int w, int h)
{
    pixman_image_t *dst_image;
    pixman_image_t *src_image;

    dst_image = pixman_image_create_bits(dst_format, w, h, NULL, 0);
    if (dst_image == NULL) {
        return NULL;
    }

    src_image = pixman_image_create_bits(src_format, w, h, src_bits, src_stride);

    if (src_image == NULL) {
        pixman_image_unref(dst_image);
        return NULL;
    }

    pixman_image_composite(PictOpSrc, src_image, NULL, dst_image,
                           0, 0, 0, 0, 0, 0, w, h);

    pixman_image_unref(src_image);
    return dst_image;
}

/**
 * Uploads a picture based on a GLAMOR_MEMORY pixmap to a texture in a
 * temporary FBO.
 */
Bool
glamor_upload_picture_to_texture(PicturePtr picture)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    PictFormatShort converted_format;
    void *bits = pixmap->devPrivate.ptr;
    int stride = pixmap->devKind;
    GLenum format, type;
    GLenum swizzle[4];
    GLenum iformat;
    Bool ret = TRUE;
    Bool needs_swizzle;
    pixman_image_t *converted_image = NULL;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);

    assert(glamor_pixmap_is_memory(pixmap));
    assert(!pixmap_priv->fbo);

    glamor_make_current(glamor_priv);

    /* No handling of large pixmap pictures here (would need to make
     * an FBO array and split the uploads across it).
     */
    if (!glamor_check_fbo_size(glamor_priv,
                               pixmap->drawable.width,
                               pixmap->drawable.height)) {
        return FALSE;
    }

    if (!glamor_get_tex_format_type_from_pictformat(screen,
                                                    picture->format,
                                                    &converted_format,
                                                    &format,
                                                    &type,
                                                    swizzle)) {
        glamor_fallback("Unknown pixmap depth %d.\n", pixmap->drawable.depth);
        return FALSE;
    }

    needs_swizzle = (swizzle[0] != GL_RED ||
                     swizzle[1] != GL_GREEN ||
                     swizzle[2] != GL_BLUE ||
                     swizzle[3] != GL_ALPHA);

    if (!glamor_priv->has_texture_swizzle && needs_swizzle) {
        glamor_fallback("Couldn't upload temporary picture due to missing "
                        "GL_ARB_texture_swizzle.\n");
        return FALSE;
    }

    if (converted_format != picture->format) {
        converted_image = glamor_get_converted_image(converted_format,
                                                     picture->format,
                                                     bits, stride,
                                                     pixmap->drawable.width,
                                                     pixmap->drawable.height);
        if (!converted_image)
            return FALSE;

        bits = pixman_image_get_data(converted_image);
        stride = pixman_image_get_stride(converted_image);
    }

    if (!glamor_priv->is_gles)
        iformat = f->internalformat;
    else
        iformat = format;

    if (!glamor_pixmap_ensure_fbo(pixmap, GLAMOR_CREATE_FBO_NO_FBO)) {
        ret = FALSE;
        goto fail;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glamor_priv->suppress_gl_out_of_memory_logging = true;

    /* We can't use glamor_pixmap_loop() because GLAMOR_MEMORY pixmaps
     * don't have initialized boxes.
     */
    glBindTexture(GL_TEXTURE_2D, pixmap_priv->fbo->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat,
                 pixmap->drawable.width, pixmap->drawable.height, 0,
                 format, type, bits);

    if (needs_swizzle) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swizzle[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swizzle[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swizzle[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swizzle[3]);
    }

    glamor_priv->suppress_gl_out_of_memory_logging = false;
    if (glGetError() == GL_OUT_OF_MEMORY) {
        ret = FALSE;
    }

fail:
    if (converted_image)
        pixman_image_unref(converted_image);

    return ret;
}
