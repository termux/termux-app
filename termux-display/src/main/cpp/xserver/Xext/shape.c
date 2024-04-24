/************************************************************

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

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

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
#include <X11/extensions/shapeproto.h>
#include "regionstr.h"
#include "gcstruct.h"
#include "extinit.h"
#include "protocol-versions.h"

typedef RegionPtr (*CreateDftPtr) (WindowPtr    /* pWin */
    );

static int ShapeFreeClient(void * /* data */ ,
                           XID    /* id */
    );
static int ShapeFreeEvents(void * /* data */ ,
                           XID    /* id */
    );
static void SShapeNotifyEvent(xShapeNotifyEvent * /* from */ ,
                              xShapeNotifyEvent *       /* to */
    );

/* SendShapeNotify, CreateBoundingShape and CreateClipShape are used
 * externally by the Xfixes extension and are now defined in window.h
 */

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

static int ShapeEventBase = 0;
static RESTYPE ClientType, ShapeEventType;      /* resource types for event masks */

/*
 * each window has a list of clients requesting
 * ShapeNotify events.  Each client has a resource
 * for each window it selects ShapeNotify input for,
 * this resource is used to delete the ShapeNotifyRec
 * entry from the per-window queue.
 */

typedef struct _ShapeEvent *ShapeEventPtr;

typedef struct _ShapeEvent {
    ShapeEventPtr next;
    ClientPtr client;
    WindowPtr window;
    XID clientResource;
} ShapeEventRec;

/****************
 * ShapeExtensionInit
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 ****************/

static int
RegionOperate(ClientPtr client,
              WindowPtr pWin,
              int kind,
              RegionPtr *destRgnp,
              RegionPtr srcRgn, int op, int xoff, int yoff, CreateDftPtr create)
{
    if (srcRgn && (xoff || yoff))
        RegionTranslate(srcRgn, xoff, yoff);
    if (!pWin->parent) {
        if (srcRgn)
            RegionDestroy(srcRgn);
        return Success;
    }

    /* May/30/2001:
     * The shape.PS specs say if src is None, existing shape is to be
     * removed (and so the op-code has no meaning in such removal);
     * see shape.PS, page 3, ShapeMask.
     */
    if (srcRgn == NULL) {
        if (*destRgnp != NULL) {
            RegionDestroy(*destRgnp);
            *destRgnp = 0;
            /* go on to remove shape and generate ShapeNotify */
        }
        else {
            /* May/30/2001:
             * The target currently has no shape in effect, so nothing to
             * do here.  The specs say that ShapeNotify is generated whenever
             * the client region is "modified"; since no modification is done
             * here, we do not generate that event.  The specs does not say
             * "it is an error to request removal when there is no shape in
             * effect", so we return good status.
             */
            return Success;
        }
    }
    else
        switch (op) {
        case ShapeSet:
            if (*destRgnp)
                RegionDestroy(*destRgnp);
            *destRgnp = srcRgn;
            srcRgn = 0;
            break;
        case ShapeUnion:
            if (*destRgnp)
                RegionUnion(*destRgnp, *destRgnp, srcRgn);
            break;
        case ShapeIntersect:
            if (*destRgnp)
                RegionIntersect(*destRgnp, *destRgnp, srcRgn);
            else {
                *destRgnp = srcRgn;
                srcRgn = 0;
            }
            break;
        case ShapeSubtract:
            if (!*destRgnp)
                *destRgnp = (*create) (pWin);
            RegionSubtract(*destRgnp, *destRgnp, srcRgn);
            break;
        case ShapeInvert:
            if (!*destRgnp)
                *destRgnp = RegionCreate((BoxPtr) 0, 0);
            else
                RegionSubtract(*destRgnp, srcRgn, *destRgnp);
            break;
        default:
            client->errorValue = op;
            return BadValue;
        }
    if (srcRgn)
        RegionDestroy(srcRgn);
    (*pWin->drawable.pScreen->SetShape) (pWin, kind);
    SendShapeNotify(pWin, kind);
    return Success;
}

