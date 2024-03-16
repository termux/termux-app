/*
 *
Copyright (c) 1992  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

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
#include <X11/extensions/saverproto.h>
#include "gcstruct.h"
#include "cursorstr.h"
#include "colormapst.h"
#include "xace.h"
#include "inputstr.h"
#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif
#include "protocol-versions.h"

#include <stdio.h>

#include "extinit.h"

static int ScreenSaverEventBase = 0;

static Bool ScreenSaverHandle(ScreenPtr /* pScreen */ ,
                              int /* xstate */ ,
                              Bool      /* force */
    );

static Bool
 CreateSaverWindow(ScreenPtr    /* pScreen */
    );

static Bool
 DestroySaverWindow(ScreenPtr   /* pScreen */
    );

static void
 UninstallSaverColormap(ScreenPtr       /* pScreen */
    );

static void
 CheckScreenPrivate(ScreenPtr   /* pScreen */
    );

static void SScreenSaverNotifyEvent(xScreenSaverNotifyEvent * /* from */ ,
                                    xScreenSaverNotifyEvent *   /* to */
    );

static RESTYPE SuspendType;     /* resource type for suspension records */

typedef struct _ScreenSaverSuspension *ScreenSaverSuspensionPtr;

/* List of clients that are suspending the screensaver. */
static ScreenSaverSuspensionPtr suspendingClients = NULL;

/*
 * clientResource is a resource ID that's added when the record is
 * allocated, so the record is freed and the screensaver resumed when
 * the client disconnects. count is the number of times the client has
 * requested the screensaver be suspended.
 */
typedef struct _ScreenSaverSuspension {
    ScreenSaverSuspensionPtr next;
    ClientPtr pClient;
    XID clientResource;
    int count;
} ScreenSaverSuspensionRec;

static int ScreenSaverFreeSuspend(void *value, XID id);

/*
 * each screen has a list of clients requesting
 * ScreenSaverNotify events.  Each client has a resource
 * for each screen it selects ScreenSaverNotify input for,
 * this resource is used to delete the ScreenSaverNotifyRec
 * entry from the per-screen queue.
 */

static RESTYPE SaverEventType;  /* resource type for event masks */

typedef struct _ScreenSaverEvent *ScreenSaverEventPtr;

typedef struct _ScreenSaverEvent {
    ScreenSaverEventPtr next;
    ClientPtr client;
    ScreenPtr screen;
    XID resource;
    CARD32 mask;
} ScreenSaverEventRec;

static int ScreenSaverFreeEvents(void * value, XID id);

static Bool setEventMask(ScreenPtr      pScreen,
                         ClientPtr      client,
                         unsigned long  mask);

static unsigned long getEventMask(ScreenPtr     pScreen,
                                  ClientPtr     client);

/*
 * when a client sets the screen saver attributes, a resource is
 * kept to be freed when the client exits
 */

static RESTYPE AttrType;        /* resource type for attributes */

typedef struct _ScreenSaverAttr {
    ScreenPtr screen;
    ClientPtr client;
    XID resource;
    short x, y;
    unsigned short width, height, borderWidth;
    unsigned char class;
    unsigned char depth;
    VisualID visual;
    CursorPtr pCursor;
    PixmapPtr pBackgroundPixmap;
    PixmapPtr pBorderPixmap;
    Colormap colormap;
    unsigned long mask;         /* no pixmaps or cursors */
    unsigned long *values;
} ScreenSaverAttrRec, *ScreenSaverAttrPtr;

static int ScreenSaverFreeAttr(void *value, XID id);

static void FreeAttrs(ScreenSaverAttrPtr pAttr);

static void FreeScreenAttr(ScreenSaverAttrPtr pAttr);

static void
SendScreenSaverNotify(ScreenPtr pScreen,
                      int       state,
                      Bool      forced);

typedef struct _ScreenSaverScreenPrivate {
    ScreenSaverEventPtr events;
    ScreenSaverAttrPtr attr;
    Bool hasWindow;
    Colormap installedMap;
} ScreenSaverScreenPrivateRec, *ScreenSaverScreenPrivatePtr;

static ScreenSaverScreenPrivatePtr MakeScreenPrivate(ScreenPtr pScreen);

static DevPrivateKeyRec ScreenPrivateKeyRec;

#define ScreenPrivateKey (&ScreenPrivateKeyRec)

#define GetScreenPrivate(s) ((ScreenSaverScreenPrivatePtr) \
    dixLookupPrivate(&(s)->devPrivates, ScreenPrivateKey))
#define SetScreenPrivate(s,v) \
    dixSetPrivate(&(s)->devPrivates, ScreenPrivateKey, v);
#define SetupScreen(s)	ScreenSaverScreenPrivatePtr pPriv = (s ? GetScreenPrivate(s) : NULL)

#define New(t)	(malloc(sizeof (t)))

