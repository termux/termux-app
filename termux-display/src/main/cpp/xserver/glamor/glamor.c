/*
 * Copyright © 2008,2011 Intel Corporation
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
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *    Chad Versace <chad.versace@linux.intel.com>
 */

/** @file glamor.c
 * This file covers the initialization and teardown of glamor, and has various
 * functions not responsible for performing rendering.
 */

#include <stdlib.h>
#include <unistd.h>

#include "glamor_priv.h"
#include "mipict.h"

DevPrivateKeyRec glamor_screen_private_key;
DevPrivateKeyRec glamor_pixmap_private_key;
DevPrivateKeyRec glamor_gc_private_key;

glamor_screen_private *
glamor_get_screen_private(ScreenPtr screen)
{
    return (glamor_screen_private *)
        dixLookupPrivate(&screen->devPrivates, &glamor_screen_private_key);
}

void
glamor_set_screen_private(ScreenPtr screen, glamor_screen_private *priv)
{
    dixSetPrivate(&screen->devPrivates, &glamor_screen_private_key, priv);
}

/**
 * glamor_get_drawable_pixmap() returns a backing pixmap for a given drawable.
 *
 * @param drawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.  Note that
 * coordinate translation is needed when drawing to the backing pixmap of a
 * redirected window, and the translation coordinates are provided by calling
 * exaGetOffscreenPixmap() on the drawable.
 */
PixmapPtr
glamor_get_drawable_pixmap(DrawablePtr drawable)
{
    if (drawable->type == DRAWABLE_WINDOW)
        return drawable->pScreen->GetWindowPixmap((WindowPtr) drawable);
    else
        return (PixmapPtr) drawable;
}

static void
glamor_init_pixmap_private_small(PixmapPtr pixmap, glamor_pixmap_private *pixmap_priv)
{
    pixmap_priv->box.x1 = 0;
    pixmap_priv->box.x2 = pixmap->drawable.width;
    pixmap_priv->box.y1 = 0;
    pixmap_priv->box.y2 = pixmap->drawable.height;
    pixmap_priv->block_w = pixmap->drawable.width;
    pixmap_priv->block_h = pixmap->drawable.height;
    pixmap_priv->block_hcnt = 1;
    pixmap_priv->block_wcnt = 1;
    pixmap_priv->box_array = &pixmap_priv->box;
    pixmap_priv->fbo_array = &pixmap_priv->fbo;
}

_X_EXPORT void
glamor_set_pixmap_type(PixmapPtr pixmap, glamor_pixmap_type_t type)
{
    glamor_pixmap_private *pixmap_priv;

    pixmap_priv = glamor_get_pixmap_private(pixmap);
    pixmap_priv->type = type;
    glamor_init_pixmap_private_small(pixmap, pixmap_priv);
}

_X_EXPORT Bool
glamor_set_pixmap_texture(PixmapPtr pixmap, unsigned int tex)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_pixmap_private *pixmap_priv;
    glamor_screen_private *glamor_priv;
    glamor_pixmap_fbo *fbo;

    glamor_priv = glamor_get_screen_private(screen);
    pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (pixmap_priv->fbo) {
        fbo = glamor_pixmap_detach_fbo(pixmap_priv);
        glamor_destroy_fbo(glamor_priv, fbo);
    }

    fbo = glamor_create_fbo_from_tex(glamor_priv, pixmap,
                                     pixmap->drawable.width,
                                     pixmap->drawable.height, tex, 0);

    if (fbo == NULL) {
        ErrorF("XXX fail to create fbo.\n");
        return FALSE;
    }

    glamor_pixmap_attach_fbo(pixmap, fbo);

    return TRUE;
}

_X_EXPORT void
glamor_clear_pixmap(PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv;
    glamor_pixmap_private *pixmap_priv;
    const struct glamor_format *pixmap_format;

    glamor_priv = glamor_get_screen_private(screen);
    pixmap_priv = glamor_get_pixmap_private(pixmap);
    pixmap_format = glamor_format_for_pixmap(pixmap);

    assert(pixmap_priv->fbo != NULL);

    glamor_pixmap_clear_fbo(glamor_priv, pixmap_priv->fbo, pixmap_format);
}

uint32_t
glamor_get_pixmap_texture(PixmapPtr pixmap)
{
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (!pixmap_priv)
        return 0;

    if (!pixmap_priv->fbo)
        return 0;

    if (pixmap_priv->type != GLAMOR_TEXTURE_ONLY)
        return 0;

    return pixmap_priv->fbo->tex;
}

