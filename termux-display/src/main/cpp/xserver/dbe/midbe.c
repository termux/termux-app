/******************************************************************************
 *
 * Copyright (c) 1994, 1995  Hewlett-Packard Company
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Hewlett-Packard
 * Company shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Hewlett-Packard Company.
 *
 *     Machine-independent DBE code
 *
 *****************************************************************************/

/* INCLUDES */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "resource.h"
#include "opaque.h"
#include "dbestruct.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "inputstr.h"
#include "midbe.h"
#include "xace.h"

#include <stdio.h>


/******************************************************************************
 *
 * DBE MI Procedure: miDbeGetVisualInfo
 *
 * Description:
 *
 *     This is the MI function for the DbeGetVisualInfo request.  This function
 *     is called through pDbeScreenPriv->GetVisualInfo.  This function is also
 *     called for the DbeAllocateBackBufferName request at the extension level;
 *     it is called by ProcDbeAllocateBackBufferName() in dbe.c.
 *
 *     If memory allocation fails or we can not get the visual info, this
 *     function returns FALSE.  Otherwise, it returns TRUE for success.
 *
 *****************************************************************************/

static Bool
miDbeGetVisualInfo(ScreenPtr pScreen, XdbeScreenVisualInfo * pScrVisInfo)
{
    register int i, j, k;
    register int count;
    DepthPtr pDepth;
    XdbeVisualInfo *visInfo;

    /* Determine number of visuals for this screen. */
    for (i = 0, count = 0; i < pScreen->numDepths; i++) {
        count += pScreen->allowedDepths[i].numVids;
    }

    /* Allocate an array of XdbeVisualInfo items. */
    if (!(visInfo = xallocarray(count, sizeof(XdbeVisualInfo)))) {
        return FALSE;           /* memory alloc failure */
    }

    for (i = 0, k = 0; i < pScreen->numDepths; i++) {
        /* For each depth of this screen, get visual information. */

        pDepth = &pScreen->allowedDepths[i];

        for (j = 0; j < pDepth->numVids; j++) {
            /* For each visual for this depth of this screen, get visual ID
             * and visual depth.  Since this is MI code, we will always return
             * the same performance level for all visuals (0).  A higher
             * performance level value indicates higher performance.
             */
            visInfo[k].visual = pDepth->vids[j];
            visInfo[k].depth = pDepth->depth;
            visInfo[k].perflevel = 0;
            k++;
        }
    }

    /* Record the number of visuals and point visual_depth to
     * the array of visual info.
     */
    pScrVisInfo->count = count;
    pScrVisInfo->visinfo = visInfo;

    return TRUE;                /* success */

}                               /* miDbeGetVisualInfo() */

/******************************************************************************
 *
 * DBE MI Procedure: miAllocBackBufferName
 *
 * Description:
 *
 *     This is the MI function for the DbeAllocateBackBufferName request.
 *
 *****************************************************************************/

