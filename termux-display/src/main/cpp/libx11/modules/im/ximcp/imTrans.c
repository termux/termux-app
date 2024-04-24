/*
 * Copyright (c) 1992, Oracle and/or its affiliates.
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
/******************************************************************

           Copyright 1992, 1993, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
FUJITSU LIMITED makes no representations about the suitability of
this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Hideki Hiura (hhiura@Sun.COM) Sun Microsystems, Inc.
          Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include "Xlibint.h"
#include <X11/Xtrans/Xtrans.h>
#include "Xlcint.h"
#include "Ximint.h"
#include "XimTrans.h"
#include "XimTrInt.h"

#ifdef WIN32
#include <X11/Xwindows.h>
#endif


#ifndef XIM_CONNECTION_RETRIES
#define XIM_CONNECTION_RETRIES 5
#endif


static Bool
_XimTransConnect(
    Xim			 im)
{
    TransSpecRec	*spec = (TransSpecRec *)im->private.proto.spec;
    int			connect_stat, retry;
    Window		window;

    for (retry = XIM_CONNECTION_RETRIES; retry >= 0; retry--)
    {
	if ((spec->trans_conn = _XimXTransOpenCOTSClient (
	    spec->address)) == NULL)
	{
	    break;
	}

	if ((connect_stat = _XimXTransConnect (
	    spec->trans_conn, spec->address)) < 0)
	{
	    _XimXTransClose (spec->trans_conn);
	    spec->trans_conn = NULL;

	    if (connect_stat == TRANS_TRY_CONNECT_AGAIN)
		continue;
	    else
		break;
	}
	else
	    break;
    }

    if (spec->trans_conn == NULL)
	return False;

    spec->fd = _XimXTransGetConnectionNumber (spec->trans_conn);

    if (!(window = XCreateSimpleWindow(im->core.display,
		DefaultRootWindow(im->core.display), 0, 0, 1, 1, 1, 0, 0)))
	return False;
    spec->window = window;

    _XRegisterFilterByType(im->core.display, window, KeyPress, KeyPress,
				_XimTransFilterWaitEvent, (XPointer)im);

    return _XRegisterInternalConnection(im->core.display, spec->fd,
			(_XInternalConnectionProc)_XimTransInternalConnection,
			(XPointer)im);
}


static Bool
_XimTransShutdown(
    Xim im)
{
    TransSpecRec *spec = (TransSpecRec *)im->private.proto.spec;

    _XimXTransDisconnect(spec->trans_conn);
    (void)_XimXTransClose(spec->trans_conn);
    _XimFreeTransIntrCallback(im);
    _XUnregisterInternalConnection(im->core.display, spec->fd);
    _XUnregisterFilter(im->core.display, spec->window,
				_XimTransFilterWaitEvent, (XPointer)im);
    XDestroyWindow(im->core.display, spec->window);
    Xfree(spec->address);
    Xfree(spec);
    return True;
}



Bool
_XimTransRegisterDispatcher(
    Xim				 im,
    Bool			 (*callback)(
					     Xim, INT16, XPointer, XPointer
					     ),
    XPointer			 call_data)
{
    TransSpecRec		*spec = (TransSpecRec *)im->private.proto.spec;
    TransIntrCallbackPtr	 rec;

    if (!(rec = Xmalloc(sizeof(TransIntrCallbackRec))))
        return False;

    rec->func       = callback;
    rec->call_data  = call_data;
    rec->next       = spec->intr_cb;
    spec->intr_cb   = rec;
    return True;
}


void
_XimFreeTransIntrCallback(
    Xim				 im)
{
    TransSpecRec		*spec = (TransSpecRec *)im->private.proto.spec;
    register TransIntrCallbackPtr	 rec, next;

    for (rec = spec->intr_cb; rec;) {
	next = rec->next;
	Xfree(rec);
	rec = next;
    }
    spec->intr_cb = NULL;
    return;
}


Bool
_XimTransCallDispatcher(Xim im, INT16 len, XPointer data)
{
    TransSpecRec		*spec = (TransSpecRec *)im->private.proto.spec;
    TransIntrCallbackRec	*rec;

    for (rec = spec->intr_cb; rec; rec = rec->next) {
	if ((*rec->func)(im, len, data, rec->call_data))
	    return True;
    }
    return False;
}


Bool
_XimTransFilterWaitEvent(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 arg)
{
    Xim			 im = (Xim)arg;
    TransSpecRec	*spec = (TransSpecRec *)im->private.proto.spec;

    spec->is_putback  = False;
    return _XimFilterWaitEvent(im);
}


void
_XimTransInternalConnection(
    Display		*d,
    int			 fd,
    XPointer		 arg)
{
    Xim			 im = (Xim)arg;
    XEvent		 ev;
    XKeyEvent		*kev;
    TransSpecRec	*spec = (TransSpecRec *)im->private.proto.spec;

    if (spec->is_putback)
	return;

    bzero(&ev, sizeof(ev));	/* FIXME: other fields may be accessed, too. */
    kev = (XKeyEvent *)&ev;
    kev->type = KeyPress;
    kev->send_event = False;
    kev->display = im->core.display;
    kev->window = spec->window;
    kev->keycode = 0;
    kev->time = 0L;
    kev->serial = LastKnownRequestProcessed(im->core.display);
#if 0
    fprintf(stderr,"%s,%d: putback FIXED kev->time=0 kev->serial=%lu\n", __FILE__, __LINE__, kev->serial);
#endif

    XPutBackEvent(im->core.display, &ev);
    XFlush(im->core.display);
    spec->is_putback = True;
    return;
}


Bool
_XimTransWrite(Xim im, INT16 len, XPointer data)
{
    TransSpecRec	*spec	= (TransSpecRec *)im->private.proto.spec;
    char		*buf = (char *)data;
    register int	 nbyte;

    while (len > 0) {
	if ((nbyte = _XimXTransWrite(spec->trans_conn, buf, len)) <= 0)
	    return False;
	len -= nbyte;
	buf += nbyte;
    }
    return True;
}


Bool
_XimTransRead(
    Xim			 im,
    XPointer		 recv_buf,
    int			 buf_len,
    int			*ret_len)
{
    TransSpecRec	*spec = (TransSpecRec *)im->private.proto.spec;
    int			 len;

    if (buf_len == 0) {
	*ret_len = 0;
	return True;
    }
    if ((len = _XimXTransRead(spec->trans_conn, recv_buf, buf_len)) <= 0)
	return False;
    *ret_len = len;
    return True;
}


void
_XimTransFlush(
    Xim		 im)
{
    return;
}



Bool
_XimTransConf(
    Xim		   	 im,
    char	 	*address)
{
    char		*paddr;
    TransSpecRec	*spec;

    if (!(paddr = strdup(address)))
	return False;

    if (!(spec = Xcalloc(1, sizeof(TransSpecRec)))) {
	Xfree(paddr);
	return False;
    }

    spec->address   = paddr;

    im->private.proto.spec     = (XPointer)spec;
    im->private.proto.connect  = _XimTransConnect;
    im->private.proto.shutdown = _XimTransShutdown;
    im->private.proto.write    = _XimTransWrite;
    im->private.proto.read     = _XimTransRead;
    im->private.proto.flush    = _XimTransFlush;
    im->private.proto.register_dispatcher = _XimTransRegisterDispatcher;
    im->private.proto.call_dispatcher = _XimTransCallDispatcher;

    return True;
}
