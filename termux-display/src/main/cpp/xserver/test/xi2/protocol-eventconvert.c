/**
 * Copyright © 2009 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 * *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>

#include "inputstr.h"
#include "eventstr.h"
#include "eventconvert.h"
#include "exevents.h"
#include "inpututils.h"
#include <X11/extensions/XI2proto.h>

#include "protocol-common.h"

static void
test_values_XIRawEvent(RawDeviceEvent *in, xXIRawEvent * out, BOOL swap)
{
    int i;
    unsigned char *ptr;
    FP3232 *value, *raw_value;
    int nvals = 0;
    int bits_set;
    int len;
    uint32_t flagmask = 0;

    if (swap) {
        swaps(&out->sequenceNumber);
        swapl(&out->length);
        swaps(&out->evtype);
        swaps(&out->deviceid);
        swapl(&out->time);
        swapl(&out->detail);
        swaps(&out->valuators_len);
        swapl(&out->flags);
    }

    assert(out->type == GenericEvent);
    assert(out->extension == 0);        /* IReqCode defaults to 0 */
    assert(out->evtype == GetXI2Type(in->type));
    assert(out->time == in->time);
    assert(out->detail == in->detail.button);
    assert(out->deviceid == in->deviceid);
    assert(out->valuators_len >=
           bytes_to_int32(bits_to_bytes(sizeof(in->valuators.mask))));

    switch (in->type) {
    case ET_RawMotion:
    case ET_RawButtonPress:
    case ET_RawButtonRelease:
        flagmask = XIPointerEmulated;
        break;
    default:
        flagmask = 0;
    }
    assert((out->flags & ~flagmask) == 0);

    ptr = (unsigned char *) &out[1];
    bits_set = 0;

    for (i = 0; out->valuators_len && i < sizeof(in->valuators.mask) * 8; i++) {
        if (i >= MAX_VALUATORS)
            assert(!XIMaskIsSet(in->valuators.mask, i));
        assert(XIMaskIsSet(in->valuators.mask, i) == XIMaskIsSet(ptr, i));
        if (XIMaskIsSet(in->valuators.mask, i))
            bits_set++;
    }

    /* length is len of valuator mask (in 4-byte units) + the number of bits
     * set. Each bit set represents 2 8-byte values, hence the
     * 'bits_set * 4' */
    len = out->valuators_len + bits_set * 4;
    assert(out->length == len);

    nvals = 0;

    for (i = 0; out->valuators_len && i < MAX_VALUATORS; i++) {
        assert(XIMaskIsSet(in->valuators.mask, i) == XIMaskIsSet(ptr, i));
        if (XIMaskIsSet(in->valuators.mask, i)) {
            FP3232 vi, vo;

            value =
                (FP3232 *) (((unsigned char *) &out[1]) +
                            out->valuators_len * 4);
            value += nvals;

            vi = double_to_fp3232(in->valuators.data[i]);

            vo.integral = value->integral;
            vo.frac = value->frac;
            if (swap) {
                swapl(&vo.integral);
                swapl(&vo.frac);
            }

            assert(vi.integral == vo.integral);
            assert(vi.frac == vo.frac);

            raw_value = value + bits_set;

            vi = double_to_fp3232(in->valuators.data_raw[i]);

            vo.integral = raw_value->integral;
            vo.frac = raw_value->frac;
            if (swap) {
                swapl(&vo.integral);
                swapl(&vo.frac);
            }

            assert(vi.integral == vo.integral);
            assert(vi.frac == vo.frac);

            nvals++;
        }
    }
}

static void
test_XIRawEvent(RawDeviceEvent *in)
{
    xXIRawEvent *out, *swapped;
    int rc;

    rc = EventToXI2((InternalEvent *) in, (xEvent **) &out);
    assert(rc == Success);

    test_values_XIRawEvent(in, out, FALSE);

    swapped = calloc(1, sizeof(xEvent) + out->length * 4);
    XI2EventSwap((xGenericEvent *) out, (xGenericEvent *) swapped);
    test_values_XIRawEvent(in, swapped, TRUE);

    free(out);
    free(swapped);
}

