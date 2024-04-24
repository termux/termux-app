/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

/* Local replacements for functions found in some, but not all, libc's */
#ifndef XFONT_REPLACE_H
#define XFONT_REPLACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xfuncproto.h>

#include <stdlib.h>
#if defined(HAVE_LIBBSD) && defined(HAVE_REALLOCARRAY)
#include <bsd/stdlib.h>       /* for reallocarray */
#endif

#ifndef HAVE_REALLOCARRAY
extern _X_HIDDEN void *
reallocarray(void *optr, size_t nmemb, size_t size);
#endif

#ifndef mallocarray
#define mallocarray(n, s)	reallocarray(NULL, n, s)
#endif

#include <string.h>
#if defined(HAVE_LIBBSD) && defined(HAVE_STRLCPY)
#include <bsd/string.h>       /* for strlcpy, strlcat */
#endif

#ifndef HAVE_STRLCPY
extern _X_HIDDEN size_t
strlcpy(char *dst, const char *src, size_t siz);
extern _X_HIDDEN size_t
strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_ERR_H
#define err(eval, ...) do { \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  exit(eval); \
  } while (0)
#define vwarn(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  } while (0)
#endif

#ifndef HAVE_REALPATH
extern _X_HIDDEN char *
realpath(const char *path, char *resolved_path);
#endif

#endif /* XFONT_REPLACE_H */
