/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/********************************************************************
 * Interface of 'exevents.c'
 */

#ifndef EXEVENTS_H
#define EXEVENTS_H

#include <X11/extensions/XIproto.h>
#include "inputstr.h"

/***************************************************************
 *              Interface available to drivers                 *
 ***************************************************************/

/**
 * Scroll flags for ::SetScrollValuator.
 */
enum ScrollFlags {
    SCROLL_FLAG_NONE = 0,
    /**
     * Do not emulate legacy button events for valuator events on this axis.
     */
    SCROLL_FLAG_DONT_EMULATE = (1 << 1),
    /**
     * This axis is the preferred axis for valuator emulation for this axis'
     * scroll type.
     */
    SCROLL_FLAG_PREFERRED = (1 << 2)
};

extern _X_EXPORT int InitProximityClassDeviceStruct(DeviceIntPtr /* dev */ );

extern _X_EXPORT Bool InitValuatorAxisStruct(DeviceIntPtr /* dev */ ,
                                             int /* axnum */ ,
                                             Atom /* label */ ,
                                             int /* minval */ ,
                                             int /* maxval */ ,
                                             int /* resolution */ ,
                                             int /* min_res */ ,
                                             int /* max_res */ ,
                                             int /* mode */ );

extern _X_EXPORT Bool SetScrollValuator(DeviceIntPtr /* dev */ ,
                                        int /* axnum */ ,
                                        enum ScrollType /* type */ ,
                                        double /* increment */ ,
                                        int /* flags */ );

/* Input device properties */
extern _X_EXPORT void XIDeleteAllDeviceProperties(DeviceIntPtr  /* device */
    );

extern _X_EXPORT int XIDeleteDeviceProperty(DeviceIntPtr /* device */ ,
                                            Atom /* property */ ,
                                            Bool        /* fromClient */
    );

extern _X_EXPORT int XIChangeDeviceProperty(DeviceIntPtr /* dev */ ,
                                            Atom /* property */ ,
                                            Atom /* type */ ,
                                            int /* format */ ,
                                            int /* mode */ ,
                                            unsigned long /* len */ ,
                                            const void * /* value */ ,
                                            Bool        /* sendevent */
    );

extern _X_EXPORT int XIGetDeviceProperty(DeviceIntPtr /* dev */ ,
                                         Atom /* property */ ,
                                         XIPropertyValuePtr *   /* value */
    );

extern _X_EXPORT int XISetDevicePropertyDeletable(DeviceIntPtr /* dev */ ,
                                                  Atom /* property */ ,
                                                  Bool  /* deletable */
    );

extern _X_EXPORT long XIRegisterPropertyHandler(DeviceIntPtr dev,
                                                int (*SetProperty) (DeviceIntPtr
                                                                    dev,
                                                                    Atom
                                                                    property,
                                                                    XIPropertyValuePtr
                                                                    prop,
                                                                    BOOL
                                                                    checkonly),
                                                int (*GetProperty) (DeviceIntPtr
                                                                    dev,
                                                                    Atom
                                                                    property),
                                                int (*DeleteProperty)
                                                (DeviceIntPtr dev,
                                                 Atom property)
    );

extern _X_EXPORT void XIUnregisterPropertyHandler(DeviceIntPtr dev, long id);

extern _X_EXPORT Atom XIGetKnownProperty(const char *name);

extern _X_EXPORT DeviceIntPtr XIGetDevice(xEvent *ev);

extern _X_EXPORT int XIPropToInt(XIPropertyValuePtr val,
                                 int *nelem_return, int **buf_return);

extern _X_EXPORT int XIPropToFloat(XIPropertyValuePtr val,
                                   int *nelem_return, float **buf_return);

/****************************************************************************
 *                      End of driver interface                             *
 ****************************************************************************/

/**
 * Attached to the devPrivates of each client. Specifies the version number as
 * supported by the client.
 */
typedef struct _XIClientRec {
    int major_version;
    int minor_version;
} XIClientRec, *XIClientPtr;

typedef struct _GrabParameters {
    int grabtype;               /* CORE, etc. */
    unsigned int ownerEvents;
    unsigned int this_device_mode;
    unsigned int other_devices_mode;
    Window grabWindow;
    Window confineTo;
    Cursor cursor;
    unsigned int modifiers;
} GrabParameters;

extern int
 UpdateDeviceState(DeviceIntPtr /* device */ ,
                   DeviceEvent * /*  xE    */ );

extern void
 ProcessOtherEvent(InternalEvent * /* ev */ ,
                   DeviceIntPtr /* other */ );

extern int
 CheckGrabValues(ClientPtr /* client */ ,
                 GrabParameters * /* param */ );

