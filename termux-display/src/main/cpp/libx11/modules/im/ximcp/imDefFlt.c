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

  Author: Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xutil.h"
#include "Xlcint.h"
#include "Ximint.h"

static long
_XimTriggerCheck(
    Xim			 im,
    XKeyEvent		*ev,
    INT32		 len,
    CARD32		*keylist)
{
    register long	 i;
    KeySym	 	 keysym;
    CARD32	 	 buf32[BUFSIZE/4];
    char	 	*buf = (char *)buf32;
    int			 modifier;
    int			 modifier_mask;
    CARD32		 min_len = sizeof(CARD32)   /* sizeof keysym */
				 + sizeof(CARD32)   /* sizeof modifier */
				 + sizeof(CARD32);  /* sizeof modifier mask */

    XLookupString(ev, buf, BUFSIZE, &keysym, NULL);
    if (!keysym)
	return -1;

    for (i = 0; len >= min_len; i += 3, len -= min_len) {
	modifier      = keylist[i + 1];
	modifier_mask = keylist[i + 2];
	if (((KeySym)keylist[i] == keysym)
	 && ((ev->state & modifier_mask) == modifier))
	    return i;
    }
    return -1;
}

static long
_XimTriggerOnCheck(
    Xim		 im,
    XKeyEvent	*ev)
{
    return _XimTriggerCheck(im, ev, (INT32)im->private.proto.im_onkeylist[0],
					&im->private.proto.im_onkeylist[1]);
}

static long
_XimTriggerOffCheck(
    Xim		 im,
    XKeyEvent	*ev)
{
    return _XimTriggerCheck(im, ev, (INT32)im->private.proto.im_offkeylist[0],
					&im->private.proto.im_offkeylist[1]);
}

static Bool
_XimOnKeysCheck(
    Xic		 ic,
    XKeyEvent	*ev)
{
    Xim		 im = (Xim)ic->core.im;
    long	 idx;

    if (IS_DYNAMIC_EVENT_FLOW(ic->core.im) &&
	    im->private.proto.im_onkeylist &&
	    im->private.proto.im_onkeylist[0]) {
	if ((idx = _XimTriggerOnCheck(im, ev)) >= 0) {
	    (void)_XimTriggerNotify(im, ic, 0, (CARD32)idx); /* Trigger on */
	    return True;
	}
    }
    return False;
}

static Bool
_XimOffKeysCheck(
    Xic		 ic,
    XKeyEvent	*ev)
{
    Xim		 im = (Xim)ic->core.im;
    long	 idx;

    if (IS_DYNAMIC_EVENT_FLOW(ic->core.im) &&
	    im->private.proto.im_offkeylist &&
	    im->private.proto.im_offkeylist[0]) {
	if ((idx = _XimTriggerOffCheck(im, ev)) >= 0) {
	    _XimTriggerNotify(im, ic, 1, (CARD32)idx); /* Trigger off */
	    return True;
	}
    }
    return False;
}

static void
_XimPendingFilter(
    Xic	 	 ic)
{
    Xim          im = (Xim)ic->core.im;

    if (IS_NEED_SYNC_REPLY(im)) {
	(void)_XimProcSyncReply(im, ic);
	UNMARK_NEED_SYNC_REPLY(im);
    }
    return;
}

static Bool
_XimProtoKeypressFilter(
    Xic		 ic,
    XKeyEvent	*ev)
{
    Xim		im = (Xim)ic->core.im;

    if (IS_FABRICATED(im)) {
	_XimPendingFilter(ic);
	UNMARK_FABRICATED(im);
	return NOTFILTERD;
    }

    if (IS_NEGLECT_EVENT(ic, KeyPressMask))
	return FILTERD;

#ifdef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
		    return NOTFILTERD;
		}
	    } else {
		return NOTFILTERD;
	    }
	} else {
	    return NOTFILTERD;
	}
    }
#else
    if (!IS_IC_CONNECTED(ic))
	return NOTFILTERD;
#endif /* XIM_CONNECTABLE */

    if (!IS_FORWARD_EVENT(ic, KeyPressMask)) {
	if (_XimOnKeysCheck(ic, ev))
	    return FILTERD;
	return NOTFILTERD;
    }
    if (_XimOffKeysCheck(ic, ev))
	return FILTERD;

    if (_XimForwardEvent(ic, (XEvent *)ev,
				IS_SYNCHRONOUS_EVENT(ic, KeyPressMask)))
	return FILTERD;

    return NOTFILTERD;
}

static Bool
_XimFilterKeypress(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 client_data)
{
    return _XimProtoKeypressFilter((Xic)client_data, (XKeyEvent *)ev );
}

static Bool
_XimProtoKeyreleaseFilter(
    Xic		 ic,
    XKeyEvent	*ev)
{
    Xim		im = (Xim)ic->core.im;

    if (IS_FABRICATED(im)) {
	_XimPendingFilter(ic);
	UNMARK_FABRICATED(im);
	return NOTFILTERD;
    }

    if (IS_NEGLECT_EVENT(ic, KeyReleaseMask))
	return FILTERD;

#ifdef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
		    return NOTFILTERD;
		}
	    } else {
		return NOTFILTERD;
	    }
	} else {
	    return NOTFILTERD;
	}
    }
