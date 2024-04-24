/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifndef OPAQUE_H
#define OPAQUE_H

#include <X11/Xmd.h>

#include "globals.h"

extern _X_EXPORT int LimitClients;
extern _X_EXPORT volatile char isItTimeToYield;
extern _X_EXPORT volatile char dispatchException;

/* bit values for dispatchException */
#define DE_RESET     1
#define DE_TERMINATE 2
#define DE_PRIORITYCHANGE 4     /* set when a client's priority changes */

extern _X_EXPORT int ScreenSaverBlanking;
extern _X_EXPORT int ScreenSaverAllowExposures;
extern _X_EXPORT int defaultScreenSaverBlanking;
extern _X_EXPORT int defaultScreenSaverAllowExposures;
extern _X_EXPORT const char *display;
extern _X_EXPORT int displayfd;
extern _X_EXPORT Bool explicit_display;

extern _X_EXPORT Bool disableBackingStore;
extern _X_EXPORT Bool enableBackingStore;
extern _X_EXPORT Bool enableIndirectGLX;
extern _X_EXPORT Bool PartialNetwork;
extern _X_EXPORT Bool RunFromSigStopParent;

#ifdef RLIMIT_DATA
extern _X_EXPORT int limitDataSpace;
#endif
#ifdef RLIMIT_STACK
extern _X_EXPORT int limitStackSpace;
#endif
#ifdef RLIMIT_NOFILE
extern _X_EXPORT int limitNoFile;
#endif
extern _X_EXPORT Bool defeatAccessControl;
extern _X_EXPORT long maxBigRequestSize;
extern _X_EXPORT Bool party_like_its_1989;
extern _X_EXPORT Bool whiteRoot;
extern _X_EXPORT Bool bgNoneRoot;

extern _X_EXPORT Bool CoreDump;
extern _X_EXPORT Bool NoListenAll;

#endif                          /* OPAQUE_H */
