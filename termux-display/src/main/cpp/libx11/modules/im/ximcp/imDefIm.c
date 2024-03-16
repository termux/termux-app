/*
 * Copyright (c) 1990, 1991, 1992, Oracle and/or its affiliates.
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
         Copyright 1993, 1994 by Sony Corporation

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital, FUJITSU
LIMITED and Sony Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.

DIGITAL, FUJITSU LIMITED AND SONY CORPORATION DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL DIGITAL, FUJITSU LIMITED
AND SONY CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Hideki Hiura (hhiura@Sun.COM) Sun Microsystems, Inc.
          Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp
          Makoto Wakamatsu     Sony Corporation
                               makoto@sm.sony.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPublic.h"
#include "XlcPubI.h"
#include "XimTrInt.h"
#include "Ximint.h"

#include <limits.h>

int
_XimCheckDataSize(
    XPointer	 buf,
    int		 len)
{
    CARD16	*buf_s = (CARD16 *)buf;

    if(len < XIM_HEADER_SIZE)
	return -1;
    return  buf_s[1];
}

void
_XimSetHeader(
    XPointer	 buf,
    CARD8	 major_opcode,
    CARD8	 minor_opcode,
    INT16	*len
)
{
    CARD8	*buf_b = (CARD8 *)buf;
    CARD16	*buf_s = (CARD16 *)buf;

    buf_b[0] = major_opcode;
    buf_b[1] = minor_opcode;
    buf_s[1] = ((*len) / 4);
    *len += XIM_HEADER_SIZE;
    return;
}

char
_XimGetMyEndian(void)
{
    CARD16	 test_card = 1;

    if(*((char *)&test_card))
	return LITTLEENDIAN;
    else
	return BIGENDIAN;
}

static Bool
_XimCheckServerName(
    Xim		   im,
    char	  *str)
{
    char	  *server_name = im->core.im_name;
    int		   len;
    int		   str_len;
    int		   category_len = strlen(XIM_SERVER_CATEGORY);
    char	  *pp;
    register char *p;

    if(server_name && *server_name)
	len = strlen(server_name);
    else
	return True;

    if((int)strlen(str) < category_len)
	return False;

    if(strncmp(str, XIM_SERVER_CATEGORY, category_len))
	return False;

    pp = &str[category_len];

    for(;;) {
	for(p = pp; (*p != ',') && (*p); p++);
	str_len = (int)(p - pp);

	if((len == str_len) && (!strncmp(pp, server_name, len)))
	    break;
	if(!(*p))
	    return False;
	pp = p + 1;
    }
    return True;
}

static char *
_XimCheckLocaleName(
    Xim		   im,
    char	  *address,
    int		   address_len,
    char	  *locale_name[],
    int		   len)
{
    int		   category_len;
    char	  *pp;
    register char *p;
    register int   n;
    Bool           finish = False;

    category_len = strlen(XIM_LOCAL_CATEGORY);
    if(address_len < category_len)
	return (char*)NULL;

    if(strncmp(address, XIM_LOCAL_CATEGORY, category_len))
	return (char*)NULL;

    pp = &address[category_len];

    for(;;) {
	for( p = pp; *p && *p != ','; p++);
	if (!*p)
	    finish = True;
	address_len = (int)(p - pp);
	*p = '\0';

	for( n = 0; n < len; n++ )
	    if( locale_name[n] && !_XlcCompareISOLatin1( pp, locale_name[n] ) )
		return locale_name[n];
	if (finish)
	    break;
	pp = p + 1;
    }
    return (char *)NULL;
}

static Bool
_XimCheckTransport(
    char	  *address,
    int		   address_len,
    const char	  *transport,
    int		   len,
    char	 **trans_addr)
{
    int		   category_len = strlen(XIM_TRANSPORT_CATEGORY);
    char	  *pp;
    register char *p;

    if(address_len < category_len)
	return False;

    if(strncmp(address, XIM_TRANSPORT_CATEGORY, category_len))
	return False;

    pp = &address[category_len];

    for(;;) {
	*trans_addr = pp;

	for(p = pp; (*p != '/') && (*p != ',') && (*p); p++);
	if(*p == ',') {
	    pp = p + 1;
	    continue;
	}
	if(!(*p))
	    return False;

	address_len = (int)(p - pp);

	if((len == address_len) && (!strncmp(pp, transport, len)))
	    break;
	pp = p + 1;
    }
    pp = p + 1;
    for(p = pp; (*p != ',') && (*p); p++);
    if (*p)
	*p = '\0';
    return True;
}

static Bool
_CheckSNEvent(
    Display		*display,
    XEvent		*xevent,
    XPointer		 arg)
{
    XSelectionEvent	*event = (XSelectionEvent *)xevent;
    Window		 window = *(Window*)arg;

    if((event->type == SelectionNotify) && (window == event->requestor))
	return True;
    return False;
}

static Bool
_XimGetSelectionNotify(
    Display		 *display,
    Window		  window,
    Atom		  target,
    char		**ret_address)
{
    XEvent		  event;
    XSelectionEvent	 *ev = (XSelectionEvent *)&event;
    Atom		  actual_type;
    int			  actual_format;
    unsigned long	  nitems, bytes_after;

    for(;;) {
	XIfEvent(display, &event, _CheckSNEvent, (XPointer)&window);
	if((ev->type == SelectionNotify) && (window == ev->requestor))
	    break;
    }

    if(ev->property == (Atom)None)
	return False;
    if( XGetWindowProperty( display, window, target, 0L, 1000000L,
			    True, target, &actual_type, &actual_format,
			    &nitems, &bytes_after,
			    (unsigned char **)&*ret_address ) != Success )
	return False;
    return True;
}

static Bool
_XimPreConnectionIM(
    Xim			 im,
    Atom		 selection)
{
    Display		*display = im->core.display;
    Atom		 locales, transport;
    char		*address;
    XLCd		 lcd;
    char		*language;
    char		*territory;
    char		*codeset;
    char		*trans_addr;
    char		*locale_name[4], *locale;
    int			 llen, tlen, clen;
    register int	 i;
    Window		 window;
    char		*str;

    if(!(lcd = im->core.lcd))
	return False;

    for( i = 0; i < 4; i++ )
	locale_name[i] = NULL;
    /* requestor window */
    if(!(window = XCreateSimpleWindow(display, DefaultRootWindow(display),
			 				0, 0, 1, 1, 1, 0, 0)))
	return False;

    /* server name check */
    if( !(str = XGetAtomName( display, selection )) )
	return False;
    if(!_XimCheckServerName(im, str)) {
	XFree( (XPointer)str );
	goto Error;
    }
    XFree( (XPointer)str );

    /* locale name check */
    _XGetLCValues(lcd, XlcNLanguage, &language, XlcNTerritory, &territory,
                    XlcNCodeset, &codeset, NULL);
    llen = strlen( language );
    tlen = territory ? strlen( territory ): 0;
    clen = codeset ? strlen( codeset ): 0;

    if( tlen != 0  &&  clen != 0 ) {
	if( (locale_name[0] = Xmalloc(llen+tlen+clen+3)) != NULL )
	    sprintf( locale_name[0], "%s_%s.%s", language, territory, codeset );
    }
    if( clen != 0 ) {
	if( (locale_name[1] = Xmalloc(llen+clen+2)) != NULL )
	    sprintf( locale_name[1], "%s.%s", language, codeset );
	else
	    goto Error;
    }
    if( tlen != 0 ) {
	if( (locale_name[2] = Xmalloc(llen+tlen+2)) != NULL )
	    sprintf( locale_name[2], "%s_%s", language, territory );
	else
	    goto Error;
    }
    if( (locale_name[3] = strdup(language)) == NULL )
	goto Error;
    if((locales = XInternAtom(display, XIM_LOCALES, True)) == (Atom)None)
	goto Error;

    XConvertSelection(display, selection, locales, locales, window,
		      CurrentTime);
    if(!(_XimGetSelectionNotify(display, window, locales, &address)))
	goto Error;

    if((locale = _XimCheckLocaleName(im, address, strlen(address), locale_name,
				     4)) == NULL) {
	XFree((XPointer)address);
	goto Error;
    }
    im->private.proto.locale_name = locale;
    for( i = 0; i < 4; i++ ) {
	if( locale_name[i] != NULL  &&  locale_name[i] != locale ) {
	    XFree( locale_name[i] );
	    locale_name[i] = NULL;
	}
    }
    XFree((XPointer)address);

    /* transport check */
    if((transport = XInternAtom(display, XIM_TRANSPORT, True)) == (Atom)None)
	goto Error;

    XConvertSelection(display, selection, transport, transport, window,
		      CurrentTime);
    if(!_XimGetSelectionNotify(display, window, transport, &address))
	goto Error;

    for(i = 0; _XimTransportRec[i].transportname ; i++) {
	if( _XimCheckTransport(address, strlen(address),
				_XimTransportRec[i].transportname,
				strlen(_XimTransportRec[i].transportname),
				 &trans_addr)) {
	    if( _XimTransportRec[i].config(im, trans_addr) ) {
		XFree((XPointer)address);
		XDestroyWindow(display, window);
		return True;
	    }
	}
    }

    XFree((XPointer)address);
