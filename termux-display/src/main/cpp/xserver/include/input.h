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

#ifndef INPUT_H
#define INPUT_H

#include "misc.h"
#include "screenint.h"
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <stdint.h>
#include "window.h"             /* for WindowPtr */
#include "xkbrules.h"
#include "events.h"
#include "list.h"
#include "os.h"
#include <X11/extensions/XI2.h>

#define DEFAULT_KEYBOARD_CLICK 	0
#define DEFAULT_BELL		50
#define DEFAULT_BELL_PITCH	400
#define DEFAULT_BELL_DURATION	100
#define DEFAULT_AUTOREPEAT	TRUE
#define DEFAULT_AUTOREPEATS	{\
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define DEFAULT_LEDS		0x0     /* all off */
#define DEFAULT_LEDS_MASK	0xffffffff      /* 32 */
#define DEFAULT_INT_RESOLUTION		1000
#define DEFAULT_INT_MIN_VALUE		0
#define DEFAULT_INT_MAX_VALUE		100
#define DEFAULT_INT_DISPLAYED		0

#define DEFAULT_PTR_NUMERATOR	2
#define DEFAULT_PTR_DENOMINATOR	1
#define DEFAULT_PTR_THRESHOLD	4

#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3
#define DEVICE_ABORT	4

#define POINTER_RELATIVE	(1 << 1)
#define POINTER_ABSOLUTE	(1 << 2)
#define POINTER_ACCELERATE	(1 << 3)
#define POINTER_SCREEN		(1 << 4)        /* Data in screen coordinates */
#define POINTER_NORAW		(1 << 5)        /* Don't generate RawEvents */
#define POINTER_EMULATED	(1 << 6)        /* Event was emulated from another event */
#define POINTER_DESKTOP		(1 << 7)        /* Data in desktop coordinates */
#define POINTER_RAWONLY         (1 << 8)        /* Only generate RawEvents */

/* GetTouchEvent flags */
#define TOUCH_ACCEPT            (1 << 0)
#define TOUCH_REJECT            (1 << 1)
#define TOUCH_PENDING_END       (1 << 2)
#define TOUCH_CLIENT_ID         (1 << 3)        /* touch ID is the client-visible id */
#define TOUCH_REPLAYING         (1 << 4)        /* event is being replayed */
#define TOUCH_POINTER_EMULATED  (1 << 5)        /* touch event may be pointer emulated */
#define TOUCH_END               (1 << 6)        /* really end this touch now */

/* GetGestureEvent flags */
#define GESTURE_CANCELLED       (1 << 0)

/*int constants for pointer acceleration schemes*/
#define PtrAccelNoOp            0
#define PtrAccelPredictable     1
#define PtrAccelLightweight     2
#define PtrAccelDefault         PtrAccelPredictable

#define MAX_VALUATORS 36
/* Maximum number of valuators, divided by six, rounded up, to get number
 * of events. */
#define MAX_VALUATOR_EVENTS 6
#define MAX_BUTTONS 256         /* completely arbitrarily chosen */

#define NO_AXIS_LIMITS -1

#define MAP_LENGTH	MAX_BUTTONS
#define DOWN_LENGTH	(MAX_BUTTONS/8)      /* 256/8 => number of bytes to hold 256 bits */
#define NullGrab ((GrabPtr)NULL)
#define PointerRootWin ((WindowPtr)PointerRoot)
#define NoneWin ((WindowPtr)None)
#define NullDevice ((DevicePtr)NULL)

#ifndef FollowKeyboard
#define FollowKeyboard 		3
#endif
#ifndef FollowKeyboardWin
#define FollowKeyboardWin  ((WindowPtr) FollowKeyboard)
#endif
#ifndef RevertToFollowKeyboard
#define RevertToFollowKeyboard	3
#endif

enum InputLevel {
    CORE = 1,
    XI = 2,
    XI2 = 3,
};

