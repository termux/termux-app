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
 *     DIX DBE code
 *
 *****************************************************************************/

/* INCLUDES */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "extnsionst.h"
#include "extinit.h"
#include "gcstruct.h"
#include "dixstruct.h"
#define NEED_DBE_PROTOCOL
#include "dbestruct.h"
#include "midbe.h"
#include "xace.h"

/* GLOBALS */

/* These are globals for use by DDX */
DevPrivateKeyRec dbeScreenPrivKeyRec;
DevPrivateKeyRec dbeWindowPrivKeyRec;

/* These are globals for use by DDX */
RESTYPE dbeDrawableResType;
RESTYPE dbeWindowPrivResType;

/* Used to generate DBE's BadBuffer error. */
static int dbeErrorBase;

/******************************************************************************
 *
 * DBE DIX Procedure: DbeStubScreen
 *
 * Description:
 *
 *     This is function stubs the function pointers in the given DBE screen
 *     private and increments the number of stubbed screens.
 *
 *****************************************************************************/

static void
DbeStubScreen(DbeScreenPrivPtr pDbeScreenPriv, int *nStubbedScreens)
{
    /* Stub DIX. */
    pDbeScreenPriv->SetupBackgroundPainter = NULL;

    /* Do not unwrap PositionWindow nor DestroyWindow.  If the DDX
     * initialization function failed, we assume that it did not wrap
     * PositionWindow.  Also, DestroyWindow is only wrapped if the DDX
     * initialization function succeeded.
     */

    /* Stub DDX. */
    pDbeScreenPriv->GetVisualInfo = NULL;
    pDbeScreenPriv->AllocBackBufferName = NULL;
    pDbeScreenPriv->SwapBuffers = NULL;
    pDbeScreenPriv->WinPrivDelete = NULL;

    (*nStubbedScreens)++;

}                               /* DbeStubScreen() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetVersion
 *
 * Description:
 *
 *     This function is for processing a DbeGetVersion request.
 *     This request returns the major and minor version numbers of this
 *     extension.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetVersion(ClientPtr client)
{
    /* REQUEST(xDbeGetVersionReq); */
    xDbeGetVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = DBE_MAJOR_VERSION,
        .minorVersion = DBE_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xDbeGetVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
    }

    WriteToClient(client, sizeof(xDbeGetVersionReply), &rep);

    return Success;

}                               /* ProcDbeGetVersion() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeAllocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeAllocateBackBufferName request.
 *     This request allocates a drawable ID used to refer to the back buffer
 *     of a window.
 *
 * Return Values:
 *
 *     BadAlloc    - server can not allocate resources
 *     BadIDChoice - id is out of range for client; id is already in use
 *     BadMatch    - window is not an InputOutput window;
 *                   visual of window is not on list returned by
 *                   DBEGetVisualInfo;
 *     BadValue    - invalid swap action is specified
 *     BadWindow   - window is not a valid window
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeAllocateBackBufferName(ClientPtr client)
{
    REQUEST(xDbeAllocateBackBufferNameReq);
    WindowPtr pWin;
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv;
    XdbeScreenVisualInfo scrVisInfo;
    register int i;
    Bool visualMatched = FALSE;
    xDbeSwapAction swapAction;
    VisualID visual;
    int status;
    int add_index;

    REQUEST_SIZE_MATCH(xDbeAllocateBackBufferNameReq);

    /* The window must be valid. */
    status = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (status != Success)
        return status;

    /* The window must be InputOutput. */
    if (pWin->drawable.class != InputOutput) {
        return BadMatch;
    }

    /* The swap action must be valid. */
    swapAction = stuff->swapAction;     /* use local var for performance. */
    if ((swapAction != XdbeUndefined) &&
        (swapAction != XdbeBackground) &&
        (swapAction != XdbeUntouched) && (swapAction != XdbeCopied)) {
        return BadValue;
    }

    /* The id must be in range and not already in use. */
    LEGAL_NEW_RESOURCE(stuff->buffer, client);

    /* The visual of the window must be in the list returned by
     * GetVisualInfo.
     */
    pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW(pWin);
    if (!pDbeScreenPriv->GetVisualInfo)
        return BadMatch;        /* screen doesn't support double buffering */

    if (!(*pDbeScreenPriv->GetVisualInfo) (pWin->drawable.pScreen, &scrVisInfo)) {
        /* GetVisualInfo() failed to allocate visual info data. */
        return BadAlloc;
    }

    /* See if the window's visual is on the list. */
    visual = wVisual(pWin);
    for (i = 0; (i < scrVisInfo.count) && !visualMatched; i++) {
        if (scrVisInfo.visinfo[i].visual == visual) {
            visualMatched = TRUE;
        }
    }

    /* Free what was allocated by the GetVisualInfo() call above. */
    free(scrVisInfo.visinfo);

    if (!visualMatched) {
        return BadMatch;
    }

    if ((pDbeWindowPriv = DBE_WINDOW_PRIV(pWin)) == NULL) {
        /* There is no buffer associated with the window.
         * Allocate a window priv.
         */

        pDbeWindowPriv = calloc(1, sizeof(DbeWindowPrivRec));
        if (!pDbeWindowPriv)
            return BadAlloc;

        /* Fill out window priv information. */
        pDbeWindowPriv->pWindow = pWin;
        pDbeWindowPriv->width = pWin->drawable.width;
        pDbeWindowPriv->height = pWin->drawable.height;
        pDbeWindowPriv->x = pWin->drawable.x;
        pDbeWindowPriv->y = pWin->drawable.y;
        pDbeWindowPriv->nBufferIDs = 0;

        /* Set the buffer ID array pointer to the initial (static) array). */
        pDbeWindowPriv->IDs = pDbeWindowPriv->initIDs;

        /* Initialize the buffer ID list. */
        pDbeWindowPriv->maxAvailableIDs = DBE_INIT_MAX_IDS;
        pDbeWindowPriv->IDs[0] = stuff->buffer;

        add_index = 0;
        for (i = 0; i < DBE_INIT_MAX_IDS; i++) {
            pDbeWindowPriv->IDs[i] = DBE_FREE_ID_ELEMENT;
        }

        /* Actually connect the window priv to the window. */
        dixSetPrivate(&pWin->devPrivates, dbeWindowPrivKey, pDbeWindowPriv);

    }                           /* if -- There is no buffer associated with the window. */

    else {
        /* A buffer is already associated with the window.
         * Add the new buffer ID to the array, reallocating the array memory
         * if necessary.
         */

        /* Determine if there is a free element in the ID array. */
        for (i = 0; i < pDbeWindowPriv->maxAvailableIDs; i++) {
            if (pDbeWindowPriv->IDs[i] == DBE_FREE_ID_ELEMENT) {
                /* There is still room in the ID array. */
                break;
            }
        }

        if (i == pDbeWindowPriv->maxAvailableIDs) {
            /* No more room in the ID array -- reallocate another array. */
            XID *pIDs;

            /* Setup an array pointer for the realloc operation below. */
            if (pDbeWindowPriv->maxAvailableIDs == DBE_INIT_MAX_IDS) {
                /* We will malloc a new array. */
                pIDs = NULL;
            }
            else {
                /* We will realloc a new array. */
                pIDs = pDbeWindowPriv->IDs;
            }

            /* malloc/realloc a new array and initialize all elements to 0. */
            pDbeWindowPriv->IDs =
                reallocarray(pIDs,
                             pDbeWindowPriv->maxAvailableIDs + DBE_INCR_MAX_IDS,
                             sizeof(XID));
            if (!pDbeWindowPriv->IDs) {
                return BadAlloc;
            }
            memset(&pDbeWindowPriv->IDs[pDbeWindowPriv->nBufferIDs], 0,
                   (pDbeWindowPriv->maxAvailableIDs + DBE_INCR_MAX_IDS -
                    pDbeWindowPriv->nBufferIDs) * sizeof(XID));

            if (pDbeWindowPriv->maxAvailableIDs == DBE_INIT_MAX_IDS) {
                /* We just went from using the initial (static) array to a
                 * newly allocated array.  Copy the IDs from the initial array
                 * to the new array.
                 */
                memcpy(pDbeWindowPriv->IDs, pDbeWindowPriv->initIDs,
                       DBE_INIT_MAX_IDS * sizeof(XID));
            }

            pDbeWindowPriv->maxAvailableIDs += DBE_INCR_MAX_IDS;
        }

        add_index = i;

    }                           /* else -- A buffer is already associated with the window. */

    /* Call the DDX routine to allocate the back buffer. */
    status = (*pDbeScreenPriv->AllocBackBufferName) (pWin, stuff->buffer,
                                                     stuff->swapAction);

    if (status == Success) {
        pDbeWindowPriv->IDs[add_index] = stuff->buffer;
        if (!AddResource(stuff->buffer, dbeWindowPrivResType,
                         (void *) pDbeWindowPriv)) {
            pDbeWindowPriv->IDs[add_index] = DBE_FREE_ID_ELEMENT;

            if (pDbeWindowPriv->nBufferIDs == 0) {
                status = BadAlloc;
                goto out_free;
            }
        }
    }
    else {
        /* The DDX buffer allocation routine failed for the first buffer of
         * this window.
         */
        if (pDbeWindowPriv->nBufferIDs == 0) {
            goto out_free;
        }
    }

    /* Increment the number of buffers (XIDs) associated with this window. */
    pDbeWindowPriv->nBufferIDs++;

    /* Set swap action on all calls. */
    pDbeWindowPriv->swapAction = stuff->swapAction;

    return status;

 out_free:
    dixSetPrivate(&pWin->devPrivates, dbeWindowPrivKey, NULL);
    free(pDbeWindowPriv);
    return status;

}                               /* ProcDbeAllocateBackBufferName() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeDeallocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeDeallocateBackBufferName request.
 *     This request frees a drawable ID that was obtained by a
 *     DbeAllocateBackBufferName request.
 *
 * Return Values:
 *
 *     BadBuffer - buffer to deallocate is not associated with a window
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeDeallocateBackBufferName(ClientPtr client)
{
    REQUEST(xDbeDeallocateBackBufferNameReq);
    DbeWindowPrivPtr pDbeWindowPriv;
    int rc, i;
    void *val;

    REQUEST_SIZE_MATCH(xDbeDeallocateBackBufferNameReq);

    /* Buffer name must be valid */
    rc = dixLookupResourceByType((void **) &pDbeWindowPriv, stuff->buffer,
                                 dbeWindowPrivResType, client,
                                 DixDestroyAccess);
    if (rc != Success)
        return rc;

    rc = dixLookupResourceByType(&val, stuff->buffer, dbeDrawableResType,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    /* Make sure that the id is valid for the window.
     * This is paranoid code since we already looked up the ID by type
     * above.
     */

    for (i = 0; i < pDbeWindowPriv->nBufferIDs; i++) {
        /* Loop through the ID list to find the ID. */
        if (pDbeWindowPriv->IDs[i] == stuff->buffer) {
            break;
        }
    }

    if (i == pDbeWindowPriv->nBufferIDs) {
        /* We did not find the ID in the ID list. */
        client->errorValue = stuff->buffer;
        return dbeErrorBase + DbeBadBuffer;
    }

    FreeResource(stuff->buffer, RT_NONE);

    return Success;

}                               /* ProcDbeDeallocateBackBufferName() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeSwapBuffers
 *
 * Description:
 *
 *     This function is for processing a DbeSwapBuffers request.
 *     This request swaps the buffers for all windows listed, applying the
 *     appropriate swap action for each window.
 *
 * Return Values:
 *
 *     BadAlloc  - local allocation failed; this return value is not defined
 *                 by the protocol
 *     BadMatch  - a window in request is not double-buffered; a window in
 *                 request is listed more than once
 *     BadValue  - invalid swap action is specified; no swap action is
 *                 specified
 *     BadWindow - a window in request is not valid
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeSwapBuffers(ClientPtr client)
{
    REQUEST(xDbeSwapBuffersReq);
    WindowPtr pWin;
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeSwapInfoPtr swapInfo;
    xDbeSwapInfo *dbeSwapInfo;
    int error;
    unsigned int i, j;
    unsigned int nStuff;
    int nStuff_i;       /* DDX API requires int for nStuff */

    REQUEST_AT_LEAST_SIZE(xDbeSwapBuffersReq);
    nStuff = stuff->n;          /* use local variable for performance. */

    if (nStuff == 0) {
        REQUEST_SIZE_MATCH(xDbeSwapBuffersReq);
        return Success;
    }

    if (nStuff > UINT32_MAX / sizeof(DbeSwapInfoRec))
        return BadAlloc;
    REQUEST_FIXED_SIZE(xDbeSwapBuffersReq, nStuff * sizeof(xDbeSwapInfo));

    /* Get to the swap info appended to the end of the request. */
    dbeSwapInfo = (xDbeSwapInfo *) &stuff[1];

    /* Allocate array to record swap information. */
    swapInfo = xallocarray(nStuff, sizeof(DbeSwapInfoRec));
    if (swapInfo == NULL) {
        return BadAlloc;
    }

    for (i = 0; i < nStuff; i++) {
        /* Check all windows to swap. */

        /* Each window must be a valid window - BadWindow. */
        error = dixLookupWindow(&pWin, dbeSwapInfo[i].window, client,
                                DixWriteAccess);
        if (error != Success) {
            free(swapInfo);
            return error;
        }

        /* Each window must be double-buffered - BadMatch. */
        if (DBE_WINDOW_PRIV(pWin) == NULL) {
            free(swapInfo);
            return BadMatch;
        }

        /* Each window must only be specified once - BadMatch. */
        for (j = i + 1; j < nStuff; j++) {
            if (dbeSwapInfo[i].window == dbeSwapInfo[j].window) {
                free(swapInfo);
                return BadMatch;
            }
        }

        /* Each swap action must be valid - BadValue. */
        if ((dbeSwapInfo[i].swapAction != XdbeUndefined) &&
            (dbeSwapInfo[i].swapAction != XdbeBackground) &&
            (dbeSwapInfo[i].swapAction != XdbeUntouched) &&
            (dbeSwapInfo[i].swapAction != XdbeCopied)) {
            free(swapInfo);
            return BadValue;
        }

        /* Everything checks out OK.  Fill in the swap info array. */
        swapInfo[i].pWindow = pWin;
        swapInfo[i].swapAction = dbeSwapInfo[i].swapAction;

    }                           /* for (i = 0; i < nStuff; i++) */

    /* Call the DDX routine to perform the swap(s).  The DDX routine should
     * scan the swap list (swap info), swap any buffers that it knows how to
     * handle, delete them from the list, and update nStuff to indicate how
     * many windows it did not handle.
     *
     * This scheme allows a range of sophistication in the DDX SwapBuffers()
     * implementation.  Naive implementations could just swap the first buffer
     * in the list, move the last buffer to the front, decrement nStuff, and
     * return.  The next level of sophistication could be to scan the whole
     * list for windows on the same screen.  Up another level, the DDX routine
     * could deal with cross-screen synchronization.
     */

    nStuff_i = nStuff;
    while (nStuff_i > 0) {
        pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW(swapInfo[0].pWindow);
        error = (*pDbeScreenPriv->SwapBuffers) (client, &nStuff_i, swapInfo);
        if (error != Success) {
            free(swapInfo);
            return error;
        }
    }

    free(swapInfo);
    return Success;

}                               /* ProcDbeSwapBuffers() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetVisualInfo
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetVisualInfo request.
 *     This request returns information about which visuals support
 *     double buffering.
 *
 * Return Values:
 *
 *     BadDrawable - value in screen specifiers is not a valid drawable
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetVisualInfo(ClientPtr client)
{
    REQUEST(xDbeGetVisualInfoReq);
    DbeScreenPrivPtr pDbeScreenPriv;
    xDbeGetVisualInfoReply rep;
    Drawable *drawables;
    DrawablePtr *pDrawables = NULL;
    register int i, j, rc;
    register int count;         /* number of visual infos in reply */
    register int length;        /* length of reply */
    ScreenPtr pScreen;
    XdbeScreenVisualInfo *pScrVisInfo;

    REQUEST_AT_LEAST_SIZE(xDbeGetVisualInfoReq);
    if (stuff->n > UINT32_MAX / sizeof(CARD32))
        return BadLength;
    REQUEST_FIXED_SIZE(xDbeGetVisualInfoReq, stuff->n * sizeof(CARD32));

    if (stuff->n > UINT32_MAX / sizeof(DrawablePtr))
        return BadAlloc;
    /* Make sure any specified drawables are valid. */
    if (stuff->n != 0) {
        if (!(pDrawables = xallocarray(stuff->n, sizeof(DrawablePtr)))) {
            return BadAlloc;
        }

        drawables = (Drawable *) &stuff[1];

        for (i = 0; i < stuff->n; i++) {
            rc = dixLookupDrawable(pDrawables + i, drawables[i], client, 0,
                                   DixGetAttrAccess);
            if (rc != Success) {
                free(pDrawables);
                return rc;
            }
        }
    }

    count = (stuff->n == 0) ? screenInfo.numScreens : stuff->n;
    if (!(pScrVisInfo = calloc(count, sizeof(XdbeScreenVisualInfo)))) {
        free(pDrawables);

        return BadAlloc;
    }

    length = 0;

    for (i = 0; i < count; i++) {
        pScreen = (stuff->n == 0) ? screenInfo.screens[i] :
            pDrawables[i]->pScreen;
        pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);

        rc = XaceHook(XACE_SCREEN_ACCESS, client, pScreen, DixGetAttrAccess);
        if (rc != Success)
            goto freeScrVisInfo;

        if (!(*pDbeScreenPriv->GetVisualInfo) (pScreen, &pScrVisInfo[i])) {
            /* We failed to alloc pScrVisInfo[i].visinfo. */
            rc = BadAlloc;

            /* Free visinfos that we allocated for previous screen infos. */
            goto freeScrVisInfo;
        }

        /* Account for n, number of xDbeVisInfo items in list. */
        length += sizeof(CARD32);

        /* Account for n xDbeVisInfo items */
        length += pScrVisInfo[i].count * sizeof(xDbeVisInfo);
    }

    rep = (xDbeGetVisualInfoReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(length),
        .m = count
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.m);
    }

    /* Send off reply. */
    WriteToClient(client, sizeof(xDbeGetVisualInfoReply), &rep);

    for (i = 0; i < count; i++) {
        CARD32 data32;

        /* For each screen in the reply, send off the visual info */

        /* Send off number of visuals. */
        data32 = (CARD32) pScrVisInfo[i].count;

        if (client->swapped) {
            swapl(&data32);
        }

        WriteToClient(client, sizeof(CARD32), &data32);

        /* Now send off visual info items. */
        for (j = 0; j < pScrVisInfo[i].count; j++) {
            xDbeVisInfo visInfo;

            /* Copy the data in the client data structure to a protocol
             * data structure.  We will send data to the client from the
             * protocol data structure.
             */

            visInfo.visualID = (CARD32) pScrVisInfo[i].visinfo[j].visual;
            visInfo.depth = (CARD8) pScrVisInfo[i].visinfo[j].depth;
            visInfo.perfLevel = (CARD8) pScrVisInfo[i].visinfo[j].perflevel;

            if (client->swapped) {
                swapl(&visInfo.visualID);

                /* We do not need to swap depth and perfLevel since they are
                 * already 1 byte quantities.
                 */
            }

            /* Write visualID(32), depth(8), perfLevel(8), and pad(16). */
            WriteToClient(client, 2 * sizeof(CARD32), &visInfo.visualID);
        }
    }

    rc = Success;

 freeScrVisInfo:
    /* Clean up memory. */
    for (i = 0; i < count; i++) {
        free(pScrVisInfo[i].visinfo);
    }
    free(pScrVisInfo);

    free(pDrawables);

    return rc;

}                               /* ProcDbeGetVisualInfo() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetbackBufferAttributes
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetbackBufferAttributes
 *     request.  This request returns information about a back buffer.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetBackBufferAttributes(ClientPtr client)
{
    REQUEST(xDbeGetBackBufferAttributesReq);
    xDbeGetBackBufferAttributesReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DbeWindowPrivPtr pDbeWindowPriv;
    int rc;

    REQUEST_SIZE_MATCH(xDbeGetBackBufferAttributesReq);

    rc = dixLookupResourceByType((void **) &pDbeWindowPriv, stuff->buffer,
                                 dbeWindowPrivResType, client,
                                 DixGetAttrAccess);
    if (rc == Success) {
        rep.attributes = pDbeWindowPriv->pWindow->drawable.id;
    }
    else {
        rep.attributes = None;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.attributes);
    }

    WriteToClient(client, sizeof(xDbeGetBackBufferAttributesReply), &rep);
    return Success;

}                               /* ProcDbeGetbackBufferAttributes() */

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeDispatch
 *
 * Description:
 *
 *     This function dispatches DBE requests.
 *
 *****************************************************************************/

