/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "stdio.h"
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#include <xkbsrv.h>
#include "xkbstr.h"
#include "extnsionst.h"
#include "xkb.h"

        /*
         * REQUEST SWAPPING
         */
static int _X_COLD
SProcXkbUseExtension(ClientPtr client)
{
    REQUEST(xkbUseExtensionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbUseExtensionReq);
    swaps(&stuff->wantedMajor);
    swaps(&stuff->wantedMinor);
    return ProcXkbUseExtension(client);
}

static int _X_COLD
SProcXkbSelectEvents(ClientPtr client)
{
    REQUEST(xkbSelectEventsReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSelectEventsReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->affectWhich);
    swaps(&stuff->clear);
    swaps(&stuff->selectAll);
    swaps(&stuff->affectMap);
    swaps(&stuff->map);
    if ((stuff->affectWhich & (~XkbMapNotifyMask)) != 0) {
        union {
            BOOL *b;
            CARD8 *c8;
            CARD16 *c16;
            CARD32 *c32;
        } from;
        register unsigned bit, ndx, maskLeft, dataLeft, size;

        from.c8 = (CARD8 *) &stuff[1];
        dataLeft = (client->req_len * 4) - SIZEOF(xkbSelectEventsReq);
        maskLeft = (stuff->affectWhich & (~XkbMapNotifyMask));
        for (ndx = 0, bit = 1; (maskLeft != 0); ndx++, bit <<= 1) {
            if (((bit & maskLeft) == 0) || (ndx == XkbMapNotify))
                continue;
            maskLeft &= ~bit;
            if ((stuff->selectAll & bit) || (stuff->clear & bit))
                continue;
            switch (ndx) {
            case XkbNewKeyboardNotify:
            case XkbStateNotify:
            case XkbNamesNotify:
            case XkbAccessXNotify:
            case XkbExtensionDeviceNotify:
                size = 2;
                break;
            case XkbControlsNotify:
            case XkbIndicatorStateNotify:
            case XkbIndicatorMapNotify:
                size = 4;
                break;
            case XkbBellNotify:
            case XkbActionMessage:
            case XkbCompatMapNotify:
                size = 1;
                break;
            default:
                client->errorValue = _XkbErrCode2(0x1, bit);
                return BadValue;
            }
            if (dataLeft < (size * 2))
                return BadLength;
            if (size == 2) {
                swaps(&from.c16[0]);
                swaps(&from.c16[1]);
            }
            else if (size == 4) {
                swapl(&from.c32[0]);
                swapl(&from.c32[1]);
            }
            else {
                size = 2;
            }
            from.c8 += (size * 2);
            dataLeft -= (size * 2);
        }
        if (dataLeft > 2) {
            ErrorF("[xkb] Extra data (%d bytes) after SelectEvents\n",
                   dataLeft);
            return BadLength;
        }
    }
    return ProcXkbSelectEvents(client);
}

static int _X_COLD
SProcXkbBell(ClientPtr client)
{
    REQUEST(xkbBellReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbBellReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->bellClass);
    swaps(&stuff->bellID);
    swapl(&stuff->name);
    swapl(&stuff->window);
    swaps(&stuff->pitch);
    swaps(&stuff->duration);
    return ProcXkbBell(client);
}

static int _X_COLD
SProcXkbGetState(ClientPtr client)
{
    REQUEST(xkbGetStateReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetStateReq);
    swaps(&stuff->deviceSpec);
    return ProcXkbGetState(client);
}

static int _X_COLD
SProcXkbLatchLockState(ClientPtr client)
{
    REQUEST(xkbLatchLockStateReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbLatchLockStateReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->groupLatch);
    return ProcXkbLatchLockState(client);
}

static int _X_COLD
SProcXkbGetControls(ClientPtr client)
{
    REQUEST(xkbGetControlsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetControlsReq);
    swaps(&stuff->deviceSpec);
    return ProcXkbGetControls(client);
}

