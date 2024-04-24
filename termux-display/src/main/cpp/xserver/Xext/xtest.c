/*

   Copyright 1992, 1998  The Open Group

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of The Open Group shall
   not be used in advertising or otherwise to promote the sale, use or
   other dealings in this Software without prior written authorization
   from The Open Group.

 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "windowstr.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "dixevents.h"
#include "sleepuntil.h"
#include "mi.h"
#include "xkbsrv.h"
#include "xkbstr.h"
#include <X11/extensions/xtestproto.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "mipointer.h"
#include "xserver-properties.h"
#include "exevents.h"
#include "eventstr.h"
#include "inpututils.h"

#include "extinit.h"

/* XTest events are sent during request processing and may be interrupted by
 * a SIGIO. We need a separate event list to avoid events overwriting each
 * other's memory.
 */
static InternalEvent *xtest_evlist;

/**
 * xtestpointer
 * is the virtual pointer for XTest. It is the first slave
 * device of the VCP.
 * xtestkeyboard
 * is the virtual keyboard for XTest. It is the first slave
 * device of the VCK
 *
 * Neither of these devices can be deleted.
 */
DeviceIntPtr xtestpointer, xtestkeyboard;

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

static int XTestSwapFakeInput(ClientPtr /* client */ ,
                              xReq *    /* req */
    );

static int
ProcXTestGetVersion(ClientPtr client)
{
    xXTestGetVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = XTestMajorVersion,
        .minorVersion = XTestMinorVersion
    };

    REQUEST_SIZE_MATCH(xXTestGetVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xXTestGetVersionReply), &rep);
    return Success;
}

