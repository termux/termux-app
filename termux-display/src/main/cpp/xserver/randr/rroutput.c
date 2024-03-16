/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2008 Red Hat, Inc.
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
#include <X11/Xatom.h>

RESTYPE RROutputType;

/*
 * Notify the output of some change
 */
void
RROutputChanged(RROutputPtr output, Bool configChanged)
{
    /* set changed bits on the primary screen only */
    ScreenPtr pScreen = output->pScreen;
    rrScrPrivPtr primarysp;

    output->changed = TRUE;
    if (!pScreen)
        return;

    if (pScreen->isGPU) {
        ScreenPtr primary = pScreen->current_primary;
        if (!primary)
            return;
        primarysp = rrGetScrPriv(primary);
    }
    else {
        primarysp = rrGetScrPriv(pScreen);
    }

    RRSetChanged(pScreen);
    if (configChanged)
        primarysp->configChanged = TRUE;
}

/*
 * Create an output
 */

RROutputPtr
RROutputCreate(ScreenPtr pScreen,
               const char *name, int nameLength, void *devPrivate)
{
    RROutputPtr output;
    RROutputPtr *outputs;
    rrScrPrivPtr pScrPriv;
    Atom nonDesktopAtom;

    if (!RRInit())
        return NULL;

    pScrPriv = rrGetScrPriv(pScreen);

    outputs = reallocarray(pScrPriv->outputs,
                           pScrPriv->numOutputs + 1, sizeof(RROutputPtr));
    if (!outputs)
        return NULL;

    pScrPriv->outputs = outputs;

    output = malloc(sizeof(RROutputRec) + nameLength + 1);
    if (!output)
        return NULL;
    output->id = FakeClientID(0);
    output->pScreen = pScreen;
    output->name = (char *) (output + 1);
    output->nameLength = nameLength;
    memcpy(output->name, name, nameLength);
    output->name[nameLength] = '\0';
    output->connection = RR_UnknownConnection;
    output->subpixelOrder = SubPixelUnknown;
    output->mmWidth = 0;
    output->mmHeight = 0;
    output->crtc = NULL;
    output->numCrtcs = 0;
    output->crtcs = NULL;
    output->numClones = 0;
    output->clones = NULL;
    output->numModes = 0;
    output->numPreferred = 0;
    output->modes = NULL;
    output->numUserModes = 0;
    output->userModes = NULL;
    output->properties = NULL;
    output->pendingProperties = FALSE;
    output->changed = FALSE;
    output->nonDesktop = FALSE;
    output->devPrivate = devPrivate;

    if (!AddResource(output->id, RROutputType, (void *) output))
        return NULL;

    pScrPriv->outputs[pScrPriv->numOutputs++] = output;

    nonDesktopAtom = MakeAtom(RR_PROPERTY_NON_DESKTOP, strlen(RR_PROPERTY_NON_DESKTOP), TRUE);
    if (nonDesktopAtom != BAD_RESOURCE) {
        static const INT32 values[2] = { 0, 1 };
        (void) RRConfigureOutputProperty(output, nonDesktopAtom, FALSE, FALSE, FALSE,
                                            2, values);
    }
    RROutputSetNonDesktop(output, FALSE);
    RRResourcesChanged(pScreen);

    return output;
}

/*
 * Notify extension that output parameters have been changed
 */
Bool
RROutputSetClones(RROutputPtr output, RROutputPtr * clones, int numClones)
{
    RROutputPtr *newClones;
    int i;

    if (numClones == output->numClones) {
        for (i = 0; i < numClones; i++)
            if (output->clones[i] != clones[i])
                break;
        if (i == numClones)
            return TRUE;
    }
    if (numClones) {
        newClones = xallocarray(numClones, sizeof(RROutputPtr));
        if (!newClones)
            return FALSE;
    }
    else
        newClones = NULL;
    free(output->clones);
    memcpy(newClones, clones, numClones * sizeof(RROutputPtr));
    output->clones = newClones;
    output->numClones = numClones;
    RROutputChanged(output, TRUE);
    return TRUE;
}

