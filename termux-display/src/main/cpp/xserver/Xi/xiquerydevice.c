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
 * Authors: Peter Hutterer
 *
 */

/**
 * @file Protocol handling for the XIQueryDevice request/reply.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/extensions/XI2proto.h>
#include "xkbstr.h"
#include "xkbsrv.h"
#include "xserver-properties.h"
#include "exevents.h"
#include "xace.h"
#include "inpututils.h"

#include "exglobals.h"
#include "privates.h"

#include "xiquerydevice.h"

static Bool ShouldSkipDevice(ClientPtr client, int deviceid, DeviceIntPtr d);
static int
 ListDeviceInfo(ClientPtr client, DeviceIntPtr dev, xXIDeviceInfo * info);
static int SizeDeviceInfo(DeviceIntPtr dev);
static void SwapDeviceInfo(DeviceIntPtr dev, xXIDeviceInfo * info);
int _X_COLD
SProcXIQueryDevice(ClientPtr client)
{
    REQUEST(xXIQueryDeviceReq);
    REQUEST_SIZE_MATCH(xXIQueryDeviceReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);

    return ProcXIQueryDevice(client);
}

int
ProcXIQueryDevice(ClientPtr client)
{
    xXIQueryDeviceReply rep;
    DeviceIntPtr dev = NULL;
    int rc = Success;
    int i = 0, len = 0;
    char *info, *ptr;
    Bool *skip = NULL;

    REQUEST(xXIQueryDeviceReq);
    REQUEST_SIZE_MATCH(xXIQueryDeviceReq);

    if (stuff->deviceid != XIAllDevices &&
        stuff->deviceid != XIAllMasterDevices) {
        rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->deviceid;
            return rc;
        }
        len += SizeDeviceInfo(dev);
    }
    else {
        skip = calloc(sizeof(Bool), inputInfo.numDevices);
        if (!skip)
            return BadAlloc;

        for (dev = inputInfo.devices; dev; dev = dev->next, i++) {
            skip[i] = ShouldSkipDevice(client, stuff->deviceid, dev);
            if (!skip[i])
                len += SizeDeviceInfo(dev);
        }

        for (dev = inputInfo.off_devices; dev; dev = dev->next, i++) {
            skip[i] = ShouldSkipDevice(client, stuff->deviceid, dev);
            if (!skip[i])
                len += SizeDeviceInfo(dev);
        }
    }

    info = calloc(1, len);
    if (!info) {
        free(skip);
        return BadAlloc;
    }

    rep = (xXIQueryDeviceReply) {
        .repType = X_Reply,
        .RepType = X_XIQueryDevice,
        .sequenceNumber = client->sequence,
        .length = len / 4,
        .num_devices = 0
    };

    ptr = info;
    if (dev) {
        len = ListDeviceInfo(client, dev, (xXIDeviceInfo *) info);
        if (client->swapped)
            SwapDeviceInfo(dev, (xXIDeviceInfo *) info);
        info += len;
        rep.num_devices = 1;
    }
    else {
        i = 0;
        for (dev = inputInfo.devices; dev; dev = dev->next, i++) {
            if (!skip[i]) {
                len = ListDeviceInfo(client, dev, (xXIDeviceInfo *) info);
                if (client->swapped)
                    SwapDeviceInfo(dev, (xXIDeviceInfo *) info);
                info += len;
                rep.num_devices++;
            }
        }

        for (dev = inputInfo.off_devices; dev; dev = dev->next, i++) {
            if (!skip[i]) {
                len = ListDeviceInfo(client, dev, (xXIDeviceInfo *) info);
                if (client->swapped)
                    SwapDeviceInfo(dev, (xXIDeviceInfo *) info);
                info += len;
                rep.num_devices++;
            }
        }
    }

    len = rep.length * 4;
    WriteReplyToClient(client, sizeof(xXIQueryDeviceReply), &rep);
    WriteToClient(client, len, ptr);
    free(ptr);
    free(skip);
    return rc;
}

void
SRepXIQueryDevice(ClientPtr client, int size, xXIQueryDeviceReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->num_devices);

    /* Device info is already swapped, see ProcXIQueryDevice */

    WriteToClient(client, size, rep);
}

/**
 * @return Whether the device should be included in the returned list.
 */
static Bool
ShouldSkipDevice(ClientPtr client, int deviceid, DeviceIntPtr dev)
{
    /* if all devices are not being queried, only master devices are */
    if (deviceid == XIAllDevices || IsMaster(dev)) {
        int rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixGetAttrAccess);

        if (rc == Success)
            return FALSE;
    }
    return TRUE;
}

/**
 * @return The number of bytes needed to store this device's xXIDeviceInfo
 * (and its classes).
 */
