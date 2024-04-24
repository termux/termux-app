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
#include <limits.h>
#include <stdbool.h>
#include "Xlibint.h"

Bool
XQueryExtension(
    register Display *dpy,
    _Xconst char *name,
    int *major_opcode,  /* RETURN */
    int *first_event,   /* RETURN */
    int *first_error)	/* RETURN */
{
    xQueryExtensionReply rep;
    register xQueryExtensionReq *req;

    if (name != NULL && strlen(name) >= USHRT_MAX)
        return false;

    LockDisplay(dpy);
    GetReq(QueryExtension, req);
    req->nbytes = name ? (CARD16) strlen(name) : 0;
    req->length += (req->nbytes+(unsigned)3)>>2;
    _XSend(dpy, name, (long)req->nbytes);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *major_opcode = rep.major_opcode;
    *first_event = rep.first_event;
    *first_error = rep.first_error;
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.present);
}