typedef unsigned long Leds;
typedef struct _OtherClients *OtherClientsPtr;
typedef struct _InputClients *InputClientsPtr;
typedef struct _DeviceIntRec *DeviceIntPtr;
typedef struct _ValuatorClassRec *ValuatorClassPtr;
typedef struct _ClassesRec *ClassesPtr;
typedef struct _SpriteRec *SpritePtr;
typedef struct _TouchClassRec *TouchClassPtr;
typedef struct _GestureClassRec *GestureClassPtr;
typedef struct _TouchPointInfo *TouchPointInfoPtr;
typedef struct _GestureInfo *GestureInfoPtr;
typedef struct _DDXTouchPointInfo *DDXTouchPointInfoPtr;
typedef union _GrabMask GrabMask;

typedef struct _ValuatorMask ValuatorMask;

/* The DIX stores incoming input events in this list */
extern InternalEvent *InputEventList;

typedef int (*DeviceProc) (DeviceIntPtr /*device */ ,
                           int /*what */ );

typedef void (*ProcessInputProc) (InternalEvent * /*event */ ,
                                  DeviceIntPtr /*device */ );

typedef Bool (*DeviceHandleProc) (DeviceIntPtr /*device */ ,
                                  void *        /*data */
    );

typedef void (*DeviceUnwrapProc) (DeviceIntPtr /*device */ ,
                                  DeviceHandleProc /*proc */ ,
                                  void *        /*data */
    );

/* pointer acceleration handling */
typedef void (*PointerAccelSchemeProc) (DeviceIntPtr /*device */ ,
                                        ValuatorMask * /*valuators */ ,
                                        CARD32 /*evtime */ );

typedef void (*DeviceCallbackProc) (DeviceIntPtr /*pDev */ );

struct _ValuatorAccelerationRec;
typedef Bool (*PointerAccelSchemeInitProc) (DeviceIntPtr /*dev */ ,
                                            struct _ValuatorAccelerationRec *
                                            /*protoScheme */ );

typedef struct _DeviceRec {
    void *devicePrivate;
    ProcessInputProc processInputProc;  /* current */
    ProcessInputProc realInputProc;     /* deliver */
    ProcessInputProc enqueueInputProc;  /* enqueue */
    Bool on;                    /* used by DDX to keep state */
} DeviceRec, *DevicePtr;

typedef struct {
    int click, bell, bell_pitch, bell_duration;
    Bool autoRepeat;
    unsigned char autoRepeats[32];
    Leds leds;
    unsigned char id;
} KeybdCtrl;

typedef struct {
    KeySym *map;
    KeyCode minKeyCode, maxKeyCode;
    int mapWidth;
} KeySymsRec, *KeySymsPtr;

typedef struct {
    int num, den, threshold;
    unsigned char id;
} PtrCtrl;

typedef struct {
    int resolution, min_value, max_value;
    int integer_displayed;
    unsigned char id;
} IntegerCtrl;

typedef struct {
    int max_symbols, num_symbols_supported;
    int num_symbols_displayed;
    KeySym *symbols_supported;
    KeySym *symbols_displayed;
    unsigned char id;
} StringCtrl;

typedef struct {
    int percent, pitch, duration;
    unsigned char id;
} BellCtrl;

typedef struct {
    Leds led_values;
    Mask led_mask;
    unsigned char id;
} LedCtrl;

extern _X_EXPORT KeybdCtrl defaultKeyboardControl;
extern _X_EXPORT PtrCtrl defaultPointerControl;

typedef struct _InputOption InputOption;
typedef struct _XI2Mask XI2Mask;

typedef struct _InputAttributes {
    char *product;
    char *vendor;
    char *device;
    char *pnp_id;
    char *usb_id;
    char **tags;                /* null-terminated */
    uint32_t flags;
} InputAttributes;

#define ATTR_KEYBOARD (1<<0)
#define ATTR_POINTER (1<<1)
#define ATTR_JOYSTICK (1<<2)
#define ATTR_TABLET (1<<3)
#define ATTR_TOUCHPAD (1<<4)
#define ATTR_TOUCHSCREEN (1<<5)
#define ATTR_KEY (1<<6)
#define ATTR_TABLET_PAD (1<<7)

/* Key/Button has been run through all input processing and events sent to clients. */
#define KEY_PROCESSED 1
#define BUTTON_PROCESSED 1
/* Key/Button has not been fully processed, no events have been sent. */
#define KEY_POSTED 2
#define BUTTON_POSTED 2

