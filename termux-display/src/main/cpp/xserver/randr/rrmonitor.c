/*
 * Copyright © 2014 Keith Packard
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

static Atom
RRMonitorCrtcName(RRCrtcPtr crtc)
{
    char        name[20];

    if (crtc->numOutputs) {
        RROutputPtr     output = crtc->outputs[0];
        return MakeAtom(output->name, output->nameLength, TRUE);
    }
    sprintf(name, "Monitor-%08lx", (unsigned long int)crtc->id);
    return MakeAtom(name, strlen(name), TRUE);
}

static Bool
RRMonitorCrtcPrimary(RRCrtcPtr crtc)
{
    ScreenPtr screen = crtc->pScreen;
    rrScrPrivPtr pScrPriv = rrGetScrPriv(screen);
    int o;

    for (o = 0; o < crtc->numOutputs; o++)
        if (crtc->outputs[o] == pScrPriv->primaryOutput)
            return TRUE;
    return FALSE;
}

#define DEFAULT_PIXELS_PER_MM   (96.0 / 25.4)

static void
RRMonitorGetCrtcGeometry(RRCrtcPtr crtc, RRMonitorGeometryPtr geometry)
{
    ScreenPtr screen = crtc->pScreen;
    rrScrPrivPtr pScrPriv = rrGetScrPriv(screen);
    BoxRec      panned_area;

    /* Check to see if crtc is panned and return the full area when applicable. */
    if (pScrPriv && pScrPriv->rrGetPanning &&
        pScrPriv->rrGetPanning(screen, crtc, &panned_area, NULL, NULL) &&
        (panned_area.x2 > panned_area.x1) &&
        (panned_area.y2 > panned_area.y1)) {
        geometry->box = panned_area;
    }
    else {
        int width, height;

        RRCrtcGetScanoutSize(crtc, &width, &height);
        geometry->box.x1 = crtc->x;
        geometry->box.y1 = crtc->y;
        geometry->box.x2 = geometry->box.x1 + width;
        geometry->box.y2 = geometry->box.y1 + height;
    }
    if (crtc->numOutputs && crtc->outputs[0]->mmWidth && crtc->outputs[0]->mmHeight) {
        RROutputPtr output = crtc->outputs[0];
        geometry->mmWidth = output->mmWidth;
        geometry->mmHeight = output->mmHeight;
    } else {
        geometry->mmWidth = floor ((geometry->box.x2 - geometry->box.x1) / DEFAULT_PIXELS_PER_MM + 0.5);
        geometry->mmHeight = floor ((geometry->box.y2 - geometry->box.y1) / DEFAULT_PIXELS_PER_MM + 0.5);
    }
}

static Bool
RRMonitorSetFromServer(RRCrtcPtr crtc, RRMonitorPtr monitor)
{
    int o;

    monitor->name = RRMonitorCrtcName(crtc);
    monitor->pScreen = crtc->pScreen;
    monitor->numOutputs = crtc->numOutputs;
    monitor->outputs = calloc(crtc->numOutputs, sizeof(RROutput));
    if (!monitor->outputs)
        return FALSE;
    for (o = 0; o < crtc->numOutputs; o++)
        monitor->outputs[o] = crtc->outputs[o]->id;
    monitor->primary = RRMonitorCrtcPrimary(crtc);
    monitor->automatic = TRUE;
    RRMonitorGetCrtcGeometry(crtc, &monitor->geometry);
    return TRUE;
}

static Bool
RRMonitorAutomaticGeometry(RRMonitorPtr monitor)
{
    return (monitor->geometry.box.x1 == 0 &&
            monitor->geometry.box.y1 == 0 &&
            monitor->geometry.box.x2 == 0 &&
            monitor->geometry.box.y2 == 0);
}