RegionPtr
CreateBoundingShape(WindowPtr pWin)
{
    BoxRec extents;

    extents.x1 = -wBorderWidth(pWin);
    extents.y1 = -wBorderWidth(pWin);
    extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
    extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
    return RegionCreate(&extents, 1);
}

RegionPtr
CreateClipShape(WindowPtr pWin)
{
    BoxRec extents;

    extents.x1 = 0;
    extents.y1 = 0;
    extents.x2 = pWin->drawable.width;
    extents.y2 = pWin->drawable.height;
    return RegionCreate(&extents, 1);
}

static int
ProcShapeQueryVersion(ClientPtr client)
{
    xShapeQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_SHAPE_MAJOR_VERSION,
        .minorVersion = SERVER_SHAPE_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xShapeQueryVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xShapeQueryVersionReply), &rep);
    return Success;
}

/*****************
 * ProcShapeRectangles
 *
 *****************/

static int
ProcShapeRectangles(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xShapeRectanglesReq);
    xRectangle *prects;
    int nrects, ctype, rc;
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    CreateDftPtr createDefault;

    REQUEST_AT_LEAST_SIZE(xShapeRectanglesReq);
    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    if ((stuff->ordering != Unsorted) && (stuff->ordering != YSorted) &&
        (stuff->ordering != YXSorted) && (stuff->ordering != YXBanded)) {
        client->errorValue = stuff->ordering;
        return BadValue;
    }
    nrects = ((stuff->length << 2) - sizeof(xShapeRectanglesReq));
    if (nrects & 4)
        return BadLength;
    nrects >>= 3;
    prects = (xRectangle *) &stuff[1];
    ctype = VerifyRectOrder(nrects, prects, (int) stuff->ordering);
    if (ctype < 0)
        return BadMatch;
    srcRgn = RegionFromRects(nrects, prects, ctype);

    if (!pWin->optional)
        MakeWindowOptional(pWin);
    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

#ifdef PANORAMIX
static int
ProcPanoramiXShapeRectangles(ClientPtr client)
{
    REQUEST(xShapeRectanglesReq);
    PanoramiXRes *win;
    int j, result;

    REQUEST_AT_LEAST_SIZE(xShapeRectanglesReq);

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    FOR_NSCREENS(j) {
        stuff->dest = win->info[j].id;
        result = ProcShapeRectangles(client);
        if (result != Success)
            break;
    }
    return result;
}
#endif

/**************
 * ProcShapeMask
 **************/

