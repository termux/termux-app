/* $XFree86: xc/lib/GL/dri/xf86dri.h,v 1.7 2000/12/07 20:26:02 dawes Exp $ */
/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright 2000 VA Linux Systems, Inc.
   Copyright (c) 2002-2012 Apple Computer, Inc.
   All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sub license, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial portions
   of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
   ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 *
 */

#ifndef _APPLEDRI_H_
#define _APPLEDRI_H_

#include <X11/Xfuncproto.h>

#define X_AppleDRIQueryVersion                0
#define X_AppleDRIQueryDirectRenderingCapable 1
#define X_AppleDRICreateSurface               2
#define X_AppleDRIDestroySurface              3
#define X_AppleDRIAuthConnection              4
#define X_AppleDRICreateSharedBuffer          5
#define X_AppleDRISwapBuffers                 6
#define X_AppleDRICreatePixmap                7
#define X_AppleDRIDestroyPixmap               8

/* Requests up to and including 18 were used in a previous version */

/* Events */
#define AppleDRIObsoleteEvent1 0
#define AppleDRIObsoleteEvent2 1
#define AppleDRIObsoleteEvent3 2
#define AppleDRISurfaceNotify  3
#define AppleDRINumberEvents   4

/* Errors */
#define AppleDRIClientNotLocal        0
#define AppleDRIOperationNotSupported 1
#define AppleDRINumberErrors          (AppleDRIOperationNotSupported + 1)

/* Kinds of SurfaceNotify events: */
#define AppleDRISurfaceNotifyChanged   0
#define AppleDRISurfaceNotifyDestroyed 1

#ifndef _APPLEDRI_SERVER_

typedef struct {
    int type;               /* of event */
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;        /* true if this came frome a SendEvent request */
    Display *display;       /* Display the event was read from */
    Window window;          /* window of event */
    Time time;              /* server timestamp when event happened */
    int kind;               /* subtype of event */
    int arg;
} XAppleDRINotifyEvent;

_XFUNCPROTOBEGIN

Bool
XAppleDRIQueryExtension(Display *dpy, int *event_base, int *error_base);

Bool
XAppleDRIQueryVersion(Display *dpy, int *majorVersion, int *minorVersion,
                      int *patchVersion);

Bool
XAppleDRIQueryDirectRenderingCapable(Display *dpy, int screen,
                                     Bool *isCapable);

void *
XAppleDRISetSurfaceNotifyHandler(void (*fun)(Display *dpy, unsigned uid,
                                             int kind));

Bool
XAppleDRIAuthConnection(Display *dpy, int screen, unsigned int magic);

Bool XAppleDRICreateSurface(Display * dpy, int screen, Drawable drawable,
                            unsigned int client_id, unsigned int key[2],
                            unsigned int* uid);

Bool
XAppleDRIDestroySurface(Display *dpy, int screen, Drawable drawable);

Bool
XAppleDRISynchronizeSurfaces(Display *dpy);

Bool
XAppleDRICreateSharedBuffer(Display *dpy, int screen, Drawable drawable,
                            Bool doubleSwap, char *path, size_t pathlen,
                            int *width,
                            int *height);

Bool
XAppleDRISwapBuffers(Display *dpy, int screen, Drawable drawable);

Bool
XAppleDRICreatePixmap(Display *dpy, int screen, Drawable drawable, int *width,
                      int *height, int *pitch, int *bpp, size_t *size,
                      char *bufname,
                      size_t bufnamesize);

Bool
XAppleDRIDestroyPixmap(Display *dpy, Pixmap pixmap);

_XFUNCPROTOEND

#endif /* _APPLEDRI_SERVER_ */
#endif /* _APPLEDRI_H_ */
