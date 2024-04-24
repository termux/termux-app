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

#include "config.h"

#ifdef _WIN32
#define PLATFORM_HAS_EGL ENABLE_EGL
#define PLATFORM_HAS_GLX ENABLE_GLX
#define PLATFORM_HAS_WGL 1
#elif defined(__APPLE__)
#define PLATFORM_HAS_EGL 0 
#define PLATFORM_HAS_GLX ENABLE_GLX
#define PLATFORM_HAS_WGL 0
#elif defined(ANDROID)
#define PLATFORM_HAS_EGL ENABLE_EGL
#define PLATFORM_HAS_GLX 0
#define PLATFORM_HAS_WGL 0
#else
#define PLATFORM_HAS_EGL ENABLE_EGL
#define PLATFORM_HAS_GLX ENABLE_GLX
#define PLATFORM_HAS_WGL 0
#endif

#include "epoxy/gl.h"
#if PLATFORM_HAS_GLX
#include "epoxy/glx.h"
#endif
#if PLATFORM_HAS_EGL
# if !ENABLE_X11
/* Disable including X11 headers if the X11 support was disabled at
 * configuration time
 */
#  define EGL_NO_X11 1
/* Older versions of Mesa use this symbol to achieve the same result
 * as EGL_NO_X11
 */
#  define MESA_EGL_NO_X11_HEADERS 1
# endif
#include "epoxy/egl.h"
#endif
#if PLATFORM_HAS_WGL
#include "epoxy/wgl.h"
#endif

#if defined(__GNUC__)
#define PACKED __attribute__((__packed__))
#define ENDPACKED
#elif defined (_MSC_VER)
#define PACKED __pragma(pack(push,1))
#define ENDPACKED __pragma(pack(pop))
#else
#define PACKED
#define ENDPACKED
#endif

/* On win32, we're going to need to keep a per-thread dispatch table,
 * since the function pointers depend on the device and pixel format
 * of the current context.
 */
#if defined(_WIN32)
#define USING_DISPATCH_TABLE 1
#else
#define USING_DISPATCH_TABLE 0
#endif

#define UNWRAPPED_PROTO(x) (GLAPIENTRY *x)
#define WRAPPER_VISIBILITY(type) static type GLAPIENTRY
#define WRAPPER(x) x ## _wrapped