static int
miDbeAllocBackBufferName(WindowPtr pWin, XID bufId, int swapAction)
{
    ScreenPtr pScreen;
    DbeWindowPrivPtr pDbeWindowPriv;
    DbeScreenPrivPtr pDbeScreenPriv;
    GCPtr pGC;
    xRectangle clearRect;
    int rc;

    pScreen = pWin->drawable.pScreen;
    pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);

    if (pDbeWindowPriv->nBufferIDs == 0) {
        /* There is no buffer associated with the window.
         * We have to create the window priv priv.  Remember, the window
         * priv was created at the DIX level, so all we need to do is
         * create the priv priv and attach it to the priv.
         */

        pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);

        /* Get a front pixmap. */
        if (!(pDbeWindowPriv->pFrontBuffer =
              (*pScreen->CreatePixmap) (pScreen, pDbeWindowPriv->width,
                                        pDbeWindowPriv->height,
                                        pWin->drawable.depth, 0))) {
            return BadAlloc;
        }

        /* Get a back pixmap. */
        if (!(pDbeWindowPriv->pBackBuffer =
              (*pScreen->CreatePixmap) (pScreen, pDbeWindowPriv->width,
                                        pDbeWindowPriv->height,
                                        pWin->drawable.depth, 0))) {
            (*pScreen->DestroyPixmap) (pDbeWindowPriv->pFrontBuffer);
            return BadAlloc;
        }

        /* Security creation/labeling check. */
        rc = XaceHook(XACE_RESOURCE_ACCESS, serverClient, bufId,
                      dbeDrawableResType, pDbeWindowPriv->pBackBuffer,
                      RT_WINDOW, pWin, DixCreateAccess);

        /* Make the back pixmap a DBE drawable resource. */
        if (rc != Success || !AddResource(bufId, dbeDrawableResType,
                                          pDbeWindowPriv->pBackBuffer)) {
            /* free the buffer and the drawable resource */
            FreeResource(bufId, RT_NONE);
            return (rc == Success) ? BadAlloc : rc;
        }

        /* Clear the back buffer. */
        pGC = GetScratchGC(pWin->drawable.depth, pWin->drawable.pScreen);
        if ((*pDbeScreenPriv->SetupBackgroundPainter) (pWin, pGC)) {
            ValidateGC((DrawablePtr) pDbeWindowPriv->pBackBuffer, pGC);
            clearRect.x = clearRect.y = 0;
            clearRect.width = pDbeWindowPriv->pBackBuffer->drawable.width;
            clearRect.height = pDbeWindowPriv->pBackBuffer->drawable.height;
            (*pGC->ops->PolyFillRect) ((DrawablePtr) pDbeWindowPriv->
                                       pBackBuffer, pGC, 1, &clearRect);
        }
        FreeScratchGC(pGC);

    }                           /* if no buffer associated with the window */

    else {
        /* A buffer is already associated with the window.
         * Place the new buffer ID information at the head of the ID list.
         */

        /* Associate the new ID with an existing pixmap. */
        if (!AddResource(bufId, dbeDrawableResType,
                         (void *) pDbeWindowPriv->pBackBuffer)) {
            return BadAlloc;
        }

    }

    return Success;

}                               /* miDbeAllocBackBufferName() */

/******************************************************************************
 *
 * DBE MI Procedure: miDbeAliasBuffers
 *
 * Description:
 *
 *     This function associates all XIDs of a buffer with the back pixmap
 *     stored in the window priv.
 *
 *****************************************************************************/

static void
miDbeAliasBuffers(DbeWindowPrivPtr pDbeWindowPriv)
{
    int i;

    for (i = 0; i < pDbeWindowPriv->nBufferIDs; i++) {
        ChangeResourceValue(pDbeWindowPriv->IDs[i], dbeDrawableResType,
                            (void *) pDbeWindowPriv->pBackBuffer);
    }

}                               /* miDbeAliasBuffers() */

/******************************************************************************
 *
 * DBE MI Procedure: miDbeSwapBuffers
 *
 * Description:
 *
 *     This is the MI function for the DbeSwapBuffers request.
 *
 *****************************************************************************/

