/*
 * Copyright © 2006 Keith Packard
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

static CARD16
 RR10CurrentSizeID(ScreenPtr pScreen);

/*
 * Edit connection information block so that new clients
 * see the current screen size on connect
 */
static void
RREditConnectionInfo(ScreenPtr pScreen)
{
    xConnSetup *connSetup;
    char *vendor;
    xPixmapFormat *formats;
    xWindowRoot *root;
    xDepth *depth;
    xVisualType *visual;
    int screen = 0;
    int d;

    if (ConnectionInfo == NULL)
        return;

    connSetup = (xConnSetup *) ConnectionInfo;
    vendor = (char *) connSetup + sizeof(xConnSetup);
    formats = (xPixmapFormat *) ((char *) vendor +
                                 pad_to_int32(connSetup->nbytesVendor));
    root = (xWindowRoot *) ((char *) formats +
                            sizeof(xPixmapFormat) *
                            screenInfo.numPixmapFormats);
    while (screen != pScreen->myNum) {
        depth = (xDepth *) ((char *) root + sizeof(xWindowRoot));
        for (d = 0; d < root->nDepths; d++) {
            visual = (xVisualType *) ((char *) depth + sizeof(xDepth));
            depth = (xDepth *) ((char *) visual +
                                depth->nVisuals * sizeof(xVisualType));
        }
        root = (xWindowRoot *) ((char *) depth);
        screen++;
    }
    root->pixWidth = pScreen->width;
    root->pixHeight = pScreen->height;
    root->mmWidth = pScreen->mmWidth;
    root->mmHeight = pScreen->mmHeight;
}

void
RRSendConfigNotify(ScreenPtr pScreen)
{
    WindowPtr pWin = pScreen->root;
    xEvent event = {
        .u.configureNotify.window = pWin->drawable.id,
        .u.configureNotify.aboveSibling = None,
        .u.configureNotify.x = 0,
        .u.configureNotify.y = 0,

    /* XXX xinerama stuff ? */

        .u.configureNotify.width = pWin->drawable.width,
        .u.configureNotify.height = pWin->drawable.height,
        .u.configureNotify.borderWidth = wBorderWidth(pWin),
        .u.configureNotify.override = pWin->overrideRedirect
    };
    event.u.u.type = ConfigureNotify;
    DeliverEvents(pWin, &event, 1, NullWindow);
}

void
RRDeliverScreenEvent(ClientPtr client, WindowPtr pWin, ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    RRCrtcPtr crtc = pScrPriv->numCrtcs ? pScrPriv->crtcs[0] : NULL;
    WindowPtr pRoot = pScreen->root;

    xRRScreenChangeNotifyEvent se = {
        .type = RRScreenChangeNotify + RREventBase,
        .rotation = (CARD8) (crtc ? crtc->rotation : RR_Rotate_0),
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .configTimestamp = pScrPriv->lastConfigTime.milliseconds,
        .root = pRoot->drawable.id,
        .window = pWin->drawable.id,
        .subpixelOrder = PictureGetSubpixelOrder(pScreen),

        .sizeID = RR10CurrentSizeID(pScreen)
    };

    if (se.rotation & (RR_Rotate_90 | RR_Rotate_270)) {
        se.widthInPixels = pScreen->height;
        se.heightInPixels = pScreen->width;
        se.widthInMillimeters = pScreen->mmHeight;
        se.heightInMillimeters = pScreen->mmWidth;
    }
    else {
        se.widthInPixels = pScreen->width;
        se.heightInPixels = pScreen->height;
        se.widthInMillimeters = pScreen->mmWidth;
        se.heightInMillimeters = pScreen->mmHeight;
    }

    WriteEventsToClient(client, 1, (xEvent *) &se);
}

/*
 * Notify the extension that the screen size has been changed.
 * The driver is responsible for calling this whenever it has changed
 * the size of the screen
 */
