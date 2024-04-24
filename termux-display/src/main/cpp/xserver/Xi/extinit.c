/************************************************************

Copyright 1989, 1998  The Open Group

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

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/********************************************************************
 *
 *  Dispatch routines and initialization routines for the X input extension.
 *
 */
#define	 NUMTYPES 15

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include "gcstruct.h"           /* pointer for extnsionst.h */
#include "extnsionst.h"         /* extension entry   */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/geproto.h>
#include "geext.h"              /* extension interfaces for ge */

#include "dixevents.h"
#include "exevents.h"
#include "extinit.h"
#include "exglobals.h"
#include "swaprep.h"
#include "privates.h"
#include "protocol-versions.h"

/* modules local to Xi */
#include "allowev.h"
#include "chgdctl.h"
#include "chgfctl.h"
#include "chgkbd.h"
#include "chgprop.h"
#include "chgptr.h"
#include "closedev.h"
#include "devbell.h"
#include "getbmap.h"
#include "getdctl.h"
#include "getfctl.h"
#include "getfocus.h"
#include "getkmap.h"
#include "getmmap.h"
#include "getprop.h"
#include "getselev.h"
#include "getvers.h"
#include "grabdev.h"
#include "grabdevb.h"
#include "grabdevk.h"
#include "gtmotion.h"
#include "listdev.h"
#include "opendev.h"
#include "queryst.h"
#include "selectev.h"
#include "sendexev.h"
#include "chgkmap.h"
#include "setbmap.h"
#include "setdval.h"
#include "setfocus.h"
#include "setmmap.h"
#include "setmode.h"
#include "ungrdev.h"
#include "ungrdevb.h"
#include "ungrdevk.h"
#include "xiallowev.h"
#include "xiselectev.h"
#include "xigrabdev.h"
#include "xipassivegrab.h"
#include "xisetdevfocus.h"
#include "xiproperty.h"
#include "xichangecursor.h"
#include "xichangehierarchy.h"
#include "xigetclientpointer.h"
#include "xiquerydevice.h"
#include "xiquerypointer.h"
#include "xiqueryversion.h"
#include "xisetclientpointer.h"
#include "xiwarppointer.h"
#include "xibarriers.h"

/* Masks for XI events have to be aligned with core event (partially anyway).
 * If DeviceButtonMotionMask is != ButtonMotionMask, event delivery
 * breaks down. The device needs the dev->button->motionMask. If DBMM is
 * the same as BMM, we can ensure that both core and device events can be
 * delivered, without the need for extra structures in the DeviceIntRec. */
const Mask DeviceProximityMask = (1L << 4);
const Mask DeviceStateNotifyMask = (1L << 5);
const Mask DevicePointerMotionHintMask = PointerMotionHintMask;
const Mask DeviceButton1MotionMask = Button1MotionMask;
const Mask DeviceButton2MotionMask = Button2MotionMask;
const Mask DeviceButton3MotionMask = Button3MotionMask;
const Mask DeviceButton4MotionMask = Button4MotionMask;
const Mask DeviceButton5MotionMask = Button5MotionMask;
const Mask DeviceButtonMotionMask = ButtonMotionMask;
const Mask DeviceFocusChangeMask = (1L << 14);
const Mask DeviceMappingNotifyMask = (1L << 15);
const Mask ChangeDeviceNotifyMask = (1L << 16);
const Mask DeviceButtonGrabMask = (1L << 17);
const Mask DeviceOwnerGrabButtonMask = (1L << 17);
const Mask DevicePresenceNotifyMask = (1L << 18);
const Mask DevicePropertyNotifyMask = (1L << 19);
const Mask XIAllMasks = (1L << 20) - 1;

int ExtEventIndex;

static struct dev_type {
    Atom type;
    const char *name;
} dev_type[] = {
    {0, XI_KEYBOARD},
    {0, XI_MOUSE},
    {0, XI_TABLET},
    {0, XI_TOUCHSCREEN},
    {0, XI_TOUCHPAD},
    {0, XI_BARCODE},
    {0, XI_BUTTONBOX},
    {0, XI_KNOB_BOX},
    {0, XI_ONE_KNOB},
    {0, XI_NINE_KNOB},
    {0, XI_TRACKBALL},
    {0, XI_QUADRATURE},
    {0, XI_ID_MODULE},
    {0, XI_SPACEBALL},
    {0, XI_DATAGLOVE},
    {0, XI_EYETRACKER},
    {0, XI_CURSORKEYS},
    {0, XI_FOOTMOUSE}
};

CARD8 event_base[numInputClasses];
XExtEventInfo EventInfo[32];

static DeviceIntRec xi_all_devices;
static DeviceIntRec xi_all_master_devices;

/**
 * Dispatch vector. Functions defined in here will be called when the matching
 * request arrives.
 */