static int
miDbeSwapBuffers(ClientPtr client, int *pNumWindows, DbeSwapInfoPtr swapInfo)
{
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv;
    GCPtr pGC;
    WindowPtr pWin;
    PixmapPtr pTmpBuffer;
    xRectangle clearRect;

    pWin = swapInfo[0].pWindow;
    pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW(pWin);
    pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);
    pGC = GetScratchGC(pWin->drawable.depth, pWin->drawable.pScreen);

    /*
     **********************************************************************
     ** Setup before swap.
     **********************************************************************
     */

    switch (swapInfo[0].swapAction) {
    case XdbeUndefined:
        break;

    case XdbeBackground:
        break;

    case XdbeUntouched:
        ValidateGC((DrawablePtr) pDbeWindowPriv->pFrontBuffer, pGC);
        (*pGC->ops->CopyArea) ((DrawablePtr) pWin,
                               (DrawablePtr) pDbeWindowPriv->pFrontBuffer,
                               pGC, 0, 0, pWin->drawable.width,
                               pWin->drawable.height, 0, 0);
        break;

    case XdbeCopied:
        break;

    }

    /*
     **********************************************************************
     ** Swap.
     **********************************************************************
     */

    ValidateGC((DrawablePtr) pWin, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pDbeWindowPriv->pBackBuffer,
                           (DrawablePtr) pWin, pGC, 0, 0,
                           pWin->drawable.width, pWin->drawable.height, 0, 0);

    /*
     **********************************************************************
     ** Tasks after swap.
     **********************************************************************
     */

    switch (swapInfo[0].swapAction) {
    case XdbeUndefined:
        break;

    case XdbeBackground:
        if ((*pDbeScreenPriv->SetupBackgroundPainter) (pWin, pGC)) {
            ValidateGC((DrawablePtr) pDbeWindowPriv->pBackBuffer, pGC);
            clearRect.x = 0;
            clearRect.y = 0;
            clearRect.width = pDbeWindowPriv->pBackBuffer->drawable.width;
            clearRect.height = pDbeWindowPriv->pBackBuffer->drawable.height;
            (*pGC->ops->PolyFillRect) ((DrawablePtr) pDbeWindowPriv->
                                       pBackBuffer, pGC, 1, &clearRect);
        }
        break;

    case XdbeUntouched:
        /* Swap pixmap pointers. */
        pTmpBuffer = pDbeWindowPriv->pBackBuffer;
        pDbeWindowPriv->pBackBuffer = pDbeWindowPriv->pFrontBuffer;
        pDbeWindowPriv->pFrontBuffer = pTmpBuffer;

        miDbeAliasBuffers(pDbeWindowPriv);

        break;

    case XdbeCopied:
        break;

    }

    /* Remove the swapped window from the swap information array and decrement
     * pNumWindows to indicate to the DIX level how many windows were actually
     * swapped.
     */

    if (*pNumWindows > 1) {
        /* We were told to swap more than one window, but we only swapped the
         * first one.  Remove the first window in the list by moving the last
         * window to the beginning.
         */
        swapInfo[0].pWindow = swapInfo[*pNumWindows - 1].pWindow;
        swapInfo[0].swapAction = swapInfo[*pNumWindows - 1].swapAction;

        /* Clear the last window information just to be safe. */
        swapInfo[*pNumWindows - 1].pWindow = (WindowPtr) NULL;
        swapInfo[*pNumWindows - 1].swapAction = 0;
    }
    else {
        /* Clear the window information just to be safe. */
        swapInfo[0].pWindow = (WindowPtr) NULL;
        swapInfo[0].swapAction = 0;
    }

    (*pNumWindows)--;

    FreeScratchGC(pGC);

    return Success;

}                               /* miSwapBuffers() */

/******************************************************************************
 *
 * DBE MI Procedure: miDbeWinPrivDelete
 *
 * Description:
 *
 *     This is the MI function for deleting the dbeWindowPrivResType resource.
 *     This function is invoked indirectly by calling FreeResource() to free
 *     the resources associated with a DBE buffer ID.  There are 5 ways that
 *     miDbeWinPrivDelete() can be called by FreeResource().  They are:
 *
 *     - A DBE window is destroyed, in which case the DbeDestroyWindow()
 *       wrapper is invoked.  The wrapper calls FreeResource() for all DBE
 *       buffer IDs.
 *
 *     - miDbeAllocBackBufferName() calls FreeResource() to clean up resources
 *       after a buffer allocation failure.
 *
 *     - The PositionWindow wrapper, miDbePositionWindow(), calls
 *       FreeResource() when it fails to create buffers of the new size.
 *       FreeResource() is called for all DBE buffer IDs.
 *
 *     - FreeClientResources() calls FreeResource() when a client dies or the
 *       the server resets.
 *
 *     When FreeResource() is called for a DBE buffer ID, the delete function
 *     for the only other type of DBE resource, dbeDrawableResType, is also
 *     invoked.  This delete function (DbeDrawableDelete) is a NOOP to make
 *     resource deletion easier.  It is not guaranteed which delete function is
 *     called first.  Hence, we will let miDbeWinPrivDelete() free all DBE
 *     resources.
 *
 *     This function deletes/frees the following stuff associated with
 *     the window private:
 *
 *     - the ID node in the ID list representing the passed in ID.
 *
 *     In addition, pDbeWindowPriv->nBufferIDs is decremented.
 *
 *     If this function is called for the last/only buffer ID for a window,
 *     these are additionally deleted/freed:
 *
 *     - the front and back pixmaps
 *     - the window priv itself
 *
 *****************************************************************************/