static void
test_convert_XIFocusEvent(void)
{
    xEvent *out;
    DeviceEvent in;
    int rc;

    in.header = ET_Internal;
    in.type = ET_Enter;
    rc = EventToXI2((InternalEvent *) &in, &out);
    assert(rc == Success);
    assert(out == NULL);

    in.header = ET_Internal;
    in.type = ET_FocusIn;
    rc = EventToXI2((InternalEvent *) &in, &out);
    assert(rc == Success);
    assert(out == NULL);

    in.header = ET_Internal;
    in.type = ET_FocusOut;
    rc = EventToXI2((InternalEvent *) &in, &out);
    assert(rc == BadImplementation);

    in.header = ET_Internal;
    in.type = ET_Leave;
    rc = EventToXI2((InternalEvent *) &in, &out);
    assert(rc == BadImplementation);
}

static void
test_convert_XIRawEvent(void)
{
    RawDeviceEvent in;
    int i;

    memset(&in, 0, sizeof(in));

    in.header = ET_Internal;
    in.type = ET_RawMotion;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawKeyPress;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawKeyRelease;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawButtonPress;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawButtonRelease;
    test_XIRawEvent(&in);

    in.detail.button = 1L;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 8;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 16;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 24;
    test_XIRawEvent(&in);
    in.detail.button = ~0L;
    test_XIRawEvent(&in);

    in.detail.button = 0;

    in.time = 1L;
    test_XIRawEvent(&in);
    in.time = 1L << 8;
    test_XIRawEvent(&in);
    in.time = 1L << 16;
    test_XIRawEvent(&in);
    in.time = 1L << 24;
    test_XIRawEvent(&in);
    in.time = ~0L;
    test_XIRawEvent(&in);

    in.deviceid = 1;
    test_XIRawEvent(&in);
    in.deviceid = 1 << 8;
    test_XIRawEvent(&in);
    in.deviceid = ~0 & 0xFF;
    test_XIRawEvent(&in);

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);
        test_XIRawEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);

        in.valuators.data[i] = i + (i * 0.0010);
        in.valuators.data_raw[i] = (i + 10) + (i * 0.0030);
        test_XIRawEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);
        test_XIRawEvent(&in);
    }
}

static void
test_values_XIDeviceEvent(DeviceEvent *in, xXIDeviceEvent * out, BOOL swap)
{
    int buttons, valuators;
    int i;
    unsigned char *ptr;
    uint32_t flagmask = 0;
    FP3232 *values;

    if (swap) {
        swaps(&out->sequenceNumber);
        swapl(&out->length);
        swaps(&out->evtype);
        swaps(&out->deviceid);
        swaps(&out->sourceid);
        swapl(&out->time);
        swapl(&out->detail);
        swapl(&out->root);
        swapl(&out->event);
        swapl(&out->child);
        swapl(&out->root_x);
        swapl(&out->root_y);
        swapl(&out->event_x);
        swapl(&out->event_y);
        swaps(&out->buttons_len);
        swaps(&out->valuators_len);
        swapl(&out->mods.base_mods);
        swapl(&out->mods.latched_mods);
        swapl(&out->mods.locked_mods);
        swapl(&out->mods.effective_mods);
        swapl(&out->flags);
    }

    assert(out->extension == 0);        /* IReqCode defaults to 0 */
    assert(out->evtype == GetXI2Type(in->type));
    assert(out->time == in->time);
    assert(out->detail == in->detail.button);
    assert(out->length >= 12);

    assert(out->deviceid == in->deviceid);
    assert(out->sourceid == in->sourceid);

    switch (in->type) {
    case ET_ButtonPress:
    case ET_Motion:
    case ET_ButtonRelease:
        flagmask = XIPointerEmulated;
        break;
    case ET_KeyPress:
        flagmask = XIKeyRepeat;
        break;
    default:
        flagmask = 0;
        break;
    }
    assert((out->flags & ~flagmask) == 0);

    assert(out->root == in->root);
    assert(out->event == None); /* set in FixUpEventFromWindow */
    assert(out->child == None); /* set in FixUpEventFromWindow */

    assert(out->mods.base_mods == in->mods.base);
    assert(out->mods.latched_mods == in->mods.latched);
    assert(out->mods.locked_mods == in->mods.locked);
    assert(out->mods.effective_mods == in->mods.effective);

    assert(out->group.base_group == in->group.base);
    assert(out->group.latched_group == in->group.latched);
    assert(out->group.locked_group == in->group.locked);
    assert(out->group.effective_group == in->group.effective);

    assert(out->event_x == 0);  /* set in FixUpEventFromWindow */
    assert(out->event_y == 0);  /* set in FixUpEventFromWindow */

    assert(out->root_x == double_to_fp1616(in->root_x + in->root_x_frac));
    assert(out->root_y == double_to_fp1616(in->root_y + in->root_y_frac));

    buttons = 0;
    for (i = 0; i < bits_to_bytes(sizeof(in->buttons)); i++) {
        if (XIMaskIsSet(in->buttons, i)) {
            assert(out->buttons_len >= bytes_to_int32(bits_to_bytes(i)));
            buttons++;
        }
    }

    ptr = (unsigned char *) &out[1];
    for (i = 0; i < sizeof(in->buttons) * 8; i++)
        assert(XIMaskIsSet(in->buttons, i) == XIMaskIsSet(ptr, i));

    valuators = 0;
    for (i = 0; i < MAX_VALUATORS; i++)
        if (XIMaskIsSet(in->valuators.mask, i))
            valuators++;

    assert(out->valuators_len >= bytes_to_int32(bits_to_bytes(valuators)));

    ptr += out->buttons_len * 4;
    values = (FP3232 *) (ptr + out->valuators_len * 4);
    for (i = 0; i < sizeof(in->valuators.mask) * 8 ||
         i < (out->valuators_len * 4) * 8; i++) {
        if (i >= MAX_VALUATORS) {
            assert(!XIMaskIsSet(in->valuators.mask, i));
            assert(!XIMaskIsSet(ptr, i));
        }
        else if (i > sizeof(in->valuators.mask) * 8)
            assert(!XIMaskIsSet(ptr, i));
        else if (i > out->valuators_len * 4 * 8)
            assert(!XIMaskIsSet(in->valuators.mask, i));
        else {
            assert(XIMaskIsSet(in->valuators.mask, i) == XIMaskIsSet(ptr, i));

            if (XIMaskIsSet(ptr, i)) {
                FP3232 vi, vo;

                vi = double_to_fp3232(in->valuators.data[i]);
                vo = *values;

                if (swap) {
                    swapl(&vo.integral);
                    swapl(&vo.frac);
                }

                assert(vi.integral == vo.integral);
                assert(vi.frac == vo.frac);
                values++;
            }
        }
    }
}