static int
ProcShapeMask(ClientPtr client)
{
    WindowPtr pWin;
    ScreenPtr pScreen;

    REQUEST(xShapeMaskReq);
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    PixmapPtr pPixmap;
    CreateDftPtr createDefault;
    int rc;

    REQUEST_SIZE_MATCH(xShapeMaskReq);
    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    pScreen = pWin->drawable.pScreen;
    if (stuff->src == None)
        srcRgn = 0;
    else {
        rc = dixLookupResourceByType((void **) &pPixmap, stuff->src,
                                     RT_PIXMAP, client, DixReadAccess);
        if (rc != Success)
            return rc;
        if (pPixmap->drawable.pScreen != pScreen ||
            pPixmap->drawable.depth != 1)
            return BadMatch;
        srcRgn = BitmapToRegion(pScreen, pPixmap);
        if (!srcRgn)
            return BadAlloc;
    }

    if (!pWin->optional)
        MakeWindowOptional(pWin);
    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

#ifdef PANORAMIX
static int
ProcPanoramiXShapeMask(ClientPtr client)
{
    REQUEST(xShapeMaskReq);
    PanoramiXRes *win, *pmap;
    int j, result;

    REQUEST_SIZE_MATCH(xShapeMaskReq);

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    if (stuff->src != None) {
        result = dixLookupResourceByType((void **) &pmap, stuff->src,
                                         XRT_PIXMAP, client, DixReadAccess);
        if (result != Success)
            return result;
    }
    else
        pmap = NULL;

    FOR_NSCREENS(j) {
        stuff->dest = win->info[j].id;
        if (pmap)
            stuff->src = pmap->info[j].id;
        result = ProcShapeMask(client);
        if (result != Success)
            break;
    }
    return result;
}
#endif

/************
 * ProcShapeCombine
 ************/

static int
ProcShapeCombine(ClientPtr client)
{
    WindowPtr pSrcWin, pDestWin;

    REQUEST(xShapeCombineReq);
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    CreateDftPtr createDefault;
    CreateDftPtr createSrc;
    RegionPtr tmp;
    int rc;

    REQUEST_SIZE_MATCH(xShapeCombineReq);
    UpdateCurrentTime();
    rc = dixLookupWindow(&pDestWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    if (!pDestWin->optional)
        MakeWindowOptional(pDestWin);
    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }

    rc = dixLookupWindow(&pSrcWin, stuff->src, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->srcKind) {
    case ShapeBounding:
        srcRgn = wBoundingShape(pSrcWin);
        createSrc = CreateBoundingShape;
        break;
    case ShapeClip:
        srcRgn = wClipShape(pSrcWin);
        createSrc = CreateClipShape;
        break;
    case ShapeInput:
        srcRgn = wInputShape(pSrcWin);
        createSrc = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->srcKind;
        return BadValue;
    }
    if (pSrcWin->drawable.pScreen != pDestWin->drawable.pScreen) {
        return BadMatch;
    }

    if (srcRgn) {
        tmp = RegionCreate((BoxPtr) 0, 0);
        RegionCopy(tmp, srcRgn);
        srcRgn = tmp;
    }
    else
        srcRgn = (*createSrc) (pSrcWin);

    if (!pDestWin->optional)
        MakeWindowOptional(pDestWin);
    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pDestWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pDestWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pDestWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pDestWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

#ifdef PANORAMIX
static int
ProcPanoramiXShapeCombine(ClientPtr client)
{
    REQUEST(xShapeCombineReq);
    PanoramiXRes *win, *win2;
    int j, result;

    REQUEST_AT_LEAST_SIZE(xShapeCombineReq);

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &win2, stuff->src, XRT_WINDOW,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    FOR_NSCREENS(j) {
        stuff->dest = win->info[j].id;
        stuff->src = win2->info[j].id;
        result = ProcShapeCombine(client);
        if (result != Success)
            break;
    }
    return result;
}
#endif

/*************
 * ProcShapeOffset
 *************/

static int
ProcShapeOffset(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xShapeOffsetReq);
    RegionPtr srcRgn;
    int rc;

    REQUEST_SIZE_MATCH(xShapeOffsetReq);
    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        srcRgn = wBoundingShape(pWin);
        break;
    case ShapeClip:
        srcRgn = wClipShape(pWin);
        break;
    case ShapeInput:
        srcRgn = wInputShape(pWin);
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    if (srcRgn) {
        RegionTranslate(srcRgn, stuff->xOff, stuff->yOff);
        (*pWin->drawable.pScreen->SetShape) (pWin, stuff->destKind);
    }
    SendShapeNotify(pWin, (int) stuff->destKind);
    return Success;
}

#ifdef PANORAMIX
static int
ProcPanoramiXShapeOffset(ClientPtr client)
{
    REQUEST(xShapeOffsetReq);
    PanoramiXRes *win;
    int j, result;

    REQUEST_AT_LEAST_SIZE(xShapeOffsetReq);

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    FOR_NSCREENS(j) {
        stuff->dest = win->info[j].id;
        result = ProcShapeOffset(client);
        if (result != Success)
            break;
    }
    return result;
}
#endif

static int
ProcShapeQueryExtents(ClientPtr client)
{
    REQUEST(xShapeQueryExtentsReq);
    WindowPtr pWin;
    xShapeQueryExtentsReply rep;
    BoxRec extents, *pExtents;
    int rc;
    RegionPtr region;

    REQUEST_SIZE_MATCH(xShapeQueryExtentsReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rep = (xShapeQueryExtentsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .boundingShaped = (wBoundingShape(pWin) != 0),
        .clipShaped = (wClipShape(pWin) != 0)
    };
    if ((region = wBoundingShape(pWin))) {
        /* this is done in two steps because of a compiler bug on SunOS 4.1.3 */
        pExtents = RegionExtents(region);
        extents = *pExtents;
    }
    else {
        extents.x1 = -wBorderWidth(pWin);
        extents.y1 = -wBorderWidth(pWin);
        extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
        extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
    }
    rep.xBoundingShape = extents.x1;
    rep.yBoundingShape = extents.y1;
    rep.widthBoundingShape = extents.x2 - extents.x1;
    rep.heightBoundingShape = extents.y2 - extents.y1;
    if ((region = wClipShape(pWin))) {
        /* this is done in two steps because of a compiler bug on SunOS 4.1.3 */
        pExtents = RegionExtents(region);
        extents = *pExtents;
    }
    else {
        extents.x1 = 0;
        extents.y1 = 0;
        extents.x2 = pWin->drawable.width;
        extents.y2 = pWin->drawable.height;
    }
    rep.xClipShape = extents.x1;
    rep.yClipShape = extents.y1;
    rep.widthClipShape = extents.x2 - extents.x1;
    rep.heightClipShape = extents.y2 - extents.y1;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.xBoundingShape);
        swaps(&rep.yBoundingShape);
        swaps(&rep.widthBoundingShape);
        swaps(&rep.heightBoundingShape);
        swaps(&rep.xClipShape);
        swaps(&rep.yClipShape);
        swaps(&rep.widthClipShape);
        swaps(&rep.heightClipShape);
    }
    WriteToClient(client, sizeof(xShapeQueryExtentsReply), &rep);
    return Success;
}

 /*ARGSUSED*/ static int
