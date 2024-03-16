/*
 * Copyright (C) 2000 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 */

#ifndef _MICOORD_H_
#define _MICOORD_H_ 1

#include "servermd.h"

/* Macros which handle a coordinate in a single register */

#define GetHighWord(x) (((int) (x)) >> 16)

#if IMAGE_BYTE_ORDER == MSBFirst
#define intToCoord(i,x,y)   (((x) = GetHighWord(i)), ((y) = (int) ((short) (i))))
#define coordToInt(x,y)	(((x) << 16) | ((y) & 0xffff))
#define intToX(i)	(GetHighWord(i))
#define intToY(i)	((int) ((short) i))
#else
#define intToCoord(i,x,y)   (((x) = (int) ((short) (i))), ((y) = GetHighWord(i)))
#define coordToInt(x,y)	(((y) << 16) | ((x) & 0xffff))
#define intToX(i)	((int) ((short) (i)))
#define intToY(i)	(GetHighWord(i))
#endif

#endif                          /* _MICOORD_H_ */
