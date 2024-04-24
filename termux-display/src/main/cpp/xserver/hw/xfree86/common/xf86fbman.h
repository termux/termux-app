
/*
 * Copyright (c) 1998-2001 by The XFree86 Project, Inc.
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

#ifndef _XF86FBMAN_H
#define _XF86FBMAN_H

#include "scrnintstr.h"
#include "regionstr.h"

#define FAVOR_AREA_THEN_WIDTH		0
#define FAVOR_AREA_THEN_HEIGHT		1
#define FAVOR_WIDTH_THEN_AREA		2
#define FAVOR_HEIGHT_THEN_AREA		3

#define PRIORITY_LOW			0
#define PRIORITY_NORMAL			1
#define PRIORITY_EXTREME		2

typedef struct _FBArea {
    ScreenPtr pScreen;
    BoxRec box;
    int granularity;
    void (*MoveAreaCallback) (struct _FBArea *, struct _FBArea *);
    void (*RemoveAreaCallback) (struct _FBArea *);
    DevUnion devPrivate;
} FBArea, *FBAreaPtr;

typedef struct _FBLinear {
    ScreenPtr pScreen;
    int size;
    int offset;
    int granularity;
    void (*MoveLinearCallback) (struct _FBLinear *, struct _FBLinear *);
    void (*RemoveLinearCallback) (struct _FBLinear *);
    DevUnion devPrivate;
} FBLinear, *FBLinearPtr;

typedef void (*FreeBoxCallbackProcPtr) (ScreenPtr, RegionPtr, void *);
typedef void (*MoveAreaCallbackProcPtr) (FBAreaPtr, FBAreaPtr);
typedef void (*RemoveAreaCallbackProcPtr) (FBAreaPtr);

typedef void (*MoveLinearCallbackProcPtr) (FBLinearPtr, FBLinearPtr);
typedef void (*RemoveLinearCallbackProcPtr) (FBLinearPtr);

typedef struct {
    FBAreaPtr(*AllocateOffscreenArea) (ScreenPtr pScreen,
                                       int w, int h,
                                       int granularity,
                                       MoveAreaCallbackProcPtr moveCB,
                                       RemoveAreaCallbackProcPtr removeCB,
                                       void *privData);
    void (*FreeOffscreenArea) (FBAreaPtr area);
    Bool (*ResizeOffscreenArea) (FBAreaPtr area, int w, int h);
    Bool (*QueryLargestOffscreenArea) (ScreenPtr pScreen,
                                       int *width, int *height,
                                       int granularity,
                                       int preferences, int priority);
    Bool (*RegisterFreeBoxCallback) (ScreenPtr pScreen,
                                     FreeBoxCallbackProcPtr FreeBoxCallback,
                                     void *devPriv);
/* linear functions */
     FBLinearPtr(*AllocateOffscreenLinear) (ScreenPtr pScreen,
                                            int size,
                                            int granularity,
                                            MoveLinearCallbackProcPtr moveCB,
                                            RemoveLinearCallbackProcPtr
                                            removeCB, void *privData);
    void (*FreeOffscreenLinear) (FBLinearPtr area);
    Bool (*ResizeOffscreenLinear) (FBLinearPtr area, int size);
    Bool (*QueryLargestOffscreenLinear) (ScreenPtr pScreen,
                                         int *size,
                                         int granularity, int priority);
    Bool (*PurgeOffscreenAreas) (ScreenPtr);
} FBManagerFuncs, *FBManagerFuncsPtr;

extern _X_EXPORT Bool xf86RegisterOffscreenManager(ScreenPtr pScreen,
                                                   FBManagerFuncsPtr funcs);

extern _X_EXPORT Bool
 xf86InitFBManagerRegion(ScreenPtr pScreen, RegionPtr ScreenRegion);

extern _X_EXPORT Bool
 xf86InitFBManagerArea(ScreenPtr pScreen, int PixalArea, int Verbosity);

extern _X_EXPORT Bool
 xf86InitFBManager(ScreenPtr pScreen, BoxPtr FullBox);

extern _X_EXPORT Bool
 xf86InitFBManagerLinear(ScreenPtr pScreen, int offset, int size);

extern _X_EXPORT Bool
 xf86FBManagerRunning(ScreenPtr pScreen);

extern _X_EXPORT FBAreaPtr
xf86AllocateOffscreenArea(ScreenPtr pScreen,
                          int w, int h,
                          int granularity,
                          MoveAreaCallbackProcPtr moveCB,
                          RemoveAreaCallbackProcPtr removeCB, void *privData);

extern _X_EXPORT FBAreaPtr
xf86AllocateLinearOffscreenArea(ScreenPtr pScreen,
                                int length,
                                int granularity,
                                MoveAreaCallbackProcPtr moveCB,
                                RemoveAreaCallbackProcPtr removeCB,
                                void *privData);

extern _X_EXPORT FBLinearPtr
xf86AllocateOffscreenLinear(ScreenPtr pScreen,
                            int length,
                            int granularity,
                            MoveLinearCallbackProcPtr moveCB,
                            RemoveLinearCallbackProcPtr removeCB,
                            void *privData);

extern _X_EXPORT void xf86FreeOffscreenArea(FBAreaPtr area);
extern _X_EXPORT void xf86FreeOffscreenLinear(FBLinearPtr area);

extern _X_EXPORT Bool
 xf86ResizeOffscreenArea(FBAreaPtr resize, int w, int h);

extern _X_EXPORT Bool
 xf86ResizeOffscreenLinear(FBLinearPtr resize, int size);

extern _X_EXPORT Bool

xf86RegisterFreeBoxCallback(ScreenPtr pScreen,
                            FreeBoxCallbackProcPtr FreeBoxCallback,
                            void *devPriv);

extern _X_EXPORT Bool
 xf86PurgeUnlockedOffscreenAreas(ScreenPtr pScreen);

extern _X_EXPORT Bool

xf86QueryLargestOffscreenArea(ScreenPtr pScreen,
                              int *width, int *height,
                              int granularity, int preferences, int priority);

extern _X_EXPORT Bool

xf86QueryLargestOffscreenLinear(ScreenPtr pScreen,
                                int *size, int granularity, int priority);

#endif                          /* _XF86FBMAN_H */
