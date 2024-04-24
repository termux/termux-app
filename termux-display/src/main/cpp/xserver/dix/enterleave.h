/*
 * Copyright © 2008 Red Hat, Inc.
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
 * Authors: Peter Hutterer
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef ENTERLEAVE_H
#define ENTERLEAVE_H

#include <dix.h> /* DoFocusEvents() */

extern void DoEnterLeaveEvents(DeviceIntPtr pDev,
                               int sourceid,
                               WindowPtr fromWin, WindowPtr toWin, int mode);

extern void EnterLeaveEvent(DeviceIntPtr mouse,
                            int type,
                            int mode, int detail, WindowPtr pWin, Window child);

extern void CoreEnterLeaveEvent(DeviceIntPtr mouse,
                                int type,
                                int mode,
                                int detail, WindowPtr pWin, Window child);
extern void DeviceEnterLeaveEvent(DeviceIntPtr mouse,
                                  int sourceid,
                                  int type,
                                  int mode,
                                  int detail, WindowPtr pWin, Window child);
extern void DeviceFocusEvent(DeviceIntPtr dev,
                             int type,
                             int mode,
                             int detail ,
                             WindowPtr pWin);

extern void EnterWindow(DeviceIntPtr dev, WindowPtr win, int mode);

extern void LeaveWindow(DeviceIntPtr dev);

extern void CoreFocusEvent(DeviceIntPtr kbd,
                           int type, int mode, int detail, WindowPtr pWin);

extern void SetFocusIn(DeviceIntPtr kbd, WindowPtr win);

extern void SetFocusOut(DeviceIntPtr dev);
#endif                          /* _ENTERLEAVE_H_ */
