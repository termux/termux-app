/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
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
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include "xf86drm.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#define _XF86DRI_SERVER_
#include <X11/dri/xf86driproto.h>
#include "swaprep.h"
#include "xf86str.h"
#include "dri.h"
#include "sarea.h"
#include "dristruct.h"
#include "mi.h"
#include "mipointer.h"
#include "xf86_OSproc.h"
#include "inputstr.h"
#include "xf86VGAarbiter.h"
#include "xf86Extensions.h"

static int DRIEntPrivIndex = -1;
static DevPrivateKeyRec DRIScreenPrivKeyRec;

#define DRIScreenPrivKey (&DRIScreenPrivKeyRec)
static DevPrivateKeyRec DRIWindowPrivKeyRec;

#define DRIWindowPrivKey (&DRIWindowPrivKeyRec)
static unsigned long DRIGeneration = 0;
static unsigned int DRIDrawableValidationStamp = 0;

static RESTYPE DRIDrawablePrivResType;
static RESTYPE DRIContextPrivResType;
static void DRIDestroyDummyContext(ScreenPtr pScreen, Bool hasCtxPriv);

drmServerInfo DRIDRMServerInfo;

                                /* Wrapper just like xf86DrvMsg, but
                                   without the verbosity level checking.
                                   This will make it easy to turn off some
                                   messages later, based on verbosity
                                   level. */

/*
 * Since we're already referencing things from the XFree86 common layer in
 * this file, we'd might as well just call xf86VDrvMsgVerb, and have
 * consistent message formatting.  The verbosity of these messages can be
 * easily changed here.
 */
#define DRI_MSG_VERBOSITY 1

static void
DRIDrvMsg(int scrnIndex, MessageType type, const char *format, ...)
    _X_ATTRIBUTE_PRINTF(3,4);

static void
DRIDrvMsg(int scrnIndex, MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, type, DRI_MSG_VERBOSITY, format, ap);
    va_end(ap);
}

static void
DRIOpenDRMCleanup(DRIEntPrivPtr pDRIEntPriv)
{
    if (pDRIEntPriv->pLSAREA != NULL) {
        drmUnmap(pDRIEntPriv->pLSAREA, pDRIEntPriv->sAreaSize);
        pDRIEntPriv->pLSAREA = NULL;
    }
    if (pDRIEntPriv->hLSAREA != 0) {
        drmRmMap(pDRIEntPriv->drmFD, pDRIEntPriv->hLSAREA);
    }
    if (pDRIEntPriv->drmFD >= 0) {
        drmClose(pDRIEntPriv->drmFD);
        pDRIEntPriv->drmFD = 0;
    }
}

int
DRIMasterFD(ScrnInfoPtr pScrn)
{
    return DRI_ENT_PRIV(pScrn)->drmFD;
}

void *
DRIMasterSareaPointer(ScrnInfoPtr pScrn)
{
    return DRI_ENT_PRIV(pScrn)->pLSAREA;
}

drm_handle_t
DRIMasterSareaHandle(ScrnInfoPtr pScrn)
{
    return DRI_ENT_PRIV(pScrn)->hLSAREA;
}

Bool
DRIOpenDRMMaster(ScrnInfoPtr pScrn,
                 unsigned long sAreaSize,
                 const char *busID, const char *drmDriverName)
{
    drmSetVersion saveSv, sv;
    Bool drmWasAvailable;
    DRIEntPrivPtr pDRIEntPriv;
    DRIEntPrivRec tmp;
    int count;
    int err;

    if (DRIEntPrivIndex == -1)
        DRIEntPrivIndex = xf86AllocateEntityPrivateIndex();

    pDRIEntPriv = DRI_ENT_PRIV(pScrn);

    if (pDRIEntPriv && pDRIEntPriv->drmFD != -1)
        return TRUE;

    drmWasAvailable = drmAvailable();

    memset(&tmp, 0, sizeof(tmp));

    tmp.drmFD = -1;
    sv.drm_di_major = 1;
    sv.drm_di_minor = 1;
    sv.drm_dd_major = -1;

    saveSv = sv;
    count = 10;
    while (count--) {
        tmp.drmFD = drmOpen(drmDriverName, busID);

        if (tmp.drmFD < 0) {
            DRIDrvMsg(-1, X_ERROR, "[drm] drmOpen failed.\n");
            goto out_err;
        }

        err = drmSetInterfaceVersion(tmp.drmFD, &sv);

        if (err != -EPERM)
            break;

        sv = saveSv;
        drmClose(tmp.drmFD);
        tmp.drmFD = -1;
        usleep(100000);
    }

    if (tmp.drmFD <= 0) {
        DRIDrvMsg(-1, X_ERROR, "[drm] DRM was busy with another master.\n");
        goto out_err;
    }

    if (!drmWasAvailable) {
        DRIDrvMsg(-1, X_INFO,
                  "[drm] loaded kernel module for \"%s\" driver.\n",
                  drmDriverName);
    }

    if (err != 0) {
        sv.drm_di_major = 1;
        sv.drm_di_minor = 0;
    }

    DRIDrvMsg(-1, X_INFO, "[drm] DRM interface version %d.%d\n",
              sv.drm_di_major, sv.drm_di_minor);

    if (sv.drm_di_major == 1 && sv.drm_di_minor >= 1)
        err = 0;
    else
        err = drmSetBusid(tmp.drmFD, busID);

    if (err) {
        DRIDrvMsg(-1, X_ERROR, "[drm] Could not set DRM device bus ID.\n");
        goto out_err;
    }

    /*
     * Create a lock-containing sarea.
     */

    if (drmAddMap(tmp.drmFD, 0, sAreaSize, DRM_SHM,
                  DRM_CONTAINS_LOCK, &tmp.hLSAREA) < 0) {
        DRIDrvMsg(-1, X_INFO, "[drm] Could not create SAREA for DRM lock.\n");
        tmp.hLSAREA = 0;
        goto out_err;
    }

    if (drmMap(tmp.drmFD, tmp.hLSAREA, sAreaSize,
               (drmAddressPtr) (&tmp.pLSAREA)) < 0) {
        DRIDrvMsg(-1, X_INFO, "[drm] Mapping SAREA for DRM lock failed.\n");
        tmp.pLSAREA = NULL;
        goto out_err;
    }

    memset(tmp.pLSAREA, 0, sAreaSize);

    /*
     * Reserved contexts are handled by the first opened screen.
     */

    tmp.resOwner = NULL;

    if (!pDRIEntPriv)
        pDRIEntPriv = xnfcalloc(sizeof(*pDRIEntPriv), 1);

    if (!pDRIEntPriv) {
        DRIDrvMsg(-1, X_INFO, "[drm] Failed to allocate memory for "
                  "DRM device.\n");
        goto out_err;
    }
    *pDRIEntPriv = tmp;
    xf86GetEntityPrivate((pScrn)->entityList[0], DRIEntPrivIndex)->ptr =
        pDRIEntPriv;

    DRIDrvMsg(-1, X_INFO, "[drm] DRM open master succeeded.\n");
    return TRUE;

 out_err:

    DRIOpenDRMCleanup(&tmp);
    return FALSE;
}

static void
 DRIClipNotifyAllDrawables(ScreenPtr pScreen);

static void
dri_crtc_notify(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    DRIClipNotifyAllDrawables(pScreen);
    xf86_unwrap_crtc_notify(pScreen, pDRIPriv->xf86_crtc_notify);
    xf86_crtc_notify(pScreen);
    pDRIPriv->xf86_crtc_notify =
        xf86_wrap_crtc_notify(pScreen, dri_crtc_notify);
}

static void
drmSIGIOHandler(int interrupt, void *closure)
{
    unsigned long key;
    void *value;
    ssize_t count;
    drm_ctx_t ctx;
    typedef void (*_drmCallback) (int, void *, void *);
    char buf[256];
    drm_context_t old;
    drm_context_t new;
    void *oldctx;
    void *newctx;
    char *pt;
    drmHashEntry *entry;
    void *hash_table;

    hash_table = drmGetHashTable();

    if (!hash_table)
        return;
    if (drmHashFirst(hash_table, &key, &value)) {
        entry = value;
        do {
            if ((count = read(entry->fd, buf, sizeof(buf) - 1)) > 0) {
                buf[count] = '\0';

                for (pt = buf; *pt != ' '; ++pt);       /* Find first space */
                ++pt;
                old = strtol(pt, &pt, 0);
                new = strtol(pt, NULL, 0);
                oldctx = drmGetContextTag(entry->fd, old);
                newctx = drmGetContextTag(entry->fd, new);
                ((_drmCallback) entry->f) (entry->fd, oldctx, newctx);
                ctx.handle = new;
                ioctl(entry->fd, DRM_IOCTL_NEW_CTX, &ctx);
            }
        } while (drmHashNext(hash_table, &key, &value));
    }
}

static int
drmInstallSIGIOHandler(int fd, void (*f) (int, void *, void *))
{
    drmHashEntry *entry;

    entry = drmGetEntry(fd);
    entry->f = f;

    return xf86InstallSIGIOHandler(fd, drmSIGIOHandler, 0);
}

static int
drmRemoveSIGIOHandler(int fd)
{
    drmHashEntry *entry = drmGetEntry(fd);

    entry->f = NULL;

    return xf86RemoveSIGIOHandler(fd);
}