static void
CheckScreenPrivate(ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    if (!pPriv)
        return;
    if (!pPriv->attr && !pPriv->events &&
        !pPriv->hasWindow && pPriv->installedMap == None) {
        free(pPriv);
        SetScreenPrivate(pScreen, NULL);
        pScreen->screensaver.ExternalScreenSaver = NULL;
    }
}

static ScreenSaverScreenPrivatePtr
MakeScreenPrivate(ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    if (pPriv)
        return pPriv;
    pPriv = New(ScreenSaverScreenPrivateRec);
    if (!pPriv)
        return 0;
    pPriv->events = 0;
    pPriv->attr = 0;
    pPriv->hasWindow = FALSE;
    pPriv->installedMap = None;
    SetScreenPrivate(pScreen, pPriv);
    pScreen->screensaver.ExternalScreenSaver = ScreenSaverHandle;
    return pPriv;
}

static unsigned long
getEventMask(ScreenPtr pScreen, ClientPtr client)
{
    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv;

    if (!pPriv)
        return 0;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
        if (pEv->client == client)
            return pEv->mask;
    return 0;
}

static Bool
setEventMask(ScreenPtr pScreen, ClientPtr client, unsigned long mask)
{
    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv, *pPrev;

    if (getEventMask(pScreen, client) == mask)
        return TRUE;
    if (!pPriv) {
        pPriv = MakeScreenPrivate(pScreen);
        if (!pPriv)
            return FALSE;
    }
    for (pPrev = &pPriv->events; (pEv = *pPrev) != 0; pPrev = &pEv->next)
        if (pEv->client == client)
            break;
    if (mask == 0) {
        FreeResource(pEv->resource, SaverEventType);
        *pPrev = pEv->next;
        free(pEv);
        CheckScreenPrivate(pScreen);
    }
    else {
        if (!pEv) {
            pEv = New(ScreenSaverEventRec);
            if (!pEv) {
                CheckScreenPrivate(pScreen);
                return FALSE;
            }
            *pPrev = pEv;
            pEv->next = NULL;
            pEv->client = client;
            pEv->screen = pScreen;
            pEv->resource = FakeClientID(client->index);
            if (!AddResource(pEv->resource, SaverEventType, (void *) pEv))
                return FALSE;
        }
        pEv->mask = mask;
    }
    return TRUE;
}

static void
FreeAttrs(ScreenSaverAttrPtr pAttr)
{
    PixmapPtr pPixmap;
    CursorPtr pCursor;

    if ((pPixmap = pAttr->pBackgroundPixmap) != 0)
        (*pPixmap->drawable.pScreen->DestroyPixmap) (pPixmap);
    if ((pPixmap = pAttr->pBorderPixmap) != 0)
        (*pPixmap->drawable.pScreen->DestroyPixmap) (pPixmap);
    if ((pCursor = pAttr->pCursor) != 0)
        FreeCursor(pCursor, (Cursor) 0);
}

static void
FreeScreenAttr(ScreenSaverAttrPtr pAttr)
{
    FreeAttrs(pAttr);
    free(pAttr->values);
    free(pAttr);
}

static int
ScreenSaverFreeEvents(void *value, XID id)
{
    ScreenSaverEventPtr pOld = (ScreenSaverEventPtr) value;
    ScreenPtr pScreen = pOld->screen;

    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv, *pPrev;

    if (!pPriv)
        return TRUE;
    for (pPrev = &pPriv->events; (pEv = *pPrev) != 0; pPrev = &pEv->next)
        if (pEv == pOld)
            break;
    if (!pEv)
        return TRUE;
    *pPrev = pEv->next;
    free(pEv);
    CheckScreenPrivate(pScreen);
    return TRUE;
}

static int
ScreenSaverFreeAttr(void *value, XID id)
{
    ScreenSaverAttrPtr pOldAttr = (ScreenSaverAttrPtr) value;
    ScreenPtr pScreen = pOldAttr->screen;

    SetupScreen(pScreen);

    if (!pPriv)
        return TRUE;
    if (pPriv->attr != pOldAttr)
        return TRUE;
    FreeScreenAttr(pOldAttr);
    pPriv->attr = NULL;
    if (pPriv->hasWindow) {
        dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverReset);
        dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverActive);
    }
    CheckScreenPrivate(pScreen);
    return TRUE;
}

static int
ScreenSaverFreeSuspend(void *value, XID id)
{
    ScreenSaverSuspensionPtr data = (ScreenSaverSuspensionPtr) value;
    ScreenSaverSuspensionPtr *prev, this;

    /* Unlink and free the suspension record for the client */
    for (prev = &suspendingClients; (this = *prev); prev = &this->next) {
        if (this == data) {
            *prev = this->next;
            free(this);
            break;
        }
    }

    /* Re-enable the screensaver if this was the last client suspending it. */
    if (screenSaverSuspended && suspendingClients == NULL) {
        screenSaverSuspended = FALSE;

        /* The screensaver could be active, since suspending it (by design)
           doesn't prevent it from being forceably activated */
#ifdef DPMSExtension
        if (screenIsSaved != SCREEN_SAVER_ON && DPMSPowerLevel == DPMSModeOn)
#else
        if (screenIsSaved != SCREEN_SAVER_ON)
#endif
        {
            DeviceIntPtr dev;
            UpdateCurrentTimeIf();
            nt_list_for_each_entry(dev, inputInfo.devices, next)
                NoticeTime(dev, currentTime);
            SetScreenSaverTimer();
        }
    }

    return Success;
}

