/*
 * Copyright © 2017 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"
#include "swaprep.h"
#include <unistd.h>

RESTYPE RRLeaseType;

/*
 * Notify of some lease change
 */
void
RRDeliverLeaseEvent(ClientPtr client, WindowPtr window)
{
    ScreenPtr screen = window->drawable.pScreen;
    rrScrPrivPtr scr_priv = rrGetScrPriv(screen);
    RRLeasePtr lease;

    UpdateCurrentTimeIf();
    xorg_list_for_each_entry(lease, &scr_priv->leases, list) {
        if (lease->id != None && (lease->state == RRLeaseCreating ||
                                  lease->state == RRLeaseTerminating))
        {
            xRRLeaseNotifyEvent le = (xRRLeaseNotifyEvent) {
                .type = RRNotify + RREventBase,
                .subCode = RRNotify_Lease,
                .timestamp = currentTime.milliseconds,
                .window = window->drawable.id,
                .lease = lease->id,
                .created = lease->state == RRLeaseCreating,
            };
            WriteEventsToClient(client, 1, (xEvent *) &le);
        }
    }
}

/*
 * Change the state of a lease and let anyone watching leases know
 */
static void
RRLeaseChangeState(RRLeasePtr lease, RRLeaseState old, RRLeaseState new)
{
    ScreenPtr screen = lease->screen;
    rrScrPrivPtr scr_priv = rrGetScrPriv(screen);

    lease->state = old;
    scr_priv->leasesChanged = TRUE;
    RRSetChanged(lease->screen);
    RRTellChanged(lease->screen);
    scr_priv->leasesChanged = FALSE;
    lease->state = new;
}

/*
 * Allocate and initialize a lease
 */
static RRLeasePtr
RRLeaseAlloc(ScreenPtr screen, RRLease lid, int numCrtcs, int numOutputs)
{
    RRLeasePtr lease;
    lease = calloc(1,
                   sizeof(RRLeaseRec) +
                   numCrtcs * sizeof (RRCrtcPtr) +
                   numOutputs * sizeof(RROutputPtr));
    if (!lease)
        return NULL;
    lease->screen = screen;
    xorg_list_init(&lease->list);
    lease->id = lid;
    lease->state = RRLeaseCreating;
    lease->numCrtcs = numCrtcs;
    lease->numOutputs = numOutputs;
    lease->crtcs = (RRCrtcPtr *) (lease + 1);
    lease->outputs = (RROutputPtr *) (lease->crtcs + numCrtcs);
    return lease;
}

/*
 * Check if a crtc is leased
 */
Bool
RRCrtcIsLeased(RRCrtcPtr crtc)
{
    ScreenPtr screen = crtc->pScreen;
    rrScrPrivPtr scr_priv = rrGetScrPriv(screen);
    RRLeasePtr lease;
    int c;

    xorg_list_for_each_entry(lease, &scr_priv->leases, list) {
        for (c = 0; c < lease->numCrtcs; c++)
            if (lease->crtcs[c] == crtc)
                return TRUE;
    }
    return FALSE;
}

/*
 * Check if an output is leased
 */
Bool
RROutputIsLeased(RROutputPtr output)
{
    ScreenPtr screen = output->pScreen;
    rrScrPrivPtr scr_priv = rrGetScrPriv(screen);
    RRLeasePtr lease;
    int o;

    xorg_list_for_each_entry(lease, &scr_priv->leases, list) {
        for (o = 0; o < lease->numOutputs; o++)
            if (lease->outputs[o] == output)
                return TRUE;
    }
    return FALSE;
}

/*
 * A lease has been terminated.
 * The driver is responsible for noticing and
 * calling this function when that happens
 */

void
RRLeaseTerminated(RRLeasePtr lease)
{
    /* Notify clients with events, but only if this isn't during lease creation */
    if (lease->state == RRLeaseRunning)
        RRLeaseChangeState(lease, RRLeaseTerminating, RRLeaseTerminating);

    if (lease->id != None)
        FreeResource(lease->id, RT_NONE);

    xorg_list_del(&lease->list);
}

/*
 * A lease is completely shut down and is
 * ready to be deallocated
 */

void
RRLeaseFree(RRLeasePtr lease)
{
    free(lease);
}

/*
 * Ask the driver to terminate a lease. The
 * driver will call RRLeaseTerminated when that has
 * finished, which may be some time after this function returns
 * if the driver operation is asynchronous
 */