ShapeFreeClient(void *data, XID id)
{
    ShapeEventPtr pShapeEvent;
    WindowPtr pWin;
    ShapeEventPtr *pHead, pCur, pPrev;
    int rc;

    pShapeEvent = (ShapeEventPtr) data;
    pWin = pShapeEvent->window;
    rc = dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                                 ShapeEventType, serverClient, DixReadAccess);
    if (rc == Success) {
        pPrev = 0;
        for (pCur = *pHead; pCur && pCur != pShapeEvent; pCur = pCur->next)
            pPrev = pCur;
        if (pCur) {
            if (pPrev)
                pPrev->next = pShapeEvent->next;
            else
                *pHead = pShapeEvent->next;
        }
    }
    free((void *) pShapeEvent);
    return 1;
}

 /*ARGSUSED*/ static int
ShapeFreeEvents(void *data, XID id)
{
    ShapeEventPtr *pHead, pCur, pNext;

    pHead = (ShapeEventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
        pNext = pCur->next;
        FreeResource(pCur->clientResource, ClientType);
        free((void *) pCur);
    }
    free((void *) pHead);
    return 1;
}

static int
ProcShapeSelectInput(ClientPtr client)
{
    REQUEST(xShapeSelectInputReq);
    WindowPtr pWin;
    ShapeEventPtr pShapeEvent, pNewShapeEvent, *pHead;
    XID clientResource;
    int rc;

    REQUEST_SIZE_MATCH(xShapeSelectInputReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixReceiveAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                                 ShapeEventType, client, DixWriteAccess);
    if (rc != Success && rc != BadValue)
        return rc;

    switch (stuff->enable) {
    case xTrue:
        if (pHead) {

            /* check for existing entry. */
            for (pShapeEvent = *pHead;
                 pShapeEvent; pShapeEvent = pShapeEvent->next) {
                if (pShapeEvent->client == client)
                    return Success;
            }
        }

        /* build the entry */
        pNewShapeEvent = malloc(sizeof(ShapeEventRec));
        if (!pNewShapeEvent)
            return BadAlloc;
        pNewShapeEvent->next = 0;
        pNewShapeEvent->client = client;
        pNewShapeEvent->window = pWin;
        /*
         * add a resource that will be deleted when
         * the client goes away
         */
        clientResource = FakeClientID(client->index);
        pNewShapeEvent->clientResource = clientResource;
        if (!AddResource(clientResource, ClientType, (void *) pNewShapeEvent))
            return BadAlloc;
        /*
         * create a resource to contain a void *to the list
         * of clients selecting input.  This must be indirect as
         * the list may be arbitrarily rearranged which cannot be
         * done through the resource database.
         */
        if (!pHead) {
            pHead = malloc(sizeof(ShapeEventPtr));
            if (!pHead ||
                !AddResource(pWin->drawable.id, ShapeEventType,
                             (void *) pHead)) {
                FreeResource(clientResource, RT_NONE);
                return BadAlloc;
            }
            *pHead = 0;
        }
        pNewShapeEvent->next = *pHead;
        *pHead = pNewShapeEvent;
        break;
    case xFalse:
        /* delete the interest */
        if (pHead) {
            pNewShapeEvent = 0;
            for (pShapeEvent = *pHead; pShapeEvent;
                 pShapeEvent = pShapeEvent->next) {
                if (pShapeEvent->client == client)
                    break;
                pNewShapeEvent = pShapeEvent;
            }
            if (pShapeEvent) {
                FreeResource(pShapeEvent->clientResource, ClientType);
                if (pNewShapeEvent)
                    pNewShapeEvent->next = pShapeEvent->next;
                else
                    *pHead = pShapeEvent->next;
                free(pShapeEvent);
            }
        }
        break;
    default:
        client->errorValue = stuff->enable;
        return BadValue;
    }
    return Success;
}

