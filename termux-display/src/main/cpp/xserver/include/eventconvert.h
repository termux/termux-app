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
 */

#ifndef _EVENTCONVERT_H_
#include <X11/X.h>
#include <X11/extensions/XIproto.h>
#include "input.h"
#include "events.h"
#include "eventstr.h"

_X_EXPORT int EventToCore(InternalEvent *event, xEvent **core, int *count);
_X_EXPORT int EventToXI(InternalEvent *ev, xEvent **xi, int *count);
_X_EXPORT int EventToXI2(InternalEvent *ev, xEvent **xi);
_X_INTERNAL int GetCoreType(enum EventType type);
_X_INTERNAL int GetXIType(enum EventType type);
_X_INTERNAL int GetXI2Type(enum EventType type);

_X_INTERNAL enum EventType GestureTypeToBegin(enum EventType type);
_X_INTERNAL enum EventType GestureTypeToEnd(enum EventType type);

#endif                          /* _EVENTCONVERT_H_ */