static int
SizeDeviceInfo(DeviceIntPtr dev)
{
    int len = sizeof(xXIDeviceInfo);

    /* 4-padded name */
    len += pad_to_int32(strlen(dev->name));

    return len + SizeDeviceClasses(dev);

}

/*
 * @return The number of bytes needed to store this device's classes.
 */
int
SizeDeviceClasses(DeviceIntPtr dev)
{
    int len = 0;

    if (dev->button) {
        len += sizeof(xXIButtonInfo);
        len += dev->button->numButtons * sizeof(Atom);
        len += pad_to_int32(bits_to_bytes(dev->button->numButtons));
    }

    if (dev->key) {
        XkbDescPtr xkb = dev->key->xkbInfo->desc;

        len += sizeof(xXIKeyInfo);
        len += (xkb->max_key_code - xkb->min_key_code + 1) * sizeof(uint32_t);
    }

    if (dev->valuator) {
        int i;

        len += (sizeof(xXIValuatorInfo)) * dev->valuator->numAxes;

        for (i = 0; i < dev->valuator->numAxes; i++) {
            if (dev->valuator->axes[i].scroll.type != SCROLL_TYPE_NONE)
                len += sizeof(xXIScrollInfo);
        }
    }

    if (dev->touch)
        len += sizeof(xXITouchInfo);

    if (dev->gesture)
        len += sizeof(xXIGestureInfo);

    return len;
}

/**
 * Get pointers to button information areas holding button mask and labels.
 */
static void
ButtonInfoData(xXIButtonInfo *info, int *mask_words, unsigned char **mask,
               Atom **atoms)
{
    *mask_words = bytes_to_int32(bits_to_bytes(info->num_buttons));
    *mask = (unsigned char*) &info[1];
    *atoms = (Atom*) ((*mask) + (*mask_words) * 4);
}

/**
 * Write button information into info.
 * @return Number of bytes written into info.
 */
int
ListButtonInfo(DeviceIntPtr dev, xXIButtonInfo * info, Bool reportState)
{
    unsigned char *bits;
    Atom *labels;
    int mask_len;
    int i;

    if (!dev || !dev->button)
        return 0;

    info->type = ButtonClass;
    info->num_buttons = dev->button->numButtons;
    ButtonInfoData(info, &mask_len, &bits, &labels);
    info->length = bytes_to_int32(sizeof(xXIButtonInfo)) +
        info->num_buttons + mask_len;
    info->sourceid = dev->button->sourceid;

    memset(bits, 0, mask_len * 4);

    if (reportState)
        for (i = 0; i < dev->button->numButtons; i++)
            if (BitIsOn(dev->button->down, i))
                SetBit(bits, i);

    memcpy(labels, dev->button->labels, dev->button->numButtons * sizeof(Atom));

    return info->length * 4;
}

static void
SwapButtonInfo(DeviceIntPtr dev, xXIButtonInfo * info)
{
    Atom *btn;
    int mask_len;
    unsigned char *mask;

    int i;
    ButtonInfoData(info, &mask_len, &mask, &btn);

    swaps(&info->type);
    swaps(&info->length);
    swaps(&info->sourceid);

    for (i = 0 ; i < info->num_buttons; i++, btn++)
        swapl(btn);

    swaps(&info->num_buttons);
}

/**
 * Write key information into info.
 * @return Number of bytes written into info.
 */
int
ListKeyInfo(DeviceIntPtr dev, xXIKeyInfo * info)
{
    int i;
    XkbDescPtr xkb = dev->key->xkbInfo->desc;
    uint32_t *kc;

    info->type = KeyClass;
    info->num_keycodes = xkb->max_key_code - xkb->min_key_code + 1;
    info->length = sizeof(xXIKeyInfo) / 4 + info->num_keycodes;
    info->sourceid = dev->key->sourceid;

    kc = (uint32_t *) &info[1];
    for (i = xkb->min_key_code; i <= xkb->max_key_code; i++, kc++)
        *kc = i;

    return info->length * 4;
}

static void
SwapKeyInfo(DeviceIntPtr dev, xXIKeyInfo * info)
{
    uint32_t *key;
    int i;

    swaps(&info->type);
    swaps(&info->length);
    swaps(&info->sourceid);

    for (i = 0, key = (uint32_t *) &info[1]; i < info->num_keycodes;
         i++, key++)
        swapl(key);

    swaps(&info->num_keycodes);
}

/**
 * List axis information for the given axis.
 *
 * @return The number of bytes written into info.
 */