static int
ProcXTestCompareCursor(ClientPtr client)
{
    REQUEST(xXTestCompareCursorReq);
    xXTestCompareCursorReply rep;
    WindowPtr pWin;
    CursorPtr pCursor;
    int rc;
    DeviceIntPtr ptr = PickPointer(client);

    REQUEST_SIZE_MATCH(xXTestCompareCursorReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (!ptr)
        return BadAccess;

    if (stuff->cursor == None)
        pCursor = NullCursor;
    else if (stuff->cursor == XTestCurrentCursor)
        pCursor = GetSpriteCursor(ptr);
    else {
        rc = dixLookupResourceByType((void **) &pCursor, stuff->cursor,
                                     RT_CURSOR, client, DixReadAccess);
        if (rc != Success) {
            client->errorValue = stuff->cursor;
            return rc;
        }
    }
    rep = (xXTestCompareCursorReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .same = (wCursor(pWin) == pCursor)
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
    }
    WriteToClient(client, sizeof(xXTestCompareCursorReply), &rep);
    return Success;
}

static int
ProcXTestFakeInput(ClientPtr client)
{
    REQUEST(xXTestFakeInputReq);
    int nev, n, type, rc;
    xEvent *ev;
    DeviceIntPtr dev = NULL;
    WindowPtr root;
    Bool extension = FALSE;
    ValuatorMask mask;
    int valuators[MAX_VALUATORS] = { 0 };
    int numValuators = 0;
    int firstValuator = 0;
    int nevents = 0;
    int i;
    int base = 0;
    int flags = 0;
    int need_ptr_update = 1;

    nev = (stuff->length << 2) - sizeof(xReq);
    if ((nev % sizeof(xEvent)) || !nev)
        return BadLength;
    nev /= sizeof(xEvent);
    UpdateCurrentTime();
    ev = (xEvent *) &((xReq *) stuff)[1];
    type = ev->u.u.type & 0177;

    if (type >= EXTENSION_EVENT_BASE) {
        extension = TRUE;

        /* check device */
        rc = dixLookupDevice(&dev, stuff->deviceid & 0177, client,
                             DixWriteAccess);
        if (rc != Success) {
            client->errorValue = stuff->deviceid & 0177;
            return rc;
        }

        /* check type */
        type -= DeviceValuator;
        switch (type) {
        case XI_DeviceKeyPress:
        case XI_DeviceKeyRelease:
            if (!dev->key) {
                client->errorValue = ev->u.u.type;
                return BadValue;
            }
            break;
        case XI_DeviceButtonPress:
        case XI_DeviceButtonRelease:
            if (!dev->button) {
                client->errorValue = ev->u.u.type;
                return BadValue;
            }
            break;
        case XI_DeviceMotionNotify:
            if (!dev->valuator) {
                client->errorValue = ev->u.u.type;
                return BadValue;
            }
            break;
        case XI_ProximityIn:
        case XI_ProximityOut:
            if (!dev->proximity) {
                client->errorValue = ev->u.u.type;
                return BadValue;
            }
            break;
        default:
            client->errorValue = ev->u.u.type;
            return BadValue;
        }

        /* check validity */
        if (nev == 1 && type == XI_DeviceMotionNotify)
            return BadLength;   /* DevMotion must be followed by DevValuator */

        if (type == XI_DeviceMotionNotify) {
            firstValuator = ((deviceValuator *) (ev + 1))->first_valuator;
            if (firstValuator > dev->valuator->numAxes) {
                client->errorValue = ev->u.u.type;
                return BadValue;
            }

            if (ev->u.u.detail == xFalse)
                flags |= POINTER_ABSOLUTE;
        }
        else {
            firstValuator = 0;
            flags |= POINTER_ABSOLUTE;
        }

        if (nev > 1 && !dev->valuator) {
            client->errorValue = firstValuator;
            return BadValue;
        }

        /* check validity of valuator events */
        base = firstValuator;
        for (n = 1; n < nev; n++) {
            deviceValuator *dv = (deviceValuator *) (ev + n);
            if (dv->type != DeviceValuator) {
                client->errorValue = dv->type;
                return BadValue;
            }
            if (dv->first_valuator != base) {
                client->errorValue = dv->first_valuator;
                return BadValue;
            }
            switch (dv->num_valuators) {
            case 6:
                valuators[base + 5] = dv->valuator5;
            case 5:
                valuators[base + 4] = dv->valuator4;
            case 4:
                valuators[base + 3] = dv->valuator3;
            case 3:
                valuators[base + 2] = dv->valuator2;
            case 2:
                valuators[base + 1] = dv->valuator1;
            case 1:
                valuators[base] = dv->valuator0;
                break;
            default:
                client->errorValue = dv->num_valuators;
                return BadValue;
            }

            base += dv->num_valuators;
            numValuators += dv->num_valuators;

            if (firstValuator + numValuators > dev->valuator->numAxes) {
                client->errorValue = dv->num_valuators;
                return BadValue;
            }
        }
        type = type - XI_DeviceKeyPress + KeyPress;

    }
    else {
        if (nev != 1)
            return BadLength;
        switch (type) {
        case KeyPress:
        case KeyRelease:
            dev = PickKeyboard(client);
            break;
        case ButtonPress:
        case ButtonRelease:
            dev = PickPointer(client);
            break;
        case MotionNotify:
            dev = PickPointer(client);
            valuators[0] = ev->u.keyButtonPointer.rootX;
            valuators[1] = ev->u.keyButtonPointer.rootY;
            numValuators = 2;
            firstValuator = 0;
            if (ev->u.u.detail == xFalse)
                flags = POINTER_ABSOLUTE | POINTER_DESKTOP;
            break;
        default:
            client->errorValue = ev->u.u.type;
            return BadValue;
        }

        /* Technically the protocol doesn't allow for BadAccess here but
         * this can only happen when all MDs are disabled.  */
        if (!dev)
            return BadAccess;

        dev = GetXTestDevice(dev);
    }


    /* If the event has a time set, wait for it to pass */
    if (ev->u.keyButtonPointer.time) {
        TimeStamp activateTime;
        CARD32 ms;

        activateTime = currentTime;
        ms = activateTime.milliseconds + ev->u.keyButtonPointer.time;
        if (ms < activateTime.milliseconds)
            activateTime.months++;
        activateTime.milliseconds = ms;
        ev->u.keyButtonPointer.time = 0;

        /* see mbuf.c:QueueDisplayRequest (from the deprecated Multibuffer
         * extension) for code similar to this */

        if (!ClientSleepUntil(client, &activateTime, NULL, NULL)) {
            return BadAlloc;
        }
        /* swap the request back so we can simply re-execute it */
        if (client->swapped) {
            (void) XTestSwapFakeInput(client, (xReq *) stuff);
            swaps(&stuff->length);
        }
        ResetCurrentRequest(client);
        client->sequence--;
        return Success;
    }

    switch (type) {
    case KeyPress:
    case KeyRelease:
        if (!dev->key)
            return BadDevice;

        if (ev->u.u.detail < dev->key->xkbInfo->desc->min_key_code ||
            ev->u.u.detail > dev->key->xkbInfo->desc->max_key_code) {
            client->errorValue = ev->u.u.detail;
            return BadValue;
        }

        need_ptr_update = 0;
        break;
    case MotionNotify:
        if (!dev->valuator)
            return BadDevice;

        if (!(extension || ev->u.keyButtonPointer.root == None)) {
            rc = dixLookupWindow(&root, ev->u.keyButtonPointer.root,
                                 client, DixGetAttrAccess);
            if (rc != Success)
                return rc;
            if (root->parent) {
                client->errorValue = ev->u.keyButtonPointer.root;
                return BadValue;
            }

            /* Add the root window's offset to the valuators */
            if ((flags & POINTER_ABSOLUTE) && firstValuator <= 1 && numValuators > 0) {
                if (firstValuator == 0)
                    valuators[0] += root->drawable.pScreen->x;
                if (firstValuator < 2 && firstValuator + numValuators > 1)
                    valuators[1 - firstValuator] += root->drawable.pScreen->y;
            }
        }
        if (ev->u.u.detail != xTrue && ev->u.u.detail != xFalse) {
            client->errorValue = ev->u.u.detail;
            return BadValue;
        }

        /* FIXME: Xinerama! */

        break;
    case ButtonPress:
    case ButtonRelease:
        if (!dev->button)
            return BadDevice;

        if (!ev->u.u.detail || ev->u.u.detail > dev->button->numButtons) {
            client->errorValue = ev->u.u.detail;
            return BadValue;
        }
        break;
    }
    if (screenIsSaved == SCREEN_SAVER_ON)
        dixSaveScreens(serverClient, SCREEN_SAVER_OFF, ScreenSaverReset);

    switch (type) {
    case MotionNotify:
        valuator_mask_set_range(&mask, firstValuator, numValuators, valuators);
        nevents = GetPointerEvents(xtest_evlist, dev, type, 0, flags, &mask);
        break;
    case ButtonPress:
    case ButtonRelease:
        valuator_mask_set_range(&mask, firstValuator, numValuators, valuators);
        nevents = GetPointerEvents(xtest_evlist, dev, type, ev->u.u.detail,
                                   flags, &mask);
        break;
    case KeyPress:
    case KeyRelease:
        nevents =
            GetKeyboardEvents(xtest_evlist, dev, type, ev->u.u.detail);
        break;
    }

    for (i = 0; i < nevents; i++)
        mieqProcessDeviceEvent(dev, &xtest_evlist[i], miPointerGetScreen(inputInfo.pointer));

    if (need_ptr_update)
        miPointerUpdateSprite(dev);
    return Success;
}

static int
ProcXTestGrabControl(ClientPtr client)
{
    REQUEST(xXTestGrabControlReq);

    REQUEST_SIZE_MATCH(xXTestGrabControlReq);
    if ((stuff->impervious != xTrue) && (stuff->impervious != xFalse)) {
        client->errorValue = stuff->impervious;
        return BadValue;
    }
    if (stuff->impervious)
        MakeClientGrabImpervious(client);
    else
        MakeClientGrabPervious(client);
    return Success;
}

static int
ProcXTestDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XTestGetVersion:
        return ProcXTestGetVersion(client);
    case X_XTestCompareCursor:
        return ProcXTestCompareCursor(client);
    case X_XTestFakeInput:
        return ProcXTestFakeInput(client);
    case X_XTestGrabControl:
        return ProcXTestGrabControl(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcXTestGetVersion(ClientPtr client)
{
    REQUEST(xXTestGetVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXTestGetVersionReq);
    swaps(&stuff->minorVersion);
    return ProcXTestGetVersion(client);
}

static int _X_COLD
SProcXTestCompareCursor(ClientPtr client)
{
    REQUEST(xXTestCompareCursorReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXTestCompareCursorReq);
    swapl(&stuff->window);
    swapl(&stuff->cursor);
    return ProcXTestCompareCursor(client);
}

static int _X_COLD
XTestSwapFakeInput(ClientPtr client, xReq * req)
{
    int nev;
    xEvent *ev;
    xEvent sev;
    EventSwapPtr proc;

    nev = ((req->length << 2) - sizeof(xReq)) / sizeof(xEvent);
    for (ev = (xEvent *) &req[1]; --nev >= 0; ev++) {
        int evtype = ev->u.u.type & 0177;
        /* Swap event */
        proc = EventSwapVector[evtype];
        /* no swapping proc; invalid event type? */
        if (!proc || proc == NotImplemented || evtype == GenericEvent) {
            client->errorValue = ev->u.u.type;
            return BadValue;
        }
        (*proc) (ev, &sev);
        *ev = sev;
    }
    return Success;
}

static int _X_COLD
SProcXTestFakeInput(ClientPtr client)
{
    int n;

    REQUEST(xReq);

    swaps(&stuff->length);
    n = XTestSwapFakeInput(client, stuff);
    if (n != Success)
        return n;
    return ProcXTestFakeInput(client);
}

static int _X_COLD
SProcXTestGrabControl(ClientPtr client)
{
    REQUEST(xXTestGrabControlReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXTestGrabControlReq);
    return ProcXTestGrabControl(client);
}

static int _X_COLD
SProcXTestDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XTestGetVersion:
        return SProcXTestGetVersion(client);
    case X_XTestCompareCursor:
        return SProcXTestCompareCursor(client);
    case X_XTestFakeInput:
        return SProcXTestFakeInput(client);
    case X_XTestGrabControl:
        return SProcXTestGrabControl(client);
    default:
        return BadRequest;
    }
}

