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
#include "Xlcint.h"
#include "Ximint.h"

Xic
_XimICOfXICID(
    Xim		  im,
    XICID	  icid)
{
    Xic		  pic;

    for (pic = (Xic)im->core.ic_chain; pic; pic = (Xic)pic->core.next) {
	if (pic->private.proto.icid == icid)
	    return pic;
    }
    return (Xic)0;
}

static void
_XimProcIMSetEventMask(
    Xim		 im,
    XPointer	 buf)
{
    EVENTMASK	*buf_l = (EVENTMASK *)buf;

    im->private.proto.forward_event_mask     = buf_l[0];
    im->private.proto.synchronous_event_mask = buf_l[1];
    return;
}

static void
_XimProcICSetEventMask(
    Xic		 ic,
    XPointer	 buf)
{
    EVENTMASK	*buf_l = (EVENTMASK *)buf;

    ic->private.proto.forward_event_mask     = buf_l[0];
    ic->private.proto.synchronous_event_mask = buf_l[1];
    _XimReregisterFilter(ic);
    return;
}

Bool
_XimSetEventMaskCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID        imid = buf_s[0];
    XICID        icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if (imid == im->private.proto.imid) {
	if (icid) {
	    if (!(ic = _XimICOfXICID(im, icid)))
		return False;
	    _XimProcICSetEventMask(ic, (XPointer)&buf_s[2]);
	} else {
	    _XimProcIMSetEventMask(im, (XPointer)&buf_s[2]);
	}
	return True;
    }
    return False;
}

static Bool
_XimSyncCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic  = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];

    if ((major_opcode == XIM_SYNC_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
	return True;
    return False;
}

Bool
_XimSync(
    Xim		 im,
    Xic		 ic)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */

    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16);			/* sizeof icid */

    _XimSetHeader((XPointer)buf, XIM_SYNC, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimSyncCheck, (XPointer)ic);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(len);
	    ret_code = _XimRead(im, &len, preply, buf_size,
					_XimSyncCheck, (XPointer)ic);
	    if(ret_code != XIM_TRUE) {
		Xfree(preply);
		return False;
	    }
	}
    } else {
	return False;
    }
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if(reply != preply)
	    Xfree(preply);
	return False;
    }
    if(reply != preply)
	Xfree(preply);
    return True;
}

Bool
_XimProcSyncReply(
    Xim		 im,
    Xic		 ic)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */

    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16);			/* sizeof icid */

    _XimSetHeader((XPointer)buf, XIM_SYNC_REPLY, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    return True;
}

Bool
_XimRespSyncReply(
    Xic		 ic,
    BITMASK16	 mode)
{
    if (mode & XimSYNCHRONUS) /* SYNC Request */
	MARK_NEED_SYNC_REPLY(ic->core.im);

    return True;
}

Bool
_XimSyncCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID        imid = buf_s[0];
    XICID        icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if ((imid == im->private.proto.imid)
     && (ic = _XimICOfXICID(im, icid))) {
	(void)_XimProcSyncReply(im, ic);
	return True;
    }
    return False;
}

static INT16
_XimSetEventToWire(
    XEvent	*ev,
    xEvent	*event)
{
    if (!(_XimProtoEventToWire(ev, event, False)))
	return 0;
    event->u.u.sequenceNumber =
		((XAnyEvent *)ev)->serial & (unsigned long)0xffff;
    return sz_xEvent;
}

static Bool
_XimForwardEventCore(
    Xic		 ic,
    XEvent	*ev,
    Bool	 sync)
{
    Xim		 im = (Xim)ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;
    INT16	 len;

    bzero(buf32, sizeof(buf32)); /* valgrind noticed uninitialized memory use! */

    if (!(len = _XimSetEventToWire(ev, (xEvent *)&buf_s[4])))
	return False;				/* X event */

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */
    buf_s[2] = sync ? XimSYNCHRONUS : 0;	/* flag */
    buf_s[3] =
        (CARD16)((((XAnyEvent *)ev)->serial & ~((unsigned long)0xffff)) >> 16);
						/* serial number */

    len += sizeof(CARD16)			/* sizeof imid */
	 + sizeof(CARD16)			/* sizeof icid */
	 + sizeof(BITMASK16)			/* sizeof flag */
	 + sizeof(CARD16);			/* sizeof serila number */

    _XimSetHeader((XPointer)buf, XIM_FORWARD_EVENT, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);

    if (sync) {
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimSyncCheck, (XPointer)ic);
	if(ret_code == XIM_TRUE) {
	    preply = reply;
	} else if(ret_code == XIM_OVERFLOW) {
	    if(len <= 0) {
		preply = reply;
	    } else {
		buf_size = len;
		preply = Xmalloc(len);
		ret_code = _XimRead(im, &len, preply, buf_size,
					_XimSyncCheck, (XPointer)ic);
		if(ret_code != XIM_TRUE) {
		    Xfree(preply);
		    return False;
		}
	    }
	} else {
	    return False;
	}
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
	if (*((CARD8 *)preply) == XIM_ERROR) {
	    _XimProcError(im, 0, (XPointer)&buf_s[3]);
	    if(reply != preply)
		Xfree(preply);
	    return False;
	}
	if(reply != preply)
	    Xfree(preply);
    }
    return True;
}

