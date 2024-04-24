/************************************************************

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

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

#ifndef INPUTSTRUCT_H
#define INPUTSTRUCT_H

#include <X11/extensions/XI2proto.h>

#include <pixman.h>
#include "input.h"
#include "window.h"
#include "dixstruct.h"
#include "cursorstr.h"
#include "geext.h"
#include "privates.h"

extern _X_EXPORT void AssignTypeAndName(DeviceIntPtr dev,
                                        Atom type,
                                        const char *name);

#define BitIsOn(ptr, bit) (!!(((const BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7))))
#define SetBit(ptr, bit)  (((BYTE *) (ptr))[(bit)>>3] |= (1 << ((bit) & 7)))
#define ClearBit(ptr, bit) (((BYTE *)(ptr))[(bit)>>3] &= ~(1 << ((bit) & 7)))
extern _X_EXPORT int CountBits(const uint8_t * mask, int len);

#define SameClient(obj,client) \
	(CLIENT_BITS((obj)->resource) == (client)->clientAsMask)

#define EMASKSIZE	(MAXDEVICES + 2)

/* This is the last XI2 event supported by the server. If you add
 * events to the protocol, the server will not support these events until
 * this number here is bumped.
 */
#define XI2LASTEVENT    XI_GestureSwipeEnd
#define XI2MASKSIZE     ((XI2LASTEVENT >> 3) + 1)       /* no of bytes for masks */

/**
 * Scroll types for ::SetScrollValuator and the scroll type in the
 * ::ScrollInfoPtr.
 */
enum ScrollType {
    SCROLL_TYPE_NONE = 0,           /**< Not a scrolling valuator */
    SCROLL_TYPE_VERTICAL = 8,
    SCROLL_TYPE_HORIZONTAL = 9,
};

/**
 * This struct stores the core event mask for each client except the client
 * that created the window.
 *
 * Each window that has events selected from other clients has at least one of
 * these masks. If multiple clients selected for events on the same window,
 * these masks are in a linked list.
 *
 * The event mask for the client that created the window is stored in
 * win->eventMask instead.
 *
 * The resource id is simply a fake client ID to associate this mask with a
 * client.
 *
 * Kludge: OtherClients and InputClients must be compatible, see code.
 */
typedef struct _OtherClients {
    OtherClientsPtr next;     /**< Pointer to the next mask */
    XID resource;                 /**< id for putting into resource manager */
    Mask mask;                /**< Core event mask */
} OtherClients;

/**
 * This struct stores the XI event mask for each client.
 *
 * Each window that has events selected has at least one of these masks. If
 * multiple client selected for events on the same window, these masks are in
 * a linked list.
 */
typedef struct _InputClients {
    InputClientsPtr next;     /**< Pointer to the next mask */
    XID resource;                 /**< id for putting into resource manager */
    Mask mask[EMASKSIZE];                /**< Actual XI event mask, deviceid is index */
    /** XI2 event masks. One per device, each bit is a mask of (1 << type) */
    struct _XI2Mask *xi2mask;
} InputClients;

/**
 * Combined XI event masks from all devices.
 *
 * This is the XI equivalent of the deliverableEvents, eventMask and
 * dontPropagate mask of the WindowRec (or WindowOptRec).
 *
 * A window that has an XI client selecting for events has exactly one
 * OtherInputMasks struct and exactly one InputClients struct hanging off
 * inputClients. Each further client appends to the inputClients list.
 * Each Mask field is per-device, with the device id as the index.
 * Exception: for non-device events (Presence events), the MAXDEVICES
 * deviceid is used.
 */
typedef struct _OtherInputMasks {
    /**
     * Bitwise OR of all masks by all clients and the window's parent's masks.
     */
    Mask deliverableEvents[EMASKSIZE];
    /**
     * Bitwise OR of all masks by all clients on this window.
     */
    Mask inputEvents[EMASKSIZE];
    /** The do-not-propagate masks for each device. */
    Mask dontPropagateMask[EMASKSIZE];
    /** The clients that selected for events */
    InputClientsPtr inputClients;
    /* XI2 event masks. One per device, each bit is a mask of (1 << type) */
    struct _XI2Mask *xi2mask;
} OtherInputMasks;