static void
SendScreenSaverNotify(ScreenPtr pScreen, int state, Bool forced)
{
    ScreenSaverScreenPrivatePtr pPriv;
    ScreenSaverEventPtr pEv;
    unsigned long mask;
    int kind;

    UpdateCurrentTimeIf();
    mask = ScreenSaverNotifyMask;
    if (state == ScreenSaverCycle)
        mask = ScreenSaverCycleMask;
    pScreen = screenInfo.screens[pScreen->myNum];
    pPriv = GetScreenPrivate(pScreen);
    if (!pPriv)
        return;
    if (pPriv->attr)
        kind = ScreenSaverExternal;
    else if (ScreenSaverBlanking != DontPreferBlanking)
        kind = ScreenSaverBlanked;
    else
        kind = ScreenSaverInternal;
    for (pEv = pPriv->events; pEv; pEv = pEv->next) {
        if (pEv->mask & mask) {
            xScreenSaverNotifyEvent ev = {
                .type = ScreenSaverNotify + ScreenSaverEventBase,
                .state = state,
                .timestamp = currentTime.milliseconds,
                .root = pScreen->root->drawable.id,
                .window = pScreen->screensaver.wid,
                .kind = kind,
                .forced = forced
            };
            WriteEventsToClient(pEv->client, 1, (xEvent *) &ev);
        }
    }
}

static void _X_COLD
SScreenSaverNotifyEvent(xScreenSaverNotifyEvent * from,
                        xScreenSaverNotifyEvent * to)
{
    to->type = from->type;
    to->state = from->state;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    to->kind = from->kind;
    to->forced = from->forced;
}

static void
UninstallSaverColormap(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ColormapPtr pCmap;
    int rc;

    if (pPriv && pPriv->installedMap != None) {
        rc = dixLookupResourceByType((void **) &pCmap, pPriv->installedMap,
                                     RT_COLORMAP, serverClient,
                                     DixUninstallAccess);
        if (rc == Success)
            (*pCmap->pScreen->UninstallColormap) (pCmap);
        pPriv->installedMap = None;
        CheckScreenPrivate(pScreen);
    }
}

static Bool
CreateSaverWindow(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ScreenSaverStuffPtr pSaver;
    ScreenSaverAttrPtr pAttr;
    WindowPtr pWin;
    int result;
    unsigned long mask;
    Colormap wantMap;
    ColormapPtr pCmap;

    pSaver = &pScreen->screensaver;
    if (pSaver->pWindow) {
        pSaver->pWindow = NullWindow;
        FreeResource(pSaver->wid, RT_NONE);
        if (pPriv) {
            UninstallSaverColormap(pScreen);
            pPriv->hasWindow = FALSE;
            CheckScreenPrivate(pScreen);
        }
    }

    if (!pPriv || !(pAttr = pPriv->attr))
        return FALSE;

    pPriv->installedMap = None;

    if (GrabInProgress && GrabInProgress != pAttr->client->index)
        return FALSE;

    pWin = CreateWindow(pSaver->wid, pScreen->root,
                        pAttr->x, pAttr->y, pAttr->width, pAttr->height,
                        pAttr->borderWidth, pAttr->class,
                        pAttr->mask, (XID *) pAttr->values,
                        pAttr->depth, serverClient, pAttr->visual, &result);
    if (!pWin)
        return FALSE;

    if (!AddResource(pWin->drawable.id, RT_WINDOW, pWin))
        return FALSE;

    mask = 0;
    if (pAttr->pBackgroundPixmap) {
        pWin->backgroundState = BackgroundPixmap;
        pWin->background.pixmap = pAttr->pBackgroundPixmap;
        pAttr->pBackgroundPixmap->refcnt++;
        mask |= CWBackPixmap;
    }
    if (pAttr->pBorderPixmap) {
        pWin->borderIsPixel = FALSE;
        pWin->border.pixmap = pAttr->pBorderPixmap;
        pAttr->pBorderPixmap->refcnt++;
        mask |= CWBorderPixmap;
    }
    if (pAttr->pCursor) {
        CursorPtr cursor;
        if (!pWin->optional)
            if (!MakeWindowOptional(pWin)) {
                FreeResource(pWin->drawable.id, RT_NONE);
                return FALSE;
            }
        cursor = RefCursor(pAttr->pCursor);
        if (pWin->optional->cursor)
            FreeCursor(pWin->optional->cursor, (Cursor) 0);
        pWin->optional->cursor = cursor;
        pWin->cursorIsNone = FALSE;
        CheckWindowOptionalNeed(pWin);
        mask |= CWCursor;
    }
    if (mask)
        (*pScreen->ChangeWindowAttributes) (pWin, mask);

    if (pAttr->colormap != None)
        (void) ChangeWindowAttributes(pWin, CWColormap, &pAttr->colormap,
                                      serverClient);

    MapWindow(pWin, serverClient);

    pPriv->hasWindow = TRUE;
    pSaver->pWindow = pWin;

    /* check and install our own colormap if it isn't installed now */
    wantMap = wColormap(pWin);
    if (wantMap == None || IsMapInstalled(wantMap, pWin))
        return TRUE;

    result = dixLookupResourceByType((void **) &pCmap, wantMap, RT_COLORMAP,
                                     serverClient, DixInstallAccess);
    if (result != Success)
        return TRUE;

    pPriv->installedMap = wantMap;

    (*pCmap->pScreen->InstallColormap) (pCmap);

    return TRUE;
}

