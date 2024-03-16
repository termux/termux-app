/*
 * Copyright © 2009 Red Hat, Inc.
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

#ifndef QUERYDEV_H
#define QUERYDEV_H 1

#include <X11/extensions/XI2proto.h>

int SProcXIQueryDevice(ClientPtr client);
int ProcXIQueryDevice(ClientPtr client);
void SRepXIQueryDevice(ClientPtr client, int size, xXIQueryDeviceReply * rep);
int SizeDeviceClasses(DeviceIntPtr dev);
int ListDeviceClasses(ClientPtr client, DeviceIntPtr dev,
                      char *any, uint16_t * nclasses);
int GetDeviceUse(DeviceIntPtr dev, uint16_t * attachment);
int ListButtonInfo(DeviceIntPtr dev, xXIButtonInfo * info, Bool reportState);
int ListKeyInfo(DeviceIntPtr dev, xXIKeyInfo * info);
int ListValuatorInfo(DeviceIntPtr dev, xXIValuatorInfo * info,
                     int axisnumber, Bool reportState);
int ListScrollInfo(DeviceIntPtr dev, xXIScrollInfo * info, int axisnumber);
int ListTouchInfo(DeviceIntPtr dev, xXITouchInfo * info);
#endif                          /* QUERYDEV_H */