/*
 * The following structure gets used for both active and passive grabs. For
 * active grabs some of the fields (e.g. modifiers) are not used. However,
 * that is not much waste since there aren't many active grabs (one per
 * keyboard/pointer device) going at once in the server.
 */

#define MasksPerDetailMask 8    /* 256 keycodes and 256 possible
                                   modifier combinations, but only
                                   3 buttons. */

typedef struct _DetailRec {     /* Grab details may be bit masks */
    unsigned int exact;
    Mask *pMask;
} DetailRec;

union _GrabMask {
    Mask core;
    Mask xi;
    struct _XI2Mask *xi2mask;
};

/**
 * Central struct for device grabs.
 * The same struct is used for both core grabs and device grabs, with
 * different fields being set.
 * If the grab is a core grab (GrabPointer/GrabKeyboard), then the eventMask
 * is a combination of standard event masks (i.e. PointerMotionMask |
 * ButtonPressMask).
 * If the grab is a device grab (GrabDevice), then the eventMask is a
 * combination of event masks for a given XI event type (see SetEventInfo).
 *
 * If the grab is a result of a ButtonPress, then eventMask is the core mask
 * and deviceMask is set to the XI event mask for the grab.
 */
typedef struct _GrabRec {
    GrabPtr next;               /* for chain of passive grabs */
    XID resource;
    DeviceIntPtr device;
    WindowPtr window;
    unsigned ownerEvents:1;
    unsigned keyboardMode:1;
    unsigned pointerMode:1;
    enum InputLevel grabtype;
    CARD8 type;                 /* event type for passive grabs, 0 for active grabs */
    DetailRec modifiersDetail;
    DeviceIntPtr modifierDevice;
    DetailRec detail;           /* key or button */
    WindowPtr confineTo;        /* always NULL for keyboards */
    CursorPtr cursor;           /* always NULL for keyboards */
    Mask eventMask;
    Mask deviceMask;
    /* XI2 event masks. One per device, each bit is a mask of (1 << type) */
    struct _XI2Mask *xi2mask;
} GrabRec;

/**
 * Sprite information for a device.
 */
typedef struct _SpriteRec {
    CursorPtr current;
    BoxRec hotLimits;           /* logical constraints of hot spot */
    Bool confined;              /* confined to screen */
    RegionPtr hotShape;         /* additional logical shape constraint */
    BoxRec physLimits;          /* physical constraints of hot spot */
    WindowPtr win;              /* window of logical position */
    HotSpot hot;                /* logical pointer position */
    HotSpot hotPhys;            /* physical pointer position */
#ifdef PANORAMIX
    ScreenPtr screen;           /* all others are in Screen 0 coordinates */
    RegionRec Reg1;             /* Region 1 for confining motion */
    RegionRec Reg2;             /* Region 2 for confining virtual motion */
    WindowPtr windows[MAXSCREENS];
    WindowPtr confineWin;       /* confine window */
#endif
    /* The window trace information is used at dix/events.c to avoid having
     * to compute all the windows between the root and the current pointer
     * window each time a button or key goes down. The grabs on each of those
     * windows must be checked.
     * spriteTraces should only be used at dix/events.c! */
    WindowPtr *spriteTrace;
    int spriteTraceSize;
    int spriteTraceGood;

    /* Due to delays between event generation and event processing, it is
     * possible that the pointer has crossed screen boundaries between the
     * time in which it begins generating events and the time when
     * those events are processed.
     *
     * pEnqueueScreen: screen the pointer was on when the event was generated
     * pDequeueScreen: screen the pointer was on when the event is processed
     */
    ScreenPtr pEnqueueScreen;
    ScreenPtr pDequeueScreen;

} SpriteRec;

typedef struct _KeyClassRec {
    int sourceid;
    CARD8 down[DOWN_LENGTH];
    CARD8 postdown[DOWN_LENGTH];
    int modifierKeyCount[8];
    struct _XkbSrvInfo *xkbInfo;
} KeyClassRec, *KeyClassPtr;

typedef struct _ScrollInfo {
    enum ScrollType type;
    double increment;
    int flags;
} ScrollInfo, *ScrollInfoPtr;