Bool
_XimForwardEvent(
    Xic		 ic,
    XEvent	*ev,
    Bool	 sync)
{
#ifdef EXT_FORWARD
    if (((ev->type == KeyPress) || (ev->type == KeyRelease)))
	if (_XimExtForwardKeyEvent(ic, (XKeyEvent *)ev, sync))
	    return True;
#endif
    return _XimForwardEventCore(ic, ev, sync);
}

static void
_XimProcEvent(
    Display		*d,
    Xic			 ic,
    XEvent		*ev,
    CARD16		*buf)
{
    INT16	 serial = buf[0];
    xEvent	*xev = (xEvent *)&buf[1];

    _XimProtoWireToEvent(ev, xev, False);
    ev->xany.serial |= serial << 16;
    ev->xany.send_event = False;
    ev->xany.display = d;
    MARK_FABRICATED(ic->core.im);
    return;
}

static Bool
_XimForwardEventRecv(
    Xim		 im,
    Xic		 ic,
    XPointer	 buf)
{
    CARD16	*buf_s = (CARD16 *)buf;
    Display	*d = im->core.display;
    XEvent	 ev;

    _XimProcEvent(d, ic, &ev, &buf_s[1]);

    (void)_XimRespSyncReply(ic, buf_s[0]);

    XPutBackEvent(d, &ev);

    return True;
}

Bool
_XimForwardEventCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID        imid = buf_s[0];
    XICID        icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if ((imid == im->private.proto.imid)
     && (ic = _XimICOfXICID(im, icid))) {
	(void)_XimForwardEventRecv(im, ic, (XPointer)&buf_s[2]);
	return True;
    }
    return False;
}

static Bool
_XimRegisterTriggerkey(
    Xim			 im,
    XPointer		 buf)
{
    CARD32		*buf_l = (CARD32 *)buf;
    CARD32		 len;
    CARD32 		*key;

    if (IS_DYNAMIC_EVENT_FLOW(im))	/* already Dynamic event flow mode */
	return True;

    /*
     *  register onkeylist
     */

    len = buf_l[0];				/* length of on-keys */
    len += sizeof(INT32);			/* sizeof length of on-keys */

    if (!(key = Xmalloc(len))) {
	_XimError(im, 0, XIM_BadAlloc, (INT16)0, (CARD16)0, (char *)NULL);
	return False;
    }
    memcpy((char *)key, (char *)buf_l, len);
    im->private.proto.im_onkeylist = key;

    MARK_DYNAMIC_EVENT_FLOW(im);

    /*
     *  register offkeylist
     */

    buf_l = (CARD32 *)((char *)buf + len);
    len = buf_l[0];				/* length of off-keys */
    len += sizeof(INT32);			/* sizeof length of off-keys */

    if (!(key = Xmalloc(len))) {
	_XimError(im, 0, XIM_BadAlloc, (INT16)0, (CARD16)0, (char *)NULL);
	return False;
    }

    memcpy((char *)key, (char *)buf_l, len);
    im->private.proto.im_offkeylist = key;

    return True;
}

Bool
_XimRegisterTriggerKeysCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    Xim		 im = (Xim)call_data;

    (void )_XimRegisterTriggerkey(im, (XPointer)&buf_s[2]);
    return True;
}

EVENTMASK
_XimGetWindowEventmask(
    Xic		 ic)
{
    Xim			im = (Xim )ic->core.im;
    XWindowAttributes	atr;

    if (!XGetWindowAttributes(im->core.display, ic->core.focus_window, &atr))
	return 0;
    return (EVENTMASK)atr.your_event_mask;
}