Error:
    for( i = 0; i < 4; i++ )
	if( locale_name[i] != NULL )
	    XFree( locale_name[i] );
    XDestroyWindow(display, window);
    return False;
}

static Bool
_XimPreConnect(
    Xim		    im)
{
    Display	   *display = im->core.display;
    Atom	    imserver;
    Atom	    actual_type;
    int		    actual_format;
    unsigned long   nitems;
    unsigned long   bytes_after;
    unsigned char  *prop_return;
    Atom	   *atoms;
    Window	    im_window = 0;
    register int    i;

    if((imserver = XInternAtom(display, XIM_SERVERS, True)) == (Atom)None)
	return False;

    if(XGetWindowProperty(display, RootWindow(display, 0),
			imserver, 0L, 1000000L, False, XA_ATOM, &actual_type,
			&actual_format, &nitems, &bytes_after,
			&prop_return) != Success)
	return False;

    if( (actual_type != XA_ATOM) || (actual_format != 32) ) {
	if( nitems )
	    XFree((XPointer)prop_return);
	return False;
    }

    atoms = (Atom *)prop_return;
    for(i = 0; i < nitems; i++) {
	if((im_window = XGetSelectionOwner(display, atoms[i])) == (Window)None)
	    continue;

	if(_XimPreConnectionIM(im, atoms[i]))
	    break;
    }

    XFree((XPointer)prop_return);
    if(i >= nitems)
	return False;

    im->private.proto.im_window = im_window;
    return True;
}

static Bool
_XimGetAuthProtocolNames(
    Xim		 im,
    CARD16	*buf,
    CARD8	*num,
    INT16	*len)
{
    if (!IS_USE_AUTHORIZATION_FUNC(im)) {
	*num = 0;
	*len = 0;
	return True;
    }
    /*
     * Not yet
     */
    return True;
}

static Bool
_XimSetAuthReplyData(
    Xim		 im,
    XPointer	 buf,
    INT16	*len)
{
    /*
     * Not yet
     */
    *len = 0;
    return True;
}

static Bool
_XimSetAuthNextData(
    Xim		 im,
    XPointer	 buf,
    INT16	*len)
{
    /*
     * Not yet
     */
    *len = 0;
    return True;
}

static Bool
_XimSetAuthRequiredData(
    Xim		 im,
    XPointer	 buf,
    INT16	*len)
{
    /*
     * Not yet
     */
    *len = 0;
    return True;
}

