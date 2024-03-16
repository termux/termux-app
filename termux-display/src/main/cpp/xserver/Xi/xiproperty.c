/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2008 Peter Hutterer
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WAXIANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WAXIANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* This code is a modified version of randr/rrproperty.c */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dix.h"
#include "inputstr.h"
#include <X11/extensions/XI.h>
#include <X11/Xatom.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include "exglobals.h"
#include "exevents.h"
#include "swaprep.h"

#include "xiproperty.h"
#include "xserver-properties.h"

/**
 * Properties used or alloced from inside the server.
 */
static struct dev_properties {
    Atom type;
    const char *name;
} dev_properties[] = {
    {0, XI_PROP_ENABLED},
    {0, XI_PROP_XTEST_DEVICE},
    {0, XATOM_FLOAT},
    {0, ACCEL_PROP_PROFILE_NUMBER},
    {0, ACCEL_PROP_CONSTANT_DECELERATION},
    {0, ACCEL_PROP_ADAPTIVE_DECELERATION},
    {0, ACCEL_PROP_VELOCITY_SCALING},
    {0, AXIS_LABEL_PROP},
    {0, AXIS_LABEL_PROP_REL_X},
    {0, AXIS_LABEL_PROP_REL_Y},
    {0, AXIS_LABEL_PROP_REL_Z},
    {0, AXIS_LABEL_PROP_REL_RX},
    {0, AXIS_LABEL_PROP_REL_RY},
    {0, AXIS_LABEL_PROP_REL_RZ},
    {0, AXIS_LABEL_PROP_REL_HWHEEL},
    {0, AXIS_LABEL_PROP_REL_DIAL},
    {0, AXIS_LABEL_PROP_REL_WHEEL},
    {0, AXIS_LABEL_PROP_REL_MISC},
    {0, AXIS_LABEL_PROP_REL_VSCROLL},
    {0, AXIS_LABEL_PROP_REL_HSCROLL},
    {0, AXIS_LABEL_PROP_ABS_X},
    {0, AXIS_LABEL_PROP_ABS_Y},
    {0, AXIS_LABEL_PROP_ABS_Z},
    {0, AXIS_LABEL_PROP_ABS_RX},
    {0, AXIS_LABEL_PROP_ABS_RY},
    {0, AXIS_LABEL_PROP_ABS_RZ},
    {0, AXIS_LABEL_PROP_ABS_THROTTLE},
    {0, AXIS_LABEL_PROP_ABS_RUDDER},
    {0, AXIS_LABEL_PROP_ABS_WHEEL},
    {0, AXIS_LABEL_PROP_ABS_GAS},
    {0, AXIS_LABEL_PROP_ABS_BRAKE},
    {0, AXIS_LABEL_PROP_ABS_HAT0X},
    {0, AXIS_LABEL_PROP_ABS_HAT0Y},
    {0, AXIS_LABEL_PROP_ABS_HAT1X},
    {0, AXIS_LABEL_PROP_ABS_HAT1Y},
    {0, AXIS_LABEL_PROP_ABS_HAT2X},
    {0, AXIS_LABEL_PROP_ABS_HAT2Y},
    {0, AXIS_LABEL_PROP_ABS_HAT3X},
    {0, AXIS_LABEL_PROP_ABS_HAT3Y},
    {0, AXIS_LABEL_PROP_ABS_PRESSURE},
    {0, AXIS_LABEL_PROP_ABS_DISTANCE},
    {0, AXIS_LABEL_PROP_ABS_TILT_X},
    {0, AXIS_LABEL_PROP_ABS_TILT_Y},
    {0, AXIS_LABEL_PROP_ABS_TOOL_WIDTH},
    {0, AXIS_LABEL_PROP_ABS_VOLUME},
    {0, AXIS_LABEL_PROP_ABS_MT_TOUCH_MAJOR},
    {0, AXIS_LABEL_PROP_ABS_MT_TOUCH_MINOR},
    {0, AXIS_LABEL_PROP_ABS_MT_WIDTH_MAJOR},
    {0, AXIS_LABEL_PROP_ABS_MT_WIDTH_MINOR},
    {0, AXIS_LABEL_PROP_ABS_MT_ORIENTATION},
    {0, AXIS_LABEL_PROP_ABS_MT_POSITION_X},
    {0, AXIS_LABEL_PROP_ABS_MT_POSITION_Y},
    {0, AXIS_LABEL_PROP_ABS_MT_TOOL_TYPE},
    {0, AXIS_LABEL_PROP_ABS_MT_BLOB_ID},
    {0, AXIS_LABEL_PROP_ABS_MT_TRACKING_ID},
    {0, AXIS_LABEL_PROP_ABS_MT_PRESSURE},
    {0, AXIS_LABEL_PROP_ABS_MT_DISTANCE},
    {0, AXIS_LABEL_PROP_ABS_MT_TOOL_X},
    {0, AXIS_LABEL_PROP_ABS_MT_TOOL_Y},
    {0, AXIS_LABEL_PROP_ABS_MISC},
    {0, BTN_LABEL_PROP},
    {0, BTN_LABEL_PROP_BTN_UNKNOWN},
    {0, BTN_LABEL_PROP_BTN_WHEEL_UP},
    {0, BTN_LABEL_PROP_BTN_WHEEL_DOWN},
    {0, BTN_LABEL_PROP_BTN_HWHEEL_LEFT},
    {0, BTN_LABEL_PROP_BTN_HWHEEL_RIGHT},
    {0, BTN_LABEL_PROP_BTN_0},
    {0, BTN_LABEL_PROP_BTN_1},
    {0, BTN_LABEL_PROP_BTN_2},
    {0, BTN_LABEL_PROP_BTN_3},
    {0, BTN_LABEL_PROP_BTN_4},
    {0, BTN_LABEL_PROP_BTN_5},
    {0, BTN_LABEL_PROP_BTN_6},
    {0, BTN_LABEL_PROP_BTN_7},
    {0, BTN_LABEL_PROP_BTN_8},
    {0, BTN_LABEL_PROP_BTN_9},
    {0, BTN_LABEL_PROP_BTN_LEFT},
    {0, BTN_LABEL_PROP_BTN_RIGHT},
    {0, BTN_LABEL_PROP_BTN_MIDDLE},
    {0, BTN_LABEL_PROP_BTN_SIDE},
    {0, BTN_LABEL_PROP_BTN_EXTRA},
    {0, BTN_LABEL_PROP_BTN_FORWARD},
    {0, BTN_LABEL_PROP_BTN_BACK},
    {0, BTN_LABEL_PROP_BTN_TASK},
    {0, BTN_LABEL_PROP_BTN_TRIGGER},
    {0, BTN_LABEL_PROP_BTN_THUMB},
    {0, BTN_LABEL_PROP_BTN_THUMB2},
    {0, BTN_LABEL_PROP_BTN_TOP},
    {0, BTN_LABEL_PROP_BTN_TOP2},
    {0, BTN_LABEL_PROP_BTN_PINKIE},
    {0, BTN_LABEL_PROP_BTN_BASE},
    {0, BTN_LABEL_PROP_BTN_BASE2},
    {0, BTN_LABEL_PROP_BTN_BASE3},
    {0, BTN_LABEL_PROP_BTN_BASE4},
    {0, BTN_LABEL_PROP_BTN_BASE5},
    {0, BTN_LABEL_PROP_BTN_BASE6},
    {0, BTN_LABEL_PROP_BTN_DEAD},
    {0, BTN_LABEL_PROP_BTN_A},
    {0, BTN_LABEL_PROP_BTN_B},
    {0, BTN_LABEL_PROP_BTN_C},
    {0, BTN_LABEL_PROP_BTN_X},
    {0, BTN_LABEL_PROP_BTN_Y},
    {0, BTN_LABEL_PROP_BTN_Z},
    {0, BTN_LABEL_PROP_BTN_TL},
    {0, BTN_LABEL_PROP_BTN_TR},
    {0, BTN_LABEL_PROP_BTN_TL2},
    {0, BTN_LABEL_PROP_BTN_TR2},
    {0, BTN_LABEL_PROP_BTN_SELECT},
    {0, BTN_LABEL_PROP_BTN_START},
    {0, BTN_LABEL_PROP_BTN_MODE},
    {0, BTN_LABEL_PROP_BTN_THUMBL},
    {0, BTN_LABEL_PROP_BTN_THUMBR},
    {0, BTN_LABEL_PROP_BTN_TOOL_PEN},
    {0, BTN_LABEL_PROP_BTN_TOOL_RUBBER},
    {0, BTN_LABEL_PROP_BTN_TOOL_BRUSH},
    {0, BTN_LABEL_PROP_BTN_TOOL_PENCIL},
    {0, BTN_LABEL_PROP_BTN_TOOL_AIRBRUSH},
    {0, BTN_LABEL_PROP_BTN_TOOL_FINGER},
    {0, BTN_LABEL_PROP_BTN_TOOL_MOUSE},
    {0, BTN_LABEL_PROP_BTN_TOOL_LENS},
    {0, BTN_LABEL_PROP_BTN_TOUCH},
    {0, BTN_LABEL_PROP_BTN_STYLUS},
    {0, BTN_LABEL_PROP_BTN_STYLUS2},
    {0, BTN_LABEL_PROP_BTN_TOOL_DOUBLETAP},
    {0, BTN_LABEL_PROP_BTN_TOOL_TRIPLETAP},
    {0, BTN_LABEL_PROP_BTN_GEAR_DOWN},
    {0, BTN_LABEL_PROP_BTN_GEAR_UP},
    {0, XI_PROP_TRANSFORM}
};

