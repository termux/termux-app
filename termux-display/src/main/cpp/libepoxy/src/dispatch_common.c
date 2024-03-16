/*
 * Copyright © 2013-2014 Intel Corporation
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

/**
 * \mainpage Epoxy
 *
 * \section intro_sec Introduction
 *
 * Epoxy is a library for handling OpenGL function pointer management for
 * you.
 *
 * It hides the complexity of `dlopen()`, `dlsym()`, `glXGetProcAddress()`,
 * `eglGetProcAddress()`, etc. from the app developer, with very little
 * knowledge needed on their part.  They get to read GL specs and write
 * code using undecorated function names like `glCompileShader()`.
 *
 * Don't forget to check for your extensions or versions being present
 * before you use them, just like before!  We'll tell you what you forgot
 * to check for instead of just segfaulting, though.
 *
 * \section features_sec Features
 *
 *   - Automatically initializes as new GL functions are used.
 *   - GL 4.6 core and compatibility context support.
 *   - GLES 1/2/3 context support.
 *   - Knows about function aliases so (e.g.) `glBufferData()` can be
 *     used with `GL_ARB_vertex_buffer_object` implementations, along
 *     with GL 1.5+ implementations.
 *   - EGL, GLX, and WGL support.
 *   - Can be mixed with non-epoxy GL usage.
 *
 * \section using_sec Using Epoxy
 *
 * Using Epoxy should be as easy as replacing:
 *
 * ```cpp
 * #include <GL/gl.h>
 * #include <GL/glx.h>
 * #include <GL/glext.h>
 * ```
 *
 * with:
 *
 * ```cpp
 * #include <epoxy/gl.h>
 * #include <epoxy/glx.h>
 * ```
 *
 * \subsection using_include_sec Headers
 *
 * Epoxy comes with the following public headers:
 *
 *  - `epoxy/gl.h`  - For GL API
 *  - `epoxy/egl.h` - For EGL API
 *  - `epoxy/glx.h` - For GLX API
 *  - `epoxy/wgl.h` - For WGL API
 *
 * \section links_sec Additional links
 *
 * The latest version of the Epoxy code is available on [GitHub](https://github.com/anholt/libepoxy).
 *
 * For bug reports and enhancements, please use the [Issues](https://github.com/anholt/libepoxy/issues)
 * link.
 *
 * The scope of this API reference does not include the documentation for
 * OpenGL and OpenGL ES. For more information on those programming interfaces
 * please visit:
 *
 *  - [Khronos](https://www.khronos.org/)
 *  - [OpenGL page on Khronos.org](https://www.khronos.org/opengl/)
 *  - [OpenGL ES page on Khronos.org](https://www.khronos.org/opengles/)
 *  - [docs.GL](http://docs.gl/)
 */

/**
 * @file dispatch_common.c
 *
 * @brief Implements common code shared by the generated GL/EGL/GLX dispatch code.
 *
 * A collection of some important specs on getting GL function pointers.
 *
 * From the linux GL ABI (http://www.opengl.org/registry/ABI/):
 *
 *     "3.4. The libraries must export all OpenGL 1.2, GLU 1.3, GLX 1.3, and
 *           ARB_multitexture entry points statically.
 *
 *      3.5. Because non-ARB extensions vary so widely and are constantly
 *           increasing in number, it's infeasible to require that they all be
 *           supported, and extensions can always be added to hardware drivers
 *           after the base link libraries are released. These drivers are
 *           dynamically loaded by libGL, so extensions not in the base
 *           library must also be obtained dynamically.
 *
 *      3.6. To perform the dynamic query, libGL also must export an entry
 *           point called
 *
 *           void (*glXGetProcAddressARB(const GLubyte *))(); 
 *
 *      The full specification of this function is available separately. It
 *      takes the string name of a GL or GLX entry point and returns a pointer
 *      to a function implementing that entry point. It is functionally
 *      identical to the wglGetProcAddress query defined by the Windows OpenGL
 *      library, except that the function pointers returned are context
 *      independent, unlike the WGL query."
 *
 * From the EGL 1.4 spec:
 *
 *    "Client API function pointers returned by eglGetProcAddress are
 *     independent of the display and the currently bound client API context,
 *     and may be used by any client API context which supports the extension.
 *
 *     eglGetProcAddress may be queried for all of the following functions:
 *
 *     • All EGL and client API extension functions supported by the
 *       implementation (whether those extensions are supported by the current
 *       client API context or not). This includes any mandatory OpenGL ES
 *       extensions.
 *
 *     eglGetProcAddress may not be queried for core (non-extension) functions
 *     in EGL or client APIs 20 .
 *
 *     For functions that are queryable with eglGetProcAddress,
 *     implementations may choose to also export those functions statically
 *     from the object libraries im- plementing those functions. However,
 *     portable clients cannot rely on this behavior.
 *
 * From the GLX 1.4 spec:
 *
 *     "glXGetProcAddress may be queried for all of the following functions:
 *
 *      • All GL and GLX extension functions supported by the implementation
 *        (whether those extensions are supported by the current context or
 *        not).
 *
 *      • All core (non-extension) functions in GL and GLX from version 1.0 up
 *        to and including the versions of those specifications supported by
 *        the implementation, as determined by glGetString(GL VERSION) and
 *        glXQueryVersion queries."
 */

