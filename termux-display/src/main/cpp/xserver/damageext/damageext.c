/*
 * Copyright © 2002 Keith Packard
 * Copyright 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "damageextint.h"
#include "damagestr.h"
#include "protocol-versions.h"
#include "extinit.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"

typedef struct {
    DamageExtPtr ext;
    DamagePtr damage[MAXSCREENS];
} PanoramiXDamageRes;

static RESTYPE XRT_DAMAGE;
static int (*PanoramiXSaveDamageCreate) (ClientPtr);

#endif

static unsigned char DamageReqCode;
static int DamageEventBase;
static RESTYPE DamageExtType;

static DevPrivateKeyRec DamageClientPrivateKeyRec;

#define DamageClientPrivateKey (&DamageClientPrivateKeyRec)

static void
DamageNoteCritical(ClientPtr pClient)
{
    DamageClientPtr pDamageClient = GetDamageClient(pClient);

    /* Composite extension marks clients with manual Subwindows as critical */
    if (pDamageClient->critical > 0) {
        SetCriticalOutputPending();
        pClient->smart_priority = SMART_MAX_PRIORITY;
    }
}

static void
damageGetGeometry(DrawablePtr draw, int *x, int *y, int *w, int *h)
{
#ifdef PANORAMIX
    if (!noPanoramiXExtension && draw->type == DRAWABLE_WINDOW) {
        WindowPtr win = (WindowPtr)draw;

        if (!win->parent) {
            *x = screenInfo.x;
            *y = screenInfo.y;
            *w = screenInfo.width;
            *h = screenInfo.height;
            return;
        }
    }
#endif

    *x = draw->x;
    *y = draw->y;
    *w = draw->width;
    *h = draw->height;
}

static void
DamageExtNotify(DamageExtPtr pDamageExt, BoxPtr pBoxes, int nBoxes)
{
    ClientPtr pClient = pDamageExt->pClient;
    DrawablePtr pDrawable = pDamageExt->pDrawable;
    xDamageNotifyEvent ev;
    int i, x, y, w, h;

    damageGetGeometry(pDrawable, &x, &y, &w, &h);

    UpdateCurrentTimeIf();
    ev = (xDamageNotifyEvent) {
        .type = DamageEventBase + XDamageNotify,
        .level = pDamageExt->level,
        .drawable = pDamageExt->drawable,
        .damage = pDamageExt->id,
        .timestamp = currentTime.milliseconds,
        .geometry.x = x,
        .geometry.y = y,
        .geometry.width = w,
        .geometry.height = h
    };
    if (pBoxes) {
        for (i = 0; i < nBoxes; i++) {
            ev.level = pDamageExt->level;
            if (i < nBoxes - 1)
                ev.level |= DamageNotifyMore;
            ev.area.x = pBoxes[i].x1;
            ev.area.y = pBoxes[i].y1;
            ev.area.width = pBoxes[i].x2 - pBoxes[i].x1;
            ev.area.height = pBoxes[i].y2 - pBoxes[i].y1;
            WriteEventsToClient(pClient, 1, (xEvent *) &ev);
        }
    }
    else {
        ev.area.x = 0;
        ev.area.y = 0;
        ev.area.width = w;
        ev.area.height = h;
        WriteEventsToClient(pClient, 1, (xEvent *) &ev);
    }

    DamageNoteCritical(pClient);
}

static void
DamageExtReport(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    DamageExtPtr pDamageExt = closure;

    switch (pDamageExt->level) {
    case DamageReportRawRegion:
    case DamageReportDeltaRegion:
        DamageExtNotify(pDamageExt, RegionRects(pRegion),
                        RegionNumRects(pRegion));
        break;
    case DamageReportBoundingBox:
        DamageExtNotify(pDamageExt, RegionExtents(pRegion), 1);
        break;
    case DamageReportNonEmpty:
        DamageExtNotify(pDamageExt, NullBox, 0);
        break;
    case DamageReportNone:
        break;
    }
}

static void
DamageExtDestroy(DamagePtr pDamage, void *closure)
{
    DamageExtPtr pDamageExt = closure;

    pDamageExt->pDamage = 0;
    if (pDamageExt->id)
        FreeResource(pDamageExt->id, RT_NONE);
}

