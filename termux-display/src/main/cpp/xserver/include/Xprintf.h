/*
 * Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
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

#ifndef XPRINTF_H
#define XPRINTF_H

#include <stdio.h>
#include <stdarg.h>
#include <X11/Xfuncproto.h>

#ifndef _X_RESTRICT_KYWD
#if defined(restrict) /* assume autoconf set it correctly */ || \
   (defined(__STDC__) && (__STDC_VERSION__ - 0 >= 199901L))     /* C99 */
#define _X_RESTRICT_KYWD  restrict
#elif defined(__GNUC__) && !defined(__STRICT_ANSI__)    /* gcc w/C89+extensions */
#define _X_RESTRICT_KYWD __restrict__
#else
#define _X_RESTRICT_KYWD
#endif
#endif

/*
 * These functions provide a portable implementation of the common (but not
 * yet universal) asprintf & vasprintf routines to allocate a buffer big
 * enough to sprintf the arguments to.  The XNF variants terminate the server
 * if the allocation fails.
 * The buffer allocated is returned in the pointer provided in the first
 * argument.   The return value is the size of the allocated buffer, or -1
 * on failure.
 */
extern _X_EXPORT int
Xasprintf(char **ret, const char *_X_RESTRICT_KYWD fmt, ...)
_X_ATTRIBUTE_PRINTF(2, 3);
extern _X_EXPORT int
Xvasprintf(char **ret, const char *_X_RESTRICT_KYWD fmt, va_list va)
_X_ATTRIBUTE_PRINTF(2, 0);
extern _X_EXPORT int
XNFasprintf(char **ret, const char *_X_RESTRICT_KYWD fmt, ...)
_X_ATTRIBUTE_PRINTF(2, 3);
extern _X_EXPORT int
XNFvasprintf(char **ret, const char *_X_RESTRICT_KYWD fmt, va_list va)
_X_ATTRIBUTE_PRINTF(2, 0);

#if !defined(HAVE_ASPRINTF) && !defined(HAVE_VASPRINTF)
#define asprintf  Xasprintf
#define vasprintf Xvasprintf
#endif

/*
 * These functions provide a portable implementation of the linux kernel
 * scnprintf & vscnprintf routines that return the number of bytes actually
 * copied during a snprintf, (excluding the final '\0').
 */
extern _X_EXPORT int
Xscnprintf(char *s, int n, const char * _X_RESTRICT_KYWD fmt, ...)
_X_ATTRIBUTE_PRINTF(3,4);
extern _X_EXPORT int
Xvscnprintf(char *s, int n, const char * _X_RESTRICT_KYWD fmt, va_list va)
_X_ATTRIBUTE_PRINTF(3,0);

#endif                          /* XPRINTF_H */
