/*
 * Copyright © 2006-2007 Daniel Stone
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
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef HOTPLUG_H
#define HOTPLUG_H

#include "list.h"

extern _X_EXPORT void config_pre_init(void);
extern _X_EXPORT void config_init(void);
extern _X_EXPORT void config_fini(void);

/* Bump this each time you add something to the struct
 * so that drivers can easily tell what is available
 */
#define ODEV_ATTRIBUTES_VERSION         1

struct OdevAttributes {
    /* path to kernel device node - Linux e.g. /dev/dri/card0 */
    char        *path;

    /* system device path - Linux e.g. /sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/drm/card1 */
    char        *syspath;

    /* DRI-style bus id */
    char        *busid;

    /* Server managed FD */
    int         fd;

    /* Major number of the device node pointed to by ODEV_ATTRIB_PATH */
    int         major;

    /* Minor number of the device node pointed to by ODEV_ATTRIB_PATH */
    int         minor;

    /* kernel driver name */
    char        *driver;
};

/* Note starting with xserver 1.16 this function never fails */
struct OdevAttributes *
config_odev_allocate_attributes(void);

void
config_odev_free_attributes(struct OdevAttributes *attribs);

typedef void (*config_odev_probe_proc_ptr)(struct OdevAttributes *attribs);
void config_odev_probe(config_odev_probe_proc_ptr probe_callback);

#ifdef CONFIG_UDEV_KMS
void NewGPUDeviceRequest(struct OdevAttributes *attribs);
void DeleteGPUDeviceRequest(struct OdevAttributes *attribs);
#endif

#define ServerIsNotSeat0() (SeatId && strcmp(SeatId, "seat0"))

struct xf86_platform_device *
xf86_find_platform_device_by_devnum(int major, int minor);

#endif                          /* HOTPLUG_H */