static Bool
_XimCheckAuthSetupData(
    Xim		 im,
    XPointer	 buf)
{
    /*
     * Not yet
     */
    return True;
}

static Bool
_XimCheckAuthNextData(
    Xim		 im,
    XPointer	 buf)
{
    /*
     * Not yet
     */
    return True;
}

#define	NO_MORE_AUTH	2
#define	GOOD_AUTH	1
#define	BAD_AUTH	0

static int
_XimClientAuthCheck(
    Xim		 im,
    XPointer	 buf)
{
    /*
     * Not yet
     */
    return NO_MORE_AUTH;
}

static void
_XimAuthNG(
    Xim		 im)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    INT16	 len = 0;

    _XimSetHeader((XPointer)buf, XIM_AUTH_NG, 0, &len);
    (void)_XimWrite(im, len, (XPointer)buf);
    _XimFlush(im);
    return;
}

static Bool
_XimAllRecv(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 arg)
{
    return True;
}

#define	CLIENT_WAIT1		1
#define	CLIENT_WAIT2		2

static Bool
_XimConnection(
    Xim		 im)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD8	*buf_b = &buf[XIM_HEADER_SIZE];
    CARD16	*buf_s = (CARD16 *)((XPointer)buf_b);
    INT16	 len;
    CARD8	 num;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;
    CARD8	 major_opcode;
    int		 wait_mode;
    int		 ret;

    if(!(_XimConnect(im)))	/* Transport Connect */
	return False;

    if(!_XimDispatchInit(im))
	return False;

    _XimRegProtoIntrCallback(im, XIM_ERROR, 0, _XimErrorCallback, (XPointer)im);

    if(!_XimGetAuthProtocolNames(im, &buf_s[4], &num, &len))
	return False;

    im->private.proto.protocol_major_version = PROTOCOLMAJORVERSION;
    im->private.proto.protocol_minor_version = PROTOCOLMINORVERSION;

    buf_b[0] = _XimGetMyEndian();
    buf_b[1] = 0;
    buf_s[1] = PROTOCOLMAJORVERSION;
    buf_s[2] = PROTOCOLMINORVERSION;
    buf_s[3] = num;
    len  += sizeof(CARD8)
          + sizeof(CARD8)
          + sizeof(CARD16)
          + sizeof(CARD16)
          + sizeof(CARD16);

    major_opcode = XIM_CONNECT;
    wait_mode = (IS_USE_AUTHORIZATION_FUNC(im)) ? CLIENT_WAIT1 : CLIENT_WAIT2;

    for(;;) {
	_XimSetHeader((XPointer)buf, major_opcode, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf)))
	    return False;
	_XimFlush(im);
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, reply, buf_size, _XimAllRecv, 0);
	if(ret_code == XIM_TRUE) {
	    preply = reply;
	} else if(ret_code == XIM_OVERFLOW) {
	    if(len <= 0) {
		preply = reply;
	    } else {
		buf_size = len;
		preply = Xmalloc(buf_size);
		ret_code = _XimRead(im, &len, preply, buf_size, _XimAllRecv, 0);
		if(ret_code != XIM_TRUE) {
		    Xfree(preply);
		    return False;
		}
	    }
	} else
	    return False;

	major_opcode = *((CARD8 *)preply);
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);

	if (wait_mode == CLIENT_WAIT1) {
	    if (major_opcode == XIM_AUTH_REQUIRED) {
		ret = _XimClientAuthCheck(im, (XPointer)buf_s);
		if(reply != preply)
		    Xfree(preply);
		if (ret == NO_MORE_AUTH) {
		    if (!(_XimSetAuthReplyData(im,
				(XPointer)&buf[XIM_HEADER_SIZE], &len))) {
			_XimAuthNG(im);
			return False;
		    }
		    major_opcode = XIM_AUTH_REPLY;
		    wait_mode = CLIENT_WAIT2;
		} else if (ret == GOOD_AUTH) {
		    if (!(_XimSetAuthNextData(im,
				(XPointer)&buf[XIM_HEADER_SIZE], &len))) {
			_XimAuthNG(im);
			return False;
		    }
		    major_opcode = XIM_AUTH_NEXT;
		} else {	/* BAD_AUTH */
		    _XimAuthNG(im);
		    return False;
		}
	    } else {
		if(reply != preply)
		    Xfree(preply);
		_XimAuthNG(im);
		return False;
	    }
	} else {	/* CLIENT_WAIT2 */
	    if (major_opcode == XIM_CONNECT_REPLY) {
		break;
	    } else if (major_opcode == XIM_AUTH_SETUP) {
		if (!(_XimCheckAuthSetupData(im, (XPointer)buf_s))) {
		    _XimAuthNG(im);
		    return False;
		}
		if(reply != preply)
		    Xfree(preply);
		if (!(_XimSetAuthRequiredData(im,
				(XPointer)&buf[XIM_HEADER_SIZE], &len))) {
		    _XimAuthNG(im);
		    return False;
		}
		major_opcode = XIM_AUTH_REQUIRED;
	    } else if (major_opcode == XIM_AUTH_NEXT) {
		if (!(_XimCheckAuthNextData(im, (XPointer)buf_s))) {
		    _XimAuthNG(im);
		    return False;
		}
		if(reply != preply)
		    Xfree(preply);
		if (!(_XimSetAuthRequiredData(im,
				(XPointer)&buf[XIM_HEADER_SIZE], &len))) {
		    _XimAuthNG(im);
		    return False;
		}
		major_opcode = XIM_AUTH_REQUIRED;
	    } else if (major_opcode == XIM_AUTH_NG) {
		if(reply != preply)
		    Xfree(preply);
		return False;
	    } else {
		_XimAuthNG(im);
		if(reply != preply)
		    Xfree(preply);
		return False;
	    }
	}
    }

    if (!( buf_s[0] == im->private.proto.protocol_major_version
        && buf_s[1] == im->private.proto.protocol_minor_version)) {
	if(reply != preply)
	    Xfree(preply);
	return False;
    }
    if(reply != preply)
	Xfree(preply);
    MARK_SERVER_CONNECTED(im);

    _XimRegProtoIntrCallback(im, XIM_REGISTER_TRIGGERKEYS, 0,
				 _XimRegisterTriggerKeysCallback, (XPointer)im);
    return True;
}

