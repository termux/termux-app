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

#if !defined( _INDIRECT_SIZE_GET_H_ )
#define _INDIRECT_SIZE_GET_H_

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

extern _X_INTERNAL PURE FASTCALL GLint __glGetBooleanv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetDoublev_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetFloatv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetIntegerv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetLightfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetLightiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetMaterialfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetMaterialiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexEnvfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexEnviv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexGendv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexGenfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexGeniv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexLevelParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetTexLevelParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetPointerv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glGetColorTableParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glGetColorTableParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glGetConvolutionParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glGetConvolutionParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetHistogramParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetHistogramParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetMinmaxParameterfv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetMinmaxParameteriv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetQueryObjectiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetQueryObjectuiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetQueryiv_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint __glGetProgramivARB_size(GLenum);
extern _X_INTERNAL PURE FASTCALL GLint
__glGetFramebufferAttachmentParameteriv_size(GLenum);

#undef PURE
#undef FASTCALL

#endif                          /* !defined( _INDIRECT_SIZE_GET_H_ ) */
