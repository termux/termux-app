/*
 * midispcur.c
 *
 * machine independent cursor display routines
 */

/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include   <X11/X.h>
#include   "misc.h"
#include   "input.h"
#include   "cursorstr.h"
#include   "windowstr.h"
#include   "regionstr.h"
#include   "dixstruct.h"
#include   "scrnintstr.h"
#include   "servermd.h"
#include   "mipointer.h"
#include   "misprite.h"
#include   "gcstruct.h"

#include   "picturestr.h"

#include "inputstr.h"

/* per-screen private data */
static DevPrivateKeyRec miDCScreenKeyRec;

#define miDCScreenKey (&miDCScreenKeyRec)

static DevScreenPrivateKeyRec miDCDeviceKeyRec;

#define miDCDeviceKey (&miDCDeviceKeyRec)

static Bool miDCCloseScreen(ScreenPtr pScreen);

/* per device private data */
typedef struct {
    GCPtr pSourceGC, pMaskGC;
    GCPtr pSaveGC, pRestoreGC;
    PixmapPtr pSave;
    PicturePtr pRootPicture;
} miDCBufferRec, *miDCBufferPtr;

#define miGetDCDevice(dev, screen) \
 ((DevHasCursor(dev)) ? \
  (miDCBufferPtr)dixLookupScreenPrivate(&dev->devPrivates, miDCDeviceKey, screen) : \
  (miDCBufferPtr)dixLookupScreenPrivate(&GetMaster(dev, MASTER_POINTER)->devPrivates, miDCDeviceKey, screen))

/*
 * The core pointer buffer will point to the index of the virtual pointer
 * in the pCursorBuffers array.
 */
typedef struct {
    CloseScreenProcPtr CloseScreen;
    PixmapPtr sourceBits;       /* source bits */
    PixmapPtr maskBits;         /* mask bits */
    PicturePtr pPicture;
    CursorPtr pCursor;
} miDCScreenRec, *miDCScreenPtr;

#define miGetDCScreen(s)	((miDCScreenPtr)(dixLookupPrivate(&(s)->devPrivates, miDCScreenKey)))

Bool
miDCInitialize(ScreenPtr pScreen, miPointerScreenFuncPtr screenFuncs)
{
    miDCScreenPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&miDCScreenKeyRec, PRIVATE_SCREEN, 0) ||
        !dixRegisterScreenPrivateKey(&miDCDeviceKeyRec, pScreen, PRIVATE_DEVICE,
                                     0))
        return FALSE;

    pScreenPriv = calloc(1, sizeof(miDCScreenRec));
    if (!pScreenPriv)
        return FALSE;

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miDCCloseScreen;

    dixSetPrivate(&pScreen->devPrivates, miDCScreenKey, pScreenPriv);

    if (!miSpriteInitialize(pScreen, screenFuncs)) {
        free((void *) pScreenPriv);
        return FALSE;
    }
    return TRUE;
}

static void
miDCSwitchScreenCursor(ScreenPtr pScreen, CursorPtr pCursor, PixmapPtr sourceBits, PixmapPtr maskBits, PicturePtr pPicture)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);

    if (pScreenPriv->sourceBits)
        (*pScreen->DestroyPixmap)(pScreenPriv->sourceBits);
    pScreenPriv->sourceBits = sourceBits;

    if (pScreenPriv->maskBits)
        (*pScreen->DestroyPixmap)(pScreenPriv->maskBits);
    pScreenPriv->maskBits = maskBits;

    if (pScreenPriv->pPicture)
        FreePicture(pScreenPriv->pPicture, 0);
    pScreenPriv->pPicture = pPicture;

    pScreenPriv->pCursor = pCursor;
}

static Bool
miDCCloseScreen(ScreenPtr pScreen)
{
    miDCScreenPtr pScreenPriv;

    pScreenPriv = (miDCScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                   miDCScreenKey);
    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    miDCSwitchScreenCursor(pScreen, NULL, NULL, NULL, NULL);
    free((void *) pScreenPriv);
    return (*pScreen->CloseScreen) (pScreen);
}