void
RRScreenSizeNotify(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    /*
     * Deliver ConfigureNotify events when root changes
     * pixel size
     */
    if (pScrPriv->width == pScreen->width &&
        pScrPriv->height == pScreen->height &&
        pScrPriv->mmWidth == pScreen->mmWidth &&
        pScrPriv->mmHeight == pScreen->mmHeight)
        return;

    pScrPriv->width = pScreen->width;
    pScrPriv->height = pScreen->height;
    pScrPriv->mmWidth = pScreen->mmWidth;
    pScrPriv->mmHeight = pScreen->mmHeight;
    RRSetChanged(pScreen);
/*    pScrPriv->sizeChanged = TRUE; */

    RRTellChanged(pScreen);
    RRSendConfigNotify(pScreen);
    RREditConnectionInfo(pScreen);

    RRPointerScreenConfigured(pScreen);
    /*
     * Fix pointer bounds and location
     */
    ScreenRestructured(pScreen);
}

/*
 * Request that the screen be resized
 */
Bool
RRScreenSizeSet(ScreenPtr pScreen,
                CARD16 width, CARD16 height, CARD32 mmWidth, CARD32 mmHeight)
{
    rrScrPriv(pScreen);

#if RANDR_12_INTERFACE
    if (pScrPriv->rrScreenSetSize) {
        return (*pScrPriv->rrScreenSetSize) (pScreen,
                                             width, height, mmWidth, mmHeight);
    }
#endif
#if RANDR_10_INTERFACE
    if (pScrPriv->rrSetConfig) {
        return TRUE;            /* can't set size separately */
    }
#endif
    return FALSE;
}

/*
 * Retrieve valid screen size range
 */
int
ProcRRGetScreenSizeRange(ClientPtr client)
{
    REQUEST(xRRGetScreenSizeRangeReq);
    xRRGetScreenSizeRangeReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    int rc;

    REQUEST_SIZE_MATCH(xRRGetScreenSizeRangeReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    rep = (xRRGetScreenSizeRangeReply) {
        .type = X_Reply,
        .pad = 0,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    if (pScrPriv) {
        if (!RRGetInfo(pScreen, FALSE))
            return BadAlloc;
        rep.minWidth = pScrPriv->minWidth;
        rep.minHeight = pScrPriv->minHeight;
        rep.maxWidth = pScrPriv->maxWidth;
        rep.maxHeight = pScrPriv->maxHeight;
    }
    else {
        rep.maxWidth = rep.minWidth = pScreen->width;
        rep.maxHeight = rep.minHeight = pScreen->height;
    }
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.minWidth);
        swaps(&rep.minHeight);
        swaps(&rep.maxWidth);
        swaps(&rep.maxHeight);
    }
    WriteToClient(client, sizeof(xRRGetScreenSizeRangeReply), &rep);
    return Success;
}

int
ProcRRSetScreenSize(ClientPtr client)
{
    REQUEST(xRRSetScreenSizeReq);
    WindowPtr pWin;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    int i, rc;

    REQUEST_SIZE_MATCH(xRRSetScreenSizeReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    if (!pScrPriv)
        return BadMatch;

    if (stuff->width < pScrPriv->minWidth || pScrPriv->maxWidth < stuff->width) {
        client->errorValue = stuff->width;
        return BadValue;
    }
    if (stuff->height < pScrPriv->minHeight ||
        pScrPriv->maxHeight < stuff->height) {
        client->errorValue = stuff->height;
        return BadValue;
    }
    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[i];
        RRModePtr mode = crtc->mode;

        if (!RRCrtcIsLeased(crtc) && mode) {
            int source_width = mode->mode.width;
            int source_height = mode->mode.height;
            Rotation rotation = crtc->rotation;

            if (rotation & (RR_Rotate_90 | RR_Rotate_270)) {
                source_width = mode->mode.height;
                source_height = mode->mode.width;
            }

            if (crtc->x + source_width > stuff->width ||
                crtc->y + source_height > stuff->height)
                return BadMatch;
        }
    }
    if (stuff->widthInMillimeters == 0 || stuff->heightInMillimeters == 0) {
        client->errorValue = 0;
        return BadValue;
    }
    if (!RRScreenSizeSet(pScreen,
                         stuff->width, stuff->height,
                         stuff->widthInMillimeters,
                         stuff->heightInMillimeters)) {
        return BadMatch;
    }
    return Success;
}


