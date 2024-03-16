/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
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

#if !defined( _INDIRECT_REQSIZE_H_ )
#define _INDIRECT_REQSIZE_H_

#include <X11/Xfuncproto.h>

#if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#define PURE __attribute__((pure))
#else
#define PURE
#endif

extern PURE _X_HIDDEN int __glXCallListsReqSize(const GLbyte * pc, Bool swap,
                                                int reqlen);
extern PURE _X_HIDDEN int __glXBitmapReqSize(const GLbyte * pc, Bool swap,
                                             int reqlen);
extern PURE _X_HIDDEN int __glXFogfvReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXFogivReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXLightfvReqSize(const GLbyte * pc, Bool swap,
                                              int reqlen);
extern PURE _X_HIDDEN int __glXLightivReqSize(const GLbyte * pc, Bool swap,
                                              int reqlen);
extern PURE _X_HIDDEN int __glXLightModelfvReqSize(const GLbyte * pc, Bool swap,
                                                   int reqlen);
extern PURE _X_HIDDEN int __glXLightModelivReqSize(const GLbyte * pc, Bool swap,
                                                   int reqlen);
extern PURE _X_HIDDEN int __glXMaterialfvReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXMaterialivReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXPolygonStippleReqSize(const GLbyte * pc,
                                                     Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexParameterfvReqSize(const GLbyte * pc,
                                                     Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexParameterivReqSize(const GLbyte * pc,
                                                     Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexImage1DReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXTexImage2DReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXTexEnvfvReqSize(const GLbyte * pc, Bool swap,
                                               int reqlen);
extern PURE _X_HIDDEN int __glXTexEnvivReqSize(const GLbyte * pc, Bool swap,
                                               int reqlen);
extern PURE _X_HIDDEN int __glXTexGendvReqSize(const GLbyte * pc, Bool swap,
                                               int reqlen);
extern PURE _X_HIDDEN int __glXTexGenfvReqSize(const GLbyte * pc, Bool swap,
                                               int reqlen);
extern PURE _X_HIDDEN int __glXTexGenivReqSize(const GLbyte * pc, Bool swap,
                                               int reqlen);
extern PURE _X_HIDDEN int __glXMap1dReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXMap1fReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXMap2dReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXMap2fReqSize(const GLbyte * pc, Bool swap,
                                            int reqlen);
extern PURE _X_HIDDEN int __glXPixelMapfvReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXPixelMapuivReqSize(const GLbyte * pc, Bool swap,
                                                  int reqlen);
extern PURE _X_HIDDEN int __glXPixelMapusvReqSize(const GLbyte * pc, Bool swap,
                                                  int reqlen);
extern PURE _X_HIDDEN int __glXDrawPixelsReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXDrawArraysReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXPrioritizeTexturesReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexSubImage1DReqSize(const GLbyte * pc,
                                                    Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexSubImage2DReqSize(const GLbyte * pc,
                                                    Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXColorTableReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXColorTableParameterfvReqSize(const GLbyte * pc,
                                                            Bool swap,
                                                            int reqlen);
extern PURE _X_HIDDEN int __glXColorTableParameterivReqSize(const GLbyte * pc,
                                                            Bool swap,
                                                            int reqlen);
extern PURE _X_HIDDEN int __glXColorSubTableReqSize(const GLbyte * pc,
                                                    Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXConvolutionFilter1DReqSize(const GLbyte * pc,
                                                          Bool swap,
                                                          int reqlen);
extern PURE _X_HIDDEN int __glXConvolutionFilter2DReqSize(const GLbyte * pc,
                                                          Bool swap,
                                                          int reqlen);
extern PURE _X_HIDDEN int __glXConvolutionParameterfvReqSize(const GLbyte * pc,
                                                             Bool swap,
                                                             int reqlen);
extern PURE _X_HIDDEN int __glXConvolutionParameterivReqSize(const GLbyte * pc,
                                                             Bool swap,
                                                             int reqlen);
extern PURE _X_HIDDEN int __glXSeparableFilter2DReqSize(const GLbyte * pc,
                                                        Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXTexImage3DReqSize(const GLbyte * pc, Bool swap,
                                                 int reqlen);
extern PURE _X_HIDDEN int __glXTexSubImage3DReqSize(const GLbyte * pc,
                                                    Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexImage1DReqSize(const GLbyte * pc,
                                                           Bool swap,
                                                           int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexImage2DReqSize(const GLbyte * pc,
                                                           Bool swap,
                                                           int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexImage3DReqSize(const GLbyte * pc,
                                                           Bool swap,
                                                           int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexSubImage1DReqSize(const GLbyte * pc,
                                                              Bool swap,
                                                              int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexSubImage2DReqSize(const GLbyte * pc,
                                                              Bool swap,
                                                              int reqlen);
extern PURE _X_HIDDEN int __glXCompressedTexSubImage3DReqSize(const GLbyte * pc,
                                                              Bool swap,
                                                              int reqlen);
extern PURE _X_HIDDEN int __glXPointParameterfvReqSize(const GLbyte * pc,
                                                       Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXPointParameterivReqSize(const GLbyte * pc,
                                                       Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXDrawBuffersReqSize(const GLbyte * pc, Bool swap,
                                                  int reqlen);
extern PURE _X_HIDDEN int __glXProgramStringARBReqSize(const GLbyte * pc,
                                                       Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXDeleteFramebuffersReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXDeleteRenderbuffersReqSize(const GLbyte * pc,
                                                          Bool swap,
                                                          int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs1dvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs1fvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs1svNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs2dvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs2fvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs2svNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs3dvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs3fvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs3svNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs4dvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs4fvNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs4svNVReqSize(const GLbyte * pc,
                                                         Bool swap, int reqlen);
extern PURE _X_HIDDEN int __glXVertexAttribs4ubvNVReqSize(const GLbyte * pc,
                                                          Bool swap,
                                                          int reqlen);

#undef PURE

#endif                          /* !defined( _INDIRECT_REQSIZE_H_ ) */
