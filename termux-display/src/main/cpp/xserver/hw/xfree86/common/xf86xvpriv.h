
/*
 * Copyright (c) 2003 by The XFree86 Project, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef _XF86XVPRIV_H_
#define _XF86XVPRIV_H_

#include "xf86xv.h"
#include "privates.h"

/*** These are DDX layer privates ***/

extern _X_EXPORT DevPrivateKey XF86XvScreenKey;

typedef struct {
    DestroyWindowProcPtr DestroyWindow;
    ClipNotifyProcPtr ClipNotify;
    WindowExposuresProcPtr WindowExposures;
    PostValidateTreeProcPtr PostValidateTree;
    void (*AdjustFrame) (ScrnInfoPtr, int, int);
    Bool (*EnterVT) (ScrnInfoPtr);
    void (*LeaveVT) (ScrnInfoPtr);
    xf86ModeSetProc *ModeSet;
    CloseScreenProcPtr CloseScreen;
} XF86XVScreenRec, *XF86XVScreenPtr;

typedef struct {
    int flags;
    PutVideoFuncPtr PutVideo;
    PutStillFuncPtr PutStill;
    GetVideoFuncPtr GetVideo;
    GetStillFuncPtr GetStill;
    StopVideoFuncPtr StopVideo;
    SetPortAttributeFuncPtr SetPortAttribute;
    GetPortAttributeFuncPtr GetPortAttribute;
    QueryBestSizeFuncPtr QueryBestSize;
    PutImageFuncPtr PutImage;
    ReputImageFuncPtr ReputImage;
    QueryImageAttributesFuncPtr QueryImageAttributes;
} XvAdaptorRecPrivate, *XvAdaptorRecPrivatePtr;

typedef struct {
    ScrnInfoPtr pScrn;
    DrawablePtr pDraw;
    unsigned char type;
    unsigned int subWindowMode;
    RegionPtr clientClip;
    RegionPtr ckeyFilled;
    RegionPtr pCompositeClip;
    Bool FreeCompositeClip;
    XvAdaptorRecPrivatePtr AdaptorRec;
    XvStatus isOn;
    Bool clipChanged;
    int vid_x, vid_y, vid_w, vid_h;
    int drw_x, drw_y, drw_w, drw_h;
    DevUnion DevPriv;
} XvPortRecPrivate, *XvPortRecPrivatePtr;

typedef struct _XF86XVWindowRec {
    XvPortRecPrivatePtr PortRec;
    struct _XF86XVWindowRec *next;
} XF86XVWindowRec, *XF86XVWindowPtr;

#endif                          /* _XF86XVPRIV_H_ */
