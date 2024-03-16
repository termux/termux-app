/*
 * File: glwindows.h
 * Purpose: Header for GLX implementation using native Windows OpenGL library
 *
 * Authors: Alexander Gottwald
 *          Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 * Copyright (c) Alexander Gottwald 2004
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

#ifndef GLWINDOWS_H
#define GLWINDOWS_H

#include <GL/gl.h>

typedef struct {
    unsigned int enableDebug:1;
    unsigned int enableTrace:1;
    unsigned int dumpPFD:1;
    unsigned int dumpHWND:1;
    unsigned int dumpDC:1;
    unsigned int enableGLcallTrace:1;
    unsigned int enableWGLcallTrace:1;
} glxWinDebugSettingsRec;

extern glxWinDebugSettingsRec glxWinDebugSettings;

void glxWinPushNativeProvider(void);
void glAddSwapHintRectWINWrapper(GLint x, GLint y, GLsizei width, GLsizei height);
int glWinSelectImplementation(int native);

#if 1
#define GLWIN_TRACE_MSG(msg, args...) if (glxWinDebugSettings.enableTrace) ErrorF(msg " [%s:%d]\n" , ##args , __FUNCTION__, __LINE__ )
#define GLWIN_DEBUG_MSG(msg, args...) if (glxWinDebugSettings.enableDebug) ErrorF(msg " [%s:%d]\n" , ##args , __FUNCTION__, __LINE__ )
#else
#define GLWIN_TRACE_MSG(a, ...)
#define GLWIN_DEBUG_MSG(a, ...)
#endif

#endif