static Bool
_XimDisconnectCheck(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 arg)
{
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);

    if ((major_opcode == XIM_DISCONNECT_REPLY)
     && (minor_opcode == 0))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0))
	return True;
    return False;
}

static Bool
_XimDisconnect(
    Xim		 im)
{
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    INT16	 len = 0;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;

    if (IS_SERVER_CONNECTED(im)) {
	_XimSetHeader((XPointer)buf, XIM_DISCONNECT, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf)))
	    return False;
	_XimFlush(im);
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
						_XimDisconnectCheck, 0);
	if(ret_code == XIM_OVERFLOW) {
	    if(len > 0) {
		buf_size = len;
		preply = Xmalloc(buf_size);
		ret_code = _XimRead(im, &len, preply, buf_size,
						 _XimDisconnectCheck, 0);
		Xfree(preply);
		if(ret_code != XIM_TRUE)
		    return False;
	    }
	} else if(ret_code == XIM_FALSE)
	    return False;

    }
    if (!(_XimShutdown(im)))	/* Transport shutdown */
	return False;
    return True;
}

static Bool
_XimOpenCheck(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 arg)
{
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);

    if ((major_opcode == XIM_OPEN_REPLY)
     && (minor_opcode == 0))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0))
	return True;
    return False;
}

static Bool
_XimOpen(
    Xim			 im)
{
    CARD32		 buf32[BUFSIZE/4];
    CARD8		*buf = (CARD8 *)buf32;
    CARD8		*buf_b = &buf[XIM_HEADER_SIZE];
    CARD16		*buf_s;
    INT16		 len;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 buf_size;
    int			 ret_code;
    char		*locale_name;
    size_t		 locale_len;

    locale_name = im->private.proto.locale_name;
    locale_len = strlen(locale_name);
    if (locale_len > UCHAR_MAX)
      return False;
    memset(buf32, 0, sizeof(buf32));
    buf_b[0] = (BYTE)locale_len;		/* length of locale name */
    memcpy(&buf_b[1], locale_name, locale_len);	   /* locale name */
    len = (INT16)(locale_len + sizeof(BYTE));	   /* sizeof length */
    XIM_SET_PAD(buf_b, len);			   /* pad */

    _XimSetHeader((XPointer)buf, XIM_OPEN, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, reply, buf_size,
					_XimOpenCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
					_XimOpenCheck, 0);
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

    im->private.proto.imid = buf_s[0];		/* imid */

    if (!(_XimGetAttributeID(im, &buf_s[1]))) {
	if(reply != preply)
	    Xfree(preply);
	return False;
    }
    if(reply != preply)
	Xfree(preply);

    if (!(_XimSetInnerIMResourceList(&(im->private.proto.im_inner_resources),
				&(im->private.proto.im_num_inner_resources))))
	return False;

    if (!(_XimSetInnerICResourceList(&(im->private.proto.ic_inner_resources),
				&(im->private.proto.ic_num_inner_resources))))
	return False;

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);
    _XimSetIMMode(im->private.proto.im_inner_resources,
				im->private.proto.im_num_inner_resources);

    /* Transport Callback */
    _XimRegProtoIntrCallback(im, XIM_SET_EVENT_MASK, 0,
				 _XimSetEventMaskCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_FORWARD_EVENT, 0,
				 _XimForwardEventCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_COMMIT, 0,
				 _XimCommitCallback, (XPointer)im);
    _XimRegProtoIntrCallback(im, XIM_SYNC, 0,
				 _XimSyncCallback, (XPointer)im);

    if(!_XimExtension(im))
	return False;

    /* register a hook for callback protocols */
    _XimRegisterDispatcher(im, _XimCbDispatch, (XPointer)im);

    return True;
}

static Bool
_XimCloseCheck(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_CLOSE_REPLY)
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

static Bool
_XimClose(
    Xim		 im)
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

    if (!IS_SERVER_CONNECTED(im))
	return True;

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = 0;				/* unused */
    len = sizeof(CARD16)			/* sizeof imid */
        + sizeof(CARD16);			/* sizeof unused */

    _XimSetHeader((XPointer)buf, XIM_CLOSE, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return False;
    _XimFlush(im);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
						_XimCloseCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size, _XimCloseCheck, 0);
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
    return True;
}