static int
ProcDbeDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_DbeGetVersion:
        return (ProcDbeGetVersion(client));

    case X_DbeAllocateBackBufferName:
        return (ProcDbeAllocateBackBufferName(client));

    case X_DbeDeallocateBackBufferName:
        return (ProcDbeDeallocateBackBufferName(client));

    case X_DbeSwapBuffers:
        return (ProcDbeSwapBuffers(client));

    case X_DbeBeginIdiom:
        return Success;

    case X_DbeEndIdiom:
        return Success;

    case X_DbeGetVisualInfo:
        return (ProcDbeGetVisualInfo(client));

    case X_DbeGetBackBufferAttributes:
        return (ProcDbeGetBackBufferAttributes(client));

    default:
        return BadRequest;
    }

}                               /* ProcDbeDispatch() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeGetVersion
 *
 * Description:
 *
 *     This function is for processing a DbeGetVersion request on a swapped
 *     server.  This request returns the major and minor version numbers of
 *     this extension.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeGetVersion(ClientPtr client)
{
    REQUEST(xDbeGetVersionReq);

    swaps(&stuff->length);
    return (ProcDbeGetVersion(client));

}                               /* SProcDbeGetVersion() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeAllocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeAllocateBackBufferName request on
 *     a swapped server.  This request allocates a drawable ID used to refer
 *     to the back buffer of a window.
 *
 * Return Values:
 *
 *     BadAlloc    - server can not allocate resources
 *     BadIDChoice - id is out of range for client; id is already in use
 *     BadMatch    - window is not an InputOutput window;
 *                   visual of window is not on list returned by
 *                   DBEGetVisualInfo;
 *     BadValue    - invalid swap action is specified
 *     BadWindow   - window is not a valid window
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeAllocateBackBufferName(ClientPtr client)
{
    REQUEST(xDbeAllocateBackBufferNameReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDbeAllocateBackBufferNameReq);

    swapl(&stuff->window);
    swapl(&stuff->buffer);
    /* stuff->swapAction is a byte.  We do not need to swap this field. */

    return (ProcDbeAllocateBackBufferName(client));

}                               /* SProcDbeAllocateBackBufferName() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeDeallocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeDeallocateBackBufferName request
 *     on a swapped server.  This request frees a drawable ID that was
 *     obtained by a DbeAllocateBackBufferName request.
 *
 * Return Values:
 *
 *     BadBuffer - buffer to deallocate is not associated with a window
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeDeallocateBackBufferName(ClientPtr client)
{
    REQUEST(xDbeDeallocateBackBufferNameReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDbeDeallocateBackBufferNameReq);

    swapl(&stuff->buffer);

    return (ProcDbeDeallocateBackBufferName(client));

}                               /* SProcDbeDeallocateBackBufferName() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeSwapBuffers
 *
 * Description:
 *
 *     This function is for processing a DbeSwapBuffers request on a swapped
 *     server.  This request swaps the buffers for all windows listed,
 *     applying the appropriate swap action for each window.
 *
 * Return Values:
 *
 *     BadMatch  - a window in request is not double-buffered; a window in
 *                 request is listed more than once; all windows in request do
 *                 not have the same root
 *     BadValue  - invalid swap action is specified
 *     BadWindow - a window in request is not valid
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeSwapBuffers(ClientPtr client)
{
    REQUEST(xDbeSwapBuffersReq);
    unsigned int i;
    xDbeSwapInfo *pSwapInfo;

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xDbeSwapBuffersReq);

    swapl(&stuff->n);
    if (stuff->n > UINT32_MAX / sizeof(DbeSwapInfoRec))
        return BadLength;
    REQUEST_FIXED_SIZE(xDbeSwapBuffersReq, stuff->n * sizeof(xDbeSwapInfo));

    if (stuff->n != 0) {
        pSwapInfo = (xDbeSwapInfo *) stuff + 1;

        /* The swap info following the fix part of this request is a window(32)
         * followed by a 1 byte swap action and then 3 pad bytes.  We only need
         * to swap the window information.
         */
        for (i = 0; i < stuff->n; i++) {
            swapl(&pSwapInfo->window);
        }
    }

    return (ProcDbeSwapBuffers(client));

}                               /* SProcDbeSwapBuffers() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeGetVisualInfo
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetVisualInfo request on a
 *     swapped server.  This request returns information about which visuals
 *     support double buffering.
 *
 * Return Values:
 *
 *     BadDrawable - value in screen specifiers is not a valid drawable
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeGetVisualInfo(ClientPtr client)
{
    REQUEST(xDbeGetVisualInfoReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xDbeGetVisualInfoReq);

    swapl(&stuff->n);
    SwapRestL(stuff);

    return (ProcDbeGetVisualInfo(client));

}                               /* SProcDbeGetVisualInfo() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeGetbackBufferAttributes
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetbackBufferAttributes
 *     request on a swapped server.  This request returns information about a
 *     back buffer.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeGetBackBufferAttributes(ClientPtr client)
{
    REQUEST(xDbeGetBackBufferAttributesReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDbeGetBackBufferAttributesReq);

    swapl(&stuff->buffer);

    return (ProcDbeGetBackBufferAttributes(client));

}                               /* SProcDbeGetBackBufferAttributes() */

