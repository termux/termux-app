/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _INDIRECT_SIZE_H_ )
#define _INDIRECT_SIZE_H_

/**
 * \file
 * Prototypes for functions used to determine the number of data elements in
 * various GLX protocol messages.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <X11/Xfuncproto.h>

#if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#define PURE __attribute__((pure))
#else
#define PURE
#endif

#if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#define FASTCALL __attribute__((fastcall))
#else
#define FASTCALL
#endif

extern _X_INTERNAL PURE FASTCALL GLint __glCallLists_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glFogfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glFogiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glLightfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glLightiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glLightModelfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glLightModeliv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMaterialfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMaterialiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexEnvfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexEnviv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexGendv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexGenfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glTexGeniv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMap1d_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMap1f_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMap2d_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glMap2f_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glColorTableParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glColorTableParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glConvolutionParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glConvolutionParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glPointParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glPointParameteriv_size(GLenum);

#undef PURE
#undef FASTCALL

#endif /* !defined( _INDIRECT_SIZE_H_ ) */