typedef struct _AxisInfo {
    int resolution;
    int min_resolution;
    int max_resolution;
    int min_value;
    int max_value;
    Atom label;
    CARD8 mode;
    ScrollInfo scroll;
} AxisInfo, *AxisInfoPtr;

typedef struct _ValuatorAccelerationRec {
    int number;
    PointerAccelSchemeProc AccelSchemeProc;
    void *accelData;            /* at disposal of AccelScheme */
    PointerAccelSchemeInitProc AccelInitProc;
    DeviceCallbackProc AccelCleanupProc;
} ValuatorAccelerationRec, *ValuatorAccelerationPtr;

typedef struct _ValuatorClassRec {
    int sourceid;
    int numMotionEvents;
    int first_motion;
    int last_motion;
    void *motion;               /* motion history buffer. Different layout
                                   for MDs and SDs! */
    WindowPtr motionHintWindow;

    AxisInfoPtr axes;
    unsigned short numAxes;
    double *axisVal;            /* always absolute, but device-coord system */
    ValuatorAccelerationRec accelScheme;
    int h_scroll_axis;          /* horiz smooth-scrolling axis */
    int v_scroll_axis;          /* vert smooth-scrolling axis */
} ValuatorClassRec;

typedef struct _TouchListener {
    XID listener;           /* grabs/event selection IDs receiving
                             * events for this touch */
    int resource_type;      /* listener's resource type */
    enum TouchListenerType type;
    enum TouchListenerState state;
    enum InputLevel level;  /* matters only for emulating touches */
    WindowPtr window;
    GrabPtr grab;
} TouchListener;

typedef struct _TouchPointInfo {
    uint32_t client_id;         /* touch ID as seen in client events */
    int sourceid;               /* Source device's ID for this touchpoint */
    Bool active;                /* whether or not the touch is active */
    Bool pending_finish;        /* true if the touch is physically inactive
                                 * but still owned by a grab */
    SpriteRec sprite;           /* window trace for delivery */
    ValuatorMask *valuators;    /* last recorded axis values */
    TouchListener *listeners;   /* set of listeners */
    int num_listeners;
    int num_grabs;              /* number of open grabs on this touch
                                 * which have not accepted or rejected */
    Bool emulate_pointer;
    DeviceEvent *history;       /* History of events on this touchpoint */
    size_t history_elements;    /* Number of current elements in history */
    size_t history_size;        /* Size of history in elements */
} TouchPointInfoRec;

typedef struct _DDXTouchPointInfo {
    uint32_t client_id;         /* touch ID as seen in client events */
    Bool active;                /* whether or not the touch is active */
    uint32_t ddx_id;            /* touch ID given by the DDX */
    Bool emulate_pointer;

    ValuatorMask *valuators;    /* last axis values as posted, pre-transform */
} DDXTouchPointInfoRec;

typedef struct _TouchClassRec {
    int sourceid;
    TouchPointInfoPtr touches;
    unsigned short num_touches; /* number of allocated touches */
    unsigned short max_touches; /* maximum number of touches, may be 0 */
    CARD8 mode;                 /* ::XIDirectTouch, XIDependentTouch */
    /* for pointer-emulation */
    CARD8 buttonsDown;          /* number of buttons down */
    unsigned short state;       /* logical button state */
    Mask motionMask;
} TouchClassRec;

typedef struct _GestureListener {
    XID listener;           /* grabs/event selection IDs receiving
                             * events for this gesture */
    int resource_type;      /* listener's resource type */
    enum GestureListenerType type;
    WindowPtr window;
    GrabPtr grab;
} GestureListener;

typedef struct _GestureInfo {
    int sourceid;               /* Source device's ID for this gesture */
    Bool active;                /* whether or not the gesture is active */
    uint8_t type;               /* Gesture type: either ET_GesturePinchBegin or
                                   ET_GestureSwipeBegin. Valid if active == TRUE */
    int num_touches;            /* The number of touches in the gesture */
    SpriteRec sprite;           /* window trace for delivery */
    GestureListener listener;   /* the listener that will receive events */
    Bool has_listener;          /* true if listener has been setup already */
} GestureInfoRec;

