/* Copyright © 2013, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/** @file dlwrap.c
 *
 * Implements a wrapper for dlopen() and dlsym() so that epoxy will
 * end up finding symbols from the testcases named
 * "override_EGL_eglWhatever()" or "override_GLES2_glWhatever()" or
 * "override_GL_glWhatever()" when it tries to dlopen() and dlsym()
 * the real GL or EGL functions in question.
 *
 * This lets us simulate some target systems in the test suite, or
 * just stub out GL functions so we can be sure of what's being
 * called.
 */

/* dladdr is a glibc extension */
#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dlwrap.h"

#define STRNCMP_LITERAL(var, literal) \
    strncmp ((var), (literal), sizeof (literal) - 1)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

void *libfips_handle;

typedef void *(*fips_dlopen_t)(const char *filename, int flag);
typedef void *(*fips_dlsym_t)(void *handle, const char *symbol);

void *override_EGL_eglGetProcAddress(const char *name);
void *override_GL_glXGetProcAddress(const char *name);
void *override_GL_glXGetProcAddressARB(const char *name);
void __dlclose(void *handle);

static struct libwrap {
    const char *filename;
    const char *symbol_prefix;
    void *handle;
} wrapped_libs[] = {
    { "libGL.so", "GL", NULL },
    { "libEGL.so", "EGL", NULL },
    { "libGLESv2.so", "GLES2", NULL },
    { "libOpenGL.so", "GL", NULL},
};

/* Match 'filename' against an internal list of libraries for which
 * libfips has wrappers.
 *
 * Returns true and sets *index_ret if a match is found.
 * Returns false if no match is found. */
static struct libwrap *
find_wrapped_library(const char *filename)
{
    unsigned i;

    if (!filename)
        return NULL;

    for (i = 0; i < ARRAY_SIZE(wrapped_libs); i++) {
        if (strncmp(wrapped_libs[i].filename, filename,
                    strlen(wrapped_libs[i].filename)) == 0) {
            return &wrapped_libs[i];
        }
    }

    return NULL;
}

/* Many (most?) OpenGL programs dlopen libGL.so.1 rather than linking
 * against it directly, which means they would not be seeing our
 * wrapped GL symbols via LD_PRELOAD. So we catch the dlopen in a
 * wrapper here and redirect it to our library.
 */
void *
dlopen(const char *filename, int flag)
{
    void *ret;
    struct libwrap *wrap;

    /* Before deciding whether to redirect this dlopen to our own
     * library, we call the real dlopen. This assures that any
     * expected side-effects from loading the intended library are
     * resolved. Below, we may still return a handle pointing to
     * our own library, and not what is opened here. */
    ret = dlwrap_real_dlopen(filename, flag);

    /* If filename is not a wrapped library, just return real dlopen */
    wrap = find_wrapped_library(filename);
    if (!wrap)
        return ret;

    wrap->handle = ret;

    /* We use wrapped_libs as our handles to libraries. */
    return wrap;
}

/**
 * Wraps dlclose to hide our faked handles from it.
 */
void
__dlclose(void *handle)
{
    struct libwrap *wrap = handle;

    if (wrap < wrapped_libs ||
        wrap >= wrapped_libs + ARRAY_SIZE(wrapped_libs)) {
        void (*real_dlclose)(void *handle) = dlwrap_real_dlsym(RTLD_NEXT, "__dlclose");
        real_dlclose(handle);
    }
}

void *
dlwrap_real_dlopen(const char *filename, int flag)
{
    static fips_dlopen_t real_dlopen = NULL;

    if (!real_dlopen) {
        real_dlopen = (fips_dlopen_t) dlwrap_real_dlsym(RTLD_NEXT, "dlopen");
        if (!real_dlopen) {
            fputs("Error: Failed to find symbol for dlopen.\n", stderr);
            exit(1);
        }
    }

    return real_dlopen(filename, flag);
}

/**
 * Return the dlsym() on the application's namespace for
 * "override_<prefix>_<name>"
 */
static void *
wrapped_dlsym(const char *prefix, const char *name)
{
    char *wrap_name;
    void *symbol;

    if (asprintf(&wrap_name, "override_%s_%s", prefix, name) < 0) {
        fputs("Error: Failed to allocate memory.\n", stderr);
        abort();
    }

    symbol = dlwrap_real_dlsym(RTLD_DEFAULT, wrap_name);
    free(wrap_name);
    return symbol;
}

/* Since we redirect dlopens of libGL.so and libEGL.so to libfips we
 * need to ensure that dlysm succeeds for all functions that might be
 * defined in the real, underlying libGL library. But we're far too
 * lazy to implement wrappers for function that would simply
 * pass-through, so instead we also wrap dlysm and arrange for it to
 * pass things through with RTLD_next if libfips does not have the
 * function desired.  */