Bool
DRIScreenInit(ScreenPtr pScreen, DRIInfoPtr pDRIInfo, int *pDRMFD)
{
    DRIScreenPrivPtr pDRIPriv;
    drm_context_t *reserved;
    int reserved_count;
    int i;
    DRIEntPrivPtr pDRIEntPriv;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DRIContextFlags flags = 0;
    DRIContextPrivPtr pDRIContextPriv;
    static Bool drm_server_inited;

    /* If the DRI extension is disabled, do not initialize the DRI */
    if (noXFree86DRIExtension) {
        DRIDrvMsg(pScreen->myNum, X_WARNING,
                  "Direct rendering has been disabled.\n");
        return FALSE;
    }

    if (!xf86VGAarbiterAllowDRI(pScreen)) {
        DRIDrvMsg(pScreen->myNum, X_WARNING,
                  "Direct rendering is not supported when VGA arb is necessary for the device\n");
        return FALSE;
    }

#ifdef PANORAMIX
    /*
     * If Xinerama is on, don't allow DRI to initialise.  It won't be usable
     * anyway.
     */
    if (!noPanoramiXExtension) {
        DRIDrvMsg(pScreen->myNum, X_WARNING,
                  "Direct rendering is not supported when Xinerama is enabled\n");
        return FALSE;
    }
#endif
    if (drm_server_inited == FALSE) {
        drmSetServerInfo(&DRIDRMServerInfo);
        drm_server_inited = TRUE;
    }

    if (!DRIOpenDRMMaster(pScrn, pDRIInfo->SAREASize,
                          pDRIInfo->busIdString, pDRIInfo->drmDriverName))
        return FALSE;

    pDRIEntPriv = DRI_ENT_PRIV(pScrn);

    if (DRIGeneration != serverGeneration)
        DRIGeneration = serverGeneration;

    if (!dixRegisterPrivateKey(&DRIScreenPrivKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&DRIWindowPrivKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;

    pDRIPriv = (DRIScreenPrivPtr) calloc(1, sizeof(DRIScreenPrivRec));
    if (!pDRIPriv) {
        dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
        return FALSE;
    }

    dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, pDRIPriv);
    pDRIPriv->drmFD = pDRIEntPriv->drmFD;
    pDRIPriv->directRenderingSupport = TRUE;
    pDRIPriv->pDriverInfo = pDRIInfo;
    pDRIPriv->nrWindows = 0;
    pDRIPriv->nrWindowsVisible = 0;
    pDRIPriv->fullscreen = NULL;

    pDRIPriv->createDummyCtx = pDRIInfo->createDummyCtx;
    pDRIPriv->createDummyCtxPriv = pDRIInfo->createDummyCtxPriv;

    pDRIPriv->grabbedDRILock = FALSE;
    pDRIPriv->drmSIGIOHandlerInstalled = FALSE;
    *pDRMFD = pDRIPriv->drmFD;

    if (pDRIEntPriv->sAreaGrabbed || pDRIInfo->allocSarea) {

        if (drmAddMap(pDRIPriv->drmFD,
                      0,
                      pDRIPriv->pDriverInfo->SAREASize,
                      DRM_SHM, 0, &pDRIPriv->hSAREA) < 0) {
            pDRIPriv->directRenderingSupport = FALSE;
            dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
            drmClose(pDRIPriv->drmFD);
            DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] drmAddMap failed\n");
            return FALSE;
        }
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] added %d byte SAREA at %p\n",
                  (int) pDRIPriv->pDriverInfo->SAREASize, (void *) (uintptr_t) pDRIPriv->hSAREA);

        /* Backwards compat. */
        if (drmMap(pDRIPriv->drmFD,
                   pDRIPriv->hSAREA,
                   pDRIPriv->pDriverInfo->SAREASize,
                   (drmAddressPtr) (&pDRIPriv->pSAREA)) < 0) {
            pDRIPriv->directRenderingSupport = FALSE;
            dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
            drmClose(pDRIPriv->drmFD);
            DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] drmMap failed\n");
            return FALSE;
        }
        DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] mapped SAREA %p to %p\n",
                  (void *) (uintptr_t) pDRIPriv->hSAREA, pDRIPriv->pSAREA);
        memset(pDRIPriv->pSAREA, 0, pDRIPriv->pDriverInfo->SAREASize);
    }
    else {
        DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] Using the DRM lock "
                  "SAREA also for drawables.\n");
        pDRIPriv->hSAREA = pDRIEntPriv->hLSAREA;
        pDRIPriv->pSAREA = (XF86DRISAREAPtr) pDRIEntPriv->pLSAREA;
        pDRIEntPriv->sAreaGrabbed = TRUE;
    }

    pDRIPriv->hLSAREA = pDRIEntPriv->hLSAREA;
    pDRIPriv->pLSAREA = pDRIEntPriv->pLSAREA;

    if (!pDRIPriv->pDriverInfo->dontMapFrameBuffer) {
        if (drmAddMap(pDRIPriv->drmFD,
                      (uintptr_t) pDRIPriv->pDriverInfo->
                      frameBufferPhysicalAddress,
                      pDRIPriv->pDriverInfo->frameBufferSize, DRM_FRAME_BUFFER,
                      0, &pDRIPriv->pDriverInfo->hFrameBuffer) < 0) {
            pDRIPriv->directRenderingSupport = FALSE;
            dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
            drmUnmap(pDRIPriv->pSAREA, pDRIPriv->pDriverInfo->SAREASize);
            drmClose(pDRIPriv->drmFD);
            DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] drmAddMap failed\n");
            return FALSE;
        }
        DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] framebuffer handle = %p\n",
                  (void *) (uintptr_t) pDRIPriv->pDriverInfo->hFrameBuffer);
    }
    else {
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] framebuffer mapped by ddx driver\n");
    }

    if (pDRIEntPriv->resOwner == NULL) {
        pDRIEntPriv->resOwner = pScreen;

        /* Add tags for reserved contexts */
        if ((reserved = drmGetReservedContextList(pDRIPriv->drmFD,
                                                  &reserved_count))) {
            int r;
            void *tag;

            for (r = 0; r < reserved_count; r++) {
                tag = DRICreateContextPrivFromHandle(pScreen,
                                                     reserved[r],
                                                     DRI_CONTEXT_RESERVED);
                drmAddContextTag(pDRIPriv->drmFD, reserved[r], tag);
            }
            drmFreeReservedContextList(reserved);
            DRIDrvMsg(pScreen->myNum, X_INFO,
                      "[drm] added %d reserved context%s for kernel\n",
                      reserved_count, reserved_count > 1 ? "s" : "");
        }
    }

    /* validate max drawable table entry set by driver */
    if ((pDRIPriv->pDriverInfo->maxDrawableTableEntry <= 0) ||
        (pDRIPriv->pDriverInfo->maxDrawableTableEntry > SAREA_MAX_DRAWABLES)) {
        DRIDrvMsg(pScreen->myNum, X_ERROR,
                  "Invalid max drawable table size set by driver: %d\n",
                  pDRIPriv->pDriverInfo->maxDrawableTableEntry);
    }

    /* Initialize drawable tables (screen private and SAREA) */
    for (i = 0; i < pDRIPriv->pDriverInfo->maxDrawableTableEntry; i++) {
        pDRIPriv->DRIDrawables[i] = NULL;
        pDRIPriv->pSAREA->drawableTable[i].stamp = 0;
        pDRIPriv->pSAREA->drawableTable[i].flags = 0;
    }

    pDRIPriv->pLockRefCount = &pDRIEntPriv->lockRefCount;
    pDRIPriv->pLockingContext = &pDRIEntPriv->lockingContext;

    if (!pDRIEntPriv->keepFDOpen)
        pDRIEntPriv->keepFDOpen = pDRIInfo->keepFDOpen;

    pDRIEntPriv->refCount++;

    /* Set up flags for DRICreateContextPriv */
    switch (pDRIInfo->driverSwapMethod) {
    case DRI_KERNEL_SWAP:
        flags = DRI_CONTEXT_2DONLY;
        break;
    case DRI_HIDE_X_CONTEXT:
        flags = DRI_CONTEXT_PRESERVED;
        break;
    }

    if (!(pDRIContextPriv = DRICreateContextPriv(pScreen,
                                                 &pDRIPriv->myContext,
                                                 flags))) {
        DRIDrvMsg(pScreen->myNum, X_ERROR, "failed to create server context\n");
        return FALSE;
    }
    pDRIPriv->myContextPriv = pDRIContextPriv;

    DRIDrvMsg(pScreen->myNum, X_INFO,
              "X context handle = %p\n", (void *) (uintptr_t) pDRIPriv->myContext);

    /* Now that we have created the X server's context, we can grab the
     * hardware lock for the X server.
     */
    DRILock(pScreen, 0);
    pDRIPriv->grabbedDRILock = TRUE;

    /* pointers so that we can prevent memory leaks later */
    pDRIPriv->hiddenContextStore = NULL;
    pDRIPriv->partial3DContextStore = NULL;

    switch (pDRIInfo->driverSwapMethod) {
    case DRI_HIDE_X_CONTEXT:
        /* Server will handle 3D swaps, and hide 2D swaps from kernel.
         * Register server context as a preserved context.
         */

        /* allocate memory for hidden context store */
        pDRIPriv->hiddenContextStore
            = (void *) calloc(1, pDRIInfo->contextSize);
        if (!pDRIPriv->hiddenContextStore) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "failed to allocate hidden context\n");
            DRIDestroyContextPriv(pDRIContextPriv);
            return FALSE;
        }

        /* allocate memory for partial 3D context store */
        pDRIPriv->partial3DContextStore
            = (void *) calloc(1, pDRIInfo->contextSize);
        if (!pDRIPriv->partial3DContextStore) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "[DRI] failed to allocate partial 3D context\n");
            free(pDRIPriv->hiddenContextStore);
            DRIDestroyContextPriv(pDRIContextPriv);
            return FALSE;
        }

        /* save initial context store */
        if (pDRIInfo->SwapContext) {
            (*pDRIInfo->SwapContext) (pScreen,
                                      DRI_NO_SYNC,
                                      DRI_2D_CONTEXT,
                                      pDRIPriv->hiddenContextStore,
                                      DRI_NO_CONTEXT, NULL);
        }
        /* fall through */

    case DRI_SERVER_SWAP:
        /* For swap methods of DRI_SERVER_SWAP and DRI_HIDE_X_CONTEXT
         * setup signal handler for receiving swap requests from kernel
         */
        if (!(pDRIPriv->drmSIGIOHandlerInstalled =
              drmInstallSIGIOHandler(pDRIPriv->drmFD, DRISwapContext))) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "[drm] failed to setup DRM signal handler\n");
            free(pDRIPriv->hiddenContextStore);
            free(pDRIPriv->partial3DContextStore);
            DRIDestroyContextPriv(pDRIContextPriv);
            return FALSE;
        }
        else {
            DRIDrvMsg(pScreen->myNum, X_INFO,
                      "[drm] installed DRM signal handler\n");
        }

    default:
        break;
    }

    return TRUE;
}

