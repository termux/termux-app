/* Copyright (C) 2003-2006 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Xlibint.h"
#include "Xxcbint.h"
#include <xcb/xcbext.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <stdio.h>

static xcb_auth_info_t xauth;

static void *alloc_copy(const void *src, int *dstn, size_t n)
{
	void *dst;
	if(n <= 0)
	{
		*dstn = 0;
		return NULL;
	}
	dst = Xmalloc(n);
	if(!dst)
		return NULL;
	memcpy(dst, src, n);
	*dstn = n;
	return dst;
}

void XSetAuthorization(char *name, int namelen, char *data, int datalen)
{
	_XLockMutex(_Xglobal_lock);
	Xfree(xauth.name);
	Xfree(xauth.data);

	/* if either of these allocs fail, _XConnectXCB won't use this auth
	 * data, so we don't need to check it here. */
	xauth.name = alloc_copy(name, &xauth.namelen, namelen);
	xauth.data = alloc_copy(data, &xauth.datalen, datalen);

#if 0 /* but, for the paranoid among us: */
	if((namelen > 0 && !xauth.name) || (datalen > 0 && !xauth.data))
	{
		Xfree(xauth.name);
		Xfree(xauth.data);
		xauth.name = xauth.data = 0;
		xauth.namelen = xauth.datalen = 0;
	}
#endif

	_XUnlockMutex(_Xglobal_lock);
}

int _XConnectXCB(Display *dpy, _Xconst char *display, int *screenp)
{
	char *host;
	int n = 0;
	xcb_connection_t *c;

	dpy->fd = -1;

	dpy->xcb = Xcalloc(1, sizeof(_X11XCBPrivate));
	if(!dpy->xcb)
		return 0;

	if(!xcb_parse_display(display, &host, &n, screenp))
		return 0;
	/* host and n are unused, but xcb_parse_display requires them */
	free(host);

	_XLockMutex(_Xglobal_lock);
	if(xauth.name && xauth.data)
		c = xcb_connect_to_display_with_auth_info(display, &xauth, NULL);
	else
		c = xcb_connect(display, NULL);
	_XUnlockMutex(_Xglobal_lock);

	dpy->fd = xcb_get_file_descriptor(c);

	dpy->xcb->connection = c;
	dpy->xcb->next_xid = xcb_generate_id(dpy->xcb->connection);

	dpy->xcb->event_notify = xcondition_malloc();
	dpy->xcb->reply_notify = xcondition_malloc();
	if (!dpy->xcb->event_notify || !dpy->xcb->reply_notify)
		return 0;
	xcondition_init(dpy->xcb->event_notify);
	xcondition_init(dpy->xcb->reply_notify);
	return !xcb_connection_has_error(c);
}

void _XFreeX11XCBStructure(Display *dpy)
{
	/* reply_data was allocated by system malloc, not Xmalloc */
	free(dpy->xcb->reply_data);
	while(dpy->xcb->pending_requests)
	{
		PendingRequest *tmp = dpy->xcb->pending_requests;
		dpy->xcb->pending_requests = tmp->next;
		free(tmp);
	}
	if (dpy->xcb->event_notify)
		xcondition_clear(dpy->xcb->event_notify);
	if (dpy->xcb->reply_notify)
		xcondition_clear(dpy->xcb->reply_notify);
	xcondition_free(dpy->xcb->event_notify);
	xcondition_free(dpy->xcb->reply_notify);
	Xfree(dpy->xcb);
	dpy->xcb = NULL;
}
