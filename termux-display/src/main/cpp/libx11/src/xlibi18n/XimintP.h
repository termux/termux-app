/*
 * Copyright (c) 1991, 1992, Oracle and/or its affiliates.
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
           Copyright 1993, 1994       by Sony Corporation

Permission to use, copy, modify, distribute, and sell this software and
its documentation for any purpose is hereby granted without fee, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of FUJITSU LIMITED and Sony Corporation
not be used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  FUJITSU LIMITED and
Sony Corporation makes no representations about the suitability of this
software for any purpose.  It is provided "as is" without express or
implied warranty.

FUJITSU LIMITED AND SONY CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL FUJITSU LIMITED OR SONY CORPORATION BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
USE OR PERFORMANCE OF THIS SOFTWARE.

  Author: Hideki Hiura (hhiura@Sun.COM) Sun Microsystems, Inc.
          Takashi Fujiwara     FUJITSU LIMITED
                                 fujiwara@a80.tech.yk.fujitsu.co.jp
	  Makoto Wakamatsu     Sony Corporation
                                 makoto@sm.sony.co.jp
	  Hiroyuki Miyamoto    Digital Equipment Corporation
                                 miyamoto@jrd.dec.com

******************************************************************/

#ifndef _XIMINTP_H
#define _XIMINTP_H

#include "XimProto.h"
#include "XlcPublic.h"

/*
 * for protocol layer callback function
 */
typedef Bool (*XimProtoIntrProc)(
	Xim, INT16, XPointer, XPointer
);
typedef struct _XimProtoIntrRec {
    XimProtoIntrProc		 func;
    CARD16			 major_code;
    CARD16			 minor_code;
    XPointer			 call_data;
    struct _XimProtoIntrRec	*next;
} XimProtoIntrRec;

/*
 * for transport layer methods
 */
typedef Bool (*XimTransConnectProc)(
	 Xim
);
typedef Bool (*XimTransShutdownProc)(
	 Xim
);
typedef Bool (*XimTransWriteProc)(
	 Xim, INT16, XPointer
);
typedef Bool (*XimTransReadProc)(
	 Xim, XPointer, int, int *
);
typedef void (*XimTransFlushProc)(
	 Xim
);
typedef Bool (*XimTransRegDispatcher)(
	 Xim, Bool (*)(Xim, INT16, XPointer, XPointer), XPointer
);
typedef Bool (*XimTransCallDispatcher)(
	 Xim, INT16, XPointer
);

/*
 * private part of IM
 */
typedef struct _XimProtoPrivateRec {
    /* The first fields are identical with XimCommonPrivateRec. */
    XlcConv			 ctom_conv;
    XlcConv			 ctow_conv;
    XlcConv			 ctoutf8_conv;
    XlcConv			 cstomb_conv;
    XlcConv			 cstowc_conv;
    XlcConv			 cstoutf8_conv;
    XlcConv			 ucstoc_conv;
    XlcConv			 ucstoutf8_conv;

    Window			 im_window;
    XIMID			 imid;
    CARD16			 unused;
    XIMStyles			*default_styles;
    CARD32			*im_onkeylist;
    CARD32			*im_offkeylist;
    BITMASK32			 flag;

    BITMASK32			 registed_filter_event;
    EVENTMASK			 forward_event_mask;
    EVENTMASK			 synchronous_event_mask;

    XimProtoIntrRec		*intrproto;
    XIMResourceList		 im_inner_resources;
    unsigned int		 im_num_inner_resources;
    XIMResourceList		 ic_inner_resources;
    unsigned int		 ic_num_inner_resources;
    char			*hold_data;
    int				 hold_data_len;
    char			*locale_name;
    CARD16			 protocol_major_version;
    CARD16			 protocol_minor_version;
    XrmQuark			*saved_imvalues;
    int				 num_saved_imvalues;

    /*
     * transport specific
     */
    XimTransConnectProc		 connect;
    XimTransShutdownProc	 shutdown;
    XimTransWriteProc		 write;
    XimTransReadProc		 read;
    XimTransFlushProc		 flush;
    XimTransRegDispatcher	 register_dispatcher;
    XimTransCallDispatcher	 call_dispatcher;
    XPointer			 spec;
} XimProtoPrivateRec;

/*
 * bit mask for the flag of XIMPrivateRec
 */
#define SERVER_CONNECTED	(1L)
#define	DYNAMIC_EVENT_FLOW	(1L << 1)
#define	USE_AUTHORIZATION_FUNC	(1L << 2)
#ifdef XIM_CONNECTABLE
#define DELAYBINDABLE		(1L << 3)
#define RECONNECTABLE		(1L << 4)
#endif /* XIM_CONNECTABLE */
#define FABRICATED		(1L << 5)
#define NEED_SYNC_REPLY		(1L << 6)

/*
 * macro for the flag of XIMPrivateRec
 */
#define IS_SERVER_CONNECTED(im) \
		((((Xim)im))->private.proto.flag & SERVER_CONNECTED)
#define MARK_SERVER_CONNECTED(im) \
		((((Xim)im))->private.proto.flag |= SERVER_CONNECTED)
#define UNMARK_SERVER_CONNECTED(im) \
		((((Xim)im))->private.proto.flag &= ~SERVER_CONNECTED)

#define	IS_DYNAMIC_EVENT_FLOW(im) \
		(((Xim)im)->private.proto.flag & DYNAMIC_EVENT_FLOW)
#define	MARK_DYNAMIC_EVENT_FLOW(im) \
		(((Xim)im)->private.proto.flag |= DYNAMIC_EVENT_FLOW)