static long XIPropHandlerID = 1;

static void
send_property_event(DeviceIntPtr dev, Atom property, int what)
{
    int state = (what == XIPropertyDeleted) ? PropertyDelete : PropertyNewValue;
    devicePropertyNotify event = {
        .type = DevicePropertyNotify,
        .deviceid = dev->id,
        .state = state,
        .atom = property,
        .time = currentTime.milliseconds
    };
    xXIPropertyEvent xi2 = {
        .type = GenericEvent,
        .extension = IReqCode,
        .length = 0,
        .evtype = XI_PropertyEvent,
        .deviceid = dev->id,
        .time = currentTime.milliseconds,
        .property = property,
        .what = what
    };

    SendEventToAllWindows(dev, DevicePropertyNotifyMask, (xEvent *) &event, 1);

    SendEventToAllWindows(dev, GetEventFilter(dev, (xEvent *) &xi2),
                          (xEvent *) &xi2, 1);
}

static int
list_atoms(DeviceIntPtr dev, int *natoms, Atom **atoms_return)
{
    XIPropertyPtr prop;
    Atom *atoms = NULL;
    int nprops = 0;

    for (prop = dev->properties.properties; prop; prop = prop->next)
        nprops++;
    if (nprops) {
        Atom *a;

        atoms = xallocarray(nprops, sizeof(Atom));
        if (!atoms)
            return BadAlloc;
        a = atoms;
        for (prop = dev->properties.properties; prop; prop = prop->next, a++)
            *a = prop->propertyName;
    }

    *natoms = nprops;
    *atoms_return = atoms;
    return Success;
}