#define update_totals(gpuscreen, pScrPriv) do {       \
    total_crtcs += pScrPriv->numCrtcs;                \
    total_outputs += pScrPriv->numOutputs;            \
    modes = RRModesForScreen(gpuscreen, &num_modes);  \
    if (!modes)                                       \
        return BadAlloc;                              \
    for (j = 0; j < num_modes; j++)                   \
        total_name_len += modes[j]->mode.nameLength;  \
    total_modes += num_modes;                         \
    free(modes);                                      \
} while(0)

static inline void swap_modeinfos(xRRModeInfo *modeinfos, int i)
{
    swapl(&modeinfos[i].id);
    swaps(&modeinfos[i].width);
    swaps(&modeinfos[i].height);
    swapl(&modeinfos[i].dotClock);
    swaps(&modeinfos[i].hSyncStart);
    swaps(&modeinfos[i].hSyncEnd);
    swaps(&modeinfos[i].hTotal);
    swaps(&modeinfos[i].hSkew);
    swaps(&modeinfos[i].vSyncStart);
    swaps(&modeinfos[i].vSyncEnd);
    swaps(&modeinfos[i].vTotal);
    swaps(&modeinfos[i].nameLength);
    swapl(&modeinfos[i].modeFlags);
}

#define update_arrays(gpuscreen, pScrPriv, primary_crtc, has_primary) do {            \
    for (j = 0; j < pScrPriv->numCrtcs; j++) {             \
        if (has_primary && \
            primary_crtc == pScrPriv->crtcs[j]) { \
            has_primary = 0;   \
            continue; \
        }\
        crtcs[crtc_count] = pScrPriv->crtcs[j]->id;        \
        if (client->swapped)                               \
            swapl(&crtcs[crtc_count]);                     \
        crtc_count++;                                      \
    }                                                      \
    for (j = 0; j < pScrPriv->numOutputs; j++) {           \
        outputs[output_count] = pScrPriv->outputs[j]->id;  \
        if (client->swapped)                               \
            swapl(&outputs[output_count]);                 \
        output_count++;                                    \
    }                                                      \
    {                                                      \
        RRModePtr mode;                                    \
        modes = RRModesForScreen(gpuscreen, &num_modes);   \
        for (j = 0; j < num_modes; j++) {                  \
            mode = modes[j];                               \
            modeinfos[mode_count] = mode->mode;            \
            if (client->swapped) {                         \
                swap_modeinfos(modeinfos, mode_count);     \
            }                                              \
            memcpy(names, mode->name, mode->mode.nameLength); \
            names += mode->mode.nameLength;                \
            mode_count++;                                  \
        }                                                  \
        free(modes);                                       \
    }                                                      \
    } while (0)