static void
test_XIDeviceEvent(DeviceEvent *in)
{
    xXIDeviceEvent *out, *swapped;
    int rc;

    rc = EventToXI2((InternalEvent *) in, (xEvent **) &out);
    assert(rc == Success);

    test_values_XIDeviceEvent(in, out, FALSE);

    swapped = calloc(1, sizeof(xEvent) + out->length * 4);
    XI2EventSwap((xGenericEvent *) out, (xGenericEvent *) swapped);
    test_values_XIDeviceEvent(in, swapped, TRUE);

    free(out);
    free(swapped);
}

static void
test_convert_XIDeviceEvent(void)
{
    DeviceEvent in;
    int i;

    memset(&in, 0, sizeof(in));

    in.header = ET_Internal;
    in.type = ET_Motion;
    in.length = sizeof(DeviceEvent);
    in.time = 0;
    in.deviceid = 1;
    in.sourceid = 2;
    in.root = 3;
    in.root_x = 4;
    in.root_x_frac = 5;
    in.root_y = 6;
    in.root_y_frac = 7;
    in.detail.button = 8;
    in.mods.base = 9;
    in.mods.latched = 10;
    in.mods.locked = 11;
    in.mods.effective = 11;
    in.group.base = 12;
    in.group.latched = 13;
    in.group.locked = 14;
    in.group.effective = 15;

    test_XIDeviceEvent(&in);

    /* 32 bit */
    in.detail.button = 1L;
    test_XIDeviceEvent(&in);
    in.detail.button = 1L << 8;
    test_XIDeviceEvent(&in);
    in.detail.button = 1L << 16;
    test_XIDeviceEvent(&in);
    in.detail.button = 1L << 24;
    test_XIDeviceEvent(&in);
    in.detail.button = ~0L;
    test_XIDeviceEvent(&in);

    /* 32 bit */
    in.time = 1L;
    test_XIDeviceEvent(&in);
    in.time = 1L << 8;
    test_XIDeviceEvent(&in);
    in.time = 1L << 16;
    test_XIDeviceEvent(&in);
    in.time = 1L << 24;
    test_XIDeviceEvent(&in);
    in.time = ~0L;
    test_XIDeviceEvent(&in);

    /* 16 bit */
    in.deviceid = 1;
    test_XIDeviceEvent(&in);
    in.deviceid = 1 << 8;
    test_XIDeviceEvent(&in);
    in.deviceid = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    /* 16 bit */
    in.sourceid = 1;
    test_XIDeviceEvent(&in);
    in.deviceid = 1 << 8;
    test_XIDeviceEvent(&in);
    in.deviceid = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    /* 32 bit */
    in.root = 1L;
    test_XIDeviceEvent(&in);
    in.root = 1L << 8;
    test_XIDeviceEvent(&in);
    in.root = 1L << 16;
    test_XIDeviceEvent(&in);
    in.root = 1L << 24;
    test_XIDeviceEvent(&in);
    in.root = ~0L;
    test_XIDeviceEvent(&in);

    /* 16 bit */
    in.root_x = 1;
    test_XIDeviceEvent(&in);
    in.root_x = 1 << 8;
    test_XIDeviceEvent(&in);
    in.root_x = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.root_x_frac = 1;
    test_XIDeviceEvent(&in);
    in.root_x_frac = 1 << 8;
    test_XIDeviceEvent(&in);
    in.root_x_frac = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.root_y = 1;
    test_XIDeviceEvent(&in);
    in.root_y = 1 << 8;
    test_XIDeviceEvent(&in);
    in.root_y = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.root_y_frac = 1;
    test_XIDeviceEvent(&in);
    in.root_y_frac = 1 << 8;
    test_XIDeviceEvent(&in);
    in.root_y_frac = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    /* 32 bit */
    in.mods.base = 1L;
    test_XIDeviceEvent(&in);
    in.mods.base = 1L << 8;
    test_XIDeviceEvent(&in);
    in.mods.base = 1L << 16;
    test_XIDeviceEvent(&in);
    in.mods.base = 1L << 24;
    test_XIDeviceEvent(&in);
    in.mods.base = ~0L;
    test_XIDeviceEvent(&in);

    in.mods.latched = 1L;
    test_XIDeviceEvent(&in);
    in.mods.latched = 1L << 8;
    test_XIDeviceEvent(&in);
    in.mods.latched = 1L << 16;
    test_XIDeviceEvent(&in);
    in.mods.latched = 1L << 24;
    test_XIDeviceEvent(&in);
    in.mods.latched = ~0L;
    test_XIDeviceEvent(&in);

    in.mods.locked = 1L;
    test_XIDeviceEvent(&in);
    in.mods.locked = 1L << 8;
    test_XIDeviceEvent(&in);
    in.mods.locked = 1L << 16;
    test_XIDeviceEvent(&in);
    in.mods.locked = 1L << 24;
    test_XIDeviceEvent(&in);
    in.mods.locked = ~0L;
    test_XIDeviceEvent(&in);

    in.mods.effective = 1L;
    test_XIDeviceEvent(&in);
    in.mods.effective = 1L << 8;
    test_XIDeviceEvent(&in);
    in.mods.effective = 1L << 16;
    test_XIDeviceEvent(&in);
    in.mods.effective = 1L << 24;
    test_XIDeviceEvent(&in);
    in.mods.effective = ~0L;
    test_XIDeviceEvent(&in);

    /* 8 bit */
    in.group.base = 1;
    test_XIDeviceEvent(&in);
    in.group.base = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.group.latched = 1;
    test_XIDeviceEvent(&in);
    in.group.latched = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.group.locked = 1;
    test_XIDeviceEvent(&in);
    in.group.locked = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    in.mods.effective = 1;
    test_XIDeviceEvent(&in);
    in.mods.effective = ~0 & 0xFF;
    test_XIDeviceEvent(&in);

    for (i = 0; i < sizeof(in.buttons) * 8; i++) {
        XISetMask(in.buttons, i);
        test_XIDeviceEvent(&in);
        XIClearMask(in.buttons, i);
    }

    for (i = 0; i < sizeof(in.buttons) * 8; i++) {
        XISetMask(in.buttons, i);
        test_XIDeviceEvent(&in);
    }

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);
        test_XIDeviceEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);

        in.valuators.data[i] = i + (i * 0.0020);
        test_XIDeviceEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < MAX_VALUATORS; i++) {
        XISetMask(in.valuators.mask, i);
        test_XIDeviceEvent(&in);
    }
}