Bool
miDCRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

#define EnsurePicture(picture,draw,win) (picture || miDCMakePicture(&picture,draw,win))

static PicturePtr
miDCMakePicture(PicturePtr * ppPicture, DrawablePtr pDraw, WindowPtr pWin)
{
    PictFormatPtr pFormat;
    XID subwindow_mode = IncludeInferiors;
    PicturePtr pPicture;
    int error;

    pFormat = PictureWindowFormat(pWin);
    if (!pFormat)
        return 0;
    pPicture = CreatePicture(0, pDraw, pFormat,
                             CPSubwindowMode, &subwindow_mode,
                             serverClient, &error);
    *ppPicture = pPicture;
    return pPicture;
}

static Bool
miDCRealize(ScreenPtr pScreen, CursorPtr pCursor)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    GCPtr pGC;
    ChangeGCVal gcvals;
    PixmapPtr   sourceBits, maskBits;

    if (pScreenPriv->pCursor == pCursor)
        return TRUE;

    if (pCursor->bits->argb) {
        PixmapPtr pPixmap;
        PictFormatPtr pFormat;
        int error;
        PicturePtr  pPicture;

        pFormat = PictureMatchFormat(pScreen, 32, PICT_a8r8g8b8);
        if (!pFormat)
            return FALSE;

        pPixmap = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
                                            pCursor->bits->height, 32,
                                            CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pPixmap)
            return FALSE;

        pGC = GetScratchGC(32, pScreen);
        if (!pGC) {
            (*pScreen->DestroyPixmap) (pPixmap);
            return FALSE;
        }
        ValidateGC(&pPixmap->drawable, pGC);
        (*pGC->ops->PutImage) (&pPixmap->drawable, pGC, 32,
                               0, 0, pCursor->bits->width,
                               pCursor->bits->height,
                               0, ZPixmap, (char *) pCursor->bits->argb);
        FreeScratchGC(pGC);
        pPicture = CreatePicture(0, &pPixmap->drawable,
                                 pFormat, 0, 0, serverClient, &error);
        (*pScreen->DestroyPixmap) (pPixmap);
        if (!pPicture)
            return FALSE;

        miDCSwitchScreenCursor(pScreen, pCursor, NULL, NULL, pPicture);
        return TRUE;
    }

    sourceBits = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
                                           pCursor->bits->height, 1, 0);
    if (!sourceBits)
        return FALSE;

    maskBits = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
                                         pCursor->bits->height, 1, 0);
    if (!maskBits) {
        (*pScreen->DestroyPixmap) (sourceBits);
        return FALSE;
    }

    /* create the two sets of bits, clipping as appropriate */

    pGC = GetScratchGC(1, pScreen);
    if (!pGC) {
        (*pScreen->DestroyPixmap) (sourceBits);
        (*pScreen->DestroyPixmap) (maskBits);
        return FALSE;
    }

    ValidateGC((DrawablePtr) sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) sourceBits, pGC, 1,
                           0, 0, pCursor->bits->width, pCursor->bits->height,
                           0, XYPixmap, (char *) pCursor->bits->source);
    gcvals.val = GXand;
    ChangeGC(NullClient, pGC, GCFunction, &gcvals);
    ValidateGC((DrawablePtr) sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) sourceBits, pGC, 1,
                           0, 0, pCursor->bits->width, pCursor->bits->height,
                           0, XYPixmap, (char *) pCursor->bits->mask);

    /* mask bits -- pCursor->mask & ~pCursor->source */
    gcvals.val = GXcopy;
    ChangeGC(NullClient, pGC, GCFunction, &gcvals);
    ValidateGC((DrawablePtr) maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) maskBits, pGC, 1,
                           0, 0, pCursor->bits->width, pCursor->bits->height,
                           0, XYPixmap, (char *) pCursor->bits->mask);
    gcvals.val = GXandInverted;
    ChangeGC(NullClient, pGC, GCFunction, &gcvals);
    ValidateGC((DrawablePtr) maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) maskBits, pGC, 1,
                           0, 0, pCursor->bits->width, pCursor->bits->height,
                           0, XYPixmap, (char *) pCursor->bits->source);
    FreeScratchGC(pGC);

    miDCSwitchScreenCursor(pScreen, pCursor, sourceBits, maskBits, NULL);
    return TRUE;
}