Bool
RROutputSetModes(RROutputPtr output,
                 RRModePtr * modes, int numModes, int numPreferred)
{
    RRModePtr *newModes;
    int i;

    if (numModes == output->numModes && numPreferred == output->numPreferred) {
        for (i = 0; i < numModes; i++)
            if (output->modes[i] != modes[i])
                break;
        if (i == numModes) {
            for (i = 0; i < numModes; i++)
                RRModeDestroy(modes[i]);
            return TRUE;
        }
    }

    if (numModes) {
        newModes = xallocarray(numModes, sizeof(RRModePtr));
        if (!newModes)
            return FALSE;
    }
    else
        newModes = NULL;
    if (output->modes) {
        for (i = 0; i < output->numModes; i++)
            RRModeDestroy(output->modes[i]);
        free(output->modes);
    }
    memcpy(newModes, modes, numModes * sizeof(RRModePtr));
    output->modes = newModes;
    output->numModes = numModes;
    output->numPreferred = numPreferred;
    RROutputChanged(output, TRUE);
    return TRUE;
}

int
RROutputAddUserMode(RROutputPtr output, RRModePtr mode)
{
    int m;
    ScreenPtr pScreen = output->pScreen;

    rrScrPriv(pScreen);
    RRModePtr *newModes;

    /* Check to see if this mode is already listed for this output */
    for (m = 0; m < output->numModes + output->numUserModes; m++) {
        RRModePtr e = (m < output->numModes ?
                       output->modes[m] :
                       output->userModes[m - output->numModes]);
        if (mode == e)
            return Success;
    }

    /* Check with the DDX to see if this mode is OK */
    if (pScrPriv->rrOutputValidateMode)
        if (!pScrPriv->rrOutputValidateMode(pScreen, output, mode))
            return BadMatch;

    if (output->userModes)
        newModes = reallocarray(output->userModes,
                                output->numUserModes + 1, sizeof(RRModePtr));
    else
        newModes = malloc(sizeof(RRModePtr));
    if (!newModes)
        return BadAlloc;

    output->userModes = newModes;
    output->userModes[output->numUserModes++] = mode;
    ++mode->refcnt;
    RROutputChanged(output, TRUE);
    RRTellChanged(pScreen);
    return Success;
}

int
RROutputDeleteUserMode(RROutputPtr output, RRModePtr mode)
{
    int m;

    /* Find this mode in the user mode list */
    for (m = 0; m < output->numUserModes; m++) {
        RRModePtr e = output->userModes[m];

        if (mode == e)
            break;
    }
    /* Not there, access error */
    if (m == output->numUserModes)
        return BadAccess;

    /* make sure the mode isn't active for this output */
    if (output->crtc && output->crtc->mode == mode)
        return BadMatch;

    memmove(output->userModes + m, output->userModes + m + 1,
            (output->numUserModes - m - 1) * sizeof(RRModePtr));
    output->numUserModes--;
    RRModeDestroy(mode);
    return Success;
}

Bool
RROutputSetCrtcs(RROutputPtr output, RRCrtcPtr * crtcs, int numCrtcs)
{
    RRCrtcPtr *newCrtcs;
    int i;

    if (numCrtcs == output->numCrtcs) {
        for (i = 0; i < numCrtcs; i++)
            if (output->crtcs[i] != crtcs[i])
                break;
        if (i == numCrtcs)
            return TRUE;
    }
    if (numCrtcs) {
        newCrtcs = xallocarray(numCrtcs, sizeof(RRCrtcPtr));
        if (!newCrtcs)
            return FALSE;
    }
    else
        newCrtcs = NULL;
    free(output->crtcs);
    memcpy(newCrtcs, crtcs, numCrtcs * sizeof(RRCrtcPtr));
    output->crtcs = newCrtcs;
    output->numCrtcs = numCrtcs;
    RROutputChanged(output, TRUE);
    return TRUE;
}