static Bool
_XimTriggerNotifyCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic  = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];

    if ((major_opcode == XIM_TRIGGER_NOTIFY_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
	return True;
    return False;
}

Bool
_XimTriggerNotify(
    Xim		 im,
    Xic		 ic,
    int		 mode,
    CARD32	 idx)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    CARD32	*buf_l = (CARD32 *)&buf[XIM_HEADER_SIZE];
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;
    INT16	 len;
    EVENTMASK	 mask = _XimGetWindowEventmask(ic);

    buf_s[0] = im->private.proto.imid;	/* imid */
    buf_s[1] = ic->private.proto.icid;	/* icid */
    buf_l[1] = mode;			/* flag */
    buf_l[2] = idx;			/* index of keys list */
    buf_l[3] = mask;			/* select-event-mask */

    len = sizeof(CARD16)		/* sizeof imid */
	+ sizeof(CARD16)		/* sizeof icid */
	+ sizeof(CARD32)		/* sizeof flag */
	+ sizeof(CARD32)		/* sizeof index of key list */
	+ sizeof(EVENTMASK);		/* sizeof select-event-mask */

    _XimSetHeader((XPointer)buf, XIM_TRIGGER_NOTIFY, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
				_XimTriggerNotifyCheck, (XPointer)ic);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(len);
	    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
				_XimTriggerNotifyCheck, (XPointer)ic);
	    if(ret_code != XIM_TRUE) {
		Xfree(preply);
		return False;
	    }
	}
    } else {
	return False;
    }
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if(reply != preply)
	    Xfree(preply);
	return False;
    }
    if(reply != preply)
	Xfree(preply);
    return True;
}

static Bool
_XimRegCommitInfo(
    Xic			 ic,
    char		*string,
    int			 string_len,
    KeySym		*keysym,
    int			 keysym_len)
{
    XimCommitInfo	info;

    if (!(info = Xmalloc(sizeof(XimCommitInfoRec))))
	return False;
    info->string	= string;
    info->string_len	= string_len;
    info->keysym	= keysym;
    info->keysym_len	= keysym_len;
    info->next = ic->private.proto.commit_info;
    ic->private.proto.commit_info = info;
    return True;
}

static void
_XimUnregCommitInfo(
    Xic			ic)
{
    XimCommitInfo	info;

    if (!(info = ic->private.proto.commit_info))
	return;


    Xfree(info->string);
    Xfree(info->keysym);
    ic->private.proto.commit_info = info->next;
    Xfree(info);
    return;
}

void
_XimFreeCommitInfo(
    Xic			ic)
{
    while (ic->private.proto.commit_info)
	_XimUnregCommitInfo(ic);
    return;
}

static Bool
_XimProcKeySym(
    Xic			  ic,
    CARD32		  sym,
    KeySym		**xim_keysym,
    int			 *xim_keysym_len)
{
    Xim			 im = (Xim)ic->core.im;

    if (!(*xim_keysym = Xmalloc(sizeof(KeySym)))) {
	_XimError(im, ic, XIM_BadAlloc, (INT16)0, (CARD16)0, (char *)NULL);
	return False;
    }

    **xim_keysym = (KeySym)sym;
    *xim_keysym_len = 1;

    return True;
}

static Bool
_XimProcCommit(
    Xic		  ic,
    BYTE	 *buf,
    int		  len,
    char	**xim_string,
    int		 *xim_string_len)
{
    Xim		 im = (Xim)ic->core.im;
    char	*string;

    if (!(string = Xmalloc(len + 1))) {
	_XimError(im, ic, XIM_BadAlloc, (INT16)0, (CARD16)0, (char *)NULL);
	return False;
    }

    (void)memcpy(string, (char *)buf, len);
    string[len] = '\0';

    *xim_string = string;
    *xim_string_len = len;
    return True;
}