static int (*ProcIVector[]) (ClientPtr) = {
    NULL,                       /*  0 */
        ProcXGetExtensionVersion,       /*  1 */
        ProcXListInputDevices,  /*  2 */
        ProcXOpenDevice,        /*  3 */
        ProcXCloseDevice,       /*  4 */
        ProcXSetDeviceMode,     /*  5 */
        ProcXSelectExtensionEvent,      /*  6 */
        ProcXGetSelectedExtensionEvents,        /*  7 */
        ProcXChangeDeviceDontPropagateList,     /*  8 */
        ProcXGetDeviceDontPropagateList,        /*  9 */
        ProcXGetDeviceMotionEvents,     /* 10 */
        ProcXChangeKeyboardDevice,      /* 11 */
        ProcXChangePointerDevice,       /* 12 */
        ProcXGrabDevice,        /* 13 */
        ProcXUngrabDevice,      /* 14 */
        ProcXGrabDeviceKey,     /* 15 */
        ProcXUngrabDeviceKey,   /* 16 */
        ProcXGrabDeviceButton,  /* 17 */
        ProcXUngrabDeviceButton,        /* 18 */
        ProcXAllowDeviceEvents, /* 19 */
        ProcXGetDeviceFocus,    /* 20 */
        ProcXSetDeviceFocus,    /* 21 */
        ProcXGetFeedbackControl,        /* 22 */
        ProcXChangeFeedbackControl,     /* 23 */
        ProcXGetDeviceKeyMapping,       /* 24 */
        ProcXChangeDeviceKeyMapping,    /* 25 */
        ProcXGetDeviceModifierMapping,  /* 26 */
        ProcXSetDeviceModifierMapping,  /* 27 */
        ProcXGetDeviceButtonMapping,    /* 28 */
        ProcXSetDeviceButtonMapping,    /* 29 */
        ProcXQueryDeviceState,  /* 30 */
        ProcXSendExtensionEvent,        /* 31 */
        ProcXDeviceBell,        /* 32 */
        ProcXSetDeviceValuators,        /* 33 */
        ProcXGetDeviceControl,  /* 34 */
        ProcXChangeDeviceControl,       /* 35 */
        /* XI 1.5 */
        ProcXListDeviceProperties,      /* 36 */
        ProcXChangeDeviceProperty,      /* 37 */
        ProcXDeleteDeviceProperty,      /* 38 */
        ProcXGetDeviceProperty, /* 39 */
        /* XI 2 */
        ProcXIQueryPointer,     /* 40 */
        ProcXIWarpPointer,      /* 41 */
        ProcXIChangeCursor,     /* 42 */
        ProcXIChangeHierarchy,  /* 43 */
        ProcXISetClientPointer, /* 44 */
        ProcXIGetClientPointer, /* 45 */
        ProcXISelectEvents,     /* 46 */
        ProcXIQueryVersion,     /* 47 */
        ProcXIQueryDevice,      /* 48 */
        ProcXISetFocus,         /* 49 */
        ProcXIGetFocus,         /* 50 */
        ProcXIGrabDevice,       /* 51 */
        ProcXIUngrabDevice,     /* 52 */
        ProcXIAllowEvents,      /* 53 */
        ProcXIPassiveGrabDevice,        /* 54 */
        ProcXIPassiveUngrabDevice,      /* 55 */
        ProcXIListProperties,   /* 56 */
        ProcXIChangeProperty,   /* 57 */
        ProcXIDeleteProperty,   /* 58 */
        ProcXIGetProperty,      /* 59 */
        ProcXIGetSelectedEvents, /* 60 */
        ProcXIBarrierReleasePointer /* 61 */
};

/* For swapped clients */
static int (*SProcIVector[]) (ClientPtr) = {
    NULL,                       /*  0 */
        SProcXGetExtensionVersion,      /*  1 */
        SProcXListInputDevices, /*  2 */
        SProcXOpenDevice,       /*  3 */
        SProcXCloseDevice,      /*  4 */
        SProcXSetDeviceMode,    /*  5 */
        SProcXSelectExtensionEvent,     /*  6 */
        SProcXGetSelectedExtensionEvents,       /*  7 */
        SProcXChangeDeviceDontPropagateList,    /*  8 */
        SProcXGetDeviceDontPropagateList,       /*  9 */
        SProcXGetDeviceMotionEvents,    /* 10 */
        SProcXChangeKeyboardDevice,     /* 11 */
        SProcXChangePointerDevice,      /* 12 */
        SProcXGrabDevice,       /* 13 */
        SProcXUngrabDevice,     /* 14 */
        SProcXGrabDeviceKey,    /* 15 */
        SProcXUngrabDeviceKey,  /* 16 */
        SProcXGrabDeviceButton, /* 17 */
        SProcXUngrabDeviceButton,       /* 18 */
        SProcXAllowDeviceEvents,        /* 19 */
        SProcXGetDeviceFocus,   /* 20 */
        SProcXSetDeviceFocus,   /* 21 */
        SProcXGetFeedbackControl,       /* 22 */
        SProcXChangeFeedbackControl,    /* 23 */
        SProcXGetDeviceKeyMapping,      /* 24 */
        SProcXChangeDeviceKeyMapping,   /* 25 */
        SProcXGetDeviceModifierMapping, /* 26 */
        SProcXSetDeviceModifierMapping, /* 27 */
        SProcXGetDeviceButtonMapping,   /* 28 */
        SProcXSetDeviceButtonMapping,   /* 29 */
        SProcXQueryDeviceState, /* 30 */
        SProcXSendExtensionEvent,       /* 31 */
        SProcXDeviceBell,       /* 32 */
        SProcXSetDeviceValuators,       /* 33 */
        SProcXGetDeviceControl, /* 34 */
        SProcXChangeDeviceControl,      /* 35 */
        SProcXListDeviceProperties,     /* 36 */
        SProcXChangeDeviceProperty,     /* 37 */
        SProcXDeleteDeviceProperty,     /* 38 */
        SProcXGetDeviceProperty,        /* 39 */
        SProcXIQueryPointer,    /* 40 */
        SProcXIWarpPointer,     /* 41 */
        SProcXIChangeCursor,    /* 42 */
        SProcXIChangeHierarchy, /* 43 */
        SProcXISetClientPointer,        /* 44 */
        SProcXIGetClientPointer,        /* 45 */
        SProcXISelectEvents,    /* 46 */
        SProcXIQueryVersion,    /* 47 */
        SProcXIQueryDevice,     /* 48 */
        SProcXISetFocus,        /* 49 */
        SProcXIGetFocus,        /* 50 */
        SProcXIGrabDevice,      /* 51 */
        SProcXIUngrabDevice,    /* 52 */
        SProcXIAllowEvents,     /* 53 */
        SProcXIPassiveGrabDevice,       /* 54 */
        SProcXIPassiveUngrabDevice,     /* 55 */
        SProcXIListProperties,  /* 56 */
        SProcXIChangeProperty,  /* 57 */
        SProcXIDeleteProperty,  /* 58 */
        SProcXIGetProperty,     /* 59 */
        SProcXIGetSelectedEvents,       /* 60 */
        SProcXIBarrierReleasePointer /* 61 */
};

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */

