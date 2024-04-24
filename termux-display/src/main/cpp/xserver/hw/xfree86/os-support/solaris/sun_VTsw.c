/*
 * Copyright (c) 2009, Oracle and/or its affiliates. All rights reserved.
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

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include <door.h>
#include <sys/vtdaemon.h>

/*
 * Handle the VT-switching interface for Solaris/OpenSolaris
 */

static int xf86VTPruneDoor = 0;

void
xf86VTRelease(int sig)
{
    if (xf86Info.vtPendingNum == -1) {
        xf86VTPruneDoor = 1;
        xf86Info.vtRequestsPending = TRUE;
        return;
    }

    ioctl(xf86Info.consoleFd, VT_RELDISP, 1);
    xf86Info.vtPendingNum = -1;

    return;
}

void
xf86VTAcquire(int sig)
{
    xf86Info.vtRequestsPending = TRUE;
    return;
}

Bool
xf86VTSwitchPending(void)
{
    return xf86Info.vtRequestsPending ? TRUE : FALSE;
}

Bool
xf86VTSwitchAway(void)
{
    int door_fd;
    vt_cmd_arg_t vt_door_arg;
    door_arg_t door_arg;

    xf86Info.vtRequestsPending = FALSE;

    if (xf86VTPruneDoor) {
        xf86VTPruneDoor = 0;
        ioctl(xf86Info.consoleFd, VT_RELDISP, 1);
        return TRUE;
    }

    vt_door_arg.vt_ev = VT_EV_HOTKEYS;
    vt_door_arg.vt_num = xf86Info.vtPendingNum;
    door_arg.data_ptr = (char *) &vt_door_arg;
    door_arg.data_size = sizeof(vt_cmd_arg_t);
    door_arg.rbuf = NULL;
    door_arg.rsize = 0;
    door_arg.desc_ptr = NULL;
    door_arg.desc_num = 0;

    if ((door_fd = open(VT_DAEMON_DOOR_FILE, O_RDONLY)) < 0)
        return FALSE;

    if (door_call(door_fd, &door_arg) != 0) {
        close(door_fd);
        return FALSE;
    }

    close(door_fd);
    return TRUE;
}

Bool
xf86VTSwitchTo(void)
{
    xf86Info.vtRequestsPending = FALSE;
    if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

Bool
xf86VTActivate(int vtno)
{
    struct vt_stat state;

    if (ioctl(xf86Info.consoleFd, VT_GETSTATE, &state) < 0)
        return FALSE;

    if ((state.v_state & (1 << vtno)) == 0)
        return FALSE;

    xf86Info.vtRequestsPending = TRUE;
    xf86Info.vtPendingNum = vtno;

    return TRUE;
}