static Bool
_XimCommitRecv(
    Xim		 im,
    Xic		 ic,
    XPointer	 buf)
{
    CARD16	*buf_s = (CARD16 *)buf;
    BITMASK16	 flag = buf_s[0];
    XKeyEvent	 ev;
    char	*string = NULL;
    int		 string_len = 0;
    KeySym	*keysym = NULL;
    int		 keysym_len = 0;

    if ((flag & XimLookupBoth) == XimLookupChars) {
	if (!(_XimProcCommit(ic, (BYTE *)&buf_s[2],
			 		(int)buf_s[1], &string, &string_len)))
	    return False;

    } else if ((flag & XimLookupBoth) == XimLookupKeySym) {
	if (!(_XimProcKeySym(ic, *(CARD32 *)&buf_s[2], &keysym, &keysym_len)))
	    return False;

    } else if ((flag & XimLookupBoth) == XimLookupBoth) {
	if (!(_XimProcKeySym(ic, *(CARD32 *)&buf_s[2], &keysym, &keysym_len)))
	    return False;

	if (!(_XimProcCommit(ic, (BYTE *)&buf_s[5],
					(int)buf_s[4], &string, &string_len))) {
	    Xfree(keysym);
	    return False;
	}
    }

    if (!(_XimRegCommitInfo(ic, string, string_len, keysym, keysym_len))) {
       Xfree(string);
	Xfree(keysym);
	_XimError(im, ic, XIM_BadAlloc, (INT16)0, (CARD16)0, (char *)NULL);
	return False;
    }

    (void)_XimRespSyncReply(ic, flag);

    if (ic->private.proto.registed_filter_event
	& (KEYPRESS_MASK | KEYRELEASE_MASK))
	    MARK_FABRICATED(im);

    bzero(&ev, sizeof(ev));	/* uninitialized : found when running kterm under valgrind */

    ev.type = KeyPress;
    ev.send_event = False;
    ev.display = im->core.display;
    ev.window = ic->core.focus_window;
    ev.keycode = 0;
    ev.state = 0;

    ev.time = 0L;
    ev.serial = LastKnownRequestProcessed(im->core.display);
    /* FIXME :
       I wish there were COMMENTs (!) about the data passed around.
    */
#if 0
    fprintf(stderr,"%s,%d: putback k press   FIXED ev.time=0 ev.serial=%lu\n", __FILE__, __LINE__, ev.serial);
#endif

    XPutBackEvent(im->core.display, (XEvent *)&ev);

    return True;
}

Bool
_XimCommitCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID        imid = buf_s[0];
    XICID        icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if ((imid == im->private.proto.imid)
     && (ic = _XimICOfXICID(im, icid))) {
	(void)_XimCommitRecv(im, ic, (XPointer)&buf_s[2]);
	return True;
    }
    return False;
}

void
_XimProcError(
    Xim		 im,
    Xic		 ic,
    XPointer	 data)
{
    return;
}

Bool
_XimErrorCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    BITMASK16	 flag = buf_s[2];
    XIMID        imid;
    XICID        icid;
    Xim		 im = (Xim)call_data;
    Xic		 ic = NULL;

    if (flag & XIM_IMID_VALID) {
	imid = buf_s[0];
	if (imid != im->private.proto.imid)
	    return False;
    }
    if (flag & XIM_ICID_VALID) {
	icid = buf_s[1];
	if (!(ic = _XimICOfXICID(im, icid)))
	    return False;
    }
    _XimProcError(im, ic, (XPointer)&buf_s[3]);

    return True;
}

Bool
_XimError(
    Xim		 im,
    Xic		 ic,
    CARD16	 error_code,
    INT16	 detail_length,
    CARD16	 type,
    char	*detail)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len = 0;

    buf_s[0] = im->private.proto.imid;	/* imid */
    buf_s[2] = XIM_IMID_VALID;		/* flag */
    if (ic) {
    	buf_s[1] = ic->private.proto.icid;	/* icid */
	buf_s[2] |= XIM_ICID_VALID;		/* flag */
    }
    buf_s[3] = error_code;			/* Error Code */
    buf_s[4] = detail_length;			/* length of error detail */
    buf_s[5] = type;				/* type of error detail */

    if (detail_length && detail) {
	len = detail_length;
	memcpy((char *)&buf_s[6], detail, len);
	XIM_SET_PAD(&buf_s[6], len);
    }

    len += sizeof(CARD16)		/* sizeof imid */
	 + sizeof(CARD16)		/* sizeof icid */
	 + sizeof(BITMASK16)		/* sizeof flag */
	 + sizeof(CARD16)		/* sizeof error_code */
	 + sizeof(INT16)		/* sizeof length of detail */
	 + sizeof(CARD16);		/* sizeof type */

    _XimSetHeader((XPointer)buf, XIM_ERROR, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    return True;
}