/*
 * deliver the event
 */

void
SendShapeNotify(WindowPtr pWin, int which)
{
    ShapeEventPtr *pHead, pShapeEvent;
    BoxRec extents;
    RegionPtr region;
    BYTE shaped;
    int rc;

    rc = dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                                 ShapeEventType, serverClient, DixReadAccess);
    if (rc != Success)
        return;
    switch (which) {
    case ShapeBounding:
        region = wBoundingShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = -wBorderWidth(pWin);
            extents.y1 = -wBorderWidth(pWin);
            extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
            extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
            shaped = xFalse;
        }
        break;
    case ShapeClip:
        region = wClipShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = 0;
            extents.y1 = 0;
            extents.x2 = pWin->drawable.width;
            extents.y2 = pWin->drawable.height;
            shaped = xFalse;
        }
        break;
    case ShapeInput:
        region = wInputShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = -wBorderWidth(pWin);
            extents.y1 = -wBorderWidth(pWin);
            extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
            extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
            shaped = xFalse;
        }
        break;
    default:
        return;
    }
    UpdateCurrentTimeIf();
    for (pShapeEvent = *pHead; pShapeEvent; pShapeEvent = pShapeEvent->next) {
        xShapeNotifyEvent se = {
            .type = ShapeNotify + ShapeEventBase,
            .kind = which,
            .window = pWin->drawable.id,
            .x = extents.x1,
            .y = extents.y1,
            .width = extents.x2 - extents.x1,
            .height = extents.y2 - extents.y1,
            .time = currentTime.milliseconds,
            .shaped = shaped
        };
        WriteEventsToClient(pShapeEvent->client, 1, (xEvent *) &se);
    }
}