static int
get_property(ClientPtr client, DeviceIntPtr dev, Atom property, Atom type,
             BOOL delete, int offset, int length,
             int *bytes_after, Atom *type_return, int *format, int *nitems,
             int *length_return, char **data)
{
    unsigned long n, len, ind;
    int rc;
    XIPropertyPtr prop;
    XIPropertyValuePtr prop_value;

    if (!ValidAtom(property)) {
        client->errorValue = property;
        return BadAtom;
    }
    if ((delete != xTrue) && (delete != xFalse)) {
        client->errorValue = delete;
        return BadValue;
    }

    if ((type != AnyPropertyType) && !ValidAtom(type)) {
        client->errorValue = type;
        return BadAtom;
    }

    for (prop = dev->properties.properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            break;

    if (!prop) {
        *bytes_after = 0;
        *type_return = None;
        *format = 0;
        *nitems = 0;
        *length_return = 0;
        return Success;
    }

    rc = XIGetDeviceProperty(dev, property, &prop_value);
    if (rc != Success) {
        client->errorValue = property;
        return rc;
    }

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((type != prop_value->type) && (type != AnyPropertyType))) {
        *bytes_after = prop_value->size;
        *format = prop_value->format;
        *length_return = 0;
        *nitems = 0;
        *type_return = prop_value->type;
        return Success;
    }

    /* Return type, format, value to client */
    n = (prop_value->format / 8) * prop_value->size;    /* size (bytes) of prop */
    ind = offset << 2;

    /* If offset is invalid such that it causes "len" to
       be negative, it's a value error. */

    if (n < ind) {
        client->errorValue = offset;
        return BadValue;
    }

    len = min(n - ind, 4 * length);

    *bytes_after = n - (ind + len);
    *format = prop_value->format;
    *length_return = len;
    if (prop_value->format)
        *nitems = len / (prop_value->format / 8);
    else
        *nitems = 0;
    *type_return = prop_value->type;

    *data = (char *) prop_value->data + ind;

    return Success;
}

static int
check_change_property(ClientPtr client, Atom property, Atom type, int format,
                      int mode, int nitems)
{
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
        (mode != PropModePrepend)) {
        client->errorValue = mode;
        return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32)) {
        client->errorValue = format;
        return BadValue;
    }

    if (!ValidAtom(property)) {
        client->errorValue = property;
        return BadAtom;
    }
    if (!ValidAtom(type)) {
        client->errorValue = type;
        return BadAtom;
    }

    return Success;
}

static int
change_property(ClientPtr client, DeviceIntPtr dev, Atom property, Atom type,
                int format, int mode, int len, void *data)
{
    int rc = Success;

    rc = XIChangeDeviceProperty(dev, property, type, format, mode, len, data,
                                TRUE);
    if (rc != Success)
        client->errorValue = property;

    return rc;
}

/**
 * Return the atom assigned to the specified string or 0 if the atom isn't known
 * to the DIX.
 *
 * If name is NULL, None is returned.
 */
Atom
XIGetKnownProperty(const char *name)
{
    int i;

    if (!name)
        return None;

    for (i = 0; i < ARRAY_SIZE(dev_properties); i++) {
        if (strcmp(name, dev_properties[i].name) == 0) {
            if (dev_properties[i].type == None) {
                dev_properties[i].type =
                    MakeAtom(dev_properties[i].name,
                             strlen(dev_properties[i].name), TRUE);
            }

            return dev_properties[i].type;
        }
    }

    return 0;
}