static void
RRMonitorGetGeometry(RRMonitorPtr monitor, RRMonitorGeometryPtr geometry)
{
    if (RRMonitorAutomaticGeometry(monitor) && monitor->numOutputs > 0) {
        ScreenPtr               screen = monitor->pScreen;
        rrScrPrivPtr            pScrPriv = rrGetScrPriv(screen);
        RRMonitorGeometryRec    first = { .box = { 0, 0, 0, 0 }, .mmWidth = 0, .mmHeight = 0 };
        RRMonitorGeometryRec    this;
        int                     c, o, co;
        int                     active_crtcs = 0;

        *geometry = first;
        for (o = 0; o < monitor->numOutputs; o++) {
            RRCrtcPtr   crtc = NULL;
            Bool        in_use = FALSE;

            for (c = 0; !in_use && c < pScrPriv->numCrtcs; c++) {
                crtc = pScrPriv->crtcs[c];
                if (!crtc->mode)
                    continue;
                for (co = 0; !in_use && co < crtc->numOutputs; co++)
                    if (monitor->outputs[o] == crtc->outputs[co]->id)
                        in_use = TRUE;
            }

            if (!in_use)
                continue;

            RRMonitorGetCrtcGeometry(crtc, &this);

            if (active_crtcs == 0) {
                first = this;
                *geometry = this;
            } else {
                geometry->box.x1 = min(this.box.x1, geometry->box.x1);
                geometry->box.x2 = max(this.box.x2, geometry->box.x2);
                geometry->box.y1 = min(this.box.y1, geometry->box.y1);
                geometry->box.y2 = max(this.box.y2, geometry->box.y2);
            }
            active_crtcs++;
        }

        /* Adjust physical sizes to account for total area */
        if (active_crtcs > 1 && first.box.x2 != first.box.x1 && first.box.y2 != first.box.y1) {
            geometry->mmWidth = (this.box.x2 - this.box.x1) / (first.box.x2 - first.box.x1) * first.mmWidth;
            geometry->mmHeight = (this.box.y2 - this.box.y1) / (first.box.y2 - first.box.y1) * first.mmHeight;
        }
    } else {
        *geometry = monitor->geometry;
    }
}

static Bool
RRMonitorSetFromClient(RRMonitorPtr client_monitor, RRMonitorPtr monitor)
{
    monitor->name = client_monitor->name;
    monitor->pScreen = client_monitor->pScreen;
    monitor->numOutputs = client_monitor->numOutputs;
    monitor->outputs = calloc(client_monitor->numOutputs, sizeof (RROutput));
    if (!monitor->outputs && client_monitor->numOutputs)
        return FALSE;
    memcpy(monitor->outputs, client_monitor->outputs, client_monitor->numOutputs * sizeof (RROutput));
    monitor->primary = client_monitor->primary;
    monitor->automatic = client_monitor->automatic;
    RRMonitorGetGeometry(client_monitor, &monitor->geometry);
    return TRUE;
}

typedef struct _rrMonitorList {
    int         num_client;
    int         num_server;
    RRCrtcPtr   *server_crtc;
    int         num_crtcs;
    int         client_primary;
    int         server_primary;
} RRMonitorListRec, *RRMonitorListPtr;

