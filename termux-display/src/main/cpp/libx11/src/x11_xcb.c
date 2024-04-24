/* Copyright (C) 2003,2006 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING. */

#include "Xlibint.h"
#include "Xxcbint.h"

xcb_connection_t *XGetXCBConnection(Display *dpy)
{
	return dpy->xcb->connection;
}

void XSetEventQueueOwner(Display *dpy, enum XEventQueueOwner owner)
{
	dpy->xcb->event_owner = owner;
}