Bool
DRIFinishScreenInit(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    /* Wrap DRI support */
    if (pDRIInfo->wrap.WindowExposures) {
        pDRIPriv->wrap.WindowExposures = pScreen->WindowExposures;
        pScreen->WindowExposures = pDRIInfo->wrap.WindowExposures;
    }

    pDRIPriv->DestroyWindow = pScreen->DestroyWindow;
    pScreen->DestroyWindow = DRIDestroyWindow;

    pDRIPriv->xf86_crtc_notify = xf86_wrap_crtc_notify(pScreen,
                                                       dri_crtc_notify);

    if (pDRIInfo->wrap.CopyWindow) {
        pDRIPriv->wrap.CopyWindow = pScreen->CopyWindow;
        pScreen->CopyWindow = pDRIInfo->wrap.CopyWindow;
    }
    if (pDRIInfo->wrap.ClipNotify) {
        pDRIPriv->wrap.ClipNotify = pScreen->ClipNotify;
        pScreen->ClipNotify = pDRIInfo->wrap.ClipNotify;
    }
    if (pDRIInfo->wrap.AdjustFrame) {
        ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

        pDRIPriv->wrap.AdjustFrame = pScrn->AdjustFrame;
        pScrn->AdjustFrame = pDRIInfo->wrap.AdjustFrame;
    }
    pDRIPriv->wrapped = TRUE;

    DRIDrvMsg(pScreen->myNum, X_INFO, "[DRI] installation complete\n");

    return TRUE;
}

void
DRICloseScreen(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo;
    drm_context_t *reserved;
    int reserved_count;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DRIEntPrivPtr pDRIEntPriv = DRI_ENT_PRIV(pScrn);
    Bool closeMaster;

    if (pDRIPriv) {

        pDRIInfo = pDRIPriv->pDriverInfo;

        if (pDRIPriv->wrapped) {
            /* Unwrap DRI Functions */
            if (pDRIInfo->wrap.WindowExposures) {
                pScreen->WindowExposures = pDRIPriv->wrap.WindowExposures;
                pDRIPriv->wrap.WindowExposures = NULL;
            }
            if (pDRIPriv->DestroyWindow) {
                pScreen->DestroyWindow = pDRIPriv->DestroyWindow;
                pDRIPriv->DestroyWindow = NULL;
            }

            xf86_unwrap_crtc_notify(pScreen, pDRIPriv->xf86_crtc_notify);

            if (pDRIInfo->wrap.CopyWindow) {
                pScreen->CopyWindow = pDRIPriv->wrap.CopyWindow;
                pDRIPriv->wrap.CopyWindow = NULL;
            }
            if (pDRIInfo->wrap.ClipNotify) {
                pScreen->ClipNotify = pDRIPriv->wrap.ClipNotify;
                pDRIPriv->wrap.ClipNotify = NULL;
            }
            if (pDRIInfo->wrap.AdjustFrame) {
                ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);

                scrn->AdjustFrame = pDRIPriv->wrap.AdjustFrame;
                pDRIPriv->wrap.AdjustFrame = NULL;
            }

            pDRIPriv->wrapped = FALSE;
        }

        if (pDRIPriv->drmSIGIOHandlerInstalled) {
            if (!drmRemoveSIGIOHandler(pDRIPriv->drmFD)) {
                DRIDrvMsg(pScreen->myNum, X_ERROR,
                          "[drm] failed to remove DRM signal handler\n");
            }
        }

        if (pDRIPriv->dummyCtxPriv && pDRIPriv->createDummyCtx) {
            DRIDestroyDummyContext(pScreen, pDRIPriv->createDummyCtxPriv);
        }

        if (!DRIDestroyContextPriv(pDRIPriv->myContextPriv)) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "failed to destroy server context\n");
        }

        /* Remove tags for reserved contexts */
        if (pDRIEntPriv->resOwner == pScreen) {
            pDRIEntPriv->resOwner = NULL;

            if ((reserved = drmGetReservedContextList(pDRIPriv->drmFD,
                                                      &reserved_count))) {
                int i;

                for (i = 0; i < reserved_count; i++) {
                    DRIDestroyContextPriv(drmGetContextTag(pDRIPriv->drmFD,
                                                           reserved[i]));
                }
                drmFreeReservedContextList(reserved);
                DRIDrvMsg(pScreen->myNum, X_INFO,
                          "[drm] removed %d reserved context%s for kernel\n",
                          reserved_count, reserved_count > 1 ? "s" : "");
            }
        }

        /* Make sure signals get unblocked etc. */
        drmUnlock(pDRIPriv->drmFD, pDRIPriv->myContext);
        pDRIPriv->pLockRefCount = NULL;
        closeMaster = (--pDRIEntPriv->refCount == 0) &&
            !pDRIEntPriv->keepFDOpen;
        if (closeMaster || pDRIPriv->hSAREA != pDRIEntPriv->hLSAREA) {
            DRIDrvMsg(pScreen->myNum, X_INFO,
                      "[drm] unmapping %d bytes of SAREA %p at %p\n",
                      (int) pDRIInfo->SAREASize, (void *) (uintptr_t) pDRIPriv->hSAREA, pDRIPriv->pSAREA);
            if (drmUnmap(pDRIPriv->pSAREA, pDRIInfo->SAREASize)) {
                DRIDrvMsg(pScreen->myNum, X_ERROR,
                          "[drm] unable to unmap %d bytes"
                          " of SAREA %p at %p\n",
                          (int) pDRIInfo->SAREASize,
                          (void *) (uintptr_t) pDRIPriv->hSAREA, pDRIPriv->pSAREA);
            }
        }
        else {
            pDRIEntPriv->sAreaGrabbed = FALSE;
        }

        if (closeMaster || (pDRIEntPriv->drmFD != pDRIPriv->drmFD)) {
            drmClose(pDRIPriv->drmFD);
            if (pDRIEntPriv->drmFD == pDRIPriv->drmFD) {
                DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] Closed DRM master.\n");
                pDRIEntPriv->drmFD = -1;
            }
        }

        free(pDRIPriv);
        dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
    }
}

#define DRM_MSG_VERBOSITY 3

static int
dri_drm_debug_print(const char *format, va_list ap)
    _X_ATTRIBUTE_PRINTF(1,0);

static int
dri_drm_debug_print(const char *format, va_list ap)
{
    xf86VDrvMsgVerb(-1, X_NONE, DRM_MSG_VERBOSITY, format, ap);
    return 0;
}

static void
dri_drm_get_perms(gid_t * group, mode_t * mode)
{
    *group = xf86ConfigDRI.group;
    *mode = xf86ConfigDRI.mode;
}

drmServerInfo DRIDRMServerInfo = {
    dri_drm_debug_print,
    xf86LoadKernelModule,
    dri_drm_get_perms,
};

Bool
DRIExtensionInit(void)
{
    if (DRIGeneration != serverGeneration) {
        return FALSE;
    }

    DRIDrawablePrivResType = CreateNewResourceType(DRIDrawablePrivDelete,
                                                   "DRIDrawable");
    DRIContextPrivResType = CreateNewResourceType(DRIContextPrivDelete,
                                                  "DRIContext");

    if (!DRIDrawablePrivResType || !DRIContextPrivResType)
        return FALSE;

    RegisterBlockAndWakeupHandlers(DRIBlockHandler, DRIWakeupHandler, NULL);

    return TRUE;
}

void
DRIReset(void)
{
    /*
     * This stub routine is called when the X Server recycles, resources
     * allocated by DRIExtensionInit need to be managed here.
     *
     * Currently this routine is a stub because all the interesting resources
     * are managed via the screen init process.
     */
}

Bool
DRIQueryDirectRenderingCapable(ScreenPtr pScreen, Bool *isCapable)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv)
        *isCapable = pDRIPriv->directRenderingSupport;
    else
        *isCapable = FALSE;

    return TRUE;
}