static Bool
RRMonitorInitList(ScreenPtr screen, RRMonitorListPtr mon_list, Bool get_active)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    int                 m, o, c, sc;
    int                 numCrtcs;
    ScreenPtr           secondary;

    if (!RRGetInfo(screen, FALSE))
        return FALSE;

    /* Count the number of crtcs in this and any secondary screens */
    numCrtcs = pScrPriv->numCrtcs;
    xorg_list_for_each_entry(secondary, &screen->secondary_list, secondary_head) {
        rrScrPrivPtr pSecondaryPriv;

        if (!secondary->is_output_secondary)
            continue;

        pSecondaryPriv = rrGetScrPriv(secondary);
        numCrtcs += pSecondaryPriv->numCrtcs;
    }
    mon_list->num_crtcs = numCrtcs;

    mon_list->server_crtc = calloc(numCrtcs * 2, sizeof (RRCrtcPtr));
    if (!mon_list->server_crtc)
        return FALSE;

    /* Collect pointers to all of the active crtcs */
    c = 0;
    for (sc = 0; sc < pScrPriv->numCrtcs; sc++, c++) {
        if (pScrPriv->crtcs[sc]->mode != NULL)
            mon_list->server_crtc[c] = pScrPriv->crtcs[sc];
    }

    xorg_list_for_each_entry(secondary, &screen->secondary_list, secondary_head) {
        rrScrPrivPtr pSecondaryPriv;

        if (!secondary->is_output_secondary)
            continue;

        pSecondaryPriv = rrGetScrPriv(secondary);
        for (sc = 0; sc < pSecondaryPriv->numCrtcs; sc++, c++) {
            if (pSecondaryPriv->crtcs[sc]->mode != NULL)
                mon_list->server_crtc[c] = pSecondaryPriv->crtcs[sc];
        }
    }

    /* Walk the list of client-defined monitors, clearing the covered
     * CRTCs from the full list and finding whether one of the
     * monitors is primary
     */
    mon_list->num_client = pScrPriv->numMonitors;
    mon_list->client_primary = -1;

    for (m = 0; m < pScrPriv->numMonitors; m++) {
        RRMonitorPtr monitor = pScrPriv->monitors[m];
        if (get_active) {
            RRMonitorGeometryRec geom;

            RRMonitorGetGeometry(monitor, &geom);
            if (geom.box.x2 - geom.box.x1 == 0 ||
                geom.box.y2 - geom.box.y1 == 0) {
                mon_list->num_client--;
                continue;
            }
        }
        if (monitor->primary && mon_list->client_primary == -1)
            mon_list->client_primary = m;
        for (o = 0; o < monitor->numOutputs; o++) {
            for (c = 0; c < numCrtcs; c++) {
                RRCrtcPtr       crtc = mon_list->server_crtc[c];
                if (crtc) {
                    int             co;
                    for (co = 0; co < crtc->numOutputs; co++)
                        if (crtc->outputs[co]->id == monitor->outputs[o]) {
                            mon_list->server_crtc[c] = NULL;
                            break;
                        }
                }
            }
        }
    }

    /* Now look at the active CRTCs, and count
     * those not covered by a client monitor, as well
     * as finding whether one of them is marked primary
     */
    mon_list->num_server = 0;
    mon_list->server_primary = -1;

    for (c = 0; c < mon_list->num_crtcs; c++) {
        RRCrtcPtr       crtc = mon_list->server_crtc[c];

        if (!crtc)
            continue;

        mon_list->num_server++;

        if (RRMonitorCrtcPrimary(crtc) && mon_list->server_primary == -1)
            mon_list->server_primary = c;
    }
    return TRUE;
}

static void
RRMonitorFiniList(RRMonitorListPtr list)
{
    free(list->server_crtc);
}

/* Construct a complete list of protocol-visible monitors, including
 * the manually generated ones as well as those generated
 * automatically from the remaining CRCTs
 */

Bool
RRMonitorMakeList(ScreenPtr screen, Bool get_active, RRMonitorPtr *monitors_ret, int *nmon_ret)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    RRMonitorListRec    list;
    int                 m, c;
    RRMonitorPtr        mon, monitors;
    Bool                has_primary = FALSE;

    if (!pScrPriv)
        return FALSE;

    if (!RRMonitorInitList(screen, &list, get_active))
        return FALSE;

    monitors = calloc(list.num_client + list.num_server, sizeof (RRMonitorRec));
    if (!monitors) {
        RRMonitorFiniList(&list);
        return FALSE;
    }

    mon = monitors;

    /* Fill in the primary monitor data first
     */
    if (list.client_primary >= 0) {
        RRMonitorSetFromClient(pScrPriv->monitors[list.client_primary], mon);
        mon++;
    } else if (list.server_primary >= 0) {
        RRMonitorSetFromServer(list.server_crtc[list.server_primary], mon);
        mon++;
    }

    /* Fill in the client-defined monitors next
     */
    for (m = 0; m < pScrPriv->numMonitors; m++) {
        if (m == list.client_primary)
            continue;
        if (get_active) {
            RRMonitorGeometryRec geom;

            RRMonitorGetGeometry(pScrPriv->monitors[m], &geom);
            if (geom.box.x2 - geom.box.x1 == 0 ||
                geom.box.y2 - geom.box.y1 == 0) {
                continue;
            }
        }
        RRMonitorSetFromClient(pScrPriv->monitors[m], mon);
        if (has_primary)
            mon->primary = FALSE;
        else if (mon->primary)
            has_primary = TRUE;
        mon++;
    }

    /* And finish with the list of crtc-inspired monitors
     */
    for (c = 0; c < list.num_crtcs; c++) {
        RRCrtcPtr crtc = list.server_crtc[c];
        if (c == list.server_primary && list.client_primary < 0)
            continue;

        if (!list.server_crtc[c])
            continue;

        RRMonitorSetFromServer(crtc, mon);
        if (has_primary)
            mon->primary = FALSE;
        else if (mon->primary)
            has_primary = TRUE;
        mon++;
    }

    RRMonitorFiniList(&list);
    *nmon_ret = list.num_client + list.num_server;
    *monitors_ret = monitors;
    return TRUE;
}