int
ListValuatorInfo(DeviceIntPtr dev, xXIValuatorInfo * info, int axisnumber,
                 Bool reportState)
{
    ValuatorClassPtr v = dev->valuator;

    info->type = ValuatorClass;
    info->length = sizeof(xXIValuatorInfo) / 4;
    info->label = v->axes[axisnumber].label;
    info->min.integral = v->axes[axisnumber].min_value;
    info->min.frac = 0;
    info->max.integral = v->axes[axisnumber].max_value;
    info->max.frac = 0;
    info->value = double_to_fp3232(v->axisVal[axisnumber]);
    info->resolution = v->axes[axisnumber].resolution;
    info->number = axisnumber;
    info->mode = valuator_get_mode(dev, axisnumber);
    info->sourceid = v->sourceid;

    if (!reportState)
        info->value = info->min;

    return info->length * 4;
}

static void
SwapValuatorInfo(DeviceIntPtr dev, xXIValuatorInfo * info)
{
    swaps(&info->type);
    swaps(&info->length);
    swapl(&info->label);
    swapl(&info->min.integral);
    swapl(&info->min.frac);
    swapl(&info->max.integral);
    swapl(&info->max.frac);
    swapl(&info->value.integral);
    swapl(&info->value.frac);
    swapl(&info->resolution);
    swaps(&info->number);
    swaps(&info->sourceid);
}

int
ListScrollInfo(DeviceIntPtr dev, xXIScrollInfo * info, int axisnumber)
{
    ValuatorClassPtr v = dev->valuator;
    AxisInfoPtr axis = &v->axes[axisnumber];

    if (axis->scroll.type == SCROLL_TYPE_NONE)
        return 0;

    info->type = XIScrollClass;
    info->length = sizeof(xXIScrollInfo) / 4;
    info->number = axisnumber;
    switch (axis->scroll.type) {
    case SCROLL_TYPE_VERTICAL:
        info->scroll_type = XIScrollTypeVertical;
        break;
    case SCROLL_TYPE_HORIZONTAL:
        info->scroll_type = XIScrollTypeHorizontal;
        break;
    default:
        ErrorF("[Xi] Unknown scroll type %d. This is a bug.\n",
               axis->scroll.type);
        break;
    }
    info->increment = double_to_fp3232(axis->scroll.increment);
    info->sourceid = v->sourceid;

    info->flags = 0;

    if (axis->scroll.flags & SCROLL_FLAG_DONT_EMULATE)
        info->flags |= XIScrollFlagNoEmulation;
    if (axis->scroll.flags & SCROLL_FLAG_PREFERRED)
        info->flags |= XIScrollFlagPreferred;

    return info->length * 4;
}

static void
SwapScrollInfo(DeviceIntPtr dev, xXIScrollInfo * info)
{
    swaps(&info->type);
    swaps(&info->length);
    swaps(&info->number);
    swaps(&info->sourceid);
    swaps(&info->scroll_type);
    swapl(&info->increment.integral);
    swapl(&info->increment.frac);
}

/**
 * List multitouch information
 *
 * @return The number of bytes written into info.
 */
int
ListTouchInfo(DeviceIntPtr dev, xXITouchInfo * touch)
{
    touch->type = XITouchClass;
    touch->length = sizeof(xXITouchInfo) >> 2;
    touch->sourceid = dev->touch->sourceid;
    touch->mode = dev->touch->mode;
    touch->num_touches = dev->touch->num_touches;

    return touch->length << 2;
}

static void
SwapTouchInfo(DeviceIntPtr dev, xXITouchInfo * touch)
{
    swaps(&touch->type);
    swaps(&touch->length);
    swaps(&touch->sourceid);
}

static Bool ShouldListGestureInfo(ClientPtr client)
{
    /* libxcb 14.1 and older are not forwards-compatible with new device classes as it does not
     * properly ignore unknown device classes. Since breaking libxcb would break quite a lot of
     * applications, we instead report Gesture device class only if the client advertised support
     * for XI 2.4. Clients may still not work in cases when a client advertises XI 2.4 support
     * and then a completely separate module within the client uses broken libxcb to call
     * XIQueryDevice.
     */
    XIClientPtr pXIClient = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);
    if (pXIClient->major_version) {
        return version_compare(pXIClient->major_version, pXIClient->minor_version, 2, 4) >= 0;
    }
    return FALSE;
}

/**
 * List gesture information
 *
 * @return The number of bytes written into info.
 */
static int
ListGestureInfo(DeviceIntPtr dev, xXIGestureInfo * gesture)
{
    gesture->type = XIGestureClass;
    gesture->length = sizeof(xXIGestureInfo) >> 2;
    gesture->sourceid = dev->gesture->sourceid;
    gesture->num_touches = dev->gesture->max_touches;

    return gesture->length << 2;
}