static Bool
DestroySaverWindow(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ScreenSaverStuffPtr pSaver;

    if (!pPriv || !pPriv->hasWindow)
        return FALSE;

    pSaver = &pScreen->screensaver;
    if (pSaver->pWindow) {
        pSaver->pWindow = NullWindow;
        FreeResource(pSaver->wid, RT_NONE);
    }
    pPriv->hasWindow = FALSE;
    CheckScreenPrivate(pScreen);
    UninstallSaverColormap(pScreen);
    return TRUE;
}

static Bool
ScreenSaverHandle(ScreenPtr pScreen, int xstate, Bool force)
{
    int state = 0;
    Bool ret = FALSE;
    ScreenSaverScreenPrivatePtr pPriv;

    switch (xstate) {
    case SCREEN_SAVER_ON:
        state = ScreenSaverOn;
        ret = CreateSaverWindow(pScreen);
        break;
    case SCREEN_SAVER_OFF:
        state = ScreenSaverOff;
        ret = DestroySaverWindow(pScreen);
        break;
    case SCREEN_SAVER_CYCLE:
        state = ScreenSaverCycle;
        pPriv = GetScreenPrivate(pScreen);
        if (pPriv && pPriv->hasWindow)
            ret = TRUE;

    }
#ifdef PANORAMIX
    if (noPanoramiXExtension || !pScreen->myNum)
#endif
        SendScreenSaverNotify(pScreen, state, force);
    return ret;
}

static int
ProcScreenSaverQueryVersion(ClientPtr client)
{
    xScreenSaverQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_SAVER_MAJOR_VERSION,
        .minorVersion = SERVER_SAVER_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xScreenSaverQueryVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }
    WriteToClient(client, sizeof(xScreenSaverQueryVersionReply), &rep);
    return Success;
}

static int
ProcScreenSaverQueryInfo(ClientPtr client)
{
    REQUEST(xScreenSaverQueryInfoReq);
    xScreenSaverQueryInfoReply rep;
    int rc;
    ScreenSaverStuffPtr pSaver;
    DrawablePtr pDraw;
    CARD32 lastInput;
    ScreenSaverScreenPrivatePtr pPriv;

    REQUEST_SIZE_MATCH(xScreenSaverQueryInfoReq);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rc = XaceHook(XACE_SCREENSAVER_ACCESS, client, pDraw->pScreen,
                  DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pSaver = &pDraw->pScreen->screensaver;
    pPriv = GetScreenPrivate(pDraw->pScreen);

    UpdateCurrentTime();
    lastInput = GetTimeInMillis() - LastEventTime(XIAllDevices).milliseconds;

    rep = (xScreenSaverQueryInfoReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .window = pSaver->wid
    };
    if (screenIsSaved != SCREEN_SAVER_OFF) {
        rep.state = ScreenSaverOn;
        if (ScreenSaverTime)
            rep.tilOrSince = lastInput - ScreenSaverTime;
        else
            rep.tilOrSince = 0;
    }
    else {
        if (ScreenSaverTime) {
            rep.state = ScreenSaverOff;
            if (ScreenSaverTime < lastInput)
                rep.tilOrSince = 0;
            else
                rep.tilOrSince = ScreenSaverTime - lastInput;
        }
        else {
            rep.state = ScreenSaverDisabled;
            rep.tilOrSince = 0;
        }
    }
    rep.idle = lastInput;
    rep.eventMask = getEventMask(pDraw->pScreen, client);
    if (pPriv && pPriv->attr)
        rep.kind = ScreenSaverExternal;
    else if (ScreenSaverBlanking != DontPreferBlanking)
        rep.kind = ScreenSaverBlanked;
    else
        rep.kind = ScreenSaverInternal;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.window);
        swapl(&rep.tilOrSince);
        swapl(&rep.idle);
        swapl(&rep.eventMask);
    }
    WriteToClient(client, sizeof(xScreenSaverQueryInfoReply), &rep);
    return Success;
}

