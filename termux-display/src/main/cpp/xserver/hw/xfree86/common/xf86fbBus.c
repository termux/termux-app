/*
 * Copyright (c) 2000-2001 by The XFree86 Project, Inc.
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

/*
 * This file contains the interfaces to the bus-specific code
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

#include "xf86Bus.h"

#define XF86_OS_PRIVS
#include "xf86_OSproc.h"

Bool fbSlotClaimed = FALSE;

int
xf86ClaimFbSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    int num;

#ifdef XSERVER_PLATFORM_BUS
    if (platformSlotClaimed)
        return -1;
#endif
#ifdef XSERVER_LIBPCIACCESS
    if (pciSlotClaimed)
        return -1;
#endif
#if defined(__sparc__) || defined (__sparc64__)
    if (sbusSlotClaimed)
        return -1;
#endif

    num = xf86AllocateEntity();
    p = xf86Entities[num];
    p->driver = drvp;
    p->chipset = 0;
    p->bus.type = BUS_NONE;
    p->active = active;
    p->inUse = FALSE;
    xf86AddDevToEntity(num, dev);

    fbSlotClaimed = TRUE;
    return num;
}

/*
 * Get the list of FB "slots" claimed by a screen
 */
int
xf86GetFbInfoForScreen(int scrnIndex)
{
    int num = 0;
    int i;
    EntityPtr p;

    for (i = 0; i < xf86Screens[scrnIndex]->numEntities; i++) {
        p = xf86Entities[xf86Screens[scrnIndex]->entityList[i]];
        if (p->bus.type == BUS_NONE) {
            num++;
        }
    }
    return num;
}