static void
test_values_XIDeviceChangedEvent(DeviceChangedEvent *in,
                                 xXIDeviceChangedEvent * out, BOOL swap)
{
    int i, j;
    unsigned char *ptr;

    if (swap) {
        swaps(&out->sequenceNumber);
        swapl(&out->length);
        swaps(&out->evtype);
        swaps(&out->deviceid);
        swaps(&out->sourceid);
        swapl(&out->time);
        swaps(&out->num_classes);
    }

    assert(out->type == GenericEvent);
    assert(out->extension == 0);        /* IReqCode defaults to 0 */
    assert(out->evtype == GetXI2Type(in->type));
    assert(out->time == in->time);
    assert(out->deviceid == in->deviceid);
    assert(out->sourceid == in->sourceid);

    ptr = (unsigned char *) &out[1];
    for (i = 0; i < out->num_classes; i++) {
        xXIAnyInfo *any = (xXIAnyInfo *) ptr;

        if (swap) {
            swaps(&any->length);
            swaps(&any->type);
            swaps(&any->sourceid);
        }

        switch (any->type) {
        case XIButtonClass:
        {
            xXIButtonInfo *b = (xXIButtonInfo *) any;
            Atom *names;

            if (swap) {
                swaps(&b->num_buttons);
            }

            assert(b->length ==
                   bytes_to_int32(sizeof(xXIButtonInfo)) +
                   bytes_to_int32(bits_to_bytes(b->num_buttons)) +
                   b->num_buttons);
            assert(b->num_buttons == in->buttons.num_buttons);

            names = (Atom *) ((char *) &b[1] +
                              pad_to_int32(bits_to_bytes(b->num_buttons)));
            for (j = 0; j < b->num_buttons; j++) {
                if (swap) {
                    swapl(&names[j]);
                }
                assert(names[j] == in->buttons.names[j]);
            }
        }
            break;
        case XIKeyClass:
        {
            xXIKeyInfo *k = (xXIKeyInfo *) any;
            uint32_t *kc;

            if (swap) {
                swaps(&k->num_keycodes);
            }

            assert(k->length ==
                   bytes_to_int32(sizeof(xXIKeyInfo)) + k->num_keycodes);
            assert(k->num_keycodes == in->keys.max_keycode -
                   in->keys.min_keycode + 1);

            kc = (uint32_t *) &k[1];
            for (j = 0; j < k->num_keycodes; j++) {
                if (swap) {
                    swapl(&kc[j]);
                }
                assert(kc[j] >= in->keys.min_keycode);
                assert(kc[j] <= in->keys.max_keycode);
            }
        }
            break;
        case XIValuatorClass:
        {
            xXIValuatorInfo *v = (xXIValuatorInfo *) any;

            assert(v->length == bytes_to_int32(sizeof(xXIValuatorInfo)));

        }
            break;
        case XIScrollClass:
        {
            xXIScrollInfo *s = (xXIScrollInfo *) any;

            assert(s->length == bytes_to_int32(sizeof(xXIScrollInfo)));

            assert(s->sourceid == in->sourceid);
            assert(s->number < in->num_valuators);
            switch (s->type) {
            case XIScrollTypeVertical:
                assert(in->valuators[s->number].scroll.type ==
                       SCROLL_TYPE_VERTICAL);
                break;
            case XIScrollTypeHorizontal:
                assert(in->valuators[s->number].scroll.type ==
                       SCROLL_TYPE_HORIZONTAL);
                break;
            }
            if (s->flags & XIScrollFlagPreferred)
                assert(in->valuators[s->number].scroll.
                       flags & SCROLL_FLAG_PREFERRED);
        }
        default:
            printf("Invalid class type.\n\n");
            assert(1);
            break;
        }

        ptr += any->length * 4;
    }

}