void
_XimProtoIMFree(
    Xim		  im)
{
    /* XIMPrivateRec */
    if (im->private.proto.im_onkeylist) {
	Xfree(im->private.proto.im_onkeylist);
	im->private.proto.im_onkeylist = NULL;
    }
    if (im->private.proto.im_offkeylist) {
	Xfree(im->private.proto.im_offkeylist);
	im->private.proto.im_offkeylist = NULL;
    }
    if (im->private.proto.intrproto) {
	_XimFreeProtoIntrCallback(im);
	im->private.proto.intrproto = NULL;
    }
    if (im->private.proto.im_inner_resources) {
	Xfree(im->private.proto.im_inner_resources);
	im->private.proto.im_inner_resources = NULL;
    }
    if (im->private.proto.ic_inner_resources) {
	Xfree(im->private.proto.ic_inner_resources);
	im->private.proto.ic_inner_resources = NULL;
    }
    if (im->private.proto.hold_data) {
	Xfree(im->private.proto.hold_data);
	im->private.proto.hold_data = NULL;
    }
    if (im->private.proto.locale_name) {
	Xfree(im->private.proto.locale_name);
	im->private.proto.locale_name = NULL;
    }
    if (im->private.proto.ctom_conv) {
	_XlcCloseConverter(im->private.proto.ctom_conv);
	im->private.proto.ctom_conv = NULL;
    }
    if (im->private.proto.ctow_conv) {
	_XlcCloseConverter(im->private.proto.ctow_conv);
	im->private.proto.ctow_conv = NULL;
    }
    if (im->private.proto.ctoutf8_conv) {
	_XlcCloseConverter(im->private.proto.ctoutf8_conv);
	im->private.proto.ctoutf8_conv = NULL;
    }
    if (im->private.proto.cstomb_conv) {
	_XlcCloseConverter(im->private.proto.cstomb_conv);
	im->private.proto.cstomb_conv = NULL;
    }
    if (im->private.proto.cstowc_conv) {
	_XlcCloseConverter(im->private.proto.cstowc_conv);
	im->private.proto.cstowc_conv = NULL;
    }
    if (im->private.proto.cstoutf8_conv) {
	_XlcCloseConverter(im->private.proto.cstoutf8_conv);
	im->private.proto.cstoutf8_conv = NULL;
    }
    if (im->private.proto.ucstoc_conv) {
	_XlcCloseConverter(im->private.proto.ucstoc_conv);
	im->private.proto.ucstoc_conv = NULL;
    }
    if (im->private.proto.ucstoutf8_conv) {
	_XlcCloseConverter(im->private.proto.ucstoutf8_conv);
	im->private.proto.ucstoutf8_conv = NULL;
    }

#ifdef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im)) {
	return;
    }
#endif /* XIM_CONNECTABLE */

    if (im->private.proto.saved_imvalues) {
        Xfree(im->private.proto.saved_imvalues);
        im->private.proto.saved_imvalues = NULL;
    }
    if (im->private.proto.default_styles) {
	Xfree(im->private.proto.default_styles);
	im->private.proto.default_styles = NULL;
    }

    /* core */
    if (im->core.res_name) {
        Xfree(im->core.res_name);
	im->core.res_name = NULL;
    }
    if (im->core.res_class) {
        Xfree(im->core.res_class);
	im->core.res_class = NULL;
    }
    if (im->core.im_values_list) {
	Xfree(im->core.im_values_list);
	im->core.im_values_list = NULL;
    }
    if (im->core.ic_values_list) {
	Xfree(im->core.ic_values_list);
	im->core.ic_values_list = NULL;
    }
    if (im->core.im_name) {
	Xfree(im->core.im_name);
	im->core.im_name = NULL;
    }
    if (im->core.styles) {
	Xfree(im->core.styles);
	im->core.styles = NULL;
    }
    if (im->core.im_resources) {
        Xfree(im->core.im_resources);
	im->core.im_resources = NULL;
    }
    if (im->core.ic_resources) {
        Xfree(im->core.ic_resources);
	im->core.ic_resources = NULL;
    }

    return;
}

static Status
_XimProtoCloseIM(
    XIM		 xim)
{
    Xim		 im = (Xim)xim;
    XIC		 ic;
    XIC		 next;
    Status	 status;

    ic = im->core.ic_chain;
    while (ic) {
	(*ic->methods->destroy) (ic);
	next = ic->core.next;
#ifdef XIM_CONNECTABLE
	if (!(!IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im))) {
	    Xfree (ic);
	}
#else
	Xfree (ic);
#endif /* XIM_CONNECTABLE */
	ic = next;
    }
#ifdef XIM_CONNECTABLE
    if (!(!IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im)))
	im->core.ic_chain = NULL;
#else
    im->core.ic_chain = NULL;
#endif

    _XimUnregisterServerFilter(im);
    _XimResetIMInstantiateCallback(im);
    status = (Status)_XimClose(im);
    status = (Status)_XimDisconnect(im) && status;
    _XimProtoIMFree(im);
#ifdef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im)) {
	_XimReconnectModeSetAttr(im);
        for (ic = im->core.ic_chain; ic; ic = ic->core.next) {
	    _XimReconnectModeCreateIC(ic);
        }
	return 0;
    }
#endif /* XIM_CONNECTABLE */
    _XimDestroyIMStructureList(im);
    return status;
}

#ifdef XIM_CONNECTABLE
static Bool
_XimCheckIMQuarkList(
    XrmQuark		*quark_list,
    int			 num_quark,
    XrmQuark		 quark)
{
    register int	 i;

    for (i = 0; i < num_quark; i++) {
	if (quark_list[i] == quark) {
	    return True;
	}
    }
    return False;
}

static Bool
_XimSaveIMValues(
    Xim			 im,
    XIMArg		*arg)
{
    register XIMArg	*p;
    register int	 n;
    XrmQuark		*quark_list;
    XrmQuark		*tmp;
    XrmQuark		 quark;
    int			 num_quark;

    if (quark_list = im->private.proto.saved_imvalues) {
	num_quark = im->private.proto.num_saved_imvalues;
	for (p = arg; p && p->name; p++) {
	    quark = XrmStringToQuark(p->name);
	    if (_XimCheckIMQuarkList(quark_list, num_quark, quark)) {
		continue;
	    }
	    if (!(tmp = Xrealloc(quark_list,
				(sizeof(XrmQuark) * (num_quark + 1))))) {
		im->private.proto.saved_imvalues = quark_list;
		im->private.proto.num_saved_imvalues = num_quark;
		return False;
	    }
	    num_quark++;
	    quark_list = tmp;
	    quark_list[num_quark] = quark;
	}
	im->private.proto.saved_imvalues = quark_list;
	im->private.proto.num_saved_imvalues = num_quark;
	return True;
    }

    for (p = arg, n = 0; p && p->name; p++, n++);

    if (!(quark_list = Xmalloc(sizeof(XrmQuark) * n))) {
	return False;
    }

    im->private.proto.saved_imvalues = quark_list;
    im->private.proto.num_saved_imvalues = n;
    for (p = arg; p && p->name; p++, quark_list++) {
	*quark_list = XrmStringToQuark(p->name);
    }

    return True;
}