static int
rrGetMultiScreenResources(ClientPtr client, Bool query, ScreenPtr pScreen)
{
    int j;
    int total_crtcs, total_outputs, total_modes, total_name_len;
    int crtc_count, output_count, mode_count;
    ScreenPtr iter;
    rrScrPrivPtr pScrPriv;
    int num_modes;
    RRModePtr *modes;
    xRRGetScreenResourcesReply rep;
    unsigned long extraLen;
    CARD8 *extra;
    RRCrtc *crtcs;
    RRCrtcPtr primary_crtc = NULL;
    RROutput *outputs;
    xRRModeInfo *modeinfos;
    CARD8 *names;
    int has_primary = 0;

    /* we need to iterate all the GPU primarys and all their output secondarys */
    total_crtcs = 0;
    total_outputs = 0;
    total_modes = 0;
    total_name_len = 0;

    pScrPriv = rrGetScrPriv(pScreen);

    if (query && pScrPriv)
        if (!RRGetInfo(pScreen, query))
            return BadAlloc;

    update_totals(pScreen, pScrPriv);

    xorg_list_for_each_entry(iter, &pScreen->secondary_list, secondary_head) {
        if (!iter->is_output_secondary)
            continue;

        pScrPriv = rrGetScrPriv(iter);

        if (query)
          if (!RRGetInfo(iter, query))
            return BadAlloc;
        update_totals(iter, pScrPriv);
    }

    pScrPriv = rrGetScrPriv(pScreen);
    rep = (xRRGetScreenResourcesReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .configTimestamp = pScrPriv->lastConfigTime.milliseconds,
        .nCrtcs = total_crtcs,
        .nOutputs = total_outputs,
        .nModes = total_modes,
        .nbytesNames = total_name_len
    };

    rep.length = (total_crtcs + total_outputs +
                  total_modes * bytes_to_int32(SIZEOF(xRRModeInfo)) +
                  bytes_to_int32(total_name_len));

    extraLen = rep.length << 2;
    if (extraLen) {
        extra = malloc(extraLen);
        if (!extra) {
            return BadAlloc;
        }
    }
    else
        extra = NULL;

    crtcs = (RRCrtc *)extra;
    outputs = (RROutput *)(crtcs + total_crtcs);
    modeinfos = (xRRModeInfo *)(outputs + total_outputs);
    names = (CARD8 *)(modeinfos + total_modes);

    crtc_count = 0;
    output_count = 0;
    mode_count = 0;

    pScrPriv = rrGetScrPriv(pScreen);
    if (pScrPriv->primaryOutput && pScrPriv->primaryOutput->crtc) {
        has_primary = 1;
        primary_crtc = pScrPriv->primaryOutput->crtc;
        crtcs[0] = pScrPriv->primaryOutput->crtc->id;
        if (client->swapped)
            swapl(&crtcs[0]);
        crtc_count = 1;
    }
    update_arrays(pScreen, pScrPriv, primary_crtc, has_primary);

    xorg_list_for_each_entry(iter, &pScreen->secondary_list, secondary_head) {
        if (!iter->is_output_secondary)
            continue;

        pScrPriv = rrGetScrPriv(iter);

        update_arrays(iter, pScrPriv, primary_crtc, has_primary);
    }

    assert(bytes_to_int32((char *) names - (char *) extra) == rep.length);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swapl(&rep.configTimestamp);
        swaps(&rep.nCrtcs);
        swaps(&rep.nOutputs);
        swaps(&rep.nModes);
        swaps(&rep.nbytesNames);
    }
    WriteToClient(client, sizeof(xRRGetScreenResourcesReply), &rep);
    if (extraLen) {
        WriteToClient(client, extraLen, extra);
        free(extra);
    }
    return Success;
}