int
RRMonitorCountList(ScreenPtr screen)
{
    RRMonitorListRec    list;
    int                 nmon;

    if (!RRMonitorInitList(screen, &list, FALSE))
        return -1;
    nmon = list.num_client + list.num_server;
    RRMonitorFiniList(&list);
    return nmon;
}

void
RRMonitorFree(RRMonitorPtr monitor)
{
    free(monitor);
}

RRMonitorPtr
RRMonitorAlloc(int noutput)
{
    RRMonitorPtr        monitor;

    monitor = calloc(1, sizeof (RRMonitorRec) + noutput * sizeof (RROutput));
    if (!monitor)
        return NULL;
    monitor->numOutputs = noutput;
    monitor->outputs = (RROutput *) (monitor + 1);
    return monitor;
}

static int
RRMonitorDelete(ClientPtr client, ScreenPtr screen, Atom name)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    int                 m;

    if (!pScrPriv) {
        client->errorValue = name;
        return BadAtom;
    }

    for (m = 0; m < pScrPriv->numMonitors; m++) {
        RRMonitorPtr    monitor = pScrPriv->monitors[m];
        if (monitor->name == name) {
            memmove(pScrPriv->monitors + m, pScrPriv->monitors + m + 1,
                    (pScrPriv->numMonitors - (m + 1)) * sizeof (RRMonitorPtr));
            --pScrPriv->numMonitors;
            RRMonitorFree(monitor);
            return Success;
        }
    }

    client->errorValue = name;
    return BadValue;
}

static Bool
RRMonitorMatchesOutputName(ScreenPtr screen, Atom name)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    int                 o;
    const char          *str = NameForAtom(name);
    int                 len = strlen(str);

    for (o = 0; o < pScrPriv->numOutputs; o++) {
        RROutputPtr     output = pScrPriv->outputs[o];

        if (output->nameLength == len && !memcmp(output->name, str, len))
            return TRUE;
    }
    return FALSE;
}