/******************************************************************************
 *
 * DBE DIX Procedure: SProcDbeDispatch
 *
 * Description:
 *
 *     This function dispatches DBE requests on a swapped server.
 *
 *****************************************************************************/

static int _X_COLD
SProcDbeDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_DbeGetVersion:
        return (SProcDbeGetVersion(client));

    case X_DbeAllocateBackBufferName:
        return (SProcDbeAllocateBackBufferName(client));

    case X_DbeDeallocateBackBufferName:
        return (SProcDbeDeallocateBackBufferName(client));

    case X_DbeSwapBuffers:
        return (SProcDbeSwapBuffers(client));

    case X_DbeBeginIdiom:
        return Success;

    case X_DbeEndIdiom:
        return Success;

    case X_DbeGetVisualInfo:
        return (SProcDbeGetVisualInfo(client));

    case X_DbeGetBackBufferAttributes:
        return (SProcDbeGetBackBufferAttributes(client));

    default:
        return BadRequest;
    }

}                               /* SProcDbeDispatch() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeSetupBackgroundPainter
 *
 * Description:
 *
 *     This function sets up pGC to clear pixmaps.
 *
 * Return Values:
 *
 *     TRUE  - setup was successful
 *     FALSE - the window's background state is NONE
 *
 *****************************************************************************/