Bool
RROutputSetConnection(RROutputPtr output, CARD8 connection)
{
    if (output->connection == connection)
        return TRUE;
    output->connection = connection;
    RROutputChanged(output, TRUE);
    return TRUE;
}

Bool
RROutputSetSubpixelOrder(RROutputPtr output, int subpixelOrder)
{
    if (output->subpixelOrder == subpixelOrder)
        return TRUE;

    output->subpixelOrder = subpixelOrder;
    RROutputChanged(output, FALSE);
    return TRUE;
}

Bool
RROutputSetPhysicalSize(RROutputPtr output, int mmWidth, int mmHeight)
{
    if (output->mmWidth == mmWidth && output->mmHeight == mmHeight)
        return TRUE;
    output->mmWidth = mmWidth;
    output->mmHeight = mmHeight;
    RROutputChanged(output, FALSE);
    return TRUE;
}

Bool
RROutputSetNonDesktop(RROutputPtr output, Bool nonDesktop)
{
    const char *nonDesktopStr = RR_PROPERTY_NON_DESKTOP;
    Atom nonDesktopProp = MakeAtom(nonDesktopStr, strlen(nonDesktopStr), TRUE);
    uint32_t value = nonDesktop ? 1 : 0;

    if (nonDesktopProp == None || nonDesktopProp == BAD_RESOURCE)
        return FALSE;

    return RRChangeOutputProperty(output, nonDesktopProp, XA_INTEGER, 32,
                                  PropModeReplace, 1, &value, TRUE, FALSE) == Success;
}

void
RRDeliverOutputEvent(ClientPtr client, WindowPtr pWin, RROutputPtr output)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    rrScrPriv(pScreen);
    RRCrtcPtr crtc = output->crtc;
    RRModePtr mode = crtc ? crtc->mode : NULL;

    xRROutputChangeNotifyEvent oe = {
        .type = RRNotify + RREventBase,
        .subCode = RRNotify_OutputChange,
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .configTimestamp = pScrPriv->lastConfigTime.milliseconds,
        .window = pWin->drawable.id,
        .output = output->id,
        .crtc = crtc ? crtc->id : None,
        .mode = mode ? mode->mode.id : None,
        .rotation = crtc ? crtc->rotation : RR_Rotate_0,
        .connection = output->nonDesktop ? RR_Disconnected : output->connection,
        .subpixelOrder = output->subpixelOrder
    };
    WriteEventsToClient(client, 1, (xEvent *) &oe);
}

/*
 * Destroy a Output at shutdown
 */
void
RROutputDestroy(RROutputPtr output)
{
    FreeResource(output->id, 0);
}

static int
RROutputDestroyResource(void *value, XID pid)
{
    RROutputPtr output = (RROutputPtr) value;
    ScreenPtr pScreen = output->pScreen;
    int m;

    if (pScreen) {
        rrScrPriv(pScreen);
        int i;
        RRLeasePtr lease, next;

        xorg_list_for_each_entry_safe(lease, next, &pScrPriv->leases, list) {
            int o;
            for (o = 0; o < lease->numOutputs; o++) {
                if (lease->outputs[o] == output) {
                    RRTerminateLease(lease);
                    break;
                }
            }
        }

        if (pScrPriv->primaryOutput == output)
            pScrPriv->primaryOutput = NULL;

        for (i = 0; i < pScrPriv->numOutputs; i++) {
            if (pScrPriv->outputs[i] == output) {
                memmove(pScrPriv->outputs + i, pScrPriv->outputs + i + 1,
                        (pScrPriv->numOutputs - (i + 1)) * sizeof(RROutputPtr));
                --pScrPriv->numOutputs;
                break;
            }
        }

        RRResourcesChanged(pScreen);
    }
    if (output->modes) {
        for (m = 0; m < output->numModes; m++)
            RRModeDestroy(output->modes[m]);
        free(output->modes);
    }

    for (m = 0; m < output->numUserModes; m++)
        RRModeDestroy(output->userModes[m]);
    free(output->userModes);

    free(output->crtcs);
    free(output->clones);
    RRDeleteAllOutputProperties(output);
    free(output);
    return 1;
}