/**
 * Allocate an virtual slave device for xtest events, this
 * is a slave device to inputInfo master devices
 */
void
InitXTestDevices(void)
{
    if (AllocXTestDevice(serverClient, "Virtual core",
                         &xtestpointer, &xtestkeyboard,
                         inputInfo.pointer, inputInfo.keyboard) != Success)
         FatalError("Failed to allocate XTest devices");

    if (ActivateDevice(xtestpointer, TRUE) != Success ||
        ActivateDevice(xtestkeyboard, TRUE) != Success)
        FatalError("Failed to activate XTest core devices.");
    if (!EnableDevice(xtestpointer, TRUE) || !EnableDevice(xtestkeyboard, TRUE))
        FatalError("Failed to enable XTest core devices.");

    AttachDevice(NULL, xtestpointer, inputInfo.pointer);

    AttachDevice(NULL, xtestkeyboard, inputInfo.keyboard);
}

/**
 * Don't allow changing the XTest property.
 */
static int
DeviceSetXTestProperty(DeviceIntPtr dev, Atom property,
                       XIPropertyValuePtr prop, BOOL checkonly)
{
    if (property == XIGetKnownProperty(XI_PROP_XTEST_DEVICE))
        return BadAccess;

    return Success;
}

/**
 * Allocate a device pair that is initialised as a slave
 * device with properties that identify the devices as belonging
 * to XTest subsystem.
 * This only creates the pair, Activate/Enable Device
 * still need to be called.
 */