void
glamor_bind_texture(glamor_screen_private *glamor_priv, GLenum texture,
                    glamor_pixmap_fbo *fbo, Bool destination_red)
{
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_2D, fbo->tex);

    /* If we're pulling data from a GL_RED texture, then whether we
     * want to make it an A,0,0,0 result or a 0,0,0,R result depends
     * on whether the destination is also a GL_RED texture.
     *
     * For GL_RED destinations, we need to leave the bits in the R
     * channel. For all other destinations, we need to clear out the R
     * channel so that it returns zero for R, G and B.
     *
     * Note that we're leaving the SWIZZLE_A value alone; for GL_RED
     * destinations, that means we'll actually be returning R,0,0,R,
     * but it doesn't matter as the bits in the alpha channel aren't
     * going anywhere.
     */

    /* Is the operand a GL_RED fbo?
     */

    if (fbo->is_red) {
        /* If destination is also GL_RED, then preserve the bits in
         * the R channel */

        if (destination_red)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
    }
}

PixmapPtr
glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
                     unsigned int usage)
{
    PixmapPtr pixmap;
    glamor_pixmap_private *pixmap_priv;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_fbo *fbo = NULL;
    int pitch;

    if (w > 32767 || h > 32767)
        return NullPixmap;

    if ((usage == GLAMOR_CREATE_PIXMAP_CPU
         || (usage == CREATE_PIXMAP_USAGE_GLYPH_PICTURE &&
             w <= glamor_priv->glyph_max_dim &&
             h <= glamor_priv->glyph_max_dim)
         || (w == 0 && h == 0)
         || !glamor_priv->formats[depth].rendering_supported))
        return fbCreatePixmap(screen, w, h, depth, usage);
    else
        pixmap = fbCreatePixmap(screen, 0, 0, depth, usage);

    pixmap_priv = glamor_get_pixmap_private(pixmap);

    pixmap_priv->is_cbcr = (usage == GLAMOR_CREATE_FORMAT_CBCR);

    pitch = (((w * pixmap->drawable.bitsPerPixel + 7) / 8) + 3) & ~3;
    screen->ModifyPixmapHeader(pixmap, w, h, 0, 0, pitch, NULL);

    pixmap_priv->type = GLAMOR_TEXTURE_ONLY;

    if (usage == GLAMOR_CREATE_PIXMAP_NO_TEXTURE) {
        glamor_init_pixmap_private_small(pixmap, pixmap_priv);
        return pixmap;
    }
    else if (usage == GLAMOR_CREATE_NO_LARGE ||
        glamor_check_fbo_size(glamor_priv, w, h))
    {
        glamor_init_pixmap_private_small(pixmap, pixmap_priv);
        fbo = glamor_create_fbo(glamor_priv, pixmap, w, h, usage);
    } else {
        int tile_size = glamor_priv->max_fbo_size;
        DEBUGF("Create LARGE pixmap %p width %d height %d, tile size %d\n",
               pixmap, w, h, tile_size);
        fbo = glamor_create_fbo_array(glamor_priv, pixmap, usage,
                                      tile_size, tile_size, pixmap_priv);
    }

    if (fbo == NULL) {
        fbDestroyPixmap(pixmap);
        return fbCreatePixmap(screen, w, h, depth, usage);
    }

    glamor_pixmap_attach_fbo(pixmap, fbo);

    return pixmap;
}

Bool
glamor_destroy_pixmap(PixmapPtr pixmap)
{
    if (pixmap->refcnt == 1) {
        glamor_pixmap_destroy_fbo(pixmap);
    }

    return fbDestroyPixmap(pixmap);
}

void
glamor_block_handler(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);
    glFlush();
}

static void
_glamor_block_handler(ScreenPtr screen, void *timeout)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);
    glFlush();

    screen->BlockHandler = glamor_priv->saved_procs.block_handler;
    screen->BlockHandler(screen, timeout);
    glamor_priv->saved_procs.block_handler = screen->BlockHandler;
    screen->BlockHandler = _glamor_block_handler;
}

static void
glamor_set_debug_level(int *debug_level)
{
    char *debug_level_string;

    debug_level_string = getenv("GLAMOR_DEBUG");
    if (debug_level_string
        && sscanf(debug_level_string, "%d", debug_level) == 1)
        return;
    *debug_level = 0;
}

int glamor_debug_level;

