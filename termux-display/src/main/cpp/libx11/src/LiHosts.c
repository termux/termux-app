/*

Copyright 1986, 1998  The Open Group

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

X Window System is a trademark of The Open Group.

*/

/*
 * Copyright (c) 2004, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* This can really be considered an os dependent routine */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <limits.h>

/*
 * can be freed using XFree.
 */

XHostAddress *XListHosts (
    register Display *dpy,
    int *nhosts,	/* RETURN */
    Bool *enabled)	/* RETURN */
{
    register XHostAddress *outbuf = NULL, *op;
    xListHostsReply reply;
    unsigned char *buf, *bp;
    register unsigned i;
    _X_UNUSED register xListHostsReq *req;
    XServerInterpretedAddress *sip;

    *nhosts = 0;
    LockDisplay(dpy);
    GetReq (ListHosts, req);

    if (!_XReply (dpy, (xReply *) &reply, 0, xFalse)) {
       UnlockDisplay(dpy);
       SyncHandle();
       return (XHostAddress *) NULL;
    }

    if (reply.nHosts) {
	unsigned long nbytes = reply.length << 2; /* number of bytes in reply */
	const unsigned long max_hosts = INT_MAX /
	    (sizeof(XHostAddress) + sizeof(XServerInterpretedAddress));

	if (reply.nHosts < max_hosts) {
	    unsigned long hostbytes = reply.nHosts *
		(sizeof(XHostAddress) + sizeof(XServerInterpretedAddress));

	    if (reply.length < (INT_MAX >> 2) &&
		(hostbytes >> 2) < ((INT_MAX >> 2) - reply.length))
		outbuf = Xmalloc(nbytes + hostbytes);
	}

	if (! outbuf) {
	    _XEatDataWords(dpy, reply.length);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XHostAddress *) NULL;
	}
	op = outbuf;
	sip = (XServerInterpretedAddress *)
	 (((unsigned char  *) outbuf) + (reply.nHosts * sizeof(XHostAddress)));
	bp = buf = ((unsigned char  *) sip)
	  + (reply.nHosts * sizeof(XServerInterpretedAddress));

	_XRead (dpy, (char *) buf, nbytes);

	for (i = 0; i < reply.nHosts; i++) {
	    if (bp > buf + nbytes - SIZEOF(xHostEntry))
		goto fail;
	    op->family = ((xHostEntry *) bp)->family;
	    op->length =((xHostEntry *) bp)->length;
	    if (op->family == FamilyServerInterpreted) {
		char *tp = (char *) (bp + SIZEOF(xHostEntry));
		char *vp;
		if (tp > (char *) (buf + nbytes - op->length))
		    goto fail;
		vp = memchr(tp, 0, op->length);

		if (vp != NULL) {
		    sip->type = tp;
		    sip->typelength = vp - tp;
		    sip->value = vp + 1;
		    sip->valuelength = op->length - (sip->typelength + 1);
		} else {
		    sip->type = sip->value = NULL;
		    sip->typelength = sip->valuelength = 0;
		}
		op->address = (char *) sip;
		sip++;
	    } else {
		op->address = (char *) (bp + SIZEOF(xHostEntry));
		if (op->address > (char *) (buf + nbytes - op->length))
		    goto fail;
	    }
	    bp += SIZEOF(xHostEntry) + (((op->length + 3) >> 2) << 2);
	    op++;
	}
    }

    *enabled = reply.enabled;
    *nhosts = reply.nHosts;
    UnlockDisplay(dpy);
    SyncHandle();
    return (outbuf);
fail:
    *enabled = reply.enabled;
    *nhosts = 0;
    Xfree(outbuf);
    return (NULL);
}