#else
    if (!IS_IC_CONNECTED(ic))
	return NOTFILTERD;
#endif /* XIM_CONNECTABLE */

    if (!IS_FORWARD_EVENT(ic, KeyReleaseMask)) {
	if (_XimOnKeysCheck(ic, ev))
	    return FILTERD;
	return NOTFILTERD;
    }
    if (_XimOffKeysCheck(ic, ev))
	    return FILTERD;

    if (_XimForwardEvent(ic, (XEvent *)ev,
				IS_SYNCHRONOUS_EVENT(ic, KeyPressMask)))
	return FILTERD;

    return NOTFILTERD;
}

static Bool
_XimFilterKeyrelease(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 client_data)
{
    return _XimProtoKeyreleaseFilter((Xic)client_data, (XKeyEvent *)ev);
}

static void
_XimRegisterKeyPressFilter(
    Xic		 ic)
{
    if (ic->core.focus_window) {
	if (!(ic->private.proto.registed_filter_event & KEYPRESS_MASK)) {
	    _XRegisterFilterByType (ic->core.im->core.display,
				    ic->core.focus_window,
				    KeyPress, KeyPress,
				    _XimFilterKeypress,
				    (XPointer)ic);
	    ic->private.proto.registed_filter_event |= KEYPRESS_MASK;
	}
    }
    return;
}

static void
_XimRegisterKeyReleaseFilter(
    Xic		 ic)
{
    if (ic->core.focus_window) {
	if (!(ic->private.proto.registed_filter_event & KEYRELEASE_MASK)) {
	    _XRegisterFilterByType (ic->core.im->core.display,
				    ic->core.focus_window,
				    KeyRelease, KeyRelease,
				    _XimFilterKeyrelease,
				    (XPointer)ic);
	    ic->private.proto.registed_filter_event |= KEYRELEASE_MASK;
	}
    }
    return;
}

static void
_XimUnregisterKeyPressFilter(
    Xic		 ic)
{
    if (ic->core.focus_window) {
	if (ic->private.proto.registed_filter_event & KEYPRESS_MASK) {
	    _XUnregisterFilter (ic->core.im->core.display,
				ic->core.focus_window,
				_XimFilterKeypress,
				(XPointer)ic);
	    ic->private.proto.registed_filter_event &= ~KEYPRESS_MASK;
	}
    }
    return;
}

static void
_XimUnregisterKeyReleaseFilter(
    Xic		 ic)
{
    if (ic->core.focus_window) {
	if (ic->private.proto.registed_filter_event & KEYRELEASE_MASK) {
	    _XUnregisterFilter (ic->core.im->core.display,
				ic->core.focus_window,
				_XimFilterKeyrelease,
				(XPointer)ic);
	    ic->private.proto.registed_filter_event &= ~KEYRELEASE_MASK;
	}
    }
    return;
}

void
_XimRegisterFilter(
    Xic		 ic)
{
    _XimRegisterKeyPressFilter(ic);
    if (IS_FORWARD_EVENT(ic, KeyReleaseMask))
	_XimRegisterKeyReleaseFilter(ic);
    return;
}

void
_XimUnregisterFilter(
    Xic		 ic)
{
    _XimUnregisterKeyPressFilter(ic);
    _XimUnregisterKeyReleaseFilter(ic);
    return;
}

void
_XimReregisterFilter(
    Xic		 ic)
{
    if (IS_FORWARD_EVENT(ic, KeyReleaseMask))
	_XimRegisterKeyReleaseFilter(ic);
    else
	_XimUnregisterKeyReleaseFilter(ic);

    return;
}

static Bool
_XimFilterServerDestroy(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 client_data)
{
    Xim			 im = (Xim)client_data;

    if (ev->type == DestroyNotify) {
	UNMARK_SERVER_CONNECTED(im);
#ifdef XIM_CONNECTABLE
	if (!IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im)) {
	    _XimServerReconectableDestroy();
	    return True;
	}
#endif /* XIM_CONNECTABLE */
	_XimServerDestroy(im);
    }
    return True;
}

void
_XimRegisterServerFilter(
    Xim		 im)
{
    if (im->private.proto.im_window) {
	if (!(im->private.proto.registed_filter_event & DESTROYNOTIFY_MASK)) {
	    _XRegisterFilterByMask(im->core.display,
		    im->private.proto.im_window,
		    StructureNotifyMask,
		    _XimFilterServerDestroy,
		    (XPointer)im);
	    XSelectInput(im->core.display, im->private.proto.im_window,
			 StructureNotifyMask);
	    im->private.proto.registed_filter_event |= DESTROYNOTIFY_MASK;
	}
    }
    return;
}

void
_XimUnregisterServerFilter(
    Xim		 im)
{
    if (im->private.proto.im_window) {
	if (im->private.proto.registed_filter_event & DESTROYNOTIFY_MASK) {
	    _XUnregisterFilter(im->core.display,
		    im->private.proto.im_window,
		    _XimFilterServerDestroy,
		    (XPointer)im);
	    im->private.proto.registed_filter_event &= ~DESTROYNOTIFY_MASK;
	}
    }
    return;
}