void
DamageExtSetCritical(ClientPtr pClient, Bool critical)
{
    DamageClientPtr pDamageClient = GetDamageClient(pClient);

    if (pDamageClient)
        pDamageClient->critical += critical ? 1 : -1;
}

static int
ProcDamageQueryVersion(ClientPtr client)
{
    DamageClientPtr pDamageClient = GetDamageClient(client);
    xDamageQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    REQUEST(xDamageQueryVersionReq);

    REQUEST_SIZE_MATCH(xDamageQueryVersionReq);

    if (stuff->majorVersion < SERVER_DAMAGE_MAJOR_VERSION) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }
    else {
        rep.majorVersion = SERVER_DAMAGE_MAJOR_VERSION;
        if (stuff->majorVersion == SERVER_DAMAGE_MAJOR_VERSION &&
            stuff->minorVersion < SERVER_DAMAGE_MINOR_VERSION)
            rep.minorVersion = stuff->minorVersion;
        else
            rep.minorVersion = SERVER_DAMAGE_MINOR_VERSION;
    }
    pDamageClient->major_version = rep.majorVersion;
    pDamageClient->minor_version = rep.minorVersion;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xDamageQueryVersionReply), &rep);
    return Success;
}

static void
DamageExtRegister(DrawablePtr pDrawable, DamagePtr pDamage, Bool report)
{
    DamageSetReportAfterOp(pDamage, TRUE);
    DamageRegister(pDrawable, pDamage);

    if (report) {
        RegionPtr pRegion = &((WindowPtr) pDrawable)->borderClip;
        RegionTranslate(pRegion, -pDrawable->x, -pDrawable->y);
        DamageReportDamage(pDamage, pRegion);
        RegionTranslate(pRegion, pDrawable->x, pDrawable->y);
    }
}

static DamageExtPtr
DamageExtCreate(DrawablePtr pDrawable, DamageReportLevel level,
                ClientPtr client, XID id, XID drawable)
{
    DamageExtPtr pDamageExt = malloc(sizeof(DamageExtRec));
    if (!pDamageExt)
        return NULL;

    pDamageExt->id = id;
    pDamageExt->drawable = drawable;
    pDamageExt->pDrawable = pDrawable;
    pDamageExt->level = level;
    pDamageExt->pClient = client;
    pDamageExt->pDamage = DamageCreate(DamageExtReport, DamageExtDestroy, level,
                                       FALSE, pDrawable->pScreen, pDamageExt);
    if (!pDamageExt->pDamage) {
        free(pDamageExt);
        return NULL;
    }

    if (!AddResource(id, DamageExtType, (void *) pDamageExt))
        return NULL;

    DamageExtRegister(pDrawable, pDamageExt->pDamage,
                      pDrawable->type == DRAWABLE_WINDOW);

    return pDamageExt;
}

static DamageExtPtr
doDamageCreate(ClientPtr client, int *rc)
{
    DrawablePtr pDrawable;
    DamageExtPtr pDamageExt;
    DamageReportLevel level;

    REQUEST(xDamageCreateReq);

    *rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                            DixGetAttrAccess | DixReadAccess);
    if (*rc != Success)
        return NULL;

    switch (stuff->level) {
    case XDamageReportRawRectangles:
        level = DamageReportRawRegion;
        break;
    case XDamageReportDeltaRectangles:
        level = DamageReportDeltaRegion;
        break;
    case XDamageReportBoundingBox:
        level = DamageReportBoundingBox;
        break;
    case XDamageReportNonEmpty:
        level = DamageReportNonEmpty;
        break;
    default:
        client->errorValue = stuff->level;
        *rc = BadValue;
        return NULL;
    }

    pDamageExt = DamageExtCreate(pDrawable, level, client, stuff->damage,
                                 stuff->drawable);
    if (!pDamageExt)
        *rc = BadAlloc;

    return pDamageExt;
}

static int
ProcDamageCreate(ClientPtr client)
{
    int rc;
    REQUEST(xDamageCreateReq);
    REQUEST_SIZE_MATCH(xDamageCreateReq);
    LEGAL_NEW_RESOURCE(stuff->damage, client);
    doDamageCreate(client, &rc);
    return rc;
}

