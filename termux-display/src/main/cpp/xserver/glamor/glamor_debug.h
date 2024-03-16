/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
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
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#ifndef __GLAMOR_DEBUG_H__
#define __GLAMOR_DEBUG_H__

#define GLAMOR_DEBUG_NONE                     0
#define GLAMOR_DEBUG_UNIMPL                   0
#define GLAMOR_DEBUG_FALLBACK                 1
#define GLAMOR_DEBUG_TEXTURE_DOWNLOAD         2
#define GLAMOR_DEBUG_TEXTURE_DYNAMIC_UPLOAD   3

extern void
AbortServer(void)
    _X_NORETURN;

#define GLAMOR_PANIC(_format_, ...)			\
  do {							\
    LogMessageVerb(X_NONE, 0, "Glamor Fatal Error"	\
		   " at %32s line %d: " _format_ "\n",	\
		   __FUNCTION__, __LINE__,		\
		   ##__VA_ARGS__ );			\
    exit(1);                                            \
  } while(0)

#define __debug_output_message(_format_, _prefix_, ...) \
  LogMessageVerb(X_NONE, 0,				\
		 "%32s:\t" _format_ ,		\
		 /*_prefix_,*/				\
		 __FUNCTION__,				\
		 ##__VA_ARGS__)

#define glamor_debug_output(_level_, _format_,...)	\
  do {							\
    if (glamor_debug_level >= _level_)			\
      __debug_output_message(_format_,			\
			     "Glamor debug",		\
			     ##__VA_ARGS__);		\
  } while(0)

#define glamor_fallback(_format_,...)			\
  do {							\
    if (glamor_debug_level >= GLAMOR_DEBUG_FALLBACK)	\
      __debug_output_message(_format_,			\
			     "Glamor fallback",		\
			     ##__VA_ARGS__);} while(0)

#define DEBUGF(str, ...)  do {} while(0)
//#define DEBUGF(str, ...) ErrorF(str, ##__VA_ARGS__)
#define DEBUGRegionPrint(x) do {} while (0)
//#define DEBUGRegionPrint RegionPrint

#endif
