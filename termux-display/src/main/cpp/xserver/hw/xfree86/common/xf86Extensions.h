/*
 * Copyright © 2011 Daniel Stone
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
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef XF86EXTENSIONS_H
#define XF86EXTENSIONS_H

#include "extnsionst.h"

#ifdef XF86DRI
extern _X_EXPORT Bool noXFree86DRIExtension;
extern void XFree86DRIExtensionInit(void);
#endif

#ifdef DRI2
#include <X11/extensions/dri2proto.h>
extern _X_EXPORT Bool noDRI2Extension;
extern void DRI2ExtensionInit(void);
#endif

#ifdef XF86VIDMODE
#include <X11/extensions/xf86vmproto.h>
extern _X_EXPORT Bool noXFree86VidModeExtension;
extern void XFree86VidModeExtensionInit(void);
#endif

#ifdef XFreeXDGA
#include <X11/extensions/xf86dgaproto.h>
extern _X_EXPORT Bool noXFree86DGAExtension;
extern void XFree86DGAExtensionInit(void);
extern void XFree86DGARegister(void);
#endif

#endif