static void
miDbeWinPrivDelete(DbeWindowPrivPtr pDbeWindowPriv, XID bufId)
{
    if (pDbeWindowPriv->nBufferIDs != 0) {
        /* We still have at least one more buffer ID associated with this
         * window.
         */
        return;
    }

    /* We have no more buffer IDs associated with this window.  We need to
     * free some stuff.
     */

    /* Destroy the front and back pixmaps. */
    if (pDbeWindowPriv->pFrontBuffer) {
        (*pDbeWindowPriv->pWindow->drawable.pScreen->
         DestroyPixmap) (pDbeWindowPriv->pFrontBuffer);
    }
    if (pDbeWindowPriv->pBackBuffer) {
        (*pDbeWindowPriv->pWindow->drawable.pScreen->
         DestroyPixmap) (pDbeWindowPriv->pBackBuffer);
    }
}                               /* miDbeWinPrivDelete() */

/******************************************************************************
 *
 * DBE MI Procedure: miDbePositionWindow
 *
 * Description:
 *
 *     This function was cloned from miMbxPositionWindow() in mimultibuf.c.
 *     This function resizes the buffer when the window is resized.
 *
 *****************************************************************************/

static Bool
miDbePositionWindow(WindowPtr pWin, int x, int y)
{
    ScreenPtr pScreen;
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv;
    int width, height;
    int dx, dy, dw, dh;
    int sourcex, sourcey;
    int destx, desty;
    int savewidth, saveheight;
    PixmapPtr pFrontBuffer;
    PixmapPtr pBackBuffer;
    Bool clear;
    GCPtr pGC;
    xRectangle clearRect;
    Bool ret;

    /*
     **************************************************************************
     ** 1. Unwrap the member routine.
     **************************************************************************
     */

    pScreen = pWin->drawable.pScreen;
    pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);
    pScreen->PositionWindow = pDbeScreenPriv->PositionWindow;

    /*
     **************************************************************************
     ** 2. Do any work necessary before the member routine is called.
     **
     **    In this case we do not need to do anything.
     **************************************************************************
     */

    /*
     **************************************************************************
     ** 3. Call the member routine, saving its result if necessary.
     **************************************************************************
     */

    ret = (*pScreen->PositionWindow) (pWin, x, y);

    /*
     **************************************************************************
     ** 4. Rewrap the member routine, restoring the wrapper value first in case
     **    the wrapper (or something that it wrapped) change this value.
     **************************************************************************
     */

    pDbeScreenPriv->PositionWindow = pScreen->PositionWindow;
    pScreen->PositionWindow = miDbePositionWindow;

    /*
     **************************************************************************
     ** 5. Do any work necessary after the member routine has been called.
     **************************************************************************
     */

    if (!(pDbeWindowPriv = DBE_WINDOW_PRIV(pWin))) {
        return ret;
    }

    if (pDbeWindowPriv->width == pWin->drawable.width &&
        pDbeWindowPriv->height == pWin->drawable.height) {
        return ret;
    }

    width = pWin->drawable.width;
    height = pWin->drawable.height;

    dx = pWin->drawable.x - pDbeWindowPriv->x;
    dy = pWin->drawable.y - pDbeWindowPriv->y;
    dw = width - pDbeWindowPriv->width;
    dh = height - pDbeWindowPriv->height;

    GravityTranslate(0, 0, -dx, -dy, dw, dh, pWin->bitGravity, &destx, &desty);

    clear = ((pDbeWindowPriv->width < (unsigned short) width) ||
             (pDbeWindowPriv->height < (unsigned short) height) ||
             (pWin->bitGravity == ForgetGravity));

    sourcex = 0;
    sourcey = 0;
    savewidth = pDbeWindowPriv->width;
    saveheight = pDbeWindowPriv->height;

    /* Clip rectangle to source and destination. */
    if (destx < 0) {
        savewidth += destx;
        sourcex -= destx;
        destx = 0;
    }

    if (destx + savewidth > width) {
        savewidth = width - destx;
    }

    if (desty < 0) {
        saveheight += desty;
        sourcey -= desty;
        desty = 0;
    }

    if (desty + saveheight > height) {
        saveheight = height - desty;
    }

    pDbeWindowPriv->width = width;
    pDbeWindowPriv->height = height;
    pDbeWindowPriv->x = pWin->drawable.x;
    pDbeWindowPriv->y = pWin->drawable.y;

    pGC = GetScratchGC(pWin->drawable.depth, pScreen);

    if (clear) {
        if ((*pDbeScreenPriv->SetupBackgroundPainter) (pWin, pGC)) {
            clearRect.x = 0;
            clearRect.y = 0;
            clearRect.width = width;
            clearRect.height = height;
        }
        else {
            clear = FALSE;
        }
    }

    /* Create DBE buffer pixmaps equal to size of resized window. */
    pFrontBuffer = (*pScreen->CreatePixmap) (pScreen, width, height,
                                             pWin->drawable.depth, 0);

    pBackBuffer = (*pScreen->CreatePixmap) (pScreen, width, height,
                                            pWin->drawable.depth, 0);

    if (!pFrontBuffer || !pBackBuffer) {
        /* We failed at creating 1 or 2 of the pixmaps. */

        if (pFrontBuffer) {
            (*pScreen->DestroyPixmap) (pFrontBuffer);
        }

        if (pBackBuffer) {
            (*pScreen->DestroyPixmap) (pBackBuffer);
        }

        /* Destroy all buffers for this window. */
        while (pDbeWindowPriv) {
            /* DbeWindowPrivDelete() will free the window private if there no
             * more buffer IDs associated with this window.
             */
            FreeResource(pDbeWindowPriv->IDs[0], RT_NONE);
            pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);
        }

        FreeScratchGC(pGC);
        return FALSE;
    }

    else {
        /* Clear out the new DBE buffer pixmaps. */

        /* I suppose this could avoid quite a bit of work if
         * it computed the minimal area required.
         */
        ValidateGC(&pFrontBuffer->drawable, pGC);
        if (clear) {
            (*pGC->ops->PolyFillRect) ((DrawablePtr) pFrontBuffer, pGC, 1,
                                       &clearRect);
        }
        /* Copy the contents of the old front pixmap to the new one. */
        if (pWin->bitGravity != ForgetGravity) {
            (*pGC->ops->CopyArea) ((DrawablePtr) pDbeWindowPriv->pFrontBuffer,
				   (DrawablePtr) pFrontBuffer, pGC,
				   sourcex, sourcey, savewidth, saveheight,
                                   destx, desty);
        }

        ValidateGC(&pBackBuffer->drawable, pGC);
        if (clear) {
            (*pGC->ops->PolyFillRect) ((DrawablePtr) pBackBuffer, pGC, 1,
                                       &clearRect);
        }
        /* Copy the contents of the old back pixmap to the new one. */
        if (pWin->bitGravity != ForgetGravity) {
            (*pGC->ops->CopyArea) ((DrawablePtr) pDbeWindowPriv->pBackBuffer,
				   (DrawablePtr) pBackBuffer, pGC,
                                   sourcex, sourcey, savewidth, saveheight,
                                   destx, desty);
        }

        /* Destroy the old pixmaps, and point the DBE window priv to the new
         * pixmaps.
         */

        (*pScreen->DestroyPixmap) (pDbeWindowPriv->pFrontBuffer);
        (*pScreen->DestroyPixmap) (pDbeWindowPriv->pBackBuffer);

        pDbeWindowPriv->pFrontBuffer = pFrontBuffer;
        pDbeWindowPriv->pBackBuffer = pBackBuffer;

        /* Make sure all XID are associated with the new back pixmap. */
        miDbeAliasBuffers(pDbeWindowPriv);

        FreeScratchGC(pGC);
    }

    return ret;

}                               /* miDbePositionWindow() */

/******************************************************************************
 *
 * DBE MI Procedure: miDbeInit
 *
 * Description:
 *
 *     This is the MI initialization function called by DbeExtensionInit().
 *
 *****************************************************************************/

Bool
miDbeInit(ScreenPtr pScreen, DbeScreenPrivPtr pDbeScreenPriv)
{
    /* Wrap functions. */
    pDbeScreenPriv->PositionWindow = pScreen->PositionWindow;
    pScreen->PositionWindow = miDbePositionWindow;

    /* Initialize the per-screen DBE function pointers. */
    pDbeScreenPriv->GetVisualInfo = miDbeGetVisualInfo;
    pDbeScreenPriv->AllocBackBufferName = miDbeAllocBackBufferName;
    pDbeScreenPriv->SwapBuffers = miDbeSwapBuffers;
    pDbeScreenPriv->WinPrivDelete = miDbeWinPrivDelete;

    return TRUE;

}                               /* miDbeInit() */
