#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _glxext_h_
#define _glxext_h_

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/* doing #include <GL/glx.h> & #include <GL/glxext.h> could cause problems
 * with overlapping definitions, so let's use the easy way
 */
#ifndef GLX_RGBA_FLOAT_BIT_ARB
#define GLX_RGBA_FLOAT_BIT_ARB             0x00000004
#endif
#ifndef GLX_RGBA_FLOAT_TYPE_ARB
#define GLX_RGBA_FLOAT_TYPE_ARB            0x20B9
#endif
#ifndef GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT
#define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT    0x00000008
#endif
#ifndef GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT
#define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT   0x20B1
#endif
#ifndef GLX_CONTEXT_RELEASE_BEHAVIOR_ARB
#define GLX_CONTEXT_RELEASE_BEHAVIOR_ARB   0x2097
#define GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098
#endif

extern void __glXFlushContextCache(void);

extern Bool __glXAddContext(__GLXcontext * cx);
extern void __glXErrorCallBack(GLenum code);
extern void __glXClearErrorOccured(void);
extern GLboolean __glXErrorOccured(void);

extern const char GLServerVersion[];
extern int DoGetString(__GLXclientState * cl, GLbyte * pc, GLboolean need_swap);

extern int
xorgGlxMakeCurrent(ClientPtr client, GLXContextTag tag, XID drawId, XID readId,
                   XID contextId, GLXContextTag newContextTag);

#endif                          /* _glxext_h_ */