static char *
_XimDelayModeSetIMValues(
    Xim			 im,
    XIMArg		*arg)
{
    XimDefIMValues	 im_values;
    char		*name;
    XIMArg		*values;

    _XimGetCurrentIMValues(im, &im_values);
    name = _XimSetIMValueData(im, (XPointer)&im_values, values,
		im->core.im_resources, im->core.im_num_resources);
    _XimSetCurrentIMValues(im, &im_values);

    return name;
}
#endif /* XIM_CONNECTABLE */

static Bool
_XimSetIMValuesCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_SET_IM_VALUES_REPLY)
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

static char *
_XimProtoSetIMValues(
    XIM			 xim,
    XIMArg		*arg)
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    XIMArg		*arg_ret;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;
    char		*name;

#ifndef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im))
	return arg->name;
#else
    if (!_XimSaveIMValues(im, arg))
	return arg->name;

    if (!IS_SERVER_CONNECTED(im)) {
	if (IS_CONNECTABLE(im)) {
	    if (!_XimConnectServer(im)) {
	        return _XimDelayModeSetIMValues(im, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    _XimGetCurrentIMValues(im, &im_values);
    memset(tmp_buf, 0, sizeof(tmp_buf32));
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    arg_ret = arg;
    for (;;) {
	data = &buf[buf_size];
	if ((name = _XimEncodeIMATTRIBUTE(im, im->core.im_resources,
		im->core.im_num_resources, arg, &arg_ret, data, data_len,
		&ret_len, (XPointer)&im_values, XIM_SETIMVALUES))) {
	    break;
	}

	total += ret_len;
	if (!(arg = arg_ret)) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xcalloc(buf_size + data_len, 1))) {
		return arg->name;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return arg->name;
	    }
            memset(&tmp[buf_size], 0, data_len);
	    buf = tmp;
	}
    }
    _XimSetCurrentIMValues(im, &im_values);

    if (!total)
	return (char *)NULL;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_SET_IM_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return arg->name;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					 _XimSetIMValuesCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = (int)len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, reply, buf_size,
					_XimSetIMValuesCheck, 0);
	    if(ret_code != XIM_TRUE) {
		Xfree(preply);
		return arg->name;
	    }
	}
    } else
	return arg->name;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if(reply != preply)
	    Xfree(preply);
	return arg->name;
    }
    if(reply != preply)
	Xfree(preply);

    return name;
}

#ifdef XIM_CONNECTABLE
static char *
_XimDelayModeGetIMValues(
    Xim			 im,
    XIMArg		*arg)
{
    XimDefIMValues	 im_values;

    _XimGetCurrentIMValues(im, &im_values);
    return(_XimGetIMValueData(im, (XPointer)&im_values, arg,
			im->core.im_resources, im->core.im_num_resources));
}
#endif /* XIM_CONNECTABLE */

static Bool
_XimGetIMValuesCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_GET_IM_VALUES_REPLY)
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

