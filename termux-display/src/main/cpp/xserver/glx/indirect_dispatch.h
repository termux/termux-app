/* DO NOT EDIT - This file generated automatically by glX_proto_recv.py (from Mesa) script */

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

#if !defined( _INDIRECT_DISPATCH_H_ )
#define _INDIRECT_DISPATCH_H_

#include <X11/Xfuncproto.h>

struct __GLXclientStateRec;

extern _X_HIDDEN void __glXDisp_MapGrid1d(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MapGrid1d(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MapGrid1f(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MapGrid1f(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_NewList(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_NewList(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN void __glXDisp_LoadIdentity(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LoadIdentity(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ConvolutionFilter1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionFilter1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord1iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord1iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4ubvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4ubvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Histogram(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Histogram(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetMapfv(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMapfv(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_RasterPos4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PolygonStipple(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PolygonStipple(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord1dv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetPixelMapfv(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetPixelMapfv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_Color3uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3uiv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_IsEnabled(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsEnabled(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib4svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalCoord2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalCoord2fv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DestroyPixmap(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyPixmap(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_FramebufferTexture1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FramebufferTexture1D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetMapiv(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMapiv(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDisp_SwapBuffers(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_SwapBuffers(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_Indexubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Indexubv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_Render(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_Render(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_TexImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexImage3D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_MakeContextCurrent(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDispSwap_MakeContextCurrent(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetFBConfigs(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetFBConfigs(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib1sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LightModeliv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LightModeliv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs1dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs1dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Normal3bv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Normal3bv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_VendorPrivate(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_VendorPrivate(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_TexGeniv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGeniv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RenderbufferStorage(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RenderbufferStorage(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyConvolutionFilter1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyConvolutionFilter1D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenQueries(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenQueries(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_BlendColor(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlendColor(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CompressedTexImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Scalef(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Scalef(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Normal3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Normal3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PassThrough(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PassThrough(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Viewport(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Viewport(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyTexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyTexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_DepthRange(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DepthRange(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetQueryiv(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetQueryiv(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_ResetHistogram(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ResetHistogram(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CompressedTexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Nbv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Nbv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs2svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs2svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetConvolutionParameteriv(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionParameteriv(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetConvolutionParameterivEXT(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionParameterivEXT(struct
                                                                __GLXclientStateRec
                                                                *, GLbyte *);
extern _X_HIDDEN void __glXDisp_Vertex2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex2dv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetVisualConfigs(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetVisualConfigs(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN void __glXDisp_DeleteRenderbuffers(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DeleteRenderbuffers(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord1fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord1fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord3iv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CopyContext(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CopyContext(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib4usv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4usv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PointSize(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PointSize(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PopName(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PopName(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Nusv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Nusv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ClampColor(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClampColor(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexEnvfv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexEnvfv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_LineStipple(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LineStipple(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexEnvi(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexEnvi(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetClipPlane(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetClipPlane(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttribs3dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs3dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs4fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs4fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Scaled(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Scaled(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CallLists(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CallLists(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_AlphaFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_AlphaFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Rotated(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rotated(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_ReadPixels(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_ReadPixels(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_EdgeFlagv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EdgeFlagv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CompressedTexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexParameterf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexParameterf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexParameteri(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexParameteri(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DestroyContext(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyContext(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_DrawPixels(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DrawPixels(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord3sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenLists(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenLists(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_MapGrid2d(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MapGrid2d(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MapGrid2f(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MapGrid2f(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Scissor(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Scissor(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Fogf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Fogf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4usv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4usv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Fogi(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Fogi(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PixelMapfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelMapfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3usv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3usv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord2iv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_AreTexturesResident(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN int __glXDispSwap_AreTexturesResident(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDisp_AreTexturesResidentEXT(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_AreTexturesResidentEXT(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_Color3bv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3bv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib2fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2fvARB(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramLocalParameterfvARB(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramLocalParameterfvARB(struct
                                                                 __GLXclientStateRec
                                                                 *, GLbyte *);
extern _X_HIDDEN void __glXDisp_ColorTable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorTable(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Accum(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Accum(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexImage(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexImage(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_ConvolutionFilter2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionFilter2D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_Finish(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_Finish(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_ClearStencil(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClearStencil(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs4ubvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs4ubvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ConvolutionParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord1fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord1fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ProgramEnvParameter4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ProgramEnvParameter4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ClearIndex(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClearIndex(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LoadMatrixd(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LoadMatrixd(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PushMatrix(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PushMatrix(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ConvolutionParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionParameterfv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexGendv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexGendv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_EndList(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_EndList(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN void __glXDisp_EvalCoord1fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalCoord1fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalMesh2(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalMesh2(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs3fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs3fvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramEnvParameterdvARB(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramEnvParameterdvARB(struct
                                                               __GLXclientStateRec
                                                               *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetFBConfigsSGIX(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetFBConfigsSGIX(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN void __glXDisp_BindFramebuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BindFramebuffer(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateNewContext(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateNewContext(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmax(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmax(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmaxEXT(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmaxEXT(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_BlendFuncSeparate(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlendFuncSeparate(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Normal3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Normal3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ProgramEnvParameter4dvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ProgramEnvParameter4dvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_End(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_End(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs3svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs3svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs2dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs2dvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateContextAttribsARB(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateContextAttribsARB(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN void __glXDisp_BindTexture(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BindTexture(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexSubImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexGenfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGenfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_DrawBuffers(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DrawBuffers(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateContextWithConfigSGIX(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateContextWithConfigSGIX(struct
                                                               __GLXclientStateRec
                                                               *, GLbyte *);
extern _X_HIDDEN int __glXDisp_CopySubBufferMESA(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CopySubBufferMESA(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN void __glXDisp_BlendEquation(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlendEquation(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetError(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetError(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_TexCoord3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Indexdv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Indexdv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PushName(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PushName(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1dv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateGLXPbufferSGIX(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateGLXPbufferSGIX(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDisp_IsRenderbuffer(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsRenderbuffer(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_DepthMask(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DepthMask(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4iv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetMaterialiv(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMaterialiv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_StencilOp(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_StencilOp(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_FramebufferTextureLayer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FramebufferTextureLayer(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Ortho(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Ortho(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexEnvfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexEnvfv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_QueryServerString(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_QueryServerString(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN void __glXDisp_LoadMatrixf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LoadMatrixf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4bv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4bv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetCompressedTexImage(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetCompressedTexImage(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib2fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ProgramLocalParameter4dvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ProgramLocalParameter4dvARB(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DeleteLists(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DeleteLists(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_LogicOp(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LogicOp(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RenderbufferStorageMultisample(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RenderbufferStorageMultisample(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ActiveTexture(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ActiveTexture(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3bv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3bv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_WaitX(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_WaitX(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib1dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1dvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenTextures(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenTextures(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_GenTexturesEXT(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenTexturesEXT(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDisp_GetDrawableAttributes(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetDrawableAttributes(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_RasterPos2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_DrawBuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DrawBuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord1sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord1sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateGLXPixmapWithConfigSGIX(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateGLXPixmapWithConfigSGIX(struct
                                                                 __GLXclientStateRec
                                                                 *, GLbyte *);
extern _X_HIDDEN void __glXDisp_DepthFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DepthFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PixelMapusv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelMapusv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_BlendFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlendFunc(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_WaitGL(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_WaitGL(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_CompressedTexImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexImage2D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_Flush(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_Flush(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_Color4uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord1sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord1sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PushAttrib(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PushAttrib(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DestroyPbuffer(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyPbuffer(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_TexParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexParameteriv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_QueryExtensionsString(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_QueryExtensionsString(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_RasterPos3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyTexSubImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyTexSubImage3D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetColorTable(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTable(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDisp_GetColorTableSGI(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTableSGI(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN void __glXDisp_Indexiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Indexiv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateContext(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateContext(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_CopyColorTable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyColorTable(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PointParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PointParameterfv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetHistogramParameterfv(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogramParameterfv(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetHistogramParameterfvEXT(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogramParameterfvEXT(struct
                                                              __GLXclientStateRec
                                                              *, GLbyte *);
extern _X_HIDDEN void __glXDisp_Frustum(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Frustum(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetString(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetString(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDisp_CreateGLXPixmap(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateGLXPixmap(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN void __glXDisp_TexEnvf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexEnvf(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenProgramsARB(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenProgramsARB(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDisp_DeleteTextures(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DeleteTextures(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDisp_DeleteTexturesEXT(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DeleteTexturesEXT(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetTexLevelParameteriv(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexLevelParameteriv(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_ClearAccum(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClearAccum(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_QueryVersion(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_QueryVersion(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_TexCoord4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_FramebufferTexture3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FramebufferTexture3D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetDrawableAttributesSGIX(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetDrawableAttributesSGIX(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN void __glXDisp_ColorTableParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorTableParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyTexImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyTexImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Lightfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Lightfv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetFramebufferAttachmentParameteriv(struct
                                                                   __GLXclientStateRec
                                                                   *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetFramebufferAttachmentParameteriv(struct
                                                                       __GLXclientStateRec
                                                                       *,
                                                                       GLbyte
                                                                       *);
extern _X_HIDDEN void __glXDisp_ClearDepth(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClearDepth(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ColorSubTable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorSubTable(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4fv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreatePixmap(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreatePixmap(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_Lightiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Lightiv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexParameteriv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexParameteriv(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_IsQuery(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsQuery(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN void __glXDisp_Rectdv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rectdv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Materialiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Materialiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3fvEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3fvEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PolygonMode(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PolygonMode(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Niv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Niv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramStringARB(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramStringARB(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN void __glXDisp_TexGeni(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGeni(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexGenf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGenf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexGend(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGend(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetPolygonStipple(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetPolygonStipple(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib2svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs1fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs1fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib2dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib2dvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DestroyWindow(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyWindow(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_Color4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PixelZoom(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelZoom(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ColorTableParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorTableParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PixelMapuiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelMapuiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3dv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_IsTexture(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsTexture(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDisp_IsTextureEXT(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsTextureEXT(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib4fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_BeginQuery(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BeginQuery(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_SetClientInfo2ARB(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_SetClientInfo2ARB(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMapdv(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMapdv(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_MultiTexCoord3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord3iv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DestroyGLXPixmap(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyGLXPixmap(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDisp_PixelStoref(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_PixelStoref(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_PrioritizeTextures(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PrioritizeTextures(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_PixelStorei(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_PixelStorei(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_DestroyGLXPbufferSGIX(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DestroyGLXPbufferSGIX(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN void __glXDisp_EvalCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ColorMaterial(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorMaterial(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttribs1svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs1svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib1fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1fvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetSeparableFilter(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetSeparableFilter(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetSeparableFilterEXT(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetSeparableFilterEXT(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN int __glXDisp_FeedbackBuffer(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_FeedbackBuffer(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_RasterPos2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_FrontFace(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FrontFace(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_RenderLarge(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_RenderLarge(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_PolygonOffset(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PolygonOffset(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Normal3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Normal3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Lightf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Lightf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MatrixMode(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MatrixMode(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetPixelMapusv(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetPixelMapusv(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_Lighti(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Lighti(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenFramebuffers(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenFramebuffers(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN int __glXDisp_IsFramebuffer(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsFramebuffer(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDisp_ChangeDrawableAttributesSGIX(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_ChangeDrawableAttributesSGIX(struct
                                                                __GLXclientStateRec
                                                                *, GLbyte *);
extern _X_HIDDEN void __glXDisp_BlendEquationSeparate(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlendEquationSeparate(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreatePbuffer(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreatePbuffer(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDisp_GetDoublev(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetDoublev(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_MultMatrixd(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultMatrixd(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultMatrixf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultMatrixf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CompressedTexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib3fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ClearColor(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClearColor(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_IsDirect(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsDirect(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib1svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PointParameteri(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PointParameteri(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PointParameterf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PointParameterf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexEnviv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexEnviv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexSubImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexSubImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4iv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_SwapIntervalSGI(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_SwapIntervalSGI(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN int __glXDisp_GetColorTableParameterfv(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTableParameterfv(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetColorTableParameterfvSGI(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTableParameterfvSGI(struct
                                                               __GLXclientStateRec
                                                               *, GLbyte *);
extern _X_HIDDEN void __glXDisp_FramebufferTexture2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FramebufferTexture2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Bitmap(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Bitmap(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexLevelParameterfv(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexLevelParameterfv(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN int __glXDisp_CheckFramebufferStatus(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CheckFramebufferStatus(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_Vertex2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex2sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetIntegerv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetIntegerv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_BindProgramARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BindProgramARB(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramEnvParameterfvARB(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramEnvParameterfvARB(struct
                                                               __GLXclientStateRec
                                                               *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib3svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3svNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexEnviv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexEnviv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_VendorPrivateWithReply(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_VendorPrivateWithReply(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN void __glXDisp_SeparableFilter2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SeparableFilter2D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetQueryObjectuiv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetQueryObjectuiv(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN void __glXDisp_Map1d(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Map1d(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Map1f(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Map1f(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexImage2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexImage2D(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_ChangeDrawableAttributes(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDispSwap_ChangeDrawableAttributes(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmaxParameteriv(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmaxParameteriv(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmaxParameterivEXT(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmaxParameterivEXT(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN void __glXDisp_PixelTransferf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelTransferf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyTexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyTexImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Fogiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Fogiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EndQuery(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EndQuery(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PixelTransferi(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PixelTransferi(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib3fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Clear(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Clear(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ReadBuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ReadBuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ConvolutionParameteri(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionParameteri(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LightModeli(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LightModeli(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ListBase(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ListBase(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ConvolutionParameterf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ConvolutionParameterf(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetColorTableParameteriv(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTableParameteriv(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetColorTableParameterivSGI(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetColorTableParameterivSGI(struct
                                                               __GLXclientStateRec
                                                               *, GLbyte *);
extern _X_HIDDEN int __glXDisp_ReleaseTexImageEXT(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDispSwap_ReleaseTexImageEXT(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN void __glXDisp_CallList(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CallList(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_GenerateMipmap(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_GenerateMipmap(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Rectiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rectiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord1iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord1iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex2fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex3sv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetQueryObjectiv(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetQueryObjectiv(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDisp_SetClientInfoARB(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_SetClientInfoARB(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDisp_BindTexImageEXT(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_BindTexImageEXT(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN void __glXDisp_ProgramLocalParameter4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ProgramLocalParameter4fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalMesh1(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalMesh1(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CompressedTexSubImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CompressedTexSubImage3D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex2iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LineWidth(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LineWidth(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexGendv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexGendv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ResetMinmax(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ResetMinmax(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetConvolutionParameterfv(struct
                                                         __GLXclientStateRec *,
                                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionParameterfv(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetConvolutionParameterfvEXT(struct
                                                            __GLXclientStateRec
                                                            *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionParameterfvEXT(struct
                                                                __GLXclientStateRec
                                                                *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMaterialfv(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMaterialfv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN void __glXDisp_WindowPos3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_WindowPos3fv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DeleteProgramsARB(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DeleteProgramsARB(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDisp_UseXFont(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_UseXFont(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN void __glXDisp_ShadeModel(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ShadeModel(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Materialfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Materialfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_FogCoordfvEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FogCoordfvEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_DrawArrays(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DrawArrays(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SampleCoverage(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SampleCoverage(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color3iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4ubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4ubv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramLocalParameterdvARB(struct
                                                             __GLXclientStateRec
                                                             *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramLocalParameterdvARB(struct
                                                                 __GLXclientStateRec
                                                                 *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetHistogramParameteriv(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogramParameteriv(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetHistogramParameterivEXT(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogramParameterivEXT(struct
                                                              __GLXclientStateRec
                                                              *, GLbyte *);
extern _X_HIDDEN void __glXDisp_PointParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PointParameteriv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Rotatef(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rotatef(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetProgramivARB(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetProgramivARB(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN void __glXDisp_BindRenderbuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BindRenderbuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalPoint2(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalPoint2(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalPoint1(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalPoint1(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PopMatrix(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PopMatrix(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_DeleteFramebuffers(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_DeleteFramebuffers(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_MakeCurrentReadSGI(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN int __glXDispSwap_MakeCurrentReadSGI(struct __GLXclientStateRec
                                                      *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetTexGeniv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexGeniv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_MakeCurrent(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_MakeCurrent(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_FramebufferRenderbuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FramebufferRenderbuffer(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_IsProgramARB(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsProgramARB(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib4uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4uiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Nsv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Nsv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Map2d(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Map2d(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Map2f(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Map2f(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ProgramStringARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ProgramStringARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4bv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4bv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetConvolutionFilter(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionFilter(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDisp_GetConvolutionFilterEXT(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetConvolutionFilterEXT(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttribs4dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs4dvNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexGenfv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexGenfv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDisp_GetHistogram(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogram(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDisp_GetHistogramEXT(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetHistogramEXT(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN void __glXDisp_ActiveStencilFaceEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ActiveStencilFaceEXT(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Materialf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Materialf(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Materiali(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Materiali(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Indexsv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Indexsv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib1fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib1fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LightModelfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LightModelfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord2dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_EvalCoord1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_EvalCoord1dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Translated(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Translated(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Translatef(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Translatef(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_StencilMask(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_StencilMask(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_CreateWindow(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_CreateWindow(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDisp_GetLightiv(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetLightiv(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDisp_IsList(struct __GLXclientStateRec *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_IsList(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDisp_RenderMode(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_RenderMode(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_LoadName(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LoadName(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyTexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyTexSubImage1D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CullFace(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CullFace(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_QueryContextInfoEXT(struct __GLXclientStateRec *,
                                                   GLbyte *);
extern _X_HIDDEN int __glXDispSwap_QueryContextInfoEXT(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttribs2fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs2fvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_StencilFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_StencilFunc(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyPixels(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyPixels(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Rectsv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rectsv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyConvolutionFilter2D(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyConvolutionFilter2D(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexParameterfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Nubv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Nubv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_ClipPlane(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ClipPlane(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3usv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3usv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord3dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord3dv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetPixelMapuiv(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetPixelMapuiv(struct __GLXclientStateRec *,
                                                  GLbyte *);
extern _X_HIDDEN void __glXDisp_Indexfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Indexfv(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_QueryContext(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_QueryContext(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_MultiTexCoord3fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord3fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_BlitFramebuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_BlitFramebuffer(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_IndexMask(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_IndexMask(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetFloatv(struct __GLXclientStateRec *,
                                         GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetFloatv(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN void __glXDisp_TexCoord3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_FogCoorddv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_FogCoorddv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_PopAttrib(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_PopAttrib(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Fogfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Fogfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_InitNames(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_InitNames(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Normal3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Normal3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Minmax(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Minmax(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_DeleteQueries(struct __GLXclientStateRec *,
                                             GLbyte *);
extern _X_HIDDEN int __glXDispSwap_DeleteQueries(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDisp_GetBooleanv(struct __GLXclientStateRec *,
                                           GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetBooleanv(struct __GLXclientStateRec *,
                                               GLbyte *);
extern _X_HIDDEN void __glXDisp_Hint(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Hint(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Color4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Color4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_CopyColorSubTable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_CopyColorSubTable(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_VertexAttrib3dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib3dvNV(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_TexCoord4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_TexCoord4dv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Begin(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Begin(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_ClientInfo(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_ClientInfo(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_Rectfv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Rectfv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_LightModelf(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_LightModelf(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetTexParameterfv(struct __GLXclientStateRec *,
                                                 GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetTexParameterfv(struct __GLXclientStateRec
                                                     *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetLightfv(struct __GLXclientStateRec *,
                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetLightfv(struct __GLXclientStateRec *,
                                              GLbyte *);
extern _X_HIDDEN void __glXDisp_Disable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Disable(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord2fvARB(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord2fvARB(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_SelectBuffer(struct __GLXclientStateRec *,
                                            GLbyte *);
extern _X_HIDDEN int __glXDispSwap_SelectBuffer(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN void __glXDisp_ColorMask(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_ColorMask(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_RasterPos4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_RasterPos4iv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Enable(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Enable(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GetRenderbufferParameteriv(struct
                                                          __GLXclientStateRec *,
                                                          GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetRenderbufferParameteriv(struct
                                                              __GLXclientStateRec
                                                              *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttribs4svNV(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttribs4svNV(GLbyte * pc);
extern _X_HIDDEN int __glXDisp_GenRenderbuffers(struct __GLXclientStateRec *,
                                                GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GenRenderbuffers(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmaxParameterfv(struct __GLXclientStateRec
                                                    *, GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmaxParameterfv(struct
                                                        __GLXclientStateRec *,
                                                        GLbyte *);
extern _X_HIDDEN int __glXDisp_GetMinmaxParameterfvEXT(struct
                                                       __GLXclientStateRec *,
                                                       GLbyte *);
extern _X_HIDDEN int __glXDispSwap_GetMinmaxParameterfvEXT(struct
                                                           __GLXclientStateRec
                                                           *, GLbyte *);
extern _X_HIDDEN void __glXDisp_VertexAttrib4Nuiv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_VertexAttrib4Nuiv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_Vertex3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_Vertex3fv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_SecondaryColor3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_SecondaryColor3sv(GLbyte * pc);
extern _X_HIDDEN void __glXDisp_MultiTexCoord2sv(GLbyte * pc);
extern _X_HIDDEN void __glXDispSwap_MultiTexCoord2sv(GLbyte * pc);

#endif                          /* !defined( _INDIRECT_DISPATCH_H_ ) */