void *
dlsym(void *handle, const char *name)
{
    struct libwrap *wrap = handle;

    /* Make sure that handle is actually one of our wrapped libs. */
    if (wrap < wrapped_libs ||
        wrap >= wrapped_libs + ARRAY_SIZE(wrapped_libs)) {
        wrap = NULL;
    }

    /* Failing that, anything specifically requested from the
     * libfips library should be redirected to a real GL
     * library. */

    if (wrap) {
        void *symbol = wrapped_dlsym(wrap->symbol_prefix, name);
        if (symbol)
            return symbol;
        else
            return dlwrap_real_dlsym(wrap->handle, name);
    }

    /* And anything else is some unrelated dlsym. Just pass it
     * through.  (This also covers the cases of lookups with
     * special handles such as RTLD_DEFAULT or RTLD_NEXT.)
     */
    return dlwrap_real_dlsym(handle, name);
}

void *
dlwrap_real_dlsym(void *handle, const char *name)
{
    static fips_dlsym_t real_dlsym = NULL;

    if (!real_dlsym) {
        /* FIXME: This brute-force, hard-coded searching for a versioned
         * symbol is really ugly. The only reason I'm doing this is because
         * I need some way to lookup the "dlsym" function in libdl, but
         * I can't use 'dlsym' to do it. So dlvsym works, but forces me
         * to guess what the right version is.
         *
         * Potential fixes here:
         *
         *   1. Use libelf to actually inspect libdl.so and
         *      find the right version, (finding the right
         *      libdl.so can be made easier with
         *      dl_iterate_phdr).
         *
         *   2. Use libelf to find the offset of the 'dlsym'
         *      symbol within libdl.so, (and then add this to
         *      the base address at which libdl.so is loaded
         *      as reported by dl_iterate_phdr).
         *
         * In the meantime, I'll just keep augmenting this
         * hard-coded version list as people report bugs. */
        const char *version[] = {
            "GLIBC_2.17",
            "GLIBC_2.4",
            "GLIBC_2.3",
            "GLIBC_2.2.5",
            "GLIBC_2.2",
            "GLIBC_2.0",
            "FBSD_1.0"
        };
        int num_versions = sizeof(version) / sizeof(version[0]);
        int i;
        for (i = 0; i < num_versions; i++) {
            real_dlsym = (fips_dlsym_t) dlvsym(RTLD_NEXT, "dlsym", version[i]);
            if (real_dlsym)
                break;
        }
        if (i == num_versions) {
            fputs("Internal error: Failed to find real dlsym\n", stderr);
            fputs("This may be a simple matter of fips not knowing about the version of GLIBC that\n"
                  "your program is using. Current known versions are:\n\n\t",
                  stderr);
            for (i = 0; i < num_versions; i++)
                fprintf(stderr, "%s ", version[i]);
            fputs("\n\nYou can inspect your version by first finding libdl.so.2:\n"
                  "\n"
                  "\tldd <your-program> | grep libdl.so\n"
                  "\n"
                  "And then inspecting the version attached to the dlsym symbol:\n"
                  "\n"
                  "\treadelf -s /path/to/libdl.so.2 | grep dlsym\n"
                  "\n"
                  "And finally, adding the version to dlwrap.c:dlwrap_real_dlsym.\n",
                  stderr);

            exit(1);
        }
    }

    return real_dlsym(handle, name);
}

void *
override_GL_glXGetProcAddress(const char *name)
{
    void *symbol;

    symbol = wrapped_dlsym("GL", name);
    if (symbol)
        return symbol;

    return DEFER_TO_GL("libGL.so.1", override_GL_glXGetProcAddress,
                       "glXGetProcAddress", (name));
}

void *
override_GL_glXGetProcAddressARB(const char *name)
{
    void *symbol;

    symbol = wrapped_dlsym("GL", name);
    if (symbol)
        return symbol;

    return DEFER_TO_GL("libGL.so.1", override_GL_glXGetProcAddressARB,
                       "glXGetProcAddressARB", (name));
}

void *
override_EGL_eglGetProcAddress(const char *name)
{
    void *symbol;

    if (!STRNCMP_LITERAL(name, "gl")) {
        symbol = wrapped_dlsym("GLES2", name);
        if (symbol)
            return symbol;
    }

    if (!STRNCMP_LITERAL(name, "egl")) {
        symbol = wrapped_dlsym("EGL", name);
        if (symbol)
            return symbol;
    }

    return DEFER_TO_GL("libEGL.so.1", override_EGL_eglGetProcAddress,
                       "eglGetProcAddress", (name));
}