Bool
DRIOpenConnection(ScreenPtr pScreen, drm_handle_t * hSAREA, char **busIdString)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *hSAREA = pDRIPriv->hSAREA;
    *busIdString = pDRIPriv->pDriverInfo->busIdString;

    return TRUE;
}

Bool
DRIAuthConnection(ScreenPtr pScreen, drm_magic_t magic)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (drmAuthMagic(pDRIPriv->drmFD, magic))
        return FALSE;
    return TRUE;
}

Bool
DRICloseConnection(ScreenPtr pScreen)
{
    return TRUE;
}

Bool
DRIGetClientDriverName(ScreenPtr pScreen,
                       int *ddxDriverMajorVersion,
                       int *ddxDriverMinorVersion,
                       int *ddxDriverPatchVersion, char **clientDriverName)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *ddxDriverMajorVersion = pDRIPriv->pDriverInfo->ddxDriverMajorVersion;
    *ddxDriverMinorVersion = pDRIPriv->pDriverInfo->ddxDriverMinorVersion;
    *ddxDriverPatchVersion = pDRIPriv->pDriverInfo->ddxDriverPatchVersion;
    *clientDriverName = pDRIPriv->pDriverInfo->clientDriverName;

    return TRUE;
}

/* DRICreateContextPriv and DRICreateContextPrivFromHandle are helper
   functions that layer on drmCreateContext and drmAddContextTag.

   DRICreateContextPriv always creates a kernel drm_context_t and then calls
   DRICreateContextPrivFromHandle to create a DRIContextPriv structure for
   DRI tracking.  For the SIGIO handler, the drm_context_t is associated with
   DRIContextPrivPtr.  Any special flags are stored in the DRIContextPriv
   area and are passed to the kernel (if necessary).

   DRICreateContextPriv returns a pointer to newly allocated
   DRIContextPriv, and returns the kernel drm_context_t in pHWContext. */

DRIContextPrivPtr
DRICreateContextPriv(ScreenPtr pScreen,
                     drm_context_t * pHWContext, DRIContextFlags flags)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (drmCreateContext(pDRIPriv->drmFD, pHWContext)) {
        return NULL;
    }

    return DRICreateContextPrivFromHandle(pScreen, *pHWContext, flags);
}

DRIContextPrivPtr
DRICreateContextPrivFromHandle(ScreenPtr pScreen,
                               drm_context_t hHWContext, DRIContextFlags flags)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv;
    int contextPrivSize;

    contextPrivSize = sizeof(DRIContextPrivRec) +
        pDRIPriv->pDriverInfo->contextSize;
    if (!(pDRIContextPriv = calloc(1, contextPrivSize))) {
        return NULL;
    }
    pDRIContextPriv->pContextStore = (void *) (pDRIContextPriv + 1);

    drmAddContextTag(pDRIPriv->drmFD, hHWContext, pDRIContextPriv);

    pDRIContextPriv->hwContext = hHWContext;
    pDRIContextPriv->pScreen = pScreen;
    pDRIContextPriv->flags = flags;
    pDRIContextPriv->valid3D = FALSE;

    if (flags & DRI_CONTEXT_2DONLY) {
        if (drmSetContextFlags(pDRIPriv->drmFD, hHWContext, DRM_CONTEXT_2DONLY)) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "[drm] failed to set 2D context flag\n");
            DRIDestroyContextPriv(pDRIContextPriv);
            return NULL;
        }
    }
    if (flags & DRI_CONTEXT_PRESERVED) {
        if (drmSetContextFlags(pDRIPriv->drmFD,
                               hHWContext, DRM_CONTEXT_PRESERVED)) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "[drm] failed to set preserved flag\n");
            DRIDestroyContextPriv(pDRIContextPriv);
            return NULL;
        }
    }
    return pDRIContextPriv;
}

Bool
DRIDestroyContextPriv(DRIContextPrivPtr pDRIContextPriv)
{
    DRIScreenPrivPtr pDRIPriv;

    if (!pDRIContextPriv)
        return TRUE;

    pDRIPriv = DRI_SCREEN_PRIV(pDRIContextPriv->pScreen);

    if (!(pDRIContextPriv->flags & DRI_CONTEXT_RESERVED)) {
        /* Don't delete reserved contexts from
           kernel area -- the kernel manages its
           reserved contexts itself. */
        if (drmDestroyContext(pDRIPriv->drmFD, pDRIContextPriv->hwContext))
            return FALSE;
    }

    /* Remove the tag last to prevent a race
       condition where the context has pending
       buffers.  The context can't be re-used
       while in this thread, but buffers can be
       dispatched asynchronously. */
    drmDelContextTag(pDRIPriv->drmFD, pDRIContextPriv->hwContext);
    free(pDRIContextPriv);
    return TRUE;
}

static Bool
DRICreateDummyContext(ScreenPtr pScreen, Bool needCtxPriv)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv;
    void *contextStore;

    if (!(pDRIContextPriv =
          DRICreateContextPriv(pScreen, &pDRIPriv->pSAREA->dummy_context, 0))) {
        return FALSE;
    }

    contextStore = DRIGetContextStore(pDRIContextPriv);
    if (pDRIPriv->pDriverInfo->CreateContext && needCtxPriv) {
        if (!pDRIPriv->pDriverInfo->CreateContext(pScreen, NULL,
                                                  pDRIPriv->pSAREA->
                                                  dummy_context, NULL,
                                                  (DRIContextType) (long)
                                                  contextStore)) {
            DRIDestroyContextPriv(pDRIContextPriv);
            return FALSE;
        }
    }

    pDRIPriv->dummyCtxPriv = pDRIContextPriv;
    return TRUE;
}

static void
DRIDestroyDummyContext(ScreenPtr pScreen, Bool hasCtxPriv)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv = pDRIPriv->dummyCtxPriv;
    void *contextStore;

    if (!pDRIContextPriv)
        return;
    if (pDRIPriv->pDriverInfo->DestroyContext && hasCtxPriv) {
        contextStore = DRIGetContextStore(pDRIContextPriv);
        pDRIPriv->pDriverInfo->DestroyContext(pDRIContextPriv->pScreen,
                                              pDRIContextPriv->hwContext,
                                              (DRIContextType) (long)
                                              contextStore);
    }

    DRIDestroyContextPriv(pDRIPriv->dummyCtxPriv);
    pDRIPriv->dummyCtxPriv = NULL;
}

Bool
DRICreateContext(ScreenPtr pScreen, VisualPtr visual,
                 XID context, drm_context_t * pHWContext)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv;
    void *contextStore;

    if (pDRIPriv->createDummyCtx && !pDRIPriv->dummyCtxPriv) {
        if (!DRICreateDummyContext(pScreen, pDRIPriv->createDummyCtxPriv)) {
            DRIDrvMsg(pScreen->myNum, X_INFO,
                      "[drm] Could not create dummy context\n");
            return FALSE;
        }
    }

    if (!(pDRIContextPriv = DRICreateContextPriv(pScreen, pHWContext, 0))) {
        return FALSE;
    }

    contextStore = DRIGetContextStore(pDRIContextPriv);
    if (pDRIPriv->pDriverInfo->CreateContext) {
        if (!((*pDRIPriv->pDriverInfo->CreateContext) (pScreen, NULL,
                                                       *pHWContext, NULL,
                                                       (DRIContextType) (long)
                                                       contextStore))) {
            DRIDestroyContextPriv(pDRIContextPriv);
            return FALSE;
        }
    }

    /* track this in case the client dies before cleanup */
    if (!AddResource(context, DRIContextPrivResType, (void *) pDRIContextPriv))
        return FALSE;

    return TRUE;
}

Bool
DRIDestroyContext(ScreenPtr pScreen, XID context)
{
    FreeResourceByType(context, DRIContextPrivResType, FALSE);

    return TRUE;
}

/* DRIContextPrivDelete is called by the resource manager. */
Bool
DRIContextPrivDelete(void *pResource, XID id)
{
    DRIContextPrivPtr pDRIContextPriv = (DRIContextPrivPtr) pResource;
    DRIScreenPrivPtr pDRIPriv;
    void *contextStore;

    pDRIPriv = DRI_SCREEN_PRIV(pDRIContextPriv->pScreen);
    if (pDRIPriv->pDriverInfo->DestroyContext) {
        contextStore = DRIGetContextStore(pDRIContextPriv);
        pDRIPriv->pDriverInfo->DestroyContext(pDRIContextPriv->pScreen,
                                              pDRIContextPriv->hwContext,
                                              (DRIContextType) (long)
                                              contextStore);
    }
    return DRIDestroyContextPriv(pDRIContextPriv);
}

/* This walks the drawable timestamp array and invalidates all of them
 * in the case of transition from private to shared backbuffers.  It's
 * not necessary for correctness, because DRIClipNotify gets called in
 * time to prevent any conflict, but the transition from
 * shared->private is sometimes missed if we don't do this.
 */
static void
DRIClipNotifyAllDrawables(ScreenPtr pScreen)
{
    int i;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    for (i = 0; i < pDRIPriv->pDriverInfo->maxDrawableTableEntry; i++) {
        pDRIPriv->pSAREA->drawableTable[i].stamp = DRIDrawableValidationStamp++;
    }
}

static void
DRITransitionToSharedBuffers(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables(pScreen);

    if (pDRIInfo->TransitionSingleToMulti3D)
        pDRIInfo->TransitionSingleToMulti3D(pScreen);
}

