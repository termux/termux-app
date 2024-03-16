/*
 * Copyright © 2010 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include "dix-config.h"
#endif

#ifndef INPUTUTILS_H
#define INPUTUTILS_H

#include "input.h"
#include "eventstr.h"
#include <X11/extensions/XI2proto.h>

extern Mask event_filters[MAXDEVICES][MAXEVENTS];

struct _ValuatorMask {
    int8_t last_bit;            /* highest bit set in mask */
    int8_t has_unaccelerated;
    uint8_t mask[(MAX_VALUATORS + 7) / 8];
    double valuators[MAX_VALUATORS];    /* valuator data */
    double unaccelerated[MAX_VALUATORS];    /* valuator data */
};

extern void verify_internal_event(const InternalEvent *ev);
extern void init_device_event(DeviceEvent *event, DeviceIntPtr dev, Time ms,
                              enum DeviceEventSource event_source);
extern void init_gesture_event(GestureEvent *event, DeviceIntPtr dev, Time ms);
extern int event_get_corestate(DeviceIntPtr mouse, DeviceIntPtr kbd);
extern void event_set_state(DeviceIntPtr mouse, DeviceIntPtr kbd,
                            DeviceEvent *event);
extern void event_set_state_gesture(DeviceIntPtr kbd, GestureEvent *event);
extern Mask event_get_filter_from_type(DeviceIntPtr dev, int evtype);
extern Mask event_get_filter_from_xi2type(int evtype);

FP3232 double_to_fp3232(double in);
FP1616 double_to_fp1616(double in);
double fp1616_to_double(FP1616 in);
double fp3232_to_double(FP3232 in);

XI2Mask *xi2mask_new(void);
XI2Mask *xi2mask_new_with_size(size_t, size_t); /* don't use it */
void xi2mask_free(XI2Mask **mask);
Bool xi2mask_isset(XI2Mask *mask, const DeviceIntPtr dev, int event_type);
Bool xi2mask_isset_for_device(XI2Mask *mask, const DeviceIntPtr dev, int event_type);
void xi2mask_set(XI2Mask *mask, int deviceid, int event_type);
void xi2mask_zero(XI2Mask *mask, int deviceid);
void xi2mask_merge(XI2Mask *dest, const XI2Mask *source);
size_t xi2mask_num_masks(const XI2Mask *mask);
size_t xi2mask_mask_size(const XI2Mask *mask);
void xi2mask_set_one_mask(XI2Mask *xi2mask, int deviceid,
                          const unsigned char *mask, size_t mask_size);
const unsigned char *xi2mask_get_one_mask(const XI2Mask *xi2mask, int deviceid);

Bool CopySprite(SpritePtr src, SpritePtr dst);

#endif