extern _X_EXPORT void set_key_down(DeviceIntPtr pDev, int key_code, int type);
extern _X_EXPORT void set_key_up(DeviceIntPtr pDev, int key_code, int type);
extern _X_EXPORT int key_is_down(DeviceIntPtr pDev, int key_code, int type);
extern _X_EXPORT void set_button_down(DeviceIntPtr pDev, int button, int type);
extern _X_EXPORT void set_button_up(DeviceIntPtr pDev, int button, int type);
extern _X_EXPORT int button_is_down(DeviceIntPtr pDev, int button, int type);

extern void InitCoreDevices(void);
extern void InitXTestDevices(void);

extern _X_EXPORT DeviceIntPtr AddInputDevice(ClientPtr /*client */ ,
                                             DeviceProc /*deviceProc */ ,
                                             Bool /*autoStart */ );

extern _X_EXPORT Bool EnableDevice(DeviceIntPtr /*device */ ,
                                   BOOL /* sendevent */ );

extern _X_EXPORT Bool ActivateDevice(DeviceIntPtr /*device */ ,
                                     BOOL /* sendevent */ );

extern _X_EXPORT Bool DisableDevice(DeviceIntPtr /*device */ ,
                                    BOOL /* sendevent */ );
extern void DisableAllDevices(void);
extern int InitAndStartDevices(void);

extern void CloseDownDevices(void);
extern void AbortDevices(void);

extern void UndisplayDevices(void);

extern _X_EXPORT int RemoveDevice(DeviceIntPtr /*dev */ ,
                                  BOOL /* sendevent */ );

extern _X_EXPORT int NumMotionEvents(void);

extern _X_EXPORT int dixLookupDevice(DeviceIntPtr * /* dev */ ,
                                     int /* id */ ,
                                     ClientPtr /* client */ ,
                                     Mask /* access_mode */ );

extern _X_EXPORT void QueryMinMaxKeyCodes(KeyCode * /*minCode */ ,
                                          KeyCode * /*maxCode */ );

extern _X_EXPORT Bool InitButtonClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                  int /*numButtons */ ,
                                                  Atom * /* labels */ ,
                                                  CARD8 * /*map */ );

extern _X_INTERNAL ValuatorClassPtr AllocValuatorClass(ValuatorClassPtr src,
                                                       int numAxes);

extern _X_EXPORT Bool InitValuatorClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                    int /*numAxes */ ,
                                                    Atom * /* labels */ ,
                                                    int /*numMotionEvents */ ,
                                                    int /*mode */ );

extern _X_EXPORT Bool InitPointerAccelerationScheme(DeviceIntPtr /*dev */ ,
                                                    int /*scheme */ );

extern _X_EXPORT Bool InitFocusClassDeviceStruct(DeviceIntPtr /*device */ );

extern _X_EXPORT Bool InitTouchClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                 unsigned int /*max_touches */ ,
                                                 unsigned int /*mode */ ,
                                                 unsigned int /*numAxes */ );

extern _X_EXPORT Bool InitGestureClassDeviceStruct(DeviceIntPtr device,
                                                   unsigned int max_touches);

typedef void (*BellProcPtr) (int percent,
                             DeviceIntPtr device,
                             void *ctrl,
                             int feedbackClass);

typedef void (*KbdCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                KeybdCtrl * /*ctrl */ );

typedef void (*PtrCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                PtrCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitPtrFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                       ,
                                                       PtrCtrlProcPtr
                                                       /*controlProc */ );