static int _X_COLD
SProcXkbSetControls(ClientPtr client)
{
    REQUEST(xkbSetControlsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbSetControlsReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->affectInternalVMods);
    swaps(&stuff->internalVMods);
    swaps(&stuff->affectIgnoreLockVMods);
    swaps(&stuff->ignoreLockVMods);
    swaps(&stuff->axOptions);
    swapl(&stuff->affectEnabledCtrls);
    swapl(&stuff->enabledCtrls);
    swapl(&stuff->changeCtrls);
    swaps(&stuff->repeatDelay);
    swaps(&stuff->repeatInterval);
    swaps(&stuff->slowKeysDelay);
    swaps(&stuff->debounceDelay);
    swaps(&stuff->mkDelay);
    swaps(&stuff->mkInterval);
    swaps(&stuff->mkTimeToMax);
    swaps(&stuff->mkMaxSpeed);
    swaps(&stuff->mkCurve);
    swaps(&stuff->axTimeout);
    swapl(&stuff->axtCtrlsMask);
    swapl(&stuff->axtCtrlsValues);
    swaps(&stuff->axtOptsMask);
    swaps(&stuff->axtOptsValues);
    return ProcXkbSetControls(client);
}

static int _X_COLD
SProcXkbGetMap(ClientPtr client)
{
    REQUEST(xkbGetMapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->full);
    swaps(&stuff->partial);
    swaps(&stuff->virtualMods);
    return ProcXkbGetMap(client);
}

static int _X_COLD
SProcXkbSetMap(ClientPtr client)
{
    REQUEST(xkbSetMapReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->present);
    swaps(&stuff->flags);
    swaps(&stuff->totalSyms);
    swaps(&stuff->totalActs);
    swaps(&stuff->virtualMods);
    return ProcXkbSetMap(client);
}

static int _X_COLD
SProcXkbGetCompatMap(ClientPtr client)
{
    REQUEST(xkbGetCompatMapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetCompatMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->firstSI);
    swaps(&stuff->nSI);
    return ProcXkbGetCompatMap(client);
}

static int _X_COLD
SProcXkbSetCompatMap(ClientPtr client)
{
    REQUEST(xkbSetCompatMapReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetCompatMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->firstSI);
    swaps(&stuff->nSI);
    return ProcXkbSetCompatMap(client);
}

static int _X_COLD
SProcXkbGetIndicatorState(ClientPtr client)
{
    REQUEST(xkbGetIndicatorStateReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetIndicatorStateReq);
    swaps(&stuff->deviceSpec);
    return ProcXkbGetIndicatorState(client);
}

static int _X_COLD
SProcXkbGetIndicatorMap(ClientPtr client)
{
    REQUEST(xkbGetIndicatorMapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetIndicatorMapReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->which);
    return ProcXkbGetIndicatorMap(client);
}

static int _X_COLD
SProcXkbSetIndicatorMap(ClientPtr client)
{
    REQUEST(xkbSetIndicatorMapReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetIndicatorMapReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->which);
    return ProcXkbSetIndicatorMap(client);
}

static int _X_COLD
SProcXkbGetNamedIndicator(ClientPtr client)
{
    REQUEST(xkbGetNamedIndicatorReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetNamedIndicatorReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->ledClass);
    swaps(&stuff->ledID);
    swapl(&stuff->indicator);
    return ProcXkbGetNamedIndicator(client);
}

static int _X_COLD
SProcXkbSetNamedIndicator(ClientPtr client)
{
    REQUEST(xkbSetNamedIndicatorReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbSetNamedIndicatorReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->ledClass);
    swaps(&stuff->ledID);
    swapl(&stuff->indicator);
    swaps(&stuff->virtualMods);
    swapl(&stuff->ctrls);
    return ProcXkbSetNamedIndicator(client);
}

static int _X_COLD
SProcXkbGetNames(ClientPtr client)
{
    REQUEST(xkbGetNamesReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetNamesReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->which);
    return ProcXkbGetNames(client);
}

static int _X_COLD
SProcXkbSetNames(ClientPtr client)
{
    REQUEST(xkbSetNamesReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetNamesReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->virtualMods);
    swapl(&stuff->which);
    swapl(&stuff->indicators);
    swaps(&stuff->totalKTLevelNames);
    return ProcXkbSetNames(client);
}

static int _X_COLD
SProcXkbGetGeometry(ClientPtr client)
{
    REQUEST(xkbGetGeometryReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetGeometryReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->name);
    return ProcXkbGetGeometry(client);
}

static int _X_COLD
SProcXkbSetGeometry(ClientPtr client)
{
    REQUEST(xkbSetGeometryReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetGeometryReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->name);
    swaps(&stuff->widthMM);
    swaps(&stuff->heightMM);
    swaps(&stuff->nProperties);
    swaps(&stuff->nColors);
    swaps(&stuff->nDoodads);
    swaps(&stuff->nKeyAliases);
    return ProcXkbSetGeometry(client);
}