static void
test_XIDeviceChangedEvent(DeviceChangedEvent *in)
{
    xXIDeviceChangedEvent *out, *swapped;
    int rc;

    rc = EventToXI2((InternalEvent *) in, (xEvent **) &out);
    assert(rc == Success);

    test_values_XIDeviceChangedEvent(in, out, FALSE);

    swapped = calloc(1, sizeof(xEvent) + out->length * 4);
    XI2EventSwap((xGenericEvent *) out, (xGenericEvent *) swapped);
    test_values_XIDeviceChangedEvent(in, swapped, TRUE);

    free(out);
    free(swapped);
}

static void
test_convert_XIDeviceChangedEvent(void)
{
    DeviceChangedEvent in;
    int i;

    memset(&in, 0, sizeof(in));
    in.header = ET_Internal;
    in.type = ET_DeviceChanged;
    in.length = sizeof(DeviceChangedEvent);
    in.time = 0;
    in.deviceid = 1;
    in.sourceid = 2;
    in.masterid = 3;
    in.num_valuators = 4;
    in.flags =
        DEVCHANGE_SLAVE_SWITCH | DEVCHANGE_POINTER_EVENT |
        DEVCHANGE_KEYBOARD_EVENT;

    for (i = 0; i < MAX_BUTTONS; i++)
        in.buttons.names[i] = i + 10;

    in.keys.min_keycode = 8;
    in.keys.max_keycode = 255;

    test_XIDeviceChangedEvent(&in);

    in.time = 1L;
    test_XIDeviceChangedEvent(&in);
    in.time = 1L << 8;
    test_XIDeviceChangedEvent(&in);
    in.time = 1L << 16;
    test_XIDeviceChangedEvent(&in);
    in.time = 1L << 24;
    test_XIDeviceChangedEvent(&in);
    in.time = ~0L;
    test_XIDeviceChangedEvent(&in);

    in.deviceid = 1L;
    test_XIDeviceChangedEvent(&in);
    in.deviceid = 1L << 8;
    test_XIDeviceChangedEvent(&in);
    in.deviceid = ~0 & 0xFFFF;
    test_XIDeviceChangedEvent(&in);

    in.sourceid = 1L;
    test_XIDeviceChangedEvent(&in);
    in.sourceid = 1L << 8;
    test_XIDeviceChangedEvent(&in);
    in.sourceid = ~0 & 0xFFFF;
    test_XIDeviceChangedEvent(&in);

    in.masterid = 1L;
    test_XIDeviceChangedEvent(&in);
    in.masterid = 1L << 8;
    test_XIDeviceChangedEvent(&in);
    in.masterid = ~0 & 0xFFFF;
    test_XIDeviceChangedEvent(&in);

    in.buttons.num_buttons = 0;
    test_XIDeviceChangedEvent(&in);

    in.buttons.num_buttons = 1;
    test_XIDeviceChangedEvent(&in);

    in.buttons.num_buttons = MAX_BUTTONS;
    test_XIDeviceChangedEvent(&in);

    in.keys.min_keycode = 0;
    in.keys.max_keycode = 0;
    test_XIDeviceChangedEvent(&in);

    in.keys.max_keycode = 1 << 8;
    test_XIDeviceChangedEvent(&in);

    in.keys.max_keycode = 0xFFFC;       /* highest range, above that the length
                                           field gives up */
    test_XIDeviceChangedEvent(&in);

    in.keys.min_keycode = 1 << 8;
    in.keys.max_keycode = 1 << 8;
    test_XIDeviceChangedEvent(&in);

    in.keys.min_keycode = 1 << 8;
    in.keys.max_keycode = 0;
    test_XIDeviceChangedEvent(&in);

    in.num_valuators = 0;
    test_XIDeviceChangedEvent(&in);

    in.num_valuators = 1;
    test_XIDeviceChangedEvent(&in);

    in.num_valuators = MAX_VALUATORS;
    test_XIDeviceChangedEvent(&in);

    for (i = 0; i < MAX_VALUATORS; i++) {
        in.valuators[i].min = 0;
        in.valuators[i].max = 0;
        test_XIDeviceChangedEvent(&in);

        in.valuators[i].max = 1 << 8;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].max = 1 << 16;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].max = 1 << 24;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].max = abs(~0);
        test_XIDeviceChangedEvent(&in);

        in.valuators[i].resolution = 1 << 8;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].resolution = 1 << 16;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].resolution = 1 << 24;
        test_XIDeviceChangedEvent(&in);
        in.valuators[i].resolution = abs(~0);
        test_XIDeviceChangedEvent(&in);

        in.valuators[i].name = i;
        test_XIDeviceChangedEvent(&in);

        in.valuators[i].mode = Relative;
        test_XIDeviceChangedEvent(&in);

        in.valuators[i].mode = Absolute;
        test_XIDeviceChangedEvent(&in);
    }
}

