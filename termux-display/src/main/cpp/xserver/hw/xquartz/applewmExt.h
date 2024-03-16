/* External interface for the server's AppleWM support
 *
 * Copyright (c) 2003-2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#ifndef _APPLEWMEXT_H_
#define _APPLEWMEXT_H_

#include "window.h"
#include <Xplugin.h>

typedef int (*DisableUpdateProc)(void);
typedef int (*EnableUpdateProc)(void);
typedef int (*SetWindowLevelProc)(WindowPtr pWin, int level);
typedef int (*FrameGetRectProc)(xp_frame_rect type, xp_frame_class class,
                                const BoxRec *outer,
                                const BoxRec *inner, BoxRec *ret);
typedef int (*FrameHitTestProc)(xp_frame_class class, int x, int y,
                                const BoxRec *outer,
                                const BoxRec *inner, int *ret);
typedef int (*FrameDrawProc)(WindowPtr pWin, xp_frame_class class,
                             xp_frame_attr attr,
                             const BoxRec *outer, const BoxRec *inner,
                             unsigned int title_len,
                             const unsigned char *title_bytes);
typedef int (*SendPSNProc)(uint32_t hi, uint32_t lo);
typedef int (*AttachTransientProc)(WindowPtr pWinChild, WindowPtr pWinParent);

/*
 * AppleWM implementation function list
 */
typedef struct _AppleWMProcs {
    DisableUpdateProc DisableUpdate;
    EnableUpdateProc EnableUpdate;
    SetWindowLevelProc SetWindowLevel;
    FrameGetRectProc FrameGetRect;
    FrameHitTestProc FrameHitTest;
    FrameDrawProc FrameDraw;
    SendPSNProc SendPSN;
    AttachTransientProc AttachTransient;
} AppleWMProcsRec, *AppleWMProcsPtr;

void
AppleWMExtensionInit(AppleWMProcsPtr procsPtr);

void
AppleWMSetScreenOrigin(WindowPtr pWin);

Bool
AppleWMDoReorderWindow(WindowPtr pWin);

void
AppleWMSendEvent(int /* type */, unsigned int /* mask */, int /* which */,
                 int                  /* arg */
                 );

unsigned int
AppleWMSelectedEvents(void);

#endif /* _APPLEWMEXT_H_ */
