
/*

Copyright 1985, 1986, 1987, 1998  The Open Group

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

/*
 *	XEvToWire.c - Internal support routines for the C subroutine
 *	interface library (Xlib) to the X Window System Protocol V11.0.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

/*
 * Reformat an XEvent structure to a wire event of the right type.
 * Return True on success.  If the type is unrecognized, return what
 * _XUnknownNativeEvent returns (i.e., False).
 */
Status
_XEventToWire(
    register Display *dpy,
    register XEvent *re,        /* in: from */
    register xEvent *event)     /* out: to */
{
	switch (event->u.u.type = re->type) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent *ev = (XKeyEvent*) re;
			event->u.keyButtonPointer.root	= ev->root;
			event->u.keyButtonPointer.event	 = ev->window;
			event->u.keyButtonPointer.child  = ev->subwindow;
			event->u.keyButtonPointer.time	 = ev->time;
			event->u.keyButtonPointer.eventX = ev->x ;
			event->u.keyButtonPointer.eventY = ev->y ;
			event->u.keyButtonPointer.rootX	 = ev->x_root;
			event->u.keyButtonPointer.rootY  = ev->y_root;
			event->u.keyButtonPointer.state  = ev->state;
			event->u.keyButtonPointer.sameScreen = ev->same_screen;
			event->u.u.detail = ev->keycode;
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent *ev =  (XButtonEvent *) re;
			event->u.keyButtonPointer.root	 = ev->root;
			event->u.keyButtonPointer.event	 = ev->window;
			event->u.keyButtonPointer.child	 = ev->subwindow;
			event->u.keyButtonPointer.time	 = ev->time;
			event->u.keyButtonPointer.eventX = ev->x;
			event->u.keyButtonPointer.eventY = ev->y;
			event->u.keyButtonPointer.rootX	 = ev->x_root;
			event->u.keyButtonPointer.rootY	 = ev->y_root;
			event->u.keyButtonPointer.state	 = ev->state;
			event->u.keyButtonPointer.sameScreen	= ev->same_screen;
			event->u.u.detail		= ev->button;
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent *ev =   (XMotionEvent *)re;
			event->u.keyButtonPointer.root	= ev->root;
			event->u.keyButtonPointer.event	= ev->window;
			event->u.keyButtonPointer.child	= ev->subwindow;
			event->u.keyButtonPointer.time	= ev->time;
			event->u.keyButtonPointer.eventX= ev->x;
			event->u.keyButtonPointer.eventY= ev->y;
			event->u.keyButtonPointer.rootX	= ev->x_root;
			event->u.keyButtonPointer.rootY	= ev->y_root;
			event->u.keyButtonPointer.state	= ev->state;
			event->u.keyButtonPointer.sameScreen= ev->same_screen;
			event->u.u.detail		= ev->is_hint;
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent *ev   = (XCrossingEvent *) re;
			event->u.enterLeave.root	= ev->root;
			event->u.enterLeave.event	= ev->window;
			event->u.enterLeave.child	= ev->subwindow;
			event->u.enterLeave.time	= ev->time;
			event->u.enterLeave.eventX	= ev->x;
			event->u.enterLeave.eventY	= ev->y;
			event->u.enterLeave.rootX	= ev->x_root;
			event->u.enterLeave.rootY	= ev->y_root;
			event->u.enterLeave.state	= ev->state;
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
			event->u.focus.window	= ev->window;
			event->u.focus.mode	= ev->mode;
			event->u.u.detail	= ev->detail;
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent *ev = (XKeymapEvent *) re;
			memcpy ((char *)(((xKeymapEvent *) event)->map),
				&ev->key_vector[1],
				sizeof (((xKeymapEvent *) event)->map));
		}
		break;
	      case Expose:
		{
			register XExposeEvent *ev = (XExposeEvent *) re;
			event->u.expose.window		= ev->window;
			event->u.expose.x		= ev->x;
			event->u.expose.y		= ev->y;
			event->u.expose.width		= ev->width;
			event->u.expose.height		= ev->height;
			event->u.expose.count		= ev->count;
		}
		break;
	      case GraphicsExpose:
		{
		    register XGraphicsExposeEvent *ev =
			(XGraphicsExposeEvent *) re;
		    event->u.graphicsExposure.drawable	= ev->drawable;
		    event->u.graphicsExposure.x		= ev->x;
		    event->u.graphicsExposure.y		= ev->y;
		    event->u.graphicsExposure.width	= ev->width;
		    event->u.graphicsExposure.height	= ev->height;
		    event->u.graphicsExposure.count	= ev->count;
		    event->u.graphicsExposure.majorEvent= ev->major_code;
		    event->u.graphicsExposure.minorEvent= ev->minor_code;
		}
		break;
	      case NoExpose:
		{
		    register XNoExposeEvent *ev = (XNoExposeEvent *) re;
		    event->u.noExposure.drawable	= ev->drawable;
		    event->u.noExposure.majorEvent	= ev->major_code;
		    event->u.noExposure.minorEvent	= ev->minor_code;
		}
		break;
	      case VisibilityNotify:
		{
		    register XVisibilityEvent *ev = (XVisibilityEvent *) re;
		    event->u.visibility.window		= ev->window;
		    event->u.visibility.state		= ev->state;
		}
		break;
	      case CreateNotify:
		{
		    register XCreateWindowEvent *ev =
			 (XCreateWindowEvent *) re;
		    event->u.createNotify.window	= ev->window;
		    event->u.createNotify.parent	= ev->parent;
		    event->u.createNotify.x		= ev->x;
		    event->u.createNotify.y		= ev->y;
		    event->u.createNotify.width		= ev->width;
		    event->u.createNotify.height	= ev->height;
		    event->u.createNotify.borderWidth	= ev->border_width;
		    event->u.createNotify.override	= ev->override_redirect;
		}
		break;
	      case DestroyNotify:
		{
		    register XDestroyWindowEvent *ev =
				(XDestroyWindowEvent *) re;
		    event->u.destroyNotify.window	= ev->window;
		    event->u.destroyNotify.event	= ev->event;
		}
		break;
	      case UnmapNotify:
		{
		    register XUnmapEvent *ev = (XUnmapEvent *) re;
		    event->u.unmapNotify.window	= ev->window;
		    event->u.unmapNotify.event	= ev->event;
		    event->u.unmapNotify.fromConfigure	= ev->from_configure;
		}
		break;
	      case MapNotify:
		{
		    register XMapEvent *ev = (XMapEvent *) re;
		    event->u.mapNotify.window	= ev->window;
		    event->u.mapNotify.event	= ev->event;
		    event->u.mapNotify.override	= ev->override_redirect;
		}
		break;
	      case MapRequest:
		{
		    register XMapRequestEvent *ev = (XMapRequestEvent *) re;
		    event->u.mapRequest.window	= ev->window;
		    event->u.mapRequest.parent	= ev->parent;
		}
		break;
	      case ReparentNotify:
		{
		    register XReparentEvent *ev = (XReparentEvent *) re;
		    event->u.reparent.window	= ev->window;
		    event->u.reparent.event	= ev->event;
		    event->u.reparent.parent	= ev->parent;
		    event->u.reparent.x		= ev->x;
		    event->u.reparent.y		= ev->y;
		    event->u.reparent.override	= ev->override_redirect;
		}
		break;
	      case ConfigureNotify:
		{
		    register XConfigureEvent *ev = (XConfigureEvent *) re;
		    event->u.configureNotify.window	= ev->window;
		    event->u.configureNotify.event	= ev->event;
		    event->u.configureNotify.aboveSibling	= ev->above;
		    event->u.configureNotify.x		= ev->x;
		    event->u.configureNotify.y		= ev->y;
		    event->u.configureNotify.width	= ev->width;
		    event->u.configureNotify.height	= ev->height;
		    event->u.configureNotify.borderWidth= ev->border_width;
		    event->u.configureNotify.override	= ev->override_redirect;
		}
		break;
	      case ConfigureRequest:
		{
		    register XConfigureRequestEvent *ev =
		        (XConfigureRequestEvent *) re;
		    event->u.configureRequest.window	= ev->window;
		    event->u.configureRequest.parent	= ev->parent;
		    event->u.configureRequest.sibling   = ev->above;
		    event->u.configureRequest.x		= ev->x;
		    event->u.configureRequest.y		= ev->y;
		    event->u.configureRequest.width	= ev->width;
		    event->u.configureRequest.height	= ev->height;
		    event->u.configureRequest.borderWidth= ev->border_width;
		    event->u.configureRequest.valueMask= ev->value_mask;
		    event->u.u.detail 			= ev->detail;
		}
		break;
	      case GravityNotify:
		{
		    register XGravityEvent *ev  = (XGravityEvent *) re;
		    event->u.gravity.window	= ev->window;
		    event->u.gravity.event	= ev->event;
		    event->u.gravity.x		= ev->x;
		    event->u.gravity.y		= ev->y;
		}
		break;
	      case ResizeRequest:
		{
		    register XResizeRequestEvent *ev =
			(XResizeRequestEvent *) re;
		    event->u.resizeRequest.window	= ev->window;
		    event->u.resizeRequest.width	= ev->width;
		    event->u.resizeRequest.height	= ev->height;
		}
		break;
	      case CirculateNotify:
		{
		    register XCirculateEvent *ev = (XCirculateEvent *) re;
		    event->u.circulate.window		= ev->window;
		    event->u.circulate.event		= ev->event;
		    event->u.circulate.place		= ev->place;
		}
		break;
	      case CirculateRequest:
		{
		    register XCirculateRequestEvent *ev =
		        (XCirculateRequestEvent *) re;
		    event->u.circulate.window		= ev->window;
		    event->u.circulate.event		= ev->parent;
		    event->u.circulate.place		= ev->place;
		}
		break;
	      case PropertyNotify:
		{
		    register XPropertyEvent *ev = (XPropertyEvent *) re;
		    event->u.property.window		= ev->window;
		    event->u.property.atom		= ev->atom;
		    event->u.property.time		= ev->time;
		    event->u.property.state		= ev->state;
		}
		break;
	      case SelectionClear:
		{
		    register XSelectionClearEvent *ev =
			 (XSelectionClearEvent *) re;
		    event->u.selectionClear.window	= ev->window;
		    event->u.selectionClear.atom	= ev->selection;
		    event->u.selectionClear.time	= ev->time;
		}
		break;
	      case SelectionRequest:
		{
		    register XSelectionRequestEvent *ev =
		        (XSelectionRequestEvent *) re;
		    event->u.selectionRequest.owner	= ev->owner;
		    event->u.selectionRequest.requestor	= ev->requestor;
		    event->u.selectionRequest.selection	= ev->selection;
		    event->u.selectionRequest.target	= ev->target;
		    event->u.selectionRequest.property	= ev->property;
		    event->u.selectionRequest.time	= ev->time;
		}
		break;
	      case SelectionNotify:
		{
		    register XSelectionEvent *ev = (XSelectionEvent *) re;
		    event->u.selectionNotify.requestor	= ev->requestor;
		    event->u.selectionNotify.selection	= ev->selection;
		    event->u.selectionNotify.target	= ev->target;
		    event->u.selectionNotify.property	= ev->property;
		    event->u.selectionNotify.time	= ev->time;
		}
		break;
	      case ColormapNotify:
		{
		    register XColormapEvent *ev = (XColormapEvent *) re;
		    event->u.colormap.window		= ev->window;
		    event->u.colormap.colormap		= ev->colormap;
		    event->u.colormap.new		= ev->new;
		    event->u.colormap.state		= ev->state;
	        }
		break;
	      case ClientMessage:
		{
		   register int i;
		   register XClientMessageEvent *ev
		   			= (XClientMessageEvent *) re;
		   event->u.clientMessage.window = ev->window;
		   event->u.u.detail		 = ev->format;
		   switch (ev->format) {
			case 8:
			  event->u.clientMessage.u.b.type   = ev->message_type;
			  for (i = 0; i < 20; i++)
			   event->u.clientMessage.u.b.bytes[i] = ev->data.b[i];
			  break;
			case 16:
			  event->u.clientMessage.u.s.type   = ev->message_type;
			  event->u.clientMessage.u.s.shorts0   = ev->data.s[0];
			  event->u.clientMessage.u.s.shorts1   = ev->data.s[1];
			  event->u.clientMessage.u.s.shorts2   = ev->data.s[2];
			  event->u.clientMessage.u.s.shorts3   = ev->data.s[3];
			  event->u.clientMessage.u.s.shorts4   = ev->data.s[4];
			  event->u.clientMessage.u.s.shorts5   = ev->data.s[5];
			  event->u.clientMessage.u.s.shorts6   = ev->data.s[6];
			  event->u.clientMessage.u.s.shorts7   = ev->data.s[7];
			  event->u.clientMessage.u.s.shorts8   = ev->data.s[8];
			  event->u.clientMessage.u.s.shorts9   = ev->data.s[9];
			  break;
			case 32:
			  event->u.clientMessage.u.l.type   = ev->message_type;
			  event->u.clientMessage.u.l.longs0   = ev->data.l[0];
			  event->u.clientMessage.u.l.longs1   = ev->data.l[1];
			  event->u.clientMessage.u.l.longs2   = ev->data.l[2];
			  event->u.clientMessage.u.l.longs3   = ev->data.l[3];
			  event->u.clientMessage.u.l.longs4   = ev->data.l[4];
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
		return(_XUnknownNativeEvent(dpy, re, event));
	}
	return(1);
}