static int
ProcScreenSaverSelectInput(ClientPtr client)
{
    REQUEST(xScreenSaverSelectInputReq);
    DrawablePtr pDraw;
    int rc;

    REQUEST_SIZE_MATCH(xScreenSaverSelectInputReq);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rc = XaceHook(XACE_SCREENSAVER_ACCESS, client, pDraw->pScreen,
                  DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (!setEventMask(pDraw->pScreen, client, stuff->eventMask))
        return BadAlloc;
    return Success;
}

static int
ScreenSaverSetAttributes(ClientPtr client)
{
    REQUEST(xScreenSaverSetAttributesReq);
    DrawablePtr pDraw;
    WindowPtr pParent;
    ScreenPtr pScreen;
    ScreenSaverScreenPrivatePtr pPriv = 0;
    ScreenSaverAttrPtr pAttr = 0;
    int ret, len, class, bw, depth;
    unsigned long visual;
    int idepth, ivisual;
    Bool fOK;
    DepthPtr pDepth;
    WindowOptPtr ancwopt;
    unsigned int *pVlist;
    unsigned long *values = 0;
    unsigned long tmask, imask;
    unsigned long val;
    Pixmap pixID;
    PixmapPtr pPixmap;
    Cursor cursorID;
    CursorPtr pCursor;
    Colormap cmap;
    ColormapPtr pCmap;

    REQUEST_AT_LEAST_SIZE(xScreenSaverSetAttributesReq);
    ret = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                            DixGetAttrAccess);
    if (ret != Success)
        return ret;
    pScreen = pDraw->pScreen;
    pParent = pScreen->root;

    ret = XaceHook(XACE_SCREENSAVER_ACCESS, client, pScreen, DixSetAttrAccess);
    if (ret != Success)
        return ret;

    len = stuff->length - bytes_to_int32(sizeof(xScreenSaverSetAttributesReq));
    if (Ones(stuff->mask) != len)
        return BadLength;
    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }
    switch (class = stuff->c_class) {
    case CopyFromParent:
    case InputOnly:
    case InputOutput:
        break;
    default:
        client->errorValue = class;
        return BadValue;
    }
    bw = stuff->borderWidth;
    depth = stuff->depth;
    visual = stuff->visualID;

    /* copied directly from CreateWindow */

    if (class == CopyFromParent)
        class = pParent->drawable.class;

    if ((class != InputOutput) && (class != InputOnly)) {
        client->errorValue = class;
        return BadValue;
    }

    if ((class != InputOnly) && (pParent->drawable.class == InputOnly))
        return BadMatch;

    if ((class == InputOnly) && ((bw != 0) || (depth != 0)))
        return BadMatch;

    if ((class == InputOutput) && (depth == 0))
        depth = pParent->drawable.depth;
    ancwopt = pParent->optional;
    if (!ancwopt)
        ancwopt = FindWindowWithOptional(pParent)->optional;
    if (visual == CopyFromParent)
        visual = ancwopt->visual;

    /* Find out if the depth and visual are acceptable for this Screen */
    if ((visual != ancwopt->visual) || (depth != pParent->drawable.depth)) {
        fOK = FALSE;
        for (idepth = 0; idepth < pScreen->numDepths; idepth++) {
            pDepth = (DepthPtr) &pScreen->allowedDepths[idepth];
            if ((depth == pDepth->depth) || (depth == 0)) {
                for (ivisual = 0; ivisual < pDepth->numVids; ivisual++) {
                    if (visual == pDepth->vids[ivisual]) {
                        fOK = TRUE;
                        break;
                    }
                }
            }
        }
        if (fOK == FALSE)
            return BadMatch;
    }

    if (((stuff->mask & (CWBorderPixmap | CWBorderPixel)) == 0) &&
        (class != InputOnly) && (depth != pParent->drawable.depth)) {
        return BadMatch;
    }

    if (((stuff->mask & CWColormap) == 0) &&
        (class != InputOnly) &&
        ((visual != ancwopt->visual) || (ancwopt->colormap == None))) {
        return BadMatch;
    }

    /* end of errors from CreateWindow */

    pPriv = GetScreenPrivate(pScreen);
    if (pPriv && pPriv->attr) {
        if (pPriv->attr->client != client)
            return BadAccess;
    }
    if (!pPriv) {
        pPriv = MakeScreenPrivate(pScreen);
        if (!pPriv)
            return FALSE;
    }
    pAttr = New(ScreenSaverAttrRec);
    if (!pAttr) {
        ret = BadAlloc;
        goto bail;
    }
    /* over allocate for override redirect */
    pAttr->values = values = xallocarray(len + 1, sizeof(unsigned long));
    if (!values) {
        ret = BadAlloc;
        goto bail;
    }
    pAttr->screen = pScreen;
    pAttr->client = client;
    pAttr->x = stuff->x;
    pAttr->y = stuff->y;
    pAttr->width = stuff->width;
    pAttr->height = stuff->height;
    pAttr->borderWidth = stuff->borderWidth;
    pAttr->class = stuff->c_class;
    pAttr->depth = depth;
    pAttr->visual = visual;
    pAttr->colormap = None;
    pAttr->pCursor = NullCursor;
    pAttr->pBackgroundPixmap = NullPixmap;
    pAttr->pBorderPixmap = NullPixmap;
    /*
     * go through the mask, checking the values,
     * looking up pixmaps and cursors and hold a reference
     * to them.
     */
    pAttr->mask = tmask = stuff->mask | CWOverrideRedirect;
    pVlist = (unsigned int *) (stuff + 1);
    while (tmask) {
        imask = lowbit(tmask);
        tmask &= ~imask;
        switch (imask) {
        case CWBackPixmap:
            pixID = (Pixmap) * pVlist;
            if (pixID == None) {
                *values++ = None;
            }
            else if (pixID == ParentRelative) {
                if (depth != pParent->drawable.depth) {
                    ret = BadMatch;
                    goto PatchUp;
                }
                *values++ = ParentRelative;
            }
            else {
                ret =
                    dixLookupResourceByType((void **) &pPixmap, pixID,
                                            RT_PIXMAP, client, DixReadAccess);
                if (ret == Success) {
                    if ((pPixmap->drawable.depth != depth) ||
                        (pPixmap->drawable.pScreen != pScreen)) {
                        ret = BadMatch;
                        goto PatchUp;
                    }
                    pAttr->pBackgroundPixmap = pPixmap;
                    pPixmap->refcnt++;
                    pAttr->mask &= ~CWBackPixmap;
                }
                else {
                    client->errorValue = pixID;
                    goto PatchUp;
                }
            }
            break;
        case CWBackPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBorderPixmap:
            pixID = (Pixmap) * pVlist;
            if (pixID == CopyFromParent) {
                if (depth != pParent->drawable.depth) {
                    ret = BadMatch;
                    goto PatchUp;
                }
                *values++ = CopyFromParent;
            }
            else {
                ret =
                    dixLookupResourceByType((void **) &pPixmap, pixID,
                                            RT_PIXMAP, client, DixReadAccess);
                if (ret == Success) {
                    if ((pPixmap->drawable.depth != depth) ||
                        (pPixmap->drawable.pScreen != pScreen)) {
                        ret = BadMatch;
                        goto PatchUp;
                    }
                    pAttr->pBorderPixmap = pPixmap;
                    pPixmap->refcnt++;
                    pAttr->mask &= ~CWBorderPixmap;
                }
                else {
                    client->errorValue = pixID;
                    goto PatchUp;
                }
            }
            break;
        case CWBorderPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBitGravity:
            val = (CARD8) *pVlist;
            if (val > StaticGravity) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWWinGravity:
            val = (CARD8) *pVlist;
            if (val > StaticGravity) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWBackingStore:
            val = (CARD8) *pVlist;
            if ((val != NotUseful) && (val != WhenMapped) && (val != Always)) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWBackingPlanes:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBackingPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWSaveUnder:
            val = (BOOL) * pVlist;
            if ((val != xTrue) && (val != xFalse)) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWEventMask:
            *values++ = (CARD32) *pVlist;
            break;
        case CWDontPropagate:
            *values++ = (CARD32) *pVlist;
            break;
        case CWOverrideRedirect:
            if (!(stuff->mask & CWOverrideRedirect))
                pVlist--;
            else {
                val = (BOOL) * pVlist;
                if ((val != xTrue) && (val != xFalse)) {
                    ret = BadValue;
                    client->errorValue = val;
                    goto PatchUp;
                }
            }
            *values++ = xTrue;
            break;
        case CWColormap:
            cmap = (Colormap) * pVlist;
            ret = dixLookupResourceByType((void **) &pCmap, cmap, RT_COLORMAP,
                                          client, DixUseAccess);
            if (ret != Success) {
                client->errorValue = cmap;
                goto PatchUp;
            }
            if (pCmap->pVisual->vid != visual || pCmap->pScreen != pScreen) {
                ret = BadMatch;
                goto PatchUp;
            }
            pAttr->colormap = cmap;
            pAttr->mask &= ~CWColormap;
            break;
        case CWCursor:
            cursorID = (Cursor) * pVlist;
            if (cursorID == None) {
                *values++ = None;
            }
            else {
                ret = dixLookupResourceByType((void **) &pCursor, cursorID,
                                              RT_CURSOR, client, DixUseAccess);
                if (ret != Success) {
                    client->errorValue = cursorID;
                    goto PatchUp;
                }
                pAttr->pCursor = RefCursor(pCursor);
                pAttr->mask &= ~CWCursor;
            }
            break;
        default:
            ret = BadValue;
            client->errorValue = stuff->mask;
            goto PatchUp;
        }
        pVlist++;
    }
    if (pPriv->attr)
        FreeResource(pPriv->attr->resource, AttrType);
    pPriv->attr = pAttr;
    pAttr->resource = FakeClientID(client->index);
    if (!AddResource(pAttr->resource, AttrType, (void *) pAttr))
        return BadAlloc;
    return Success;
 PatchUp:
    FreeAttrs(pAttr);
 bail:
    CheckScreenPrivate(pScreen);
    if (pAttr)
        free(pAttr->values);
    free(pAttr);
    return ret;
}