static int
ProcDamageDestroy(ClientPtr client)
{
    REQUEST(xDamageDestroyReq);
    DamageExtPtr pDamageExt;

    REQUEST_SIZE_MATCH(xDamageDestroyReq);
    VERIFY_DAMAGEEXT(pDamageExt, stuff->damage, client, DixWriteAccess);
    FreeResource(stuff->damage, RT_NONE);
    return Success;
}

#ifdef PANORAMIX
static RegionPtr
DamageExtSubtractWindowClip(DamageExtPtr pDamageExt)
{
    WindowPtr win = (WindowPtr)pDamageExt->pDrawable;
    PanoramiXRes *res = NULL;
    RegionPtr ret;
    int i;

    if (!win->parent)
        return &PanoramiXScreenRegion;

    dixLookupResourceByType((void **)&res, win->drawable.id, XRT_WINDOW,
                            serverClient, DixReadAccess);
    if (!res)
        return NULL;

    ret = RegionCreate(NULL, 0);
    if (!ret)
        return NULL;

    FOR_NSCREENS_FORWARD(i) {
        ScreenPtr screen;
        if (Success != dixLookupWindow(&win, res->info[i].id, serverClient,
                                       DixReadAccess))
            goto out;

        screen = win->drawable.pScreen;

        RegionTranslate(ret, -screen->x, -screen->y);
        if (!RegionUnion(ret, ret, &win->borderClip))
            goto out;
        RegionTranslate(ret, screen->x, screen->y);
    }

    return ret;

out:
    RegionDestroy(ret);
    return NULL;
}

static void
DamageExtFreeWindowClip(RegionPtr reg)
{
    if (reg != &PanoramiXScreenRegion)
        RegionDestroy(reg);
}
#endif

/*
 * DamageSubtract intersects with borderClip, so we must reconstruct the
 * protocol's perspective of same...
 */
static Bool
DamageExtSubtract(DamageExtPtr pDamageExt, const RegionPtr pRegion)
{
    DamagePtr pDamage = pDamageExt->pDamage;

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        RegionPtr damage = DamageRegion(pDamage);
        RegionSubtract(damage, damage, pRegion);

        if (pDamageExt->pDrawable->type == DRAWABLE_WINDOW) {
            DrawablePtr pDraw = pDamageExt->pDrawable;
            RegionPtr clip = DamageExtSubtractWindowClip(pDamageExt);
            if (clip) {
                RegionTranslate(clip, -pDraw->x, -pDraw->y);
                RegionIntersect(damage, damage, clip);
                RegionTranslate(clip, pDraw->x, pDraw->y);
                DamageExtFreeWindowClip(clip);
            }
        }

        return RegionNotEmpty(damage);
    }
#endif

    return DamageSubtract(pDamage, pRegion);
}

static int
ProcDamageSubtract(ClientPtr client)
{
    REQUEST(xDamageSubtractReq);
    DamageExtPtr pDamageExt;
    RegionPtr pRepair;
    RegionPtr pParts;

    REQUEST_SIZE_MATCH(xDamageSubtractReq);
    VERIFY_DAMAGEEXT(pDamageExt, stuff->damage, client, DixWriteAccess);
    VERIFY_REGION_OR_NONE(pRepair, stuff->repair, client, DixWriteAccess);
    VERIFY_REGION_OR_NONE(pParts, stuff->parts, client, DixWriteAccess);

    if (pDamageExt->level != DamageReportRawRegion) {
        DamagePtr pDamage = pDamageExt->pDamage;

        if (pRepair) {
            if (pParts)
                RegionIntersect(pParts, DamageRegion(pDamage), pRepair);
            if (DamageExtSubtract(pDamageExt, pRepair))
                DamageExtReport(pDamage, DamageRegion(pDamage),
                                (void *) pDamageExt);
        }
        else {
            if (pParts)
                RegionCopy(pParts, DamageRegion(pDamage));
            DamageEmpty(pDamage);
        }
    }

    return Success;
}

static int
ProcDamageAdd(ClientPtr client)
{
    REQUEST(xDamageAddReq);
    DrawablePtr pDrawable;
    RegionPtr pRegion;
    int rc;

    REQUEST_SIZE_MATCH(xDamageAddReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixWriteAccess);
    if (rc != Success)
        return rc;

    /* The region is relative to the drawable origin, so translate it out to
     * screen coordinates like damage expects.
     */
    RegionTranslate(pRegion, pDrawable->x, pDrawable->y);
    DamageDamageRegion(pDrawable, pRegion);
    RegionTranslate(pRegion, -pDrawable->x, -pDrawable->y);

    return Success;
}