static int
ProcShapeInputSelected(ClientPtr client)
{
    REQUEST(xShapeInputSelectedReq);
    WindowPtr pWin;
    ShapeEventPtr pShapeEvent, *pHead;
    int enabled, rc;
    xShapeInputSelectedReply rep;

    REQUEST_SIZE_MATCH(xShapeInputSelectedReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                                 ShapeEventType, client, DixReadAccess);
    if (rc != Success && rc != BadValue)
        return rc;
    enabled = xFalse;
    if (pHead) {
        for (pShapeEvent = *pHead; pShapeEvent; pShapeEvent = pShapeEvent->next) {
            if (pShapeEvent->client == client) {
                enabled = xTrue;
                break;
            }
        }
    }
    rep = (xShapeInputSelectedReply) {
        .type = X_Reply,
        .enabled = enabled,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }
    WriteToClient(client, sizeof(xShapeInputSelectedReply), &rep);
    return Success;
}

static int
ProcShapeGetRectangles(ClientPtr client)
{
    REQUEST(xShapeGetRectanglesReq);
    WindowPtr pWin;
    xShapeGetRectanglesReply rep;
    xRectangle *rects;
    int nrects, i, rc;
    RegionPtr region;

    REQUEST_SIZE_MATCH(xShapeGetRectanglesReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->kind) {
    case ShapeBounding:
        region = wBoundingShape(pWin);
        break;
    case ShapeClip:
        region = wClipShape(pWin);
        break;
    case ShapeInput:
        region = wInputShape(pWin);
        break;
    default:
        client->errorValue = stuff->kind;
        return BadValue;
    }
    if (!region) {
        nrects = 1;
        rects = malloc(sizeof(xRectangle));
        if (!rects)
            return BadAlloc;
        switch (stuff->kind) {
        case ShapeBounding:
            rects->x = -(int) wBorderWidth(pWin);
            rects->y = -(int) wBorderWidth(pWin);
            rects->width = pWin->drawable.width + wBorderWidth(pWin);
            rects->height = pWin->drawable.height + wBorderWidth(pWin);
            break;
        case ShapeClip:
            rects->x = 0;
            rects->y = 0;
            rects->width = pWin->drawable.width;
            rects->height = pWin->drawable.height;
            break;
        case ShapeInput:
            rects->x = -(int) wBorderWidth(pWin);
            rects->y = -(int) wBorderWidth(pWin);
            rects->width = pWin->drawable.width + wBorderWidth(pWin);
            rects->height = pWin->drawable.height + wBorderWidth(pWin);
            break;
        }
    }
    else {
        BoxPtr box;

        nrects = RegionNumRects(region);
        box = RegionRects(region);
        rects = xallocarray(nrects, sizeof(xRectangle));
        if (!rects && nrects)
            return BadAlloc;
        for (i = 0; i < nrects; i++, box++) {
            rects[i].x = box->x1;
            rects[i].y = box->y1;
            rects[i].width = box->x2 - box->x1;
            rects[i].height = box->y2 - box->y1;
        }
    }
    rep = (xShapeGetRectanglesReply) {
        .type = X_Reply,
        .ordering = YXBanded,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(nrects * sizeof(xRectangle)),
        .nrects = nrects
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.nrects);
        SwapShorts((short *) rects, (unsigned long) nrects * 4);
    }
    WriteToClient(client, sizeof(rep), &rep);
    WriteToClient(client, nrects * sizeof(xRectangle), rects);
    free(rects);
    return Success;
}

static int
ProcShapeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_ShapeQueryVersion:
        return ProcShapeQueryVersion(client);
    case X_ShapeRectangles:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShapeRectangles(client);
        else
#endif
            return ProcShapeRectangles(client);
    case X_ShapeMask:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShapeMask(client);
        else
#endif
            return ProcShapeMask(client);
    case X_ShapeCombine:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShapeCombine(client);
        else
#endif
            return ProcShapeCombine(client);
    case X_ShapeOffset:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShapeOffset(client);
        else
#endif
            return ProcShapeOffset(client);
    case X_ShapeQueryExtents:
        return ProcShapeQueryExtents(client);
    case X_ShapeSelectInput:
        return ProcShapeSelectInput(client);
    case X_ShapeInputSelected:
        return ProcShapeInputSelected(client);
    case X_ShapeGetRectangles:
        return ProcShapeGetRectangles(client);
    default:
        return BadRequest;
    }
}

