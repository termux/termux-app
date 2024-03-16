
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
 * This file contains definitions of the private XFree86 data structures/types.
 * None of the data structures here should be used by video drivers.
 */

#ifndef _XF86PRIVSTR_H
#define _XF86PRIVSTR_H

#include "xf86str.h"

typedef enum {
    XF86_GlxVisualsMinimal,
    XF86_GlxVisualsTypical,
    XF86_GlxVisualsAll,
} XF86_GlxVisuals;

/*
 * xf86InfoRec contains global parameters which the video drivers never
 * need to access.  Global parameters which the video drivers do need
 * should be individual globals.
 */

typedef struct {
    int consoleFd;
    int vtno;

    /* event handler part */
    int lastEventTime;
    Bool vtRequestsPending;
#ifdef __sun
    int vtPendingNum;
#endif
    Bool dontVTSwitch;
    Bool autoVTSwitch;
    Bool ShareVTs;
    Bool dontZap;
    Bool dontZoom;

    /* graphics part */
    ScreenPtr currentScreen;
#if defined(CSRG_BASED) || defined(__FreeBSD_kernel__)
    int consType;               /* Which console driver? */
#endif

    /* Other things */
    Bool allowMouseOpenFail;
    Bool vidModeEnabled;        /* VidMode extension enabled */
    Bool vidModeAllowNonLocal;  /* allow non-local VidMode
                                 * connections */
    Bool miscModInDevEnabled;   /* Allow input devices to be
                                 * changed */
    Bool miscModInDevAllowNonLocal;
    Bool pmFlag;
    MessageType iglxFrom;
    XF86_GlxVisuals glxVisuals;
    MessageType glxVisualsFrom;

    Bool useDefaultFontPath;
    Bool ignoreABI;

    Bool forceInputDevices;     /* force xorg.conf or built-in input devices */
    Bool autoAddDevices;        /* Whether to succeed NIDR, or ignore. */
    Bool autoEnableDevices;     /* Whether to enable, or let the client
                                 * control. */

    Bool dri2;
    MessageType dri2From;

    Bool autoAddGPU;
    const char *debug;
    Bool autoBindGPU;
} xf86InfoRec, *xf86InfoPtr;

/* ISC's cc can't handle ~ of UL constants, so explicitly type cast them. */
#define XLED1   ((unsigned long) 0x00000001)
#define XLED2   ((unsigned long) 0x00000002)
#define XLED3   ((unsigned long) 0x00000004)
#define XLED4	((unsigned long) 0x00000008)
#define XCAPS   ((unsigned long) 0x20000000)
#define XNUM    ((unsigned long) 0x40000000)
#define XSCR    ((unsigned long) 0x80000000)
#define XCOMP	((unsigned long) 0x00008000)

/* BSD console driver types (consType) */
#if defined(CSRG_BASED) || defined(__FreeBSD_kernel__)
#define PCCONS		   0
#define CODRV011	   1
#define CODRV01X	   2
#define SYSCONS		   8
#define PCVT		  16
#define WSCONS		  32
#endif

/* Root window property to tell clients whether our VT is currently active.
 * Name chosen to match the "XFree86_VT" property. */
#define HAS_VT_ATOM_NAME "XFree86_has_VT"

#endif                          /* _XF86PRIVSTR_H */
