/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#ifndef WINCLIPBOARD_INTERNAL_H
#define WINCLIPBOARD_INTERNAL_H

#include <xcb/xproto.h>
#include <X11/Xfuncproto.h> // for _X_ATTRIBUTE_PRINTF
#include <X11/Xmd.h> // for BOOL

/* Windows headers */
#include <X11/Xwindows.h>

#define WIN_XEVENTS_SUCCESS			0  // more like 'CONTINUE'
#define WIN_XEVENTS_FAILED			1
#define WIN_XEVENTS_NOTIFY_DATA			3
#define WIN_XEVENTS_NOTIFY_TARGETS		4

#define WM_WM_QUIT                             (WM_USER + 2)

#define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))

/*
 * References to external symbols
 */

extern void winDebug(const char *format, ...) _X_ATTRIBUTE_PRINTF(1, 2);
extern void ErrorF(const char *format, ...) _X_ATTRIBUTE_PRINTF(1, 2);

/*
 * winclipboardtextconv.c
 */

void
 winClipboardDOStoUNIX(char *pszData, int iLength);

void
 winClipboardUNIXtoDOS(char **ppszData, int iLength);

/*
 * winclipboardthread.c
 */

typedef struct
{
    xcb_atom_t atomClipboard;
    xcb_atom_t atomLocalProperty;
    xcb_atom_t atomUTF8String;
    xcb_atom_t atomCompoundText;
    xcb_atom_t atomTargets;
    xcb_atom_t atomIncr;
} ClipboardAtoms;

/*
 * winclipboardwndproc.c
 */

BOOL winClipboardFlushWindowsMessageQueue(HWND hwnd);

LRESULT CALLBACK
winClipboardWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

typedef struct
{
  xcb_connection_t *pClipboardDisplay;
  xcb_window_t iClipboardWindow;
  ClipboardAtoms *atoms;
} ClipboardWindowCreationParams;

/*
 * winclipboardxevents.c
 */

typedef struct
{
  xcb_atom_t *targetList;
  unsigned char *incr;
  unsigned long int incrsize;
} ClipboardConversionData;

int
winClipboardFlushXEvents(HWND hwnd,
                         xcb_window_t iWindow, xcb_connection_t * pDisplay,
                         ClipboardConversionData *data, ClipboardAtoms *atoms);

xcb_atom_t
winClipboardGetLastOwnedSelectionAtom(ClipboardAtoms *atoms);

void
winClipboardInitMonitoredSelections(void);

#endif
