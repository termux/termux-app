/*
 * Xplugin rootless implementation
 *
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
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

#ifndef XPR_H
#define XPR_H

#include "windowstr.h"
#include "screenint.h"
#include <Xplugin.h>

#include "darwin.h"

#undef DEBUG_LOG
#define DEBUG_LOG(msg, args ...) ASL_LOG(ASL_LEVEL_DEBUG, "xpr", msg, ## args)

Bool
QuartzModeBundleInit(void);

void
AppleDRIExtensionInit(void);
void
xprAppleWMInit(void);
Bool
xprInit(ScreenPtr pScreen);
Bool
xprIsX11Window(int windowNumber);
WindowPtr
xprGetXWindow(xp_window_id wid);

void
xprHideWindows(Bool hide);

Bool
QuartzInitCursor(ScreenPtr pScreen);
void
QuartzSuspendXCursor(ScreenPtr pScreen);
void
QuartzResumeXCursor(ScreenPtr pScreen);

/* If we are rooted, we need the root window and desktop levels to be below
 * the menubar (24) but above native windows.  Normal window level is 0.
 * Floating window level is 3.  The rest are filled in as appropriate.
 * See CGWindowLevel.h
 */

#include <X11/extensions/applewmconst.h>
static const int normal_window_levels[AppleWMNumWindowLevels + 1] = {
    0, 3, 4, 5, INT_MIN + 30, INT_MIN + 29,
};
static const int rooted_window_levels[AppleWMNumWindowLevels + 1] = {
    20, 21, 22, 23, 19, 18,
};

#endif /* XPR_H */
