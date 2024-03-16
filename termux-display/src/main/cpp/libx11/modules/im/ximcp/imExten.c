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

/*
 * index of extensions
 */

#define	XIM_EXT_SET_EVENT_MASK_IDX	0
#ifdef EXT_FORWARD
#define	XIM_EXT_FORWARD_KEYEVENT_IDX	1
#endif
#ifdef EXT_MOVE
#define	XIM_EXT_MOVE_IDX		2
#endif

typedef struct	_XIM_QueryExtRec {
    Bool	 is_support;
    const char	*name;
    int		 name_len;
    CARD16	 major_opcode;
    CARD16	 minor_opcode;
    int		 idx;
} XIM_QueryExtRec;

static XIM_QueryExtRec	extensions[] = {
	{False, "XIM_EXT_SET_EVENT_MASK", 0, 0, 0,
					XIM_EXT_SET_EVENT_MASK_IDX},
#ifdef EXT_FORWARD
	{False, "XIM_EXT_FORWARD_KEYEVENT", 0, 0, 0,
					XIM_EXT_FORWARD_KEYEVENT_IDX},
#endif
#ifdef EXT_MOVE
	{False, "XIM_EXT_MOVE", 0, 0, 0, XIM_EXT_MOVE_IDX},
#endif
	{False, NULL, 0, 0, 0, 0}		/* dummy */
};

static int
_XimIsSupportExt(
    int		 idx)
{
    register int i;
    int		 n = XIMNumber(extensions) - 1;

    for (i = 0; i < n; i++) {
	if (extensions[i].idx == idx) {
	    if (extensions[i].is_support)
		return i;
	    else
		break;
	}
    }
    return -1;
}

static Bool
_XimProcExtSetEventMask(
    Xim		 im,
    Xic		 ic,
    XPointer	 buf)
{
    EVENTMASK	*buf_l = (EVENTMASK *)buf;
    EVENTMASK	 select_mask = _XimGetWindowEventmask(ic);

    ic->private.proto.filter_event_mask      = buf_l[0];
    ic->private.proto.intercept_event_mask   = buf_l[1];
    ic->private.proto.select_event_mask      = buf_l[2];
    ic->private.proto.forward_event_mask     = buf_l[3];
    ic->private.proto.synchronous_event_mask = buf_l[4];

    select_mask &= ~ic->private.proto.intercept_event_mask;
						/* deselected event mask */
    select_mask |= ic->private.proto.select_event_mask;
						/* selected event mask */
    XSelectInput(im->core.display, ic->core.focus_window, select_mask);
    _XimReregisterFilter(ic);

    if (!(_XimProcSyncReply(im, ic)))
	return False;
    return True;
}

static Bool
_XimExtSetEventMaskCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if ((imid == im->private.proto.imid)
     && (ic = _XimICOfXICID(im, icid))) {
	(void)_XimProcExtSetEventMask(im, ic, (XPointer)&buf_s[2]);
	return True;
    }
    return False;
}

#ifdef EXT_FORWARD
static Bool
_XimProcExtForwardKeyEvent(
    Xim		 im,
    Xic		 ic,
    XPointer	 buf)
{
    CARD8	*buf_b = (CARD8 *)buf;
    CARD16	*buf_s = (CARD16 *)buf;
    CARD32	*buf_l = (CARD32 *)buf;
    XEvent	 ev;
    XKeyEvent	*kev = (XKeyEvent *)&ev;

    bzero(&ev, sizeof(XEvent));
    kev->send_event	= False;
    kev->display	= im->core.display;
    kev->serial		= buf_s[1];		/* sequence number */
    kev->type		= buf_b[4] & 0x7f;	/* xEvent.u.u.type */
    kev->keycode	= buf_b[5];		/* Keycode */
    kev->state		= buf_s[3];		/* state */
    kev->time		= buf_l[2];		/* time */

    XPutBackEvent(im->core.display, &ev);

    _XimRespSyncReply(ic, buf_s[0]);
    MARK_FABRICATED(im);

    return True;
}

static Bool
_XimExtForwardKeyEventCallback(
    Xim		 xim,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];
    Xim		 im = (Xim)call_data;
    Xic		 ic;

    if ((imid == im->private.proto.imid)
     && (ic = _XimICOfXICID(im, icid))) {
	(void)_XimProcExtForwardKeyEvent(im, ic, (XPointer)&buf_s[2]);
	return True;
    }
    return False;
}

