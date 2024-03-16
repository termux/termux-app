/*
 * Copyright © 2009 Red Hat, Inc.
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
 * Author: Peter Hutterer
 */

/***********************************************************************
 *
 * Request to grab or ungrab input device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>
#include "swaprep.h"

#include "exglobals.h"          /* BadDevice */
#include "exevents.h"
#include "xipassivegrab.h"
#include "dixgrabs.h"
#include "misc.h"
#include "inpututils.h"

int _X_COLD
SProcXIPassiveGrabDevice(ClientPtr client)
{
    int i;
    uint32_t *mods;

    REQUEST(xXIPassiveGrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xXIPassiveGrabDeviceReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->grab_window);
    swapl(&stuff->cursor);
    swapl(&stuff->time);
    swapl(&stuff->detail);
    swaps(&stuff->mask_len);
    swaps(&stuff->num_modifiers);

    REQUEST_FIXED_SIZE(xXIPassiveGrabDeviceReq,
        ((uint32_t) stuff->mask_len + stuff->num_modifiers) *4);
    mods = (uint32_t *) &stuff[1] + stuff->mask_len;

    for (i = 0; i < stuff->num_modifiers; i++, mods++) {
        swapl(mods);
    }

    return ProcXIPassiveGrabDevice(client);
}