static int
_Ximctsconvert(
    XlcConv	 conv,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state)
{
    int		 from_left;
    int		 to_left;
    int		 from_savelen;
    int		 to_savelen;
    int		 from_cnvlen;
    int		 to_cnvlen;
    char	*from_buf;
    char	*to_buf;
    char	 scratchbuf[BUFSIZ];
    Status	 tmp_state;

    if (!state)
	state = &tmp_state;

    if (!conv || !from || !from_len) {
	*state = XLookupNone;
	return 0;
    }

    /* Reset the converter.  The CompoundText at 'from' starts in
       initial state.  */
    _XlcResetConverter(conv);

    from_left = from_len;
    to_left = BUFSIZ;
    from_cnvlen = 0;
    to_cnvlen = 0;
    for (;;) {
	from_buf = &from[from_cnvlen];
	from_savelen = from_left;
	to_buf = &scratchbuf[to_cnvlen];
	to_savelen = to_left;
	if (_XlcConvert(conv, (XPointer *)&from_buf, &from_left,
				 (XPointer *)&to_buf, &to_left, NULL, 0) < 0) {
	    *state = XLookupNone;
	    return 0;
	}
	from_cnvlen += (from_savelen - from_left);
	to_cnvlen += (to_savelen - to_left);
	if (from_left == 0) {
	    if (!to_cnvlen) {
		*state = XLookupNone;
		return 0;
           }
	   break;
	}
    }

    if (!to || !to_len || (to_len < to_cnvlen)) {
       *state = XBufferOverflow;
    } else {
       memcpy(to, scratchbuf, to_cnvlen);
       *state = XLookupChars;
    }
    return to_cnvlen;
}

int
_Ximctstombs(XIM xim, char *from, int from_len,
	     char *to, int to_len, Status *state)
{
    return _Ximctsconvert(((Xim)xim)->private.proto.ctom_conv,
			  from, from_len, to, to_len, state);
}

int
_Ximctstowcs(
    XIM		 xim,
    char	*from,
    int		 from_len,
    wchar_t	*to,
    int		 to_len,
    Status	*state)
{
    Xim		 im = (Xim)xim;
    XlcConv	 conv = im->private.proto.ctow_conv;
    int		 from_left;
    int		 to_left;
    int		 from_savelen;
    int		 to_savelen;
    int		 from_cnvlen;
    int		 to_cnvlen;
    char	*from_buf;
    wchar_t	*to_buf;
    wchar_t	 scratchbuf[BUFSIZ];
    Status	 tmp_state;

    if (!state)
	state = &tmp_state;

    if (!conv || !from || !from_len) {
	*state = XLookupNone;
	return 0;
    }

    /* Reset the converter.  The CompoundText at 'from' starts in
       initial state.  */
    _XlcResetConverter(conv);

    from_left = from_len;
    to_left = BUFSIZ;
    from_cnvlen = 0;
    to_cnvlen = 0;
    for (;;) {
	from_buf = &from[from_cnvlen];
       from_savelen = from_left;
       to_buf = &scratchbuf[to_cnvlen];
       to_savelen = to_left;
	if (_XlcConvert(conv, (XPointer *)&from_buf, &from_left,
				 (XPointer *)&to_buf, &to_left, NULL, 0) < 0) {
	    *state = XLookupNone;
	    return 0;
	}
	from_cnvlen += (from_savelen - from_left);
       to_cnvlen += (to_savelen - to_left);
	if (from_left == 0) {
           if (!to_cnvlen){
		*state = XLookupNone;
               return 0;
           }
	    break;
	}
    }

    if (!to || !to_len || (to_len < to_cnvlen)) {
       *state = XBufferOverflow;
    } else {
       memcpy(to, scratchbuf, to_cnvlen * sizeof(wchar_t));
       *state = XLookupChars;
    }
    return to_cnvlen;
}

int
_Ximctstoutf8(
    XIM		 xim,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state)
{
    return _Ximctsconvert(((Xim)xim)->private.proto.ctoutf8_conv,
			  from, from_len, to, to_len, state);
}

