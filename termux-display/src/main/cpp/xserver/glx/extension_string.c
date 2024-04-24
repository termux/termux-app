/*
 * (C) Copyright IBM Corporation 2002-2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file extension_string.c
 * Routines to manage the GLX extension string and GLX version for AIGLX
 * drivers.  This code is loosely based on src/glx/x11/glxextensions.c from
 * Mesa.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "dix-config.h"
#include "extension_string.h"
#include "opaque.h"

#define SET_BIT(m,b)    (m[ (b) / 8 ] |=  (1U << ((b) % 8)))
#define CLR_BIT(m,b)    (m[ (b) / 8 ] &= ~(1U << ((b) % 8)))
#define IS_SET(m,b)    ((m[ (b) / 8 ] &   (1U << ((b) % 8))) != 0)
#define CONCAT(a,b) a ## b
#define GLX(n) "GLX_" # n, 4 + sizeof( # n ) - 1, CONCAT(n,_bit)
#define VER(a,b)  a, b
#define Y  1
#define N  0
#define EXT_ENABLED(bit,supported) (IS_SET(supported, bit))

struct extension_info {
    const char *const name;
    unsigned name_len;

    unsigned char bit;

    /**
     * This is the lowest version of GLX that "requires" this extension.
     * For example, GLX 1.3 requires SGIX_fbconfig, SGIX_pbuffer, and
     * SGI_make_current_read.  If the extension is not required by any known
     * version of GLX, use 0, 0.
     */
    unsigned char version_major;
    unsigned char version_minor;

    /**
     * Is driver support forced by the ABI?
     */
    unsigned char driver_support;
};

/**
 * List of known GLX Extensions.
 * The last Y/N switch informs whether the support of this extension is always enabled.
 */
static const struct extension_info known_glx_extensions[] = {
/*   GLX_ARB_get_proc_address is implemented on the client. */
    /* *INDENT-OFF* */
    { GLX(ARB_context_flush_control),   VER(0,0), N, },
    { GLX(ARB_create_context),          VER(0,0), N, },
    { GLX(ARB_create_context_no_error), VER(0,0), N, },
    { GLX(ARB_create_context_profile),  VER(0,0), N, },
    { GLX(ARB_create_context_robustness), VER(0,0), N, },
    { GLX(ARB_fbconfig_float),          VER(0,0), N, },
    { GLX(ARB_framebuffer_sRGB),        VER(0,0), N, },
    { GLX(ARB_multisample),             VER(1,4), Y, },

    { GLX(EXT_create_context_es_profile), VER(0,0), N, },
    { GLX(EXT_create_context_es2_profile), VER(0,0), N, },
    { GLX(EXT_fbconfig_packed_float),   VER(0,0), N, },
    { GLX(EXT_framebuffer_sRGB),        VER(0,0), N, },
    { GLX(EXT_get_drawable_type),       VER(0,0), Y, },
    { GLX(EXT_import_context),          VER(0,0), N, },
    { GLX(EXT_libglvnd),                VER(0,0), N, },
    { GLX(EXT_no_config_context),       VER(0,0), N, },
    { GLX(EXT_stereo_tree),             VER(0,0), N, },
    { GLX(EXT_texture_from_pixmap),     VER(0,0), N, },
    { GLX(EXT_visual_info),             VER(0,0), Y, },
    { GLX(EXT_visual_rating),           VER(0,0), Y, },

    { GLX(MESA_copy_sub_buffer),        VER(0,0), N, },
    { GLX(OML_swap_method),             VER(0,0), Y, },
    { GLX(SGI_make_current_read),       VER(1,3), Y, },
    { GLX(SGI_swap_control),            VER(0,0), N, },
    { GLX(SGIS_multisample),            VER(0,0), Y, },
    { GLX(SGIX_fbconfig),               VER(1,3), Y, },
    { GLX(SGIX_pbuffer),                VER(1,3), Y, },
    { GLX(SGIX_visual_select_group),    VER(0,0), Y, },
    { GLX(INTEL_swap_event),            VER(0,0), N, },
    { NULL }
    /* *INDENT-ON* */
};

/**
 * Create a GLX extension string for a set of enable bits.
 *
 * Creates a GLX extension string for the set of bit in \c enable_bits.  This
 * string is then stored in \c buffer if buffer is not \c NULL.  This allows
 * two-pass operation.  On the first pass the caller passes \c NULL for
 * \c buffer, and the function determines how much space is required to store
 * the extension string.  The caller allocates the buffer and calls the
 * function again.
 *
 * \param enable_bits  Bits representing the enabled extensions.
 * \param buffer       Buffer to store the extension string.  May be \c NULL.
 *
 * \return
 * The number of characters in \c buffer that were written to.  If \c buffer
 * is \c NULL, this is the size of buffer that must be allocated by the
 * caller.
 */
int
__glXGetExtensionString(const unsigned char *enable_bits, char *buffer)
{
    unsigned i;
    int length = 0;

    for (i = 0; known_glx_extensions[i].name != NULL; i++) {
        const unsigned bit = known_glx_extensions[i].bit;
        const size_t len = known_glx_extensions[i].name_len;

        if (EXT_ENABLED(bit, enable_bits)) {
            if (buffer != NULL) {
                (void) memcpy(&buffer[length], known_glx_extensions[i].name,
                              len);

                buffer[length + len + 0] = ' ';
                buffer[length + len + 1] = '\0';
            }

            length += len + 1;
        }
    }

    return length + 1;
}

void
__glXEnableExtension(unsigned char *enable_bits, const char *ext)
{
    const size_t ext_name_len = strlen(ext);
    unsigned i;

    for (i = 0; known_glx_extensions[i].name != NULL; i++) {
        if ((ext_name_len == known_glx_extensions[i].name_len)
            && (memcmp(ext, known_glx_extensions[i].name, ext_name_len) == 0)) {
            SET_BIT(enable_bits, known_glx_extensions[i].bit);
            break;
        }
    }
}

void
__glXInitExtensionEnableBits(unsigned char *enable_bits)
{
    unsigned i;

    (void) memset(enable_bits, 0, __GLX_EXT_BYTES);

    for (i = 0; known_glx_extensions[i].name != NULL; i++) {
        if (known_glx_extensions[i].driver_support) {
            SET_BIT(enable_bits, known_glx_extensions[i].bit);
        }
    }

    if (enableIndirectGLX)
        __glXEnableExtension(enable_bits, "GLX_EXT_import_context");
}
