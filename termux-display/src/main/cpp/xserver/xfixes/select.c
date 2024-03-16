/*
 * Copyright © 2002 Keith Packard
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

#include "xfixesint.h"
#include "xace.h"

static RESTYPE SelectionClientType, SelectionWindowType;
static Bool SelectionCallbackRegistered = FALSE;

/*
 * There is a global list of windows selecting for selection events
 * on every selection.  This should be plenty efficient for the
 * expected usage, if it does become a problem, it should be easily
 * replaced with a hash table of some kind keyed off the selection atom
 */

typedef struct _SelectionEvent *SelectionEventPtr;

typedef struct _SelectionEvent {
    SelectionEventPtr next;
    Atom selection;
    CARD32 eventMask;
    ClientPtr pClient;
    WindowPtr pWindow;
    XID clientResource;
} SelectionEventRec;

static SelectionEventPtr selectionEvents;

static void
XFixesSelectionCallback(CallbackListPtr *callbacks, void *data, void *args)
{
    SelectionEventPtr e;
    SelectionInfoRec *info = (SelectionInfoRec *) args;
    Selection *selection = info->selection;
    int subtype;
    CARD32 eventMask;

    switch (info->kind) {
    case SelectionSetOwner:
        subtype = XFixesSetSelectionOwnerNotify;
        eventMask = XFixesSetSelectionOwnerNotifyMask;
        break;
    case SelectionWindowDestroy:
        subtype = XFixesSelectionWindowDestroyNotify;
        eventMask = XFixesSelectionWindowDestroyNotifyMask;
        break;
    case SelectionClientClose:
        subtype = XFixesSelectionClientCloseNotify;
        eventMask = XFixesSelectionClientCloseNotifyMask;
        break;
    default:
        return;
    }
    UpdateCurrentTimeIf();
    for (e = selectionEvents; e; e = e->next) {
        if (e->selection == selection->selection && (e->eventMask & eventMask)) {
            xXFixesSelectionNotifyEvent ev = {
                .type = XFixesEventBase + XFixesSelectionNotify,
                .subtype = subtype,
                .window = e->pWindow->drawable.id,
                .owner = (subtype == XFixesSetSelectionOwnerNotify) ?
                            selection->window : 0,
                .selection = e->selection,
                .timestamp = currentTime.milliseconds,
                .selectionTimestamp = selection->lastTimeChanged.milliseconds
            };
            WriteEventsToClient(e->pClient, 1, (xEvent *) &ev);
        }
    }
}

static Bool
CheckSelectionCallback(void)
{
    if (selectionEvents) {
        if (!SelectionCallbackRegistered) {
            if (!AddCallback(&SelectionCallback, XFixesSelectionCallback, NULL))
                return FALSE;
            SelectionCallbackRegistered = TRUE;
        }
    }
    else {
        if (SelectionCallbackRegistered) {
            DeleteCallback(&SelectionCallback, XFixesSelectionCallback, NULL);
            SelectionCallbackRegistered = FALSE;
        }
    }
    return TRUE;
}

#define SelectionAllEvents (XFixesSetSelectionOwnerNotifyMask |\
			    XFixesSelectionWindowDestroyNotifyMask |\
			    XFixesSelectionClientCloseNotifyMask)

static int
XFixesSelectSelectionInput(ClientPtr pClient,
                           Atom selection, WindowPtr pWindow, CARD32 eventMask)
{
    void *val;
    int rc;
    SelectionEventPtr *prev, e;

    rc = XaceHook(XACE_SELECTION_ACCESS, pClient, selection, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    for (prev = &selectionEvents; (e = *prev); prev = &e->next) {
        if (e->selection == selection &&
            e->pClient == pClient && e->pWindow == pWindow) {
            break;
        }
    }
    if (!eventMask) {
        if (e) {
            FreeResource(e->clientResource, 0);
        }
        return Success;
    }
    if (!e) {
        e = (SelectionEventPtr) malloc(sizeof(SelectionEventRec));
        if (!e)
            return BadAlloc;

        e->next = 0;
        e->selection = selection;
        e->pClient = pClient;
        e->pWindow = pWindow;
        e->clientResource = FakeClientID(pClient->index);

        /*
         * Add a resource hanging from the window to
         * catch window destroy
         */
        rc = dixLookupResourceByType(&val, pWindow->drawable.id,
                                     SelectionWindowType, serverClient,
                                     DixGetAttrAccess);
        if (rc != Success)
            if (!AddResource(pWindow->drawable.id, SelectionWindowType,
                             (void *) pWindow)) {
                free(e);
                return BadAlloc;
            }

        if (!AddResource(e->clientResource, SelectionClientType, (void *) e))
            return BadAlloc;

        *prev = e;
        if (!CheckSelectionCallback()) {
            FreeResource(e->clientResource, 0);
            return BadAlloc;
        }
    }
    e->eventMask = eventMask;
    return Success;
}

int
ProcXFixesSelectSelectionInput(ClientPtr client)
{
    REQUEST(xXFixesSelectSelectionInputReq);
    WindowPtr pWin;
    int rc;

    REQUEST_SIZE_MATCH(xXFixesSelectSelectionInputReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (stuff->eventMask & ~SelectionAllEvents) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    return XFixesSelectSelectionInput(client, stuff->selection,
                                      pWin, stuff->eventMask);
}

int _X_COLD
SProcXFixesSelectSelectionInput(ClientPtr client)
{
    REQUEST(xXFixesSelectSelectionInputReq);

    REQUEST_SIZE_MATCH(xXFixesSelectSelectionInputReq);
    swaps(&stuff->length);
    swapl(&stuff->window);
    swapl(&stuff->selection);
    swapl(&stuff->eventMask);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

void _X_COLD
SXFixesSelectionNotifyEvent(xXFixesSelectionNotifyEvent * from,
                            xXFixesSelectionNotifyEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->owner, to->owner);
    cpswapl(from->selection, to->selection);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->selectionTimestamp, to->selectionTimestamp);
}

static int
SelectionFreeClient(void *data, XID id)
{
    SelectionEventPtr old = (SelectionEventPtr) data;
    SelectionEventPtr *prev, e;

    for (prev = &selectionEvents; (e = *prev); prev = &e->next) {
        if (e == old) {
            *prev = e->next;
            free(e);
            CheckSelectionCallback();
            break;
        }
    }
    return 1;
}

static int
SelectionFreeWindow(void *data, XID id)
{
    WindowPtr pWindow = (WindowPtr) data;
    SelectionEventPtr e, next;

    for (e = selectionEvents; e; e = next) {
        next = e->next;
        if (e->pWindow == pWindow) {
            FreeResource(e->clientResource, 0);
        }
    }
    return 1;
}

Bool
XFixesSelectionInit(void)
{
    SelectionClientType = CreateNewResourceType(SelectionFreeClient,
                                                "XFixesSelectionClient");
    SelectionWindowType = CreateNewResourceType(SelectionFreeWindow,
                                                "XFixesSelectionWindow");
    return SelectionClientType && SelectionWindowType;
}