Bool
miDCUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);

    if (pCursor == pScreenPriv->pCursor)
        miDCSwitchScreenCursor(pScreen, NULL, NULL, NULL, NULL);
    return TRUE;
}

static void
miDCPutBits(DrawablePtr pDrawable,
            GCPtr sourceGC,
            GCPtr maskGC,
            int x_org,
            int y_org,
            unsigned w, unsigned h, unsigned long source, unsigned long mask)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pDrawable->pScreen->devPrivates, miDCScreenKey);
    ChangeGCVal gcval;
    int x, y;

    if (sourceGC->fgPixel != source) {
        gcval.val = source;
        ChangeGC(NullClient, sourceGC, GCForeground, &gcval);
    }
    if (sourceGC->serialNumber != pDrawable->serialNumber)
        ValidateGC(pDrawable, sourceGC);

    if (sourceGC->miTranslate) {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    }
    else {
        x = x_org;
        y = y_org;
    }

    (*sourceGC->ops->PushPixels) (sourceGC, pScreenPriv->sourceBits, pDrawable, w, h,
                                  x, y);
    if (maskGC->fgPixel != mask) {
        gcval.val = mask;
        ChangeGC(NullClient, maskGC, GCForeground, &gcval);
    }
    if (maskGC->serialNumber != pDrawable->serialNumber)
        ValidateGC(pDrawable, maskGC);

    if (maskGC->miTranslate) {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    }
    else {
        x = x_org;
        y = y_org;
    }

    (*maskGC->ops->PushPixels) (maskGC, pScreenPriv->maskBits, pDrawable, w, h, x, y);
}

static GCPtr
miDCMakeGC(WindowPtr pWin)
{
    GCPtr pGC;
    int status;
    XID gcvals[2];

    gcvals[0] = IncludeInferiors;
    gcvals[1] = FALSE;
    pGC = CreateGC((DrawablePtr) pWin,
                   GCSubwindowMode | GCGraphicsExposures, gcvals, &status,
                   (XID) 0, serverClient);
    return pGC;
}

Bool
miDCPutUpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor,
                int x, int y, unsigned long source, unsigned long mask)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    miDCBufferPtr pBuffer;
    WindowPtr pWin;

    if (!miDCRealize(pScreen, pCursor))
        return FALSE;

    pWin = pScreen->root;
    pBuffer = miGetDCDevice(pDev, pScreen);

    if (pScreenPriv->pPicture) {
        if (!EnsurePicture(pBuffer->pRootPicture, &pWin->drawable, pWin))
            return FALSE;
        CompositePicture(PictOpOver,
                         pScreenPriv->pPicture,
                         NULL,
                         pBuffer->pRootPicture,
                         0, 0, 0, 0,
                         x, y, pCursor->bits->width, pCursor->bits->height);
    }
    else
    {
        miDCPutBits((DrawablePtr) pWin,
                    pBuffer->pSourceGC, pBuffer->pMaskGC,
                    x, y, pCursor->bits->width, pCursor->bits->height,
                    source, mask);
    }
    return TRUE;
}