static void
test_values_XITouchOwnershipEvent(TouchOwnershipEvent *in,
                                  xXITouchOwnershipEvent * out, BOOL swap)
{
    if (swap) {
        swaps(&out->sequenceNumber);
        swapl(&out->length);
        swaps(&out->evtype);
        swaps(&out->deviceid);
        swaps(&out->sourceid);
        swapl(&out->time);
        swapl(&out->touchid);
        swapl(&out->root);
        swapl(&out->event);
        swapl(&out->child);
        swapl(&out->time);
    }

    assert(out->type == GenericEvent);
    assert(out->extension == 0);        /* IReqCode defaults to 0 */
    assert(out->evtype == GetXI2Type(in->type));
    assert(out->time == in->time);
    assert(out->deviceid == in->deviceid);
    assert(out->sourceid == in->sourceid);
    assert(out->touchid == in->touchid);
    assert(out->flags == in->reason);
}

static void
test_XITouchOwnershipEvent(TouchOwnershipEvent *in)
{
    xXITouchOwnershipEvent *out, *swapped;
    int rc;

    rc = EventToXI2((InternalEvent *) in, (xEvent **) &out);
    assert(rc == Success);

    test_values_XITouchOwnershipEvent(in, out, FALSE);

    swapped = calloc(1, sizeof(xEvent) + out->length * 4);
    XI2EventSwap((xGenericEvent *) out, (xGenericEvent *) swapped);
    test_values_XITouchOwnershipEvent(in, swapped, TRUE);
    free(out);
    free(swapped);
}

