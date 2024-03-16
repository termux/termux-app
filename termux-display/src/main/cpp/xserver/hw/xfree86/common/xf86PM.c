/*
 * Copyright (c) 2000-2002 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Xinput.h"
#include "xf86_OSproc.h"

int (*xf86PMGetEventFromOs) (int fd, pmEvent * events, int num) = NULL;
pmWait (*xf86PMConfirmEventToOs) (int fd, pmEvent event) = NULL;

static Bool suspended = FALSE;

static int
eventName(pmEvent event, const char **str)
{
    switch (event) {
    case XF86_APM_SYS_STANDBY:
        *str = "System Standby Request";
        return 0;
    case XF86_APM_SYS_SUSPEND:
        *str = "System Suspend Request";
        return 0;
    case XF86_APM_CRITICAL_SUSPEND:
        *str = "Critical Suspend";
        return 0;
    case XF86_APM_USER_STANDBY:
        *str = "User System Standby Request";
        return 0;
    case XF86_APM_USER_SUSPEND:
        *str = "User System Suspend Request";
        return 0;
    case XF86_APM_STANDBY_RESUME:
        *str = "System Standby Resume";
        return 0;
    case XF86_APM_NORMAL_RESUME:
        *str = "Normal Resume System";
        return 0;
    case XF86_APM_CRITICAL_RESUME:
        *str = "Critical Resume System";
        return 0;
    case XF86_APM_LOW_BATTERY:
        *str = "Battery Low";
        return 3;
    case XF86_APM_POWER_STATUS_CHANGE:
        *str = "Power Status Change";
        return 3;
    case XF86_APM_UPDATE_TIME:
        *str = "Update Time";
        return 3;
    case XF86_APM_CAPABILITY_CHANGED:
        *str = "Capability Changed";
        return 3;
    case XF86_APM_STANDBY_FAILED:
        *str = "Standby Request Failed";
        return 0;
    case XF86_APM_SUSPEND_FAILED:
        *str = "Suspend Request Failed";
        return 0;
    default:
        *str = "Unknown Event";
        return 0;
    }
}

static void
suspend(pmEvent event, Bool undo)
{
    int i;
    InputInfoPtr pInfo;

    for (i = 0; i < xf86NumScreens; i++) {
        if (xf86Screens[i]->EnableDisableFBAccess)
            (*xf86Screens[i]->EnableDisableFBAccess) (xf86Screens[i], FALSE);
    }
    pInfo = xf86InputDevs;
    while (pInfo) {
        DisableDevice(pInfo->dev, TRUE);
        pInfo = pInfo->next;
    }
    input_lock();
    for (i = 0; i < xf86NumScreens; i++) {
        if (xf86Screens[i]->PMEvent)
            xf86Screens[i]->PMEvent(xf86Screens[i], event, undo);
        else {
            xf86Screens[i]->LeaveVT(xf86Screens[i]);
            xf86Screens[i]->vtSema = FALSE;
        }
    }
}

static void
resume(pmEvent event, Bool undo)
{
    int i;
    InputInfoPtr pInfo;

    for (i = 0; i < xf86NumScreens; i++) {
        if (xf86Screens[i]->PMEvent)
            xf86Screens[i]->PMEvent(xf86Screens[i], event, undo);
        else {
            xf86Screens[i]->vtSema = TRUE;
            xf86Screens[i]->EnterVT(xf86Screens[i]);
        }
    }
    input_unlock();
    for (i = 0; i < xf86NumScreens; i++) {
        if (xf86Screens[i]->EnableDisableFBAccess)
            (*xf86Screens[i]->EnableDisableFBAccess) (xf86Screens[i], TRUE);
    }
    dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverReset);
    pInfo = xf86InputDevs;
    while (pInfo) {
        EnableDevice(pInfo->dev, TRUE);
        pInfo = pInfo->next;
    }
}

static void
DoApmEvent(pmEvent event, Bool undo)
{
    int i;

    switch (event) {
#if 0
    case XF86_APM_SYS_STANDBY:
    case XF86_APM_USER_STANDBY:
#endif
    case XF86_APM_SYS_SUSPEND:
    case XF86_APM_CRITICAL_SUSPEND:    /*do we want to delay a critical suspend? */
    case XF86_APM_USER_SUSPEND:
        /* should we do this ? */
        if (!undo && !suspended) {
            suspend(event, undo);
            suspended = TRUE;
        }
        else if (undo && suspended) {
            resume(event, undo);
            suspended = FALSE;
        }
        break;
#if 0
    case XF86_APM_STANDBY_RESUME:
#endif
    case XF86_APM_NORMAL_RESUME:
    case XF86_APM_CRITICAL_RESUME:
        if (suspended) {
            resume(event, undo);
            suspended = FALSE;
        }
        break;
    default:
        input_lock();
        for (i = 0; i < xf86NumScreens; i++) {
            if (xf86Screens[i]->PMEvent) {
                xf86Screens[i]->PMEvent(xf86Screens[i], event, undo);
            }
        }
        input_unlock();
        break;
    }
}

#define MAX_NO_EVENTS 8

void
xf86HandlePMEvents(int fd, void *data)
{
    pmEvent events[MAX_NO_EVENTS];
    int i, n;
    Bool wait = FALSE;

    if (!xf86PMGetEventFromOs)
        return;

    if ((n = xf86PMGetEventFromOs(fd, events, MAX_NO_EVENTS))) {
        do {
            for (i = 0; i < n; i++) {
                const char *str = NULL;
                int verb = eventName(events[i], &str);

                xf86MsgVerb(X_INFO, verb, "PM Event received: %s\n", str);
                DoApmEvent(events[i], FALSE);
                switch (xf86PMConfirmEventToOs(fd, events[i])) {
                case PM_WAIT:
                    wait = TRUE;
                    break;
                case PM_CONTINUE:
                    wait = FALSE;
                    break;
                case PM_FAILED:
                    DoApmEvent(events[i], TRUE);
                    wait = FALSE;
                    break;
                default:
                    break;
                }
            }
            if (wait)
                n = xf86PMGetEventFromOs(fd, events, MAX_NO_EVENTS);
            else
                break;
        } while (1);
    }
}