typedef struct _GestureClassRec {
    int sourceid;
    GestureInfoRec gesture;
    unsigned short max_touches; /* maximum number of touches, may be 0 */
} GestureClassRec;

typedef struct _ButtonClassRec {
    int sourceid;
    CARD8 numButtons;
    CARD8 buttonsDown;          /* number of buttons currently down
                                   This counts logical buttons, not
                                   physical ones, i.e if some buttons
                                   are mapped to 0, they're not counted
                                   here */
    unsigned short state;
    Mask motionMask;
    CARD8 down[DOWN_LENGTH];
    CARD8 postdown[DOWN_LENGTH];
    CARD8 map[MAP_LENGTH];
    union _XkbAction *xkb_acts;
    Atom labels[MAX_BUTTONS];
} ButtonClassRec, *ButtonClassPtr;

typedef struct _FocusClassRec {
    int sourceid;
    WindowPtr win;              /* May be set to a int constant (e.g. PointerRootWin)! */
    int revert;
    TimeStamp time;
    WindowPtr *trace;
    int traceSize;
    int traceGood;
} FocusClassRec, *FocusClassPtr;

typedef struct _ProximityClassRec {
    int sourceid;
    char in_proximity;
} ProximityClassRec, *ProximityClassPtr;

typedef struct _KbdFeedbackClassRec *KbdFeedbackPtr;
typedef struct _PtrFeedbackClassRec *PtrFeedbackPtr;
typedef struct _IntegerFeedbackClassRec *IntegerFeedbackPtr;
typedef struct _StringFeedbackClassRec *StringFeedbackPtr;
typedef struct _BellFeedbackClassRec *BellFeedbackPtr;
typedef struct _LedFeedbackClassRec *LedFeedbackPtr;

typedef struct _KbdFeedbackClassRec {
    BellProcPtr BellProc;
    KbdCtrlProcPtr CtrlProc;
    KeybdCtrl ctrl;
    KbdFeedbackPtr next;
    struct _XkbSrvLedInfo *xkb_sli;
} KbdFeedbackClassRec;

typedef struct _PtrFeedbackClassRec {
    PtrCtrlProcPtr CtrlProc;
    PtrCtrl ctrl;
    PtrFeedbackPtr next;
} PtrFeedbackClassRec;

typedef struct _IntegerFeedbackClassRec {
    IntegerCtrlProcPtr CtrlProc;
    IntegerCtrl ctrl;
    IntegerFeedbackPtr next;
} IntegerFeedbackClassRec;

typedef struct _StringFeedbackClassRec {
    StringCtrlProcPtr CtrlProc;
    StringCtrl ctrl;
    StringFeedbackPtr next;
} StringFeedbackClassRec;

typedef struct _BellFeedbackClassRec {
    BellProcPtr BellProc;
    BellCtrlProcPtr CtrlProc;
    BellCtrl ctrl;
    BellFeedbackPtr next;
} BellFeedbackClassRec;

typedef struct _LedFeedbackClassRec {
    LedCtrlProcPtr CtrlProc;
    LedCtrl ctrl;
    LedFeedbackPtr next;
    struct _XkbSrvLedInfo *xkb_sli;
} LedFeedbackClassRec;

typedef struct _ClassesRec {
    KeyClassPtr key;
    ValuatorClassPtr valuator;
    TouchClassPtr touch;
    GestureClassPtr gesture;
    ButtonClassPtr button;
    FocusClassPtr focus;
    ProximityClassPtr proximity;
    KbdFeedbackPtr kbdfeed;
    PtrFeedbackPtr ptrfeed;
    IntegerFeedbackPtr intfeed;
    StringFeedbackPtr stringfeed;
    BellFeedbackPtr bell;
    LedFeedbackPtr leds;
} ClassesRec;

/* Device properties */
typedef struct _XIPropertyValue {
    Atom type;                  /* ignored by server */
    short format;               /* format of data for swapping - 8,16,32 */
    long size;                  /* size of data in (format/8) bytes */
    void *data;                 /* private to client */
} XIPropertyValueRec;

typedef struct _XIProperty {
    struct _XIProperty *next;
    Atom propertyName;
    BOOL deletable;             /* clients can delete this prop? */
    XIPropertyValueRec value;
} XIPropertyRec;