#include <assert.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <err.h>
#include <pthread.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "dispatch_common.h"

#if defined(__APPLE__)
#define GLX_LIB "/opt/X11/lib/libGL.1.dylib"
#define OPENGL_LIB "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
#define GLES1_LIB "libGLESv1_CM.so"
#define GLES2_LIB "libGLESv2.so"
#elif defined(__ANDROID__)
#define GLX_LIB "libGLESv2.so"
#define EGL_LIB "libEGL.so"
#define GLES1_LIB "libGLESv1_CM.so"
#define GLES2_LIB "libGLESv2.so"
#elif defined(_WIN32)
#define EGL_LIB "libEGL.dll"
#define GLES1_LIB "libGLES_CM.dll"
#define GLES2_LIB "libGLESv2.dll"
#define OPENGL_LIB "OPENGL32"
#else
#define GLVND_GLX_LIB "libGLX.so.1"
#define GLX_LIB "libGL.so.1"
#define EGL_LIB "libEGL.so.1"
#define GLES1_LIB "libGLESv1_CM.so.1"
#define GLES2_LIB "libGLESv2.so.2"
#define OPENGL_LIB "libOpenGL.so.0"
#endif

#ifdef __GNUC__
#define CONSTRUCT(_func) static void _func (void) __attribute__((constructor));
#define DESTRUCT(_func) static void _func (void) __attribute__((destructor));
#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
#define CONSTRUCT(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _wrapper;

#define DESTRUCT(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#else
#error "You will need constructor support for your compiler"
#endif

struct api {
#ifndef _WIN32
    /*
     * Locking for making sure we don't double-dlopen().
     */
    pthread_mutex_t mutex;
#endif

    /*
     * dlopen() return value for the GLX API. This is libGLX.so.1 if the
     * runtime is glvnd-enabled, else libGL.so.1
     */
    void *glx_handle;

    /*
     * dlopen() return value for the desktop GL library.
     *
     * On Windows this is OPENGL32. On OSX this is classic libGL. On Linux
     * this is either libOpenGL (if the runtime is glvnd-enabled) or
     * classic libGL.so.1
     */
    void *gl_handle;

    /* dlopen() return value for libEGL.so.1 */
    void *egl_handle;

    /* dlopen() return value for libGLESv1_CM.so.1 */
    void *gles1_handle;

    /* dlopen() return value for libGLESv2.so.2 */
    void *gles2_handle;

    /*
     * This value gets incremented when any thread is in
     * glBegin()/glEnd() called through epoxy.
     *
     * We're not guaranteed to be called through our wrapper, so the
     * conservative paths also try to handle the failure cases they'll
     * see if begin_count didn't reflect reality.  It's also a bit of
     * a bug that the conservative paths might return success because
     * some other thread was in epoxy glBegin/glEnd while our thread
     * is trying to resolve, but given that it's basically just for
     * informative error messages, we shouldn't need to care.
     */
    long begin_count;
};

static struct api api = {
#ifndef _WIN32
    .mutex = PTHREAD_MUTEX_INITIALIZER,
#else
	0,
#endif
};

static bool library_initialized;

static bool epoxy_current_context_is_glx(void);

#if PLATFORM_HAS_EGL
static EGLenum
epoxy_egl_get_current_gl_context_api(void);
#endif

CONSTRUCT (library_init)

static void
library_init(void)
{
    library_initialized = true;
}