int IReqCode = 0;
int IEventBase = 0;
int BadDevice = 0;
static int BadEvent = 1;
int BadMode = 2;
int DeviceBusy = 3;
int BadClass = 4;

int DeviceValuator;
int DeviceKeyPress;
int DeviceKeyRelease;
int DeviceButtonPress;
int DeviceButtonRelease;
int DeviceMotionNotify;
int DeviceFocusIn;
int DeviceFocusOut;
int ProximityIn;
int ProximityOut;
int DeviceStateNotify;
int DeviceKeyStateNotify;
int DeviceButtonStateNotify;
int DeviceMappingNotify;
int ChangeDeviceNotify;
int DevicePresenceNotify;
int DevicePropertyNotify;

RESTYPE RT_INPUTCLIENT;

/*****************************************************************
 *
 * Externs defined elsewhere in the X server.
 *
 */

extern XExtensionVersion XIVersion;

/*****************************************************************
 *
 * Versioning support
 *
 */

DevPrivateKeyRec XIClientPrivateKeyRec;

/*****************************************************************
 *
 * Declarations of local routines.
 *
 */

/*************************************************************************
 *
 * ProcIDispatch - main dispatch routine for requests to this extension.
 * This routine is used if server and client have the same byte ordering.
 *
 */

static int
ProcIDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= ARRAY_SIZE(ProcIVector) || !ProcIVector[stuff->data])
        return BadRequest;

    UpdateCurrentTimeIf();
    return (*ProcIVector[stuff->data]) (client);
}

/*******************************************************************************
 *
 * SProcXDispatch
 *
 * Main swapped dispatch routine for requests to this extension.
 * This routine is used if server and client do not have the same byte ordering.
 *
 */

static int _X_COLD
SProcIDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= ARRAY_SIZE(SProcIVector) || !SProcIVector[stuff->data])
        return BadRequest;

    UpdateCurrentTimeIf();
    return (*SProcIVector[stuff->data]) (client);
}

/**********************************************************************
 *
 * SReplyIDispatch
 * Swap any replies defined in this extension.
 *
 */