static void
test_convert_XITouchOwnershipEvent(void)
{
    TouchOwnershipEvent in;
    long i;

    memset(&in, 0, sizeof(in));
    in.header = ET_Internal;
    in.type = ET_TouchOwnership;
    in.length = sizeof(in);
    in.time = 0;
    in.deviceid = 1;
    in.sourceid = 2;
    in.touchid = 0;
    in.reason = 0;
    in.resource = 0;
    in.flags = 0;

    test_XITouchOwnershipEvent(&in);

    in.flags = XIAcceptTouch;
    test_XITouchOwnershipEvent(&in);

    in.flags = XIRejectTouch;
    test_XITouchOwnershipEvent(&in);

    for (i = 1; i <= 0xFFFF; i <<= 1) {
        in.deviceid = i;
        test_XITouchOwnershipEvent(&in);
    }

    for (i = 1; i <= 0xFFFF; i <<= 1) {
        in.sourceid = i;
        test_XITouchOwnershipEvent(&in);
    }

    for (i = 1;; i <<= 1) {
        in.touchid = i;
        test_XITouchOwnershipEvent(&in);
        if (i == ((long) 1 << 31))
            break;
    }
}

static void
test_XIBarrierEvent(BarrierEvent *in)
{
    xXIBarrierEvent *out, *swapped;
    int count;
    int rc;
    int eventlen;
    FP3232 value;

    rc = EventToXI((InternalEvent*)in, (xEvent**)&out, &count);
    assert(rc == BadMatch);

    rc = EventToCore((InternalEvent*)in, (xEvent**)&out, &count);
    assert(rc == BadMatch);

    rc = EventToXI2((InternalEvent*)in, (xEvent**)&out);

    assert(out->type == GenericEvent);
    assert(out->extension == 0); /* IReqCode defaults to 0 */
    assert(out->evtype == GetXI2Type(in->type));
    assert(out->time == in->time);
    assert(out->deviceid == in->deviceid);
    assert(out->sourceid == in->sourceid);
    assert(out->barrier == in->barrierid);
    assert(out->flags == in->flags);
    assert(out->event == in->window);
    assert(out->root == in->root);
    assert(out->dtime == in->dt);
    assert(out->eventid == in->event_id);
    assert(out->root_x == double_to_fp1616(in->root_x));
    assert(out->root_y == double_to_fp1616(in->root_y));

    value = double_to_fp3232(in->dx);
    assert(out->dx.integral == value.integral);
    assert(out->dx.frac == value.frac);
    value = double_to_fp3232(in->dy);
    assert(out->dy.integral == value.integral);
    assert(out->dy.frac == value.frac);

    eventlen = sizeof(xEvent) + out->length * 4;
    swapped = calloc(1, eventlen);
    XI2EventSwap((xGenericEvent *) out, (xGenericEvent *) swapped);

    swaps(&swapped->sequenceNumber);
    swapl(&swapped->length);
    swaps(&swapped->evtype);
    swaps(&swapped->deviceid);
    swapl(&swapped->time);
    swapl(&swapped->eventid);
    swapl(&swapped->root);
    swapl(&swapped->event);
    swapl(&swapped->barrier);
    swapl(&swapped->dtime);
    swaps(&swapped->sourceid);
    swapl(&swapped->root_x);
    swapl(&swapped->root_y);
    swapl(&swapped->dx.integral);
    swapl(&swapped->dx.frac);
    swapl(&swapped->dy.integral);
    swapl(&swapped->dy.frac);

    assert(memcmp(swapped, out, eventlen) == 0);

    free(swapped);
    free(out);
}

