/*
 * File: wgl_ext_api.h
 * Purpose: Wrapper functions for Win32 OpenGL wgl extension functions
 *
 * Authors: Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef wgl_ext_api_h
#define wgl_ext_api_h

#include <GL/wglext.h>

void wglResolveExtensionProcs(void);

/*
  Prototypes for wrapper functions we actually use
  XXX: should be automatically generated as well
*/

const char *wglGetExtensionsStringARBWrapper(HDC hdc);
BOOL wglMakeContextCurrentARBWrapper(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
HDC wglGetCurrentReadDCARBWrapper(VOID);

BOOL wglGetPixelFormatAttribivARBWrapper(HDC hdc,
                                         int iPixelFormat,
                                         int iLayerPlane,
                                         UINT nAttributes,
                                         const int *piAttributes,
                                         int *piValues);

BOOL wglGetPixelFormatAttribfvARBWrapper(HDC hdc,
                                         int iPixelFormat,
                                         int iLayerPlane,
                                         UINT nAttributes,
                                         const int *piAttributes,
                                         FLOAT * pfValues);

BOOL wglChoosePixelFormatARBWrapper(HDC hdc,
                                    const int *piAttribIList,
                                    const FLOAT * pfAttribFList,
                                    UINT nMaxFormats,
                                    int *piFormats, UINT * nNumFormats);

HPBUFFERARB wglCreatePbufferARBWrapper(HDC hDC,
                                       int iPixelFormat,
                                       int iWidth,
                                       int iHeight, const int *piAttribList);

HDC wglGetPbufferDCARBWrapper(HPBUFFERARB hPbuffer);

int wglReleasePbufferDCARBWrapper(HPBUFFERARB hPbuffer, HDC hDC);

BOOL wglDestroyPbufferARBWrapper(HPBUFFERARB hPbuffer);

BOOL wglQueryPbufferARBWrapper(HPBUFFERARB hPbuffer,
                               int iAttribute, int *piValue);

BOOL wglSwapIntervalEXTWrapper(int interval);

int wglGetSwapIntervalEXTWrapper(void);

#endif                          /* wgl_ext_api_h */