int
RRMonitorAdd(ClientPtr client, ScreenPtr screen, RRMonitorPtr monitor)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    int                 m;
    ScreenPtr           secondary;
    RRMonitorPtr        *monitors;

    if (!pScrPriv)
        return BadAlloc;

    /* 	'name' must not match the name of any Output on the screen, or
     *	a Value error results.
     */

    if (RRMonitorMatchesOutputName(screen, monitor->name)) {
        client->errorValue = monitor->name;
        return BadValue;
    }

    xorg_list_for_each_entry(secondary, &screen->secondary_list, secondary_head) {
        if (!secondary->is_output_secondary)
            continue;

        if (RRMonitorMatchesOutputName(secondary, monitor->name)) {
            client->errorValue = monitor->name;
            return BadValue;
        }
    }

    /* 'name' must not match the name of any Monitor on the screen, or
     * a Value error results.
     */

    for (m = 0; m < pScrPriv->numMonitors; m++) {
        if (pScrPriv->monitors[m]->name == monitor->name) {
            client->errorValue = monitor->name;
            return BadValue;
        }
    }

    /* Allocate space for the new pointer. This is done before
     * removing matching monitors as it may fail, and the request
     * needs to not have any side-effects on failure
     */
    if (pScrPriv->numMonitors)
        monitors = reallocarray(pScrPriv->monitors,
                                pScrPriv->numMonitors + 1,
                                sizeof (RRMonitorPtr));
    else
        monitors = malloc(sizeof (RRMonitorPtr));

    if (!monitors)
        return BadAlloc;

    pScrPriv->monitors = monitors;

    for (m = 0; m < pScrPriv->numMonitors; m++) {
        RRMonitorPtr    existing = pScrPriv->monitors[m];
        int             o, eo;

	/* If 'name' matches an existing Monitor on the screen, the
         * existing one will be deleted as if RRDeleteMonitor were called.
         */
        if (existing->name == monitor->name) {
            (void) RRMonitorDelete(client, screen, existing->name);
            continue;
        }

        /* For each output in 'info.outputs', each one is removed from all
         * pre-existing Monitors. If removing the output causes the list
         * of outputs for that Monitor to become empty, then that
         * Monitor will be deleted as if RRDeleteMonitor were called.
         */

        for (eo = 0; eo < existing->numOutputs; eo++) {
            for (o = 0; o < monitor->numOutputs; o++) {
                if (monitor->outputs[o] == existing->outputs[eo]) {
                    memmove(existing->outputs + eo, existing->outputs + eo + 1,
                            (existing->numOutputs - (eo + 1)) * sizeof (RROutput));
                    --existing->numOutputs;
                    --eo;
                    break;
                }
            }
            if (existing->numOutputs == 0) {
                (void) RRMonitorDelete(client, screen, existing->name);
                break;
            }
        }
        if (monitor->primary)
            existing->primary = FALSE;
    }

    /* Add the new one to the list
     */
    pScrPriv->monitors[pScrPriv->numMonitors++] = monitor;

    return Success;
}

void
RRMonitorFreeList(RRMonitorPtr monitors, int nmon)
{
    int m;

    for (m = 0; m < nmon; m++)
        free(monitors[m].outputs);
    free(monitors);
}

void
RRMonitorInit(ScreenPtr screen)
{
    rrScrPrivPtr pScrPriv = rrGetScrPriv(screen);

    if (!pScrPriv)
        return;

    pScrPriv->numMonitors = 0;
    pScrPriv->monitors = NULL;
}

void
RRMonitorClose(ScreenPtr screen)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);
    int                 m;

    if (!pScrPriv)
        return;

    for (m = 0; m < pScrPriv->numMonitors; m++)
        RRMonitorFree(pScrPriv->monitors[m]);
    free(pScrPriv->monitors);
    pScrPriv->monitors = NULL;
    pScrPriv->numMonitors = 0;
}

static CARD32
RRMonitorTimestamp(ScreenPtr screen)
{
    rrScrPrivPtr        pScrPriv = rrGetScrPriv(screen);

    /* XXX should take client monitor changes into account */
    return pScrPriv->lastConfigTime.milliseconds;
}

