/**
 * Copyright © 2011 Red Hat, Inc.
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
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "inpututils.h"
#include "inputstr.h"
#include "assert.h"

#include "protocol-common.h"

ClientRec client_window;

static void
xi2mask_test(void)
{
    XI2Mask *xi2mask = NULL, *mergemask = NULL;
    unsigned char *mask;
    DeviceIntRec dev;
    DeviceIntRec all_devices, all_master_devices;
    int i;

    all_devices.id = XIAllDevices;
    inputInfo.all_devices = &all_devices;
    all_master_devices.id = XIAllMasterDevices;
    inputInfo.all_master_devices = &all_master_devices;

    /* size >= nmasks * 2 for the test cases below */
    xi2mask = xi2mask_new_with_size(MAXDEVICES + 2, (MAXDEVICES + 2) * 2);
    assert(xi2mask);
    assert(xi2mask->nmasks > 0);
    assert(xi2mask->mask_size > 0);

    assert(xi2mask_mask_size(xi2mask) == xi2mask->mask_size);
    assert(xi2mask_num_masks(xi2mask) == xi2mask->nmasks);

    mask = calloc(1, xi2mask_mask_size(xi2mask));
    /* ensure zeros */
    for (i = 0; i < xi2mask_num_masks(xi2mask); i++) {
        const unsigned char *m = xi2mask_get_one_mask(xi2mask, i);

        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
    }

    /* set various bits */
    for (i = 0; i < xi2mask_num_masks(xi2mask); i++) {
        const unsigned char *m;

        xi2mask_set(xi2mask, i, i);

        dev.id = i;
        assert(xi2mask_isset(xi2mask, &dev, i));

        m = xi2mask_get_one_mask(xi2mask, i);
        SetBit(mask, i);
        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
        ClearBit(mask, i);
    }

    /* ensure zeros one-by-one */
    for (i = 0; i < xi2mask_num_masks(xi2mask); i++) {
        const unsigned char *m = xi2mask_get_one_mask(xi2mask, i);

        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) != 0);
        xi2mask_zero(xi2mask, i);
        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
    }

    /* re-set, zero all */
    for (i = 0; i < xi2mask_num_masks(xi2mask); i++)
        xi2mask_set(xi2mask, i, i);
    xi2mask_zero(xi2mask, -1);

    for (i = 0; i < xi2mask_num_masks(xi2mask); i++) {
        const unsigned char *m = xi2mask_get_one_mask(xi2mask, i);

        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
    }

    for (i = 0; i < xi2mask_num_masks(xi2mask); i++) {
        const unsigned char *m;

        SetBit(mask, i);
        xi2mask_set_one_mask(xi2mask, i, mask, xi2mask_mask_size(xi2mask));
        m = xi2mask_get_one_mask(xi2mask, i);
        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
        ClearBit(mask, i);
    }

    mergemask = xi2mask_new_with_size(MAXDEVICES + 2, (MAXDEVICES + 2) * 2);
    for (i = 0; i < xi2mask_num_masks(mergemask); i++) {
        dev.id = i;
        xi2mask_set(mergemask, i, i * 2);
    }

    /* xi2mask still has all i bits set, should now also have all i * 2 bits */
    xi2mask_merge(xi2mask, mergemask);
    for (i = 0; i < xi2mask_num_masks(mergemask); i++) {
        const unsigned char *m = xi2mask_get_one_mask(xi2mask, i);

        SetBit(mask, i);
        SetBit(mask, i * 2);
        assert(memcmp(mask, m, xi2mask_mask_size(xi2mask)) == 0);
        ClearBit(mask, i);
        ClearBit(mask, i * 2);
    }

    xi2mask_free(&xi2mask);
    assert(xi2mask == NULL);

    xi2mask_free(&mergemask);
    assert(mergemask == NULL);
    free(mask);
}

int
xi2_test(void)
{
    xi2mask_test();

    return 0;
}