/* Major version controls available requests */
static const int version_requests[] = {
    X_DamageQueryVersion,       /* before client sends QueryVersion */
    X_DamageAdd,                /* Version 1 */
};

static int (*ProcDamageVector[XDamageNumberRequests]) (ClientPtr) = {
    /*************** Version 1 ******************/
    ProcDamageQueryVersion,
    ProcDamageCreate,
    ProcDamageDestroy,
    ProcDamageSubtract,
    /*************** Version 1.1 ****************/
    ProcDamageAdd,
};

static int
ProcDamageDispatch(ClientPtr client)
{
    REQUEST(xDamageReq);
    DamageClientPtr pDamageClient = GetDamageClient(client);

    if (pDamageClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->damageReqType > version_requests[pDamageClient->major_version])
        return BadRequest;
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int _X_COLD
SProcDamageQueryVersion(ClientPtr client)
{
    REQUEST(xDamageQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDamageQueryVersionReq);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int _X_COLD
SProcDamageCreate(ClientPtr client)
{
    REQUEST(xDamageCreateReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDamageCreateReq);
    swapl(&stuff->damage);
    swapl(&stuff->drawable);
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int _X_COLD
SProcDamageDestroy(ClientPtr client)
{
    REQUEST(xDamageDestroyReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDamageDestroyReq);
    swapl(&stuff->damage);
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int _X_COLD
SProcDamageSubtract(ClientPtr client)
{
    REQUEST(xDamageSubtractReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDamageSubtractReq);
    swapl(&stuff->damage);
    swapl(&stuff->repair);
    swapl(&stuff->parts);
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int _X_COLD
SProcDamageAdd(ClientPtr client)
{
    REQUEST(xDamageAddReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDamageSubtractReq);
    swapl(&stuff->drawable);
    swapl(&stuff->region);
    return (*ProcDamageVector[stuff->damageReqType]) (client);
}

static int (*SProcDamageVector[XDamageNumberRequests]) (ClientPtr) = {
    /*************** Version 1 ******************/
    SProcDamageQueryVersion,
    SProcDamageCreate,
    SProcDamageDestroy,
    SProcDamageSubtract,
    /*************** Version 1.1 ****************/
    SProcDamageAdd,
};

static int _X_COLD
SProcDamageDispatch(ClientPtr client)
{
    REQUEST(xDamageReq);
    DamageClientPtr pDamageClient = GetDamageClient(client);

    if (pDamageClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->damageReqType > version_requests[pDamageClient->major_version])
        return BadRequest;
    return (*SProcDamageVector[stuff->damageReqType]) (client);
}

static int
FreeDamageExt(void *value, XID did)
{
    DamageExtPtr pDamageExt = (DamageExtPtr) value;

    /*
     * Get rid of the resource table entry hanging from the window id
     */
    pDamageExt->id = 0;
    if (pDamageExt->pDamage) {
        DamageDestroy(pDamageExt->pDamage);
    }
    free(pDamageExt);
    return Success;
}

static void _X_COLD
SDamageNotifyEvent(xDamageNotifyEvent * from, xDamageNotifyEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->drawable, to->drawable);
    cpswapl(from->damage, to->damage);
    cpswaps(from->area.x, to->area.x);
    cpswaps(from->area.y, to->area.y);
    cpswaps(from->area.width, to->area.width);
    cpswaps(from->area.height, to->area.height);
    cpswaps(from->geometry.x, to->geometry.x);
    cpswaps(from->geometry.y, to->geometry.y);
    cpswaps(from->geometry.width, to->geometry.width);
    cpswaps(from->geometry.height, to->geometry.height);
}

#ifdef PANORAMIX

static void
PanoramiXDamageReport(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    PanoramiXDamageRes *res = closure;
    DamageExtPtr pDamageExt = res->ext;
    WindowPtr pWin = (WindowPtr)pDamage->pDrawable;
    ScreenPtr pScreen = pDamage->pScreen;

    /* happens on unmap? sigh xinerama */
    if (RegionNil(pRegion))
        return;

    /* translate root windows if necessary */
    if (!pWin->parent)
        RegionTranslate(pRegion, pScreen->x, pScreen->y);

    /* add our damage to the protocol view */
    DamageReportDamage(pDamageExt->pDamage, pRegion);

    /* empty our view */
    DamageEmpty(pDamage);
}

static void
PanoramiXDamageExtDestroy(DamagePtr pDamage, void *closure)
{
    PanoramiXDamageRes *damage = closure;
    damage->damage[pDamage->pScreen->myNum] = NULL;
}

static int
PanoramiXDamageCreate(ClientPtr client)
{
    PanoramiXDamageRes *damage;
    PanoramiXRes *draw;
    int i, rc;

    REQUEST(xDamageCreateReq);

    REQUEST_SIZE_MATCH(xDamageCreateReq);
    LEGAL_NEW_RESOURCE(stuff->damage, client);
    rc = dixLookupResourceByClass((void **)&draw, stuff->drawable, XRC_DRAWABLE,
                                  client, DixGetAttrAccess | DixReadAccess);
    if (rc != Success)
        return rc;

    if (!(damage = calloc(1, sizeof(PanoramiXDamageRes))))
        return BadAlloc;

    if (!AddResource(stuff->damage, XRT_DAMAGE, damage))
        return BadAlloc;

    damage->ext = doDamageCreate(client, &rc);
    if (rc == Success && draw->type == XRT_WINDOW) {
        FOR_NSCREENS_FORWARD(i) {
            DrawablePtr pDrawable;
            DamagePtr pDamage = DamageCreate(PanoramiXDamageReport,
                                             PanoramiXDamageExtDestroy,
                                             DamageReportRawRegion,
                                             FALSE,
                                             screenInfo.screens[i],
                                             damage);
            if (!pDamage) {
                rc = BadAlloc;
            } else {
                damage->damage[i] = pDamage;
                rc = dixLookupDrawable(&pDrawable, draw->info[i].id, client,
                                       M_WINDOW,
                                       DixGetAttrAccess | DixReadAccess);
            }
            if (rc != Success)
                break;

            DamageExtRegister(pDrawable, pDamage, i != 0);
        }
    }

    if (rc != Success)
        FreeResource(stuff->damage, RT_NONE);

    return rc;
}

static int
PanoramiXDamageDelete(void *res, XID id)
{
    int i;
    PanoramiXDamageRes *damage = res;

    FOR_NSCREENS_BACKWARD(i) {
        if (damage->damage[i]) {
            DamageDestroy(damage->damage[i]);
            damage->damage[i] = NULL;
        }
    }

    free(damage);
    return 1;
}

void
PanoramiXDamageInit(void)
{
    XRT_DAMAGE = CreateNewResourceType(PanoramiXDamageDelete, "XineramaDamage");
    if (!XRT_DAMAGE)
        FatalError("Couldn't Xineramify Damage extension\n");

    PanoramiXSaveDamageCreate = ProcDamageVector[X_DamageCreate];
    ProcDamageVector[X_DamageCreate] = PanoramiXDamageCreate;
}

void
PanoramiXDamageReset(void)
{
    ProcDamageVector[X_DamageCreate] = PanoramiXSaveDamageCreate;
}

#endif /* PANORAMIX */

void
DamageExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int s;

    for (s = 0; s < screenInfo.numScreens; s++)
        DamageSetup(screenInfo.screens[s]);

    DamageExtType = CreateNewResourceType(FreeDamageExt, "DamageExt");
    if (!DamageExtType)
        return;

    if (!dixRegisterPrivateKey
        (&DamageClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(DamageClientRec)))
        return;

    if ((extEntry = AddExtension(DAMAGE_NAME, XDamageNumberEvents,
                                 XDamageNumberErrors,
                                 ProcDamageDispatch, SProcDamageDispatch,
                                 NULL, StandardMinorOpcode)) != 0) {
        DamageReqCode = (unsigned char) extEntry->base;
        DamageEventBase = extEntry->eventBase;
        EventSwapVector[DamageEventBase + XDamageNotify] =
            (EventSwapPtr) SDamageNotifyEvent;
        SetResourceTypeErrorValue(DamageExtType,
                                  extEntry->errorBase + BadDamage);
#ifdef PANORAMIX
        if (XRT_DAMAGE)
            SetResourceTypeErrorValue(XRT_DAMAGE,
                                      extEntry->errorBase + BadDamage);
#endif
    }
}