void
glamor_gldrawarrays_quads_using_indices(glamor_screen_private *glamor_priv,
                                        unsigned count)
{
    unsigned i;

    /* For a single quad, don't bother with an index buffer. */
    if (count ==  1)
        goto fallback;

    if (glamor_priv->ib_size < count) {
        /* Basic GLES2 doesn't have any way to map buffer objects for
         * writing, but it's long past time for drivers to have
         * MapBufferRange.
         */
        if (!glamor_priv->has_map_buffer_range)
            goto fallback;

        /* Lazy create the buffer name, and only bind it once since
         * none of the glamor code binds it to anything else.
         */
        if (!glamor_priv->ib) {
            glGenBuffers(1, &glamor_priv->ib);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glamor_priv->ib);
        }

        /* For now, only support GL_UNSIGNED_SHORTs. */
        if (count > ((1 << 16) - 1) / 4) {
            goto fallback;
        } else {
            uint16_t *data;
            size_t size = count * 6 * sizeof(GLushort);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
            data = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER,
                                    0, size,
                                    GL_MAP_WRITE_BIT |
                                    GL_MAP_INVALIDATE_BUFFER_BIT);
            for (i = 0; i < count; i++) {
                data[i * 6 + 0] = i * 4 + 0;
                data[i * 6 + 1] = i * 4 + 1;
                data[i * 6 + 2] = i * 4 + 2;
                data[i * 6 + 3] = i * 4 + 0;
                data[i * 6 + 4] = i * 4 + 2;
                data[i * 6 + 5] = i * 4 + 3;
            }
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

            glamor_priv->ib_size = count;
            glamor_priv->ib_type = GL_UNSIGNED_SHORT;
        }
    }

    glDrawElements(GL_TRIANGLES, count * 6, glamor_priv->ib_type, NULL);
    return;

fallback:
    for (i = 0; i < count; i++)
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
}


static Bool
glamor_check_instruction_count(int gl_version)
{
    GLint max_native_alu_instructions;

    /* Avoid using glamor if the reported instructions limit is too low,
     * as this would cause glamor to fallback on sw due to large shaders
     * which ends up being unbearably slow.
     */
    if (gl_version < 30) {
        if (!epoxy_has_gl_extension("GL_ARB_fragment_program")) {
            ErrorF("GL_ARB_fragment_program required\n");
            return FALSE;
        }

        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
                          GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB,
                          &max_native_alu_instructions);
        if (max_native_alu_instructions < GLAMOR_MIN_ALU_INSTRUCTIONS) {
            LogMessage(X_WARNING,
                       "glamor requires at least %d instructions (%d reported)\n",
                       GLAMOR_MIN_ALU_INSTRUCTIONS, max_native_alu_instructions);
            return FALSE;
        }
    }

    return TRUE;
}

static void GLAPIENTRY
glamor_debug_output_callback(GLenum source,
                             GLenum type,
                             GLuint id,
                             GLenum severity,
                             GLsizei length,
                             const GLchar *message,
                             const void *userParam)
{
    ScreenPtr screen = (void *)userParam;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    if (glamor_priv->suppress_gl_out_of_memory_logging &&
        source == GL_DEBUG_SOURCE_API && type == GL_DEBUG_TYPE_ERROR) {
        return;
    }

    LogMessageVerb(X_ERROR, 0, "glamor%d: GL error: %*s\n",
               screen->myNum, length, message);
    xorg_backtrace();
}

/**
 * Configures GL_ARB_debug_output to give us immediate callbacks when
 * GL errors occur, so that we can log them.
 */
static void
glamor_setup_debug_output(ScreenPtr screen)
{
    if (!epoxy_has_gl_extension("GL_KHR_debug") &&
        !epoxy_has_gl_extension("GL_ARB_debug_output"))
        return;

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    /* Disable debugging messages other than GL API errors */
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL,
                          GL_FALSE);
    glDebugMessageControl(GL_DEBUG_SOURCE_API,
                          GL_DEBUG_TYPE_ERROR,
                          GL_DONT_CARE,
                          0, NULL, GL_TRUE);
    glDebugMessageCallback(glamor_debug_output_callback,
                           screen);

    /* If KHR_debug is present, all debug output is disabled by
     * default on non-debug contexts.
     */
    if (epoxy_has_gl_extension("GL_KHR_debug"))
        glEnable(GL_DEBUG_OUTPUT);
}

const struct glamor_format *
glamor_format_for_pixmap(PixmapPtr pixmap)
{
    ScreenPtr pScreen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (pixmap_priv->is_cbcr)
        return &glamor_priv->cbcr_format;
    else
        return &glamor_priv->formats[pixmap->drawable.depth];
}