static void
SwapGestureInfo(DeviceIntPtr dev, xXIGestureInfo * gesture)
{
    swaps(&gesture->type);
    swaps(&gesture->length);
    swaps(&gesture->sourceid);
}

int
GetDeviceUse(DeviceIntPtr dev, uint16_t * attachment)
{
    DeviceIntPtr master = GetMaster(dev, MASTER_ATTACHED);
    int use;

    if (IsMaster(dev)) {
        DeviceIntPtr paired = GetPairedDevice(dev);

        use = IsPointerDevice(dev) ? XIMasterPointer : XIMasterKeyboard;
        *attachment = (paired ? paired->id : 0);
    }
    else if (!IsFloating(dev)) {
        use = IsPointerDevice(master) ? XISlavePointer : XISlaveKeyboard;
        *attachment = master->id;
    }
    else
        use = XIFloatingSlave;

    return use;
}

/**
 * Write the info for device dev into the buffer pointed to by info.
 *
 * @return The number of bytes used.
 */
static int
ListDeviceInfo(ClientPtr client, DeviceIntPtr dev, xXIDeviceInfo * info)
{
    char *any = (char *) &info[1];
    int len = 0, total_len = 0;

    info->deviceid = dev->id;
    info->use = GetDeviceUse(dev, &info->attachment);
    info->num_classes = 0;
    info->name_len = strlen(dev->name);
    info->enabled = dev->enabled;
    total_len = sizeof(xXIDeviceInfo);

    len = pad_to_int32(info->name_len);
    memset(any, 0, len);
    strncpy(any, dev->name, info->name_len);
    any += len;
    total_len += len;

    total_len += ListDeviceClasses(client, dev, any, &info->num_classes);
    return total_len;
}

/**
 * Write the class info of the device into the memory pointed to by any, set
 * nclasses to the number of classes in total and return the number of bytes
 * written.
 */
int
ListDeviceClasses(ClientPtr client, DeviceIntPtr dev,
                  char *any, uint16_t * nclasses)
{
    int total_len = 0;
    int len;
    int i;
    int rc;

    /* Check if the current device state should be suppressed */
    rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixReadAccess);

    if (dev->button) {
        (*nclasses)++;
        len = ListButtonInfo(dev, (xXIButtonInfo *) any, rc == Success);
        any += len;
        total_len += len;
    }

    if (dev->key) {
        (*nclasses)++;
        len = ListKeyInfo(dev, (xXIKeyInfo *) any);
        any += len;
        total_len += len;
    }

    for (i = 0; dev->valuator && i < dev->valuator->numAxes; i++) {
        (*nclasses)++;
        len = ListValuatorInfo(dev, (xXIValuatorInfo *) any, i, rc == Success);
        any += len;
        total_len += len;
    }

    for (i = 0; dev->valuator && i < dev->valuator->numAxes; i++) {
        len = ListScrollInfo(dev, (xXIScrollInfo *) any, i);
        if (len)
            (*nclasses)++;
        any += len;
        total_len += len;
    }

    if (dev->touch) {
        (*nclasses)++;
        len = ListTouchInfo(dev, (xXITouchInfo *) any);
        any += len;
        total_len += len;
    }

    if (dev->gesture && ShouldListGestureInfo(client)) {
        (*nclasses)++;
        len = ListGestureInfo(dev, (xXIGestureInfo *) any);
        any += len;
        total_len += len;
    }

    return total_len;
}

static void
SwapDeviceInfo(DeviceIntPtr dev, xXIDeviceInfo * info)
{
    char *any = (char *) &info[1];
    int i;

    /* Skip over name */
    any += pad_to_int32(info->name_len);

    for (i = 0; i < info->num_classes; i++) {
        int len = ((xXIAnyInfo *) any)->length;

        switch (((xXIAnyInfo *) any)->type) {
        case XIButtonClass:
            SwapButtonInfo(dev, (xXIButtonInfo *) any);
            break;
        case XIKeyClass:
            SwapKeyInfo(dev, (xXIKeyInfo *) any);
            break;
        case XIValuatorClass:
            SwapValuatorInfo(dev, (xXIValuatorInfo *) any);
            break;
        case XIScrollClass:
            SwapScrollInfo(dev, (xXIScrollInfo *) any);
            break;
        case XITouchClass:
            SwapTouchInfo(dev, (xXITouchInfo *) any);
            break;
        case XIGestureClass:
            SwapGestureInfo(dev, (xXIGestureInfo *) any);
            break;
        }

        any += len * 4;
    }

    swaps(&info->deviceid);
    swaps(&info->use);
    swaps(&info->attachment);
    swaps(&info->num_classes);
    swaps(&info->name_len);

}