static bool
get_dlopen_handle(void **handle, const char *lib_name, bool exit_on_fail, bool load)
{
    if (*handle)
        return true;

    if (!library_initialized) {
        fputs("Attempting to dlopen() while in the dynamic linker.\n", stderr);
        abort();
    }

#ifdef _WIN32
    *handle = LoadLibraryA(lib_name);
#else
    pthread_mutex_lock(&api.mutex);
    if (!*handle) {
        int flags = RTLD_LAZY | RTLD_LOCAL;
        if (!load)
            flags |= RTLD_NOLOAD;

        *handle = dlopen(lib_name, flags);
        if (!*handle) {
            if (exit_on_fail) {
                fprintf(stderr, "Couldn't open %s: %s\n", lib_name, dlerror());
                abort();
            } else {
                (void)dlerror();
            }
        }
    }
    pthread_mutex_unlock(&api.mutex);
#endif

    return *handle != NULL;
}

static void *
do_dlsym(void **handle, const char *name, bool exit_on_fail)
{
    void *result;
    const char *error = "";

#ifdef _WIN32
    result = GetProcAddress(*handle, name);
#else
    result = dlsym(*handle, name);
    if (!result)
        error = dlerror();
#endif
    if (!result && exit_on_fail) {
        fprintf(stderr, "%s() not found: %s\n", name, error);
        abort();
    }

    return result;
}

/**
 * @brief Checks whether we're using OpenGL or OpenGL ES
 *
 * @return `true` if we're using OpenGL
 */
bool
epoxy_is_desktop_gl(void)
{
    const char *es_prefix = "OpenGL ES";
    const char *version;

#if PLATFORM_HAS_EGL
    /* PowerVR's OpenGL ES implementation (and perhaps other) don't
     * comply with the standard, which states that
     * "glGetString(GL_VERSION)" should return a string starting with
     * "OpenGL ES". Therefore, to distinguish desktop OpenGL from
     * OpenGL ES, we must also check the context type through EGL (we
     * can do that as PowerVR is only usable through EGL).
     */
    if (!epoxy_current_context_is_glx()) {
        switch (epoxy_egl_get_current_gl_context_api()) {
        case EGL_OPENGL_API:     return true;
        case EGL_OPENGL_ES_API:  return false;
        case EGL_NONE:
        default:  break;
        }
    }
#endif

    if (api.begin_count)
        return true;

    version = (const char *)glGetString(GL_VERSION);

    /* If we didn't get a version back, there are only two things that
     * could have happened: either malloc failure (which basically
     * doesn't exist), or we were called within a glBegin()/glEnd().
     * Assume the second, which only exists for desktop GL.
     */
    if (!version)
        return true;

    return strncmp(es_prefix, version, strlen(es_prefix));
}

static int
epoxy_internal_gl_version(GLenum version_string, int error_version, int factor)
{
    const char *version = (const char *)glGetString(version_string);
    GLint major, minor;
    int scanf_count;

    if (!version)
        return error_version;

    /* skip to version number */
    while (!isdigit(*version) && *version != '\0')
        version++;

    /* Interpret version number */
    scanf_count = sscanf(version, "%i.%i", &major, &minor);
    if (scanf_count != 2) {
        fprintf(stderr, "Unable to interpret GL_VERSION string: %s\n",
                version);
        abort();
    }

    return factor * major + minor;
}

/**
 * @brief Returns the version of OpenGL we are using
 *
 * The version is encoded as:
 *
 * ```
 *
 *   version = major * 10 + minor
 *
 * ```
 *
 * So it can be easily used for version comparisons.
 *
 * @return The encoded version of OpenGL we are using
 */
int
epoxy_gl_version(void)
{
    return epoxy_internal_gl_version(GL_VERSION, 0, 10);
}

int
epoxy_conservative_gl_version(void)
{
    if (api.begin_count)
        return 100;

    return epoxy_internal_gl_version(GL_VERSION, 100, 10);
}

/**
 * @brief Returns the version of the GL Shading Language we are using
 *
 * The version is encoded as:
 *
 * ```
 *
 *   version = major * 100 + minor
 *
 * ```
 *
 * So it can be easily used for version comparisons.
 *
 * @return The encoded version of the GL Shading Language we are using
 */
int
epoxy_glsl_version(void)
{
    if (epoxy_gl_version() >= 20 ||
        epoxy_has_gl_extension ("GL_ARB_shading_language_100"))
        return epoxy_internal_gl_version(GL_SHADING_LANGUAGE_VERSION, 0, 100);

    return 0;
}

