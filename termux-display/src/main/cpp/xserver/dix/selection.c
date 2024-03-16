/************************************************************

Copyright 1987, 1989, 1998  The Open Group

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

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "windowstr.h"
#include "dixstruct.h"
#include "dispatch.h"
#include "selection.h"
#include "xace.h"

/*****************************************************************
 * Selection Stuff
 *
 *    dixLookupSelection
 *
 *   Selections are global to the server.  The list of selections should
 *   not be traversed directly.  Instead, use the functions listed above.
 *
 *****************************************************************/

Selection *CurrentSelections;
CallbackListPtr SelectionCallback;

int
dixLookupSelection(Selection ** result, Atom selectionName,
                   ClientPtr client, Mask access_mode)
{
    Selection *pSel;
    int rc = BadMatch;

    client->errorValue = selectionName;

    for (pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->selection == selectionName)
            break;

    if (pSel)
        rc = XaceHookSelectionAccess(client, &pSel, access_mode);
    *result = pSel;
    return rc;
}

void
InitSelections(void)
{
    Selection *pSel, *pNextSel;

    pSel = CurrentSelections;
    while (pSel) {
        pNextSel = pSel->next;
        dixFreeObjectWithPrivates(pSel, PRIVATE_SELECTION);
        pSel = pNextSel;
    }

    CurrentSelections = NULL;
}

static _X_INLINE void
CallSelectionCallback(Selection * pSel, ClientPtr client,
                      SelectionCallbackKind kind)
{
    SelectionInfoRec info = { pSel, client, kind };
    CallCallbacks(&SelectionCallback, &info);
}

void
DeleteWindowFromAnySelections(WindowPtr pWin)
{
    Selection *pSel;

    for (pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->pWin == pWin) {
            CallSelectionCallback(pSel, NULL, SelectionWindowDestroy);

            pSel->pWin = (WindowPtr) NULL;
            pSel->window = None;
            pSel->client = NullClient;
        }
}

void
DeleteClientFromAnySelections(ClientPtr client)
{
    Selection *pSel;

    for (pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->client == client) {
            CallSelectionCallback(pSel, NULL, SelectionClientClose);

            pSel->pWin = (WindowPtr) NULL;
            pSel->window = None;
            pSel->client = NullClient;
        }
}

int
ProcSetSelectionOwner(ClientPtr client)
{
    WindowPtr pWin = NULL;
    TimeStamp time;
    Selection *pSel;
    int rc;

    REQUEST(xSetSelectionOwnerReq);
    REQUEST_SIZE_MATCH(xSetSelectionOwnerReq);

    UpdateCurrentTime();
    time = ClientTimeToServerTime(stuff->time);

    /* If the client's time stamp is in the future relative to the server's
       time stamp, do not set the selection, just return success. */
    if (CompareTimeStamps(time, currentTime) == LATER)
        return Success;

    if (stuff->window != None) {
        rc = dixLookupWindow(&pWin, stuff->window, client, DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }
    if (!ValidAtom(stuff->selection)) {
        client->errorValue = stuff->selection;
        return BadAtom;
    }

    /*
     * First, see if the selection is already set...
     */
    rc = dixLookupSelection(&pSel, stuff->selection, client, DixSetAttrAccess);

    if (rc == Success) {
        /* If the timestamp in client's request is in the past relative
           to the time stamp indicating the last time the owner of the
           selection was set, do not set the selection, just return
           success. */
        if (CompareTimeStamps(time, pSel->lastTimeChanged) == EARLIER)
            return Success;
        if (pSel->client && (!pWin || (pSel->client != client))) {
            xEvent event = {
                .u.selectionClear.time = time.milliseconds,
                .u.selectionClear.window = pSel->window,
                .u.selectionClear.atom = pSel->selection
            };
            event.u.u.type = SelectionClear;
            WriteEventsToClient(pSel->client, 1, &event);
        }
    }
    else if (rc == BadMatch) {
        /*
         * It doesn't exist, so add it...
         */
        pSel = dixAllocateObjectWithPrivates(Selection, PRIVATE_SELECTION);
        if (!pSel)
            return BadAlloc;

        pSel->selection = stuff->selection;

        /* security creation/labeling check */
        rc = XaceHookSelectionAccess(client, &pSel,
                                     DixCreateAccess | DixSetAttrAccess);
        if (rc != Success) {
            free(pSel);
            return rc;
        }

        pSel->next = CurrentSelections;
        CurrentSelections = pSel;
    }
    else
        return rc;

    pSel->lastTimeChanged = time;
    pSel->window = stuff->window;
    pSel->pWin = pWin;
    pSel->client = (pWin ? client : NullClient);

    CallSelectionCallback(pSel, client, SelectionSetOwner);
    return Success;
}

int
ProcGetSelectionOwner(ClientPtr client)
{
    int rc;
    Selection *pSel;
    xGetSelectionOwnerReply reply;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    if (!ValidAtom(stuff->id)) {
        client->errorValue = stuff->id;
        return BadAtom;
    }

    reply = (xGetSelectionOwnerReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
    };

    rc = dixLookupSelection(&pSel, stuff->id, client, DixGetAttrAccess);
    if (rc == Success)
        reply.owner = pSel->window;
    else if (rc == BadMatch)
        reply.owner = None;
    else
        return rc;

    WriteReplyToClient(client, sizeof(xGetSelectionOwnerReply), &reply);
    return Success;
}

int
ProcConvertSelection(ClientPtr client)
{
    Bool paramsOkay;
    xEvent event;
    WindowPtr pWin;
    Selection *pSel;
    int rc;

    REQUEST(xConvertSelectionReq);
    REQUEST_SIZE_MATCH(xConvertSelectionReq);

    rc = dixLookupWindow(&pWin, stuff->requestor, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    paramsOkay = ValidAtom(stuff->selection) && ValidAtom(stuff->target);
    paramsOkay &= (stuff->property == None) || ValidAtom(stuff->property);
    if (!paramsOkay) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    if (stuff->time == CurrentTime)
        UpdateCurrentTime();

    rc = dixLookupSelection(&pSel, stuff->selection, client, DixReadAccess);

    memset(&event, 0, sizeof(xEvent));
    if (rc != Success && rc != BadMatch)
        return rc;
    else if (rc == Success && pSel->window != None) {
        event.u.u.type = SelectionRequest;
        event.u.selectionRequest.owner = pSel->window;
        event.u.selectionRequest.time = stuff->time;
        event.u.selectionRequest.requestor = stuff->requestor;
        event.u.selectionRequest.selection = stuff->selection;
        event.u.selectionRequest.target = stuff->target;
        event.u.selectionRequest.property = stuff->property;
        if (pSel->client && pSel->client != serverClient &&
            !pSel->client->clientGone) {
            WriteEventsToClient(pSel->client, 1, &event);
            return Success;
        }
    }

    event.u.u.type = SelectionNotify;
    event.u.selectionNotify.time = stuff->time;
    event.u.selectionNotify.requestor = stuff->requestor;
    event.u.selectionNotify.selection = stuff->selection;
    event.u.selectionNotify.target = stuff->target;
    event.u.selectionNotify.property = None;
    WriteEventsToClient(client, 1, &event);
    return Success;
}
