/******************************************************************

           Copyright 1992, 1993 by FUJITSU LIMITED

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

#ifndef _XIMPROTO_H
#define _XIMPROTO_H

/*
 * Default Preconnection selection target
 */
#define XIM_SERVERS		"XIM_SERVERS"
#define XIM_LOCALES		"LOCALES"
#define XIM_TRANSPORT		"TRANSPORT"

/*
 * categories in XIM_SERVERS
 */
#define XIM_SERVER_CATEGORY	"@server="
#define XIM_LOCAL_CATEGORY	"@locale="
#define XIM_TRANSPORT_CATEGORY	"@transport="

/*
 * Xim implementation revision
 */
#define PROTOCOLMAJORVERSION		1
#define PROTOCOLMINORVERSION		0

/*
 * Major Protocol number
 */
#define	XIM_CONNECT			  1
#define	XIM_CONNECT_REPLY		  2
#define	XIM_DISCONNECT			  3
#define	XIM_DISCONNECT_REPLY		  4

#define XIM_AUTH_REQUIRED		 10
#define XIM_AUTH_REPLY			 11
#define XIM_AUTH_NEXT			 12
#define XIM_AUTH_SETUP			 13
#define XIM_AUTH_NG			 14

#define	XIM_ERROR			 20

#define	XIM_OPEN			 30
#define	XIM_OPEN_REPLY			 31
#define	XIM_CLOSE			 32
#define	XIM_CLOSE_REPLY			 33
#define	XIM_REGISTER_TRIGGERKEYS	 34
#define	XIM_TRIGGER_NOTIFY		 35
#define	XIM_TRIGGER_NOTIFY_REPLY	 36
#define	XIM_SET_EVENT_MASK		 37
#define	XIM_ENCODING_NEGOTIATION	 38
#define	XIM_ENCODING_NEGOTIATION_REPLY	 39
#define	XIM_QUERY_EXTENSION		 40
#define	XIM_QUERY_EXTENSION_REPLY	 41
#define	XIM_SET_IM_VALUES		 42
#define	XIM_SET_IM_VALUES_REPLY		 43
#define	XIM_GET_IM_VALUES		 44
#define	XIM_GET_IM_VALUES_REPLY		 45

#define XIM_CREATE_IC			 50
#define	XIM_CREATE_IC_REPLY		 51
#define	XIM_DESTROY_IC			 52
#define	XIM_DESTROY_IC_REPLY		 53
#define XIM_SET_IC_VALUES		 54
#define	XIM_SET_IC_VALUES_REPLY		 55
#define XIM_GET_IC_VALUES		 56
#define XIM_GET_IC_VALUES_REPLY		 57
#define	XIM_SET_IC_FOCUS		 58
#define	XIM_UNSET_IC_FOCUS		 59
#define	XIM_FORWARD_EVENT		 60
#define	XIM_SYNC			 61
#define	XIM_SYNC_REPLY			 62
#define	XIM_COMMIT			 63
#define	XIM_RESET_IC			 64
#define	XIM_RESET_IC_REPLY		 65

#define	XIM_GEOMETRY			 70
#define	XIM_STR_CONVERSION		 71
#define	XIM_STR_CONVERSION_REPLY	 72
#define	XIM_PREEDIT_START		 73
#define	XIM_PREEDIT_START_REPLY		 74
#define	XIM_PREEDIT_DRAW		 75
#define	XIM_PREEDIT_CARET		 76
#define XIM_PREEDIT_CARET_REPLY		 77
#define	XIM_PREEDIT_DONE		 78
#define	XIM_STATUS_START		 79
#define	XIM_STATUS_DRAW			 80
#define	XIM_STATUS_DONE			 81
#define	XIM_PREEDITSTATE		 82

/*
 * values for the flag of XIM_ERROR
 */
#define	XIM_IMID_VALID			0x0001
#define	XIM_ICID_VALID			0x0002

/*
 * XIM Error Code
 */
#define XIM_BadAlloc			1
#define XIM_BadStyle			2
#define XIM_BadClientWindow		3
#define XIM_BadFocusWindow		4
#define XIM_BadArea			5
#define XIM_BadSpotLocation		6
#define XIM_BadColormap			7
#define XIM_BadAtom			8
#define XIM_BadPixel			9
#define XIM_BadPixmap			10
#define XIM_BadName			11
#define XIM_BadCursor			12
#define XIM_BadProtocol			13
#define XIM_BadForeground		14
#define XIM_BadBackground		15
#define XIM_LocaleNotSupported		16
#define XIM_BadSomething		999

/*
 * byte order
 */
#define BIGENDIAN	(CARD8)0x42	/* MSB first */
#define LITTLEENDIAN	(CARD8)0x6c	/* LSB first */

/*
 * values for the type of XIMATTR & XICATTR
 */
#define	XimType_SeparatorOfNestedList	0
#define	XimType_CARD8			1
#define	XimType_CARD16			2
#define	XimType_CARD32			3
#define	XimType_STRING8			4
#define	XimType_Window			5
#define	XimType_XIMStyles		10
#define	XimType_XRectangle		11
#define	XimType_XPoint			12
#define XimType_XFontSet		13
#define XimType_XIMOptions		14
#define XimType_XIMHotKeyTriggers	15
#define XimType_XIMHotKeyState		16
#define XimType_XIMStringConversion	17
#define	XimType_NEST			0x7fff

/*
 * values for the category of XIM_ENCODING_NEGITIATON_REPLY
 */
#define	XIM_Encoding_NameCategory	0
#define	XIM_Encoding_DetailCategory	1

/*
 * value for the index of XIM_ENCODING_NEGITIATON_REPLY
 */
#define	XIM_Default_Encoding_IDX	-1

/*
 * value for the flag of XIM_FORWARD_EVENT, XIM_COMMIT
 */
#define XimSYNCHRONUS		  0x0001
#define XimLookupChars		  0x0002
#define XimLookupKeySym		  0x0004
#define XimLookupBoth		  0x0006

/*
 * request packet header size
 */
#define XIM_HEADER_SIZE						\
	( sizeof(CARD8)		/* sizeof major-opcode */	\
	+ sizeof(CARD8)		/* sizeof minor-opcode */	\
	+ sizeof(INT16)	)	/* sizeof length */

/*
 * Client Message data size
 */
#define	XIM_CM_DATA_SIZE	20

/*
 * XIM data structure
 */
typedef CARD16	BITMASK16;
typedef CARD32	BITMASK32;
typedef CARD32	EVENTMASK;

typedef CARD16	XIMID;		/* Input Method ID */
typedef CARD16	XICID;		/* Input Context ID */

/*
 * Padding macro
 */
#define	XIM_PAD(length) ((4 - ((length) % 4)) % 4)

#define XIM_SET_PAD(ptr, length)					\
    {									\
	register int	 Counter = XIM_PAD((int)length);		\
	if (Counter) {							\
	    register char	*Ptr = (char *)(ptr) + (length);	\
	    length += Counter;						\
	    for (; Counter; --Counter, ++Ptr)				\
		*Ptr = '\0';						\
	}								\
    }

#endif /* _XIMPROTO_H */
