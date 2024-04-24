/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <X11/Xfuncproto.h>

#ifndef HAVE_REALLOCARRAY
extern _X_HIDDEN void *xreallocarray(void *optr, size_t nmemb, size_t size);
# define reallocarray(ptr, n, size)	xreallocarray((ptr), (size_t)(n), (size_t)(size))
#endif

#if defined(MALLOC_0_RETURNS_NULL) || defined(__clang_analyzer__)
# define Xreallocarray(ptr, n, size) \
    reallocarray((ptr), ((n) == 0 ? 1 : (n)), size)
#else
# define Xreallocarray(ptr, n, size)	reallocarray((ptr), (n), (size))
#endif

#define Xmallocarray(n, size)		Xreallocarray(NULL, (n), (size))