/**
 * @brief Checks for the presence of an extension in an OpenGL extension string
 *
 * @param extension_list The string containing the list of extensions to check
 * @param ext The name of the GL extension
 * @return `true` if the extension is available'
 *
 * @note If you are looking to check whether a normal GL, EGL or GLX extension
 * is supported by the client, this probably isn't the function you want.
 *
 * Some parts of the spec for OpenGL and friends will return an OpenGL formatted
 * extension string that is separate from the usual extension strings for the
 * spec. This function provides easy parsing of those strings.
 *
 * @see epoxy_has_gl_extension()
 * @see epoxy_has_egl_extension()
 * @see epoxy_has_glx_extension()
 */
bool
epoxy_extension_in_string(const char *extension_list, const char *ext)
{
    const char *ptr = extension_list;
    int len;

    if (!ext)
        return false;

    len = strlen(ext);

    if (extension_list == NULL || *extension_list == '\0')
        return false;

    /* Make sure that don't just find an extension with our name as a prefix. */
    while (true) {
        ptr = strstr(ptr, ext);
        if (!ptr)
            return false;

        if (ptr[len] == ' ' || ptr[len] == 0)
            return true;
        ptr += len;
    }
}

static bool
epoxy_internal_has_gl_extension(const char *ext, bool invalid_op_mode)
{
    if (epoxy_gl_version() < 30) {
        const char *exts = (const char *)glGetString(GL_EXTENSIONS);
        if (!exts)
            return invalid_op_mode;
        return epoxy_extension_in_string(exts, ext);
    } else {
        int num_extensions;
        int i;

        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        if (num_extensions == 0)
            return invalid_op_mode;

        for (i = 0; i < num_extensions; i++) {
            const char *gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
            if (!gl_ext)
                return false;
            if (strcmp(ext, gl_ext) == 0)
                return true;
        }

        return false;
    }
}

bool
epoxy_load_glx(bool exit_if_fails, bool load)
{
#if PLATFORM_HAS_GLX
# ifdef GLVND_GLX_LIB
    /* prefer the glvnd library if it exists */
    if (!api.glx_handle)
	get_dlopen_handle(&api.glx_handle, GLVND_GLX_LIB, false, load);
# endif
    if (!api.glx_handle)
        get_dlopen_handle(&api.glx_handle, GLX_LIB, exit_if_fails, load);
#endif
    return api.glx_handle != NULL;
}

void *
epoxy_conservative_glx_dlsym(const char *name, bool exit_if_fails)
{
#if PLATFORM_HAS_GLX
    if (epoxy_load_glx(exit_if_fails, exit_if_fails))
        return do_dlsym(&api.glx_handle, name, exit_if_fails);
#endif
    return NULL;
}

/**
 * Tests whether the currently bound context is EGL or GLX, trying to
 * avoid loading libraries unless necessary.
 */
static bool
epoxy_current_context_is_glx(void)
{
#if !PLATFORM_HAS_GLX
    return false;
#else
    void *sym;

    sym = epoxy_conservative_glx_dlsym("glXGetCurrentContext", false);
    if (sym) {
        if (glXGetCurrentContext())
            return true;
    } else {
        (void)dlerror();
    }

#if PLATFORM_HAS_EGL
    sym = epoxy_conservative_egl_dlsym("eglGetCurrentContext", false);
    if (sym) {
        if (epoxy_egl_get_current_gl_context_api() != EGL_NONE)
            return false;
    } else {
        (void)dlerror();
    }
#endif /* PLATFORM_HAS_EGL */

    return false;
#endif /* PLATFORM_HAS_GLX */
}

/**
 * @brief Returns true if the given GL extension is supported in the current context.
 *
 * @param ext The name of the GL extension
 * @return `true` if the extension is available
 *
 * @note that this function can't be called from within `glBegin()` and `glEnd()`.
 *
 * @see epoxy_has_egl_extension()
 * @see epoxy_has_glx_extension()
 */
bool
epoxy_has_gl_extension(const char *ext)
{
    return epoxy_internal_has_gl_extension(ext, false);
}

bool
epoxy_conservative_has_gl_extension(const char *ext)
{
    if (api.begin_count)
        return true;

    return epoxy_internal_has_gl_extension(ext, true);
}

bool
epoxy_load_egl(bool exit_if_fails, bool load)
{
#if PLATFORM_HAS_EGL
    return get_dlopen_handle(&api.egl_handle, EGL_LIB, exit_if_fails, load);
#else
    return false;
#endif
}