static void _X_COLD
SReplyIDispatch(ClientPtr client, int len, xGrabDeviceReply * rep)
{
    /* All we look at is the type field */
    /* This is common to all replies    */
    if (rep->RepType == X_GetExtensionVersion)
        SRepXGetExtensionVersion(client, len,
                                 (xGetExtensionVersionReply *) rep);
    else if (rep->RepType == X_ListInputDevices)
        SRepXListInputDevices(client, len, (xListInputDevicesReply *) rep);
    else if (rep->RepType == X_OpenDevice)
        SRepXOpenDevice(client, len, (xOpenDeviceReply *) rep);
    else if (rep->RepType == X_SetDeviceMode)
        SRepXSetDeviceMode(client, len, (xSetDeviceModeReply *) rep);
    else if (rep->RepType == X_GetSelectedExtensionEvents)
        SRepXGetSelectedExtensionEvents(client, len,
                                        (xGetSelectedExtensionEventsReply *)
                                        rep);
    else if (rep->RepType == X_GetDeviceDontPropagateList)
        SRepXGetDeviceDontPropagateList(client, len,
                                        (xGetDeviceDontPropagateListReply *)
                                        rep);
    else if (rep->RepType == X_GetDeviceMotionEvents)
        SRepXGetDeviceMotionEvents(client, len,
                                   (xGetDeviceMotionEventsReply *) rep);
    else if (rep->RepType == X_GrabDevice)
        SRepXGrabDevice(client, len, (xGrabDeviceReply *) rep);
    else if (rep->RepType == X_GetDeviceFocus)
        SRepXGetDeviceFocus(client, len, (xGetDeviceFocusReply *) rep);
    else if (rep->RepType == X_GetFeedbackControl)
        SRepXGetFeedbackControl(client, len, (xGetFeedbackControlReply *) rep);
    else if (rep->RepType == X_GetDeviceKeyMapping)
        SRepXGetDeviceKeyMapping(client, len,
                                 (xGetDeviceKeyMappingReply *) rep);
    else if (rep->RepType == X_GetDeviceModifierMapping)
        SRepXGetDeviceModifierMapping(client, len,
                                      (xGetDeviceModifierMappingReply *) rep);
    else if (rep->RepType == X_SetDeviceModifierMapping)
        SRepXSetDeviceModifierMapping(client, len,
                                      (xSetDeviceModifierMappingReply *) rep);
    else if (rep->RepType == X_GetDeviceButtonMapping)
        SRepXGetDeviceButtonMapping(client, len,
                                    (xGetDeviceButtonMappingReply *) rep);
    else if (rep->RepType == X_SetDeviceButtonMapping)
        SRepXSetDeviceButtonMapping(client, len,
                                    (xSetDeviceButtonMappingReply *) rep);
    else if (rep->RepType == X_QueryDeviceState)
        SRepXQueryDeviceState(client, len, (xQueryDeviceStateReply *) rep);
    else if (rep->RepType == X_SetDeviceValuators)
        SRepXSetDeviceValuators(client, len, (xSetDeviceValuatorsReply *) rep);
    else if (rep->RepType == X_GetDeviceControl)
        SRepXGetDeviceControl(client, len, (xGetDeviceControlReply *) rep);
    else if (rep->RepType == X_ChangeDeviceControl)
        SRepXChangeDeviceControl(client, len,
                                 (xChangeDeviceControlReply *) rep);
    else if (rep->RepType == X_ListDeviceProperties)
        SRepXListDeviceProperties(client, len,
                                  (xListDevicePropertiesReply *) rep);
    else if (rep->RepType == X_GetDeviceProperty)
        SRepXGetDeviceProperty(client, len, (xGetDevicePropertyReply *) rep);
    else if (rep->RepType == X_XIQueryPointer)
        SRepXIQueryPointer(client, len, (xXIQueryPointerReply *) rep);
    else if (rep->RepType == X_XIGetClientPointer)
        SRepXIGetClientPointer(client, len, (xXIGetClientPointerReply *) rep);
    else if (rep->RepType == X_XIQueryVersion)
        SRepXIQueryVersion(client, len, (xXIQueryVersionReply *) rep);
    else if (rep->RepType == X_XIQueryDevice)
        SRepXIQueryDevice(client, len, (xXIQueryDeviceReply *) rep);
    else if (rep->RepType == X_XIGrabDevice)
        SRepXIGrabDevice(client, len, (xXIGrabDeviceReply *) rep);
    else if (rep->RepType == X_XIPassiveGrabDevice)
        SRepXIPassiveGrabDevice(client, len, (xXIPassiveGrabDeviceReply *) rep);
    else if (rep->RepType == X_XIListProperties)
        SRepXIListProperties(client, len, (xXIListPropertiesReply *) rep);
    else if (rep->RepType == X_XIGetProperty)
        SRepXIGetProperty(client, len, (xXIGetPropertyReply *) rep);
    else if (rep->RepType == X_XIGetSelectedEvents)
        SRepXIGetSelectedEvents(client, len, (xXIGetSelectedEventsReply *) rep);
    else if (rep->RepType == X_XIGetFocus)
        SRepXIGetFocus(client, len, (xXIGetFocusReply *) rep);
    else {
        FatalError("XINPUT confused sending swapped reply");
    }
}

/************************************************************************
 *
 * This function swaps the DeviceValuator event.
 *
 */

static void
SEventDeviceValuator(deviceValuator * from, deviceValuator * to)
{
    int i;
    INT32 *ip;

    *to = *from;
    swaps(&to->sequenceNumber);
    swaps(&to->device_state);
    ip = &to->valuator0;
    for (i = 0; i < 6; i++) {
        swapl(ip + i);
    }
}

static void
SEventFocus(deviceFocus * from, deviceFocus * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swapl(&to->window);
}

static void
SDeviceStateNotifyEvent(deviceStateNotify * from, deviceStateNotify * to)
{
    int i;
    INT32 *ip;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    ip = &to->valuator0;
    for (i = 0; i < 3; i++) {
        swapl(ip + i);
    }
}

static void
SDeviceKeyStateNotifyEvent(deviceKeyStateNotify * from,
                           deviceKeyStateNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
}

static void
SDeviceButtonStateNotifyEvent(deviceButtonStateNotify * from,
                              deviceButtonStateNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
}

static void
SChangeDeviceNotifyEvent(changeDeviceNotify * from, changeDeviceNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
}

static void
SDeviceMappingNotifyEvent(deviceMappingNotify * from, deviceMappingNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
}

static void
SDevicePresenceNotifyEvent(devicePresenceNotify * from,
                           devicePresenceNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swaps(&to->control);
}

static void
SDevicePropertyNotifyEvent(devicePropertyNotify * from,
                           devicePropertyNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swapl(&to->atom);
}

static void
SDeviceLeaveNotifyEvent(xXILeaveEvent * from, xXILeaveEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);
    swaps(&to->sourceid);
    swaps(&to->buttons_len);
    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
}