int
ProcRRGetMonitors(ClientPtr client)
{
    REQUEST(xRRGetMonitorsReq);
    xRRGetMonitorsReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
    };
    WindowPtr           window;
    ScreenPtr           screen;
    int                 r;
    RRMonitorPtr        monitors;
    int                 nmonitors;
    int                 noutputs;
    int                 m;
    Bool                get_active;
    REQUEST_SIZE_MATCH(xRRGetMonitorsReq);
    r = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (r != Success)
        return r;
    screen = window->drawable.pScreen;

    get_active = stuff->get_active;
    if (!RRMonitorMakeList(screen, get_active, &monitors, &nmonitors))
        return BadAlloc;

    rep.timestamp = RRMonitorTimestamp(screen);

    noutputs = 0;
    for (m = 0; m < nmonitors; m++) {
        rep.length += SIZEOF(xRRMonitorInfo) >> 2;
        rep.length += monitors[m].numOutputs;
        noutputs += monitors[m].numOutputs;
    }

    rep.nmonitors = nmonitors;
    rep.noutputs = noutputs;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swapl(&rep.nmonitors);
        swapl(&rep.noutputs);
    }
    WriteToClient(client, sizeof(xRRGetMonitorsReply), &rep);

    client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;

    for (m = 0; m < nmonitors; m++) {
        RRMonitorPtr    monitor = &monitors[m];
        xRRMonitorInfo  info = {
            .name = monitor->name,
            .primary = monitor->primary,
            .automatic = monitor->automatic,
            .noutput = monitor->numOutputs,
            .x = monitor->geometry.box.x1,
            .y = monitor->geometry.box.y1,
            .width = monitor->geometry.box.x2 - monitor->geometry.box.x1,
            .height = monitor->geometry.box.y2 - monitor->geometry.box.y1,
            .widthInMillimeters = monitor->geometry.mmWidth,
            .heightInMillimeters = monitor->geometry.mmHeight,
        };
        if (client->swapped) {
            swapl(&info.name);
            swaps(&info.noutput);
            swaps(&info.x);
            swaps(&info.y);
            swaps(&info.width);
            swaps(&info.height);
            swapl(&info.widthInMillimeters);
            swapl(&info.heightInMillimeters);
        }

        WriteToClient(client, sizeof(xRRMonitorInfo), &info);
        WriteSwappedDataToClient(client, monitor->numOutputs * sizeof (RROutput), monitor->outputs);
    }

    RRMonitorFreeList(monitors, nmonitors);

    return Success;
}

int
ProcRRSetMonitor(ClientPtr client)
{
    REQUEST(xRRSetMonitorReq);
    WindowPtr           window;
    ScreenPtr           screen;
    RRMonitorPtr        monitor;
    int                 r;

    REQUEST_AT_LEAST_SIZE(xRRSetMonitorReq);

    if (stuff->monitor.noutput != stuff->length - (SIZEOF(xRRSetMonitorReq) >> 2))
        return BadLength;

    r = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (r != Success)
        return r;
    screen = window->drawable.pScreen;

    if (!ValidAtom(stuff->monitor.name))
        return BadAtom;

    /* Allocate the new monitor */
    monitor = RRMonitorAlloc(stuff->monitor.noutput);
    if (!monitor)
        return BadAlloc;

    /* Fill in the bits from the request */
    monitor->pScreen = screen;
    monitor->name = stuff->monitor.name;
    monitor->primary = stuff->monitor.primary;
    monitor->automatic = FALSE;
    memcpy(monitor->outputs, stuff + 1, stuff->monitor.noutput * sizeof (RROutput));
    monitor->geometry.box.x1 = stuff->monitor.x;
    monitor->geometry.box.y1 = stuff->monitor.y;
    monitor->geometry.box.x2 = stuff->monitor.x + stuff->monitor.width;
    monitor->geometry.box.y2 = stuff->monitor.y + stuff->monitor.height;
    monitor->geometry.mmWidth = stuff->monitor.widthInMillimeters;
    monitor->geometry.mmHeight = stuff->monitor.heightInMillimeters;

    r = RRMonitorAdd(client, screen, monitor);
    if (r == Success)
        RRSendConfigNotify(screen);
    else
        RRMonitorFree(monitor);
    return r;
}

int
ProcRRDeleteMonitor(ClientPtr client)
{
    REQUEST(xRRDeleteMonitorReq);
    WindowPtr           window;
    ScreenPtr           screen;
    int                 r;

    REQUEST_SIZE_MATCH(xRRDeleteMonitorReq);
    r = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (r != Success)
        return r;
    screen = window->drawable.pScreen;

    if (!ValidAtom(stuff->name)) {
        client->errorValue = stuff->name;
        return BadAtom;
    }

    r = RRMonitorDelete(client, screen, stuff->name);
    if (r == Success)
        RRSendConfigNotify(screen);
    return r;
}
