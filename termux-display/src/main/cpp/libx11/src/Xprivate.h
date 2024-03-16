/* Copyright (C) 2008 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING.
 *
 * As Xlibint.h has long become effectively public API, this header exists
 * for new private functions that nothing outside of libX11 should call.
 */

#ifndef XPRIVATE_H
#define XPRIVATE_H

#include <X11/Xlib.h>

extern _X_HIDDEN void _XIDHandler(Display *dpy);
extern _X_HIDDEN void _XSeqSyncFunction(Display *dpy);
extern _X_HIDDEN void _XSetPrivSyncFunction(Display *dpy);
extern _X_HIDDEN void _XSetSeqSyncFunction(Display *dpy);

#ifdef XTHREADS
#if defined(XTHREADS_WARN) || defined(XTHREADS_FILE_LINE)
#define InternalLockDisplay(d,wskip) if ((d)->lock) \
    (*(d)->lock->internal_lock_display)(d,wskip,__FILE__,__LINE__)
#else
#define InternalLockDisplay(d,wskip) if ((d)->lock) \
    (*(d)->lock->internal_lock_display)(d,wskip)
#endif
#else /* XTHREADS else */
#define InternalLockDisplay(d,wskip)
#endif /* XTHREADS else */

#endif /* XPRIVATE_H */