void
RRTerminateLease(RRLeasePtr lease)
{
    ScreenPtr screen = lease->screen;
    rrScrPrivPtr scr_priv = rrGetScrPriv(screen);

    scr_priv->rrTerminateLease(screen, lease);
}

/*
 * Destroy a lease resource ID. All this
 * does is note that the lease no longer has an ID, and
 * so doesn't appear over the protocol anymore.
 */
static int
RRLeaseDestroyResource(void *value, XID pid)
{
    RRLeasePtr lease = value;

    lease->id = None;
    return 1;
}

/*
 * Create the lease resource type during server initialization
 */
Bool
RRLeaseInit(void)
{
    RRLeaseType = CreateNewResourceType(RRLeaseDestroyResource, "LEASE");
    if (!RRLeaseType)
        return FALSE;
    return TRUE;
}

int
ProcRRCreateLease(ClientPtr client)
{
    REQUEST(xRRCreateLeaseReq);
    xRRCreateLeaseReply rep;
    WindowPtr window;
    ScreenPtr screen;
    rrScrPrivPtr scr_priv;
    RRLeasePtr lease;
    RRCrtc *crtcIds;
    RROutput *outputIds;
    int fd;
    int rc;
    unsigned long len;
    int c, o;

    REQUEST_AT_LEAST_SIZE(xRRCreateLeaseReq);

    LEGAL_NEW_RESOURCE(stuff->lid, client);

    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    len = client->req_len - bytes_to_int32(sizeof(xRRCreateLeaseReq));

    if (len != stuff->nCrtcs + stuff->nOutputs)
        return BadLength;

    screen = window->drawable.pScreen;
    scr_priv = rrGetScrPriv(screen);

    if (!scr_priv)
        return BadMatch;

    if (!scr_priv->rrCreateLease)
        return BadMatch;

    /* Allocate a structure to hold all of the lease information */

    lease = RRLeaseAlloc(screen, stuff->lid, stuff->nCrtcs, stuff->nOutputs);
    if (!lease)
        return BadAlloc;

    /* Look up all of the crtcs */
    crtcIds = (RRCrtc *) (stuff + 1);
    for (c = 0; c < stuff->nCrtcs; c++) {
        RRCrtcPtr crtc;

	rc = dixLookupResourceByType((void **)&crtc, crtcIds[c],
                                     RRCrtcType, client, DixSetAttrAccess);

        if (rc != Success) {
            client->errorValue = crtcIds[c];
            goto bail_lease;
        }

        if (RRCrtcIsLeased(crtc)) {
            client->errorValue = crtcIds[c];
            rc = BadAccess;
            goto bail_lease;
        }

        lease->crtcs[c] = crtc;
    }

    /* Look up all of the outputs */
    outputIds = (RROutput *) (crtcIds + stuff->nCrtcs);
    for (o = 0; o < stuff->nOutputs; o++) {
        RROutputPtr output;

	rc = dixLookupResourceByType((void **)&output, outputIds[o],
                                     RROutputType, client, DixSetAttrAccess);
        if (rc != Success) {
            client->errorValue = outputIds[o];
            goto bail_lease;
        }

        if (RROutputIsLeased(output)) {
            client->errorValue = outputIds[o];
            rc = BadAccess;
            goto bail_lease;
        }

        lease->outputs[o] = output;
    }

    rc = scr_priv->rrCreateLease(screen, lease, &fd);
    if (rc != Success)
        goto bail_lease;

    xorg_list_add(&lease->list, &scr_priv->leases);

    if (!AddResource(stuff->lid, RRLeaseType, lease)) {
        close(fd);
        return BadAlloc;
    }

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        RRTerminateLease(lease);
        close(fd);
        return BadAlloc;
    }

    RRLeaseChangeState(lease, RRLeaseCreating, RRLeaseRunning);

    rep = (xRRCreateLeaseReply) {
        .type = X_Reply,
        .nfd = 1,
        .sequenceNumber = client->sequence,
        .length = 0,
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    WriteToClient(client, sizeof (rep), &rep);

    return Success;

bail_lease:
    free(lease);
    return rc;
}

int
ProcRRFreeLease(ClientPtr client)
{
    REQUEST(xRRFreeLeaseReq);
    RRLeasePtr lease;

    REQUEST_SIZE_MATCH(xRRFreeLeaseReq);

    VERIFY_RR_LEASE(stuff->lid, lease, DixDestroyAccess);

    if (stuff->terminate)
        RRTerminateLease(lease);
    else
        /* Get rid of the resource database entry */
        FreeResource(stuff->lid, RT_NONE);

    return Success;
}
