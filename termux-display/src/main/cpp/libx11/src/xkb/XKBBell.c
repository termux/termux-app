/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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
#include <stdio.h>
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"


Bool
XkbDeviceBell(Display *dpy,
              Window window,
              int deviceID,
              int bellClass,
              int bellID,
              int percent,
              Atom name)
{
    register xkbBellReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbBell, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbBell;
    req->deviceSpec = deviceID;
    req->window = (CARD32) window;
    req->bellClass = (CARD16) bellClass;
    req->bellID = (CARD16) bellID;
    req->percent = percent;
    req->forceSound = False;
    req->eventOnly = False;
    req->pitch = 0;
    req->duration = 0;
    req->name = (CARD32) name;
    req->pad1 = 0;
    req->pad2 = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbForceDeviceBell(Display *dpy,
                   int deviceID,
                   int bellClass,
                   int bellID,
                   int percent)
{
    register xkbBellReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbBell, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbBell;
    req->deviceSpec = deviceID;
    req->window = (CARD32) None;
    req->bellClass = (CARD16) bellClass;
    req->bellID = (CARD16) bellID;
    req->percent = percent;
    req->forceSound = True;
    req->eventOnly = False;
    req->pitch = 0;
    req->duration = 0;
    req->name = None;
    req->pad1 = 0;
    req->pad2 = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbDeviceBellEvent(Display *dpy,
                   Window window,
                   int deviceID,
                   int bellClass,
                   int bellID,
                   int percent,
                   Atom name)
{
    register xkbBellReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbBell, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbBell;
    req->deviceSpec = deviceID;
    req->window = (CARD32) window;
    req->bellClass = (CARD16) bellClass;
    req->bellID = (CARD16) bellID;
    req->percent = percent;
    req->forceSound = False;
    req->eventOnly = True;
    req->pitch = 0;
    req->duration = 0;
    req->name = (CARD32) name;
    req->pad1 = 0;
    req->pad2 = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbBell(Display *dpy, Window window, int percent, Atom name)
{
    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL))) {
        XBell(dpy, percent);
        return False;
    }
    return XkbDeviceBell(dpy, window, XkbUseCoreKbd,
                         XkbDfltXIClass, XkbDfltXIId, percent, name);
}

Bool
XkbForceBell(Display *dpy, int percent)
{
    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL))) {
        XBell(dpy, percent);
        return False;
    }
    return XkbForceDeviceBell(dpy, XkbUseCoreKbd, XkbDfltXIClass, XkbDfltXIId,
                              percent);
}

Bool
XkbBellEvent(Display *dpy, Window window, int percent, Atom name)
{
    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL))) {
        return False;
    }
    /* class 0 = KbdFeedbackClass (X Input Extension) */
    return XkbDeviceBellEvent(dpy, window, XkbUseCoreKbd,
                              XkbDfltXIClass, XkbDfltXIId, percent, name);
}