void
XIResetProperties(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(dev_properties); i++)
        dev_properties[i].type = None;
}

/**
 * Convert the given property's value(s) into @nelem_return integer values and
 * store them in @buf_return. If @nelem_return is larger than the number of
 * values in the property, @nelem_return is set to the number of values in the
 * property.
 *
 * If *@buf_return is NULL and @nelem_return is 0, memory is allocated
 * automatically and must be freed by the caller.
 *
 * Possible return codes.
 * Success ... No error.
 * BadMatch ... Wrong atom type, atom is not XA_INTEGER
 * BadAlloc ... NULL passed as buffer and allocation failed.
 * BadLength ... @buff is NULL but @nelem_return is non-zero.
 *
 * @param val The property value
 * @param nelem_return The maximum number of elements to return.
 * @param buf_return Pointer to an array of at least @nelem_return values.
 * @return Success or the error code if an error occurred.
 */
_X_EXPORT int
XIPropToInt(XIPropertyValuePtr val, int *nelem_return, int **buf_return)
{
    int i;
    int *buf;

    if (val->type != XA_INTEGER)
        return BadMatch;
    if (!*buf_return && *nelem_return)
        return BadLength;

    switch (val->format) {
    case 8:
    case 16:
    case 32:
        break;
    default:
        return BadValue;
    }

    buf = *buf_return;

    if (!buf && !(*nelem_return)) {
        buf = calloc(val->size, sizeof(int));
        if (!buf)
            return BadAlloc;
        *buf_return = buf;
        *nelem_return = val->size;
    }
    else if (val->size < *nelem_return)
        *nelem_return = val->size;

    for (i = 0; i < val->size && i < *nelem_return; i++) {
        switch (val->format) {
        case 8:
            buf[i] = ((CARD8 *) val->data)[i];
            break;
        case 16:
            buf[i] = ((CARD16 *) val->data)[i];
            break;
        case 32:
            buf[i] = ((CARD32 *) val->data)[i];
            break;
        }
    }

    return Success;
}

/**
 * Convert the given property's value(s) into @nelem_return float values and
 * store them in @buf_return. If @nelem_return is larger than the number of
 * values in the property, @nelem_return is set to the number of values in the
 * property.
 *
 * If *@buf_return is NULL and @nelem_return is 0, memory is allocated
 * automatically and must be freed by the caller.
 *
 * Possible errors returned:
 * Success
 * BadMatch ... Wrong atom type, atom is not XA_FLOAT
 * BadValue ... Wrong format, format is not 32
 * BadAlloc ... NULL passed as buffer and allocation failed.
 * BadLength ... @buff is NULL but @nelem_return is non-zero.
 *
 * @param val The property value
 * @param nelem_return The maximum number of elements to return.
 * @param buf_return Pointer to an array of at least @nelem_return values.
 * @return Success or the error code if an error occurred.
 */
_X_EXPORT int
XIPropToFloat(XIPropertyValuePtr val, int *nelem_return, float **buf_return)
{
    int i;
    float *buf;

    if (!val->type || val->type != XIGetKnownProperty(XATOM_FLOAT))
        return BadMatch;

    if (val->format != 32)
        return BadValue;
    if (!*buf_return && *nelem_return)
        return BadLength;

    buf = *buf_return;

    if (!buf && !(*nelem_return)) {
        buf = calloc(val->size, sizeof(float));
        if (!buf)
            return BadAlloc;
        *buf_return = buf;
        *nelem_return = val->size;
    }
    else if (val->size < *nelem_return)
        *nelem_return = val->size;

    for (i = 0; i < val->size && i < *nelem_return; i++)
        buf[i] = ((float *) val->data)[i];

    return Success;
}

/* Registers a new property handler on the given device and returns a unique
 * identifier for this handler. This identifier is required to unregister the
 * property handler again.
 * @return The handler's identifier or 0 if an error occurred.
 */
long
XIRegisterPropertyHandler(DeviceIntPtr dev,
                          int (*SetProperty) (DeviceIntPtr dev,
                                              Atom property,
                                              XIPropertyValuePtr prop,
                                              BOOL checkonly),
                          int (*GetProperty) (DeviceIntPtr dev,
                                              Atom property),
                          int (*DeleteProperty) (DeviceIntPtr dev,
                                                 Atom property))
{
    XIPropertyHandlerPtr new_handler;

    new_handler = calloc(1, sizeof(XIPropertyHandler));
    if (!new_handler)
        return 0;

    new_handler->id = XIPropHandlerID++;
    new_handler->SetProperty = SetProperty;
    new_handler->GetProperty = GetProperty;
    new_handler->DeleteProperty = DeleteProperty;
    new_handler->next = dev->properties.handlers;
    dev->properties.handlers = new_handler;

    return new_handler->id;
}