static void
glamor_add_format(ScreenPtr screen, int depth, CARD32 render_format,
                  GLenum internalformat, GLenum format, GLenum type,
                  Bool rendering_supported)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    struct glamor_format *f = &glamor_priv->formats[depth];

    /* If we're trying to run on GLES, make sure that we get the read
     * formats that we're expecting, since glamor_transfer relies on
     * them matching to get data back out.  To avoid this limitation, we
     * would need to have a more general glReadPixels() path in
     * glamor_transfer that re-encoded the bits to the pixel format that
     * we intended after.
     *
     * Note that we can't just create a pixmap because we're in
     * screeninit.
     */
    if (rendering_supported && glamor_priv->is_gles) {
        unsigned fbo, tex;
        int read_format, read_type;
        GLenum status;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, 1, 1, 0,
                     format, type, NULL);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, tex, 0);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            ErrorF("glamor: Test fbo for depth %d incomplete.  "
                   "Falling back to software.\n", depth);
            glDeleteTextures(1, &tex);
            glDeleteFramebuffers(1, &fbo);
            return;
        }

        glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &read_format);
        glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &read_type);

        glDeleteTextures(1, &tex);
        glDeleteFramebuffers(1, &fbo);

        if (format != read_format || type != read_type) {
            ErrorF("glamor: Implementation returned 0x%x/0x%x read format/type "
                   "for depth %d, expected 0x%x/0x%x.  "
                   "Falling back to software.\n",
                   read_format, read_type, depth, format, type);
            return;
        }
    }

    f->depth = depth;
    f->render_format = render_format;
    f->internalformat = internalformat;
    f->format = format;
    f->type = type;
    f->rendering_supported = rendering_supported;
}

/* Set up the GL format/types that glamor will use for the various depths
 *
 * X11's pixel data doesn't have channels, but to store our data in GL
 * we have to pick some sort of format to move X11 pixel data in and
 * out with in glamor_transfer.c.  For X11 core operations, other than
 * GL logic ops (non-GXcopy GC ops) what the driver chooses internally
 * doesn't matter as long as it doesn't drop any bits (we expect them
 * to generally expand, if anything).  For Render, we can expect
 * clients to tend to render with PictFormats matching our channel
 * layouts here since ultimately X11 pixels tend to end up on the
 * screen.  The render implementation will fall back to fb if the
 * channels don't match.
 *
 * Note that these formats don't affect what glamor_egl.c or
 * Xwayland's EGL layer choose for surfaces exposed through DRI or
 * scanout.  For now, those layers need to match what we're choosing
 * here, or channels will end up swizzled around.  Similarly, the
 * driver's visual masks also need to match what we're doing here.
 */
static void
glamor_setup_formats(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    /* Prefer r8 textures since they're required by GLES3 and core,
     * only falling back to a8 if we can't do them.
     */
    if (glamor_priv->is_gles || epoxy_has_gl_extension("GL_ARB_texture_rg")) {
        glamor_add_format(screen, 1, PICT_a1,
                          GL_R8, GL_RED, GL_UNSIGNED_BYTE, FALSE);
        glamor_add_format(screen, 8, PICT_a8,
                          GL_R8, GL_RED, GL_UNSIGNED_BYTE, TRUE);
    } else {
        glamor_add_format(screen, 1, PICT_a1,
                          GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, FALSE);
        glamor_add_format(screen, 8, PICT_a8,
                          GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, TRUE);
    }

    if (glamor_priv->is_gles) {
        /* For 15bpp, GLES supports format/type RGBA/5551, rather than
         * bgra/1555_rev.  GL_EXT_bgra lets the impl say the color
         * read format/type is bgra/1555 even if we had to create it
         * with rgba/5551, with Mesa does.  That means we can't use
         * the same format/type for TexSubImage and readpixels.
         *
         * Instead, just store 16 bits using the trusted 565 path, and
         * disable render accel for now.
         */
        glamor_add_format(screen, 15, PICT_x1r5g5b5,
                          GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, TRUE);
    } else {
        glamor_add_format(screen, 15, PICT_x1r5g5b5,
                          GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, TRUE);
    }

    glamor_add_format(screen, 16, PICT_r5g6b5,
                      GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, TRUE);

    if (glamor_priv->is_gles) {
        assert(X_BYTE_ORDER == X_LITTLE_ENDIAN);
        glamor_add_format(screen, 24, PICT_x8b8g8r8,
                          GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, TRUE);
        glamor_add_format(screen, 32, PICT_a8b8g8r8,
                          GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, TRUE);
    } else {
        glamor_add_format(screen, 24, PICT_x8r8g8b8,
                          GL_RGBA, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, TRUE);
        glamor_add_format(screen, 32, PICT_a8r8g8b8,
                          GL_RGBA, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, TRUE);
    }

    if (glamor_priv->is_gles) {
        glamor_add_format(screen, 30, PICT_x2b10g10r10,
                          GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, TRUE);
    } else {
        glamor_add_format(screen, 30, PICT_x2r10g10b10,
                          GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, TRUE);
    }

    glamor_priv->cbcr_format.depth = 16;
    glamor_priv->cbcr_format.internalformat = GL_RG8;
    glamor_priv->cbcr_format.format = GL_RG;
    glamor_priv->cbcr_format.type = GL_UNSIGNED_BYTE;
    glamor_priv->cbcr_format.rendering_supported = TRUE;
}

