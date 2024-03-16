/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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
 * This file contains definitions of the bus-related data structures/types.
 * Everything contained here is private to xf86Bus.c.  In particular the
 * video drivers must not include this file.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _XF86_BUS_H
#define _XF86_BUS_H

#include "xf86pciBus.h"
#if defined(__sparc__) || defined(__sparc)
#include "xf86sbusBus.h"
#endif
#include "xf86platformBus.h"

typedef struct {
    DriverPtr driver;
    int chipset;
    int entityProp;
    Bool active;
    Bool inUse;
    BusRec bus;
    DevUnion *entityPrivates;
    int numInstances;
    GDevPtr *devices;
} EntityRec, *EntityPtr;

#define ACCEL_IS_SHARABLE 0x100
#define IS_SHARED_ACCEL 0x200
#define SA_PRIM_INIT_DONE 0x400

extern EntityPtr *xf86Entities;
extern int xf86NumEntities;
extern BusRec primaryBus;

int xf86AllocateEntity(void);
BusType StringToBusType(const char *busID, const char **retID);

#endif                          /* _XF86_BUS_H */
