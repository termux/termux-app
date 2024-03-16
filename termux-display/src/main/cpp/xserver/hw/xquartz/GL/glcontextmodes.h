/*
 * (C) Copyright IBM Corporation 2003
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
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glcontextmodes.h
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef GLCONTEXTMODES_H
#define GLCONTEXTMODES_H

#if !defined(IN_MINI_GLX)
extern GLint
_gl_convert_from_x_visual_type(int visualType);
extern GLint
_gl_convert_to_x_visual_type(int visualType);
extern void
_gl_copy_visual_to_context_mode(__GLcontextModes * mode,
                                const __GLXvisualConfig * config);
extern int
_gl_get_context_mode_data(const __GLcontextModes *mode, int attribute,
                          int *value_return);
#endif /* !defined(IN_MINI_GLX) */

extern __GLcontextModes *
_gl_context_modes_create(unsigned count, size_t minimum_size);
extern void
_gl_context_modes_destroy(__GLcontextModes * modes);
extern __GLcontextModes *
_gl_context_modes_find_visual(__GLcontextModes *modes, int vid);
extern __GLcontextModes *
_gl_context_modes_find_fbconfig(__GLcontextModes *modes, int fbid);
extern GLboolean
_gl_context_modes_are_same(const __GLcontextModes * a,
                           const __GLcontextModes * b);

#endif /* GLCONTEXTMODES_H */