/** Set up glamor for an already-configured GL context. */
Bool
glamor_init(ScreenPtr screen, unsigned int flags)
{
    glamor_screen_private *glamor_priv;
    int gl_version;
    int glsl_major, glsl_minor;
    int max_viewport_size[2];
    const char *shading_version_string;
    int shading_version_offset;

    PictureScreenPtr ps = GetPictureScreenIfSet(screen);

    if (flags & ~GLAMOR_VALID_FLAGS) {
        ErrorF("glamor_init: Invalid flags %x\n", flags);
        return FALSE;
    }
    glamor_priv = calloc(1, sizeof(*glamor_priv));
    if (glamor_priv == NULL)
        return FALSE;

    glamor_priv->flags = flags;

    if (!dixRegisterPrivateKey(&glamor_screen_private_key, PRIVATE_SCREEN, 0)) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to allocate screen private\n",
                   screen->myNum);
        goto free_glamor_private;
    }

    glamor_set_screen_private(screen, glamor_priv);

    if (!dixRegisterPrivateKey(&glamor_pixmap_private_key, PRIVATE_PIXMAP,
                               sizeof(struct glamor_pixmap_private))) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to allocate pixmap private\n",
                   screen->myNum);
        goto free_glamor_private;
    }

    if (!dixRegisterPrivateKey(&glamor_gc_private_key, PRIVATE_GC,
                               sizeof (glamor_gc_private))) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to allocate gc private\n",
                   screen->myNum);
        goto free_glamor_private;
    }

    glamor_priv->saved_procs.close_screen = screen->CloseScreen;
    screen->CloseScreen = glamor_close_screen;

    glamor_priv->saved_procs.destroy_pixmap = screen->DestroyPixmap;
    screen->DestroyPixmap = glamor_destroy_pixmap;

    /* If we are using egl screen, call egl screen init to
     * register correct close screen function. */
    if (flags & GLAMOR_USE_EGL_SCREEN) {
        glamor_egl_screen_init(screen, &glamor_priv->ctx);
    } else {
        if (!glamor_glx_screen_init(&glamor_priv->ctx))
            goto fail;
    }

    glamor_make_current(glamor_priv);

    if (!epoxy_is_desktop_gl())
        glamor_priv->is_gles = TRUE;

    gl_version = epoxy_gl_version();

    /* assume a core profile if we are GL 3.1 and don't have ARB_compatibility */
    glamor_priv->is_core_profile =
        gl_version >= 31 && !epoxy_has_gl_extension("GL_ARB_compatibility");

    shading_version_string = (char *) glGetString(GL_SHADING_LANGUAGE_VERSION);

    if (!shading_version_string) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to get GLSL version\n",
                   screen->myNum);
        goto fail;
    }

    shading_version_offset = 0;
    if (strncmp("OpenGL ES GLSL ES ", shading_version_string, 18) == 0)
        shading_version_offset = 18;

    if (sscanf(shading_version_string + shading_version_offset,
               "%i.%i",
               &glsl_major,
               &glsl_minor) != 2) {
        LogMessage(X_WARNING,
                   "glamor%d: Failed to parse GLSL version string %s\n",
                   screen->myNum, shading_version_string);
        goto fail;
    }
    glamor_priv->glsl_version = glsl_major * 100 + glsl_minor;

    if (glamor_priv->is_gles) {
        /* Force us back to the base version of our programs on an ES
         * context, anyway.  Basically glamor only uses desktop 1.20
         * or 1.30 currently.  1.30's new features are also present in
         * ES 3.0, but our glamor_program.c constructions use a lot of
         * compatibility features (to reduce the diff between 1.20 and
         * 1.30 programs).
         */
        glamor_priv->glsl_version = 120;
    }

    /* We'd like to require GL_ARB_map_buffer_range or
     * GL_OES_map_buffer_range, since it offers more information to
     * the driver than plain old glMapBuffer() or glBufferSubData().
     * It's been supported on Mesa on the desktop since 2009 and on
     * GLES2 since October 2012.  It's supported on Apple's iOS
     * drivers for SGX535 and A7, but apparently not on most Android
     * devices (the OES extension spec wasn't released until June
     * 2012).
     *
     * 82% of 0 A.D. players (desktop GL) submitting hardware reports
     * have support for it, with most of the ones lacking it being on
     * Windows with Intel 4-series (G45) graphics or older.
     */
    if (!glamor_priv->is_gles) {
        if (gl_version < 21) {
            ErrorF("Require OpenGL version 2.1 or later.\n");
            goto fail;
        }

        if (!glamor_priv->is_core_profile &&
            !epoxy_has_gl_extension("GL_ARB_texture_border_clamp")) {
            ErrorF("GL_ARB_texture_border_clamp required\n");
            goto fail;
        }

        if (!glamor_check_instruction_count(gl_version))
            goto fail;

        /* Glamor rendering assumes that platforms with GLSL 130+
         * have instanced arrays, but this is not always the case.
         * etnaviv offers GLSL 140 with OpenGL 2.1.
         */
        if (glamor_glsl_has_ints(glamor_priv) &&
            !epoxy_has_gl_extension("GL_ARB_instanced_arrays"))
                glamor_priv->glsl_version = 120;
    } else {
        if (gl_version < 20) {
            ErrorF("Require Open GLES2.0 or later.\n");
            goto fail;
        }

        if (!epoxy_has_gl_extension("GL_EXT_texture_format_BGRA8888")) {
            ErrorF("GL_EXT_texture_format_BGRA8888 required\n");
            goto fail;
        }

        if (!epoxy_has_gl_extension("GL_OES_texture_border_clamp")) {
            ErrorF("GL_OES_texture_border_clamp required\n");
            goto fail;
        }
    }

    if (!epoxy_has_gl_extension("GL_ARB_vertex_array_object") &&
        !epoxy_has_gl_extension("GL_OES_vertex_array_object")) {
        ErrorF("GL_{ARB,OES}_vertex_array_object required\n");
        goto fail;
    }

    if (!glamor_priv->is_gles && glamor_priv->glsl_version == 120 &&
        epoxy_has_gl_extension("GL_ARB_instanced_arrays"))
        glamor_priv->use_gpu_shader4 = epoxy_has_gl_extension("GL_EXT_gpu_shader4");

    glamor_priv->has_rw_pbo = FALSE;
    if (!glamor_priv->is_gles)
        glamor_priv->has_rw_pbo = TRUE;

    glamor_priv->has_khr_debug = epoxy_has_gl_extension("GL_KHR_debug");
    glamor_priv->has_pack_invert =
        epoxy_has_gl_extension("GL_MESA_pack_invert");
    glamor_priv->has_fbo_blit =
        epoxy_has_gl_extension("GL_EXT_framebuffer_blit");
    glamor_priv->has_map_buffer_range =
        epoxy_has_gl_extension("GL_ARB_map_buffer_range") ||
        epoxy_has_gl_extension("GL_EXT_map_buffer_range");
    glamor_priv->has_buffer_storage =
        epoxy_has_gl_extension("GL_ARB_buffer_storage");
    glamor_priv->has_mesa_tile_raster_order =
        epoxy_has_gl_extension("GL_MESA_tile_raster_order");
    glamor_priv->has_nv_texture_barrier =
        epoxy_has_gl_extension("GL_NV_texture_barrier");
    glamor_priv->has_unpack_subimage =
        !glamor_priv->is_gles ||
        epoxy_gl_version() >= 30 ||
        epoxy_has_gl_extension("GL_EXT_unpack_subimage");
    glamor_priv->has_pack_subimage =
        !glamor_priv->is_gles ||
        epoxy_gl_version() >= 30 ||
        epoxy_has_gl_extension("GL_NV_pack_subimage");
    glamor_priv->has_dual_blend =
        glamor_glsl_has_ints(glamor_priv) &&
        epoxy_has_gl_extension("GL_ARB_blend_func_extended");
    glamor_priv->has_clear_texture =
        epoxy_gl_version() >= 44 ||
        epoxy_has_gl_extension("GL_ARB_clear_texture");

    glamor_priv->can_copyplane = (gl_version >= 30);

    glamor_setup_debug_output(screen);

    glamor_priv->use_quads = !glamor_priv->is_gles &&
                             !glamor_priv->is_core_profile;

    /* Driver-specific hack: Avoid using GL_QUADS on VC4, where
     * they'll be emulated more expensively than we can with our
     * cached IB.
     */
    if (strstr((char *)glGetString(GL_VENDOR), "Broadcom") &&
        (strstr((char *)glGetString(GL_RENDERER), "VC4") ||
         strstr((char *)glGetString(GL_RENDERER), "V3D")))
        glamor_priv->use_quads = FALSE;

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glamor_priv->max_fbo_size);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glamor_priv->max_fbo_size);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport_size);
    glamor_priv->max_fbo_size = MIN(glamor_priv->max_fbo_size, max_viewport_size[0]);
    glamor_priv->max_fbo_size = MIN(glamor_priv->max_fbo_size, max_viewport_size[1]);
