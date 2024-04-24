/*
 * Copyright © 2012 Red Hat.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Dave Airlie <airlied@redhat.com>
 */
#ifndef XF86_PLATFORM_BUS_H
#define XF86_PLATFORM_BUS_H

#include "hotplug.h"
#include "xf86MatchDrivers.h"

struct xf86_platform_device {
    struct OdevAttributes *attribs;
    /* for PCI devices */
    struct pci_device *pdev;
    int flags;
};

/* xf86_platform_device flags */
#define XF86_PDEV_UNOWNED       0x01
#define XF86_PDEV_SERVER_FD     0x02
#define XF86_PDEV_PAUSED        0x04

#ifdef XSERVER_PLATFORM_BUS
int xf86platformProbe(void);
int xf86platformProbeDev(DriverPtr drvp);
int xf86platformAddGPUDevices(DriverPtr drvp);
void xf86MergeOutputClassOptions(int entityIndex, void **options);

extern int xf86_num_platform_devices;
extern struct xf86_platform_device *xf86_platform_devices;

extern int
xf86_add_platform_device(struct OdevAttributes *attribs, Bool unowned);
extern int
xf86_remove_platform_device(int dev_index);
extern Bool
xf86_get_platform_device_unowned(int index);

extern int
xf86platformAddDevice(int index);
extern void
xf86platformRemoveDevice(int index);

static inline struct OdevAttributes *
xf86_platform_device_odev_attributes(struct xf86_platform_device *device)
{
    return device->attribs;
}

static inline struct OdevAttributes *
xf86_platform_odev_attributes(int index)
{
    struct xf86_platform_device *device = &xf86_platform_devices[index];

    return device->attribs;
}

#ifndef _XORG_CONFIG_H_
/*
 * Define the legacy API only for external builds
 */

/* path to kernel device node - Linux e.g. /dev/dri/card0 */
#define ODEV_ATTRIB_PATH        1
/* system device path - Linux e.g. /sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/drm/card1 */
#define ODEV_ATTRIB_SYSPATH     2
/* DRI-style bus id */
#define ODEV_ATTRIB_BUSID       3
/* Server managed FD */
#define ODEV_ATTRIB_FD          4
/* Major number of the device node pointed to by ODEV_ATTRIB_PATH */
#define ODEV_ATTRIB_MAJOR       5
/* Minor number of the device node pointed to by ODEV_ATTRIB_PATH */
#define ODEV_ATTRIB_MINOR       6
/* kernel driver name */
#define ODEV_ATTRIB_DRIVER      7

/* Protect against a mismatch attribute type by generating a compiler
 * error using a negative array size when an incorrect attribute is
 * passed
 */

#define _ODEV_ATTRIB_IS_STRING(x)       ((x) == ODEV_ATTRIB_PATH ||     \
                                         (x) == ODEV_ATTRIB_SYSPATH ||  \
                                         (x) == ODEV_ATTRIB_BUSID ||    \
                                         (x) == ODEV_ATTRIB_DRIVER)

#define _ODEV_ATTRIB_STRING_CHECK(x)    ((int (*)[_ODEV_ATTRIB_IS_STRING(x)-1]) 0)

static inline char *
_xf86_get_platform_device_attrib(struct xf86_platform_device *device, int attrib, int (*fake)[0])
{
    switch (attrib) {
    case ODEV_ATTRIB_PATH:
        return xf86_platform_device_odev_attributes(device)->path;
    case ODEV_ATTRIB_SYSPATH:
        return xf86_platform_device_odev_attributes(device)->syspath;
    case ODEV_ATTRIB_BUSID:
        return xf86_platform_device_odev_attributes(device)->busid;
    case ODEV_ATTRIB_DRIVER:
        return xf86_platform_device_odev_attributes(device)->driver;
    default:
        assert(FALSE);
        return NULL;
    }
}

#define xf86_get_platform_device_attrib(device, attrib) _xf86_get_platform_device_attrib(device,attrib,_ODEV_ATTRIB_STRING_CHECK(attrib))

#define _ODEV_ATTRIB_IS_INT(x)                  ((x) == ODEV_ATTRIB_FD || (x) == ODEV_ATTRIB_MAJOR || (x) == ODEV_ATTRIB_MINOR)
#define _ODEV_ATTRIB_INT_DEFAULT(x)             ((x) == ODEV_ATTRIB_FD ? -1 : 0)
#define _ODEV_ATTRIB_DEFAULT_CHECK(x,def)       (_ODEV_ATTRIB_INT_DEFAULT(x) == (def))
#define _ODEV_ATTRIB_INT_CHECK(x,def)           ((int (*)[_ODEV_ATTRIB_IS_INT(x)*_ODEV_ATTRIB_DEFAULT_CHECK(x,def)-1]) 0)

static inline int
_xf86_get_platform_device_int_attrib(struct xf86_platform_device *device, int attrib, int (*fake)[0])
{
    switch (attrib) {
    case ODEV_ATTRIB_FD:
        return xf86_platform_device_odev_attributes(device)->fd;
    case ODEV_ATTRIB_MAJOR:
        return xf86_platform_device_odev_attributes(device)->major;
    case ODEV_ATTRIB_MINOR:
        return xf86_platform_device_odev_attributes(device)->minor;
    default:
        assert(FALSE);
        return 0;
    }
}

#define xf86_get_platform_device_int_attrib(device, attrib, def) _xf86_get_platform_device_int_attrib(device,attrib,_ODEV_ATTRIB_INT_CHECK(attrib,def))

#endif

extern _X_EXPORT Bool
xf86PlatformDeviceCheckBusID(struct xf86_platform_device *device, const char *busid);

extern _X_EXPORT void
xf86PlatformMatchDriver(XF86MatchedDrivers *);

extern void xf86platformVTProbe(void);
extern void xf86platformPrimary(void);

#else

static inline int xf86platformAddGPUDevices(DriverPtr drvp) { return FALSE; }
static inline void xf86MergeOutputClassOptions(int index, void **options) {}

#endif

#endif