int
AllocXTestDevice(ClientPtr client, const char *name,
                 DeviceIntPtr *ptr, DeviceIntPtr *keybd,
                 DeviceIntPtr master_ptr, DeviceIntPtr master_keybd)
{
    int retval;
    char *xtestname;
    char dummy = 1;

    if (asprintf(&xtestname, "%s XTEST", name) == -1)
        return BadAlloc;

    retval =
        AllocDevicePair(client, xtestname, ptr, keybd, CorePointerProc,
                        CoreKeyboardProc, FALSE);
    if (retval == Success) {
        (*ptr)->xtest_master_id = master_ptr->id;
        (*keybd)->xtest_master_id = master_keybd->id;

        XIChangeDeviceProperty(*ptr, XIGetKnownProperty(XI_PROP_XTEST_DEVICE),
                               XA_INTEGER, 8, PropModeReplace, 1, &dummy,
                               FALSE);
        XISetDevicePropertyDeletable(*ptr,
                                     XIGetKnownProperty(XI_PROP_XTEST_DEVICE),
                                     FALSE);
        XIRegisterPropertyHandler(*ptr, DeviceSetXTestProperty, NULL, NULL);
        XIChangeDeviceProperty(*keybd, XIGetKnownProperty(XI_PROP_XTEST_DEVICE),
                               XA_INTEGER, 8, PropModeReplace, 1, &dummy,
                               FALSE);
        XISetDevicePropertyDeletable(*keybd,
                                     XIGetKnownProperty(XI_PROP_XTEST_DEVICE),
                                     FALSE);
        XIRegisterPropertyHandler(*keybd, DeviceSetXTestProperty, NULL, NULL);
    }

    free(xtestname);

    return retval;
}

/**
 * If master is NULL, return TRUE if the given device is an xtest device or
 * FALSE otherwise.
 * If master is not NULL, return TRUE if the given device is this master's
 * xtest device.
 */
BOOL
IsXTestDevice(DeviceIntPtr dev, DeviceIntPtr master)
{
    if (IsMaster(dev))
        return FALSE;

    /* deviceid 0 is reserved for XIAllDevices, non-zero mid means XTest
     * device */
    if (master)
        return dev->xtest_master_id == master->id;

    return dev->xtest_master_id != 0;
}

/**
 * @return The X Test virtual device for the given master.
 */
DeviceIntPtr
GetXTestDevice(DeviceIntPtr master)
{
    DeviceIntPtr it;

    for (it = inputInfo.devices; it; it = it->next) {
        if (IsXTestDevice(it, master))
            return it;
    }

    /* This only happens if master is a slave device. don't do that */
    return NULL;
}

static void
XTestExtensionTearDown(ExtensionEntry * e)
{
    FreeEventList(xtest_evlist, GetMaximumEventsNum());
    xtest_evlist = NULL;
}

void
XTestExtensionInit(void)
{
    AddExtension(XTestExtensionName, 0, 0,
                 ProcXTestDispatch, SProcXTestDispatch,
                 XTestExtensionTearDown, StandardMinorOpcode);

    xtest_evlist = InitEventList(GetMaximumEventsNum());
}