#ifdef MAX_FBO_SIZE
    glamor_priv->max_fbo_size = MAX_FBO_SIZE;
#endif

    glamor_priv->has_texture_swizzle =
        (epoxy_has_gl_extension("GL_ARB_texture_swizzle") ||
         (glamor_priv->is_gles && gl_version >= 30));

    glamor_setup_formats(screen);

    glamor_set_debug_level(&glamor_debug_level);

    if (!glamor_font_init(screen))
        goto fail;

    glamor_priv->saved_procs.block_handler = screen->BlockHandler;
    screen->BlockHandler = _glamor_block_handler;

    if (!glamor_composite_glyphs_init(screen)) {
        ErrorF("Failed to initialize composite masks\n");
        goto fail;
    }

    glamor_priv->saved_procs.create_gc = screen->CreateGC;
    screen->CreateGC = glamor_create_gc;

    glamor_priv->saved_procs.create_pixmap = screen->CreatePixmap;
    screen->CreatePixmap = glamor_create_pixmap;

    glamor_priv->saved_procs.get_spans = screen->GetSpans;
    screen->GetSpans = glamor_get_spans;

    glamor_priv->saved_procs.get_image = screen->GetImage;
    screen->GetImage = glamor_get_image;

    glamor_priv->saved_procs.change_window_attributes =
        screen->ChangeWindowAttributes;
    screen->ChangeWindowAttributes = glamor_change_window_attributes;

    glamor_priv->saved_procs.copy_window = screen->CopyWindow;
    screen->CopyWindow = glamor_copy_window;

    glamor_priv->saved_procs.bitmap_to_region = screen->BitmapToRegion;
    screen->BitmapToRegion = glamor_bitmap_to_region;

    glamor_priv->saved_procs.composite = ps->Composite;
    ps->Composite = glamor_composite;

    glamor_priv->saved_procs.trapezoids = ps->Trapezoids;
    ps->Trapezoids = glamor_trapezoids;

    glamor_priv->saved_procs.triangles = ps->Triangles;
    ps->Triangles = glamor_triangles;

    glamor_priv->saved_procs.addtraps = ps->AddTraps;
    ps->AddTraps = glamor_add_traps;

    glamor_priv->saved_procs.composite_rects = ps->CompositeRects;
    ps->CompositeRects = glamor_composite_rectangles;

    glamor_priv->saved_procs.glyphs = ps->Glyphs;
    ps->Glyphs = glamor_composite_glyphs;

    glamor_init_vbo(screen);
    glamor_init_gradient_shader(screen);
    glamor_pixmap_init(screen);
    glamor_sync_init(screen);

    glamor_priv->screen = screen;

    return TRUE;

 fail:
    /* Restore default CloseScreen and DestroyPixmap handlers */
    screen->CloseScreen = glamor_priv->saved_procs.close_screen;
    screen->DestroyPixmap = glamor_priv->saved_procs.destroy_pixmap;

 free_glamor_private:
    free(glamor_priv);
    glamor_set_screen_private(screen, NULL);
    return FALSE;
}

