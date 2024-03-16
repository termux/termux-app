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

/* Prototypes for DRI functions */

#ifndef _DRI_H_

#include <pciaccess.h>

#include "scrnintstr.h"
#include "xf86dri.h"

typedef int DRISyncType;

#define DRI_NO_SYNC 0
#define DRI_2D_SYNC 1
#define DRI_3D_SYNC 2

typedef int DRIContextType;

typedef struct _DRIContextPrivRec DRIContextPrivRec, *DRIContextPrivPtr;

typedef enum _DRIContextFlags {
    DRI_CONTEXT_2DONLY = 0x01,
    DRI_CONTEXT_PRESERVED = 0x02,
    DRI_CONTEXT_RESERVED = 0x04 /* DRI Only -- no kernel equivalent */
} DRIContextFlags;

#define DRI_NO_CONTEXT 0
#define DRI_2D_CONTEXT 1
#define DRI_3D_CONTEXT 2

typedef int DRISwapMethod;

#define DRI_HIDE_X_CONTEXT 0
#define DRI_SERVER_SWAP    1
#define DRI_KERNEL_SWAP    2

typedef int DRIWindowRequests;

#define DRI_NO_WINDOWS       0
#define DRI_3D_WINDOWS_ONLY  1
#define DRI_ALL_WINDOWS      2

typedef void (*ClipNotifyPtr) (WindowPtr, int, int);
typedef void (*AdjustFramePtr) (ScrnInfoPtr pScrn, int x, int y);

/*
 * These functions can be wrapped by the DRI.  Each of these have
 * generic default funcs (initialized in DRICreateInfoRec) and can be
 * overridden by the driver in its [driver]DRIScreenInit function.
 */
typedef struct {
    ScreenWakeupHandlerProcPtr WakeupHandler;
    ScreenBlockHandlerProcPtr BlockHandler;
    WindowExposuresProcPtr WindowExposures;
    CopyWindowProcPtr CopyWindow;
    ClipNotifyProcPtr ClipNotify;
    AdjustFramePtr AdjustFrame;
} DRIWrappedFuncsRec, *DRIWrappedFuncsPtr;

/*
 * Prior to Xorg 6.8.99.8, the DRIInfoRec structure was implicitly versioned
 * by the XF86DRI_*_VERSION defines in xf86dristr.h.  These numbers were also
 * being used to version the XFree86-DRI protocol.  Bugs #3066 and #3163
 * showed that this was inadequate.  The DRIInfoRec structure is now versioned
 * by the DRIINFO_*_VERSION defines in this file. - ajax, 2005-05-18.
 *
 * Revision history:
 * 4.1.0 and earlier: DRIQueryVersion returns XF86DRI_*_VERSION.
 * 4.2.0: DRIQueryVersion begins returning DRIINFO_*_VERSION.
 * 5.0.0: frameBufferPhysicalAddress changed from CARD32 to pointer.
 */

#define DRIINFO_MAJOR_VERSION   5
#define DRIINFO_MINOR_VERSION   4
#define DRIINFO_PATCH_VERSION   0

typedef unsigned long long (*DRITexOffsetStartProcPtr) (PixmapPtr pPix);
typedef void (*DRITexOffsetFinishProcPtr) (PixmapPtr pPix);