static void
DRITransitionToPrivateBuffers(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables(pScreen);

    if (pDRIInfo->TransitionMultiToSingle3D)
        pDRIInfo->TransitionMultiToSingle3D(pScreen);
}

static void
DRITransitionTo3d(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables(pScreen);

    if (pDRIInfo->TransitionTo3d)
        pDRIInfo->TransitionTo3d(pScreen);
}

static void
DRITransitionTo2d(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables(pScreen);

    if (pDRIInfo->TransitionTo2d)
        pDRIInfo->TransitionTo2d(pScreen);
}

static int
DRIDCNTreeTraversal(WindowPtr pWin, void *data)
{
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if (pDRIDrawablePriv) {
        ScreenPtr pScreen = pWin->drawable.pScreen;
        DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

        if (RegionNumRects(&pWin->clipList) > 0) {
            WindowPtr *pDRIWindows = (WindowPtr *) data;
            int i = 0;

            while (pDRIWindows[i])
                i++;

            pDRIWindows[i] = pWin;

            pDRIPriv->nrWalked++;
        }

        if (pDRIPriv->nrWindows == pDRIPriv->nrWalked)
            return WT_STOPWALKING;
    }

    return WT_WALKCHILDREN;
}

static void
DRIDriverClipNotify(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv->pDriverInfo->ClipNotify) {
        WindowPtr *pDRIWindows = calloc(sizeof(WindowPtr), pDRIPriv->nrWindows);
        DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

        if (pDRIPriv->nrWindows > 0) {
            pDRIPriv->nrWalked = 0;
            TraverseTree(pScreen->root, DRIDCNTreeTraversal,
                         (void *) pDRIWindows);
        }

        pDRIInfo->ClipNotify(pScreen, pDRIWindows, pDRIPriv->nrWindows);

        free(pDRIWindows);
    }
}

static void
DRIIncreaseNumberVisible(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    switch (++pDRIPriv->nrWindowsVisible) {
    case 1:
        DRITransitionTo3d(pScreen);
        break;
    case 2:
        DRITransitionToSharedBuffers(pScreen);
        break;
    default:
        break;
    }

    DRIDriverClipNotify(pScreen);
}

static void
DRIDecreaseNumberVisible(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    switch (--pDRIPriv->nrWindowsVisible) {
    case 0:
        DRITransitionTo2d(pScreen);
        break;
    case 1:
        DRITransitionToPrivateBuffers(pScreen);
        break;
    default:
        break;
    }

    DRIDriverClipNotify(pScreen);
}

Bool
DRICreateDrawable(ScreenPtr pScreen, ClientPtr client, DrawablePtr pDrawable,
                  drm_drawable_t * hHWDrawable)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv;
    WindowPtr pWin;

    if (pDrawable->type == DRAWABLE_WINDOW) {
        pWin = (WindowPtr) pDrawable;
        if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {
            pDRIDrawablePriv->refCount++;

            if (!pDRIDrawablePriv->hwDrawable) {
                drmCreateDrawable(pDRIPriv->drmFD,
                                  &pDRIDrawablePriv->hwDrawable);
            }
        }
        else {
            /* allocate a DRI Window Private record */
            if (!(pDRIDrawablePriv = malloc(sizeof(DRIDrawablePrivRec)))) {
                return FALSE;
            }

            /* Only create a drm_drawable_t once */
            if (drmCreateDrawable(pDRIPriv->drmFD,
                                  &pDRIDrawablePriv->hwDrawable)) {
                free(pDRIDrawablePriv);
                return FALSE;
            }

            /* add it to the list of DRI drawables for this screen */
            pDRIDrawablePriv->pScreen = pScreen;
            pDRIDrawablePriv->refCount = 1;
            pDRIDrawablePriv->drawableIndex = -1;
            pDRIDrawablePriv->nrects = RegionNumRects(&pWin->clipList);

            /* save private off of preallocated index */
            dixSetPrivate(&pWin->devPrivates, DRIWindowPrivKey,
                          pDRIDrawablePriv);
            pDRIPriv->nrWindows++;

            if (pDRIDrawablePriv->nrects)
                DRIIncreaseNumberVisible(pScreen);
        }

        /* track this in case the client dies */
        if (!AddResource(FakeClientID(client->index), DRIDrawablePrivResType,
                         (void *) (intptr_t) pDrawable->id))
            return FALSE;

        if (pDRIDrawablePriv->hwDrawable) {
            drmUpdateDrawableInfo(pDRIPriv->drmFD,
                                  pDRIDrawablePriv->hwDrawable,
                                  DRM_DRAWABLE_CLIPRECTS,
                                  RegionNumRects(&pWin->clipList),
                                  RegionRects(&pWin->clipList));
            *hHWDrawable = pDRIDrawablePriv->hwDrawable;
        }
    }
    else if (pDrawable->type != DRAWABLE_PIXMAP) {      /* PBuffer */
        /* NOT_DONE */
        return FALSE;
    }

    return TRUE;
}

static void
DRIDrawablePrivDestroy(WindowPtr pWin)
{
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
    ScreenPtr pScreen;
    DRIScreenPrivPtr pDRIPriv;

    if (!pDRIDrawablePriv)
        return;

    pScreen = pWin->drawable.pScreen;
    pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIDrawablePriv->drawableIndex != -1) {
        /* bump stamp to force outstanding 3D requests to resync */
        pDRIPriv->pSAREA->drawableTable[pDRIDrawablePriv->drawableIndex].stamp
            = DRIDrawableValidationStamp++;

        /* release drawable table entry */
        pDRIPriv->DRIDrawables[pDRIDrawablePriv->drawableIndex] = NULL;
    }

    pDRIPriv->nrWindows--;

    if (pDRIDrawablePriv->nrects)
        DRIDecreaseNumberVisible(pScreen);

    drmDestroyDrawable(pDRIPriv->drmFD, pDRIDrawablePriv->hwDrawable);

    free(pDRIDrawablePriv);
    dixSetPrivate(&pWin->devPrivates, DRIWindowPrivKey, NULL);
}

static Bool
DRIDestroyDrawableCB(void *value, XID id, void *data)
{
    if (value == data) {
        /* This calls back DRIDrawablePrivDelete which frees private area */
        FreeResourceByType(id, DRIDrawablePrivResType, FALSE);

        return TRUE;
    }

    return FALSE;
}

Bool
DRIDestroyDrawable(ScreenPtr pScreen, ClientPtr client, DrawablePtr pDrawable)
{
    if (pDrawable->type == DRAWABLE_WINDOW) {
        LookupClientResourceComplex(client, DRIDrawablePrivResType,
                                    DRIDestroyDrawableCB,
                                    (void *) (intptr_t) pDrawable->id);
    }
    else {                      /* pixmap (or for GLX 1.3, a PBuffer) */
        /* NOT_DONE */
        return FALSE;
    }

    return TRUE;
}

Bool
DRIDrawablePrivDelete(void *pResource, XID id)
{
    WindowPtr pWin;
    int rc;

    /* For DRIDrawablePrivResType, the XID is the client's fake ID. The
     * important XID is the value in pResource. */
    id = (XID) (intptr_t) pResource;
    rc = dixLookupWindow(&pWin, id, serverClient, DixGetAttrAccess);

    if (rc == Success) {
        DRIDrawablePrivPtr pDRIDrwPriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

        if (!pDRIDrwPriv)
            return FALSE;

        if (--pDRIDrwPriv->refCount == 0)
            DRIDrawablePrivDestroy(pWin);

        return TRUE;
    }
    else {                      /* pixmap (or for GLX 1.3, a PBuffer) */
        /* NOT_DONE */
        return FALSE;
    }
}

Bool
DRIGetDrawableInfo(ScreenPtr pScreen,
                   DrawablePtr pDrawable,
                   unsigned int *index,
                   unsigned int *stamp,
                   int *X,
                   int *Y,
                   int *W,
                   int *H,
                   int *numClipRects,
                   drm_clip_rect_t ** pClipRects,
                   int *backX,
                   int *backY,
                   int *numBackClipRects, drm_clip_rect_t ** pBackClipRects)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv, pOldDrawPriv;
    WindowPtr pWin, pOldWin;
    int i;

#if 0
    printf("maxDrawableTableEntry = %d\n",
           pDRIPriv->pDriverInfo->maxDrawableTableEntry);