static Bool
DbeSetupBackgroundPainter(WindowPtr pWin, GCPtr pGC)
{
    ChangeGCVal gcvalues[4];
    int ts_x_origin, ts_y_origin;
    PixUnion background;
    int backgroundState;
    Mask gcmask;

    /* First take care of any ParentRelative stuff by altering the
     * tile/stipple origin to match the coordinates of the upper-left
     * corner of the first ancestor without a ParentRelative background.
     * This coordinate is, of course, negative.
     */
    ts_x_origin = ts_y_origin = 0;
    while (pWin->backgroundState == ParentRelative) {
        ts_x_origin -= pWin->origin.x;
        ts_y_origin -= pWin->origin.y;

        pWin = pWin->parent;
    }
    backgroundState = pWin->backgroundState;
    background = pWin->background;

    switch (backgroundState) {
    case BackgroundPixel:
        gcvalues[0].val = background.pixel;
        gcvalues[1].val = FillSolid;
        gcmask = GCForeground | GCFillStyle;
        break;

    case BackgroundPixmap:
        gcvalues[0].val = FillTiled;
        gcvalues[1].ptr = background.pixmap;
        gcvalues[2].val = ts_x_origin;
        gcvalues[3].val = ts_y_origin;
        gcmask = GCFillStyle | GCTile | GCTileStipXOrigin | GCTileStipYOrigin;
        break;

    default:
        /* pWin->backgroundState == None */
        return FALSE;
    }

    return ChangeGC(NullClient, pGC, gcmask, gcvalues) == 0;
}                               /* DbeSetupBackgroundPainter() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeDrawableDelete
 *
 * Description:
 *
 *     This is the resource delete function for dbeDrawableResType.
 *     It is registered when the drawable resource type is created in
 *     DbeExtensionInit().
 *
 *     To make resource deletion simple, we do not do anything in this function
 *     and leave all resource deletion to DbeWindowPrivDelete(), which will
 *     eventually be called or already has been called.  Deletion functions are
 *     not guaranteed to be called in any particular order.
 *
 *****************************************************************************/