static void _X_COLD
SShapeNotifyEvent(xShapeNotifyEvent * from, xShapeNotifyEvent * to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswapl(from->window, to->window);
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswaps(from->x, to->x);
    cpswaps(from->y, to->y);
    cpswaps(from->width, to->width);
    cpswaps(from->height, to->height);
    cpswapl(from->time, to->time);
    to->shaped = from->shaped;
}

static int _X_COLD
SProcShapeQueryVersion(ClientPtr client)
{
    REQUEST(xShapeQueryVersionReq);

    swaps(&stuff->length);
    return ProcShapeQueryVersion(client);
}

static int _X_COLD
SProcShapeRectangles(ClientPtr client)
{
    REQUEST(xShapeRectanglesReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xShapeRectanglesReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    SwapRestS(stuff);
    return ProcShapeRectangles(client);
}

static int _X_COLD
SProcShapeMask(ClientPtr client)
{
    REQUEST(xShapeMaskReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeMaskReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    swapl(&stuff->src);
    return ProcShapeMask(client);
}

static int _X_COLD
SProcShapeCombine(ClientPtr client)
{
    REQUEST(xShapeCombineReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeCombineReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    swapl(&stuff->src);
    return ProcShapeCombine(client);
}

static int _X_COLD
SProcShapeOffset(ClientPtr client)
{
    REQUEST(xShapeOffsetReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeOffsetReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    return ProcShapeOffset(client);
}

static int _X_COLD
SProcShapeQueryExtents(ClientPtr client)
{
    REQUEST(xShapeQueryExtentsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeQueryExtentsReq);
    swapl(&stuff->window);
    return ProcShapeQueryExtents(client);
}

static int _X_COLD
SProcShapeSelectInput(ClientPtr client)
{
    REQUEST(xShapeSelectInputReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeSelectInputReq);
    swapl(&stuff->window);
    return ProcShapeSelectInput(client);
}

static int _X_COLD
SProcShapeInputSelected(ClientPtr client)
{
    REQUEST(xShapeInputSelectedReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeInputSelectedReq);
    swapl(&stuff->window);
    return ProcShapeInputSelected(client);
}

static int _X_COLD
SProcShapeGetRectangles(ClientPtr client)
{
    REQUEST(xShapeGetRectanglesReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShapeGetRectanglesReq);
    swapl(&stuff->window);
    return ProcShapeGetRectangles(client);
}

static int _X_COLD
SProcShapeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_ShapeQueryVersion:
        return SProcShapeQueryVersion(client);
    case X_ShapeRectangles:
        return SProcShapeRectangles(client);
    case X_ShapeMask:
        return SProcShapeMask(client);
    case X_ShapeCombine:
        return SProcShapeCombine(client);
    case X_ShapeOffset:
        return SProcShapeOffset(client);
    case X_ShapeQueryExtents:
        return SProcShapeQueryExtents(client);
    case X_ShapeSelectInput:
        return SProcShapeSelectInput(client);
    case X_ShapeInputSelected:
        return SProcShapeInputSelected(client);
    case X_ShapeGetRectangles:
        return SProcShapeGetRectangles(client);
    default:
        return BadRequest;
    }
}

void
ShapeExtensionInit(void)
{
    ExtensionEntry *extEntry;

    ClientType = CreateNewResourceType(ShapeFreeClient, "ShapeClient");
    ShapeEventType = CreateNewResourceType(ShapeFreeEvents, "ShapeEvent");
    if (ClientType && ShapeEventType &&
        (extEntry = AddExtension(SHAPENAME, ShapeNumberEvents, 0,
                                 ProcShapeDispatch, SProcShapeDispatch,
                                 NULL, StandardMinorOpcode))) {
        ShapeEventBase = extEntry->eventBase;
        EventSwapVector[ShapeEventBase] = (EventSwapPtr) SShapeNotifyEvent;
    }
}