#endif

    if (pDrawable->type == DRAWABLE_WINDOW) {
        pWin = (WindowPtr) pDrawable;
        if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {

            /* Manage drawable table */
            if (pDRIDrawablePriv->drawableIndex == -1) {        /* load SAREA table */

                /* Search table for empty entry */
                i = 0;
                while (i < pDRIPriv->pDriverInfo->maxDrawableTableEntry) {
                    if (!(pDRIPriv->DRIDrawables[i])) {
                        pDRIPriv->DRIDrawables[i] = pDrawable;
                        pDRIDrawablePriv->drawableIndex = i;
                        pDRIPriv->pSAREA->drawableTable[i].stamp =
                            DRIDrawableValidationStamp++;
                        break;
                    }
                    i++;
                }

                /* Search table for oldest entry */
                if (i == pDRIPriv->pDriverInfo->maxDrawableTableEntry) {
                    unsigned int oldestStamp = ~0;
                    int oldestIndex = 0;

                    i = pDRIPriv->pDriverInfo->maxDrawableTableEntry;
                    while (i--) {
                        if (pDRIPriv->pSAREA->drawableTable[i].stamp <
                            oldestStamp) {
                            oldestIndex = i;
                            oldestStamp =
                                pDRIPriv->pSAREA->drawableTable[i].stamp;
                        }
                    }
                    pDRIDrawablePriv->drawableIndex = oldestIndex;

                    /* release oldest drawable table entry */
                    pOldWin = (WindowPtr) pDRIPriv->DRIDrawables[oldestIndex];
                    pOldDrawPriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pOldWin);
                    pOldDrawPriv->drawableIndex = -1;

                    /* claim drawable table entry */
                    pDRIPriv->DRIDrawables[oldestIndex] = pDrawable;

                    /* validate SAREA entry */
                    pDRIPriv->pSAREA->drawableTable[oldestIndex].stamp =
                        DRIDrawableValidationStamp++;

                    /* check for stamp wrap around */
                    if (oldestStamp > DRIDrawableValidationStamp) {

                        /* walk SAREA table and invalidate all drawables */
                        for (i = 0;
                             i < pDRIPriv->pDriverInfo->maxDrawableTableEntry;
                             i++) {
                            pDRIPriv->pSAREA->drawableTable[i].stamp =
                                DRIDrawableValidationStamp++;
                        }
                    }
                }

                /* If the driver wants to be notified when the index is
                 * set for a drawable, let it know now.
                 */
                if (pDRIPriv->pDriverInfo->SetDrawableIndex)
                    pDRIPriv->pDriverInfo->SetDrawableIndex(pWin,
                                                            pDRIDrawablePriv->
                                                            drawableIndex);

                /* reinit drawable ID if window is visible */
                if ((pWin->viewable) &&
                    (pDRIPriv->pDriverInfo->bufferRequests != DRI_NO_WINDOWS)) {
                    (*pDRIPriv->pDriverInfo->InitBuffers) (pWin,
                                                           &pWin->clipList,
                                                           pDRIDrawablePriv->
                                                           drawableIndex);
                }
            }

            *index = pDRIDrawablePriv->drawableIndex;
            *stamp = pDRIPriv->pSAREA->drawableTable[*index].stamp;
            *X = (int) (pWin->drawable.x);
            *Y = (int) (pWin->drawable.y);
            *W = (int) (pWin->drawable.width);
            *H = (int) (pWin->drawable.height);
            *numClipRects = RegionNumRects(&pWin->clipList);
            *pClipRects = (drm_clip_rect_t *) RegionRects(&pWin->clipList);

            if (!*numClipRects && pDRIPriv->fullscreen) {
                /* use fake full-screen clip rect */
                pDRIPriv->fullscreen_rect.x1 = *X;
                pDRIPriv->fullscreen_rect.y1 = *Y;
                pDRIPriv->fullscreen_rect.x2 = *X + *W;
                pDRIPriv->fullscreen_rect.y2 = *Y + *H;

                *numClipRects = 1;
                *pClipRects = &pDRIPriv->fullscreen_rect;
            }

            *backX = *X;
            *backY = *Y;

            if (pDRIPriv->nrWindowsVisible == 1 && *numClipRects) {
                /* Use a single cliprect. */

                int x0 = *X;
                int y0 = *Y;
                int x1 = x0 + *W;
                int y1 = y0 + *H;

                if (x0 < 0)
                    x0 = 0;
                if (y0 < 0)
                    y0 = 0;
                if (x1 > pScreen->width)
                    x1 = pScreen->width;
                if (y1 > pScreen->height)
                    y1 = pScreen->height;

                if (y0 >= y1 || x0 >= x1) {
                    *numBackClipRects = 0;
                    *pBackClipRects = NULL;
                }
                else {
                    pDRIPriv->private_buffer_rect.x1 = x0;
                    pDRIPriv->private_buffer_rect.y1 = y0;
                    pDRIPriv->private_buffer_rect.x2 = x1;
                    pDRIPriv->private_buffer_rect.y2 = y1;

                    *numBackClipRects = 1;
                    *pBackClipRects = &(pDRIPriv->private_buffer_rect);
                }
            }
            else {
                /* Use the frontbuffer cliprects for back buffers.  */
                *numBackClipRects = 0;
                *pBackClipRects = 0;
            }
        }
        else {
            /* Not a DRIDrawable */
            return FALSE;
        }
    }
    else {                      /* pixmap (or for GLX 1.3, a PBuffer) */
        /* NOT_DONE */
        return FALSE;
    }

    return TRUE;
}

Bool
DRIGetDeviceInfo(ScreenPtr pScreen,
                 drm_handle_t * hFrameBuffer,
                 int *fbOrigin,
                 int *fbSize,
                 int *fbStride, int *devPrivateSize, void **pDevPrivate)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *hFrameBuffer = pDRIPriv->pDriverInfo->hFrameBuffer;
    *fbOrigin = 0;
    *fbSize = pDRIPriv->pDriverInfo->frameBufferSize;
    *fbStride = pDRIPriv->pDriverInfo->frameBufferStride;
    *devPrivateSize = pDRIPriv->pDriverInfo->devPrivateSize;
    *pDevPrivate = pDRIPriv->pDriverInfo->devPrivate;

    return TRUE;
}

DRIInfoPtr
DRICreateInfoRec(void)
{
    DRIInfoPtr inforec = (DRIInfoPtr) calloc(1, sizeof(DRIInfoRec));

    if (!inforec)
        return NULL;

    /* Initialize defaults */
    inforec->busIdString = NULL;

    /* Wrapped function defaults */
    inforec->wrap.WakeupHandler = DRIDoWakeupHandler;
    inforec->wrap.BlockHandler = DRIDoBlockHandler;
    inforec->wrap.WindowExposures = DRIWindowExposures;
    inforec->wrap.CopyWindow = DRICopyWindow;
    inforec->wrap.ClipNotify = DRIClipNotify;
    inforec->wrap.AdjustFrame = DRIAdjustFrame;

    inforec->TransitionTo2d = 0;
    inforec->TransitionTo3d = 0;
    inforec->SetDrawableIndex = 0;

    return inforec;
}

void
DRIDestroyInfoRec(DRIInfoPtr DRIInfo)
{
    free(DRIInfo->busIdString);
    free((char *) DRIInfo);
}

void
DRIWakeupHandler(void *wakeupData, int result)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

        if (pDRIPriv && pDRIPriv->pDriverInfo->wrap.WakeupHandler)
            (*pDRIPriv->pDriverInfo->wrap.WakeupHandler) (pScreen, result);
    }
}

void
DRIBlockHandler(void *blockData, void *pTimeout)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

        if (pDRIPriv && pDRIPriv->pDriverInfo->wrap.BlockHandler)
            (*pDRIPriv->pDriverInfo->wrap.BlockHandler) (pScreen, pTimeout);
    }
}

void
DRIDoWakeupHandler(ScreenPtr pScreen, int result)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    DRILock(pScreen, 0);
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
        /* hide X context by swapping 2D component here */
        (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                               DRI_3D_SYNC,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->partial3DContextStore,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->hiddenContextStore);
    }
}

void
DRIDoBlockHandler(ScreenPtr pScreen, void *timeout)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
        /* hide X context by swapping 2D component here */
        (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                               DRI_2D_SYNC,
                                               DRI_NO_CONTEXT,
                                               NULL,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->partial3DContextStore);
    }

    if (pDRIPriv->windowsTouched)
        DRM_SPINUNLOCK(&pDRIPriv->pSAREA->drawable_lock, 1);
    pDRIPriv->windowsTouched = FALSE;

    DRIUnlock(pScreen);
}

void
DRISwapContext(int drmFD, void *oldctx, void *newctx)
{
    DRIContextPrivPtr oldContext = (DRIContextPrivPtr) oldctx;
    DRIContextPrivPtr newContext = (DRIContextPrivPtr) newctx;
    ScreenPtr pScreen = newContext->pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    void *oldContextStore = NULL;
    DRIContextType oldContextType;
    void *newContextStore = NULL;
    DRIContextType newContextType;
    DRISyncType syncType;

#ifdef DEBUG
    static int count = 0;

    if (!newContext) {
        DRIDrvMsg(pScreen->myNum, X_ERROR,
                  "[DRI] Context Switch Error: oldContext=%p, newContext=%p\n",
                  oldContext, newContext);
        return;
    }

    /* useful for debugging, just print out after n context switches */
    if (!count || !(count % 1)) {
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[DRI] Context switch %5d from %p/0x%08x (%d)\n",
                  count,
                  oldContext,
                  oldContext ? oldContext->flags : 0,
                  oldContext ? oldContext->hwContext : -1);
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[DRI] Context switch %5d to   %p/0x%08x (%d)\n",
                  count,
                  newContext,
                  newContext ? newContext->flags : 0,
                  newContext ? newContext->hwContext : -1);
    }
    ++count;