static int
rrGetScreenResources(ClientPtr client, Bool query)
{
    REQUEST(xRRGetScreenResourcesReq);
    xRRGetScreenResourcesReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    CARD8 *extra;
    unsigned long extraLen;
    int i, rc, has_primary = 0;
    RRCrtc *crtcs;
    RROutput *outputs;
    xRRModeInfo *modeinfos;
    CARD8 *names;

    REQUEST_SIZE_MATCH(xRRGetScreenResourcesReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    if (query && pScrPriv)
        if (!RRGetInfo(pScreen, query))
            return BadAlloc;

    if (pScreen->output_secondarys)
        return rrGetMultiScreenResources(client, query, pScreen);

    if (!pScrPriv) {
        rep = (xRRGetScreenResourcesReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0,
            .timestamp = currentTime.milliseconds,
            .configTimestamp = currentTime.milliseconds,
            .nCrtcs = 0,
            .nOutputs = 0,
            .nModes = 0,
            .nbytesNames = 0
        };
        extra = NULL;
        extraLen = 0;
    }
    else {
        RRModePtr *modes;
        int num_modes;

        modes = RRModesForScreen(pScreen, &num_modes);
        if (!modes)
            return BadAlloc;

        rep = (xRRGetScreenResourcesReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0,
            .timestamp = pScrPriv->lastSetTime.milliseconds,
            .configTimestamp = pScrPriv->lastConfigTime.milliseconds,
            .nCrtcs = pScrPriv->numCrtcs,
            .nOutputs = pScrPriv->numOutputs,
            .nModes = num_modes,
            .nbytesNames = 0
        };


        for (i = 0; i < num_modes; i++)
            rep.nbytesNames += modes[i]->mode.nameLength;

        rep.length = (pScrPriv->numCrtcs +
                      pScrPriv->numOutputs +
                      num_modes * bytes_to_int32(SIZEOF(xRRModeInfo)) +
                      bytes_to_int32(rep.nbytesNames));

        extraLen = rep.length << 2;
        if (extraLen) {
            extra = calloc(1, extraLen);
            if (!extra) {
                free(modes);
                return BadAlloc;
            }
        }
        else
            extra = NULL;

        crtcs = (RRCrtc *) extra;
        outputs = (RROutput *) (crtcs + pScrPriv->numCrtcs);
        modeinfos = (xRRModeInfo *) (outputs + pScrPriv->numOutputs);
        names = (CARD8 *) (modeinfos + num_modes);

        if (pScrPriv->primaryOutput && pScrPriv->primaryOutput->crtc) {
            has_primary = 1;
            crtcs[0] = pScrPriv->primaryOutput->crtc->id;
            if (client->swapped)
                swapl(&crtcs[0]);
        }

        for (i = 0; i < pScrPriv->numCrtcs; i++) {
            if (has_primary &&
                pScrPriv->primaryOutput->crtc == pScrPriv->crtcs[i]) {
                has_primary = 0;
                continue;
            }
            crtcs[i + has_primary] = pScrPriv->crtcs[i]->id;
            if (client->swapped)
                swapl(&crtcs[i + has_primary]);
        }

        for (i = 0; i < pScrPriv->numOutputs; i++) {
            outputs[i] = pScrPriv->outputs[i]->id;
            if (client->swapped)
                swapl(&outputs[i]);
        }

        for (i = 0; i < num_modes; i++) {
            RRModePtr mode = modes[i];

            modeinfos[i] = mode->mode;
            if (client->swapped) {
                swapl(&modeinfos[i].id);
                swaps(&modeinfos[i].width);
                swaps(&modeinfos[i].height);
                swapl(&modeinfos[i].dotClock);
                swaps(&modeinfos[i].hSyncStart);
                swaps(&modeinfos[i].hSyncEnd);
                swaps(&modeinfos[i].hTotal);
                swaps(&modeinfos[i].hSkew);
                swaps(&modeinfos[i].vSyncStart);
                swaps(&modeinfos[i].vSyncEnd);
                swaps(&modeinfos[i].vTotal);
                swaps(&modeinfos[i].nameLength);
                swapl(&modeinfos[i].modeFlags);
            }
            memcpy(names, mode->name, mode->mode.nameLength);
            names += mode->mode.nameLength;
        }
        free(modes);
        assert(bytes_to_int32((char *) names - (char *) extra) == rep.length);
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swapl(&rep.configTimestamp);
        swaps(&rep.nCrtcs);
        swaps(&rep.nOutputs);
        swaps(&rep.nModes);
        swaps(&rep.nbytesNames);
    }
    WriteToClient(client, sizeof(xRRGetScreenResourcesReply), (char *) &rep);
    if (extraLen) {
        WriteToClient(client, extraLen, (char *) extra);
        free(extra);
    }
    return Success;
}

int
ProcRRGetScreenResources(ClientPtr client)
{
    return rrGetScreenResources(client, TRUE);
}

int
ProcRRGetScreenResourcesCurrent(ClientPtr client)
{
    return rrGetScreenResources(client, FALSE);
}

typedef struct _RR10Data {
    RRScreenSizePtr sizes;
    int nsize;
    int nrefresh;
    int size;
    CARD16 refresh;
} RR10DataRec, *RR10DataPtr;

/*
 * Convert 1.2 monitor data into 1.0 screen data
 */
static RR10DataPtr
RR10GetData(ScreenPtr pScreen, RROutputPtr output)
{
    RR10DataPtr data;
    RRScreenSizePtr size;
    int nmode = output->numModes + output->numUserModes;
    int o, os, l, r;
    RRScreenRatePtr refresh;
    CARD16 vRefresh;
    RRModePtr mode;
    Bool *used;

    /* Make sure there is plenty of space for any combination */
    data = malloc(sizeof(RR10DataRec) +
                  sizeof(RRScreenSize) * nmode +
                  sizeof(RRScreenRate) * nmode + sizeof(Bool) * nmode);
    if (!data)
        return NULL;
    size = (RRScreenSizePtr) (data + 1);
    refresh = (RRScreenRatePtr) (size + nmode);
    used = (Bool *) (refresh + nmode);
    memset(used, '\0', sizeof(Bool) * nmode);
    data->sizes = size;
    data->nsize = 0;
    data->nrefresh = 0;
    data->size = 0;
    data->refresh = 0;

    /*
     * find modes not yet listed
     */
    for (o = 0; o < output->numModes + output->numUserModes; o++) {
        if (used[o])
            continue;

        if (o < output->numModes)
            mode = output->modes[o];
        else
            mode = output->userModes[o - output->numModes];

        l = data->nsize;
        size[l].id = data->nsize;
        size[l].width = mode->mode.width;
        size[l].height = mode->mode.height;
        if (output->mmWidth && output->mmHeight) {
            size[l].mmWidth = output->mmWidth;
            size[l].mmHeight = output->mmHeight;
        }
        else {
            size[l].mmWidth = pScreen->mmWidth;
            size[l].mmHeight = pScreen->mmHeight;
        }
        size[l].nRates = 0;
        size[l].pRates = &refresh[data->nrefresh];
        data->nsize++;

        /*
         * Find all modes with matching size
         */
        for (os = o; os < output->numModes + output->numUserModes; os++) {
            if (os < output->numModes)
                mode = output->modes[os];
            else
                mode = output->userModes[os - output->numModes];
            if (mode->mode.width == size[l].width &&
                mode->mode.height == size[l].height) {
                vRefresh = RRVerticalRefresh(&mode->mode);
                used[os] = TRUE;

                for (r = 0; r < size[l].nRates; r++)
                    if (vRefresh == size[l].pRates[r].rate)
                        break;
                if (r == size[l].nRates) {
                    size[l].pRates[r].rate = vRefresh;
                    size[l].pRates[r].mode = mode;
                    size[l].nRates++;
                    data->nrefresh++;
                }
                if (mode == output->crtc->mode) {
                    data->size = l;
                    data->refresh = vRefresh;
                }
            }
        }
    }
    return data;
}

int
ProcRRGetScreenInfo(ClientPtr client)
{
    REQUEST(xRRGetScreenInfoReq);
    xRRGetScreenInfoReply rep;
    WindowPtr pWin;
    int rc;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    CARD8 *extra;
    unsigned long extraLen;
    RROutputPtr output;

    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    if (pScrPriv)
        if (!RRGetInfo(pScreen, TRUE))
            return BadAlloc;

    output = RRFirstOutput(pScreen);

    if (!pScrPriv || !output) {
        rep = (xRRGetScreenInfoReply) {
            .type = X_Reply,
            .setOfRotations = RR_Rotate_0,
            .sequenceNumber = client->sequence,
            .length = 0,
            .root = pWin->drawable.pScreen->root->drawable.id,
            .timestamp = currentTime.milliseconds,
            .configTimestamp = currentTime.milliseconds,
            .nSizes = 0,
            .sizeID = 0,
            .rotation = RR_Rotate_0,
            .rate = 0,
            .nrateEnts = 0
        };
        extra = 0;
        extraLen = 0;
    }
    else {
        int i, j;
        xScreenSizes *size;
        CARD16 *rates;
        CARD8 *data8;
        Bool has_rate = RRClientKnowsRates(client);
        RR10DataPtr pData;
        RRScreenSizePtr pSize;

        pData = RR10GetData(pScreen, output);
        if (!pData)
            return BadAlloc;

        rep = (xRRGetScreenInfoReply) {
            .type = X_Reply,
            .setOfRotations = output->crtc->rotations,
            .sequenceNumber = client->sequence,
            .length = 0,
            .root = pWin->drawable.pScreen->root->drawable.id,
            .timestamp = pScrPriv->lastSetTime.milliseconds,
            .configTimestamp = pScrPriv->lastConfigTime.milliseconds,
            .rotation = output->crtc->rotation,
            .nSizes = pData->nsize,
            .nrateEnts = pData->nrefresh + pData->nsize,
            .sizeID = pData->size,
            .rate = pData->refresh
        };

        extraLen = rep.nSizes * sizeof(xScreenSizes);
        if (has_rate)
            extraLen += rep.nrateEnts * sizeof(CARD16);

        if (extraLen) {
            extra = (CARD8 *) malloc(extraLen);
            if (!extra) {
                free(pData);
                return BadAlloc;
            }
        }
        else
            extra = NULL;

        /*
         * First comes the size information
         */
        size = (xScreenSizes *) extra;
        rates = (CARD16 *) (size + rep.nSizes);
        for (i = 0; i < pData->nsize; i++) {
            pSize = &pData->sizes[i];
            size->widthInPixels = pSize->width;
            size->heightInPixels = pSize->height;
            size->widthInMillimeters = pSize->mmWidth;
            size->heightInMillimeters = pSize->mmHeight;
            if (client->swapped) {
                swaps(&size->widthInPixels);
                swaps(&size->heightInPixels);
                swaps(&size->widthInMillimeters);
                swaps(&size->heightInMillimeters);
            }
            size++;
            if (has_rate) {
                *rates = pSize->nRates;
                if (client->swapped) {
                    swaps(rates);
                }
                rates++;
                for (j = 0; j < pSize->nRates; j++) {
                    *rates = pSize->pRates[j].rate;
                    if (client->swapped) {
                        swaps(rates);
                    }
                    rates++;
                }
            }
        }
        free(pData);

        data8 = (CARD8 *) rates;

        if (data8 - (CARD8 *) extra != extraLen)
            FatalError("RRGetScreenInfo bad extra len %ld != %ld\n",
                       (unsigned long) (data8 - (CARD8 *) extra), extraLen);
        rep.length = bytes_to_int32(extraLen);
    }
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swapl(&rep.configTimestamp);
        swaps(&rep.rotation);
        swaps(&rep.nSizes);
        swaps(&rep.sizeID);
        swaps(&rep.rate);
        swaps(&rep.nrateEnts);
    }
    WriteToClient(client, sizeof(xRRGetScreenInfoReply), &rep);
    if (extraLen) {
        WriteToClient(client, extraLen, extra);
        free(extra);
    }
    return Success;
}

