/***********************************************************
Copyright 1993 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include "Ximint.h"


#define sw16(n, s) ((s) ?                    \
		      (((n) << 8 & 0xff00) | \
		       ((n) >> 8 & 0xff)     \
		      ) : n)

#define sw32(n, s) ((s) ?                         \
		      (((n) << 24 & 0xff000000) | \
		       ((n) <<  8 & 0xff0000) |   \
		       ((n) >>  8 & 0xff00) |     \
		       ((n) >> 24 & 0xff)         \
		      ) : n)

Status
_XimProtoEventToWire(
    register XEvent *re,	/* pointer to where event should be reformatted */
    register xEvent *event,	/* wire protocol event */
    Bool sw)         /* swap byte? */
{
	switch (event->u.u.type = re->type) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent *ev = (XKeyEvent*) re;
			event->u.keyButtonPointer.root	= sw32(ev->root, sw);
			event->u.keyButtonPointer.event	 =
			    sw32(ev->window, sw);
			event->u.keyButtonPointer.child  =
			    sw32(ev->subwindow, sw);
			event->u.keyButtonPointer.time	 = sw32(ev->time, sw);
			event->u.keyButtonPointer.eventX = sw16(ev->x, sw) ;
			event->u.keyButtonPointer.eventY = sw16(ev->y, sw) ;
			event->u.keyButtonPointer.rootX	 =
			    sw16(ev->x_root, sw);
			event->u.keyButtonPointer.rootY  =
			    sw16(ev->y_root, sw);
			event->u.keyButtonPointer.state  = sw16(ev->state, sw);
			event->u.keyButtonPointer.sameScreen = ev->same_screen;
			event->u.u.detail = ev->keycode;
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent *ev =  (XButtonEvent *) re;
			event->u.keyButtonPointer.root	 = sw32(ev->root, sw);
			event->u.keyButtonPointer.event	 = sw32(ev->window, sw);
			event->u.keyButtonPointer.child	 = sw32(ev->subwindow, sw);
			event->u.keyButtonPointer.time	 = sw32(ev->time, sw);
			event->u.keyButtonPointer.eventX = sw16(ev->x, sw);
			event->u.keyButtonPointer.eventY = sw16(ev->y, sw);
			event->u.keyButtonPointer.rootX	 = sw16(ev->x_root, sw);
			event->u.keyButtonPointer.rootY	 = sw16(ev->y_root, sw);
			event->u.keyButtonPointer.state	 = sw16(ev->state, sw);
			event->u.keyButtonPointer.sameScreen	= ev->same_screen;
			event->u.u.detail		= ev->button;
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent *ev =   (XMotionEvent *)re;
			event->u.keyButtonPointer.root	= sw32(ev->root, sw);
			event->u.keyButtonPointer.event	= sw32(ev->window, sw);
			event->u.keyButtonPointer.child	= sw32(ev->subwindow, sw);
			event->u.keyButtonPointer.time	= sw32(ev->time, sw);
			event->u.keyButtonPointer.eventX= sw16(ev->x, sw);
			event->u.keyButtonPointer.eventY= sw16(ev->y, sw);
			event->u.keyButtonPointer.rootX	= sw16(ev->x_root, sw);
			event->u.keyButtonPointer.rootY	= sw16(ev->y_root, sw);
			event->u.keyButtonPointer.state	= sw16(ev->state, sw);
			event->u.keyButtonPointer.sameScreen= ev->same_screen;
			event->u.u.detail		= ev->is_hint;
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent *ev   = (XCrossingEvent *) re;
			event->u.enterLeave.root	= sw32(ev->root, sw);
			event->u.enterLeave.event	= sw32(ev->window, sw);
			event->u.enterLeave.child	= sw32(ev->subwindow, sw);
			event->u.enterLeave.time	= sw32(ev->time, sw);
			event->u.enterLeave.eventX	= sw16(ev->x, sw);
			event->u.enterLeave.eventY	= sw16(ev->y, sw);
			event->u.enterLeave.rootX	= sw16(ev->x_root, sw);
			event->u.enterLeave.rootY	= sw16(ev->y_root, sw);
			event->u.enterLeave.state	= sw16(ev->state, sw);
			event->u.enterLeave.mode	= ev->mode;
			event->u.enterLeave.flags	= 0;
			if (ev->same_screen) {
				event->u.enterLeave.flags |= ELFlagSameScreen;
				}
			if (ev->focus) {
				event->u.enterLeave.flags |= ELFlagFocus;
				}
			event->u.u.detail		= ev->detail;
		}
		  break;
	      case FocusIn:
	      case FocusOut:
		{
			register XFocusChangeEvent *ev = (XFocusChangeEvent *) re;
			event->u.focus.window	= sw32(ev->window, sw);
			event->u.focus.mode	= ev->mode;
			event->u.u.detail	= ev->detail;
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent *ev = (XKeymapEvent *) re;
			memcpy((char *)(((xKeymapEvent *) event)->map),
			       &ev->key_vector[1],
			       sizeof (((xKeymapEvent *) event)->map));
		}
		break;
	      case Expose:
		{
			register XExposeEvent *ev = (XExposeEvent *) re;
			event->u.expose.window		= sw32(ev->window, sw);
			event->u.expose.x		= sw16(ev->x, sw);
			event->u.expose.y		= sw16(ev->y, sw);
			event->u.expose.width		= sw16(ev->width, sw);
			event->u.expose.height		= sw16(ev->height, sw);
			event->u.expose.count		= sw16(ev->count, sw);
		}
		break;
	      case GraphicsExpose:
		{
		    register XGraphicsExposeEvent *ev =
			(XGraphicsExposeEvent *) re;
		    event->u.graphicsExposure.drawable	= sw32(ev->drawable, sw);
		    event->u.graphicsExposure.x		= sw16(ev->x, sw);
		    event->u.graphicsExposure.y		= sw16(ev->y, sw);
		    event->u.graphicsExposure.width	= sw16(ev->width, sw);
		    event->u.graphicsExposure.height	= sw16(ev->height, sw);
		    event->u.graphicsExposure.count	= sw16(ev->count, sw);
		    event->u.graphicsExposure.majorEvent= ev->major_code;
		    event->u.graphicsExposure.minorEvent= sw16(ev->minor_code, sw);
		}
		break;
	      case NoExpose:
		{
		    register XNoExposeEvent *ev = (XNoExposeEvent *) re;
		    event->u.noExposure.drawable	= sw32(ev->drawable, sw);
		    event->u.noExposure.majorEvent	= ev->major_code;
		    event->u.noExposure.minorEvent	= sw16(ev->minor_code, sw);
		}
		break;
	      case VisibilityNotify:
		{
		    register XVisibilityEvent *ev = (XVisibilityEvent *) re;
		    event->u.visibility.window		= sw32(ev->window, sw);
		    event->u.visibility.state		= ev->state;
		}
		break;
	      case CreateNotify:
		{
		    register XCreateWindowEvent *ev =
			 (XCreateWindowEvent *) re;
		    event->u.createNotify.window	= sw32(ev->window, sw);
		    event->u.createNotify.parent	= sw32(ev->parent, sw);
		    event->u.createNotify.x		= sw16(ev->x, sw);
		    event->u.createNotify.y		= sw16(ev->y, sw);
		    event->u.createNotify.width		= sw16(ev->width, sw);
		    event->u.createNotify.height	= sw16(ev->height, sw);
		    event->u.createNotify.borderWidth	= sw16(ev->border_width, sw);
		    event->u.createNotify.override	= ev->override_redirect;
		}
		break;
	      case DestroyNotify:
		{
		    register XDestroyWindowEvent *ev =
				(XDestroyWindowEvent *) re;
		    event->u.destroyNotify.window	= sw32(ev->window, sw);
		    event->u.destroyNotify.event	= sw32(ev->event, sw);
		}
		break;
	      case UnmapNotify:
		{
		    register XUnmapEvent *ev = (XUnmapEvent *) re;
		    event->u.unmapNotify.window	= sw32(ev->window, sw);
		    event->u.unmapNotify.event	= sw32(ev->event, sw);
		    event->u.unmapNotify.fromConfigure	= ev->from_configure;
		}
		break;
	      case MapNotify:
		{
		    register XMapEvent *ev = (XMapEvent *) re;
		    event->u.mapNotify.window	= sw32(ev->window, sw);
		    event->u.mapNotify.event	= sw32(ev->event, sw);
		    event->u.mapNotify.override	= ev->override_redirect;
		}
		break;
	      case MapRequest:
		{
		    register XMapRequestEvent *ev = (XMapRequestEvent *) re;
		    event->u.mapRequest.window	= sw32(ev->window, sw);
		    event->u.mapRequest.parent	= sw32(ev->parent, sw);
		}
		break;
	      case ReparentNotify:
		{
		    register XReparentEvent *ev = (XReparentEvent *) re;
		    event->u.reparent.window	= sw32(ev->window, sw);
		    event->u.reparent.event	= sw32(ev->event, sw);
		    event->u.reparent.parent	= sw32(ev->parent, sw);
		    event->u.reparent.x		= sw16(ev->x, sw);
		    event->u.reparent.y		= sw16(ev->y, sw);
		    event->u.reparent.override	= ev->override_redirect;
		}
		break;
	      case ConfigureNotify:
		{
		    register XConfigureEvent *ev = (XConfigureEvent *) re;
		    event->u.configureNotify.window	= sw32(ev->window, sw);
		    event->u.configureNotify.event	= sw32(ev->event, sw);
		    event->u.configureNotify.aboveSibling	= sw32(ev->above, sw);
		    event->u.configureNotify.x		= sw16(ev->x, sw);
		    event->u.configureNotify.y		= sw16(ev->y, sw);
		    event->u.configureNotify.width	= sw16(ev->width, sw);
		    event->u.configureNotify.height	= sw16(ev->height, sw);
		    event->u.configureNotify.borderWidth= sw16(ev->border_width, sw);
		    event->u.configureNotify.override	= ev->override_redirect;
		}
		break;
	      case ConfigureRequest:
		{
		    register XConfigureRequestEvent *ev =
		        (XConfigureRequestEvent *) re;
		    event->u.configureRequest.window	= sw32(ev->window, sw);
		    event->u.configureRequest.parent	= sw32(ev->parent, sw);
		    event->u.configureRequest.sibling   = sw32(ev->above, sw);
		    event->u.configureRequest.x		= sw16(ev->x, sw);
		    event->u.configureRequest.y		= sw16(ev->y, sw);
		    event->u.configureRequest.width	= sw16(ev->width, sw);
		    event->u.configureRequest.height	= sw16(ev->height, sw);
		    event->u.configureRequest.borderWidth= sw16(ev->border_width, sw);
		    event->u.configureRequest.valueMask= sw16(ev->value_mask, sw);
		    event->u.u.detail 			= ev->detail;
		}
		break;
	      case GravityNotify:
		{
		    register XGravityEvent *ev  = (XGravityEvent *) re;
		    event->u.gravity.window	= sw32(ev->window, sw);
		    event->u.gravity.event	= sw32(ev->event, sw);
		    event->u.gravity.x		= sw16(ev->x, sw);
		    event->u.gravity.y		= sw16(ev->y, sw);
		}
		break;
	      case ResizeRequest:
		{
		    register XResizeRequestEvent *ev =
			(XResizeRequestEvent *) re;
		    event->u.resizeRequest.window	= sw32(ev->window, sw);
		    event->u.resizeRequest.width	= sw16(ev->width, sw);
		    event->u.resizeRequest.height	= sw16(ev->height, sw);
		}
		break;
	      case CirculateNotify:
		{
		    register XCirculateEvent *ev = (XCirculateEvent *) re;
		    event->u.circulate.window		= sw32(ev->window, sw);
		    event->u.circulate.event		= sw32(ev->event, sw);
		    event->u.circulate.place		= ev->place;
		}
		break;
	      case CirculateRequest:
		{
		    register XCirculateRequestEvent *ev =
		        (XCirculateRequestEvent *) re;
		    event->u.circulate.window		= sw32(ev->window, sw);
		    event->u.circulate.event		= sw32(ev->parent, sw);
		    event->u.circulate.place		= ev->place;
		}
		break;
	      case PropertyNotify:
		{
		    register XPropertyEvent *ev = (XPropertyEvent *) re;
		    event->u.property.window		= sw32(ev->window, sw);
		    event->u.property.atom		= sw32(ev->atom, sw);
		    event->u.property.time		= sw32(ev->time, sw);
		    event->u.property.state		= ev->state;
		}
		break;
	      case SelectionClear:
		{
		    register XSelectionClearEvent *ev =
			 (XSelectionClearEvent *) re;
		    event->u.selectionClear.window	= sw32(ev->window, sw);
		    event->u.selectionClear.atom	= sw32(ev->selection, sw);
		    event->u.selectionClear.time	= sw32(ev->time, sw);
		}
		break;
	      case SelectionRequest:
		{
		    register XSelectionRequestEvent *ev =
		        (XSelectionRequestEvent *) re;
		    event->u.selectionRequest.owner	= sw32(ev->owner, sw);
		    event->u.selectionRequest.requestor	= sw32(ev->requestor, sw);
		    event->u.selectionRequest.selection	= sw32(ev->selection, sw);
		    event->u.selectionRequest.target	= sw32(ev->target, sw);
		    event->u.selectionRequest.property	= sw32(ev->property, sw);
		    event->u.selectionRequest.time	= sw32(ev->time, sw);
		}
		break;
	      case SelectionNotify:
		{
		    register XSelectionEvent *ev = (XSelectionEvent *) re;
		    event->u.selectionNotify.requestor	= sw32(ev->requestor, sw);
		    event->u.selectionNotify.selection	= sw32(ev->selection, sw);
		    event->u.selectionNotify.target	= sw32(ev->target, sw);
		    event->u.selectionNotify.property	= sw32(ev->property, sw);
		    event->u.selectionNotify.time	= sw32(ev->time, sw);
		}
		break;
	      case ColormapNotify:
		{
		    register XColormapEvent *ev = (XColormapEvent *) re;
		    event->u.colormap.window		= sw32(ev->window, sw);
		    event->u.colormap.colormap		= sw32(ev->colormap, sw);
		    event->u.colormap.new		= ev->new;
		    event->u.colormap.state		= ev->state;
	        }
		break;
	      case ClientMessage:
		{
		   register int i;
		   register XClientMessageEvent *ev
		   			= (XClientMessageEvent *) re;
		   event->u.clientMessage.window = sw32(ev->window, sw);
		   event->u.u.detail		 = ev->format;
		   switch (ev->format) {
			case 8:
			  event->u.clientMessage.u.b.type   = sw32(ev->message_type, sw);
			  for (i = 0; i < 20; i++)
			   event->u.clientMessage.u.b.bytes[i] = ev->data.b[i];
			  break;
			case 16:
			  event->u.clientMessage.u.s.type   = sw32(ev->message_type, sw);
			  event->u.clientMessage.u.s.shorts0   = sw16(ev->data.s[0], sw);
			  event->u.clientMessage.u.s.shorts1   = sw16(ev->data.s[1], sw);
			  event->u.clientMessage.u.s.shorts2   = sw16(ev->data.s[2], sw);
			  event->u.clientMessage.u.s.shorts3   = sw16(ev->data.s[3], sw);
			  event->u.clientMessage.u.s.shorts4   = sw16(ev->data.s[4], sw);
			  event->u.clientMessage.u.s.shorts5   = sw16(ev->data.s[5], sw);
			  event->u.clientMessage.u.s.shorts6   = sw16(ev->data.s[6], sw);
			  event->u.clientMessage.u.s.shorts7   = sw16(ev->data.s[7], sw);
			  event->u.clientMessage.u.s.shorts8   = sw16(ev->data.s[8], sw);
			  event->u.clientMessage.u.s.shorts9   = sw16(ev->data.s[9], sw);
			  break;
			case 32:
			  event->u.clientMessage.u.l.type   = sw32(ev->message_type, sw);
			  event->u.clientMessage.u.l.longs0   = sw32(ev->data.l[0], sw);
			  event->u.clientMessage.u.l.longs1   = sw32(ev->data.l[1], sw);
			  event->u.clientMessage.u.l.longs2   = sw32(ev->data.l[2], sw);
			  event->u.clientMessage.u.l.longs3   = sw32(ev->data.l[3], sw);
			  event->u.clientMessage.u.l.longs4   = sw32(ev->data.l[4], sw);
			  break;
			default:
			  /* client passing bogus data, let server complain */
			  break;
			}
		    }
		break;
	      case MappingNotify:
		  {
		    register XMappingEvent *ev = (XMappingEvent *) re;
		    event->u.mappingNotify.firstKeyCode = ev->first_keycode;
		    event->u.mappingNotify.request 	= ev->request;
		    event->u.mappingNotify.count	= ev->count;
		   }
		break;

	      default:
		return(0);
	}
	/* Common process */
	if (((XAnyEvent *)re)->send_event)
	    event->u.u.type |= 0x80;
	event->u.u.sequenceNumber =
	    ((XAnyEvent *)re)->serial & ~((unsigned long)0xffff);
	event->u.u.sequenceNumber = sw16(event->u.u.sequenceNumber, sw);
	return(1);
}


