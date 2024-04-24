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

#ifndef DLWRAP_H
#define DLWRAP_H

#define _GNU_SOURCE
#include <dlfcn.h>

/* Call the *real* dlopen. We have our own wrapper for dlopen that, of
 * necessity must use claim the symbol 'dlopen'. So whenever anything
 * internal needs to call the real, underlying dlopen function, the
 * thing to call is dlwrap_real_dlopen.
 */
void *
dlwrap_real_dlopen(const char *filename, int flag);

/* Perform a dlopen on the libfips library itself.
 *
 * Many places in fips need to lookup symbols within the libfips
 * library itself, (and not in any other library). This function
 * provides a reliable way to get a handle for performing such
 * lookups.
 *
 * The returned handle can be passed to dlwrap_real_dlsym for the
 * lookups. */
void *
dlwrap_dlopen_libfips(void);

/* Call the *real* dlsym. We have our own wrapper for dlsym that, of
 * necessity must use claim the symbol 'dlsym'. So whenever anything
 * internal needs to call the real, underlying dlysm function, the
 * thing to call is dlwrap_real_dlsym.
 */
void *
dlwrap_real_dlsym(void *handle, const char *symbol);

#define DEFER_TO_GL(library, func, name, args)                          \
({                                                                      \
    void *lib = dlwrap_real_dlopen(library, RTLD_LAZY | RTLD_LOCAL);    \
    typeof(&func) real_func = dlwrap_real_dlsym(lib, name);             \
    /* gcc extension -- func's return value is the return value of      \
     * the statement.                                                   \
     */                                                                 \
    real_func args;                                                     \
})

#endif

