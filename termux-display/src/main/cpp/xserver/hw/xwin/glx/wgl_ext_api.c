/*
 * File: wgl_ext_api.c
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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glx/glxserver.h>
#include <glx/glxext.h>
#include <GL/wglext.h>
#include <wgl_ext_api.h>
#include "glwindows.h"

#define RESOLVE_DECL(type) \
    static type type##proc = NULL;

#define PRERESOLVE(type, symbol) \
    type##proc = (type)wglGetProcAddress(symbol);

#define RESOLVE_RET(type, symbol, retval) \
  if (type##proc == NULL) { \
    ErrorF("wglwrap: Can't resolve \"%s\"\n", symbol); \
    __glXErrorCallBack(0); \
    return retval; \
  }

#define RESOLVE(procname, symbol) RESOLVE_RET(procname, symbol,)

#define RESOLVED_PROC(type) type##proc

/*
 * Include generated cdecl wrappers for stdcall WGL functions
 *
 * There are extensions to the wgl*() API as well; again we call
 * these functions by using wglGetProcAddress() to get a pointer
 * to the function, and wrapping it for cdecl/stdcall conversion
 *
 * We arrange to resolve the functions up front, as they need a
 * context to work, as we like to use them to be able to select
 * a context.  Again, this assumption fails badly on multimontor
 * systems...
 */

#include "generated_wgl_wrappers.ic"