static int _X_COLD
SProcXkbPerClientFlags(ClientPtr client)
{
    REQUEST(xkbPerClientFlagsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbPerClientFlagsReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->change);
    swapl(&stuff->value);
    swapl(&stuff->ctrlsToChange);
    swapl(&stuff->autoCtrls);
    swapl(&stuff->autoCtrlValues);
    return ProcXkbPerClientFlags(client);
}

static int _X_COLD
SProcXkbListComponents(ClientPtr client)
{
    REQUEST(xkbListComponentsReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbListComponentsReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->maxNames);
    return ProcXkbListComponents(client);
}

static int _X_COLD
SProcXkbGetKbdByName(ClientPtr client)
{
    REQUEST(xkbGetKbdByNameReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbGetKbdByNameReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->want);
    swaps(&stuff->need);
    return ProcXkbGetKbdByName(client);
}

static int _X_COLD
SProcXkbGetDeviceInfo(ClientPtr client)
{
    REQUEST(xkbGetDeviceInfoReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xkbGetDeviceInfoReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->wanted);
    swaps(&stuff->ledClass);
    swaps(&stuff->ledID);
    return ProcXkbGetDeviceInfo(client);
}

static int _X_COLD
SProcXkbSetDeviceInfo(ClientPtr client)
{
    REQUEST(xkbSetDeviceInfoReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetDeviceInfoReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->change);
    swaps(&stuff->nDeviceLedFBs);
    return ProcXkbSetDeviceInfo(client);
}

static int _X_COLD
SProcXkbSetDebuggingFlags(ClientPtr client)
{
    REQUEST(xkbSetDebuggingFlagsReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xkbSetDebuggingFlagsReq);
    swapl(&stuff->affectFlags);
    swapl(&stuff->flags);
    swapl(&stuff->affectCtrls);
    swapl(&stuff->ctrls);
    swaps(&stuff->msgLength);
    return ProcXkbSetDebuggingFlags(client);
}

int _X_COLD
SProcXkbDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_kbUseExtension:
        return SProcXkbUseExtension(client);
    case X_kbSelectEvents:
        return SProcXkbSelectEvents(client);
    case X_kbBell:
        return SProcXkbBell(client);
    case X_kbGetState:
        return SProcXkbGetState(client);
    case X_kbLatchLockState:
        return SProcXkbLatchLockState(client);
    case X_kbGetControls:
        return SProcXkbGetControls(client);
    case X_kbSetControls:
        return SProcXkbSetControls(client);
    case X_kbGetMap:
        return SProcXkbGetMap(client);
    case X_kbSetMap:
        return SProcXkbSetMap(client);
    case X_kbGetCompatMap:
        return SProcXkbGetCompatMap(client);
    case X_kbSetCompatMap:
        return SProcXkbSetCompatMap(client);
    case X_kbGetIndicatorState:
        return SProcXkbGetIndicatorState(client);
    case X_kbGetIndicatorMap:
        return SProcXkbGetIndicatorMap(client);
    case X_kbSetIndicatorMap:
        return SProcXkbSetIndicatorMap(client);
    case X_kbGetNamedIndicator:
        return SProcXkbGetNamedIndicator(client);
    case X_kbSetNamedIndicator:
        return SProcXkbSetNamedIndicator(client);
    case X_kbGetNames:
        return SProcXkbGetNames(client);
    case X_kbSetNames:
        return SProcXkbSetNames(client);
    case X_kbGetGeometry:
        return SProcXkbGetGeometry(client);
    case X_kbSetGeometry:
        return SProcXkbSetGeometry(client);
    case X_kbPerClientFlags:
        return SProcXkbPerClientFlags(client);
    case X_kbListComponents:
        return SProcXkbListComponents(client);
    case X_kbGetKbdByName:
        return SProcXkbGetKbdByName(client);
    case X_kbGetDeviceInfo:
        return SProcXkbGetDeviceInfo(client);
    case X_kbSetDeviceInfo:
        return SProcXkbSetDeviceInfo(client);
    case X_kbSetDebuggingFlags:
        return SProcXkbSetDebuggingFlags(client);
    default:
        return BadRequest;
    }
}