typedef void (*StringCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                   StringCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitStringFeedbackClassDeviceStruct(DeviceIntPtr
                                                          /*device */ ,
                                                          StringCtrlProcPtr
                                                          /*controlProc */ ,
                                                          int /*max_symbols */ ,
                                                          int
                                                          /*num_symbols_supported */
                                                          ,
                                                          KeySym * /*symbols */
                                                          );

typedef void (*BellCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                 BellCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitBellFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                        ,
                                                        BellProcPtr
                                                        /*bellProc */ ,
                                                        BellCtrlProcPtr
                                                        /*controlProc */ );

typedef void (*LedCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                LedCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitLedFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                       ,
                                                       LedCtrlProcPtr
                                                       /*controlProc */ );

typedef void (*IntegerCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                    IntegerCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitIntegerFeedbackClassDeviceStruct(DeviceIntPtr
                                                           /*device */ ,
                                                           IntegerCtrlProcPtr
                                                           /*controlProc */ );

extern _X_EXPORT Bool InitPointerDeviceStruct(DevicePtr /*device */ ,
                                              CARD8 * /*map */ ,
                                              int /*numButtons */ ,
                                              Atom * /* btn_labels */ ,
                                              PtrCtrlProcPtr /*controlProc */ ,
                                              int /*numMotionEvents */ ,
                                              int /*numAxes */ ,
                                              Atom * /* axes_labels */ );

extern _X_EXPORT Bool InitKeyboardDeviceStruct(DeviceIntPtr /*device */ ,
                                               XkbRMLVOSet * /*rmlvo */ ,
                                               BellProcPtr /*bellProc */ ,
                                               KbdCtrlProcPtr /*controlProc */
                                               );

extern _X_EXPORT Bool InitKeyboardDeviceStructFromString(DeviceIntPtr dev,
							 const char *keymap,
							 int keymap_length,
							 BellProcPtr bell_func,
							 KbdCtrlProcPtr ctrl_func);

extern int ApplyPointerMapping(DeviceIntPtr /* pDev */ ,
                               CARD8 * /* map */ ,
                               int /* len */ ,
                               ClientPtr /* client */ );

extern Bool BadDeviceMap(BYTE * /*buff */ ,
                         int /*length */ ,
                         unsigned /*low */ ,
                         unsigned /*high */ ,
                         XID * /*errval */ );

extern void NoteLedState(DeviceIntPtr /*keybd */ ,
                         int /*led */ ,
                         Bool /*on */ );

extern void MaybeStopHint(DeviceIntPtr /*device */ ,
                          ClientPtr /*client */ );

extern void ProcessPointerEvent(InternalEvent * /* ev */ ,
                                DeviceIntPtr /*mouse */ );

extern void ProcessKeyboardEvent(InternalEvent * /*ev */ ,
                                 DeviceIntPtr /*keybd */ );

extern _X_EXPORT void ProcessInputEvents(void);

extern _X_EXPORT void InitInput(int /*argc */ ,
                                char ** /*argv */ );
extern _X_EXPORT void CloseInput(void);

extern _X_EXPORT int GetMaximumEventsNum(void);

extern _X_EXPORT InternalEvent *InitEventList(int num_events);
extern _X_EXPORT void FreeEventList(InternalEvent *list, int num_events);

extern void CreateClassesChangedEvent(InternalEvent *event,
                                      DeviceIntPtr master,
                                      DeviceIntPtr slave, int flags);

extern InternalEvent *UpdateFromMaster(InternalEvent *events,
                                       DeviceIntPtr pDev,
                                       int type, int *num_events);

extern _X_EXPORT int GetPointerEvents(InternalEvent *events,
                                      DeviceIntPtr pDev,
                                      int type,
                                      int buttons,
                                      int flags, const ValuatorMask *mask);

extern _X_EXPORT void QueuePointerEvents(DeviceIntPtr pDev,
                                         int type,
                                         int buttons,
                                         int flags, const ValuatorMask *mask);

extern _X_EXPORT int GetKeyboardEvents(InternalEvent *events,
                                       DeviceIntPtr pDev,
                                       int type,
                                       int key_code);

extern _X_EXPORT void QueueKeyboardEvents(DeviceIntPtr pDev,
                                          int type,
                                          int key_code);

extern int GetTouchEvents(InternalEvent *events,
                          DeviceIntPtr pDev,
                          uint32_t ddx_touchid,
                          uint16_t type,
                          uint32_t flags, const ValuatorMask *mask);

void QueueTouchEvents(DeviceIntPtr device,
                      int type,
                      uint32_t ddx_touchid,
                      int flags, const ValuatorMask *mask);

void InitGestureEvent(InternalEvent *ievent, DeviceIntPtr dev, CARD32 ms,
                      int type, uint16_t num_touches, uint32_t flags,
                      double delta_x, double delta_y,
                      double delta_unaccel_x, double delta_unaccel_y,
                      double scale, double delta_angle);

int GetGestureEvents(InternalEvent *events, DeviceIntPtr dev,
                     uint16_t type, uint16_t num_touches, uint32_t flags,
                     double delta_x, double delta_y,
                     double delta_unaccel_x,
                     double delta_unaccel_y,
                     double scale, double delta_angle);


void QueueGesturePinchEvents(DeviceIntPtr dev, uint16_t type,
                             uint16_t num_touches, uint32_t flags,
                             double delta_x, double delta_y,
                             double delta_unaccel_x,
                             double delta_unaccel_y,
                             double scale, double delta_angle);

void QueueGestureSwipeEvents(DeviceIntPtr dev, uint16_t type,
                             uint16_t num_touches, uint32_t flags,
                             double delta_x, double delta_y,
                             double delta_unaccel_x,
                             double delta_unaccel_y);

extern int GetTouchOwnershipEvents(InternalEvent *events,
                                   DeviceIntPtr pDev,
                                   TouchPointInfoPtr ti,
                                   uint8_t mode, XID resource, uint32_t flags);

extern void GetDixTouchEnd(InternalEvent *ievent,
                           DeviceIntPtr dev,
                           TouchPointInfoPtr ti,
                           uint32_t flags);

extern _X_EXPORT int GetProximityEvents(InternalEvent *events,
                                        DeviceIntPtr pDev,
                                        int type, const ValuatorMask *mask);

extern _X_EXPORT void QueueProximityEvents(DeviceIntPtr pDev,
                                           int type, const ValuatorMask *mask);

#ifdef PANORAMIX
_X_EXPORT
#endif
extern void PostSyntheticMotion(DeviceIntPtr pDev,
                                int x, int y, int screen, unsigned long time);

extern _X_EXPORT int GetMotionHistorySize(void);

extern _X_EXPORT void AllocateMotionHistory(DeviceIntPtr pDev);

extern _X_EXPORT int GetMotionHistory(DeviceIntPtr pDev,
                                      xTimecoord ** buff,
                                      unsigned long start,
                                      unsigned long stop,
                                      ScreenPtr pScreen, BOOL core);

extern void ReleaseButtonsAndKeys(DeviceIntPtr dev);

extern int AttachDevice(ClientPtr client,
                        DeviceIntPtr slave, DeviceIntPtr master);

extern _X_EXPORT DeviceIntPtr GetPairedDevice(DeviceIntPtr kbd);
extern _X_EXPORT DeviceIntPtr GetMaster(DeviceIntPtr dev, int type);

extern _X_EXPORT int AllocDevicePair(ClientPtr client,
                                     const char *name,
                                     DeviceIntPtr *ptr,
                                     DeviceIntPtr *keybd,
                                     DeviceProc ptr_proc,
                                     DeviceProc keybd_proc, Bool master);
extern void DeepCopyDeviceClasses(DeviceIntPtr from,
                                  DeviceIntPtr to, DeviceChangedEvent *dce);

/* Helper functions. */
extern _X_EXPORT int generate_modkeymap(ClientPtr client, DeviceIntPtr dev,
                                        KeyCode **modkeymap,
                                        int *max_keys_per_mod);
extern int change_modmap(ClientPtr client, DeviceIntPtr dev, KeyCode *map,
                         int max_keys_per_mod);
extern int AllocXTestDevice(ClientPtr client, const char *name,
                            DeviceIntPtr *ptr, DeviceIntPtr *keybd,
                            DeviceIntPtr master_ptr, DeviceIntPtr master_keybd);
extern BOOL IsXTestDevice(DeviceIntPtr dev, DeviceIntPtr master);
extern DeviceIntPtr GetXTestDevice(DeviceIntPtr master);
extern void SendDevicePresenceEvent(int deviceid, int type);
extern void DeliverDeviceClassesChangedEvent(int sourceid, Time time);
extern _X_EXPORT InputAttributes *DuplicateInputAttributes(InputAttributes *
                                                           attrs);
extern _X_EXPORT void FreeInputAttributes(InputAttributes * attrs);

enum TouchListenerState {
    TOUCH_LISTENER_AWAITING_BEGIN = 0, /**< Waiting for a TouchBegin event */
    TOUCH_LISTENER_AWAITING_OWNER,     /**< Waiting for a TouchOwnership event */
    TOUCH_LISTENER_EARLY_ACCEPT,       /**< Waiting for ownership, has already
                                            accepted */
    TOUCH_LISTENER_IS_OWNER,           /**< Is the current owner, hasn't
                                            accepted */
    TOUCH_LISTENER_HAS_ACCEPTED,       /**< Is the current owner, has accepted */
    TOUCH_LISTENER_HAS_END,            /**< Has already received the end event */
};

enum TouchListenerType {
    TOUCH_LISTENER_GRAB,
    TOUCH_LISTENER_POINTER_GRAB,
    TOUCH_LISTENER_REGULAR,
    TOUCH_LISTENER_POINTER_REGULAR,
};

enum GestureListenerType {
    GESTURE_LISTENER_GRAB,
    GESTURE_LISTENER_NONGESTURE_GRAB,
    GESTURE_LISTENER_REGULAR
};

extern void TouchInitDDXTouchPoint(DeviceIntPtr dev,
                                   DDXTouchPointInfoPtr ddxtouch);
extern DDXTouchPointInfoPtr TouchBeginDDXTouch(DeviceIntPtr dev,
                                               uint32_t ddx_id);
extern void TouchEndDDXTouch(DeviceIntPtr dev, DDXTouchPointInfoPtr ti);
extern DDXTouchPointInfoPtr TouchFindByDDXID(DeviceIntPtr dev,
                                             uint32_t ddx_id, Bool create);
extern Bool TouchInitTouchPoint(TouchClassPtr touch, ValuatorClassPtr v,
                                int index);
extern void TouchFreeTouchPoint(DeviceIntPtr dev, int index);
extern TouchPointInfoPtr TouchBeginTouch(DeviceIntPtr dev, int sourceid,
                                         uint32_t touchid,
                                         Bool emulate_pointer);
extern TouchPointInfoPtr TouchFindByClientID(DeviceIntPtr dev,
                                             uint32_t client_id);
extern void TouchEndTouch(DeviceIntPtr dev, TouchPointInfoPtr ti);
extern Bool TouchEventHistoryAllocate(TouchPointInfoPtr ti);
extern void TouchEventHistoryFree(TouchPointInfoPtr ti);
extern void TouchEventHistoryPush(TouchPointInfoPtr ti, const DeviceEvent *ev);
extern void TouchEventHistoryReplay(TouchPointInfoPtr ti, DeviceIntPtr dev,
                                    XID resource);
extern Bool TouchResourceIsOwner(TouchPointInfoPtr ti, XID resource);
extern void TouchAddListener(TouchPointInfoPtr ti, XID resource, int resource_type,
                             enum InputLevel level, enum TouchListenerType type,
                             enum TouchListenerState state, WindowPtr window, GrabPtr grab);
extern Bool TouchRemoveListener(TouchPointInfoPtr ti, XID resource);
extern void TouchSetupListeners(DeviceIntPtr dev, TouchPointInfoPtr ti,
                                InternalEvent *ev);
extern Bool TouchBuildSprite(DeviceIntPtr sourcedev, TouchPointInfoPtr ti,
                             InternalEvent *ev);
extern Bool TouchBuildDependentSpriteTrace(DeviceIntPtr dev, SpritePtr sprite);
extern int TouchConvertToPointerEvent(const InternalEvent *ev,
                                      InternalEvent *motion,
                                      InternalEvent *button);
extern int TouchGetPointerEventType(const InternalEvent *ev);
extern void TouchRemovePointerGrab(DeviceIntPtr dev);
extern void TouchListenerGone(XID resource);
extern int TouchListenerAcceptReject(DeviceIntPtr dev, TouchPointInfoPtr ti,
                                     int listener, int mode);
extern int TouchAcceptReject(ClientPtr client, DeviceIntPtr dev, int mode,
                             uint32_t touchid, Window grab_window, XID *error);
extern void TouchEndPhysicallyActiveTouches(DeviceIntPtr dev);
extern void TouchEmitTouchEnd(DeviceIntPtr dev, TouchPointInfoPtr ti, int flags, XID resource);
extern void TouchAcceptAndEnd(DeviceIntPtr dev, int touchid);

extern Bool GestureInitGestureInfo(GestureInfoPtr gesture);
extern GestureInfoPtr GestureBeginGesture(DeviceIntPtr dev, InternalEvent *ev);
extern GestureInfoPtr GestureFindActiveByEventType(DeviceIntPtr dev, int type);
extern void GestureEndGesture(GestureInfoPtr gi);
extern Bool GestureResourceIsOwner(GestureInfoPtr gi, XID resource);
extern void GestureAddListener(GestureInfoPtr gi, XID resource, int resource_type,
                               enum GestureListenerType type,
                               WindowPtr window, GrabPtr grab);
extern void GestureSetupListener(DeviceIntPtr dev, GestureInfoPtr gi,
                                 InternalEvent *ev);
extern Bool GestureBuildSprite(DeviceIntPtr sourcedev, GestureInfoPtr gi);
extern void GestureListenerGone(XID resource);
extern void GestureEndActiveGestures(DeviceIntPtr dev);
extern void GestureEmitGestureEndToOwner(DeviceIntPtr dev, GestureInfoPtr gi);
extern void ProcessGestureEvent(InternalEvent *ev, DeviceIntPtr dev);

/* misc event helpers */
extern void CopyPartialInternalEvent(InternalEvent* dst_event, const InternalEvent* src_event);
extern Mask GetEventMask(DeviceIntPtr dev, xEvent *ev, InputClientsPtr clients);
extern Mask GetEventFilter(DeviceIntPtr dev, xEvent *event);
extern Bool WindowXI2MaskIsset(DeviceIntPtr dev, WindowPtr win, xEvent *ev);
extern int GetXI2MaskByte(XI2Mask *mask, DeviceIntPtr dev, int event_type);
void FixUpEventFromWindow(SpritePtr pSprite,
                          xEvent *xE,
                          WindowPtr pWin, Window child, Bool calcChild);
extern Bool PointInBorderSize(WindowPtr pWin, int x, int y);
extern WindowPtr XYToWindow(SpritePtr pSprite, int x, int y);
extern int EventIsDeliverable(DeviceIntPtr dev, int evtype, WindowPtr win);
extern Bool ActivatePassiveGrab(DeviceIntPtr dev, GrabPtr grab,
                                InternalEvent *ev, InternalEvent *real_event);
extern void ActivateGrabNoDelivery(DeviceIntPtr dev, GrabPtr grab,
                                   InternalEvent *event,
                                   InternalEvent *real_event);
/**
 * Masks specifying the type of event to deliver for an InternalEvent; used
 * by EventIsDeliverable.
 * @defgroup EventIsDeliverable return flags
 * @{
 */
#define EVENT_XI1_MASK                (1 << 0) /**< XI1.x event */
#define EVENT_CORE_MASK               (1 << 1) /**< Core event */
#define EVENT_DONT_PROPAGATE_MASK     (1 << 2) /**< DontPropagate mask set */
#define EVENT_XI2_MASK                (1 << 3) /**< XI2 mask set on window */
/* @} */

enum EventDeliveryState {
    EVENT_DELIVERED,     /**< Event has been delivered to a client  */
    EVENT_NOT_DELIVERED, /**< Event was not delivered to any client */
    EVENT_SKIP,          /**< Event can be discarded by the caller  */
    EVENT_REJECTED,      /**< Event was rejected for delivery to the client */
};

/* Implemented by the DDX. */
extern _X_EXPORT int NewInputDeviceRequest(InputOption *options,
                                           InputAttributes * attrs,
                                           DeviceIntPtr *dev);
extern _X_EXPORT void DeleteInputDeviceRequest(DeviceIntPtr dev);
extern _X_EXPORT void RemoveInputDeviceTraces(const char *config_info);

extern _X_EXPORT void DDXRingBell(int volume, int pitch, int duration);

#define VALUATOR_MODE_ALL_AXES -1
extern _X_HIDDEN int valuator_get_mode(DeviceIntPtr dev, int axis);
extern _X_HIDDEN void valuator_set_mode(DeviceIntPtr dev, int axis, int mode);

/* Set to TRUE by default - os/utils.c sets it to FALSE on user request,
   xfixes/cursor.c uses it to determine if the cursor is enabled */
extern Bool EnableCursor;

/* Set to FALSE by default - ChangeWindowAttributes sets it to TRUE on
 * CWCursor, xfixes/cursor.c uses it to determine if the cursor is enabled
 */
extern Bool CursorVisible;

extern _X_EXPORT ValuatorMask *valuator_mask_new(int num_valuators);
extern _X_EXPORT void valuator_mask_free(ValuatorMask **mask);
extern _X_EXPORT void valuator_mask_set_range(ValuatorMask *mask,
                                              int first_valuator,
                                              int num_valuators,
                                              const int *valuators);
extern _X_EXPORT void valuator_mask_set(ValuatorMask *mask, int valuator,
                                        int data);
extern _X_EXPORT void valuator_mask_set_double(ValuatorMask *mask, int valuator,
                                               double data);
extern _X_EXPORT void valuator_mask_zero(ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_size(const ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_isset(const ValuatorMask *mask, int bit);
extern _X_EXPORT void valuator_mask_unset(ValuatorMask *mask, int bit);
extern _X_EXPORT int valuator_mask_num_valuators(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_copy(ValuatorMask *dest,
                                         const ValuatorMask *src);
extern _X_EXPORT int valuator_mask_get(const ValuatorMask *mask, int valnum);
extern _X_EXPORT double valuator_mask_get_double(const ValuatorMask *mask,
                                                 int valnum);
extern _X_EXPORT Bool valuator_mask_fetch(const ValuatorMask *mask,
                                          int valnum, int *val);
extern _X_EXPORT Bool valuator_mask_fetch_double(const ValuatorMask *mask,
                                                 int valnum, double *val);
extern _X_EXPORT Bool valuator_mask_has_unaccelerated(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_set_unaccelerated(ValuatorMask *mask,
                                                      int valuator,
                                                      double accel,
                                                      double unaccel);
extern _X_EXPORT void valuator_mask_set_absolute_unaccelerated(ValuatorMask *mask,
                                                               int valuator,
                                                               int absolute,
                                                               double unaccel);
extern _X_EXPORT double valuator_mask_get_accelerated(const ValuatorMask *mask,
                                                      int valuator);
extern _X_EXPORT double valuator_mask_get_unaccelerated(const ValuatorMask *mask,
                                                        int valuator);
extern _X_EXPORT Bool valuator_mask_fetch_unaccelerated(const ValuatorMask *mask,
                                                        int valuator,
                                                        double *accel,
                                                        double *unaccel);
extern _X_HIDDEN void valuator_mask_drop_unaccelerated(ValuatorMask *mask);

/* InputOption handling interface */
extern _X_EXPORT InputOption *input_option_new(InputOption *list,
                                               const char *key,
                                               const char *value);
extern _X_EXPORT void input_option_free_list(InputOption **opt);
extern _X_EXPORT InputOption *input_option_free_element(InputOption *opt,
                                                        const char *key);
extern _X_EXPORT InputOption *input_option_find(InputOption *list,
                                                const char *key);
extern _X_EXPORT const char *input_option_get_key(const InputOption *opt);
extern _X_EXPORT const char *input_option_get_value(const InputOption *opt);
extern _X_EXPORT void input_option_set_key(InputOption *opt, const char *key);
extern _X_EXPORT void input_option_set_value(InputOption *opt,
                                             const char *value);

extern _X_HIDDEN Bool point_on_screen(ScreenPtr pScreen, int x, int y);
extern _X_HIDDEN void update_desktop_dimensions(void);

extern _X_HIDDEN void input_constrain_cursor(DeviceIntPtr pDev, ScreenPtr screen,
                                             int current_x, int current_y,
                                             int dest_x, int dest_y,
                                             int *out_x, int *out_y,
                                             int *nevents, InternalEvent* events);

extern _X_EXPORT void input_lock(void);
extern _X_EXPORT void input_unlock(void);
extern _X_EXPORT void input_force_unlock(void);
extern _X_EXPORT int in_input_thread(void);

extern void InputThreadPreInit(void);
extern void InputThreadInit(void);
extern void InputThreadFini(void);

extern int InputThreadRegisterDev(int fd,
                                  NotifyFdProcPtr readInputProc,
                                  void *readInputArgs);

extern int InputThreadUnregisterDev(int fd);

extern _X_EXPORT Bool InputThreadEnable;

#endif                          /* INPUT_H */