#endif

    if (!pDRIPriv->pDriverInfo->SwapContext) {
        DRIDrvMsg(pScreen->myNum, X_ERROR,
                  "[DRI] DDX driver missing context swap call back\n");
        return;
    }

    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {

        /* only 3D contexts are swapped in this case */
        if (oldContext) {
            oldContextStore = DRIGetContextStore(oldContext);
            oldContext->valid3D = TRUE;
            oldContextType = DRI_3D_CONTEXT;
        }
        else {
            oldContextType = DRI_NO_CONTEXT;
        }
        newContextStore = DRIGetContextStore(newContext);
        if ((newContext->valid3D) &&
            (newContext->hwContext != pDRIPriv->myContext)) {
            newContextType = DRI_3D_CONTEXT;
        }
        else {
            newContextType = DRI_2D_CONTEXT;
        }
        syncType = DRI_3D_SYNC;
    }
    else {                      /* default: driverSwapMethod == DRI_SERVER_SWAP */

        /* optimize 2D context swaps */

        if (newContext->flags & DRI_CONTEXT_2DONLY) {
            /* go from 3D context to 2D context and only save 2D
             * subset of 3D state
             */
            oldContextStore = DRIGetContextStore(oldContext);
            oldContextType = DRI_2D_CONTEXT;
            newContextStore = DRIGetContextStore(newContext);
            newContextType = DRI_2D_CONTEXT;
            syncType = DRI_3D_SYNC;
            pDRIPriv->lastPartial3DContext = oldContext;
        }
        else if (oldContext->flags & DRI_CONTEXT_2DONLY) {
            if (pDRIPriv->lastPartial3DContext == newContext) {
                /* go from 2D context back to previous 3D context and
                 * only restore 2D subset of previous 3D state
                 */
                oldContextStore = DRIGetContextStore(oldContext);
                oldContextType = DRI_2D_CONTEXT;
                newContextStore = DRIGetContextStore(newContext);
                newContextType = DRI_2D_CONTEXT;
                syncType = DRI_2D_SYNC;
            }
            else {
                /* go from 2D context to a different 3D context */

                /* call DDX driver to do partial restore */
                oldContextStore = DRIGetContextStore(oldContext);
                newContextStore =
                    DRIGetContextStore(pDRIPriv->lastPartial3DContext);
                (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                                       DRI_2D_SYNC,
                                                       DRI_2D_CONTEXT,
                                                       oldContextStore,
                                                       DRI_2D_CONTEXT,
                                                       newContextStore);

                /* now setup for a complete 3D swap */
                oldContextStore = newContextStore;
                oldContext->valid3D = TRUE;
                oldContextType = DRI_3D_CONTEXT;
                newContextStore = DRIGetContextStore(newContext);
                if ((newContext->valid3D) &&
                    (newContext->hwContext != pDRIPriv->myContext)) {
                    newContextType = DRI_3D_CONTEXT;
                }
                else {
                    newContextType = DRI_2D_CONTEXT;
                }
                syncType = DRI_NO_SYNC;
            }
        }
        else {
            /* now setup for a complete 3D swap */
            oldContextStore = newContextStore;
            oldContext->valid3D = TRUE;
            oldContextType = DRI_3D_CONTEXT;
            newContextStore = DRIGetContextStore(newContext);
            if ((newContext->valid3D) &&
                (newContext->hwContext != pDRIPriv->myContext)) {
                newContextType = DRI_3D_CONTEXT;
            }
            else {
                newContextType = DRI_2D_CONTEXT;
            }
            syncType = DRI_3D_SYNC;
        }
    }

    /* call DDX driver to perform the swap */
    (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                           syncType,
                                           oldContextType,
                                           oldContextStore,
                                           newContextType, newContextStore);
}

void *
DRIGetContextStore(DRIContextPrivPtr context)
{
    return ((void *) context->pContextStore);
}

void
DRIWindowExposures(WindowPtr pWin, RegionPtr prgn)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if (pDRIDrawablePriv) {
        (*pDRIPriv->pDriverInfo->InitBuffers) (pWin, prgn,
                                               pDRIDrawablePriv->drawableIndex);
    }

    /* call lower wrapped functions */
    if (pDRIPriv && pDRIPriv->wrap.WindowExposures) {

        /* unwrap */
        pScreen->WindowExposures = pDRIPriv->wrap.WindowExposures;

        /* call lower layers */
        (*pScreen->WindowExposures) (pWin, prgn);

        /* rewrap */
        pDRIPriv->wrap.WindowExposures = pScreen->WindowExposures;
        pScreen->WindowExposures = DRIWindowExposures;
    }
}

static int
DRITreeTraversal(WindowPtr pWin, void *data)
{
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if (pDRIDrawablePriv) {
        ScreenPtr pScreen = pWin->drawable.pScreen;
        DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

        if (RegionNumRects(&(pWin->clipList)) > 0) {
            RegionPtr reg = (RegionPtr) data;

            RegionUnion(reg, reg, &(pWin->clipList));
            pDRIPriv->nrWalked++;
        }

        if (pDRIPriv->nrWindows == pDRIPriv->nrWalked)
            return WT_STOPWALKING;
    }
    return WT_WALKCHILDREN;
}

Bool
DRIDestroyWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    Bool retval = TRUE;

    DRIDrawablePrivDestroy(pWin);

    /* call lower wrapped functions */
    if (pDRIPriv->DestroyWindow) {
        /* unwrap */
        pScreen->DestroyWindow = pDRIPriv->DestroyWindow;

        /* call lower layers */
        retval = (*pScreen->DestroyWindow) (pWin);

        /* rewrap */
        pDRIPriv->DestroyWindow = pScreen->DestroyWindow;
        pScreen->DestroyWindow = DRIDestroyWindow;
    }

    return retval;
}

void
DRICopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv)
        return;

    if (pDRIPriv->nrWindowsVisible > 0) {
        RegionRec reg;

        RegionNull(&reg);
        pDRIPriv->nrWalked = 0;
        TraverseTree(pWin, DRITreeTraversal, (void *) (&reg));

        if (RegionNotEmpty(&reg)) {
            RegionTranslate(&reg, ptOldOrg.x - pWin->drawable.x,
                            ptOldOrg.y - pWin->drawable.y);
            RegionIntersect(&reg, &reg, prgnSrc);

            /* The MoveBuffers interface is not ideal */
            (*pDRIPriv->pDriverInfo->MoveBuffers) (pWin, ptOldOrg, &reg,
                                                   pDRIPriv->pDriverInfo->
                                                   ddxDrawableTableEntry);
        }

        RegionUninit(&reg);
    }

    /* call lower wrapped functions */
    if (pDRIPriv->wrap.CopyWindow) {
        /* unwrap */
        pScreen->CopyWindow = pDRIPriv->wrap.CopyWindow;

        /* call lower layers */
        (*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);

        /* rewrap */
        pDRIPriv->wrap.CopyWindow = pScreen->CopyWindow;
        pScreen->CopyWindow = DRICopyWindow;
    }
}

static void
DRIGetSecs(long *secs, long *usecs)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    *secs = tv.tv_sec;
    *usecs = tv.tv_usec;
}

static unsigned long
DRIComputeMilliSeconds(unsigned long s_secs, unsigned long s_usecs,
                       unsigned long f_secs, unsigned long f_usecs)
{
    if (f_usecs < s_usecs) {
        --f_secs;
        f_usecs += 1000000;
    }
    return (f_secs - s_secs) * 1000 + (f_usecs - s_usecs) / 1000;
}

static void
DRISpinLockTimeout(drmLock * lock, int val, unsigned long timeout /* in mS */ )
{
    int count = 10000;

#if !defined(__alpha__) && !defined(__powerpc__)
    char ret;
#else
    int ret;
#endif
    long s_secs, s_usecs;
    long f_secs, f_usecs;
    long msecs;
    long prev = 0;

    DRIGetSecs(&s_secs, &s_usecs);

    do {
        DRM_SPINLOCK_COUNT(lock, val, count, ret);
        if (!ret)
            return;             /* Got lock */
        DRIGetSecs(&f_secs, &f_usecs);
        msecs = DRIComputeMilliSeconds(s_secs, s_usecs, f_secs, f_usecs);
        if (msecs - prev < 250)
            count *= 2;         /* Not more than 0.5S */
    } while (msecs < timeout);

    /* Didn't get lock, so take it.  The worst
       that can happen is that there is some
       garbage written to the wrong part of the
       framebuffer that a refresh will repair.
       That's undesirable, but better than
       locking the server.  This should be a
       very rare event. */
    DRM_SPINLOCK_TAKE(lock, val);
}

static void
DRILockTree(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv)
        return;

    /* Restore the last known 3D context if the X context is hidden */
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
        (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                               DRI_2D_SYNC,
                                               DRI_NO_CONTEXT,
                                               NULL,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->partial3DContextStore);
    }

    /* Call kernel to release lock */
    DRIUnlock(pScreen);

    /* Grab drawable spin lock: a time out between 10 and 30 seconds is
       appropriate, since this should never time out except in the case of
       client death while the lock is being held.  The timeout must be
       greater than any reasonable rendering time. */
    DRISpinLockTimeout(&pDRIPriv->pSAREA->drawable_lock, 1, 10000);     /*10 secs */

    /* Call kernel flush outstanding buffers and relock */
    DRILock(pScreen, DRM_LOCK_QUIESCENT | DRM_LOCK_FLUSH_ALL);

    /* Switch back to our 2D context if the X context is hidden */
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
        /* hide X context by swapping 2D component here */
        (*pDRIPriv->pDriverInfo->SwapContext) (pScreen,
                                               DRI_3D_SYNC,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->partial3DContextStore,
                                               DRI_2D_CONTEXT,
                                               pDRIPriv->hiddenContextStore);
    }
}