static Bool
_XimExtForwardKeyEventCheck(
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
_XimExtForwardKeyEvent(
    Xic		 ic,
    XKeyEvent	*ev,
    Bool	 sync)
{
    Xim		 im = (Xim) ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD8	*buf_b = &buf[XIM_HEADER_SIZE];
    CARD16	*buf_s = (CARD16 *)buf_b;
    CARD32	*buf_l = (CARD32 *)buf_b;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	preply;
    int		buf_size;
    int		ret_code;
    INT16	len;
    int		idx;

    if ((idx = _XimIsSupportExt(XIM_EXT_FORWARD_KEYEVENT_IDX)) < 0)
	return False;

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */
    buf_s[2] = sync ? XimSYNCHRONUS : 0;	/* flag */
    buf_s[3] = (CARD16)(((XAnyEvent *)ev)->serial & ((unsigned long) 0xffff));
						/* sequence number */
    buf_b[8] = ev->type;			/* xEvent.u.u.type */
    buf_b[9] = ev->keycode;			/* keycode */
    buf_s[5] = ev->state;			/* state */
    buf_l[3] = ev->time;			/* time */
    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16)			/* sizeof icid */
	+ sizeof(BITMASK16)			/* sizeof flag */
	+ sizeof(CARD16)			/* sizeof sequence number */
	+ sizeof(BYTE)				/* sizeof xEvent.u.u.type */
	+ sizeof(BYTE)				/* sizeof keycode */
	+ sizeof(CARD16)			/* sizeof state */
	+ sizeof(CARD32);			/* sizeof time */

    _XimSetHeader((XPointer)buf,
		extensions[idx].major_opcode,
		extensions[idx].minor_opcode, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    if (sync) {
    	buf_size = BUFSIZE;
    	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    				_XimExtForwardKeyEventCheck, (XPointer)ic);
    	if(ret_code == XIM_TRUE) {
    	    preply = reply;
    	} else if(ret_code == XIM_OVERFLOW) {
    	    if(len <= 0) {
    		preply = reply;
    	    } else {
    		buf_sizex = len;
		preply = Xmalloc(buf_size);
    		ret_code = _XimRead(im, &len, preply, buf_size,
    				_XimExtForwardKeyEventCheck, (XPointer)ic);
    		if(ret_code != XIM_TRUE) {
		    Xfree(preply);
    		    return False;
		}
    	    }
    	} else
	    return False;
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
#endif /* EXT_FORWARD */

static int
_XimCheckExtensionListSize(void)
{
    register int i;
    int		 len;
    int		 total = 0;
    int		 n = XIMNumber(extensions) - 1;

    for (i = 0; i < n; i++) {
	len = strlen(extensions[i].name);
	extensions[i].name_len = len;
	len += sizeof(BYTE);
	total += len;
    }
    return total;
}

static void
_XimSetExtensionList(
    CARD8	*buf)
{
    register int i;
    int		 len;
    int		 n = XIMNumber(extensions) - 1;

    for (i = 0; i < n; i++) {
	len = extensions[i].name_len;
	buf[0] = (BYTE)len;
	(void)strcpy((char *)&buf[1], extensions[i].name);
	len += sizeof(BYTE);
	buf += len;
    }
    return;
}

static unsigned int
_XimCountNumberOfExtension(
    INT16	 total,
    CARD8	*ext)
{
    unsigned int n;
    INT16	 len;
    INT16	 min_len = sizeof(CARD8)
			 + sizeof(CARD8)
			 + sizeof(INT16);

    n = 0;
    while (total > min_len) {
	len = *((INT16 *)(&ext[2]));
	len += (min_len + XIM_PAD(len));
	total -= len;
	ext += len;
	n++;
    }
    return n;
}

static Bool
_XimParseExtensionList(
    Xim			 im,
    CARD16		*data)
{
    int			 num = XIMNumber(extensions) - 1;
    unsigned int	 n;
    CARD8		*buf;
    register int	 i;
    register int	 j;
    INT16		 len;

    if (!(n = _XimCountNumberOfExtension(data[0], (CARD8 *)&data[1])))
	return True;

    buf = (CARD8 *)&data[1];
    for (i = 0; i < n; i++) {
	len = *((INT16 *)(&buf[2]));
	for (j = 0; j < num; j++) {
	    if (!(strncmp(extensions[j].name, (char *)&buf[4], len))) {
		extensions[j].major_opcode = buf[0];
		extensions[j].minor_opcode = buf[1];
		extensions[j].is_support   = True;
		break;
	    }
	}
	len += sizeof(CARD8)		/* sizeof major_opcode */
	     + sizeof(CARD8)		/* sizeof minor_opcode */
	     + sizeof(INT16)		/* sizeof length */
	     + XIM_PAD(len);		/* sizeof pad */
	buf += len;
    }

    return True;
}

static Bool
_XimQueryExtensionCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_QUERY_EXTENSION_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid))
	return True;
    return False;
}