typedef struct {
    /* driver call back functions
     *
     * New fields should be added at the end for backwards compatibility.
     * Bump the DRIINFO patch number to indicate bugfixes.
     * Bump the DRIINFO minor number to indicate new fields.
     * Bump the DRIINFO major number to indicate binary-incompatible changes.
     */
    Bool (*CreateContext) (ScreenPtr pScreen,
                           VisualPtr visual,
                           drm_context_t hHWContext,
                           void *pVisualConfigPriv, DRIContextType context);
    void (*DestroyContext) (ScreenPtr pScreen,
                            drm_context_t hHWContext, DRIContextType context);
    void (*SwapContext) (ScreenPtr pScreen,
                         DRISyncType syncType,
                         DRIContextType readContextType,
                         void *readContextStore,
                         DRIContextType writeContextType,
                         void *writeContextStore);
    void (*InitBuffers) (WindowPtr pWin, RegionPtr prgn, CARD32 indx);
    void (*MoveBuffers) (WindowPtr pWin,
                         DDXPointRec ptOldOrg, RegionPtr prgnSrc, CARD32 indx);
    void (*TransitionTo3d) (ScreenPtr pScreen);
    void (*TransitionTo2d) (ScreenPtr pScreen);

    void (*SetDrawableIndex) (WindowPtr pWin, CARD32 indx);
    Bool (*OpenFullScreen) (ScreenPtr pScreen);
    Bool (*CloseFullScreen) (ScreenPtr pScreen);

    /* wrapped functions */
    DRIWrappedFuncsRec wrap;

    /* device info */
    char *drmDriverName;
    char *clientDriverName;
    char *busIdString;
    int ddxDriverMajorVersion;
    int ddxDriverMinorVersion;
    int ddxDriverPatchVersion;
    void *frameBufferPhysicalAddress;
    long frameBufferSize;
    long frameBufferStride;
    long SAREASize;
    int maxDrawableTableEntry;
    int ddxDrawableTableEntry;
    long contextSize;
    DRISwapMethod driverSwapMethod;
    DRIWindowRequests bufferRequests;
    int devPrivateSize;
    void *devPrivate;
    Bool createDummyCtx;
    Bool createDummyCtxPriv;

    /* New with DRI version 4.1.0 */
    void (*TransitionSingleToMulti3D) (ScreenPtr pScreen);
    void (*TransitionMultiToSingle3D) (ScreenPtr pScreen);

    /* New with DRI version 5.1.0 */
    void (*ClipNotify) (ScreenPtr pScreen, WindowPtr *ppWin, int num);

    /* New with DRI version 5.2.0 */
    Bool allocSarea;
    Bool keepFDOpen;

    /* New with DRI version 5.3.0 */
    DRITexOffsetStartProcPtr texOffsetStart;
    DRITexOffsetFinishProcPtr texOffsetFinish;

    /* New with DRI version 5.4.0 */
    int dontMapFrameBuffer;
    drm_handle_t hFrameBuffer;  /* Handle to framebuffer, either
                                 * mapped by DDX driver or DRI */

} DRIInfoRec, *DRIInfoPtr;

extern _X_EXPORT Bool DRIOpenDRMMaster(ScrnInfoPtr pScrn,
                                       unsigned long sAreaSize,
                                       const char *busID,
                                       const char *drmDriverName);

extern _X_EXPORT Bool DRIScreenInit(ScreenPtr pScreen,
                                    DRIInfoPtr pDRIInfo, int *pDRMFD);

extern _X_EXPORT void DRICloseScreen(ScreenPtr pScreen);

extern Bool DRIExtensionInit(void);

extern _X_EXPORT void DRIReset(void);

extern _X_EXPORT Bool DRIQueryDirectRenderingCapable(ScreenPtr pScreen,
                                                     Bool *isCapable);

extern _X_EXPORT Bool DRIOpenConnection(ScreenPtr pScreen,
                                        drm_handle_t * hSAREA,
                                        char **busIdString);

extern _X_EXPORT Bool DRIAuthConnection(ScreenPtr pScreen, drm_magic_t magic);

extern _X_EXPORT Bool DRICloseConnection(ScreenPtr pScreen);

extern _X_EXPORT Bool DRIGetClientDriverName(ScreenPtr pScreen,
                                             int *ddxDriverMajorVersion,
                                             int *ddxDriverMinorVersion,
                                             int *ddxDriverPatchVersion,
                                             char **clientDriverName);

extern _X_EXPORT Bool DRICreateContext(ScreenPtr pScreen,
                                       VisualPtr visual,
                                       XID context, drm_context_t * pHWContext);

extern _X_EXPORT Bool DRIDestroyContext(ScreenPtr pScreen, XID context);

extern _X_EXPORT Bool DRIContextPrivDelete(void *pResource, XID id);

extern _X_EXPORT Bool DRICreateDrawable(ScreenPtr pScreen,
                                        ClientPtr client,
                                        DrawablePtr pDrawable,
                                        drm_drawable_t * hHWDrawable);

extern _X_EXPORT Bool DRIDestroyDrawable(ScreenPtr pScreen,
                                         ClientPtr client,
                                         DrawablePtr pDrawable);

extern _X_EXPORT Bool DRIDrawablePrivDelete(void *pResource, XID id);