static int
ScreenSaverUnsetAttributes(ClientPtr client)
{
    REQUEST(xScreenSaverSetAttributesReq);
    DrawablePtr pDraw;
    ScreenSaverScreenPrivatePtr pPriv;
    int rc;

    REQUEST_SIZE_MATCH(xScreenSaverUnsetAttributesReq);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;
    pPriv = GetScreenPrivate(pDraw->pScreen);
    if (pPriv && pPriv->attr && pPriv->attr->client == client) {
        FreeResource(pPriv->attr->resource, AttrType);
        FreeScreenAttr(pPriv->attr);
        pPriv->attr = NULL;
        CheckScreenPrivate(pDraw->pScreen);
    }
    return Success;
}

static int
ProcScreenSaverSetAttributes(ClientPtr client)
{
#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        REQUEST(xScreenSaverSetAttributesReq);
        PanoramiXRes *draw;
        PanoramiXRes *backPix = NULL;
        PanoramiXRes *bordPix = NULL;
        PanoramiXRes *cmap = NULL;
        int i, status, len;
        int pback_offset = 0, pbord_offset = 0, cmap_offset = 0;
        XID orig_visual, tmp;

        REQUEST_AT_LEAST_SIZE(xScreenSaverSetAttributesReq);

        status = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                          XRC_DRAWABLE, client, DixWriteAccess);
        if (status != Success)
            return (status == BadValue) ? BadDrawable : status;

        len =
            stuff->length -
            bytes_to_int32(sizeof(xScreenSaverSetAttributesReq));
        if (Ones(stuff->mask) != len)
            return BadLength;

        if ((Mask) stuff->mask & CWBackPixmap) {
            pback_offset = Ones((Mask) stuff->mask & (CWBackPixmap - 1));
            tmp = *((CARD32 *) &stuff[1] + pback_offset);
            if ((tmp != None) && (tmp != ParentRelative)) {
                status = dixLookupResourceByType((void **) &backPix, tmp,
                                                 XRT_PIXMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        if ((Mask) stuff->mask & CWBorderPixmap) {
            pbord_offset = Ones((Mask) stuff->mask & (CWBorderPixmap - 1));
            tmp = *((CARD32 *) &stuff[1] + pbord_offset);
            if (tmp != CopyFromParent) {
                status = dixLookupResourceByType((void **) &bordPix, tmp,
                                                 XRT_PIXMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        if ((Mask) stuff->mask & CWColormap) {
            cmap_offset = Ones((Mask) stuff->mask & (CWColormap - 1));
            tmp = *((CARD32 *) &stuff[1] + cmap_offset);
            if (tmp != CopyFromParent) {
                status = dixLookupResourceByType((void **) &cmap, tmp,
                                                 XRT_COLORMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        orig_visual = stuff->visualID;

        FOR_NSCREENS_BACKWARD(i) {
            stuff->drawable = draw->info[i].id;
            if (backPix)
                *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[i].id;
            if (bordPix)
                *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[i].id;
            if (cmap)
                *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[i].id;

            if (orig_visual != CopyFromParent)
                stuff->visualID = PanoramiXTranslateVisualID(i, orig_visual);

            status = ScreenSaverSetAttributes(client);
        }

        return status;
    }
#endif

    return ScreenSaverSetAttributes(client);
}

static int
ProcScreenSaverUnsetAttributes(ClientPtr client)
{
#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        REQUEST(xScreenSaverUnsetAttributesReq);
        PanoramiXRes *draw;
        int rc, i;

        REQUEST_SIZE_MATCH(xScreenSaverUnsetAttributesReq);

        rc = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
        if (rc != Success)
            return (rc == BadValue) ? BadDrawable : rc;

        for (i = PanoramiXNumScreens - 1; i > 0; i--) {
            stuff->drawable = draw->info[i].id;
            ScreenSaverUnsetAttributes(client);
        }

        stuff->drawable = draw->info[0].id;
    }
#endif

    return ScreenSaverUnsetAttributes(client);
}

static int
ProcScreenSaverSuspend(ClientPtr client)
{
    ScreenSaverSuspensionPtr *prev, this;
    BOOL suspend;

    REQUEST(xScreenSaverSuspendReq);
    REQUEST_SIZE_MATCH(xScreenSaverSuspendReq);

    /*
     * Old versions of XCB encode suspend as 1 byte followed by three
     * pad bytes (which are always cleared), instead of a 4 byte
     * value. Be compatible by just checking for a non-zero value in
     * all 32-bits.
     */
    suspend = stuff->suspend != 0;

    /* Check if this client is suspending the screensaver */
    for (prev = &suspendingClients; (this = *prev); prev = &this->next)
        if (this->pClient == client)
            break;

    if (this) {
        if (suspend == TRUE)
            this->count++;
        else if (--this->count == 0)
            FreeResource(this->clientResource, RT_NONE);

        return Success;
    }

    /* If we get to this point, this client isn't suspending the screensaver */
    if (suspend == FALSE)
        return Success;

    /*
     * Allocate a suspension record for the client, and stop the screensaver
     * if it isn't already suspended by another client. We attach a resource ID
     * to the record, so the screensaver will be re-enabled and the record freed
     * if the client disconnects without reenabling it first.
     */
    this = malloc(sizeof(ScreenSaverSuspensionRec));

    if (!this)
        return BadAlloc;

    this->next = NULL;
    this->pClient = client;
    this->count = 1;
    this->clientResource = FakeClientID(client->index);

    if (!AddResource(this->clientResource, SuspendType, (void *) this)) {
        free(this);
        return BadAlloc;
    }

    *prev = this;
    if (!screenSaverSuspended) {
        screenSaverSuspended = TRUE;
        FreeScreenSaverTimer();
    }

    return Success;
}

static int (*NormalVector[]) (ClientPtr /* client */ ) = {
ProcScreenSaverQueryVersion,
        ProcScreenSaverQueryInfo,
        ProcScreenSaverSelectInput,
        ProcScreenSaverSetAttributes,
        ProcScreenSaverUnsetAttributes, ProcScreenSaverSuspend,};

static int
ProcScreenSaverDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < ARRAY_SIZE(NormalVector))
        return (*NormalVector[stuff->data]) (client);
    return BadRequest;
}

static int _X_COLD
SProcScreenSaverQueryVersion(ClientPtr client)
{
    REQUEST(xScreenSaverQueryVersionReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xScreenSaverQueryVersionReq);
    return ProcScreenSaverQueryVersion(client);
}

static int _X_COLD
SProcScreenSaverQueryInfo(ClientPtr client)
{
    REQUEST(xScreenSaverQueryInfoReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xScreenSaverQueryInfoReq);
    swapl(&stuff->drawable);
    return ProcScreenSaverQueryInfo(client);
}

static int _X_COLD
SProcScreenSaverSelectInput(ClientPtr client)
{
    REQUEST(xScreenSaverSelectInputReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xScreenSaverSelectInputReq);
    swapl(&stuff->drawable);
    swapl(&stuff->eventMask);
    return ProcScreenSaverSelectInput(client);
}

static int _X_COLD
SProcScreenSaverSetAttributes(ClientPtr client)
{
    REQUEST(xScreenSaverSetAttributesReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xScreenSaverSetAttributesReq);
    swapl(&stuff->drawable);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swaps(&stuff->borderWidth);
    swapl(&stuff->visualID);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ProcScreenSaverSetAttributes(client);
}

static int _X_COLD
SProcScreenSaverUnsetAttributes(ClientPtr client)
{
    REQUEST(xScreenSaverUnsetAttributesReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xScreenSaverUnsetAttributesReq);
    swapl(&stuff->drawable);
    return ProcScreenSaverUnsetAttributes(client);
}

static int _X_COLD
SProcScreenSaverSuspend(ClientPtr client)
{
    REQUEST(xScreenSaverSuspendReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xScreenSaverSuspendReq);
    swapl(&stuff->suspend);
    return ProcScreenSaverSuspend(client);
}

static int (*SwappedVector[]) (ClientPtr /* client */ ) = {
SProcScreenSaverQueryVersion,
        SProcScreenSaverQueryInfo,
        SProcScreenSaverSelectInput,
        SProcScreenSaverSetAttributes,
        SProcScreenSaverUnsetAttributes, SProcScreenSaverSuspend,};

static int _X_COLD
SProcScreenSaverDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < ARRAY_SIZE(NormalVector))
        return (*SwappedVector[stuff->data]) (client);
    return BadRequest;
}

void
ScreenSaverExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int i;
    ScreenPtr pScreen;

    if (!dixRegisterPrivateKey(&ScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return;

    AttrType = CreateNewResourceType(ScreenSaverFreeAttr, "SaverAttr");
    SaverEventType = CreateNewResourceType(ScreenSaverFreeEvents, "SaverEvent");
    SuspendType = CreateNewResourceType(ScreenSaverFreeSuspend, "SaverSuspend");

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];
        SetScreenPrivate(pScreen, NULL);
    }
    if (AttrType && SaverEventType && SuspendType &&
        (extEntry = AddExtension(ScreenSaverName, ScreenSaverNumberEvents, 0,
                                 ProcScreenSaverDispatch,
                                 SProcScreenSaverDispatch, NULL,
                                 StandardMinorOpcode))) {
        ScreenSaverEventBase = extEntry->eventBase;
        EventSwapVector[ScreenSaverEventBase] =
            (EventSwapPtr) SScreenSaverNotifyEvent;
    }
}