void *
epoxy_conservative_egl_dlsym(const char *name, bool exit_if_fails)
{
#if PLATFORM_HAS_EGL
    if (epoxy_load_egl(exit_if_fails, exit_if_fails))
        return do_dlsym(&api.egl_handle, name, exit_if_fails);
#endif
    return NULL;
}

void *
epoxy_egl_dlsym(const char *name)
{
    return epoxy_conservative_egl_dlsym(name, true);
}

void *
epoxy_glx_dlsym(const char *name)
{
    return epoxy_conservative_glx_dlsym(name, true);
}

static void
epoxy_load_gl(void)
{
    if (api.gl_handle)
	return;

#if defined(_WIN32) || defined(__APPLE__)
    get_dlopen_handle(&api.gl_handle, OPENGL_LIB, true, true);
#else

    // Prefer GLX_LIB over OPENGL_LIB to maintain existing behavior.
    // Using the inverse ordering OPENGL_LIB -> GLX_LIB, causes issues such as:
    // https://github.com/anholt/libepoxy/issues/240 (apitrace missing calls)
    // https://github.com/anholt/libepoxy/issues/252 (Xorg boot crash)
    get_dlopen_handle(&api.glx_handle, GLX_LIB, false, true);
    api.gl_handle = api.glx_handle;

#if defined(OPENGL_LIB)
    if (!api.gl_handle)
        get_dlopen_handle(&api.gl_handle, OPENGL_LIB, false, true);
#endif

    if (!api.gl_handle) {
#if defined(OPENGL_LIB)
        fprintf(stderr, "Couldn't open %s or %s\n", GLX_LIB, OPENGL_LIB);
#else
        fprintf(stderr, "Couldn't open %s\n", GLX_LIB);
#endif
        abort();
    }

#endif
}

void *
epoxy_gl_dlsym(const char *name)
{
    epoxy_load_gl();

    return do_dlsym(&api.gl_handle, name, true);
}

void *
epoxy_gles1_dlsym(const char *name)
{
    if (epoxy_current_context_is_glx()) {
        return epoxy_get_proc_address(name);
    } else {
        get_dlopen_handle(&api.gles1_handle, GLES1_LIB, true, true);
        return do_dlsym(&api.gles1_handle, name, true);
    }
}

void *
epoxy_gles2_dlsym(const char *name)
{
    if (epoxy_current_context_is_glx()) {
        return epoxy_get_proc_address(name);
    } else {
        get_dlopen_handle(&api.gles2_handle, GLES2_LIB, true, true);
        return do_dlsym(&api.gles2_handle, name, true);
    }
}

/**
 * Does the appropriate dlsym() or eglGetProcAddress() for GLES3
 * functions.
 *
 * Mesa interpreted GLES as intending that the GLES3 functions were
 * available only through eglGetProcAddress() and not dlsym(), while
 * ARM's Mali drivers interpreted GLES as intending that GLES3
 * functions were available only through dlsym() and not
 * eglGetProcAddress().  Thanks, Khronos.
 */
void *
epoxy_gles3_dlsym(const char *name)
{
    if (epoxy_current_context_is_glx()) {
        return epoxy_get_proc_address(name);
    } else {
        if (get_dlopen_handle(&api.gles2_handle, GLES2_LIB, false, true)) {
            void *func = do_dlsym(&api.gles2_handle, name, false);

            if (func)
                return func;
        }

        return epoxy_get_proc_address(name);
    }
}

/**
 * Performs either the dlsym or glXGetProcAddress()-equivalent for
 * core functions in desktop GL.
 */
void *
epoxy_get_core_proc_address(const char *name, int core_version)
{
#ifdef _WIN32
    int core_symbol_support = 11;
#elif defined(__ANDROID__)
    /**
     * All symbols must be resolved through eglGetProcAddress
     * on Android
     */
    int core_symbol_support = 0;
#else
    int core_symbol_support = 12;
#endif

    if (core_version <= core_symbol_support) {
        return epoxy_gl_dlsym(name);
    } else {
        return epoxy_get_proc_address(name);
    }
}

#if PLATFORM_HAS_EGL
static EGLenum
epoxy_egl_get_current_gl_context_api(void)
{
    EGLint curapi;

    if (eglQueryContext(eglGetCurrentDisplay(), eglGetCurrentContext(),
			EGL_CONTEXT_CLIENT_TYPE, &curapi) == EGL_FALSE) {
	(void)eglGetError();
	return EGL_NONE;
    }

    return (EGLenum) curapi;
}
#endif /* PLATFORM_HAS_EGL */

