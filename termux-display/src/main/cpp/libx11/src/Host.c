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


/* this might be rightly regarded an os dependent file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

static inline int
changehost (Display *dpy, XHostAddress *host, BYTE mode)
{
    xChangeHostsReq *req;
    int length;
    XServerInterpretedAddress *siAddr;
    int addrlen;

    siAddr = host->family == FamilyServerInterpreted ?
	(XServerInterpretedAddress *)host->address : NULL;
    addrlen = siAddr ?
	siAddr->typelength + siAddr->valuelength + 1 : host->length;

    length = (addrlen + 3) & ~0x3;	/* round up */

    LockDisplay(dpy);
    GetReqExtra (ChangeHosts, length, req);
    if (!req) {
	UnlockDisplay(dpy);
	return 0;
    }
    req->mode = mode;
    req->hostFamily = host->family;
    req->hostLength = addrlen;
    if (siAddr) {
	char *dest = (char *) NEXTPTR(req,xChangeHostsReq);
	memcpy(dest, siAddr->type, (size_t) siAddr->typelength);
	dest[siAddr->typelength] = '\0';
	memcpy(dest + siAddr->typelength + 1,siAddr->value,(size_t) siAddr->valuelength);
    } else {
	memcpy((char *) NEXTPTR(req,xChangeHostsReq), host->address, (size_t) addrlen);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

int
XAddHost (
    register Display *dpy,
    XHostAddress *host)
{
    return changehost(dpy, host, HostInsert);
}

int
XRemoveHost (
    register Display *dpy,
    XHostAddress *host)
{
    return changehost(dpy, host, HostDelete);
}

int
XAddHosts (
    register Display *dpy,
    XHostAddress *hosts,
    int n)
{
    register int i;
    for (i = 0; i < n; i++) {
	(void) XAddHost(dpy, &hosts[i]);
      }
    return 1;
}

int
XRemoveHosts (
    register Display *dpy,
    XHostAddress *hosts,
    int n)
{
    register int i;
    for (i = 0; i < n; i++) {
	(void) XRemoveHost(dpy, &hosts[i]);
      }
    return 1;
}