static void
SDeviceChangedEvent(xXIDeviceChangedEvent * from, xXIDeviceChangedEvent * to)
{
    int i, j;
    xXIAnyInfo *any;

    *to = *from;
    memcpy(&to[1], &from[1], from->length * 4);

    any = (xXIAnyInfo *) &to[1];
    for (i = 0; i < to->num_classes; i++) {
        int length = any->length;

        switch (any->type) {
        case KeyClass:
        {
            xXIKeyInfo *ki = (xXIKeyInfo *) any;
            uint32_t *key = (uint32_t *) &ki[1];

            for (j = 0; j < ki->num_keycodes; j++, key++)
                swapl(key);
            swaps(&ki->num_keycodes);
        }
            break;
        case ButtonClass:
        {
            xXIButtonInfo *bi = (xXIButtonInfo *) any;
            Atom *labels = (Atom *) ((char *) bi + sizeof(xXIButtonInfo) +
                                     pad_to_int32(bits_to_bytes
                                                  (bi->num_buttons)));
            for (j = 0; j < bi->num_buttons; j++)
                swapl(&labels[j]);
            swaps(&bi->num_buttons);
        }
            break;
        case ValuatorClass:
        {
            xXIValuatorInfo *ai = (xXIValuatorInfo *) any;

            swapl(&ai->label);
            swapl(&ai->min.integral);
            swapl(&ai->min.frac);
            swapl(&ai->max.integral);
            swapl(&ai->max.frac);
            swapl(&ai->resolution);
            swaps(&ai->number);
        }
            break;
        }

        swaps(&any->type);
        swaps(&any->length);
        swaps(&any->sourceid);

        any = (xXIAnyInfo *) ((char *) any + length * 4);
    }

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swaps(&to->num_classes);
    swaps(&to->sourceid);

}

static void
SDeviceEvent(xXIDeviceEvent * from, xXIDeviceEvent * to)
{
    int i;
    char *ptr;
    char *vmask;

    memcpy(to, from, sizeof(xEvent) + from->length * 4);

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);
    swaps(&to->buttons_len);
    swaps(&to->valuators_len);
    swaps(&to->sourceid);
    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);

    ptr = (char *) (&to[1]);
    ptr += from->buttons_len * 4;
    vmask = ptr;                /* valuator mask */
    ptr += from->valuators_len * 4;
    for (i = 0; i < from->valuators_len * 32; i++) {
        if (BitIsOn(vmask, i)) {
            swapl(((uint32_t *) ptr));
            ptr += 4;
            swapl(((uint32_t *) ptr));
            ptr += 4;
        }
    }
}

static void
SDeviceHierarchyEvent(xXIHierarchyEvent * from, xXIHierarchyEvent * to)
{
    int i;
    xXIHierarchyInfo *info;

    *to = *from;
    memcpy(&to[1], &from[1], from->length * 4);
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->flags);
    swaps(&to->num_info);

    info = (xXIHierarchyInfo *) &to[1];
    for (i = 0; i < from->num_info; i++) {
        swaps(&info->deviceid);
        swaps(&info->attachment);
        info++;
    }
}

static void
SXIPropertyEvent(xXIPropertyEvent * from, xXIPropertyEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->property);
}

static void
SRawEvent(xXIRawEvent * from, xXIRawEvent * to)
{
    int i;
    FP3232 *values;
    unsigned char *mask;

    memcpy(to, from, sizeof(xEvent) + from->length * 4);

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);

    mask = (unsigned char *) &to[1];
    values = (FP3232 *) (mask + from->valuators_len * 4);

    for (i = 0; i < from->valuators_len * 4 * 8; i++) {
        if (BitIsOn(mask, i)) {
            /* for each bit set there are two FP3232 values on the wire, in
             * the order abcABC for data and data_raw. Here we swap as if
             * they were in aAbBcC order because it's easier and really
             * doesn't matter.
             */
            swapl(&values->integral);
            swapl(&values->frac);
            values++;
            swapl(&values->integral);
            swapl(&values->frac);
            values++;
        }
    }

    swaps(&to->valuators_len);
}

static void
STouchOwnershipEvent(xXITouchOwnershipEvent * from, xXITouchOwnershipEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swaps(&to->sourceid);
    swapl(&to->touchid);
    swapl(&to->flags);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
}

static void
SBarrierEvent(xXIBarrierEvent * from,
              xXIBarrierEvent * to) {

    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swapl(&to->time);
    swaps(&to->deviceid);
    swaps(&to->sourceid);
    swapl(&to->event);
    swapl(&to->root);
    swapl(&to->root_x);
    swapl(&to->root_y);

    swapl(&to->dx.integral);
    swapl(&to->dx.frac);
    swapl(&to->dy.integral);
    swapl(&to->dy.frac);
    swapl(&to->dtime);
    swapl(&to->barrier);
    swapl(&to->eventid);
}

static void
SGesturePinchEvent(xXIGesturePinchEvent* from,
                   xXIGesturePinchEvent* to)
{
    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);

    swapl(&to->delta_x);
    swapl(&to->delta_y);
    swapl(&to->delta_unaccel_x);
    swapl(&to->delta_unaccel_y);
    swapl(&to->scale);
    swapl(&to->delta_angle);
    swaps(&to->sourceid);

    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);
}

static void
SGestureSwipeEvent(xXIGestureSwipeEvent* from,
                   xXIGestureSwipeEvent* to)
{
    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);

    swapl(&to->delta_x);
    swapl(&to->delta_y);
    swapl(&to->delta_unaccel_x);
    swapl(&to->delta_unaccel_y);
    swaps(&to->sourceid);

    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);
}