/**
 * Performs the dlsym() for the core GL 1.0 functions that we use for
 * determining version and extension support for deciding on dlsym
 * versus glXGetProcAddress() for all other functions.
 *
 * This needs to succeed on implementations without GLX (since
 * glGetString() and glGetIntegerv() are both in GLES1/2 as well, and
 * at call time we don't know for sure what API they're trying to use
 * without inspecting contexts ourselves).
 */
void *
epoxy_get_bootstrap_proc_address(const char *name)
{
    /* If we already have a library that links to libglapi loaded,
     * use that.
     */
#if PLATFORM_HAS_GLX
    if (api.glx_handle && glXGetCurrentContext())
        return epoxy_gl_dlsym(name);
#endif

    /* If epoxy hasn't loaded any API-specific library yet, try to
     * figure out what API the context is using and use that library,
     * since future calls will also use that API (this prevents a
     * non-X11 ES2 context from loading a bunch of X11 junk).
     */
#if PLATFORM_HAS_EGL
    get_dlopen_handle(&api.egl_handle, EGL_LIB, false, true);
    if (api.egl_handle) {
        int version = 0;
        switch (epoxy_egl_get_current_gl_context_api()) {
        case EGL_OPENGL_API:
            return epoxy_gl_dlsym(name);
        case EGL_OPENGL_ES_API:
            if (eglQueryContext(eglGetCurrentDisplay(),
                                eglGetCurrentContext(),
                                EGL_CONTEXT_CLIENT_VERSION,
                                &version)) {
                if (version >= 2)
                    return epoxy_gles2_dlsym(name);
                else
                    return epoxy_gles1_dlsym(name);
            }
        }
    }
#endif /* PLATFORM_HAS_EGL */

    /* Fall back to GLX */
    return epoxy_gl_dlsym(name);
}

void *
epoxy_get_proc_address(const char *name)
{
#if PLATFORM_HAS_EGL
    GLenum egl_api = EGL_NONE;

    if (!epoxy_current_context_is_glx())
      egl_api = epoxy_egl_get_current_gl_context_api();

    switch (egl_api) {
    case EGL_OPENGL_API:
    case EGL_OPENGL_ES_API:
        return eglGetProcAddress(name);
    case EGL_NONE:
        break;
    }
#endif

#if defined(_WIN32)
    return wglGetProcAddress(name);
#elif defined(__APPLE__)
    return epoxy_gl_dlsym(name);
#elif PLATFORM_HAS_GLX
    if (epoxy_current_context_is_glx())
        return glXGetProcAddressARB((const GLubyte *)name);
    assert(0 && "Couldn't find current GLX or EGL context.\n");
#endif

    return NULL;
}

WRAPPER_VISIBILITY (void)
WRAPPER(epoxy_glBegin)(GLenum primtype)
{
#ifdef _WIN32
    InterlockedIncrement(&api.begin_count);
#else
    pthread_mutex_lock(&api.mutex);
    api.begin_count++;
    pthread_mutex_unlock(&api.mutex);
#endif

    epoxy_glBegin_unwrapped(primtype);
}

WRAPPER_VISIBILITY (void)
WRAPPER(epoxy_glEnd)(void)
{
    epoxy_glEnd_unwrapped();

#ifdef _WIN32
    InterlockedDecrement(&api.begin_count);
#else
    pthread_mutex_lock(&api.mutex);
    api.begin_count--;
    pthread_mutex_unlock(&api.mutex);
#endif
}

PFNGLBEGINPROC epoxy_glBegin = epoxy_glBegin_wrapped;
PFNGLENDPROC epoxy_glEnd = epoxy_glEnd_wrapped;

epoxy_resolver_failure_handler_t epoxy_resolver_failure_handler;

/**
 * Sets the function that will be called every time Epoxy fails to
 * resolve a symbol.
 *
 * @param handler The new handler function
 * @return The previous handler function
 */
epoxy_resolver_failure_handler_t
epoxy_set_resolver_failure_handler(epoxy_resolver_failure_handler_t handler)
{
#ifdef _WIN32
    return InterlockedExchangePointer((void**)&epoxy_resolver_failure_handler,
				      handler);
#else
    epoxy_resolver_failure_handler_t old;
    pthread_mutex_lock(&api.mutex);
    old = epoxy_resolver_failure_handler;
    epoxy_resolver_failure_handler = handler;
    pthread_mutex_unlock(&api.mutex);
    return old;
#endif
}