static void
test_convert_XIBarrierEvent(void)
{
    BarrierEvent in;

    memset(&in, 0, sizeof(in));
    in.header = ET_Internal;
    in.type = ET_BarrierHit;
    in.length = sizeof(in);
    in.time = 0;
    in.deviceid = 1;
    in.sourceid = 2;

    test_XIBarrierEvent(&in);

    in.deviceid = 1;
    while(in.deviceid & 0xFFFF) {
        test_XIBarrierEvent(&in);
        in.deviceid <<= 1;
    }
    in.deviceid = 0;

    in.sourceid = 1;
    while(in.sourceid & 0xFFFF) {
        test_XIBarrierEvent(&in);
        in.sourceid <<= 1;
    }
    in.sourceid = 0;

    in.flags = 1;
    while(in.flags) {
        test_XIBarrierEvent(&in);
        in.flags <<= 1;
    }

    in.barrierid = 1;
    while(in.barrierid) {
        test_XIBarrierEvent(&in);
        in.barrierid <<= 1;
    }

    in.dt = 1;
    while(in.dt) {
        test_XIBarrierEvent(&in);
        in.dt <<= 1;
    }

    in.event_id = 1;
    while(in.event_id) {
        test_XIBarrierEvent(&in);
        in.event_id <<= 1;
    }

    in.window = 1;
    while(in.window) {
        test_XIBarrierEvent(&in);
        in.window <<= 1;
    }

    in.root = 1;
    while(in.root) {
        test_XIBarrierEvent(&in);
        in.root <<= 1;
    }

    /* pseudo-random 16 bit numbers */
    in.root_x = 1;
    test_XIBarrierEvent(&in);
    in.root_x = 1.3;
    test_XIBarrierEvent(&in);
    in.root_x = 264.908;
    test_XIBarrierEvent(&in);
    in.root_x = 35638.292;
    test_XIBarrierEvent(&in);

    in.root_x = -1;
    test_XIBarrierEvent(&in);
    in.root_x = -1.3;
    test_XIBarrierEvent(&in);
    in.root_x = -264.908;
    test_XIBarrierEvent(&in);
    in.root_x = -35638.292;
    test_XIBarrierEvent(&in);

    in.root_y = 1;
    test_XIBarrierEvent(&in);
    in.root_y = 1.3;
    test_XIBarrierEvent(&in);
    in.root_y = 264.908;
    test_XIBarrierEvent(&in);
    in.root_y = 35638.292;
    test_XIBarrierEvent(&in);

    in.root_y = -1;
    test_XIBarrierEvent(&in);
    in.root_y = -1.3;
    test_XIBarrierEvent(&in);
    in.root_y = -264.908;
    test_XIBarrierEvent(&in);
    in.root_y = -35638.292;
    test_XIBarrierEvent(&in);

    /* equally pseudo-random 32 bit numbers */
    in.dx = 1;
    test_XIBarrierEvent(&in);
    in.dx = 1.3;
    test_XIBarrierEvent(&in);
    in.dx = 264.908;
    test_XIBarrierEvent(&in);
    in.dx = 35638.292;
    test_XIBarrierEvent(&in);
    in.dx = 2947813871.2342;
    test_XIBarrierEvent(&in);

    in.dx = -1;
    test_XIBarrierEvent(&in);
    in.dx = -1.3;
    test_XIBarrierEvent(&in);
    in.dx = -264.908;
    test_XIBarrierEvent(&in);
    in.dx = -35638.292;
    test_XIBarrierEvent(&in);
    in.dx = -2947813871.2342;
    test_XIBarrierEvent(&in);

    in.dy = 1;
    test_XIBarrierEvent(&in);
    in.dy = 1.3;
    test_XIBarrierEvent(&in);
    in.dy = 264.908;
    test_XIBarrierEvent(&in);
    in.dy = 35638.292;
    test_XIBarrierEvent(&in);
    in.dy = 2947813871.2342;
    test_XIBarrierEvent(&in);

    in.dy = -1;
    test_XIBarrierEvent(&in);
    in.dy = -1.3;
    test_XIBarrierEvent(&in);
    in.dy = -264.908;
    test_XIBarrierEvent(&in);
    in.dy = -35638.292;
    test_XIBarrierEvent(&in);
    in.dy = -2947813871.2342;
    test_XIBarrierEvent(&in);
}

int
protocol_eventconvert_test(void)
{
    test_convert_XIRawEvent();
    test_convert_XIFocusEvent();
    test_convert_XIDeviceEvent();
    test_convert_XIDeviceChangedEvent();
    test_convert_XITouchOwnershipEvent();
    test_convert_XIBarrierEvent();

    return 0;
}
