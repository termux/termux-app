/*

Copyright 1986, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

int
XGetScreenSaver(
     register Display *dpy,
     /* the following are return only vars */
     int *timeout,
     int *interval,
     int *prefer_blanking,
     int *allow_exp)  /*boolean */

{
    xGetScreenSaverReply rep;
    _X_UNUSED register xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetScreenSaver, req);

    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *timeout = rep.timeout;
    *interval = rep.interval;
    *prefer_blanking = rep.preferBlanking;
    *allow_exp = rep.allowExposures;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