void
XIUnregisterPropertyHandler(DeviceIntPtr dev, long id)
{
    XIPropertyHandlerPtr curr, prev = NULL;

    curr = dev->properties.handlers;
    while (curr && curr->id != id) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return;

    if (!prev)                  /* first one */
        dev->properties.handlers = curr->next;
    else
        prev->next = curr->next;

    free(curr);
}

static XIPropertyPtr
XICreateDeviceProperty(Atom property)
{
    XIPropertyPtr prop;

    prop = (XIPropertyPtr) malloc(sizeof(XIPropertyRec));
    if (!prop)
        return NULL;

    prop->next = NULL;
    prop->propertyName = property;
    prop->value.type = None;
    prop->value.format = 0;
    prop->value.size = 0;
    prop->value.data = NULL;
    prop->deletable = TRUE;

    return prop;
}

static XIPropertyPtr
XIFetchDeviceProperty(DeviceIntPtr dev, Atom property)
{
    XIPropertyPtr prop;

    for (prop = dev->properties.properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            return prop;
    return NULL;
}

static void
XIDestroyDeviceProperty(XIPropertyPtr prop)
{
    free(prop->value.data);
    free(prop);
}

/* This function destroys all of the device's property-related stuff,
 * including removing all device handlers.
 * DO NOT CALL FROM THE DRIVER.
 */
void
XIDeleteAllDeviceProperties(DeviceIntPtr device)
{
    XIPropertyPtr prop, next;
    XIPropertyHandlerPtr curr_handler, next_handler;

    UpdateCurrentTimeIf();
    for (prop = device->properties.properties; prop; prop = next) {
        next = prop->next;
        send_property_event(device, prop->propertyName, XIPropertyDeleted);
        XIDestroyDeviceProperty(prop);
    }

    device->properties.properties = NULL;

    /* Now free all handlers */
    curr_handler = device->properties.handlers;
    while (curr_handler) {
        next_handler = curr_handler->next;
        free(curr_handler);
        curr_handler = next_handler;
    }

    device->properties.handlers = NULL;
}

int
XIDeleteDeviceProperty(DeviceIntPtr device, Atom property, Bool fromClient)
{
    XIPropertyPtr prop, *prev;
    int rc = Success;

    for (prev = &device->properties.properties; (prop = *prev);
         prev = &(prop->next))
        if (prop->propertyName == property)
            break;

    if (!prop)
        return Success;

    if (fromClient && !prop->deletable)
        return BadAccess;

    /* Ask handlers if we may delete the property */
    if (device->properties.handlers) {
        XIPropertyHandlerPtr handler = device->properties.handlers;

        while (handler) {
            if (handler->DeleteProperty)
                rc = handler->DeleteProperty(device, prop->propertyName);
            if (rc != Success)
                return rc;
            handler = handler->next;
        }
    }

    if (prop) {
        UpdateCurrentTimeIf();
        *prev = prop->next;
        send_property_event(device, prop->propertyName, XIPropertyDeleted);
        XIDestroyDeviceProperty(prop);
    }

    return Success;
}

int
XIChangeDeviceProperty(DeviceIntPtr dev, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       const void *value, Bool sendevent)
{
    XIPropertyPtr prop;
    int size_in_bytes;
    unsigned long total_len;
    XIPropertyValuePtr prop_value;
    XIPropertyValueRec new_value;
    Bool add = FALSE;
    int rc;

    size_in_bytes = format >> 3;

    /* first see if property already exists */
    prop = XIFetchDeviceProperty(dev, property);
    if (!prop) {                /* just add to list */
        prop = XICreateDeviceProperty(property);
        if (!prop)
            return BadAlloc;
        add = TRUE;
        mode = PropModeReplace;
    }
    prop_value = &prop->value;

    /* To append or prepend to a property the request format and type
       must match those of the already defined property.  The
       existing format and type are irrelevant when using the mode
       "PropModeReplace" since they will be written over. */

    if ((format != prop_value->format) && (mode != PropModeReplace))
        return BadMatch;
    if ((prop_value->type != type) && (mode != PropModeReplace))
        return BadMatch;
    new_value = *prop_value;
    if (mode == PropModeReplace)
        total_len = len;
    else
        total_len = prop_value->size + len;

    if (mode == PropModeReplace || len > 0) {
        void *new_data = NULL, *old_data = NULL;

        new_value.data = xallocarray(total_len, size_in_bytes);
        if (!new_value.data && total_len && size_in_bytes) {
            if (add)
                XIDestroyDeviceProperty(prop);
            return BadAlloc;
        }
        new_value.size = len;
        new_value.type = type;
        new_value.format = format;

        switch (mode) {
        case PropModeReplace:
            new_data = new_value.data;
            old_data = NULL;
            break;
        case PropModeAppend:
            new_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            old_data = new_value.data;
            break;
        case PropModePrepend:
            new_data = new_value.data;
            old_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            break;
        }
        if (new_data)
            memcpy((char *) new_data, value, len * size_in_bytes);
        if (old_data)
            memcpy((char *) old_data, (char *) prop_value->data,
                   prop_value->size * size_in_bytes);

        if (dev->properties.handlers) {
            XIPropertyHandlerPtr handler;
            BOOL checkonly = TRUE;

            /* run through all handlers with checkonly TRUE, then again with
             * checkonly FALSE. Handlers MUST return error codes on the
             * checkonly run, errors on the second run are ignored */
            do {
                handler = dev->properties.handlers;
                while (handler) {
                    if (handler->SetProperty) {
                        input_lock();
                        rc = handler->SetProperty(dev, prop->propertyName,
                                                  &new_value, checkonly);
                        input_unlock();
                        if (checkonly && rc != Success) {
                            free(new_value.data);
                            if (add)
                                XIDestroyDeviceProperty(prop);
                            return rc;
                        }
                    }
                    handler = handler->next;
                }
                checkonly = !checkonly;
            } while (!checkonly);
        }
        free(prop_value->data);
        *prop_value = new_value;
    }
    else if (len == 0) {
        /* do nothing */
    }

    if (add) {
        prop->next = dev->properties.properties;
        dev->properties.properties = prop;
    }

    if (sendevent) {
        UpdateCurrentTimeIf();
        send_property_event(dev, prop->propertyName,
                            (add) ? XIPropertyCreated : XIPropertyModified);
    }

    return Success;
}

int
XIGetDeviceProperty(DeviceIntPtr dev, Atom property, XIPropertyValuePtr *value)
{
    XIPropertyPtr prop = XIFetchDeviceProperty(dev, property);
    int rc;

    if (!prop) {
        *value = NULL;
        return BadAtom;
    }

    /* If we can, try to update the property value first */
    if (dev->properties.handlers) {
        XIPropertyHandlerPtr handler = dev->properties.handlers;

        while (handler) {
            if (handler->GetProperty) {
                rc = handler->GetProperty(dev, prop->propertyName);
                if (rc != Success) {
                    *value = NULL;
                    return rc;
                }
            }
            handler = handler->next;
        }
    }

    *value = &prop->value;
    return Success;
}

int
XISetDevicePropertyDeletable(DeviceIntPtr dev, Atom property, Bool deletable)
{
    XIPropertyPtr prop = XIFetchDeviceProperty(dev, property);

    if (!prop)
        return BadAtom;

    prop->deletable = deletable;
    return Success;
}

int
ProcXListDeviceProperties(ClientPtr client)
{
    Atom *atoms;
    xListDevicePropertiesReply rep;
    int natoms;
    DeviceIntPtr dev;
    int rc = Success;

    REQUEST(xListDevicePropertiesReq);
    REQUEST_SIZE_MATCH(xListDevicePropertiesReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixListPropAccess);
    if (rc != Success)
        return rc;

    rc = list_atoms(dev, &natoms, &atoms);
    if (rc != Success)
        return rc;

    rep = (xListDevicePropertiesReply) {
        .repType = X_Reply,
        .RepType = X_ListDeviceProperties,
        .sequenceNumber = client->sequence,
        .length = natoms,
        .nAtoms = natoms
    };

    WriteReplyToClient(client, sizeof(xListDevicePropertiesReply), &rep);
    if (natoms) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, natoms * sizeof(Atom), atoms);
        free(atoms);
    }
    return rc;
}