Bool
miDCSaveUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                    int x, int y, int w, int h)
{
    miDCBufferPtr pBuffer;
    PixmapPtr pSave;
    WindowPtr pWin;
    GCPtr pGC;

    pBuffer = miGetDCDevice(pDev, pScreen);

    pSave = pBuffer->pSave;
    pWin = pScreen->root;
    if (!pSave || pSave->drawable.width < w || pSave->drawable.height < h) {
        if (pSave)
            (*pScreen->DestroyPixmap) (pSave);
        pBuffer->pSave = pSave =
            (*pScreen->CreatePixmap) (pScreen, w, h, pScreen->rootDepth, 0);
        if (!pSave)
            return FALSE;
    }

    pGC = pBuffer->pSaveGC;
    if (pSave->drawable.serialNumber != pGC->serialNumber)
        ValidateGC((DrawablePtr) pSave, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
                           x, y, w, h, 0, 0);
    return TRUE;
}

Bool
miDCRestoreUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                       int x, int y, int w, int h)
{
    miDCBufferPtr pBuffer;
    PixmapPtr pSave;
    WindowPtr pWin;
    GCPtr pGC;

    pBuffer = miGetDCDevice(pDev, pScreen);
    pSave = pBuffer->pSave;

    pWin = pScreen->root;
    if (!pSave)
        return FALSE;

    pGC = pBuffer->pRestoreGC;
    if (pWin->drawable.serialNumber != pGC->serialNumber)
        ValidateGC((DrawablePtr) pWin, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
                           0, 0, w, h, x, y);
    return TRUE;
}

Bool
miDCDeviceInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miDCBufferPtr pBuffer;
    WindowPtr pWin;
    int i;

    if (!DevHasCursor(pDev))
        return TRUE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];

        pBuffer = calloc(1, sizeof(miDCBufferRec));
        if (!pBuffer)
            goto failure;

        dixSetScreenPrivate(&pDev->devPrivates, miDCDeviceKey, pScreen,
                            pBuffer);
        pWin = pScreen->root;

        pBuffer->pSourceGC = miDCMakeGC(pWin);
        if (!pBuffer->pSourceGC)
            goto failure;

        pBuffer->pMaskGC = miDCMakeGC(pWin);
        if (!pBuffer->pMaskGC)
            goto failure;

        pBuffer->pSaveGC = miDCMakeGC(pWin);
        if (!pBuffer->pSaveGC)
            goto failure;

        pBuffer->pRestoreGC = miDCMakeGC(pWin);
        if (!pBuffer->pRestoreGC)
            goto failure;

        pBuffer->pRootPicture = NULL;

        /* (re)allocated lazily depending on the cursor size */
        pBuffer->pSave = NULL;
    }

    return TRUE;

 failure:

    miDCDeviceCleanup(pDev, pScreen);

    return FALSE;
}

void
miDCDeviceCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miDCBufferPtr pBuffer;
    int i;

    if (DevHasCursor(pDev)) {
        for (i = 0; i < screenInfo.numScreens; i++) {
            pScreen = screenInfo.screens[i];

            pBuffer = miGetDCDevice(pDev, pScreen);

            if (pBuffer) {
                if (pBuffer->pSourceGC)
                    FreeGC(pBuffer->pSourceGC, (GContext) 0);
                if (pBuffer->pMaskGC)
                    FreeGC(pBuffer->pMaskGC, (GContext) 0);
                if (pBuffer->pSaveGC)
                    FreeGC(pBuffer->pSaveGC, (GContext) 0);
                if (pBuffer->pRestoreGC)
                    FreeGC(pBuffer->pRestoreGC, (GContext) 0);

                /* If a pRootPicture was allocated for a root window, it
                 * is freed when that root window is destroyed, so don't
                 * free it again here. */

                if (pBuffer->pSave)
                    (*pScreen->DestroyPixmap) (pBuffer->pSave);

                free(pBuffer);
                dixSetScreenPrivate(&pDev->devPrivates, miDCDeviceKey, pScreen,
                                    NULL);
            }
        }
    }
}
