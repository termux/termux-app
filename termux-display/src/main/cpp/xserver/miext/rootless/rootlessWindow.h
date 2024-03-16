/*
 * Rootless window management
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _ROOTLESSWINDOW_H
#define _ROOTLESSWINDOW_H

#include "rootlessCommon.h"

Bool RootlessCreateWindow(WindowPtr pWin);
Bool RootlessDestroyWindow(WindowPtr pWin);

void RootlessSetShape(WindowPtr pWin, int kind);

Bool RootlessChangeWindowAttributes(WindowPtr pWin, unsigned long vmask);
Bool RootlessPositionWindow(WindowPtr pWin, int x, int y);
Bool RootlessRealizeWindow(WindowPtr pWin);
Bool RootlessUnrealizeWindow(WindowPtr pWin);
void RootlessRestackWindow(WindowPtr pWin, WindowPtr pOldNextSib);
void RootlessCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                        RegionPtr prgnSrc);
void RootlessPaintWindow(WindowPtr pWin, RegionPtr prgn, int what);
void RootlessMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pSib,
                        VTKind kind);
void RootlessResizeWindow(WindowPtr pWin, int x, int y, unsigned int w,
                          unsigned int h, WindowPtr pSib);
void RootlessReparentWindow(WindowPtr pWin, WindowPtr pPriorParent);
void RootlessChangeBorderWidth(WindowPtr pWin, unsigned int width);

#ifdef __APPLE__
void RootlessNativeWindowMoved(WindowPtr pWin);
void RootlessNativeWindowStateChanged(WindowPtr pWin, unsigned int state);
#endif

#endif