#define	IS_USE_AUTHORIZATION_FUNC(im) \
		(((Xim)im)->private.proto.flag & USE_AUTHORIZATION_FUNC)
#define	MARK_USE_AUTHORIZATION_FUNC(im) \
		(((Xim)im)->private.proto.flag |= USE_AUTHORIZATION_FUNC)

#ifdef XIM_CONNECTABLE
#define IS_DELAYBINDABLE(im) \
		(((Xim)im)->private.proto.flag & DELAYBINDABLE)
#define MARK_DELAYBINDABLE(im) \
		(((Xim)im)->private.proto.flag |= DELAYBINDABLE)

#define IS_RECONNECTABLE(im) \
		(((Xim)im)->private.proto.flag & RECONNECTABLE)
#define MARK_RECONNECTABLE(im) \
		(((Xim)im)->private.proto.flag |= RECONNECTABLE)

#define IS_CONNECTABLE(im) \
    (((Xim)im)->private.proto.flag & (DELAYBINDABLE|RECONNECTABLE))
#define UNMAKE_CONNECTABLE(im) \
    (((Xim)im)->private.proto.flag &= ~(DELAYBINDABLE|RECONNECTABLE))
#endif /* XIM_CONNECTABLE */

#define IS_FABRICATED(im) \
		(((Xim)im)->private.proto.flag & FABRICATED)
#define MARK_FABRICATED(im) \
		(((Xim)im)->private.proto.flag |= FABRICATED)
#define UNMARK_FABRICATED(im) \
		(((Xim)im)->private.proto.flag &= ~FABRICATED)

#define IS_NEED_SYNC_REPLY(im) \
		(((Xim)im)->private.proto.flag & NEED_SYNC_REPLY)
#define MARK_NEED_SYNC_REPLY(im) \
		(((Xim)im)->private.proto.flag |= NEED_SYNC_REPLY)
#define UNMARK_NEED_SYNC_REPLY(im) \
		(((Xim)im)->private.proto.flag &= ~NEED_SYNC_REPLY)

/*
 * bit mask for the register_filter_event of XIMPrivateRec/XICPrivateRec
 */
#define KEYPRESS_MASK		(1L)
#define KEYRELEASE_MASK		(1L << 1)
#define DESTROYNOTIFY_MASK	(1L << 2)

typedef struct _XimCommitInfoRec {
    struct _XimCommitInfoRec	*next;
    char			*string;
    int				 string_len;
    KeySym			*keysym;
    int				 keysym_len;
} XimCommitInfoRec, *XimCommitInfo;

typedef struct _XimPendingCallback {
    int				 major_opcode;
    Xim				 im;
    Xic				 ic;
    char			*proto;
    int				 proto_len;
    struct _XimPendingCallback	*next;
} XimPendingCallbackRec, *XimPendingCallback;


/*
 * private part of IC
 */
typedef struct _XicProtoPrivateRec {
    XICID	         icid;			/* ICID		*/
    CARD16		 dmy;
    BITMASK32		 flag;			/* Input Mode	*/

    BITMASK32		 registed_filter_event; /* registed filter mask */
    EVENTMASK		 forward_event_mask;	/* forward event mask */
    EVENTMASK		 synchronous_event_mask;/* sync event mask */
    EVENTMASK		 filter_event_mask;	/* negrect event mask */
    EVENTMASK		 intercept_event_mask;	/* deselect event mask */
    EVENTMASK		 select_event_mask;	/* select event mask */

    char		*preedit_font;		/* Base font name list */
    int			 preedit_font_length;	/* length of base font name */
    char		*status_font;		/* Base font name list */
    int			 status_font_length;	/* length of base font name */

    XimCommitInfo	 commit_info;
    XIMResourceList	 ic_resources;
    unsigned int	 ic_num_resources;
    XIMResourceList	 ic_inner_resources;
    unsigned int	 ic_num_inner_resources;
    XrmQuark		*saved_icvalues;
    int			 num_saved_icvalues;
    XimPendingCallback	 pend_cb_que;
    Bool		 waitCallback;
} XicProtoPrivateRec ;

/*
 * bit mask for the flag of XICPrivateRec
 */
#define IC_CONNECTED		(1L)

/*
 * macro for the flag of XICPrivateRec
 */
#define	IS_IC_CONNECTED(ic) \
		(((Xic)ic)->private.proto.flag & IC_CONNECTED)
#define	MARK_IC_CONNECTED(ic) \
		(((Xic)ic)->private.proto.flag |= IC_CONNECTED)
#define	UNMARK_IC_CONNECTED(ic) \
		(((Xic)ic)->private.proto.flag &= ~IC_CONNECTED)

/*
 * macro for the filter_event_mask of XICPrivateRec
 */
#define	IS_NEGLECT_EVENT(ic, mask) \
		(((Xic)ic)->private.proto.filter_event_mask & (mask))

/*
 * macro for the forward_event_mask of XICPrivateRec
 */
#define	IS_FORWARD_EVENT(ic, mask) \
		(((Xic)ic)->private.proto.forward_event_mask & (mask))

/*
 * macro for the synchronous_event_mask of XICPrivateRec
 */
#define	IS_SYNCHRONOUS_EVENT(ic, mask) \
   ((((Xic)ic)->private.proto.synchronous_event_mask & (mask)) ? True: False)

#define XIM_MAXIMNAMELEN 64
#define XIM_MAXLCNAMELEN 64

#endif /* _XIMINTP_H */