/** Event swapping function for XI2 events. */
void _X_COLD
XI2EventSwap(xGenericEvent *from, xGenericEvent *to)
{
    switch (from->evtype) {
    case XI_Enter:
    case XI_Leave:
    case XI_FocusIn:
    case XI_FocusOut:
        SDeviceLeaveNotifyEvent((xXILeaveEvent *) from, (xXILeaveEvent *) to);
        break;
    case XI_DeviceChanged:
        SDeviceChangedEvent((xXIDeviceChangedEvent *) from,
                            (xXIDeviceChangedEvent *) to);
        break;
    case XI_HierarchyChanged:
        SDeviceHierarchyEvent((xXIHierarchyEvent *) from,
                              (xXIHierarchyEvent *) to);
        break;
    case XI_PropertyEvent:
        SXIPropertyEvent((xXIPropertyEvent *) from, (xXIPropertyEvent *) to);
        break;
    case XI_Motion:
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd:
        SDeviceEvent((xXIDeviceEvent *) from, (xXIDeviceEvent *) to);
        break;
    case XI_TouchOwnership:
        STouchOwnershipEvent((xXITouchOwnershipEvent *) from,
                             (xXITouchOwnershipEvent *) to);
        break;
    case XI_RawMotion:
    case XI_RawKeyPress:
    case XI_RawKeyRelease:
    case XI_RawButtonPress:
    case XI_RawButtonRelease:
    case XI_RawTouchBegin:
    case XI_RawTouchUpdate:
    case XI_RawTouchEnd:
        SRawEvent((xXIRawEvent *) from, (xXIRawEvent *) to);
        break;
    case XI_BarrierHit:
    case XI_BarrierLeave:
        SBarrierEvent((xXIBarrierEvent *) from,
                      (xXIBarrierEvent *) to);
        break;
    case XI_GesturePinchBegin:
    case XI_GesturePinchUpdate:
    case XI_GesturePinchEnd:
        SGesturePinchEvent((xXIGesturePinchEvent*) from,
                           (xXIGesturePinchEvent*) to);
        break;
    case XI_GestureSwipeBegin:
    case XI_GestureSwipeUpdate:
    case XI_GestureSwipeEnd:
        SGestureSwipeEvent((xXIGestureSwipeEvent*) from,
                           (xXIGestureSwipeEvent*) to);
        break;
    default:
        ErrorF("[Xi] Unknown event type to swap. This is a bug.\n");
        break;
    }
}

/**************************************************************************
 *
 * Record an event mask where there is no unique corresponding event type.
 * We can't call SetMaskForEvent, since that would clobber the existing
 * mask for that event.  MotionHint and ButtonMotion are examples.
 *
 * Since extension event types will never be less than 64, we can use
 * 0-63 in the EventInfo array as the "type" to be used to look up this
 * mask.  This means that the corresponding macros such as
 * DevicePointerMotionHint must have access to the same constants.
 *
 */

static void
SetEventInfo(Mask mask, int constant)
{
    EventInfo[ExtEventIndex].mask = mask;
    EventInfo[ExtEventIndex++].type = constant;
}

/**************************************************************************
 *
 * Assign the specified mask to the specified event.
 *
 */

static void
SetMaskForExtEvent(Mask mask, int event)
{
    int i;

    EventInfo[ExtEventIndex].mask = mask;
    EventInfo[ExtEventIndex++].type = event;

    if ((event < LASTEvent) || (event >= 128))
        FatalError("MaskForExtensionEvent: bogus event number");

    for (i = 0; i < MAXDEVICES; i++)
        SetMaskForEvent(i, mask, event);
}

/************************************************************************
 *
 * This function sets up extension event types and masks.
 *
 */

static void
FixExtensionEvents(ExtensionEntry * extEntry)
{
    DeviceValuator = extEntry->eventBase;
    DeviceKeyPress = DeviceValuator + 1;
    DeviceKeyRelease = DeviceKeyPress + 1;
    DeviceButtonPress = DeviceKeyRelease + 1;
    DeviceButtonRelease = DeviceButtonPress + 1;
    DeviceMotionNotify = DeviceButtonRelease + 1;
    DeviceFocusIn = DeviceMotionNotify + 1;
    DeviceFocusOut = DeviceFocusIn + 1;
    ProximityIn = DeviceFocusOut + 1;
    ProximityOut = ProximityIn + 1;
    DeviceStateNotify = ProximityOut + 1;
    DeviceMappingNotify = DeviceStateNotify + 1;
    ChangeDeviceNotify = DeviceMappingNotify + 1;
    DeviceKeyStateNotify = ChangeDeviceNotify + 1;
    DeviceButtonStateNotify = DeviceKeyStateNotify + 1;
    DevicePresenceNotify = DeviceButtonStateNotify + 1;
    DevicePropertyNotify = DevicePresenceNotify + 1;

    event_base[KeyClass] = DeviceKeyPress;
    event_base[ButtonClass] = DeviceButtonPress;
    event_base[ValuatorClass] = DeviceMotionNotify;
    event_base[ProximityClass] = ProximityIn;
    event_base[FocusClass] = DeviceFocusIn;
    event_base[OtherClass] = DeviceStateNotify;

    BadDevice += extEntry->errorBase;
    BadEvent += extEntry->errorBase;
    BadMode += extEntry->errorBase;
    DeviceBusy += extEntry->errorBase;
    BadClass += extEntry->errorBase;

    SetMaskForExtEvent(KeyPressMask, DeviceKeyPress);
    SetCriticalEvent(DeviceKeyPress);

    SetMaskForExtEvent(KeyReleaseMask, DeviceKeyRelease);
    SetCriticalEvent(DeviceKeyRelease);

    SetMaskForExtEvent(ButtonPressMask, DeviceButtonPress);
    SetCriticalEvent(DeviceButtonPress);

    SetMaskForExtEvent(ButtonReleaseMask, DeviceButtonRelease);
    SetCriticalEvent(DeviceButtonRelease);

    SetMaskForExtEvent(DeviceProximityMask, ProximityIn);
    SetMaskForExtEvent(DeviceProximityMask, ProximityOut);

    SetMaskForExtEvent(DeviceStateNotifyMask, DeviceStateNotify);

    SetMaskForExtEvent(PointerMotionMask, DeviceMotionNotify);
    SetCriticalEvent(DeviceMotionNotify);

    SetEventInfo(DevicePointerMotionHintMask, _devicePointerMotionHint);
    SetEventInfo(DeviceButton1MotionMask, _deviceButton1Motion);
    SetEventInfo(DeviceButton2MotionMask, _deviceButton2Motion);
    SetEventInfo(DeviceButton3MotionMask, _deviceButton3Motion);
    SetEventInfo(DeviceButton4MotionMask, _deviceButton4Motion);
    SetEventInfo(DeviceButton5MotionMask, _deviceButton5Motion);
    SetEventInfo(DeviceButtonMotionMask, _deviceButtonMotion);

    SetMaskForExtEvent(DeviceFocusChangeMask, DeviceFocusIn);
    SetMaskForExtEvent(DeviceFocusChangeMask, DeviceFocusOut);

    SetMaskForExtEvent(DeviceMappingNotifyMask, DeviceMappingNotify);
    SetMaskForExtEvent(ChangeDeviceNotifyMask, ChangeDeviceNotify);

    SetEventInfo(DeviceButtonGrabMask, _deviceButtonGrab);
    SetEventInfo(DeviceOwnerGrabButtonMask, _deviceOwnerGrabButton);
    SetEventInfo(DevicePresenceNotifyMask, _devicePresence);
    SetMaskForExtEvent(DevicePropertyNotifyMask, DevicePropertyNotify);

    SetEventInfo(0, _noExtensionEvent);
}