typedef XIPropertyRec *XIPropertyPtr;
typedef XIPropertyValueRec *XIPropertyValuePtr;

typedef struct _XIPropertyHandler {
    struct _XIPropertyHandler *next;
    long id;
    int (*SetProperty) (DeviceIntPtr dev,
                        Atom property, XIPropertyValuePtr prop, BOOL checkonly);
    int (*GetProperty) (DeviceIntPtr dev, Atom property);
    int (*DeleteProperty) (DeviceIntPtr dev, Atom property);
} XIPropertyHandler, *XIPropertyHandlerPtr;

/* states for devices */

#define NOT_GRABBED		0
#define THAWED			1
#define THAWED_BOTH		2       /* not a real state */
#define FREEZE_NEXT_EVENT	3
#define FREEZE_BOTH_NEXT_EVENT	4
#define FROZEN			5       /* any state >= has device frozen */
#define FROZEN_NO_EVENT		5
#define FROZEN_WITH_EVENT	6
#define THAW_OTHERS		7

typedef struct _GrabInfoRec {
    TimeStamp grabTime;
    Bool fromPassiveGrab;       /* true if from passive grab */
    Bool implicitGrab;          /* implicit from ButtonPress */
    GrabPtr unused;             /* Kept for ABI stability, remove soon */
    GrabPtr grab;
    CARD8 activatingKey;
    void (*ActivateGrab) (DeviceIntPtr /*device */ ,
                          GrabPtr /*grab */ ,
                          TimeStamp /*time */ ,
                          Bool /*autoGrab */ );
    void (*DeactivateGrab) (DeviceIntPtr /*device */ );
    struct {
        Bool frozen;
        int state;
        GrabPtr other;          /* if other grab has this frozen */
        InternalEvent *event;   /* saved to be replayed */
    } sync;
} GrabInfoRec, *GrabInfoPtr;

typedef struct _SpriteInfoRec {
    /* sprite must always point to a valid sprite. For devices sharing the
     * sprite, let sprite point to a paired spriteOwner's sprite. */
    SpritePtr sprite;           /* sprite information */
    Bool spriteOwner;           /* True if device owns the sprite */
    DeviceIntPtr paired;        /* The paired device. Keyboard if
                                   spriteOwner is TRUE, otherwise the
                                   pointer that owns the sprite. */

    /* keep states for animated cursor */
    struct {
        CursorPtr pCursor;
        ScreenPtr pScreen;
        int elt;
    } anim;
} SpriteInfoRec, *SpriteInfoPtr;

/* device types */
#define MASTER_POINTER          1
#define MASTER_KEYBOARD         2
#define SLAVE                   3
/* special types for GetMaster */
#define MASTER_ATTACHED         4       /* Master for this device */
#define KEYBOARD_OR_FLOAT       5       /* Keyboard master for this device or this device if floating */
#define POINTER_OR_FLOAT        6       /* Pointer master for this device or this device if floating */