int
ProcXChangeDeviceProperty(ClientPtr client)
{
    REQUEST(xChangeDevicePropertyReq);
    DeviceIntPtr dev;
    unsigned long len;
    uint64_t totalSize;
    int rc;

    REQUEST_AT_LEAST_SIZE(xChangeDevicePropertyReq);
    UpdateCurrentTime();

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixSetPropAccess);
    if (rc != Success)
        return rc;

    rc = check_change_property(client, stuff->property, stuff->type,
                               stuff->format, stuff->mode, stuff->nUnits);
    if (rc != Success)
        return rc;

    len = stuff->nUnits;
    if (len > (bytes_to_int32(0xffffffff - sizeof(xChangeDevicePropertyReq))))
        return BadLength;

    totalSize = len * (stuff->format / 8);
    REQUEST_FIXED_SIZE(xChangeDevicePropertyReq, totalSize);

    rc = change_property(client, dev, stuff->property, stuff->type,
                         stuff->format, stuff->mode, len, (void *) &stuff[1]);
    return rc;
}

int
ProcXDeleteDeviceProperty(ClientPtr client)
{
    REQUEST(xDeleteDevicePropertyReq);
    DeviceIntPtr dev;
    int rc;

    REQUEST_SIZE_MATCH(xDeleteDevicePropertyReq);
    UpdateCurrentTime();
    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixSetPropAccess);
    if (rc != Success)
        return rc;

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    rc = XIDeleteDeviceProperty(dev, stuff->property, TRUE);
    return rc;
}