static int
DbeDrawableDelete(void *pDrawable, XID id)
{
    return Success;

}                               /* DbeDrawableDelete() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeWindowPrivDelete
 *
 * Description:
 *
 *     This is the resource delete function for dbeWindowPrivResType.
 *     It is registered when the drawable resource type is created in
 *     DbeExtensionInit().
 *
 *****************************************************************************/
static int
DbeWindowPrivDelete(void *pDbeWinPriv, XID id)
{
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv = (DbeWindowPrivPtr) pDbeWinPriv;
    int i;

    /*
     **************************************************************************
     ** Remove the buffer ID from the ID array.
     **************************************************************************
     */

    /* Find the ID in the ID array. */
    i = 0;
    while ((i < pDbeWindowPriv->nBufferIDs) && (pDbeWindowPriv->IDs[i] != id)) {
        i++;
    }

    if (i == pDbeWindowPriv->nBufferIDs) {
        /* We did not find the ID in the array.  We should never get here. */
        return BadValue;
    }

    /* Remove the ID from the array. */

    if (i < (pDbeWindowPriv->nBufferIDs - 1)) {
        /* Compress the buffer ID array, overwriting the ID in the process. */
        memmove(&pDbeWindowPriv->IDs[i], &pDbeWindowPriv->IDs[i + 1],
                (pDbeWindowPriv->nBufferIDs - i - 1) * sizeof(XID));
    }
    else {
        /* We are removing the last ID in the array, in which case, the
         * assignment below is all that we need to do.
         */
    }
    pDbeWindowPriv->IDs[pDbeWindowPriv->nBufferIDs - 1] = DBE_FREE_ID_ELEMENT;

    pDbeWindowPriv->nBufferIDs--;

    /* If an extended array was allocated, then check to see if the remaining
     * buffer IDs will fit in the static array.
     */

    if ((pDbeWindowPriv->maxAvailableIDs > DBE_INIT_MAX_IDS) &&
        (pDbeWindowPriv->nBufferIDs == DBE_INIT_MAX_IDS)) {
        /* Copy the IDs back into the static array. */
        memcpy(pDbeWindowPriv->initIDs, pDbeWindowPriv->IDs,
               DBE_INIT_MAX_IDS * sizeof(XID));

        /* Free the extended array; use the static array. */
        free(pDbeWindowPriv->IDs);
        pDbeWindowPriv->IDs = pDbeWindowPriv->initIDs;
        pDbeWindowPriv->maxAvailableIDs = DBE_INIT_MAX_IDS;
    }

    /*
     **************************************************************************
     ** Perform DDX level tasks.
     **************************************************************************
     */

    pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW_PRIV((DbeWindowPrivPtr)
                                                      pDbeWindowPriv);
    (*pDbeScreenPriv->WinPrivDelete) ((DbeWindowPrivPtr) pDbeWindowPriv, id);

    /*
     **************************************************************************
     ** Perform miscellaneous tasks if this is the last buffer associated
     ** with the window.
     **************************************************************************
     */

    if (pDbeWindowPriv->nBufferIDs == 0) {
        /* Reset the DBE window priv pointer. */
        dixSetPrivate(&pDbeWindowPriv->pWindow->devPrivates, dbeWindowPrivKey,
                      NULL);

        /* We are done with the window priv. */
        free(pDbeWindowPriv);
    }

    return Success;

}                               /* DbeWindowPrivDelete() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeResetProc
 *
 * Description:
 *
 *     This routine is called at the end of every server generation.
 *     It deallocates any memory reserved for the extension and performs any
 *     other tasks related to shutting down the extension.
 *
 *****************************************************************************/