extern _X_EXPORT Bool DRIGetDrawableInfo(ScreenPtr pScreen,
                                         DrawablePtr pDrawable,
                                         unsigned int *indx,
                                         unsigned int *stamp,
                                         int *X,
                                         int *Y,
                                         int *W,
                                         int *H,
                                         int *numClipRects,
                                         drm_clip_rect_t ** pClipRects,
                                         int *backX,
                                         int *backY,
                                         int *numBackClipRects,
                                         drm_clip_rect_t ** pBackClipRects);

extern _X_EXPORT Bool DRIGetDeviceInfo(ScreenPtr pScreen,
                                       drm_handle_t * hFrameBuffer,
                                       int *fbOrigin,
                                       int *fbSize,
                                       int *fbStride,
                                       int *devPrivateSize, void **pDevPrivate);

extern _X_EXPORT DRIInfoPtr DRICreateInfoRec(void);

extern _X_EXPORT void DRIDestroyInfoRec(DRIInfoPtr DRIInfo);

extern _X_EXPORT Bool DRIFinishScreenInit(ScreenPtr pScreen);

extern _X_EXPORT void DRIWakeupHandler(void *wakeupData, int result);

extern _X_EXPORT void DRIBlockHandler(void *blockData, void *timeout);

extern _X_EXPORT void DRIDoWakeupHandler(ScreenPtr pScreen, int result);

extern _X_EXPORT void DRIDoBlockHandler(ScreenPtr pScreen, void *timeout);

extern _X_EXPORT void DRISwapContext(int drmFD, void *oldctx, void *newctx);

extern _X_EXPORT void *DRIGetContextStore(DRIContextPrivPtr context);

extern _X_EXPORT void DRIWindowExposures(WindowPtr pWin, RegionPtr prgn);

extern _X_EXPORT Bool DRIDestroyWindow(WindowPtr pWin);

extern _X_EXPORT void DRICopyWindow(WindowPtr pWin,
                                    DDXPointRec ptOldOrg, RegionPtr prgnSrc);

extern _X_EXPORT void DRIClipNotify(WindowPtr pWin, int dx, int dy);

extern _X_EXPORT CARD32 DRIGetDrawableIndex(WindowPtr pWin);

extern _X_EXPORT void DRIPrintDrawableLock(ScreenPtr pScreen, char *msg);

extern _X_EXPORT void DRILock(ScreenPtr pScreen, int flags);

extern _X_EXPORT void DRIUnlock(ScreenPtr pScreen);

extern _X_EXPORT DRIWrappedFuncsRec *DRIGetWrappedFuncs(ScreenPtr pScreen);

extern _X_EXPORT void *DRIGetSAREAPrivate(ScreenPtr pScreen);

extern _X_EXPORT unsigned int DRIGetDrawableStamp(ScreenPtr pScreen,
                                                  CARD32 drawable_index);

extern _X_EXPORT DRIContextPrivPtr DRICreateContextPriv(ScreenPtr pScreen,
                                                        drm_context_t *
                                                        pHWContext,
                                                        DRIContextFlags flags);

extern _X_EXPORT DRIContextPrivPtr DRICreateContextPrivFromHandle(ScreenPtr
                                                                  pScreen,
                                                                  drm_context_t
                                                                  hHWContext,
                                                                  DRIContextFlags
                                                                  flags);

extern _X_EXPORT Bool DRIDestroyContextPriv(DRIContextPrivPtr pDRIContextPriv);

extern _X_EXPORT drm_context_t DRIGetContext(ScreenPtr pScreen);

extern _X_EXPORT void DRIQueryVersion(int *majorVersion,
                                      int *minorVersion, int *patchVersion);

extern _X_EXPORT void DRIAdjustFrame(ScrnInfoPtr pScrn, int x, int y);

extern _X_EXPORT void DRIMoveBuffersHelper(ScreenPtr pScreen,
                                           int dx,
                                           int dy,
                                           int *xdir, int *ydir, RegionPtr reg);

extern _X_EXPORT int DRIMasterFD(ScrnInfoPtr pScrn);

extern _X_EXPORT void *DRIMasterSareaPointer(ScrnInfoPtr pScrn);

extern _X_EXPORT drm_handle_t DRIMasterSareaHandle(ScrnInfoPtr pScrn);

extern _X_EXPORT void DRIGetTexOffsetFuncs(ScreenPtr pScreen,
                                           DRITexOffsetStartProcPtr *
                                           texOffsetStartFunc,
                                           DRITexOffsetFinishProcPtr *
                                           texOffsetFinishFunc);

#define _DRI_H_

#endif