int
ProcXIPassiveGrabDevice(ClientPtr client)
{
    DeviceIntPtr dev, mod_dev;
    xXIPassiveGrabDeviceReply rep = {
        .repType = X_Reply,
        .RepType = X_XIPassiveGrabDevice,
        .sequenceNumber = client->sequence,
        .length = 0,
        .num_modifiers = 0
    };
    int i, ret = Success;
    uint32_t *modifiers;
    xXIGrabModifierInfo *modifiers_failed = NULL;
    GrabMask mask = { 0 };
    GrabParameters param;
    void *tmp;
    int mask_len;

    REQUEST(xXIPassiveGrabDeviceReq);
    REQUEST_FIXED_SIZE(xXIPassiveGrabDeviceReq,
        ((uint32_t) stuff->mask_len + stuff->num_modifiers) * 4);

    if (stuff->deviceid == XIAllDevices)
        dev = inputInfo.all_devices;
    else if (stuff->deviceid == XIAllMasterDevices)
        dev = inputInfo.all_master_devices;
    else {
        ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGrabAccess);
        if (ret != Success) {
            client->errorValue = stuff->deviceid;
            return ret;
        }
    }

    if (stuff->grab_type != XIGrabtypeButton &&
        stuff->grab_type != XIGrabtypeKeycode &&
        stuff->grab_type != XIGrabtypeEnter &&
        stuff->grab_type != XIGrabtypeFocusIn &&
        stuff->grab_type != XIGrabtypeTouchBegin &&
        stuff->grab_type != XIGrabtypeGesturePinchBegin &&
        stuff->grab_type != XIGrabtypeGestureSwipeBegin) {
        client->errorValue = stuff->grab_type;
        return BadValue;
    }

    if ((stuff->grab_type == XIGrabtypeEnter ||
         stuff->grab_type == XIGrabtypeFocusIn ||
         stuff->grab_type == XIGrabtypeTouchBegin ||
         stuff->grab_type == XIGrabtypeGesturePinchBegin ||
         stuff->grab_type == XIGrabtypeGestureSwipeBegin) && stuff->detail != 0) {
        client->errorValue = stuff->detail;
        return BadValue;
    }

    if (stuff->grab_type == XIGrabtypeTouchBegin &&
        (stuff->grab_mode != XIGrabModeTouch ||
         stuff->paired_device_mode != GrabModeAsync)) {
        client->errorValue = stuff->grab_mode;
        return BadValue;
    }

    /* XI2 allows 32-bit keycodes but thanks to XKB we can never
     * implement this. Just return an error for all keycodes that
     * cannot work anyway, same for buttons > 255. */
    if (stuff->detail > 255)
        return XIAlreadyGrabbed;

    if (XICheckInvalidMaskBits(client, (unsigned char *) &stuff[1],
                               stuff->mask_len * 4) != Success)
        return BadValue;

    mask.xi2mask = xi2mask_new();
    if (!mask.xi2mask)
        return BadAlloc;

    mask_len = min(xi2mask_mask_size(mask.xi2mask), stuff->mask_len * 4);
    xi2mask_set_one_mask(mask.xi2mask, stuff->deviceid,
                         (unsigned char *) &stuff[1], mask_len * 4);

    memset(&param, 0, sizeof(param));
    param.grabtype = XI2;
    param.ownerEvents = stuff->owner_events;
    param.grabWindow = stuff->grab_window;
    param.cursor = stuff->cursor;

    if (IsKeyboardDevice(dev)) {
        param.this_device_mode = stuff->grab_mode;
        param.other_devices_mode = stuff->paired_device_mode;
    }
    else {
        param.this_device_mode = stuff->paired_device_mode;
        param.other_devices_mode = stuff->grab_mode;
    }

    if (stuff->cursor != None) {
        ret = dixLookupResourceByType(&tmp, stuff->cursor,
                                      RT_CURSOR, client, DixUseAccess);
        if (ret != Success) {
            client->errorValue = stuff->cursor;
            goto out;
        }
    }

    ret =
        dixLookupWindow((WindowPtr *) &tmp, stuff->grab_window, client,
                        DixSetAttrAccess);
    if (ret != Success)
        goto out;

    ret = CheckGrabValues(client, &param);
    if (ret != Success)
        goto out;

    modifiers = (uint32_t *) &stuff[1] + stuff->mask_len;
    modifiers_failed =
        calloc(stuff->num_modifiers, sizeof(xXIGrabModifierInfo));
    if (!modifiers_failed) {
        ret = BadAlloc;
        goto out;
    }

    mod_dev = (IsFloating(dev)) ? dev : GetMaster(dev, MASTER_KEYBOARD);

    for (i = 0; i < stuff->num_modifiers; i++, modifiers++) {
        uint8_t status = Success;

        param.modifiers = *modifiers;
        ret = CheckGrabValues(client, &param);
        if (ret != Success)
            goto out;

        switch (stuff->grab_type) {
        case XIGrabtypeButton:
            status = GrabButton(client, dev, mod_dev, stuff->detail,
                                &param, XI2, &mask);
            break;
        case XIGrabtypeKeycode:
            status = GrabKey(client, dev, mod_dev, stuff->detail,
                             &param, XI2, &mask);
            break;
        case XIGrabtypeEnter:
        case XIGrabtypeFocusIn:
            status = GrabWindow(client, dev, stuff->grab_type, &param, &mask);
            break;
        case XIGrabtypeTouchBegin:
            status = GrabTouchOrGesture(client, dev, mod_dev, XI_TouchBegin,
                                        &param, &mask);
            break;
        case XIGrabtypeGesturePinchBegin:
            status = GrabTouchOrGesture(client, dev, mod_dev,
                                        XI_GesturePinchBegin, &param, &mask);
            break;
        case XIGrabtypeGestureSwipeBegin:
            status = GrabTouchOrGesture(client, dev, mod_dev,
                                        XI_GestureSwipeBegin, &param, &mask);
            break;
        }

        if (status != GrabSuccess) {
            xXIGrabModifierInfo *info = modifiers_failed + rep.num_modifiers;

            info->status = status;
            info->modifiers = *modifiers;
            if (client->swapped)
                swapl(&info->modifiers);

            rep.num_modifiers++;
            rep.length += bytes_to_int32(sizeof(xXIGrabModifierInfo));
        }
    }

    WriteReplyToClient(client, sizeof(rep), &rep);
    if (rep.num_modifiers)
        WriteToClient(client, rep.length * 4, modifiers_failed);

 out:
    free(modifiers_failed);
    xi2mask_free(&mask.xi2mask);
    return ret;
}

void _X_COLD
SRepXIPassiveGrabDevice(ClientPtr client, int size,
                        xXIPassiveGrabDeviceReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->num_modifiers);

    WriteToClient(client, size, rep);
}