static void
glamor_release_screen_priv(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv;

    glamor_priv = glamor_get_screen_private(screen);
    glamor_fini_vbo(screen);
    glamor_pixmap_fini(screen);
    free(glamor_priv);

    glamor_set_screen_private(screen, NULL);
}

Bool
glamor_close_screen(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv;
    PixmapPtr screen_pixmap;
    PictureScreenPtr ps = GetPictureScreenIfSet(screen);

    glamor_priv = glamor_get_screen_private(screen);
    glamor_sync_close(screen);
    glamor_composite_glyphs_fini(screen);
    screen->CloseScreen = glamor_priv->saved_procs.close_screen;

    screen->CreateGC = glamor_priv->saved_procs.create_gc;
    screen->CreatePixmap = glamor_priv->saved_procs.create_pixmap;
    screen->DestroyPixmap = glamor_priv->saved_procs.destroy_pixmap;
    screen->GetSpans = glamor_priv->saved_procs.get_spans;
    screen->ChangeWindowAttributes =
        glamor_priv->saved_procs.change_window_attributes;
    screen->CopyWindow = glamor_priv->saved_procs.copy_window;
    screen->BitmapToRegion = glamor_priv->saved_procs.bitmap_to_region;
    screen->BlockHandler = glamor_priv->saved_procs.block_handler;

    ps->Composite = glamor_priv->saved_procs.composite;
    ps->Trapezoids = glamor_priv->saved_procs.trapezoids;
    ps->Triangles = glamor_priv->saved_procs.triangles;
    ps->CompositeRects = glamor_priv->saved_procs.composite_rects;
    ps->Glyphs = glamor_priv->saved_procs.glyphs;

    screen_pixmap = screen->GetScreenPixmap(screen);
    glamor_pixmap_destroy_fbo(screen_pixmap);

    glamor_release_screen_priv(screen);

    return screen->CloseScreen(screen);
}