#define GEN_GLOBAL_REWRITE_PTR(name, args, passthrough)          \
    static void EPOXY_CALLSPEC                                   \
    name##_global_rewrite_ptr args                               \
    {                                                            \
        if (name == (void *)name##_global_rewrite_ptr)           \
            name = (void *)name##_resolver();                    \
        name passthrough;                                        \
    }

#define GEN_GLOBAL_REWRITE_PTR_RET(ret, name, args, passthrough) \
    static ret EPOXY_CALLSPEC                                    \
    name##_global_rewrite_ptr args                               \
    {                                                            \
        if (name == (void *)name##_global_rewrite_ptr)           \
            name = (void *)name##_resolver();                    \
        return name passthrough;                                 \
    }

#if USING_DISPATCH_TABLE
#define GEN_DISPATCH_TABLE_REWRITE_PTR(name, args, passthrough)            \
    static void EPOXY_CALLSPEC                                             \
    name##_dispatch_table_rewrite_ptr args                                 \
    {                                                                      \
        struct dispatch_table *dispatch_table = get_dispatch_table();      \
                                                                           \
        dispatch_table->name = (void *)name##_resolver();                  \
        dispatch_table->name passthrough;                                  \
    }

#define GEN_DISPATCH_TABLE_REWRITE_PTR_RET(ret, name, args, passthrough)   \
    static ret EPOXY_CALLSPEC                                              \
    name##_dispatch_table_rewrite_ptr args                                 \
    {                                                                      \
        struct dispatch_table *dispatch_table = get_dispatch_table();      \
                                                                           \
        dispatch_table->name = (void *)name##_resolver();                  \
        return dispatch_table->name passthrough;                           \
    }

#define GEN_DISPATCH_TABLE_THUNK(name, args, passthrough)                  \
    static void EPOXY_CALLSPEC                                             \
    name##_dispatch_table_thunk args                                       \
    {                                                                      \
        get_dispatch_table()->name passthrough;                            \
    }

#define GEN_DISPATCH_TABLE_THUNK_RET(ret, name, args, passthrough)         \
    static ret EPOXY_CALLSPEC                                              \
    name##_dispatch_table_thunk args                                       \
    {                                                                      \
        return get_dispatch_table()->name passthrough;                     \
    }

#else
#define GEN_DISPATCH_TABLE_REWRITE_PTR(name, args, passthrough)
#define GEN_DISPATCH_TABLE_REWRITE_PTR_RET(ret, name, args, passthrough)
#define GEN_DISPATCH_TABLE_THUNK(name, args, passthrough)
#define GEN_DISPATCH_TABLE_THUNK_RET(ret, name, args, passthrough)
#endif

#define GEN_THUNKS(name, args, passthrough)                          \
    GEN_GLOBAL_REWRITE_PTR(name, args, passthrough)                  \
    GEN_DISPATCH_TABLE_REWRITE_PTR(name, args, passthrough)          \
    GEN_DISPATCH_TABLE_THUNK(name, args, passthrough)

#define GEN_THUNKS_RET(ret, name, args, passthrough)                 \
    GEN_GLOBAL_REWRITE_PTR_RET(ret, name, args, passthrough)         \
    GEN_DISPATCH_TABLE_REWRITE_PTR_RET(ret, name, args, passthrough) \
    GEN_DISPATCH_TABLE_THUNK_RET(ret, name, args, passthrough)

void *epoxy_egl_dlsym(const char *name);
void *epoxy_glx_dlsym(const char *name);
void *epoxy_gl_dlsym(const char *name);
void *epoxy_gles1_dlsym(const char *name);
void *epoxy_gles2_dlsym(const char *name);
void *epoxy_gles3_dlsym(const char *name);
void *epoxy_get_proc_address(const char *name);
void *epoxy_get_core_proc_address(const char *name, int core_version);
void *epoxy_get_bootstrap_proc_address(const char *name);

int epoxy_conservative_gl_version(void);
bool epoxy_conservative_has_gl_extension(const char *name);
int epoxy_conservative_glx_version(void);
bool epoxy_conservative_has_glx_extension(const char *name);
int epoxy_conservative_egl_version(void);
bool epoxy_conservative_has_egl_extension(const char *name);
bool epoxy_conservative_has_wgl_extension(const char *name);
void *epoxy_conservative_egl_dlsym(const char *name, bool exit_if_fails);
void *epoxy_conservative_glx_dlsym(const char *name, bool exit_if_fails);

bool epoxy_load_glx(bool exit_if_fails, bool load);
bool epoxy_load_egl(bool exit_if_fails, bool load);

#define glBegin_unwrapped epoxy_glBegin_unwrapped
#define glEnd_unwrapped epoxy_glEnd_unwrapped
extern void UNWRAPPED_PROTO(glBegin_unwrapped)(GLenum primtype);
extern void UNWRAPPED_PROTO(glEnd_unwrapped)(void);

extern epoxy_resolver_failure_handler_t epoxy_resolver_failure_handler;

#if USING_DISPATCH_TABLE
void gl_init_dispatch_table(void);
void gl_switch_to_dispatch_table(void);
void wgl_init_dispatch_table(void);
void wgl_switch_to_dispatch_table(void);
extern uint32_t gl_tls_index, gl_tls_size;
extern uint32_t wgl_tls_index, wgl_tls_size;

#define wglMakeCurrent_unwrapped epoxy_wglMakeCurrent_unwrapped
#define wglMakeContextCurrentARB_unwrapped epoxy_wglMakeContextCurrentARB_unwrapped
#define wglMakeContextCurrentEXT_unwrapped epoxy_wglMakeContextCurrentEXT_unwrapped
#define wglMakeAssociatedContextCurrentAMD_unwrapped epoxy_wglMakeAssociatedContextCurrentAMD_unwrapped
extern BOOL UNWRAPPED_PROTO(wglMakeCurrent_unwrapped)(HDC hdc, HGLRC hglrc);
extern BOOL UNWRAPPED_PROTO(wglMakeContextCurrentARB_unwrapped)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
extern BOOL UNWRAPPED_PROTO(wglMakeContextCurrentEXT_unwrapped)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
extern BOOL UNWRAPPED_PROTO(wglMakeAssociatedContextCurrentAMD_unwrapped)(HGLRC hglrc);
#endif /* _WIN32_ */