typedef struct _DeviceIntRec {
    DeviceRec public;
    DeviceIntPtr next;
    Bool startup;               /* true if needs to be turned on at
                                   server initialization time */
    DeviceProc deviceProc;      /* proc(DevicePtr, DEVICE_xx). It is
                                   used to initialize, turn on, or
                                   turn off the device */
    Bool inited;                /* TRUE if INIT returns Success */
    Bool enabled;               /* TRUE if ON returns Success */
    Bool coreEvents;            /* TRUE if device also sends core */
    GrabInfoRec deviceGrab;     /* grab on the device */
    int type;                   /* MASTER_POINTER, MASTER_KEYBOARD, SLAVE */
    Atom xinput_type;
    char *name;
    int id;
    KeyClassPtr key;
    ValuatorClassPtr valuator;
    TouchClassPtr touch;
    GestureClassPtr gesture;
    ButtonClassPtr button;
    FocusClassPtr focus;
    ProximityClassPtr proximity;
    KbdFeedbackPtr kbdfeed;
    PtrFeedbackPtr ptrfeed;
    IntegerFeedbackPtr intfeed;
    StringFeedbackPtr stringfeed;
    BellFeedbackPtr bell;
    LedFeedbackPtr leds;
    struct _XkbInterest *xkb_interest;
    char *config_info;          /* used by the hotplug layer */
    ClassesPtr unused_classes;  /* for master devices */
    int saved_master_id;        /* for slaves while grabbed */
    PrivateRec *devPrivates;
    DeviceUnwrapProc unwrapProc;
    SpriteInfoPtr spriteInfo;
    DeviceIntPtr master;        /* master device */
    DeviceIntPtr lastSlave;     /* last slave device used */

    /* last valuator values recorded, not posted to client;
     * for slave devices, valuators is in device coordinates, mapped to the
     * desktop
     * for master devices, valuators is in desktop coordinates.
     * see dix/getevents.c
     * remainder supports acceleration
     */
    struct {
        double valuators[MAX_VALUATORS];
        int numValuators;
        DeviceIntPtr slave;
        ValuatorMask *scroll;
        int num_touches;        /* size of the touches array */
        DDXTouchPointInfoPtr touches;
    } last;

    /* Input device property handling. */
    struct {
        XIPropertyPtr properties;
        XIPropertyHandlerPtr handlers;  /* NULL-terminated */
    } properties;

    /* coordinate transformation matrix for relative movement. Matrix with
     * the translation component dropped */
    struct pixman_f_transform relative_transform;
    /* scale matrix for absolute devices, this is the combined matrix of
       [1/scale] . [transform] . [scale]. See DeviceSetTransform */
    struct pixman_f_transform scale_and_transform;

    /* XTest related master device id */
    int xtest_master_id;

    struct _SyncCounter *idle_counter;
} DeviceIntRec;

typedef struct {
    int numDevices;             /* total number of devices */
    DeviceIntPtr devices;       /* all devices turned on */
    DeviceIntPtr off_devices;   /* all devices turned off */
    DeviceIntPtr keyboard;      /* the main one for the server */
    DeviceIntPtr pointer;
    DeviceIntPtr all_devices;
    DeviceIntPtr all_master_devices;
} InputInfo;

extern _X_EXPORT InputInfo inputInfo;

/* for keeping the events for devices grabbed synchronously */
typedef struct _QdEvent *QdEventPtr;
typedef struct _QdEvent {
    struct xorg_list next;
    DeviceIntPtr device;
    ScreenPtr pScreen;          /* what screen the pointer was on */
    unsigned long months;       /* milliseconds is in the event */
    InternalEvent *event;
} QdEventRec;

/**
 * syncEvents is the global structure for queued events.
 *
 * Devices can be frozen through GrabModeSync pointer grabs. If this is the
 * case, events from these devices are added to "pending" instead of being
 * processed normally. When the device is unfrozen, events in "pending" are
 * replayed and processed as if they would come from the device directly.
 */
typedef struct _EventSyncInfo {
    struct xorg_list pending;

    /** The device to replay events for. Only set in AllowEvents(), in which
     * case it is set to the device specified in the request. */
    DeviceIntPtr replayDev;     /* kludgy rock to put flag for */

    /**
     * The window the events are supposed to be replayed on.
     * This window may be set to the grab's window (but only when
     * Replay{Pointer|Keyboard} is given in the XAllowEvents()
     * request. */
    WindowPtr replayWin;        /*   ComputeFreezes            */
    /**
     * Flag to indicate whether we're in the process of
     * replaying events. Only set in ComputeFreezes(). */
    Bool playingEvents;
    TimeStamp time;
} EventSyncInfoRec, *EventSyncInfoPtr;

extern EventSyncInfoRec syncEvents;

/**
 * Given a sprite, returns the window at the bottom of the trace (i.e. the
 * furthest window from the root).
 */
static inline WindowPtr
DeepestSpriteWin(SpritePtr sprite)
{
    assert(sprite->spriteTraceGood > 0);
    return sprite->spriteTrace[sprite->spriteTraceGood - 1];
}

struct _XI2Mask {
    unsigned char **masks;      /* event mask in masks[deviceid][event type byte] */
    size_t nmasks;              /* number of masks */
    size_t mask_size;           /* size of each mask in bytes */
};

#endif                          /* INPUTSTRUCT_H */
