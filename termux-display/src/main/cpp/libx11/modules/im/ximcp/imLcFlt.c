/******************************************************************

              Copyright 1992 by Fuji Xerox Co., Ltd.
              Copyright 1992, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Fuji Xerox,
FUJITSU LIMITED not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission. Fuji Xerox, FUJITSU LIMITED make no representations
about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJI XEROX, FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJI XEROX,
FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author   : Kazunori Nishihara	Fuji Xerox
  Modifier : Takashi Fujiwara   FUJITSU LIMITED
                                fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/keysym.h>
#include "Xlcint.h"
#include "Ximint.h"

Bool
_XimLocalFilter(Display *d, Window w, XEvent *ev, XPointer client_data)
{
    Xic		 ic = (Xic)client_data;
    KeySym	 keysym;
    static char	 buf[256];
    static unsigned prevcode = 0, prevstate = 0;
    unsigned    currstate;
    DefTree	*b = ic->private.local.base.tree;
    DTIndex	 t;
    Bool	 anymodifier = False;
    unsigned char braillePattern = 0;

    if(ev->xkey.keycode == 0)
	return (False);

    XLookupString((XKeyEvent *)ev, buf, sizeof(buf), &keysym, NULL);

    if(keysym >= XK_braille_dot_1 && keysym <= XK_braille_dot_8) {
	if(ev->type == KeyPress) {
	    ic->private.local.brl_pressed |=
		1<<(keysym-XK_braille_dot_1);
	    return(True);
	} else {
	    if(!ic->private.local.brl_committing
		    || ev->xkey.time - ic->private.local.brl_release_start > 300) {
	    	ic->private.local.brl_committing = ic->private.local.brl_pressed;
		ic->private.local.brl_release_start = ev->xkey.time;
	    }
	    ic->private.local.brl_pressed &= ~(1<<(keysym-XK_braille_dot_1));
	    if(!ic->private.local.brl_pressed && ic->private.local.brl_committing) {
		/* Committed a braille pattern, let it go through compose tree */
		keysym = XK_braille_blank | ic->private.local.brl_committing;
		ev->type = KeyPress;
		braillePattern = ic->private.local.brl_committing;
		ic->private.local.brl_committing = 0;
	    } else {
	        return(True);
	    }
	}
    }

    if(((Xim)ic->core.im)->private.local.top == 0 )
	goto emit_braille;

    currstate = ev->xkey.state;
    if(ev->type == KeyPress) {
	prevcode = ev->xkey.keycode;
	prevstate = currstate;

	if(IsModifierKey(keysym))
	    return(False);
	prevcode = 0;
    } else {
	if(prevcode != ev->xkey.keycode)
	    return False;

	/* For lookup, we use the state at the time when the key was pressed, */
	/* because this state was not affected by the modifier that is mapped */
	/* to the key. */
	ev->xkey.state = prevstate;
	XLookupString((XKeyEvent *)ev, buf, sizeof(buf), &keysym, NULL);
    }

    for(t = ic->private.local.context; t; t = b[t].next) {
	if(IsModifierKey(b[t].keysym))
	    anymodifier = True;
	if(((ev->xkey.state & b[t].modifier_mask) == b[t].modifier) &&
	   (keysym == b[t].keysym))
	    break;
    }

    /* Restore the state */
    ev->xkey.state = currstate;

    if(t) { /* Matched */
	if(b[t].succession) { /* Intermediate */
	    ic->private.local.context = b[t].succession;
	    return (ev->type == KeyPress);
	} else { /* Terminate (reached to leaf) */
	    ic->private.local.composed = t;
	    ic->private.local.brl_committed = 0;
	    /* return back to client KeyPressEvent keycode == 0 */
	    ev->xkey.keycode = 0;
	    ev->xkey.type = KeyPress;
	    XPutBackEvent(d, ev);
	    if(prevcode){
		/* For modifier key releases, restore the event, as we do not */
		/* filter it.  */
		ev->xkey.type = KeyRelease;
		ev->xkey.keycode = prevcode;
	    }
	    /* initialize internal state for next key sequence */
	    ic->private.local.context = ((Xim)ic->core.im)->private.local.top;
	    return (ev->type == KeyPress);
	}
    } else { /* Unmatched */
	/* Unmatched modifier key releases abort matching only in the case that */
	/* there was any modifier that would have matched */
	if((ic->private.local.context == ((Xim)ic->core.im)->private.local.top) ||
	   (ev->type == KeyRelease && !anymodifier)) {
	    goto emit_braille;
	}
	/* Error (Sequence Unmatch occurred) */
	/* initialize internal state for next key sequence */
	ic->private.local.context = ((Xim)ic->core.im)->private.local.top;
	return (ev->type == KeyPress);
    }

emit_braille:
    if(braillePattern) {
	/* Braille pattern is not in compose tree, emit alone */
	ic->private.local.brl_committed = braillePattern;
	ic->private.local.composed = 0;
	ev->xkey.keycode = 0;
	_XPutBackEvent(d, ev);
	return(True);
    }
    return(False);
}