static void
DbeResetProc(ExtensionEntry * extEntry)
{
    int i;
    ScreenPtr pScreen;
    DbeScreenPrivPtr pDbeScreenPriv;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];
        pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);

        if (pDbeScreenPriv) {
            /* Unwrap DestroyWindow, which was wrapped in DbeExtensionInit(). */
            pScreen->DestroyWindow = pDbeScreenPriv->DestroyWindow;
            pScreen->PositionWindow = pDbeScreenPriv->PositionWindow;
            free(pDbeScreenPriv);
        }
    }
}                               /* DbeResetProc() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeDestroyWindow
 *
 * Description:
 *
 *     This is the wrapper for pScreen->DestroyWindow.
 *     This function frees buffer resources for a window before it is
 *     destroyed.
 *
 *****************************************************************************/

static Bool
DbeDestroyWindow(WindowPtr pWin)
{
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv;
    ScreenPtr pScreen;
    Bool ret;

    /*
     **************************************************************************
     ** 1. Unwrap the member routine.
     **************************************************************************
     */

    pScreen = pWin->drawable.pScreen;
    pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);
    pScreen->DestroyWindow = pDbeScreenPriv->DestroyWindow;

    /*
     **************************************************************************
     ** 2. Do any work necessary before the member routine is called.
     **
     **    Call the window priv delete function for all buffer IDs associated
     **    with this window.
     **************************************************************************
     */

    if ((pDbeWindowPriv = DBE_WINDOW_PRIV(pWin))) {
        while (pDbeWindowPriv) {
            /* *DbeWinPrivDelete() will free the window private and set it to
             * NULL if there are no more buffer IDs associated with this
             * window.
             */
            FreeResource(pDbeWindowPriv->IDs[0], RT_NONE);
            pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);
        }
    }

    /*
     **************************************************************************
     ** 3. Call the member routine, saving its result if necessary.
     **************************************************************************
     */

    ret = (*pScreen->DestroyWindow) (pWin);

    /*
     **************************************************************************
     ** 4. Rewrap the member routine, restoring the wrapper value first in case
     **    the wrapper (or something that it wrapped) change this value.
     **************************************************************************
     */

    pDbeScreenPriv->DestroyWindow = pScreen->DestroyWindow;
    pScreen->DestroyWindow = DbeDestroyWindow;

    /*
     **************************************************************************
     ** 5. Do any work necessary after the member routine has been called.
     **
     **    In this case we do not need to do anything.
     **************************************************************************
     */

    return ret;

}                               /* DbeDestroyWindow() */

