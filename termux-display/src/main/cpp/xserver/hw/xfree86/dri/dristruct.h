/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 */

#ifndef DRI_STRUCT_H
#define DRI_STRUCT_H

#include "xf86drm.h"
#include "xf86Crtc.h"

#define DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin) ((DRIDrawablePrivPtr) \
    dixLookupPrivate(&(pWin)->devPrivates, DRIWindowPrivKey))
#define DRI_DRAWABLE_PRIV_FROM_PIXMAP(pPix) ((DRIDrawablePrivPtr) \
    dixLookupPrivate(&(pPix)->devPrivates, DRIWindowPrivKey))

typedef struct _DRIDrawablePrivRec {
    drm_drawable_t hwDrawable;
    int drawableIndex;
    ScreenPtr pScreen;
    int refCount;
    int nrects;
} DRIDrawablePrivRec, *DRIDrawablePrivPtr;

struct _DRIContextPrivRec {
    drm_context_t hwContext;
    ScreenPtr pScreen;
    Bool valid3D;
    DRIContextFlags flags;
    void **pContextStore;
};

#define DRI_SCREEN_PRIV(pScreen) ((DRIScreenPrivPtr) \
    (dixPrivateKeyRegistered(DRIScreenPrivKey) ?			\
     dixLookupPrivate(&(pScreen)->devPrivates, DRIScreenPrivKey) : NULL))

#define DRI_SCREEN_PRIV_FROM_INDEX(screenIndex) ((DRIScreenPrivPtr) \
    dixLookupPrivate(&screenInfo.screens[screenIndex]->devPrivates, \
		     DRIScreenPrivKey))

#define DRI_ENT_PRIV(pScrn)  \
    ((DRIEntPrivIndex < 0) ? \
     NULL:		     \
     ((DRIEntPrivPtr)(xf86GetEntityPrivate((pScrn)->entityList[0], \
					   DRIEntPrivIndex)->ptr)))

typedef struct _DRIScreenPrivRec {
    Bool directRenderingSupport;
    int drmFD;                  /* File descriptor for /dev/video/?   */
    drm_handle_t hSAREA;        /* Handle to SAREA, for mapping       */
    XF86DRISAREAPtr pSAREA;     /* Mapped pointer to SAREA            */
    drm_context_t myContext;    /* DDX Driver's context               */
    DRIContextPrivPtr myContextPriv;    /* Pointer to server's private area   */
    DRIContextPrivPtr lastPartial3DContext;     /* last one partially saved  */
    void **hiddenContextStore;  /* hidden X context          */
    void **partial3DContextStore;       /* parital 3D context        */
    DRIInfoPtr pDriverInfo;
    int nrWindows;
    int nrWindowsVisible;
    int nrWalked;
    drm_clip_rect_t private_buffer_rect;        /* management of private buffers */
    DrawablePtr fullscreen;     /* pointer to fullscreen drawable */
    drm_clip_rect_t fullscreen_rect;    /* fake rect for fullscreen mode */
    DRIWrappedFuncsRec wrap;
    DestroyWindowProcPtr DestroyWindow;
    DrawablePtr DRIDrawables[SAREA_MAX_DRAWABLES];
    DRIContextPrivPtr dummyCtxPriv;     /* Pointer to dummy context */
    Bool createDummyCtx;
    Bool createDummyCtxPriv;
    Bool grabbedDRILock;
    Bool drmSIGIOHandlerInstalled;
    Bool wrapped;
    Bool windowsTouched;
    int lockRefCount;
    drm_handle_t hLSAREA;       /* Handle to SAREA containing lock, for mapping */
    XF86DRILSAREAPtr pLSAREA;   /* Mapped pointer to SAREA containing lock */
    int *pLockRefCount;
    int *pLockingContext;
    xf86_crtc_notify_proc_ptr xf86_crtc_notify;
} DRIScreenPrivRec, *DRIScreenPrivPtr;

typedef struct _DRIEntPrivRec {
    int drmFD;
    Bool drmOpened;
    Bool sAreaGrabbed;
    drm_handle_t hLSAREA;
    XF86DRILSAREAPtr pLSAREA;
    unsigned long sAreaSize;
    int lockRefCount;
    int lockingContext;
    ScreenPtr resOwner;
    Bool keepFDOpen;
    int refCount;
} DRIEntPrivRec, *DRIEntPrivPtr;

#endif                          /* DRI_STRUCT_H */
