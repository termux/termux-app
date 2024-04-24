/*
 * Copyright © 2014 Intel Corporation
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
 * @file glamor_vbo.c
 *
 * Helpers for managing streamed vertex buffers used in glamor.
 */

#include "glamor_priv.h"

/** Default size of the VBO, in bytes.
 *
 * If a single request is larger than this size, we'll resize the VBO
 * and return an appropriate mapping, but we'll resize back down after
 * that to avoid hogging that memory forever.  We don't anticipate
 * normal usage actually requiring larger VBO sizes.
 */
#define GLAMOR_VBO_SIZE (512 * 1024)

/**
 * Returns a pointer to @size bytes of VBO storage, which should be
 * accessed by the GL using vbo_offset within the VBO.
 */
void *
glamor_get_vbo_space(ScreenPtr screen, unsigned size, char **vbo_offset)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    void *data;

    glamor_make_current(glamor_priv);

    glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

    if (glamor_priv->has_buffer_storage) {
        if (glamor_priv->vbo_size < glamor_priv->vbo_offset + size) {
            if (glamor_priv->vbo_size)
                glUnmapBuffer(GL_ARRAY_BUFFER);

            if (size > glamor_priv->vbo_size) {
                glamor_priv->vbo_size = MAX(GLAMOR_VBO_SIZE, size);

                /* We aren't allowed to resize glBufferStorage()
                 * buffers, so we need to gen a new one.
                 */
                glDeleteBuffers(1, &glamor_priv->vbo);
                glGenBuffers(1, &glamor_priv->vbo);
                glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

                assert(glGetError() == GL_NO_ERROR);
                glBufferStorage(GL_ARRAY_BUFFER, glamor_priv->vbo_size, NULL,
                                GL_MAP_WRITE_BIT |
                                GL_MAP_PERSISTENT_BIT |
                                GL_MAP_COHERENT_BIT);

                if (glGetError() != GL_NO_ERROR) {
                    /* If the driver failed our coherent mapping, fall
                     * back to the ARB_mbr path.
                     */
                    glamor_priv->has_buffer_storage = false;
                    glamor_priv->vbo_size = 0;

                    return glamor_get_vbo_space(screen, size, vbo_offset);
                }
            }

            glamor_priv->vbo_offset = 0;
            glamor_priv->vb = glMapBufferRange(GL_ARRAY_BUFFER,
                                               0, glamor_priv->vbo_size,
                                               GL_MAP_WRITE_BIT |
                                               GL_MAP_INVALIDATE_BUFFER_BIT |
                                               GL_MAP_PERSISTENT_BIT |
                                               GL_MAP_COHERENT_BIT);
        }
        *vbo_offset = (void *)(uintptr_t)glamor_priv->vbo_offset;
        data = glamor_priv->vb + glamor_priv->vbo_offset;
        glamor_priv->vbo_offset += size;
    } else if (glamor_priv->has_map_buffer_range) {
        /* Avoid GL errors on GL 4.5 / ES 3.0 with mapping size == 0,
         * which callers may sometimes pass us (for example, if
         * clipping leads to zero rectangles left).  Prior to that
         * version, Mesa would sometimes throw errors on unmapping a
         * zero-size mapping.
         */
        if (size == 0)
            return NULL;

        if (glamor_priv->vbo_size < glamor_priv->vbo_offset + size) {
            glamor_priv->vbo_size = MAX(GLAMOR_VBO_SIZE, size);
            glamor_priv->vbo_offset = 0;
            glBufferData(GL_ARRAY_BUFFER,
                         glamor_priv->vbo_size, NULL, GL_STREAM_DRAW);
        }

        data = glMapBufferRange(GL_ARRAY_BUFFER,
                                glamor_priv->vbo_offset,
                                size,
                                GL_MAP_WRITE_BIT |
                                GL_MAP_UNSYNCHRONIZED_BIT |
                                GL_MAP_INVALIDATE_RANGE_BIT);
        *vbo_offset = (char *)(uintptr_t)glamor_priv->vbo_offset;
        glamor_priv->vbo_offset += size;
        glamor_priv->vbo_mapped = TRUE;
    } else {
        /* Return a pointer to the statically allocated non-VBO
         * memory. We'll upload it through glBufferData() later.
         */
        if (glamor_priv->vbo_size < size) {
            glamor_priv->vbo_size = MAX(GLAMOR_VBO_SIZE, size);
            free(glamor_priv->vb);
            glamor_priv->vb = xnfalloc(glamor_priv->vbo_size);
        }
        *vbo_offset = NULL;
        /* We point to the start of glamor_priv->vb every time, and
         * the vbo_offset determines the size of the glBufferData().
         */
        glamor_priv->vbo_offset = size;
        data = glamor_priv->vb;
    }

    return data;
}

void
glamor_put_vbo_space(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);

    if (glamor_priv->has_buffer_storage) {
        /* If we're in the ARB_buffer_storage path, we have a
         * persistent mapping, so we can leave it around until we
         * reach the end of the buffer.
         */
    } else if (glamor_priv->has_map_buffer_range) {
        if (glamor_priv->vbo_mapped) {
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glamor_priv->vbo_mapped = FALSE;
        }
    } else {
        glBufferData(GL_ARRAY_BUFFER, glamor_priv->vbo_offset,
                     glamor_priv->vb, GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
glamor_init_vbo(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);

    glGenBuffers(1, &glamor_priv->vbo);
    glGenVertexArrays(1, &glamor_priv->vao);
    glBindVertexArray(glamor_priv->vao);
}

void
glamor_fini_vbo(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);

    glDeleteVertexArrays(1, &glamor_priv->vao);
    glamor_priv->vao = 0;
    if (!glamor_priv->has_map_buffer_range)
        free(glamor_priv->vb);
}