extern int
 GrabButton(ClientPtr /* client */ ,
            DeviceIntPtr /* dev */ ,
            DeviceIntPtr /* modifier_device */ ,
            int /* button */ ,
            GrabParameters * /* param */ ,
            enum InputLevel /* grabtype */ ,
            GrabMask * /* eventMask */ );

extern int
 GrabKey(ClientPtr /* client */ ,
         DeviceIntPtr /* dev */ ,
         DeviceIntPtr /* modifier_device */ ,
         int /* key */ ,
         GrabParameters * /* param */ ,
         enum InputLevel /* grabtype */ ,
         GrabMask * /* eventMask */ );

extern int
 GrabWindow(ClientPtr /* client */ ,
            DeviceIntPtr /* dev */ ,
            int /* type */ ,
            GrabParameters * /* param */ ,
            GrabMask * /* eventMask */ );

extern int
 GrabTouchOrGesture(ClientPtr /* client */ ,
                    DeviceIntPtr /* dev */ ,
                    DeviceIntPtr /* mod_dev */ ,
                    int /* type */ ,
                    GrabParameters * /* param */ ,
                    GrabMask * /* eventMask */ );

extern int
 SelectForWindow(DeviceIntPtr /* dev */ ,
                 WindowPtr /* pWin */ ,
                 ClientPtr /* client */ ,
                 Mask /* mask */ ,
                 Mask /* exclusivemasks */ );

extern int
 AddExtensionClient(WindowPtr /* pWin */ ,
                    ClientPtr /* client */ ,
                    Mask /* mask */ ,
                    int /* mskidx */ );

extern void
 RecalculateDeviceDeliverableEvents(WindowPtr /* pWin */ );

extern int
 InputClientGone(WindowPtr /* pWin */ ,
                 XID /* id */ );

extern void
 WindowGone(WindowPtr /* win */ );

extern int
 SendEvent(ClientPtr /* client */ ,
           DeviceIntPtr /* d */ ,
           Window /* dest */ ,
           Bool /* propagate */ ,
           xEvent * /* ev */ ,
           Mask /* mask */ ,
           int /* count */ );

extern int
 SetButtonMapping(ClientPtr /* client */ ,
                  DeviceIntPtr /* dev */ ,
                  int /* nElts */ ,
                  BYTE * /* map */ );

extern int
 ChangeKeyMapping(ClientPtr /* client */ ,
                  DeviceIntPtr /* dev */ ,
                  unsigned /* len */ ,
                  int /* type */ ,
                  KeyCode /* firstKeyCode */ ,
                  CARD8 /* keyCodes */ ,
                  CARD8 /* keySymsPerKeyCode */ ,
                  KeySym * /* map */ );

extern void
 DeleteWindowFromAnyExtEvents(WindowPtr /* pWin */ ,
                              Bool /* freeResources */ );

extern int
 MaybeSendDeviceMotionNotifyHint(deviceKeyButtonPointer * /* pEvents */ ,
                                 Mask /* mask */ );

extern void
 CheckDeviceGrabAndHintWindow(WindowPtr /* pWin */ ,
                              int /* type */ ,
                              deviceKeyButtonPointer * /* xE */ ,
                              GrabPtr /* grab */ ,
                              ClientPtr /* client */ ,
                              Mask /* deliveryMask */ );

extern void
 MaybeStopDeviceHint(DeviceIntPtr /* dev */ ,
                     ClientPtr /* client */ );

extern int
 DeviceEventSuppressForWindow(WindowPtr /* pWin */ ,
                              ClientPtr /* client */ ,
                              Mask /* mask */ ,
                              int /* maskndx */ );

extern void
 SendEventToAllWindows(DeviceIntPtr /* dev */ ,
                       Mask /* mask */ ,
                       xEvent * /* ev */ ,
                       int /* count */ );

extern void
 TouchRejected(DeviceIntPtr /* sourcedev */ ,
               TouchPointInfoPtr /* ti */ ,
               XID /* resource */ ,
               TouchOwnershipEvent * /* ev */ );

extern _X_HIDDEN void XI2EventSwap(xGenericEvent * /* from */ ,
                                   xGenericEvent * /* to */ );

/* For an event such as MappingNotify which affects client interpretation
 * of input events sent by device dev, should we notify the client, or
 * would it merely be irrelevant and confusing? */
extern int
 XIShouldNotify(ClientPtr client, DeviceIntPtr dev);

extern void
 XISendDeviceChangedEvent(DeviceIntPtr device, DeviceChangedEvent *dce);

extern int

XISetEventMask(DeviceIntPtr dev, WindowPtr win, ClientPtr client,
               unsigned int len, unsigned char *mask);

extern int
 XICheckInvalidMaskBits(ClientPtr client, unsigned char *mask, int len);

#endif                          /* EXEVENTS_H */
