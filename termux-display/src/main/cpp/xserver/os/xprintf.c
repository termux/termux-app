/**
 * @file
 *
 * @section DESCRIPTION
 *
 * These functions provide a portable implementation of the common (but not
 * yet universal) asprintf & vasprintf routines to allocate a buffer big
 * enough to sprintf the arguments to.  The XNF variants terminate the server
 * if the allocation fails.
 */
/*
 * Copyright (c) 2004 Alexander Gottwald
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
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xos.h>
#include "os.h"
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef asprintf
#undef asprintf
#endif
#ifdef vasprintf
#undef vasprintf
#endif

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#error "no working va_copy was found"
#endif
#endif

/**
 * Varargs sprintf that allocates a string buffer the right size for
 * the pattern & data provided and prints the requested data to it.
 *
 * @param ret     Pointer to which the newly allocated buffer is written
 *                (contents undefined on error)
 * @param format  printf style format string
 * @param va      variable argument list
 * @return        size of allocated buffer, or -1 on error.
 */
int
Xvasprintf(char **ret, const char *_X_RESTRICT_KYWD format, va_list va)
{
#ifdef HAVE_VASPRINTF
    return vasprintf(ret, format, va);
#else
    int size;
    va_list va2;

    va_copy(va2, va);
    size = vsnprintf(NULL, 0, format, va2);
    va_end(va2);

    *ret = malloc(size + 1);
    if (*ret == NULL)
        return -1;

    vsnprintf(*ret, size + 1, format, va);
    (*ret)[size] = 0;
    return size;
#endif
}

#ifndef HAVE_VASPRINTF
#define vasprintf Xvasprintf
#endif

/**
 * sprintf that allocates a string buffer the right size for
 * the pattern & data provided and prints the requested data to it.
 *
 * @param ret     Pointer to which the newly allocated buffer is written
 *                (contents undefined on error)
 * @param format  printf style format string
 * @param ...     arguments for specified format
 * @return        size of allocated buffer, or -1 on error.
 */
int
Xasprintf(char **ret, const char *_X_RESTRICT_KYWD format, ...)
{
    int size;
    va_list va;

    va_start(va, format);
    size = vasprintf(ret, format, va);
    va_end(va);
    return size;
}

/**
 * Varargs sprintf that allocates a string buffer the right size for
 * the pattern & data provided and prints the requested data to it.
 * On failure, issues a FatalError message and aborts the server.
 *
 * @param ret     Pointer to which the newly allocated buffer is written
 *                (contents undefined on error)
 * @param format  printf style format string
 * @param va      variable argument list
 * @return        size of allocated buffer
 */
int
XNFvasprintf(char **ret, const char *_X_RESTRICT_KYWD format, va_list va)
{
    int size = vasprintf(ret, format, va);

    if ((size == -1) || (*ret == NULL)) {
        FatalError("XNFvasprintf failed: %s", strerror(errno));
    }
    return size;
}

/**
 * sprintf that allocates a string buffer the right size for
 * the pattern & data provided and prints the requested data to it.
 * On failure, issues a FatalError message and aborts the server.
 *
 * @param ret     Pointer to which the newly allocated buffer is written
 *                (contents undefined on error)
 * @param format  printf style format string
 * @param ...     arguments for specified format
 * @return        size of allocated buffer
 */
int
XNFasprintf(char **ret, const char *_X_RESTRICT_KYWD format, ...)
{
    int size;
    va_list va;

    va_start(va, format);
    size = XNFvasprintf(ret, format, va);
    va_end(va);
    return size;
}

/**
 * Varargs snprintf that returns the actual number of bytes (excluding final
 * '\0') that were copied into the buffer.
 * This is opposed to the normal sprintf() usually returns the number of bytes
 * that would have been written.
 *
 * @param s       buffer to copy into
 * @param n       size of buffer s
 * @param format  printf style format string
 * @param va      variable argument list
 * @return        number of bytes actually copied, excluding final '\0'
 */
int
Xvscnprintf(char *s, int n, const char *format, va_list args)
{
    int x;
    if (n == 0)
        return 0;
    x = vsnprintf(s, n , format, args);
    return (x >= n) ? (n - 1) : x;
}

/**
 * snprintf that returns the actual number of bytes (excluding final '\0') that
 * were copied into the buffer.
 * This is opposed to the normal sprintf() usually returns the number of bytes
 * that would have been written.
 *
 * @param s       buffer to copy into
 * @param n       size of buffer s
 * @param format  printf style format string
 * @param ...     arguments for specified format
 * @return        number of bytes actually copied, excluding final '\0'
 */
int Xscnprintf(char *s, int n, const char *format, ...)
{
    int x;
    va_list ap;
    va_start(ap, format);
    x = Xvscnprintf(s, n, format, ap);
    va_end(ap);
    return x;
}

/* Old api, now deprecated, may be removed in the future */
char *
Xvprintf(const char *format, va_list va)
{
    char *ret;

    if (vasprintf(&ret, format, va) == -1)
        ret = NULL;

    return ret;
}

char *
Xprintf(const char *format, ...)
{
    char *ret;
    va_list va;

    va_start(va, format);
    if (vasprintf(&ret, format, va) == -1)
        ret = NULL;
    va_end(va);
    return ret;
}

char *
XNFvprintf(const char *format, va_list va)
{
    char *ret;

    XNFvasprintf(&ret, format, va);

    return ret;
}

char *
XNFprintf(const char *format, ...)
{
    char *ret;
    va_list va;

    va_start(va, format);
    XNFvasprintf(&ret, format, va);
    va_end(va);
    return ret;
}