Bool
_XimExtension(
    Xim		 im)
{
    CARD8	*buf;
    CARD16	*buf_s;
    int		 buf_len;
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;
    int		 idx;

    if (!(len = _XimCheckExtensionListSize()))
	return True;

    buf_len = XIM_HEADER_SIZE
	    + sizeof(CARD16)
	    + sizeof(INT16)
	    + len
	    + XIM_PAD(len);

    if (!(buf = Xmalloc(buf_len)))
	return False;
    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    buf_s[0] = im->private.proto.imid;	/* imid */
    buf_s[1] = len;			/* length of Extensions */
    _XimSetExtensionList((CARD8 *)&buf_s[2]);
					/* extensions supported */
    XIM_SET_PAD(&buf_s[2], len);	/* pad */
    len += sizeof(CARD16)		/* sizeof imid */
	 + sizeof(INT16);		/* sizeof length of extensions */

    _XimSetHeader((XPointer)buf, XIM_QUERY_EXTENSION, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
        XFree(buf);
	return False;
    }
    XFree(buf);
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    					_XimQueryExtensionCheck, 0);
    if(ret_code == XIM_TRUE) {
    	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
    	if(len <= 0) {
    	    preply = reply;
    	} else {
    	    buf_size = len;
	    preply = Xmalloc(buf_size);
    	    ret_code = _XimRead(im, &len, reply, buf_size,
    					_XimQueryExtensionCheck, 0);
    	    if(ret_code != XIM_TRUE) {
		Xfree(preply);
    		return False;
	    }
    	}
    } else
	return False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
    	if(reply != preply)
    	    Xfree(preply);
	return False;
    }

    if (!(_XimParseExtensionList(im, &buf_s[1]))) {
    	if(reply != preply)
    	    Xfree(preply);
	return False;
    }
    if(reply != preply)
    	Xfree(preply);

    if ((idx = _XimIsSupportExt(XIM_EXT_SET_EVENT_MASK_IDX)) >= 0)
	_XimRegProtoIntrCallback(im,
	 	extensions[idx].major_opcode,
	 	extensions[idx].minor_opcode,
		_XimExtSetEventMaskCallback, (XPointer)im);
#ifdef EXT_FORWARD
    if ((idx = _XimIsSupportExt(XIM_EXT_FORWARD_KEYEVENT_IDX)) >= 0)
	_XimRegProtoIntrCallback(im,
		extensions[idx].major_opcode,
		extensions[idx].minor_opcode,
		_XimExtForwardKeyEventCallback, (XPointer)im);
#endif

    return True;
}

#ifdef EXT_MOVE
/* flag of ExtenArgCheck */
#define	EXT_XNSPOTLOCATION	(1L<<0)

/* macro for ExtenArgCheck */
#define SET_EXT_XNSPOTLOCATION(flag) (flag |= EXT_XNSPOTLOCATION)
#define IS_EXT_XNSPOTLOCATION(flag)  (flag & EXT_XNSPOTLOCATION)

/* length of XPoint attribute */
#define	XIM_Xpoint_length	12

static Bool
_XimExtMove(
    Xim		 im,
    Xic		 ic,
    CARD16	 x,
    CARD16	 y)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;
    int		idx;

    if ((idx = _XimIsSupportExt(XIM_EXT_MOVE_IDX)) < 0)
	return False;

    buf_s[0] = im->private.proto.imid;	/* imid */
    buf_s[1] = ic->private.proto.icid;	/* icid */
    buf_s[2] = x;			/* X */
    buf_s[3] = y;			/* Y */
    len = sizeof(CARD16)		/* sizeof imid */
	+ sizeof(CARD16)		/* sizeof icid */
	+ sizeof(INT16)			/* sizeof X */
	+ sizeof(INT16);		/* sizeof Y */

    _XimSetHeader((XPointer)buf, extensions[idx].major_opcode,
			extensions[idx].minor_opcode, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    return True;
}

BITMASK32
_XimExtenArgCheck(
    XIMArg	*arg)
{
    CARD32	flag = 0L;
    if (!strcmp(arg->name, XNSpotLocation))
	SET_EXT_XNSPOTLOCATION(flag);
    return flag;
}

Bool
_XimExtenMove(
    Xim		 im,
    Xic		 ic,
    CARD32	 flag,
    CARD16	*buf,
    INT16	 length)
{
    if ((IS_EXT_XNSPOTLOCATION(flag)) && (length == XIM_Xpoint_length))
	return _XimExtMove(im, ic, buf[4], buf[5]);
    return False;
}
#endif /* EXT_MOVE */