void
DRIClipNotify(WindowPtr pWin, int dx, int dy)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv;

    if (!pDRIPriv)
        return;

    if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {
        int nrects = RegionNumRects(&pWin->clipList);

        if (!pDRIPriv->windowsTouched) {
            DRILockTree(pScreen);
            pDRIPriv->windowsTouched = TRUE;
        }

        if (nrects && !pDRIDrawablePriv->nrects)
            DRIIncreaseNumberVisible(pScreen);
        else if (!nrects && pDRIDrawablePriv->nrects)
            DRIDecreaseNumberVisible(pScreen);
        else
            DRIDriverClipNotify(pScreen);

        pDRIDrawablePriv->nrects = nrects;

        pDRIPriv->pSAREA->drawableTable[pDRIDrawablePriv->drawableIndex].stamp
            = DRIDrawableValidationStamp++;

        drmUpdateDrawableInfo(pDRIPriv->drmFD, pDRIDrawablePriv->hwDrawable,
                              DRM_DRAWABLE_CLIPRECTS,
                              nrects, RegionRects(&pWin->clipList));
    }

    /* call lower wrapped functions */
    if (pDRIPriv->wrap.ClipNotify) {

        /* unwrap */
        pScreen->ClipNotify = pDRIPriv->wrap.ClipNotify;

        /* call lower layers */
        (*pScreen->ClipNotify) (pWin, dx, dy);

        /* rewrap */
        pDRIPriv->wrap.ClipNotify = pScreen->ClipNotify;
        pScreen->ClipNotify = DRIClipNotify;
    }
}

CARD32
DRIGetDrawableIndex(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
    CARD32 index;

    if (pDRIDrawablePriv) {
        index = pDRIDrawablePriv->drawableIndex;
    }
    else {
        index = pDRIPriv->pDriverInfo->ddxDrawableTableEntry;
    }

    return index;
}

unsigned int
DRIGetDrawableStamp(ScreenPtr pScreen, CARD32 drawable_index)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    return pDRIPriv->pSAREA->drawableTable[drawable_index].stamp;
}

void
DRIPrintDrawableLock(ScreenPtr pScreen, char *msg)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    ErrorF("%s: %d\n", msg, pDRIPriv->pSAREA->drawable_lock.lock);
}

void
DRILock(ScreenPtr pScreen, int flags)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv || !pDRIPriv->pLockRefCount)
        return;

    if (!*pDRIPriv->pLockRefCount) {
        DRM_LOCK(pDRIPriv->drmFD, pDRIPriv->pLSAREA, pDRIPriv->myContext,
                 flags);
        *pDRIPriv->pLockingContext = pDRIPriv->myContext;
    }
    else if (*pDRIPriv->pLockingContext != pDRIPriv->myContext) {
        DRIDrvMsg(pScreen->myNum, X_ERROR,
                  "[DRI] Locking deadlock.\n"
                  "\tAlready locked with context %p,\n"
                  "\ttrying to lock with context %p.\n",
                  pDRIPriv->pLockingContext, (void *) (uintptr_t) pDRIPriv->myContext);
    }
    (*pDRIPriv->pLockRefCount)++;
}

void
DRIUnlock(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv || !pDRIPriv->pLockRefCount)
        return;

    if (*pDRIPriv->pLockRefCount > 0) {
        if (pDRIPriv->myContext != *pDRIPriv->pLockingContext) {
            DRIDrvMsg(pScreen->myNum, X_ERROR,
                      "[DRI] Unlocking inconsistency:\n"
                      "\tContext %p trying to unlock lock held by context %p\n",
                      pDRIPriv->pLockingContext, (void *) (uintptr_t) pDRIPriv->myContext);
        }
        (*pDRIPriv->pLockRefCount)--;
    }
    else {
        DRIDrvMsg(pScreen->myNum, X_ERROR,
                  "DRIUnlock called when not locked.\n");
        return;
    }
    if (!*pDRIPriv->pLockRefCount)
        DRM_UNLOCK(pDRIPriv->drmFD, pDRIPriv->pLSAREA, pDRIPriv->myContext);
}

void *
DRIGetSAREAPrivate(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv)
        return 0;

    return (void *) (((char *) pDRIPriv->pSAREA) + sizeof(XF86DRISAREARec));
}

drm_context_t
DRIGetContext(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv)
        return 0;

    return pDRIPriv->myContext;
}

void
DRIGetTexOffsetFuncs(ScreenPtr pScreen,
                     DRITexOffsetStartProcPtr * texOffsetStartFunc,
                     DRITexOffsetFinishProcPtr * texOffsetFinishFunc)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (!pDRIPriv)
        return;

    *texOffsetStartFunc = pDRIPriv->pDriverInfo->texOffsetStart;
    *texOffsetFinishFunc = pDRIPriv->pDriverInfo->texOffsetFinish;
}

/* This lets get at the unwrapped functions so that they can correctly
 * call the lowerlevel functions, and choose whether they will be
 * called at every level of recursion (eg in validatetree).
 */
DRIWrappedFuncsRec *
DRIGetWrappedFuncs(ScreenPtr pScreen)
{
    return &(DRI_SCREEN_PRIV(pScreen)->wrap);
}

/* note that this returns the library version, not the protocol version */
void
DRIQueryVersion(int *majorVersion, int *minorVersion, int *patchVersion)
{
    *majorVersion = DRIINFO_MAJOR_VERSION;
    *minorVersion = DRIINFO_MINOR_VERSION;
    *patchVersion = DRIINFO_PATCH_VERSION;
}

static void
_DRIAdjustFrame(ScrnInfoPtr pScrn, DRIScreenPrivPtr pDRIPriv, int x, int y)
{
    pDRIPriv->pSAREA->frame.x = x;
    pDRIPriv->pSAREA->frame.y = y;
    pDRIPriv->pSAREA->frame.width = pScrn->frameX1 - x + 1;
    pDRIPriv->pSAREA->frame.height = pScrn->frameY1 - y + 1;
}

void
DRIAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    int px, py;

    if (!pDRIPriv || !pDRIPriv->pSAREA) {
        DRIDrvMsg(pScrn->scrnIndex, X_ERROR, "[DRI] No SAREA (%p %p)\n",
                  pDRIPriv, pDRIPriv ? pDRIPriv->pSAREA : NULL);
        return;
    }

    if (pDRIPriv->fullscreen) {
        /* Fix up frame */
        pScrn->frameX0 = pDRIPriv->pSAREA->frame.x;
        pScrn->frameY0 = pDRIPriv->pSAREA->frame.y;
        pScrn->frameX1 = pScrn->frameX0 + pDRIPriv->pSAREA->frame.width - 1;
        pScrn->frameY1 = pScrn->frameY0 + pDRIPriv->pSAREA->frame.height - 1;

        /* Fix up cursor */
        miPointerGetPosition(inputInfo.pointer, &px, &py);

        if (px < pScrn->frameX0)
            px = pScrn->frameX0;
        if (px > pScrn->frameX1)
            px = pScrn->frameX1;
        if (py < pScrn->frameY0)
            py = pScrn->frameY0;
        if (py > pScrn->frameY1)
            py = pScrn->frameY1;
        pScreen->SetCursorPosition(inputInfo.pointer, pScreen, px, py, TRUE);

        return;
    }

    if (pDRIPriv->wrap.AdjustFrame) {
        /* unwrap */
        pScrn->AdjustFrame = pDRIPriv->wrap.AdjustFrame;
        /* call lower layers */
        (*pScrn->AdjustFrame) (pScrn, x, y);
        /* rewrap */
        pDRIPriv->wrap.AdjustFrame = pScrn->AdjustFrame;
        pScrn->AdjustFrame = DRIAdjustFrame;
    }

    _DRIAdjustFrame(pScrn, pDRIPriv, x, y);
}

/*
 * DRIMoveBuffersHelper swaps the regions rects in place leaving you
 * a region with the rects in the order that you need to blit them,
 * but it is possibly (likely) an invalid region afterwards.  If you
 * need to use the region again for anything you have to call
 * REGION_VALIDATE on it, or better yet, save a copy first.
 */

void
DRIMoveBuffersHelper(ScreenPtr pScreen,
                     int dx, int dy, int *xdir, int *ydir, RegionPtr reg)
{
    BoxPtr extents, pbox, firstBox, lastBox;
    BoxRec tmpBox;
    int y, nbox;

    extents = RegionExtents(reg);
    nbox = RegionNumRects(reg);
    pbox = RegionRects(reg);

    if ((dy > 0) && (dy < (extents->y2 - extents->y1))) {
        *ydir = -1;
        if (nbox > 1) {
            firstBox = pbox;
            lastBox = pbox + nbox - 1;
            while ((unsigned long) firstBox < (unsigned long) lastBox) {
                tmpBox = *firstBox;
                *firstBox = *lastBox;
                *lastBox = tmpBox;
                firstBox++;
                lastBox--;
            }
        }
    }
    else
        *ydir = 1;

    if ((dx > 0) && (dx < (extents->x2 - extents->x1))) {
        *xdir = -1;
        if (nbox > 1) {
            firstBox = lastBox = pbox;
            y = pbox->y1;
            while (--nbox) {
                pbox++;
                if (pbox->y1 == y)
                    lastBox++;
                else {
                    while ((unsigned long) firstBox < (unsigned long) lastBox) {
                        tmpBox = *firstBox;
                        *firstBox = *lastBox;
                        *lastBox = tmpBox;
                        firstBox++;
                        lastBox--;
                    }

                    firstBox = lastBox = pbox;
                    y = pbox->y1;
                }
            }
            while ((unsigned long) firstBox < (unsigned long) lastBox) {
                tmpBox = *firstBox;
                *firstBox = *lastBox;
                *lastBox = tmpBox;
                firstBox++;
                lastBox--;
            }
        }
    }
    else
        *xdir = 1;

}
