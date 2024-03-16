/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "dispatch_common.h"

static bool first_context_current = false;
static bool already_switched_to_dispatch_table = false;

/**
 * If we can determine the WGL extension support from the current
 * context, then return that, otherwise give the answer that will just
 * send us on to get_proc_address().
 */
bool
epoxy_conservative_has_wgl_extension(const char *ext)
{
    HDC hdc = wglGetCurrentDC();

    if (!hdc)
        return true;

    return epoxy_has_wgl_extension(hdc, ext);
}

bool
epoxy_has_wgl_extension(HDC hdc, const char *ext)
 {
     PFNWGLGETEXTENSIONSSTRINGARBPROC getext;

     getext = (void *)wglGetProcAddress("wglGetExtensionsStringARB");
     if (!getext) {
         fputs("Implementation unexpectedly missing "
               "WGL_ARB_extensions_string.  Probably a libepoxy bug.\n",
               stderr);
         return false;
     }

    return epoxy_extension_in_string(getext(hdc), ext);
}

/**
 * Does the work necessary to update the win32 per-thread dispatch
 * tables when wglMakeCurrent() is called.
 *
 * Right now, we use global function pointers until the second
 * MakeCurrent occurs, at which point we switch to dispatch tables.
 * This could be improved in the future to track a resolved dispatch
 * table per context and reuse it when the context is made current
 * again.
 */
void
epoxy_handle_external_wglMakeCurrent(void)
{
    if (!first_context_current) {
        first_context_current = true;
    } else {
        if (!already_switched_to_dispatch_table) {
            already_switched_to_dispatch_table = true;
            gl_switch_to_dispatch_table();
            wgl_switch_to_dispatch_table();
        }

        gl_init_dispatch_table();
        wgl_init_dispatch_table();
    }
}

/**
 * This global symbol is apparently looked up by Windows when loading
 * a DLL, but it doesn't declare the prototype.
 */
BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved);

BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
    void *data;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        gl_tls_index = TlsAlloc();
        if (gl_tls_index == TLS_OUT_OF_INDEXES)
            return FALSE;
        wgl_tls_index = TlsAlloc();
        if (wgl_tls_index == TLS_OUT_OF_INDEXES)
            return FALSE;

        first_context_current = false;

        /* FALLTHROUGH */

    case DLL_THREAD_ATTACH:
        data = LocalAlloc(LPTR, gl_tls_size);
        TlsSetValue(gl_tls_index, data);

        data = LocalAlloc(LPTR, wgl_tls_size);
        TlsSetValue(wgl_tls_index, data);

        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        data = TlsGetValue(gl_tls_index);
        LocalFree(data);

        data = TlsGetValue(wgl_tls_index);
        LocalFree(data);

        if (reason == DLL_PROCESS_DETACH) {
            TlsFree(gl_tls_index);
            TlsFree(wgl_tls_index);
        }
        break;
    }

    return TRUE;
}

WRAPPER_VISIBILITY (BOOL)
WRAPPER(epoxy_wglMakeCurrent)(HDC hdc, HGLRC hglrc)
{
    BOOL ret = epoxy_wglMakeCurrent_unwrapped(hdc, hglrc);

    epoxy_handle_external_wglMakeCurrent();

    return ret;
}


WRAPPER_VISIBILITY (BOOL)
WRAPPER(epoxy_wglMakeContextCurrentARB)(HDC hDrawDC,
                                        HDC hReadDC,
                                        HGLRC hglrc)
{
    BOOL ret = epoxy_wglMakeContextCurrentARB_unwrapped(hDrawDC, hReadDC,
                                                        hglrc);

    epoxy_handle_external_wglMakeCurrent();

    return ret;
}


WRAPPER_VISIBILITY (BOOL)
WRAPPER(epoxy_wglMakeContextCurrentEXT)(HDC hDrawDC,
                                        HDC hReadDC,
                                        HGLRC hglrc)
{
    BOOL ret = epoxy_wglMakeContextCurrentEXT_unwrapped(hDrawDC, hReadDC,
                                                        hglrc);

    epoxy_handle_external_wglMakeCurrent();

    return ret;
}


WRAPPER_VISIBILITY (BOOL)
WRAPPER(epoxy_wglMakeAssociatedContextCurrentAMD)(HGLRC hglrc)
{
    BOOL ret = epoxy_wglMakeAssociatedContextCurrentAMD_unwrapped(hglrc);

    epoxy_handle_external_wglMakeCurrent();

    return ret;
}

PFNWGLMAKECURRENTPROC epoxy_wglMakeCurrent = epoxy_wglMakeCurrent_wrapped;
PFNWGLMAKECONTEXTCURRENTEXTPROC epoxy_wglMakeContextCurrentEXT = epoxy_wglMakeContextCurrentEXT_wrapped;
PFNWGLMAKECONTEXTCURRENTARBPROC epoxy_wglMakeContextCurrentARB = epoxy_wglMakeContextCurrentARB_wrapped;
PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC epoxy_wglMakeAssociatedContextCurrentEXT = epoxy_wglMakeAssociatedContextCurrentAMD_wrapped;