int
ProcXGetDeviceProperty(ClientPtr client)
{
    REQUEST(xGetDevicePropertyReq);
    DeviceIntPtr dev;
    int length;
    int rc, format, nitems, bytes_after;
    char *data;
    Atom type;
    xGetDevicePropertyReply reply;

    REQUEST_SIZE_MATCH(xGetDevicePropertyReq);
    if (stuff->delete)
        UpdateCurrentTime();
    rc = dixLookupDevice(&dev, stuff->deviceid, client,
                         stuff->delete ? DixSetPropAccess : DixGetPropAccess);
    if (rc != Success)
        return rc;

    rc = get_property(client, dev, stuff->property, stuff->type,
                      stuff->delete, stuff->longOffset, stuff->longLength,
                      &bytes_after, &type, &format, &nitems, &length, &data);

    if (rc != Success)
        return rc;

    reply = (xGetDevicePropertyReply) {
        .repType = X_Reply,
        .RepType = X_GetDeviceProperty,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(length),
        .propertyType = type,
        .bytesAfter = bytes_after,
        .nItems = nitems,
        .format = format,
        .deviceid = dev->id
    };

    if (stuff->delete && (reply.bytesAfter == 0))
        send_property_event(dev, stuff->property, XIPropertyDeleted);

    WriteReplyToClient(client, sizeof(xGenericReply), &reply);

    if (length) {
        switch (reply.format) {
        case 32:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;
            break;
        case 16:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap16Write;
            break;
        default:
            client->pSwapReplyFunc = (ReplySwapPtr) WriteToClient;
            break;
        }
        WriteSwappedDataToClient(client, length, data);
    }

    /* delete the Property */
    if (stuff->delete && (reply.bytesAfter == 0)) {
        XIPropertyPtr prop, *prev;

        for (prev = &dev->properties.properties; (prop = *prev);
             prev = &prop->next) {
            if (prop->propertyName == stuff->property) {
                *prev = prop->next;
                XIDestroyDeviceProperty(prop);
                break;
            }
        }
    }
    return Success;
}

int _X_COLD
SProcXListDeviceProperties(ClientPtr client)
{
    REQUEST(xListDevicePropertiesReq);
    REQUEST_SIZE_MATCH(xListDevicePropertiesReq);

    swaps(&stuff->length);
    return (ProcXListDeviceProperties(client));
}

int _X_COLD
SProcXChangeDeviceProperty(ClientPtr client)
{
    REQUEST(xChangeDevicePropertyReq);

    REQUEST_AT_LEAST_SIZE(xChangeDevicePropertyReq);
    swaps(&stuff->length);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->nUnits);
    return (ProcXChangeDeviceProperty(client));
}

int _X_COLD
SProcXDeleteDeviceProperty(ClientPtr client)
{
    REQUEST(xDeleteDevicePropertyReq);
    REQUEST_SIZE_MATCH(xDeleteDevicePropertyReq);

    swaps(&stuff->length);
    swapl(&stuff->property);
    return (ProcXDeleteDeviceProperty(client));
}

int _X_COLD
SProcXGetDeviceProperty(ClientPtr client)
{
    REQUEST(xGetDevicePropertyReq);
    REQUEST_SIZE_MATCH(xGetDevicePropertyReq);

    swaps(&stuff->length);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->longOffset);
    swapl(&stuff->longLength);
    return (ProcXGetDeviceProperty(client));
}

/* Reply swapping */

void _X_COLD
SRepXListDeviceProperties(ClientPtr client, int size,
                          xListDevicePropertiesReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->nAtoms);
    /* properties will be swapped later, see ProcXListDeviceProperties */
    WriteToClient(client, size, rep);
}

void _X_COLD
SRepXGetDeviceProperty(ClientPtr client, int size,
                       xGetDevicePropertyReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swapl(&rep->propertyType);
    swapl(&rep->bytesAfter);
    swapl(&rep->nItems);
    /* data will be swapped, see ProcXGetDeviceProperty */
    WriteToClient(client, size, rep);
}

/* XI2 Request/reply handling */
int
ProcXIListProperties(ClientPtr client)
{
    Atom *atoms;
    xXIListPropertiesReply rep;
    int natoms;
    DeviceIntPtr dev;
    int rc = Success;

    REQUEST(xXIListPropertiesReq);
    REQUEST_SIZE_MATCH(xXIListPropertiesReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixListPropAccess);
    if (rc != Success)
        return rc;

    rc = list_atoms(dev, &natoms, &atoms);
    if (rc != Success)
        return rc;

    rep = (xXIListPropertiesReply) {
        .repType = X_Reply,
        .RepType = X_XIListProperties,
        .sequenceNumber = client->sequence,
        .length = natoms,
        .num_properties = natoms
    };

    WriteReplyToClient(client, sizeof(xXIListPropertiesReply), &rep);
    if (natoms) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, natoms * sizeof(Atom), atoms);
        free(atoms);
    }
    return rc;
}

