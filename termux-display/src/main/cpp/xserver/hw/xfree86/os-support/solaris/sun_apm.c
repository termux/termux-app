/* Based on hw/xfree86/os-support/bsd/bsd_apm.c which bore no explicit
 * copyright notice, so is covered by the following notice:
 *
 * Copyright (C) 1994-2003 The XFree86 Project, Inc.  All Rights Reserved.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/* Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
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
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "xf86_OSlib.h"

#ifndef PLEASE_FIX_THIS
#define APM_STANDBY_REQ 0xa01
#define APM_SUSPEND_REQ 0xa02
#define APM_NORMAL_RESUME 0xa03
#define APM_CRIT_RESUME 0xa04
#define APM_BATTERY_LOW 0xa05
#define APM_POWER_CHANGE 0xa06
#define APM_UPDATE_TIME 0xa07
#define APM_CRIT_SUSPEND_REQ 0xa08
#define APM_USER_STANDBY_REQ 0xa09
#define APM_USER_SUSPEND_REQ 0xa0a
#define APM_SYS_STANDBY_RESUME 0xa0b
#define APM_IOC_NEXTEVENT 0xa0c
#define APM_IOC_RESUME 0xa0d
#define APM_IOC_SUSPEND 0xa0e
#define APM_IOC_STANDBY 0xa0f
#endif

typedef struct apm_event_info {
    int type;
} apm_event_info;

/*
 This may be replaced with a better device name
 very soon...
*/
#define APM_DEVICE "/dev/srn"
#define APM_DEVICE1 "/dev/apm"

static void *APMihPtr = NULL;
static void sunCloseAPM(void);

static struct {
    u_int apmBsd;
    pmEvent xf86;
} sunToXF86Array[] = {
    {APM_STANDBY_REQ, XF86_APM_SYS_STANDBY},
    {APM_SUSPEND_REQ, XF86_APM_SYS_SUSPEND},
    {APM_NORMAL_RESUME, XF86_APM_NORMAL_RESUME},
    {APM_CRIT_RESUME, XF86_APM_CRITICAL_RESUME},
    {APM_BATTERY_LOW, XF86_APM_LOW_BATTERY},
    {APM_POWER_CHANGE, XF86_APM_POWER_STATUS_CHANGE},
    {APM_UPDATE_TIME, XF86_APM_UPDATE_TIME},
    {APM_CRIT_SUSPEND_REQ, XF86_APM_CRITICAL_SUSPEND},
    {APM_USER_STANDBY_REQ, XF86_APM_USER_STANDBY},
    {APM_USER_SUSPEND_REQ, XF86_APM_USER_SUSPEND},
    {APM_SYS_STANDBY_RESUME, XF86_APM_STANDBY_RESUME},
#ifdef APM_CAPABILITY_CHANGE
    {APM_CAPABILITY_CHANGE, XF86_APM_CAPABILITY_CHANGED},
#endif
};

static pmEvent
sunToXF86(int type)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(sunToXF86Array); i++) {
        if (type == sunToXF86Array[i].apmBsd) {
            return sunToXF86Array[i].xf86;
        }
    }
    return XF86_APM_UNKNOWN;
}

/*
 * APM events can be requested directly from /dev/apm
 */
static int
sunPMGetEventFromOS(int fd, pmEvent * events, int num)
{
    struct apm_event_info sunEvent;
    int i;

    for (i = 0; i < num; i++) {

        if (ioctl(fd, APM_IOC_NEXTEVENT, &sunEvent) < 0) {
            if (errno != EAGAIN) {
                xf86Msg(X_WARNING, "sunPMGetEventFromOS: APM_IOC_NEXTEVENT"
                        " %s\n", strerror(errno));
            }
            break;
        }
        events[i] = sunToXF86(sunEvent.type);
    }
    xf86Msg(X_WARNING, "Got some events\n");
    return i;
}

static pmWait
sunPMConfirmEventToOs(int fd, pmEvent event)
{
    switch (event) {
/* XXX: NOT CURRENTLY RETURNED FROM OS */
    case XF86_APM_SYS_STANDBY:
    case XF86_APM_USER_STANDBY:
        if (ioctl(fd, APM_IOC_STANDBY, NULL) == 0)
            return PM_WAIT;     /* should we stop the Xserver in standby, too? */
        else
            return PM_NONE;
    case XF86_APM_SYS_SUSPEND:
    case XF86_APM_CRITICAL_SUSPEND:
    case XF86_APM_USER_SUSPEND:
        xf86Msg(X_WARNING, "Got SUSPENDED\n");
        if (ioctl(fd, APM_IOC_SUSPEND, NULL) == 0)
            return PM_CONTINUE;
        else {
            xf86Msg(X_WARNING, "sunPMConfirmEventToOs: APM_IOC_SUSPEND"
                    " %s\n", strerror(errno));
            return PM_FAILED;
        }
    case XF86_APM_STANDBY_RESUME:
    case XF86_APM_NORMAL_RESUME:
    case XF86_APM_CRITICAL_RESUME:
    case XF86_APM_STANDBY_FAILED:
    case XF86_APM_SUSPEND_FAILED:
        xf86Msg(X_WARNING, "Got RESUME\n");
        if (ioctl(fd, APM_IOC_RESUME, NULL) == 0)
            return PM_CONTINUE;
        else {
            xf86Msg(X_WARNING, "sunPMConfirmEventToOs: APM_IOC_RESUME"
                    " %s\n", strerror(errno));
            return PM_FAILED;
        }
    default:
        return PM_NONE;
    }
}

PMClose
xf86OSPMOpen(void)
{
    int fd;

    if (APMihPtr || !xf86Info.pmFlag) {
        return NULL;
    }

    if ((fd = open(APM_DEVICE, O_RDWR)) == -1) {
        if ((fd = open(APM_DEVICE1, O_RDWR)) == -1) {
            return NULL;
        }
    }
    xf86PMGetEventFromOs = sunPMGetEventFromOS;
    xf86PMConfirmEventToOs = sunPMConfirmEventToOs;
    APMihPtr = xf86AddGeneralHandler(fd, xf86HandlePMEvents, NULL);
    return sunCloseAPM;
}

static void
sunCloseAPM(void)
{
    int fd;

    if (APMihPtr) {
        fd = xf86RemoveGeneralHandler(APMihPtr);
        close(fd);
        APMihPtr = NULL;
    }
}
