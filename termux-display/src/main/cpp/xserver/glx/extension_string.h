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
 * \file extension_string.h
 * Routines to manage the GLX extension string and GLX version for AIGLX
 * drivers.  This code is loosely based on src/glx/x11/glxextensions.c from
 * Mesa.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef GLX_EXTENSION_STRING_H
#define GLX_EXTENSION_STRING_H

enum {
/*   GLX_ARB_get_proc_address is implemented on the client. */
    ARB_context_flush_control_bit = 0,
    ARB_create_context_bit,
    ARB_create_context_no_error_bit,
    ARB_create_context_profile_bit,
    ARB_create_context_robustness_bit,
    ARB_fbconfig_float_bit,
    ARB_framebuffer_sRGB_bit,
    ARB_multisample_bit,
    EXT_create_context_es_profile_bit,
    EXT_create_context_es2_profile_bit,
    EXT_fbconfig_packed_float_bit,
    EXT_get_drawable_type_bit,
    EXT_import_context_bit,
    EXT_libglvnd_bit,
    EXT_no_config_context_bit,
    EXT_stereo_tree_bit,
    EXT_texture_from_pixmap_bit,
    EXT_visual_info_bit,
    EXT_visual_rating_bit,
    MESA_copy_sub_buffer_bit,
    OML_swap_method_bit,
    SGI_make_current_read_bit,
    SGI_swap_control_bit,
    SGI_video_sync_bit,
    SGIS_multisample_bit,
    SGIX_fbconfig_bit,
    SGIX_pbuffer_bit,
    SGIX_visual_select_group_bit,
    INTEL_swap_event_bit,
    __NUM_GLX_EXTS,
};

/* For extensions which have identical ARB and EXT implementation
 * in GLX area, use one enabling bit for both. */
#define EXT_framebuffer_sRGB_bit ARB_framebuffer_sRGB_bit

#define __GLX_EXT_BYTES ((__NUM_GLX_EXTS + 7) / 8)

extern int __glXGetExtensionString(const unsigned char *enable_bits,
                                   char *buffer);
extern void __glXEnableExtension(unsigned char *enable_bits, const char *ext);
extern void __glXInitExtensionEnableBits(unsigned char *enable_bits);

#endif                          /* GLX_EXTENSION_STRING_H */