int
ProcXIChangeProperty(ClientPtr client)
{
    int rc;
    DeviceIntPtr dev;
    uint64_t totalSize;
    unsigned long len;

    REQUEST(xXIChangePropertyReq);
    REQUEST_AT_LEAST_SIZE(xXIChangePropertyReq);
    UpdateCurrentTime();

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixSetPropAccess);
    if (rc != Success)
        return rc;

    rc = check_change_property(client, stuff->property, stuff->type,
                               stuff->format, stuff->mode, stuff->num_items);
    if (rc != Success)
        return rc;

    len = stuff->num_items;
    if (len > bytes_to_int32(0xffffffff - sizeof(xXIChangePropertyReq)))
        return BadLength;

    totalSize = len * (stuff->format / 8);
    REQUEST_FIXED_SIZE(xXIChangePropertyReq, totalSize);

    rc = change_property(client, dev, stuff->property, stuff->type,
                         stuff->format, stuff->mode, len, (void *) &stuff[1]);
    return rc;
}

int
ProcXIDeleteProperty(ClientPtr client)
{
    DeviceIntPtr dev;
    int rc;

    REQUEST(xXIDeletePropertyReq);

    REQUEST_SIZE_MATCH(xXIDeletePropertyReq);
    UpdateCurrentTime();
    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixSetPropAccess);
    if (rc != Success)
        return rc;

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    rc = XIDeleteDeviceProperty(dev, stuff->property, TRUE);
    return rc;
}

int
ProcXIGetProperty(ClientPtr client)
{
    REQUEST(xXIGetPropertyReq);
    DeviceIntPtr dev;
    xXIGetPropertyReply reply;
    int length;
    int rc, format, nitems, bytes_after;
    char *data;
    Atom type;

    REQUEST_SIZE_MATCH(xXIGetPropertyReq);
    if (stuff->delete)
        UpdateCurrentTime();
    rc = dixLookupDevice(&dev, stuff->deviceid, client,
                         stuff->delete ? DixSetPropAccess : DixGetPropAccess);
    if (rc != Success)
        return rc;

    rc = get_property(client, dev, stuff->property, stuff->type,
                      stuff->delete, stuff->offset, stuff->len,
                      &bytes_after, &type, &format, &nitems, &length, &data);

    if (rc != Success)
        return rc;

    reply = (xXIGetPropertyReply) {
        .repType = X_Reply,
        .RepType = X_XIGetProperty,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(length),
        .type = type,
        .bytes_after = bytes_after,
        .num_items = nitems,
        .format = format
    };

    if (length && stuff->delete && (reply.bytes_after == 0))
        send_property_event(dev, stuff->property, XIPropertyDeleted);

    WriteReplyToClient(client, sizeof(xXIGetPropertyReply), &reply);

    if (length) {
        switch (reply.format) {
        case 32:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;
            break;
        case 16:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap16Write;
            break;
        default:
            client->pSwapReplyFunc = (ReplySwapPtr) WriteToClient;
            break;
        }
        WriteSwappedDataToClient(client, length, data);
    }

    /* delete the Property */
    if (stuff->delete && (reply.bytes_after == 0)) {
        XIPropertyPtr prop, *prev;

        for (prev = &dev->properties.properties; (prop = *prev);
             prev = &prop->next) {
            if (prop->propertyName == stuff->property) {
                *prev = prop->next;
                XIDestroyDeviceProperty(prop);
                break;
            }
        }
    }

    return Success;
}

int _X_COLD
SProcXIListProperties(ClientPtr client)
{
    REQUEST(xXIListPropertiesReq);
    REQUEST_SIZE_MATCH(xXIListPropertiesReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    return (ProcXIListProperties(client));
}

int _X_COLD
SProcXIChangeProperty(ClientPtr client)
{
    REQUEST(xXIChangePropertyReq);

    REQUEST_AT_LEAST_SIZE(xXIChangePropertyReq);
    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->num_items);
    return (ProcXIChangeProperty(client));
}

int _X_COLD
SProcXIDeleteProperty(ClientPtr client)
{
    REQUEST(xXIDeletePropertyReq);
    REQUEST_SIZE_MATCH(xXIDeletePropertyReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->property);
    return (ProcXIDeleteProperty(client));
}

int _X_COLD
SProcXIGetProperty(ClientPtr client)
{
    REQUEST(xXIGetPropertyReq);
    REQUEST_SIZE_MATCH(xXIGetPropertyReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->offset);
    swapl(&stuff->len);
    return (ProcXIGetProperty(client));
}

void _X_COLD
SRepXIListProperties(ClientPtr client, int size, xXIListPropertiesReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->num_properties);
    /* properties will be swapped later, see ProcXIListProperties */
    WriteToClient(client, size, rep);
}

void _X_COLD
SRepXIGetProperty(ClientPtr client, int size, xXIGetPropertyReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swapl(&rep->type);
    swapl(&rep->bytes_after);
    swapl(&rep->num_items);
    /* data will be swapped, see ProcXIGetProperty */
    WriteToClient(client, size, rep);
}