int _X_COLD
SProcXIPassiveUngrabDevice(ClientPtr client)
{
    int i;
    uint32_t *modifiers;

    REQUEST(xXIPassiveUngrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xXIPassiveUngrabDeviceReq);

    swaps(&stuff->length);
    swapl(&stuff->grab_window);
    swaps(&stuff->deviceid);
    swapl(&stuff->detail);
    swaps(&stuff->num_modifiers);

    REQUEST_FIXED_SIZE(xXIPassiveUngrabDeviceReq,
                       ((uint32_t) stuff->num_modifiers) << 2);
    modifiers = (uint32_t *) &stuff[1];

    for (i = 0; i < stuff->num_modifiers; i++, modifiers++)
        swapl(modifiers);

    return ProcXIPassiveUngrabDevice(client);
}

int
ProcXIPassiveUngrabDevice(ClientPtr client)
{
    DeviceIntPtr dev, mod_dev;
    WindowPtr win;
    GrabPtr tempGrab;
    uint32_t *modifiers;
    int i, rc;

    REQUEST(xXIPassiveUngrabDeviceReq);
    REQUEST_FIXED_SIZE(xXIPassiveUngrabDeviceReq,
                       ((uint32_t) stuff->num_modifiers) << 2);

    if (stuff->deviceid == XIAllDevices)
        dev = inputInfo.all_devices;
    else if (stuff->deviceid == XIAllMasterDevices)
        dev = inputInfo.all_master_devices;
    else {
        rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGrabAccess);
        if (rc != Success)
            return rc;
    }

    if (stuff->grab_type != XIGrabtypeButton &&
        stuff->grab_type != XIGrabtypeKeycode &&
        stuff->grab_type != XIGrabtypeEnter &&
        stuff->grab_type != XIGrabtypeFocusIn &&
        stuff->grab_type != XIGrabtypeTouchBegin &&
        stuff->grab_type != XIGrabtypeGesturePinchBegin &&
        stuff->grab_type != XIGrabtypeGestureSwipeBegin) {
        client->errorValue = stuff->grab_type;
        return BadValue;
    }

    if ((stuff->grab_type == XIGrabtypeEnter ||
         stuff->grab_type == XIGrabtypeFocusIn ||
         stuff->grab_type == XIGrabtypeTouchBegin) && stuff->detail != 0) {
        client->errorValue = stuff->detail;
        return BadValue;
    }

    /* We don't allow passive grabs for details > 255 anyway */
    if (stuff->detail > 255) {
        client->errorValue = stuff->detail;
        return BadValue;
    }

    rc = dixLookupWindow(&win, stuff->grab_window, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    mod_dev = (IsFloating(dev)) ? dev : GetMaster(dev, MASTER_KEYBOARD);

    tempGrab = AllocGrab(NULL);
    if (!tempGrab)
        return BadAlloc;

    tempGrab->resource = client->clientAsMask;
    tempGrab->device = dev;
    tempGrab->window = win;
    switch (stuff->grab_type) {
    case XIGrabtypeButton:
        tempGrab->type = XI_ButtonPress;
        break;
    case XIGrabtypeKeycode:
        tempGrab->type = XI_KeyPress;
        break;
    case XIGrabtypeEnter:
        tempGrab->type = XI_Enter;
        break;
    case XIGrabtypeFocusIn:
        tempGrab->type = XI_FocusIn;
        break;
    case XIGrabtypeTouchBegin:
        tempGrab->type = XI_TouchBegin;
        break;
    case XIGrabtypeGesturePinchBegin:
        tempGrab->type = XI_GesturePinchBegin;
        break;
    case XIGrabtypeGestureSwipeBegin:
        tempGrab->type = XI_GestureSwipeBegin;
        break;
    }
    tempGrab->grabtype = XI2;
    tempGrab->modifierDevice = mod_dev;
    tempGrab->modifiersDetail.pMask = NULL;
    tempGrab->detail.exact = stuff->detail;
    tempGrab->detail.pMask = NULL;

    modifiers = (uint32_t *) &stuff[1];

    for (i = 0; i < stuff->num_modifiers; i++, modifiers++) {
        tempGrab->modifiersDetail.exact = *modifiers;
        DeletePassiveGrabFromList(tempGrab);
    }

    FreeGrab(tempGrab);

    return Success;
}