/*
 * Initialize output type
 */
Bool
RROutputInit(void)
{
    RROutputType = CreateNewResourceType(RROutputDestroyResource, "OUTPUT");
    if (!RROutputType)
        return FALSE;

    return TRUE;
}

/*
 * Initialize output type error value
 */
void
RROutputInitErrorValue(void)
{
    SetResourceTypeErrorValue(RROutputType, RRErrorBase + BadRROutput);
}

#define OutputInfoExtra	(SIZEOF(xRRGetOutputInfoReply) - 32)

int
ProcRRGetOutputInfo(ClientPtr client)
{
    REQUEST(xRRGetOutputInfoReq);
    xRRGetOutputInfoReply rep;
    RROutputPtr output;
    CARD8 *extra;
    unsigned long extraLen;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    RRCrtc *crtcs;
    RRMode *modes;
    RROutput *clones;
    char *name;
    int i;
    Bool leased;

    REQUEST_SIZE_MATCH(xRRGetOutputInfoReq);
    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    leased = RROutputIsLeased(output);

    pScreen = output->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    if (leased) {
        rep = (xRRGetOutputInfoReply) {
            .type = X_Reply,
            .status = RRSetConfigSuccess,
            .sequenceNumber = client->sequence,
            .length = bytes_to_int32(OutputInfoExtra),
            .timestamp = pScrPriv->lastSetTime.milliseconds,
            .crtc = None,
            .mmWidth = 0,
            .mmHeight = 0,
            .connection = RR_Disconnected,
            .subpixelOrder = SubPixelUnknown,
            .nCrtcs = 0,
            .nModes = 0,
            .nPreferred = 0,
            .nClones = 0,
            .nameLength = output->nameLength
        };
        extraLen = bytes_to_int32(rep.nameLength) << 2;
        if (extraLen) {
            rep.length += bytes_to_int32(extraLen);
            extra = calloc(1, extraLen);
            if (!extra)
                return BadAlloc;
        }
        else
            extra = NULL;

        name = (char *) extra;
    } else {
        rep = (xRRGetOutputInfoReply) {
            .type = X_Reply,
            .status = RRSetConfigSuccess,
            .sequenceNumber = client->sequence,
            .length = bytes_to_int32(OutputInfoExtra),
            .timestamp = pScrPriv->lastSetTime.milliseconds,
            .crtc = output->crtc ? output->crtc->id : None,
            .mmWidth = output->mmWidth,
            .mmHeight = output->mmHeight,
            .connection = output->nonDesktop ? RR_Disconnected : output->connection,
            .subpixelOrder = output->subpixelOrder,
            .nCrtcs = output->numCrtcs,
            .nModes = output->numModes + output->numUserModes,
            .nPreferred = output->numPreferred,
            .nClones = output->numClones,
            .nameLength = output->nameLength
        };
        extraLen = ((output->numCrtcs +
                     output->numModes + output->numUserModes +
                     output->numClones + bytes_to_int32(rep.nameLength)) << 2);

        if (extraLen) {
            rep.length += bytes_to_int32(extraLen);
            extra = calloc(1, extraLen);
            if (!extra)
                return BadAlloc;
        }
        else
            extra = NULL;

        crtcs = (RRCrtc *) extra;
        modes = (RRMode *) (crtcs + output->numCrtcs);
        clones = (RROutput *) (modes + output->numModes + output->numUserModes);
        name = (char *) (clones + output->numClones);

        for (i = 0; i < output->numCrtcs; i++) {
            crtcs[i] = output->crtcs[i]->id;
            if (client->swapped)
                swapl(&crtcs[i]);
        }
        for (i = 0; i < output->numModes + output->numUserModes; i++) {
            if (i < output->numModes)
                modes[i] = output->modes[i]->mode.id;
            else
                modes[i] = output->userModes[i - output->numModes]->mode.id;
            if (client->swapped)
                swapl(&modes[i]);
        }
        for (i = 0; i < output->numClones; i++) {
            clones[i] = output->clones[i]->id;
            if (client->swapped)
                swapl(&clones[i]);
        }
    }
    memcpy(name, output->name, output->nameLength);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swapl(&rep.crtc);
        swapl(&rep.mmWidth);
        swapl(&rep.mmHeight);
        swaps(&rep.nCrtcs);
        swaps(&rep.nModes);
        swaps(&rep.nPreferred);
        swaps(&rep.nClones);
        swaps(&rep.nameLength);
    }
    WriteToClient(client, sizeof(xRRGetOutputInfoReply), &rep);
    if (extraLen) {
        WriteToClient(client, extraLen, extra);
        free(extra);
    }

    return Success;
}

