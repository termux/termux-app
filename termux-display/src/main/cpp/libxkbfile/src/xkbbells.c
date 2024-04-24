/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include "XKBbells.h"

static const char *_xkbStdBellNames[XkbBI_NumBells] = {
    "Info",
    "Warning",
    "MinorError",
    "MajorError",
    "BadValue",
    "InvalidLocation",
    "Question",
    "Start",
    "End",
    "Success",
    "Failure",
    "Wait",
    "Proceed",
    "Ignore",
    "Iconify",
    "Deconify",
    "Open",
    "Close",
    "TerminalBell",
    "MarginBell",
    "CursorStuck",
    "NewMail",
    "LaunchApp",
    "AppDeath",
    "ImAlive",
    "ClockChimeHour",
    "ClockChimeHalf",
    "ClockChimeQuarter",
    "RepeatingLastBell",
    "ComposeFail",
    "AX_FeatureOn",
    "AX_FeatureOff",
    "AX_FeatureChange",
    "AX_IndicatorOn",
    "AX_IndicatorOff",
    "AX_IndicatorChange",
    "AX_SlowKeysWarning",
    "AX_SlowKeyPress",
    "AX_SlowKeyAccept",
    "AX_SlowKeyReject",
    "AX_SlowKeyRelease",
    "AX_BounceKeyReject",
    "AX_StickyLatch",
    "AX_StickyLock",
    "AX_StickyUnlock"
};

static Atom _xkbStdBellAtoms[XkbBI_NumBells];

Bool
XkbStdBell(Display *dpy, Window win, int percent, int bellDef)
{
    if ((bellDef < 0) || (bellDef >= XkbBI_NumBells))
        bellDef = XkbBI_Info;
    if (_xkbStdBellAtoms[bellDef] == None)
        _xkbStdBellAtoms[bellDef] =
            XInternAtom(dpy, _xkbStdBellNames[bellDef], 0);
    return XkbBell(dpy, win, percent, _xkbStdBellAtoms[bellDef]);
}

Bool
XkbStdBellEvent(Display *dpy, Window win, int percent, int bellDef)
{
    if ((bellDef < 0) || (bellDef >= XkbBI_NumBells))
        bellDef = XkbBI_Info;
    if (_xkbStdBellAtoms[bellDef] == None)
        _xkbStdBellAtoms[bellDef] =
            XInternAtom(dpy, _xkbStdBellNames[bellDef], 0);
    return XkbBellEvent(dpy, win, percent, _xkbStdBellAtoms[bellDef]);
}