void
glamor_fini(ScreenPtr screen)
{
    /* Do nothing currently. */
}

void
glamor_enable_dri3(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_priv->dri3_enabled = TRUE;
}

Bool
glamor_supports_pixmap_import_export(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    return glamor_priv->dri3_enabled;
}

_X_EXPORT void
glamor_set_drawable_modifiers_func(ScreenPtr screen,
                                   GetDrawableModifiersFuncPtr func)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_priv->get_drawable_modifiers = func;
}

_X_EXPORT Bool
glamor_get_drawable_modifiers(DrawablePtr draw, uint32_t format,
                              uint32_t *num_modifiers, uint64_t **modifiers)
{
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(draw->pScreen);

    if (glamor_priv->get_drawable_modifiers) {
        return glamor_priv->get_drawable_modifiers(draw, format,
                                                   num_modifiers, modifiers);
    }
    *num_modifiers = 0;
    *modifiers = NULL;
    return TRUE;
}

static int
_glamor_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                        uint32_t *strides, uint32_t *offsets,
                        CARD32 *size, uint64_t *modifier)
{
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    glamor_screen_private *glamor_priv =
        glamor_get_screen_private(pixmap->drawable.pScreen);

    if (!glamor_priv->dri3_enabled)
        return 0;
    switch (pixmap_priv->type) {
    case GLAMOR_TEXTURE_DRM:
    case GLAMOR_TEXTURE_ONLY:
        if (!glamor_pixmap_ensure_fbo(pixmap, 0))
            return 0;

        if (modifier) {
            return glamor_egl_fds_from_pixmap(screen, pixmap, fds,
                                              strides, offsets,
                                              modifier);
        } else {
            CARD16 stride;

            fds[0] = glamor_egl_fd_from_pixmap(screen, pixmap, &stride, size);
            strides[0] = stride;

            return fds[0] >= 0;
        }
    default:
        break;
    }
    return 0;
}

_X_EXPORT int
glamor_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                       uint32_t *strides, uint32_t *offsets,
                       uint64_t *modifier)
{
    return _glamor_fds_from_pixmap(screen, pixmap, fds, strides, offsets,
                                   NULL, modifier);
}

_X_EXPORT int
glamor_fd_from_pixmap(ScreenPtr screen,
                      PixmapPtr pixmap, CARD16 *stride, CARD32 *size)
{
    int fd;
    int ret;
    uint32_t stride32;

    ret = _glamor_fds_from_pixmap(screen, pixmap, &fd, &stride32, NULL, size,
                                  NULL);
    if (ret != 1)
        return -1;

    *stride = stride32;
    return fd;
}

_X_EXPORT int
glamor_shareable_fd_from_pixmap(ScreenPtr screen,
                                PixmapPtr pixmap, CARD16 *stride, CARD32 *size)
{
    unsigned orig_usage_hint = pixmap->usage_hint;
    int ret;

    /*
     * The actual difference between a shareable and non-shareable buffer
     * is decided 4 call levels deep in glamor_make_pixmap_exportable()
     * based on pixmap->usage_hint == CREATE_PIXMAP_USAGE_SHARED
     * 2 of those calls are also exported API, so we cannot just add a flag.
     */
    pixmap->usage_hint = CREATE_PIXMAP_USAGE_SHARED;

    ret = glamor_fd_from_pixmap(screen, pixmap, stride, size);

    pixmap->usage_hint = orig_usage_hint;
    return ret;
}

int
glamor_name_from_pixmap(PixmapPtr pixmap, CARD16 *stride, CARD32 *size)
{
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    switch (pixmap_priv->type) {
    case GLAMOR_TEXTURE_DRM:
    case GLAMOR_TEXTURE_ONLY:
        if (!glamor_pixmap_ensure_fbo(pixmap, 0))
            return -1;
        return glamor_egl_fd_name_from_pixmap(pixmap->drawable.pScreen,
                                              pixmap, stride, size);
    default:
        break;
    }
    return -1;
}

void
glamor_finish(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);
    glFinish();
}