/************************************************************************
 *
 * This function restores extension event types and masks to their
 * initial state.
 *
 */

static void
RestoreExtensionEvents(void)
{
    int i, j;

    IReqCode = 0;
    IEventBase = 0;

    for (i = 0; i < ExtEventIndex - 1; i++) {
        if ((EventInfo[i].type >= LASTEvent) && (EventInfo[i].type < 128)) {
            for (j = 0; j < MAXDEVICES; j++)
                SetMaskForEvent(j, 0, EventInfo[i].type);
        }
        EventInfo[i].mask = 0;
        EventInfo[i].type = 0;
    }
    ExtEventIndex = 0;
    DeviceValuator = 0;
    DeviceKeyPress = 1;
    DeviceKeyRelease = 2;
    DeviceButtonPress = 3;
    DeviceButtonRelease = 4;
    DeviceMotionNotify = 5;
    DeviceFocusIn = 6;
    DeviceFocusOut = 7;
    ProximityIn = 8;
    ProximityOut = 9;
    DeviceStateNotify = 10;
    DeviceMappingNotify = 11;
    ChangeDeviceNotify = 12;
    DeviceKeyStateNotify = 13;
    DeviceButtonStateNotify = 13;
    DevicePresenceNotify = 14;
    DevicePropertyNotify = 15;

    BadDevice = 0;
    BadEvent = 1;
    BadMode = 2;
    DeviceBusy = 3;
    BadClass = 4;

}

/***********************************************************************
 *
 * IResetProc.
 * Remove reply-swapping routine.
 * Remove event-swapping routine.
 *
 */

static void
IResetProc(ExtensionEntry * unused)
{
    ReplySwapVector[IReqCode] = ReplyNotSwappd;
    EventSwapVector[DeviceValuator] = NotImplemented;
    EventSwapVector[DeviceKeyPress] = NotImplemented;
    EventSwapVector[DeviceKeyRelease] = NotImplemented;
    EventSwapVector[DeviceButtonPress] = NotImplemented;
    EventSwapVector[DeviceButtonRelease] = NotImplemented;
    EventSwapVector[DeviceMotionNotify] = NotImplemented;
    EventSwapVector[DeviceFocusIn] = NotImplemented;
    EventSwapVector[DeviceFocusOut] = NotImplemented;
    EventSwapVector[ProximityIn] = NotImplemented;
    EventSwapVector[ProximityOut] = NotImplemented;
    EventSwapVector[DeviceStateNotify] = NotImplemented;
    EventSwapVector[DeviceKeyStateNotify] = NotImplemented;
    EventSwapVector[DeviceButtonStateNotify] = NotImplemented;
    EventSwapVector[DeviceMappingNotify] = NotImplemented;
    EventSwapVector[ChangeDeviceNotify] = NotImplemented;
    EventSwapVector[DevicePresenceNotify] = NotImplemented;
    EventSwapVector[DevicePropertyNotify] = NotImplemented;
    RestoreExtensionEvents();

    free(xi_all_devices.name);
    free(xi_all_master_devices.name);

    XIBarrierReset();
}

/***********************************************************************
 *
 * Assign an id and type to an input device.
 *
 */

void
AssignTypeAndName(DeviceIntPtr dev, Atom type, const char *name)
{
    dev->xinput_type = type;
    dev->name = strdup(name);
}

/***********************************************************************
 *
 * Make device type atoms.
 *
 */

