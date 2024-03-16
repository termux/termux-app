/*
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 * Copyright (c) 2003-2004 Torrey T. Lyons. All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef QUARTZ_KEYBOARD_H
#define QUARTZ_KEYBOARD_H 1

#define XK_TECHNICAL      // needed to get XK_Escape
#define XK_PUBLISHING
#include "X11/keysym.h"
#include "inputstr.h"

// Each key can generate 4 glyphs. They are, in order:
// unshifted, shifted, modeswitch unshifted, modeswitch shifted
#define GLYPHS_PER_KEY 4
#define NUM_KEYCODES   248      // NX_NUMKEYCODES might be better
#define MIN_KEYCODE    XkbMinLegalKeyCode      // unfortunately, this isn't 0...
#define MAX_KEYCODE    NUM_KEYCODES + MIN_KEYCODE - 1

/* These functions need to be implemented by Xquartz, XDarwin, etc. */
Bool
QuartsResyncKeymap(Bool sendDDXEvent);

/* Provided for darwinEvents.c */
void
DarwinKeyboardReloadHandler(void);
int
DarwinModifierNXKeycodeToNXKey(unsigned char keycode, int *outSide);
int
DarwinModifierNXKeyToNXKeycode(int key, int side);
int
DarwinModifierNXKeyToNXMask(int key);
int
DarwinModifierNXMaskToNXKey(int mask);
int
DarwinModifierStringToNXMask(const char *string, int separatelr);

/* Provided for darwin.c */
void
DarwinKeyboardInit(DeviceIntPtr pDev);

#endif /* QUARTZ_KEYBOARD_H */
