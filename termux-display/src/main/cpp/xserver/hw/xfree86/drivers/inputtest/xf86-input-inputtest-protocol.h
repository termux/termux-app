/*
 * Copyright © 2020 Povilas Kanapickas <povilas@radix.lt>
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef XF86_INPUT_INPUTTEST_PROTOCOL_H_
#define XF86_INPUT_INPUTTEST_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define XF86IT_PROTOCOL_VERSION_MAJOR 1
#define XF86IT_PROTOCOL_VERSION_MINOR 1

enum xf86ITResponseType {
    XF86IT_RESPONSE_SERVER_VERSION,
    XF86IT_RESPONSE_SYNC_FINISHED,
};

typedef struct {
    uint32_t length; /* length of the whole event in bytes, including the header */
    enum xf86ITResponseType type;
} xf86ITResponseHeader;

typedef struct {
    xf86ITResponseHeader header;
    uint16_t major;
    uint16_t minor;
} xf86ITResponseServerVersion;

typedef struct {
    xf86ITResponseHeader header;
} xf86ITResponseSyncFinished;

typedef union {
    xf86ITResponseHeader header;
    xf86ITResponseServerVersion version;
} xf86ITResponseAny;

/* We care more about preserving the binary input driver protocol more than the
   size of the messages, so hardcode a larger valuator count than the server has */
#define XF86IT_MAX_VALUATORS 64

enum xf86ITEventType {
    XF86IT_EVENT_CLIENT_VERSION,
    XF86IT_EVENT_WAIT_FOR_SYNC,
    XF86IT_EVENT_MOTION,
    XF86IT_EVENT_PROXIMITY,
    XF86IT_EVENT_BUTTON,
    XF86IT_EVENT_KEY,
    XF86IT_EVENT_TOUCH,
    XF86IT_EVENT_GESTURE_PINCH,
    XF86IT_EVENT_GESTURE_SWIPE,
};

typedef struct {
    uint32_t length; /* length of the whole event in bytes, including the header */
    enum xf86ITEventType type;
} xf86ITEventHeader;

typedef struct {
    uint32_t has_unaccelerated;
    uint8_t mask[(XF86IT_MAX_VALUATORS + 7) / 8];
    double valuators[XF86IT_MAX_VALUATORS];
    double unaccelerated[XF86IT_MAX_VALUATORS];
} xf86ITValuatorData;

typedef struct {
    xf86ITEventHeader header;
    uint16_t major;
    uint16_t minor;
} xf86ITEventClientVersion;

typedef struct {
    xf86ITEventHeader header;
} xf86ITEventWaitForSync;

typedef struct {
    xf86ITEventHeader header;
    uint32_t is_absolute;
    xf86ITValuatorData valuators;
} xf86ITEventMotion;

typedef struct {
    xf86ITEventHeader header;
    uint32_t is_prox_in;
    xf86ITValuatorData valuators;
} xf86ITEventProximity;

typedef struct {
    xf86ITEventHeader header;
    int32_t is_absolute;
    int32_t button;
    uint32_t is_press;
    xf86ITValuatorData valuators;
} xf86ITEventButton;

typedef struct {
    xf86ITEventHeader header;
    int32_t key_code;
    uint32_t is_press;
} xf86ITEventKey;

typedef struct {
    xf86ITEventHeader header;
    uint32_t touchid;
    uint32_t touch_type;
    xf86ITValuatorData valuators;
} xf86ITEventTouch;

typedef struct {
    xf86ITEventHeader header;
    uint16_t gesture_type;
    uint16_t num_touches;
    uint32_t flags;
    double delta_x;
    double delta_y;
    double delta_unaccel_x;
    double delta_unaccel_y;
    double scale;
    double delta_angle;
} xf86ITEventGesturePinch;

typedef struct {
    xf86ITEventHeader header;
    uint16_t gesture_type;
    uint16_t num_touches;
    uint32_t flags;
    double delta_x;
    double delta_y;
    double delta_unaccel_x;
    double delta_unaccel_y;
} xf86ITEventGestureSwipe;

typedef union {
    xf86ITEventHeader header;
    xf86ITEventClientVersion version;
    xf86ITEventMotion motion;
    xf86ITEventProximity proximity;
    xf86ITEventButton button;
    xf86ITEventKey key;
    xf86ITEventTouch touch;
    xf86ITEventGesturePinch pinch;
    xf86ITEventGestureSwipe swipe;
} xf86ITEventAny;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XF86_INPUT_INPUTTEST_PROTOCOL_H_ */