int
ProcRRSetScreenConfig(ClientPtr client)
{
    REQUEST(xRRSetScreenConfigReq);
    xRRSetScreenConfigReply rep;
    DrawablePtr pDraw;
    int rc;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    TimeStamp time;
    int i;
    Rotation rotation;
    int rate;
    Bool has_rate;
    CARD8 status;
    RROutputPtr output;
    RRCrtcPtr crtc;
    RRModePtr mode;
    RR10DataPtr pData = NULL;
    RRScreenSizePtr pSize;
    int width, height;

    UpdateCurrentTime();

    if (RRClientKnowsRates(client)) {
        REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
        has_rate = TRUE;
    }
    else {
        REQUEST_SIZE_MATCH(xRR1_0SetScreenConfigReq);
        has_rate = FALSE;
    }

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixWriteAccess);
    if (rc != Success)
        return rc;

    pScreen = pDraw->pScreen;

    pScrPriv = rrGetScrPriv(pScreen);

    time = ClientTimeToServerTime(stuff->timestamp);

    if (!pScrPriv) {
        time = currentTime;
        status = RRSetConfigFailed;
        goto sendReply;
    }
    if (!RRGetInfo(pScreen, FALSE))
        return BadAlloc;

    output = RRFirstOutput(pScreen);
    if (!output) {
        time = currentTime;
        status = RRSetConfigFailed;
        goto sendReply;
    }

    crtc = output->crtc;

    /*
     * If the client's config timestamp is not the same as the last config
     * timestamp, then the config information isn't up-to-date and
     * can't even be validated.
     *
     * Note that the client only knows about the milliseconds part of the
     * timestamp, so using CompareTimeStamps here would cause randr to suddenly
     * stop working after several hours have passed (freedesktop bug #6502).
     */
    if (stuff->configTimestamp != pScrPriv->lastConfigTime.milliseconds) {
        status = RRSetConfigInvalidConfigTime;
        goto sendReply;
    }

    pData = RR10GetData(pScreen, output);
    if (!pData)
        return BadAlloc;

    if (stuff->sizeID >= pData->nsize) {
        /*
         * Invalid size ID
         */
        client->errorValue = stuff->sizeID;
        free(pData);
        return BadValue;
    }
    pSize = &pData->sizes[stuff->sizeID];

    /*
     * Validate requested rotation
     */
    rotation = (Rotation) stuff->rotation;

    /* test the rotation bits only! */
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_90:
    case RR_Rotate_180:
    case RR_Rotate_270:
        break;
    default:
        /*
         * Invalid rotation
         */
        client->errorValue = stuff->rotation;
        free(pData);
        return BadValue;
    }

    if ((~crtc->rotations) & rotation) {
        /*
         * requested rotation or reflection not supported by screen
         */
        client->errorValue = stuff->rotation;
        free(pData);
        return BadMatch;
    }

    /*
     * Validate requested refresh
     */
    if (has_rate)
        rate = (int) stuff->rate;
    else
        rate = 0;

    if (rate) {
        for (i = 0; i < pSize->nRates; i++) {
            if (pSize->pRates[i].rate == rate)
                break;
        }
        if (i == pSize->nRates) {
            /*
             * Invalid rate
             */
            client->errorValue = rate;
            free(pData);
            return BadValue;
        }
        mode = pSize->pRates[i].mode;
    }
    else
        mode = pSize->pRates[0].mode;

    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps(time, pScrPriv->lastSetTime) < 0) {
        status = RRSetConfigInvalidTime;
        goto sendReply;
    }

    /*
     * If the screen size is changing, adjust all of the other outputs
     * to fit the new size, mirroring as much as possible
     */
    width = mode->mode.width;
    height = mode->mode.height;
    if (width < pScrPriv->minWidth || pScrPriv->maxWidth < width) {
        client->errorValue = width;
        free(pData);
        return BadValue;
    }
    if (height < pScrPriv->minHeight || pScrPriv->maxHeight < height) {
        client->errorValue = height;
        free(pData);
        return BadValue;
    }

    if (rotation & (RR_Rotate_90 | RR_Rotate_270)) {
        width = mode->mode.height;
        height = mode->mode.width;
    }

    if (width != pScreen->width || height != pScreen->height) {
        int c;

        for (c = 0; c < pScrPriv->numCrtcs; c++) {
            if (!RRCrtcSet(pScrPriv->crtcs[c], NULL, 0, 0, RR_Rotate_0,
                           0, NULL)) {
                status = RRSetConfigFailed;
                /* XXX recover from failure */
                goto sendReply;
            }
        }
        if (!RRScreenSizeSet(pScreen, width, height,
                             pScreen->mmWidth, pScreen->mmHeight)) {
            status = RRSetConfigFailed;
            /* XXX recover from failure */
            goto sendReply;
        }
    }

    if (!RRCrtcSet(crtc, mode, 0, 0, stuff->rotation, 1, &output))
        status = RRSetConfigFailed;
    else {
        pScrPriv->lastSetTime = time;
        status = RRSetConfigSuccess;
    }

    /*
     * XXX Configure other crtcs to mirror as much as possible
     */

 sendReply:

    free(pData);

    rep = (xRRSetScreenConfigReply) {
        .type = X_Reply,
        .status = status,
        .sequenceNumber = client->sequence,
        .length = 0,

        .newTimestamp = pScrPriv->lastSetTime.milliseconds,
        .newConfigTimestamp = pScrPriv->lastConfigTime.milliseconds,
        .root = pDraw->pScreen->root->drawable.id,
        /* .subpixelOrder = ?? */
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.newTimestamp);
        swapl(&rep.newConfigTimestamp);
        swapl(&rep.root);
    }
    WriteToClient(client, sizeof(xRRSetScreenConfigReply), &rep);

    return Success;
}

static CARD16
RR10CurrentSizeID(ScreenPtr pScreen)
{
    CARD16 sizeID = 0xffff;
    RROutputPtr output = RRFirstOutput(pScreen);

    if (output) {
        RR10DataPtr data = RR10GetData(pScreen, output);

        if (data) {
            int i;

            for (i = 0; i < data->nsize; i++)
                if (data->sizes[i].width == pScreen->width &&
                    data->sizes[i].height == pScreen->height) {
                    sizeID = (CARD16) i;
                    break;
                }
            free(data);
        }
    }
    return sizeID;
}