/*
 * reformat a wire event into an XEvent structure of the right type.
 */
Bool
_XimProtoWireToEvent(
    register XEvent *re,	/* pointer to where event should be reformatted */
    register xEvent *event,	/* wire protocol event */
    Bool sw)                /* swap byte? */
{

	re->type = event->u.u.type & 0x7f;
	((XAnyEvent *)re)->serial = sw16(event->u.u.sequenceNumber, sw);
	((XAnyEvent *)re)->send_event = ((event->u.u.type & 0x80) != 0);
	((XAnyEvent *)re)->display = NULL;

	/* Ignore the leading bit of the event type since it is set when a
		client sends an event rather than the server. */

	switch (event-> u.u.type & 0177) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent *ev = (XKeyEvent*) re;
			ev->root 	= sw32(event->u.keyButtonPointer.root, sw);
			ev->window 	= sw32(event->u.keyButtonPointer.event, sw);
			ev->subwindow 	= sw32(event->u.keyButtonPointer.child, sw);
			ev->time 	= sw32(event->u.keyButtonPointer.time, sw);
			ev->x 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventX, sw));
			ev->y 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventY, sw));
			ev->x_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootX, sw));
			ev->y_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootY, sw));
			ev->state	= sw16(event->u.keyButtonPointer.state, sw);
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->keycode 	= event->u.u.detail;
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent *ev =  (XButtonEvent *) re;
			ev->root 	= sw32(event->u.keyButtonPointer.root, sw);
			ev->window 	= sw32(event->u.keyButtonPointer.event, sw);
			ev->subwindow 	= sw32(event->u.keyButtonPointer.child, sw);
			ev->time 	= sw32(event->u.keyButtonPointer.time, sw);
			ev->x 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventX, sw));
			ev->y 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventY, sw));
			ev->x_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootX, sw));
			ev->y_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootY, sw));
			ev->state	= sw16(event->u.keyButtonPointer.state, sw);
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->button 	= event->u.u.detail;
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent *ev =   (XMotionEvent *)re;
			ev->root 	= sw32(event->u.keyButtonPointer.root, sw);
			ev->window 	= sw32(event->u.keyButtonPointer.event, sw);
			ev->subwindow 	= sw32(event->u.keyButtonPointer.child, sw);
			ev->time 	= sw32(event->u.keyButtonPointer.time, sw);
			ev->x 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventX, sw));
			ev->y 		= cvtINT16toInt(sw16(event->u.keyButtonPointer.eventY, sw));
			ev->x_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootX, sw));
			ev->y_root 	= cvtINT16toInt(sw16(event->u.keyButtonPointer.rootY, sw));
			ev->state	= sw16(event->u.keyButtonPointer.state, sw);
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->is_hint 	= event->u.u.detail;
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent *ev   = (XCrossingEvent *) re;
			ev->root	= sw32(event->u.enterLeave.root, sw);
			ev->window	= sw32(event->u.enterLeave.event, sw);
			ev->subwindow	= sw32(event->u.enterLeave.child, sw);
			ev->time	= sw32(event->u.enterLeave.time, sw);
			ev->x		= cvtINT16toInt(sw16(event->u.enterLeave.eventX, sw));
			ev->y		= cvtINT16toInt(sw16(event->u.enterLeave.eventY, sw));
			ev->x_root	= cvtINT16toInt(sw16(event->u.enterLeave.rootX, sw));
			ev->y_root	= cvtINT16toInt(sw16(event->u.enterLeave.rootY, sw));
			ev->state	= sw16(event->u.enterLeave.state, sw);
			ev->mode	= event->u.enterLeave.mode;
			ev->same_screen = (event->u.enterLeave.flags &
				ELFlagSameScreen) && True;
			ev->focus	= (event->u.enterLeave.flags &
			  	ELFlagFocus) && True;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case FocusIn:
	      case FocusOut:
		{
			register XFocusChangeEvent *ev = (XFocusChangeEvent *) re;
			ev->window 	= sw32(event->u.focus.window, sw);
			ev->mode	= event->u.focus.mode;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent *ev = (XKeymapEvent *) re;
			ev->window	= None;
			memcpy(&ev->key_vector[1],
			       (char *)((xKeymapEvent *) event)->map,
			       sizeof (((xKeymapEvent *) event)->map));
		}
		break;
	      case Expose:
		{
			register XExposeEvent *ev = (XExposeEvent *) re;
			ev->window	= sw32(event->u.expose.window, sw);
			ev->x		= sw16(event->u.expose.x, sw);
			ev->y		= sw16(event->u.expose.y, sw);
			ev->width	= sw16(event->u.expose.width, sw);
			ev->height	= sw16(event->u.expose.height, sw);
			ev->count	= sw16(event->u.expose.count, sw);
		}
		break;
	      case GraphicsExpose:
		{
		    register XGraphicsExposeEvent *ev =
			(XGraphicsExposeEvent *) re;
		    ev->drawable	= sw32(event->u.graphicsExposure.drawable, sw);
		    ev->x		= sw16(event->u.graphicsExposure.x, sw);
		    ev->y		= sw16(event->u.graphicsExposure.y, sw);
		    ev->width		= sw16(event->u.graphicsExposure.width, sw);
		    ev->height		= sw16(event->u.graphicsExposure.height, sw);
		    ev->count		= sw16(event->u.graphicsExposure.count, sw);
		    ev->major_code	= event->u.graphicsExposure.majorEvent;
		    ev->minor_code	= sw16(event->u.graphicsExposure.minorEvent, sw);
		}
		break;
	      case NoExpose:
		{
		    register XNoExposeEvent *ev = (XNoExposeEvent *) re;
		    ev->drawable	= sw32(event->u.noExposure.drawable, sw);
		    ev->major_code	= event->u.noExposure.majorEvent;
		    ev->minor_code	= sw16(event->u.noExposure.minorEvent, sw);
		}
		break;
	      case VisibilityNotify:
		{
		    register XVisibilityEvent *ev = (XVisibilityEvent *) re;
		    ev->window		= sw32(event->u.visibility.window, sw);
		    ev->state		= event->u.visibility.state;
		}
		break;
	      case CreateNotify:
		{
		    register XCreateWindowEvent *ev =
			 (XCreateWindowEvent *) re;
		    ev->window		= sw32(event->u.createNotify.window, sw);
		    ev->parent		= sw32(event->u.createNotify.parent, sw);
		    ev->x		= cvtINT16toInt(sw16(event->u.createNotify.x, sw));
		    ev->y		= cvtINT16toInt(sw16(event->u.createNotify.y, sw));
		    ev->width		= sw16(event->u.createNotify.width, sw);
		    ev->height		= sw16(event->u.createNotify.height, sw);
		    ev->border_width	= sw16(event->u.createNotify.borderWidth, sw);
		    ev->override_redirect	= event->u.createNotify.override;
		}
		break;
	      case DestroyNotify:
		{
		    register XDestroyWindowEvent *ev =
				(XDestroyWindowEvent *) re;
		    ev->window		= sw32(event->u.destroyNotify.window, sw);
		    ev->event		= sw32(event->u.destroyNotify.event, sw);
		}
		break;
	      case UnmapNotify:
		{
		    register XUnmapEvent *ev = (XUnmapEvent *) re;
		    ev->window		= sw32(event->u.unmapNotify.window, sw);
		    ev->event		= sw32(event->u.unmapNotify.event, sw);
		    ev->from_configure	= event->u.unmapNotify.fromConfigure;
		}
		break;
	      case MapNotify:
		{
		    register XMapEvent *ev = (XMapEvent *) re;
		    ev->window		= sw32(event->u.mapNotify.window, sw);
		    ev->event		= sw32(event->u.mapNotify.event, sw);
		    ev->override_redirect	= event->u.mapNotify.override;
		}
		break;
	      case MapRequest:
		{
		    register XMapRequestEvent *ev = (XMapRequestEvent *) re;
		    ev->window		= sw32(event->u.mapRequest.window, sw);
		    ev->parent		= sw32(event->u.mapRequest.parent, sw);
		}
		break;
	      case ReparentNotify:
		{
		    register XReparentEvent *ev = (XReparentEvent *) re;
		    ev->event		= sw32(event->u.reparent.event, sw);
		    ev->window		= sw32(event->u.reparent.window, sw);
		    ev->parent		= sw32(event->u.reparent.parent, sw);
		    ev->x		= cvtINT16toInt(sw16(event->u.reparent.x, sw));
		    ev->y		= cvtINT16toInt(sw16(event->u.reparent.y, sw));
		    ev->override_redirect	= event->u.reparent.override;
		}
		break;
	      case ConfigureNotify:
		{
		    register XConfigureEvent *ev = (XConfigureEvent *) re;
		    ev->event	= sw32(event->u.configureNotify.event, sw);
		    ev->window	= sw32(event->u.configureNotify.window, sw);
		    ev->above	= sw32(event->u.configureNotify.aboveSibling, sw);
		    ev->x	= cvtINT16toInt(sw16(event->u.configureNotify.x, sw));
		    ev->y	= cvtINT16toInt(sw16(event->u.configureNotify.y, sw));
		    ev->width	= sw16(event->u.configureNotify.width, sw);
		    ev->height	= sw16(event->u.configureNotify.height, sw);
		    ev->border_width  = sw16(event->u.configureNotify.borderWidth, sw);
		    ev->override_redirect = event->u.configureNotify.override;
		}
		break;
	      case ConfigureRequest:
		{
		    register XConfigureRequestEvent *ev =
		        (XConfigureRequestEvent *) re;
		    ev->window		= sw32(event->u.configureRequest.window, sw);
		    ev->parent		= sw32(event->u.configureRequest.parent, sw);
		    ev->above		= sw32(event->u.configureRequest.sibling, sw);
		    ev->x		= cvtINT16toInt(sw16(event->u.configureRequest.x, sw));
		    ev->y		= cvtINT16toInt(sw16(event->u.configureRequest.y, sw));
		    ev->width		= sw16(event->u.configureRequest.width, sw);
		    ev->height		= sw16(event->u.configureRequest.height, sw);
		    ev->border_width	= sw16(event->u.configureRequest.borderWidth, sw);
		    ev->value_mask	= sw16(event->u.configureRequest.valueMask, sw);
		    ev->detail  	= event->u.u.detail;
		}
		break;
	      case GravityNotify:
		{
		    register XGravityEvent *ev = (XGravityEvent *) re;
		    ev->window		= sw32(event->u.gravity.window, sw);
		    ev->event		= sw32(event->u.gravity.event, sw);
		    ev->x		= cvtINT16toInt(sw16(event->u.gravity.x, sw));
		    ev->y		= cvtINT16toInt(sw16(event->u.gravity.y, sw));
		}
		break;
	      case ResizeRequest:
		{
		    register XResizeRequestEvent *ev =
			(XResizeRequestEvent *) re;
		    ev->window		= sw32(event->u.resizeRequest.window, sw);
		    ev->width		= sw16(event->u.resizeRequest.width, sw);
		    ev->height		= sw16(event->u.resizeRequest.height, sw);
		}
		break;
	      case CirculateNotify:
		{
		    register XCirculateEvent *ev = (XCirculateEvent *) re;
		    ev->window		= sw32(event->u.circulate.window, sw);
		    ev->event		= sw32(event->u.circulate.event, sw);
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case CirculateRequest:
		{
		    register XCirculateRequestEvent *ev =
		        (XCirculateRequestEvent *) re;
		    ev->window		= sw32(event->u.circulate.window, sw);
		    ev->parent		= sw32(event->u.circulate.event, sw);
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case PropertyNotify:
		{
		    register XPropertyEvent *ev = (XPropertyEvent *) re;
		    ev->window		= sw32(event->u.property.window, sw);
		    ev->atom		= sw32(event->u.property.atom, sw);
		    ev->time		= sw32(event->u.property.time, sw);
		    ev->state		= event->u.property.state;
		}
		break;
	      case SelectionClear:
		{
		    register XSelectionClearEvent *ev =
			 (XSelectionClearEvent *) re;
		    ev->window		= sw32(event->u.selectionClear.window, sw);
		    ev->selection	= sw32(event->u.selectionClear.atom, sw);
		    ev->time		= sw32(event->u.selectionClear.time, sw);
		}
		break;
	      case SelectionRequest:
		{
		    register XSelectionRequestEvent *ev =
		        (XSelectionRequestEvent *) re;
		    ev->owner		= sw32(event->u.selectionRequest.owner, sw);
		    ev->requestor	= sw32(event->u.selectionRequest.requestor, sw);
		    ev->selection	= sw32(event->u.selectionRequest.selection, sw);
		    ev->target		= sw32(event->u.selectionRequest.target, sw);
		    ev->property	= sw32(event->u.selectionRequest.property, sw);
		    ev->time		= sw32(event->u.selectionRequest.time, sw);
		}
		break;
	      case SelectionNotify:
		{
		    register XSelectionEvent *ev = (XSelectionEvent *) re;
		    ev->requestor	= sw32(event->u.selectionNotify.requestor, sw);
		    ev->selection	= sw32(event->u.selectionNotify.selection, sw);
		    ev->target		= sw32(event->u.selectionNotify.target, sw);
		    ev->property	= sw32(event->u.selectionNotify.property, sw);
		    ev->time		= sw32(event->u.selectionNotify.time, sw);
		}
		break;
	      case ColormapNotify:
		{
		    register XColormapEvent *ev = (XColormapEvent *) re;
		    ev->window		= sw32(event->u.colormap.window, sw);
		    ev->colormap	= sw32(event->u.colormap.colormap, sw);
		    ev->new		= event->u.colormap.new;
		    ev->state		= event->u.colormap.state;
	        }
		break;
	      case ClientMessage:
		{
		   register int i;
		   register XClientMessageEvent *ev
		   			= (XClientMessageEvent *) re;
		   ev->window		= sw32(event->u.clientMessage.window, sw);
		   ev->format		= event->u.u.detail;
		   switch (ev->format) {
			case 8:
			   ev->message_type = sw32(event->u.clientMessage.u.b.type, sw);
			   for (i = 0; i < 20; i++)
			     ev->data.b[i] = event->u.clientMessage.u.b.bytes[i];
			   break;
			case 16:
			   ev->message_type = sw32(event->u.clientMessage.u.s.type, sw);
			   ev->data.s[0] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts0, sw));
			   ev->data.s[1] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts1, sw));
			   ev->data.s[2] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts2, sw));
			   ev->data.s[3] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts3, sw));
			   ev->data.s[4] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts4, sw));
			   ev->data.s[5] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts5, sw));
			   ev->data.s[6] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts6, sw));
			   ev->data.s[7] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts7, sw));
			   ev->data.s[8] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts8, sw));
			   ev->data.s[9] = cvtINT16toShort(sw16(event->u.clientMessage.u.s.shorts9, sw));
			   break;
			case 32:
			   ev->message_type = sw32(event->u.clientMessage.u.l.type, sw);
			   ev->data.l[0] = cvtINT32toLong(sw32(event->u.clientMessage.u.l.longs0, sw));
			   ev->data.l[1] = cvtINT32toLong(sw32(event->u.clientMessage.u.l.longs1, sw));
			   ev->data.l[2] = cvtINT32toLong(sw32(event->u.clientMessage.u.l.longs2, sw));
			   ev->data.l[3] = cvtINT32toLong(sw32(event->u.clientMessage.u.l.longs3, sw));
			   ev->data.l[4] = cvtINT32toLong(sw32(event->u.clientMessage.u.l.longs4, sw));
			   break;
			default: /* XXX should never occur */
				break;
		    }
	        }
		break;
	      case MappingNotify:
		{
		   register XMappingEvent *ev = (XMappingEvent *)re;
		   ev->window		= 0;
		   ev->first_keycode 	= event->u.mappingNotify.firstKeyCode;
		   ev->request 		= event->u.mappingNotify.request;
		   ev->count 		= event->u.mappingNotify.count;
		}
		break;
	      default:
		return(False);
	}
	return(True);
}
