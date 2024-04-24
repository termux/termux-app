/* $XFree86: xc/include/extensions/xf86rush.h,v 1.4 2000/02/29 03:09:00 dawes Exp $ */
/*

Copyright (c) 1998  Daryll Strauss

*/

#ifndef _XF86RUSH_H_
#define _XF86RUSH_H_

#include <X11/extensions/Xv.h>
#include <X11/Xfuncproto.h>

#define X_XF86RushQueryVersion		0
#define X_XF86RushLockPixmap		1
#define X_XF86RushUnlockPixmap		2
#define X_XF86RushUnlockAllPixmaps	3
#define X_XF86RushGetCopyMode		4
#define X_XF86RushSetCopyMode		5
#define X_XF86RushGetPixelStride	6
#define X_XF86RushSetPixelStride	7
#define X_XF86RushOverlayPixmap		8
#define X_XF86RushStatusRegOffset	9
#define X_XF86RushAT3DEnableRegs	10
#define X_XF86RushAT3DDisableRegs	11

#define XF86RushNumberEvents		0

#define XF86RushClientNotLocal		0
#define XF86RushNumberErrors		(XF86RushClientNotLocal + 1)

#ifndef _XF86RUSH_SERVER_

_XFUNCPROTOBEGIN

Bool XF86RushQueryVersion(
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
);

Bool XF86RushQueryExtension(
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
);

Bool XF86RushLockPixmap(
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */,
    void **		/* Return address */
);

Bool XF86RushUnlockPixmap(
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */
);

Bool XF86RushUnlockAllPixmaps(
    Display *		/* dpy */
);

Bool XF86RushSetCopyMode(
    Display *		/* dpy */,
    int			/* screen */,
    int			/* copy mode */
);

Bool XF86RushSetPixelStride(
    Display *		/* dpy */,
    int			/* screen */,
    int			/* pixel stride */
);

Bool XF86RushOverlayPixmap(
    Display *		/* dpy */,
    XvPortID		/* port */,
    Drawable		/* d */,
    GC			/* gc */,
    Pixmap		/* pixmap */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* src_w */,
    unsigned int	/* src_h */,
    int			/* dest_x */,
    int			/* dest_y */,
    unsigned int	/* dest_w */,
    unsigned int	/* dest_h */,
    unsigned int	/* id */
);

int XF86RushStatusRegOffset(
    Display *		/* dpy */,
    int			/* screen */
);

Bool XF86RushAT3DEnableRegs(
    Display *		/* dpy */,
    int			/* screen */
);

Bool XF86RushAT3DDisableRegs(
    Display *		/* dpy */,
    int			/* screen */
);

_XFUNCPROTOEND

#endif /* _XF86RUSH_SERVER_ */

#endif /* _XF86RUSH_H_ */