int
_XimProtoMbLookupString(
    XIC			 xic,
    XKeyEvent		*ev,
    char		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*state)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    int			 ret;
    Status		 tmp_state;
    XimCommitInfo	 info;

    if (!IS_SERVER_CONNECTED(im))
	return 0;

    if (!state)
	state = &tmp_state;

    if ((ev->type == KeyPress) && (ev->keycode == 0)) { /* Filter function */
	if (!(info = ic->private.proto.commit_info)) {
	    *state = XLookupNone;
	    return 0;
	}

	ret = im->methods->ctstombs((XIM)im, info->string,
			 	info->string_len, buffer, bytes, state);
	if (*state == XBufferOverflow)
            return ret;
	if (keysym && (info->keysym && *(info->keysym))) {
	    *keysym = *(info->keysym);
	    if (*state == XLookupChars)
		*state = XLookupBoth;
	    else
		*state = XLookupKeySym;
	}
	_XimUnregCommitInfo(ic);

    } else  if (ev->type == KeyPress) {
	ret = _XimLookupMBText(ic, ev, buffer, bytes, keysym, NULL);
	if (ret > 0) {
           if (ret > bytes)
               *state = XBufferOverflow;
           else if (keysym && *keysym != NoSymbol)
		*state = XLookupBoth;
	    else
		*state = XLookupChars;
	} else {
	    if (keysym && *keysym != NoSymbol)
		*state = XLookupKeySym;
	    else
		*state = XLookupNone;
	}
    } else {
	*state = XLookupNone;
	ret = 0;
    }

    return ret;
}

int
_XimProtoWcLookupString(
    XIC			 xic,
    XKeyEvent		*ev,
    wchar_t		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*state)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    int			 ret;
    Status		 tmp_state;
    XimCommitInfo	 info;

    if (!IS_SERVER_CONNECTED(im))
	return 0;

    if (!state)
	state = &tmp_state;

    if (ev->type == KeyPress && ev->keycode == 0) { /* Filter function */
	if (!(info = ic->private.proto.commit_info)) {
           *state = XLookupNone;
	    return 0;
	}

	ret = im->methods->ctstowcs((XIM)im, info->string,
			 	info->string_len, buffer, bytes, state);
	if (*state == XBufferOverflow)
           return ret;
	if (keysym && (info->keysym && *(info->keysym))) {
	    *keysym = *(info->keysym);
	    if (*state == XLookupChars)
		*state = XLookupBoth;
	    else
		*state = XLookupKeySym;
	}
	_XimUnregCommitInfo(ic);

    } else if (ev->type == KeyPress) {
	ret = _XimLookupWCText(ic, ev, buffer, bytes, keysym, NULL);
	if (ret > 0) {
           if (ret > bytes)
               *state = XBufferOverflow;
           else if (keysym && *keysym != NoSymbol)
		*state = XLookupBoth;
	    else
		*state = XLookupChars;
	} else {
	    if (keysym && *keysym != NoSymbol)
		*state = XLookupKeySym;
	    else
		*state = XLookupNone;
	}
    } else {
	*state = XLookupNone;
	ret = 0;
    }

    return ret;
}

int
_XimProtoUtf8LookupString(
    XIC			 xic,
    XKeyEvent		*ev,
    char		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*state)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    int			 ret;
    Status		 tmp_state;
    XimCommitInfo	 info;

    if (!IS_SERVER_CONNECTED(im))
	return 0;

    if (!state)
	state = &tmp_state;

    if (ev->type == KeyPress && ev->keycode == 0) { /* Filter function */
	if (!(info = ic->private.proto.commit_info)) {
           *state = XLookupNone;
	    return 0;
	}

	ret = im->methods->ctstoutf8((XIM)im, info->string,
			 	info->string_len, buffer, bytes, state);
	if (*state == XBufferOverflow)
           return ret;
	if (keysym && (info->keysym && *(info->keysym))) {
	    *keysym = *(info->keysym);
	    if (*state == XLookupChars)
		*state = XLookupBoth;
	    else
		*state = XLookupKeySym;
	}
	_XimUnregCommitInfo(ic);

    } else if (ev->type == KeyPress) {
	ret = _XimLookupUTF8Text(ic, ev, buffer, bytes, keysym, NULL);
	if (ret > 0) {
           if (ret > bytes)
               *state = XBufferOverflow;
           else if (keysym && *keysym != NoSymbol)
		*state = XLookupBoth;
	    else
		*state = XLookupChars;
	} else {
	    if (keysym && *keysym != NoSymbol)
		*state = XLookupKeySym;
	    else
		*state = XLookupNone;
	}
    } else {
	*state = XLookupNone;
	ret = 0;
    }

    return ret;
}
