/*
 * Copyright (c) 2008 Apple, Inc.
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
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

#ifndef _DARWIN_EVENTS_H
#define _DARWIN_EVENTS_H

/* For extra precision of our cursor and other valuators */
#define XQUARTZ_VALUATOR_LIMIT (1 << 16)

Bool
DarwinEQInit(void);
void
DarwinEQFini(void);
void
DarwinEQEnqueue(const xEventPtr e);
void
DarwinEQPointerPost(DeviceIntPtr pDev, xEventPtr e);
void
DarwinEQSwitchScreen(ScreenPtr pScreen, Bool fromDIX);
void
DarwinInputReleaseButtonsAndKeys(DeviceIntPtr pDev);
void
DarwinSendTabletEvents(DeviceIntPtr pDev, int ev_type, int ev_button,
                       double pointer_x, double pointer_y, double pressure,
                       double tilt_x, double tilt_y);
void
DarwinSendPointerEvents(DeviceIntPtr pDev, int ev_type, int ev_button,
                        double pointer_x, double pointer_y,
                        double pointer_dx, double pointer_dy);
void
DarwinSendKeyboardEvents(int ev_type, int keycode);
void
DarwinSendScrollEvents(double scroll_x, double scroll_y);
void
DarwinUpdateModKeys(int flags);
void
DarwinListenOnOpenFD(int fd);

/*
 * Subtypes for the ET_XQuartz event type
 */
enum {
    kXquartzReloadKeymap,     // Reload system keymap
    kXquartzActivate,         // restore X drawing and cursor
    kXquartzDeactivate,       // clip X drawing and switch to Aqua cursor
    kXquartzSetRootClip,      // enable or disable drawing to the X screen
    kXquartzQuit,             // kill the X server and release the display
    kXquartzBringAllToFront,  // bring all X windows to front
    kXquartzToggleFullscreen, // Enable/Disable fullscreen mode
    kXquartzSetRootless,      // Set rootless mode
    kXquartzSpaceChanged,     // Spaces changed
    kXquartzListenOnOpenFD,   // Listen to the launchd fd (passed as arg)
    /*
     * AppleWM events
     */
    kXquartzControllerNotify, // send an AppleWMControllerNotify event
    kXquartzPasteboardNotify, // notify the WM to copy or paste
    kXquartzReloadPreferences, // send AppleWMReloadPreferences
    /*
     * Xplugin notification events
     */
    kXquartzDisplayChanged,   // display configuration has changed
    kXquartzWindowState,      // window visibility state has changed
    kXquartzWindowMoved,      // window has moved on screen
};

/* Send one of the above events to the server thread. */
void
DarwinSendDDXEvent(int type, int argc, ...);

/* A mask of the modifiers that are in our X11 keyboard layout:
 * (Fn for example is just useful for 3button mouse emulation) */
extern int darwin_all_modifier_mask;

/* A mask of the modifiers that are in our X11 keyboard layout:
 * (Fn for example is just useful for 3button mouse emulation) */
extern int darwin_x11_modifier_mask;

/* The current state of the above listed modifiers */
extern int darwin_all_modifier_flags;

#endif  /* _DARWIN_EVENTS_H */