static char *
_XimProtoGetIMValues(
    XIM			 xim,
    XIMArg		*arg)
{
    Xim			 im = (Xim)xim;
    register XIMArg	*p;
    register int	 n;
    CARD8		*buf;
    CARD16		*buf_s;
    INT16		 len;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply = NULL;
    int			 buf_size;
    int			 ret_code;
    char		*makeid_name;
    char		*decode_name;
    CARD16		*data = NULL;
    INT16		 data_len = 0;

#ifndef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im))
	return arg->name;
#else
    if (!IS_SERVER_CONNECTED(im)) {
	if (IS_CONNECTABLE(im)) {
	    if (!_XimConnectServer(im)) {
	        return _XimDelayModeGetIMValues(im, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    for (n = 0, p = arg; p->name; p++)
	n++;

    if (!n)
	return (char *)NULL;

    buf_size =  sizeof(CARD16) * n;
    buf_size += XIM_HEADER_SIZE
	     + sizeof(CARD16)
	     + sizeof(INT16)
	     + XIM_PAD(buf_size);

    if (!(buf = Xcalloc(buf_size, 1)))
	return arg->name;
    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    makeid_name = _XimMakeIMAttrIDList(im, im->core.im_resources,
				im->core.im_num_resources, arg,
				&buf_s[2], &len, XIM_GETIMVALUES);

    if (len) {
	buf_s[0] = im->private.proto.imid;	/* imid */
	buf_s[1] = len;				/* length of im-attr-id */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += sizeof(CARD16)			/* sizeof imid */
	     + sizeof(INT16);			/* sizeof length of attr */

	_XimSetHeader((XPointer)buf, XIM_GET_IM_VALUES, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf))) {
	    Xfree(buf);
	    return arg->name;
	}
	_XimFlush(im);
	Xfree(buf);
    	buf_size = BUFSIZE;
    	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    						_XimGetIMValuesCheck, 0);
	if(ret_code == XIM_TRUE) {
	    preply = reply;
	} else if(ret_code == XIM_OVERFLOW) {
	    if(len <= 0) {
		preply = reply;
	    } else {
		buf_size = len;
		preply = Xmalloc(buf_size);
		ret_code = _XimRead(im, &len, preply, buf_size,
						_XimGetIMValuesCheck, 0);
		if(ret_code != XIM_TRUE) {
		    Xfree(preply);
		    return arg->name;
		}
	    }
	} else
	    return arg->name;
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
	if (*((CARD8 *)preply) == XIM_ERROR) {
	    _XimProcError(im, 0, (XPointer)&buf_s[3]);
	    if(reply != preply)
		Xfree(preply);
	    return arg->name;
	}
	data = &buf_s[2];
	data_len = buf_s[1];
    }
    decode_name = _XimDecodeIMATTRIBUTE(im, im->core.im_resources,
			im->core.im_num_resources, data, data_len,
			arg, XIM_GETIMVALUES);
    if (reply != preply)
	Xfree(preply);

    if (decode_name)
	return decode_name;
    else
	return makeid_name;
}

static XIMMethodsRec     im_methods = {
    _XimProtoCloseIM,           /* close */
    _XimProtoSetIMValues,       /* set_values */
    _XimProtoGetIMValues,       /* get_values */
    _XimProtoCreateIC,          /* create_ic */
    _Ximctstombs,		/* ctstombs */
    _Ximctstowcs,		/* ctstowcs */
    _Ximctstoutf8		/* ctstoutf8 */
};

static Bool
_XimSetEncodingByName(
    Xim		  im,
    char	**buf,
    int		 *len)
{
    char	*encoding = (char *)NULL;
    int		 encoding_len;
    int		 compound_len;
    BYTE	*ret;

    _XGetLCValues(im->core.lcd, XlcNCodeset, &encoding, NULL);
    if (!encoding) {
	*buf = (char *)NULL;
	*len = 0;
	return True;
    }
    encoding_len = strlen(encoding);
    compound_len = strlen("COMPOUND_TEXT");
    *len = encoding_len + sizeof(BYTE) + compound_len + sizeof(BYTE);
    if (!(ret = Xmalloc(*len))) {
	return False;
    }
    *buf = (char *)ret;

    ret[0] = (BYTE)encoding_len;
    memcpy(&ret[1], encoding, encoding_len);
    ret += (encoding_len + sizeof(BYTE));
    ret[0] = (BYTE)compound_len;
    memcpy(&ret[1], "COMPOUND_TEXT", compound_len);
    return True;
}

static Bool
_XimSetEncodingByDetail(
    Xim		 im,
    char	**buf,
    int		 *len)
{
    *len = 0;
    *buf = NULL;
    return True;
}

static Bool
_XimGetEncoding(
    Xim		 im,
    CARD16	*buf,
    char	*name,
    int		 name_len,
    char	*detail,
    int		 detail_len)
{
    XLCd	 lcd = im->core.lcd;
    CARD16	 category = buf[0];
    CARD16	 idx = buf[1];
    int		 len;
    XlcConv	 ctom_conv = NULL;
    XlcConv	 ctow_conv = NULL;
    XlcConv	 ctoutf8_conv = NULL;
    XlcConv	 conv;
    XimProtoPrivateRec *private = &im->private.proto;

    if (idx == (CARD16)XIM_Default_Encoding_IDX) { /* XXX */
	if (!(ctom_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNMultiByte)))
	    return False;
	if (!(ctow_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNWideChar)))
	    return False;
	if (!(ctoutf8_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNUtf8String)))
	    return False;
    }

    if (category == XIM_Encoding_NameCategory) {
	while (name_len > 0) {
	    len = (int)name[0];
	    if (!strncmp(&name[1], "COMPOUND_TEXT", len)) {
		if (!(ctom_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNMultiByte)))
		    return False;
		if (!(ctow_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNWideChar)))
		    return False;
		if (!(ctoutf8_conv = _XlcOpenConverter(lcd,
				 XlcNCompoundText, lcd, XlcNUtf8String)))
		    return False;
		break;
	    } else {
		/*
		 * Not yet
		 */
	    }
	    len += sizeof(BYTE);
	    name_len -= len;
	    name += len;
	}
    } else if (category == XIM_Encoding_DetailCategory) {
	/*
	 * Not yet
	 */
    } else {
	return False;
    }

    private->ctom_conv = ctom_conv;
    private->ctow_conv = ctow_conv;
    private->ctoutf8_conv = ctoutf8_conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNMultiByte)))
	return False;
    private->cstomb_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNWideChar)))
	return False;
    private->cstowc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNUtf8String)))
	return False;
    private->cstoutf8_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNChar)))
	return False;
    private->ucstoc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNUtf8String)))
	return False;
    private->ucstoutf8_conv = conv;

    return True;
}

static Bool
_XimEncodingNegoCheck(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_ENCODING_NEGOTIATION_REPLY)
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

static Bool
_XimEncodingNegotiation(
    Xim		 im)
{
    char	*name_ptr = 0;
    int		 name_len = 0;
    char	*detail_ptr = 0;
    int		 detail_len = 0;
    CARD8	*buf;
    CARD16	*buf_s;
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;

    if (!(_XimSetEncodingByName(im, &name_ptr, &name_len)))
	return False;

    if (!(_XimSetEncodingByDetail(im, &detail_ptr, &detail_len)))
	goto free_name_ptr;

    len = sizeof(CARD16)
	+ sizeof(INT16)
	+ name_len
	+ XIM_PAD(name_len)
	+ sizeof(INT16)
	+ sizeof(CARD16)
	+ detail_len;

    if (!(buf = Xcalloc(XIM_HEADER_SIZE + len, 1)))
	goto free_detail_ptr;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)name_len;
    if (name_ptr)
	(void)memcpy((char *)&buf_s[2], name_ptr, name_len);
    XIM_SET_PAD(&buf_s[2], name_len);
    buf_s = (CARD16 *)((char *)&buf_s[2] + name_len);
    buf_s[0] = detail_len;
    buf_s[1] = 0;
    if (detail_ptr)
	(void)memcpy((char *)&buf_s[2], detail_ptr, detail_len);

    _XimSetHeader((XPointer)buf, XIM_ENCODING_NEGOTIATION, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	Xfree(buf);
	goto free_detail_ptr;
    }
    _XimFlush(im);
    Xfree(buf);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimEncodingNegoCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
					_XimEncodingNegoCheck, 0);
	    if(ret_code != XIM_TRUE)
		goto free_preply;
	}
    } else
	goto free_detail_ptr;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	goto free_preply;
    }

    if (!(_XimGetEncoding(im, &buf_s[1], name_ptr, name_len,
						detail_ptr, detail_len)))
	goto free_preply;

    Xfree(name_ptr);
    Xfree(detail_ptr);

    if(reply != preply)
	Xfree(preply);

    return True;