/******************************************************************************
 *
 * DBE DIX Procedure: DbeExtensionInit
 *
 * Description:
 *
 *     Called from InitExtensions in main()
 *
 *****************************************************************************/

void
DbeExtensionInit(void)
{
    ExtensionEntry *extEntry;
    register int i, j;
    ScreenPtr pScreen = NULL;
    DbeScreenPrivPtr pDbeScreenPriv;
    int nStubbedScreens = 0;
    Bool ddxInitSuccess;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        return;
#endif

    /* Create the resource types. */
    dbeDrawableResType =
        CreateNewResourceType(DbeDrawableDelete, "dbeDrawable");
    if (!dbeDrawableResType)
        return;
    dbeDrawableResType |= RC_DRAWABLE;

    dbeWindowPrivResType =
        CreateNewResourceType(DbeWindowPrivDelete, "dbeWindow");
    if (!dbeWindowPrivResType)
        return;

    if (!dixRegisterPrivateKey(&dbeScreenPrivKeyRec, PRIVATE_SCREEN, 0))
        return;

    if (!dixRegisterPrivateKey(&dbeWindowPrivKeyRec, PRIVATE_WINDOW, 0))
        return;

    for (i = 0; i < screenInfo.numScreens; i++) {
        /* For each screen, set up DBE screen privates and init DIX and DDX
         * interface.
         */

        pScreen = screenInfo.screens[i];

        if (!(pDbeScreenPriv = malloc(sizeof(DbeScreenPrivRec)))) {
            /* If we can not alloc a window or screen private,
             * then free any privates that we already alloc'ed and return
             */

            for (j = 0; j < i; j++) {
                free(dixLookupPrivate(&screenInfo.screens[j]->devPrivates,
                                      dbeScreenPrivKey));
                dixSetPrivate(&screenInfo.screens[j]->devPrivates,
                              dbeScreenPrivKey, NULL);
            }
            return;
        }

        dixSetPrivate(&pScreen->devPrivates, dbeScreenPrivKey, pDbeScreenPriv);

        {
            /* We don't have DDX support for DBE anymore */

#ifndef DISABLE_MI_DBE_BY_DEFAULT
            /* Setup DIX. */
            pDbeScreenPriv->SetupBackgroundPainter = DbeSetupBackgroundPainter;

            /* Setup DDX. */
            ddxInitSuccess = miDbeInit(pScreen, pDbeScreenPriv);

            /* DDX DBE initialization may have the side affect of
             * reallocating pDbeScreenPriv, so we need to update it.
             */
            pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);

            if (ddxInitSuccess) {
                /* Wrap DestroyWindow.  The DDX initialization function
                 * already wrapped PositionWindow for us.
                 */

                pDbeScreenPriv->DestroyWindow = pScreen->DestroyWindow;
                pScreen->DestroyWindow = DbeDestroyWindow;
            }
            else {
                /* DDX initialization failed.  Stub the screen. */
                DbeStubScreen(pDbeScreenPriv, &nStubbedScreens);
            }
#else
            DbeStubScreen(pDbeScreenPriv, &nStubbedScreens);
#endif

        }

    }                           /* for (i = 0; i < screenInfo.numScreens; i++) */

    if (nStubbedScreens == screenInfo.numScreens) {
        /* All screens stubbed.  Clean up and return. */

        for (i = 0; i < screenInfo.numScreens; i++) {
            free(dixLookupPrivate(&screenInfo.screens[i]->devPrivates,
                                  dbeScreenPrivKey));
            dixSetPrivate(&pScreen->devPrivates, dbeScreenPrivKey, NULL);
        }
        return;
    }

    /* Now add the extension. */
    extEntry = AddExtension(DBE_PROTOCOL_NAME, DbeNumberEvents,
                            DbeNumberErrors, ProcDbeDispatch, SProcDbeDispatch,
                            DbeResetProc, StandardMinorOpcode);

    dbeErrorBase = extEntry->errorBase;
    SetResourceTypeErrorValue(dbeWindowPrivResType,
                              dbeErrorBase + DbeBadBuffer);
    SetResourceTypeErrorValue(dbeDrawableResType, dbeErrorBase + DbeBadBuffer);

}                               /* DbeExtensionInit() */