static void
RRSetPrimaryOutput(ScreenPtr pScreen, rrScrPrivPtr pScrPriv, RROutputPtr output)
{
    if (pScrPriv->primaryOutput == output)
        return;

    /* clear the old primary */
    if (pScrPriv->primaryOutput) {
        RROutputChanged(pScrPriv->primaryOutput, 0);
        pScrPriv->primaryOutput = NULL;
    }

    /* set the new primary */
    if (output) {
        pScrPriv->primaryOutput = output;
        RROutputChanged(output, 0);
    }

    pScrPriv->layoutChanged = TRUE;

    RRTellChanged(pScreen);
}

int
ProcRRSetOutputPrimary(ClientPtr client)
{
    REQUEST(xRRSetOutputPrimaryReq);
    RROutputPtr output = NULL;
    WindowPtr pWin;
    rrScrPrivPtr pScrPriv;
    int ret;
    ScreenPtr secondary;

    REQUEST_SIZE_MATCH(xRRSetOutputPrimaryReq);

    ret = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    if (stuff->output) {
        VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

        if (RROutputIsLeased(output))
            return BadAccess;

        if (!output->pScreen->isGPU && output->pScreen != pWin->drawable.pScreen) {
            client->errorValue = stuff->window;
            return BadMatch;
        }
        if (output->pScreen->isGPU && output->pScreen->current_primary != pWin->drawable.pScreen) {
            client->errorValue = stuff->window;
            return BadMatch;
        }
    }

    pScrPriv = rrGetScrPriv(pWin->drawable.pScreen);
    if (pScrPriv)
    {
        RRSetPrimaryOutput(pWin->drawable.pScreen, pScrPriv, output);

        xorg_list_for_each_entry(secondary,
                                 &pWin->drawable.pScreen->secondary_list,
                                 secondary_head) {
            if (secondary->is_output_secondary)
                RRSetPrimaryOutput(secondary, rrGetScrPriv(secondary), output);
        }
    }

    return Success;
}

int
ProcRRGetOutputPrimary(ClientPtr client)
{
    REQUEST(xRRGetOutputPrimaryReq);
    WindowPtr pWin;
    rrScrPrivPtr pScrPriv;
    xRRGetOutputPrimaryReply rep;
    RROutputPtr primary = NULL;
    int rc;

    REQUEST_SIZE_MATCH(xRRGetOutputPrimaryReq);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScrPriv = rrGetScrPriv(pWin->drawable.pScreen);
    if (pScrPriv)
        primary = pScrPriv->primaryOutput;

    rep = (xRRGetOutputPrimaryReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .output = primary ? primary->id : None
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.output);
    }

    WriteToClient(client, sizeof(xRRGetOutputPrimaryReply), &rep);

    return Success;
}