free_preply:
    if (reply != preply)
	Xfree(preply);

free_detail_ptr:
    Xfree(detail_ptr);

free_name_ptr:
    Xfree(name_ptr);

    return False;
}

#ifdef XIM_CONNECTABLE
static Bool
_XimSendSavedIMValues(
    Xim			 im)
{
    XimDefIMValues	 im_values;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    int			 idx;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;

    _XimGetCurrentIMValues(im, &im_values);
    memset(tmp_buf, 0, sizeof(tmp_buf32));
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    idx = 0;
    for (;;) {
	data = &buf[buf_size];
	if (!_XimEncodeSavedIMATTRIBUTE(im, im->core.im_resources,
		im->core.im_num_resources, &idx, data, data_len,
		&ret_len, (XPointer)&im_values, XIM_SETIMVALUES)) {
	    if (buf != tmp_buf)
		Xfree(buf);
	    return False;
	}

	total += ret_len;
	if (idx == -1) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xcalloc(buf_size + data_len, 1))) {
		return False;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return False;
	    }
            memset(&tmp[buf_size], 0, data_len);
	    buf = tmp;
	}
    }

    if (!total)
	return True;

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_SET_IM_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return False;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    					 _XimSetIMValuesCheck, 0);
    if(ret_code == XIM_TRUE) {
	preply = reply;
    } else if(ret_code == XIM_OVERFLOW) {
	if(len <= 0) {
	    preply = reply;
	} else {
	    buf_size = (int)len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, reply, buf_size,
					_XimSetIMValuesCheck, 0);
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

    return True;
}

static void
_XimDelayModeIMFree(
    Xim		 im)
{
    if (im->core.im_resources) {
	Xfree(im->core.im_resources);
	im->core.im_resources = NULL;
    }
    if (im->core.ic_resources) {
	Xfree(im->core.ic_resources);
	im->core.ic_resources = NULL;
    }
    if (im->core.im_values_list) {
	Xfree(im->core.im_values_list);
	im->core.im_values_list = NULL;
    }
    if (im->core.ic_values_list) {
	Xfree(im->core.ic_values_list);
	im->core.ic_values_list = NULL;
    }
    return;
}

Bool
_XimConnectServer(
    Xim		 im)
{
    Xim		 save_im;

    if (!(save_im = Xmalloc(sizeof(XimRec))))
	return False;
    memcpy((char *)save_im, (char *)im, sizeof(XimRec));

    if (_XimPreConnect(im) && _XimConnection(im)
			&& _XimOpen(im) && _XimEncodingNegotiation(im)) {
	if (_XimSendSavedIMValues(im)) {
	    _XimDelayModeIMFree(save_im);
	    _XimRegisterServerFilter(im);
	    Xfree(save_im);
	    return True;
	}
    }
    memcpy((char *)im, (char *)save_im, sizeof(XimRec));
    Xfree(save_im);
    return False;
}

Bool
_XimDelayModeSetAttr(
    Xim			 im)
{
    XimDefIMValues	 im_values;

    if(!_XimSetIMResourceList(&im->core.im_resources,
						&im->core.im_num_resources)) {
	return False;
    }
    if(!_XimSetICResourceList(&im->core.ic_resources,
						&im->core.ic_num_resources)) {
	return False;
    }

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);

    _XimGetCurrentIMValues(im, &im_values);
    if(!_XimSetLocalIMDefaults(im, (XPointer)&im_values,
			im->core.im_resources, im->core.im_num_resources)) {
	return False;
    }
    _XimSetCurrentIMValues(im, &im_values);
    if (im->private.proto.default_styles) {
        if (im->core.styles)
	    Xfree(im->core.styles);
        im->core.styles = im->private.proto.default_styles;
    }

    return True;
}

static Bool
_XimReconnectModeSetAttr(
    Xim			 im)
{
    XimDefIMValues	 im_values;

    if(!_XimSetIMResourceList(&im->core.im_resources,
						&im->core.im_num_resources)) {
	return False;
    }
    if(!_XimSetICResourceList(&im->core.ic_resources,
						&im->core.ic_num_resources)) {
	return False;
    }

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);

    if (im->private.proto.default_styles) {
        if (im->core.styles)
	    Xfree(im->core.styles);
        im->core.styles = im->private.proto.default_styles;
    }

    return True;
}
#endif /* XIM_CONNECTABLE */

Bool
_XimProtoOpenIM(
    Xim		 im)
{
    _XimInitialResourceInfo();

    im->methods = &im_methods;

#ifdef XIM_CONNECTABLE
    _XimSetProtoResource(im);
#endif /* XIM_CONNECTABLE */

    if (_XimPreConnect(im)) {
	if (_XimConnection(im) && _XimOpen(im) && _XimEncodingNegotiation(im)) {
	    _XimRegisterServerFilter(im);
	    return True;
	}
	_XimShutdown(im);
#ifdef XIM_CONNECTABLE
    } else if (IS_DELAYBINDABLE(im)) {
	if (_XimDelayModeSetAttr(im))
	    return True;
#endif /* XIM_CONNECTABLE */
    }
    _XimProtoIMFree(im);
    return False;
}
