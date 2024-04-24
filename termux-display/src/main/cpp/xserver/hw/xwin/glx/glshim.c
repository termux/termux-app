/*
 * File: glshim.c
 * Purpose: GL shim which redirects to a specified DLL
 *
 * Copyright (c) Jon TURNEY 2013
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

/*
   A GL shim which redirects to a specified DLL

   XWin is statically linked with this, rather than the system libGL, so that
   GL calls can be directed to mesa cygGL-1.dll, or cygnativeGLthunk.dll
   (which contains cdecl-to-stdcall thunks to the native openGL32.dll)
*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#define GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#undef GL_ARB_imaging
#undef GL_VERSION_1_3
#include <GL/glext.h>

#include <X11/Xwindows.h>
#include <os.h>
#include "glwindows.h"
#include <glx/glxserver.h>

extern void *glXGetProcAddressARB(const char *);

static HMODULE hMod = NULL;

/*
  Implement the __glGetProcAddress function by just using GetProcAddress() on the selected DLL
*/
void *glXGetProcAddressARB(const char *symbol)
{
    void *proc;

    /* Default to the mesa GL implementation if one hasn't been selected yet */
    if (!hMod)
        glWinSelectImplementation(0);

    proc = GetProcAddress(hMod, symbol);

    if (glxWinDebugSettings.enableGLcallTrace)
        ErrorF("glXGetProcAddressARB: Resolved '%s' in %p to %p\n", symbol, hMod, proc);

    return proc;
}

/*
  Select a GL implementation DLL
*/
int glWinSelectImplementation(int native)
{
    const char *dllname;

    if (native) {
        dllname = "cygnativeGLthunk.dll";
    }
    else {
        dllname = "cygGL-1.dll";
    }

    hMod = LoadLibraryEx(dllname, NULL, 0);
    if (hMod == NULL) {
        ErrorF("glWinSelectGLimplementation: Could not load '%s'\n", dllname);
        return -1;
    }

    ErrorF("glWinSelectGLimplementation: Loaded '%s'\n", dllname);

    /* Connect __glGetProcAddress() to our implementation of glXGetProcAddressARB() above */
    __glXsetGetProcAddress((glx_gpa_proc)glXGetProcAddressARB);

    return 0;
}

#define RESOLVE_RET(proctype, symbol, retval) \
    proctype proc = (proctype)glXGetProcAddressARB(symbol);   \
    if (proc == NULL) return retval;

#define RESOLVE(proctype, symbol) RESOLVE_RET(proctype, symbol,)
#define RESOLVED_PROC proc

/* Include generated shims for direct linkage to GL functions which are in the ABI */
#include "generated_gl_shim.ic"

/*
  Special wrapper for glAddSwapHintRectWIN for copySubBuffers

  Only used with native GL if the GL_WIN_swap_hint extension is present, so we enable
  GLX_MESA_copy_sub_buffer
*/
typedef void (__stdcall * PFNGLADDSWAPHINTRECTWIN) (GLint x, GLint y,
                                                    GLsizei width,
                                                    GLsizei height);

void
glAddSwapHintRectWINWrapper(GLint x, GLint y, GLsizei width,
                            GLsizei height)
{
    RESOLVE(PFNGLADDSWAPHINTRECTWIN, "glAddSwapHintRectWIN");
    RESOLVED_PROC(x, y, width, height);
}