static void
MakeDeviceTypeAtoms(void)
{
    int i;

    for (i = 0; i < NUMTYPES; i++)
        dev_type[i].type =
            MakeAtom(dev_type[i].name, strlen(dev_type[i].name), 1);
}

/*****************************************************************************
 *
 *	SEventIDispatch
 *
 *	Swap any events defined in this extension.
 */
#define DO_SWAP(func,type) func ((type *)from, (type *)to)

static void _X_COLD
SEventIDispatch(xEvent *from, xEvent *to)
{
    int type = from->u.u.type & 0177;

    if (type == DeviceValuator)
        DO_SWAP(SEventDeviceValuator, deviceValuator);
    else if (type == DeviceKeyPress) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceKeyRelease) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceButtonPress) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceButtonRelease) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceMotionNotify) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceFocusIn)
        DO_SWAP(SEventFocus, deviceFocus);
    else if (type == DeviceFocusOut)
        DO_SWAP(SEventFocus, deviceFocus);
    else if (type == ProximityIn) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == ProximityOut) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == DeviceStateNotify)
        DO_SWAP(SDeviceStateNotifyEvent, deviceStateNotify);
    else if (type == DeviceKeyStateNotify)
        DO_SWAP(SDeviceKeyStateNotifyEvent, deviceKeyStateNotify);
    else if (type == DeviceButtonStateNotify)
        DO_SWAP(SDeviceButtonStateNotifyEvent, deviceButtonStateNotify);
    else if (type == DeviceMappingNotify)
        DO_SWAP(SDeviceMappingNotifyEvent, deviceMappingNotify);
    else if (type == ChangeDeviceNotify)
        DO_SWAP(SChangeDeviceNotifyEvent, changeDeviceNotify);
    else if (type == DevicePresenceNotify)
        DO_SWAP(SDevicePresenceNotifyEvent, devicePresenceNotify);
    else if (type == DevicePropertyNotify)
        DO_SWAP(SDevicePropertyNotifyEvent, devicePropertyNotify);
    else {
        FatalError("XInputExtension: Impossible event!\n");
    }
}

/**********************************************************************
 *
 * IExtensionInit - initialize the input extension.
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * This extension has several events and errors.
 *
 * XI is mandatory nowadays, so if we fail to init XI, we die.
 */

void
XInputExtensionInit(void)
{
    ExtensionEntry *extEntry;

    XExtensionVersion thisversion = { XI_Present,
        SERVER_XI_MAJOR_VERSION,
        SERVER_XI_MINOR_VERSION,
    };

    if (!dixRegisterPrivateKey
        (&XIClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(XIClientRec)))
        FatalError("Cannot request private for XI.\n");

    if (!XIBarrierInit())
        FatalError("Could not initialize barriers.\n");

    extEntry = AddExtension(INAME, IEVENTS, IERRORS, ProcIDispatch,
                            SProcIDispatch, IResetProc, StandardMinorOpcode);
    if (extEntry) {
        IReqCode = extEntry->base;
        IEventBase = extEntry->eventBase;
        XIVersion = thisversion;
        MakeDeviceTypeAtoms();
        RT_INPUTCLIENT = CreateNewResourceType((DeleteType) InputClientGone,
                                               "INPUTCLIENT");
        if (!RT_INPUTCLIENT)
            FatalError("Failed to add resource type for XI.\n");
        FixExtensionEvents(extEntry);
        ReplySwapVector[IReqCode] = (ReplySwapPtr) SReplyIDispatch;
        EventSwapVector[DeviceValuator] = SEventIDispatch;
        EventSwapVector[DeviceKeyPress] = SEventIDispatch;
        EventSwapVector[DeviceKeyRelease] = SEventIDispatch;
        EventSwapVector[DeviceButtonPress] = SEventIDispatch;
        EventSwapVector[DeviceButtonRelease] = SEventIDispatch;
        EventSwapVector[DeviceMotionNotify] = SEventIDispatch;
        EventSwapVector[DeviceFocusIn] = SEventIDispatch;
        EventSwapVector[DeviceFocusOut] = SEventIDispatch;
        EventSwapVector[ProximityIn] = SEventIDispatch;
        EventSwapVector[ProximityOut] = SEventIDispatch;
        EventSwapVector[DeviceStateNotify] = SEventIDispatch;
        EventSwapVector[DeviceKeyStateNotify] = SEventIDispatch;
        EventSwapVector[DeviceButtonStateNotify] = SEventIDispatch;
        EventSwapVector[DeviceMappingNotify] = SEventIDispatch;
        EventSwapVector[ChangeDeviceNotify] = SEventIDispatch;
        EventSwapVector[DevicePresenceNotify] = SEventIDispatch;

        GERegisterExtension(IReqCode, XI2EventSwap);

        memset(&xi_all_devices, 0, sizeof(xi_all_devices));
        memset(&xi_all_master_devices, 0, sizeof(xi_all_master_devices));
        xi_all_devices.id = XIAllDevices;
        xi_all_devices.name = strdup("XIAllDevices");
        xi_all_master_devices.id = XIAllMasterDevices;
        xi_all_master_devices.name = strdup("XIAllMasterDevices");

        inputInfo.all_devices = &xi_all_devices;
        inputInfo.all_master_devices = &xi_all_master_devices;

        XIResetProperties();
    }
    else {
        FatalError("IExtensionInit: AddExtensions failed\n");
    }
}
