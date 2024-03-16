/******************************************************************

                Copyright 1992, 1993, 1994 by FUJITSU LIMITED
                Copyright 1993, 1994 by Sony Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED and
Sony Corporation not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.  FUJITSU LIMITED and Sony Corporation makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

FUJITSU LIMITED AND SONY CORPORATION DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJITSU LIMITED AND
SONY CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author:   Takashi Fujiwara   FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp
  Motifier: Makoto Wakamatsu   Sony Corporation
			       makoto@sm.sony.co.jp

******************************************************************/

#ifndef _XIMINT_H
#define _XIMINT_H

#include <stdio.h>
#include <X11/Xutil.h>

typedef struct _Xim	*Xim;
typedef struct _Xic	*Xic;

/*
 * Input Method data
 */
#include "XimintP.h"
#include "XimintL.h"

/*
 * XIM dependent data
 */

typedef struct _XimCommonPrivateRec {
    /* This struct is also inlined in XimLocalPrivateRec, XimProtoPrivateRec. */
    XlcConv		ctom_conv;
    XlcConv		ctow_conv;
    XlcConv		ctoutf8_conv;
    XlcConv		cstomb_conv;
    XlcConv		cstowc_conv;
    XlcConv		cstoutf8_conv;
    XlcConv		ucstoc_conv;
    XlcConv		ucstoutf8_conv;
} XimCommonPrivateRec;

typedef union _XIMPrivateRec {
    XimCommonPrivateRec  common;
    XimLocalPrivateRec   local;
    XimProtoPrivateRec   proto;
} XIMPrivateRec;

/*
 * IM struct
 */
typedef struct _Xim {
    XIMMethods		methods;
    XIMCoreRec		core;
    XIMPrivateRec	private;
} XimRec;

/*
 * IC deprndent data
 */
typedef union _XICPrivateRec {
    XicLocalPrivateRec   local;
    XicProtoPrivateRec   proto;
} XICPrivateRec;

/*
 * IC struct
 */
typedef struct _Xic {
	XICMethods	methods;
	XICCoreRec	core;
	XICPrivateRec	private;
} XicRec;

typedef struct _XimDefIMValues {
	XIMValuesList		*im_values_list;
	XIMValuesList		*ic_values_list;
	XIMStyles		*styles;
	XIMCallback		 destroy_callback;
	char			*res_name;
	char			*res_class;
	Bool			 visible_position;
} XimDefIMValues;

typedef struct _XimDefICValues {
    XIMStyle			 input_style;
    Window			 client_window;
    Window			 focus_window;
    unsigned long		 filter_events;
    XICCallback			 geometry_callback;
    char			*res_name;
    char			*res_class;
    XICCallback			 destroy_callback;
    XICCallback			 preedit_state_notify_callback;
    XICCallback			 string_conversion_callback;
    XIMStringConversionText	 string_conversion;
    XIMResetState		 reset_state;
    XIMHotKeyTriggers		*hotkey;
    XIMHotKeyState		 hotkey_state;
    ICPreeditAttributes		 preedit_attr;
    ICStatusAttributes		 status_attr;
} XimDefICValues;

#define XIM_MODE_IM_GET		(1 << 0)
#define XIM_MODE_IM_SET		(1 << 1)
#define XIM_MODE_IM_DEFAULT	(1 << 2)

#define XIM_MODE_PRE_GET	(1 << 0)
#define XIM_MODE_PRE_SET	(1 << 1)
#define XIM_MODE_PRE_CREATE	(1 << 2)
#define XIM_MODE_PRE_ONCE	(1 << 3)
#define XIM_MODE_PRE_DEFAULT	(1 << 4)

#define XIM_MODE_STS_GET	(1 << 5)
#define XIM_MODE_STS_SET	(1 << 6)
#define XIM_MODE_STS_CREATE	(1 << 7)
#define XIM_MODE_STS_ONCE	(1 << 8)
#define XIM_MODE_STS_DEFAULT	(1 << 9)

#define XIM_MODE_IC_GET		(XIM_MODE_PRE_GET      | XIM_MODE_STS_GET)
#define XIM_MODE_IC_SET		(XIM_MODE_PRE_SET      | XIM_MODE_STS_SET)
#define XIM_MODE_IC_CREATE	(XIM_MODE_PRE_CREATE   | XIM_MODE_STS_CREATE)
#define XIM_MODE_IC_ONCE	(XIM_MODE_PRE_ONCE     | XIM_MODE_STS_ONCE)
#define XIM_MODE_IC_DEFAULT	(XIM_MODE_PRE_DEFAULT  | XIM_MODE_STS_DEFAULT)

#define XIM_MODE_PRE_MASK	(XIM_MODE_PRE_GET    | XIM_MODE_PRE_SET    | \
				 XIM_MODE_PRE_CREATE | XIM_MODE_PRE_ONCE   | \
				 XIM_MODE_PRE_DEFAULT)
#define XIM_MODE_STS_MASK	(XIM_MODE_STS_GET    | XIM_MODE_STS_SET    | \
				 XIM_MODE_STS_CREATE | XIM_MODE_STS_ONCE   | \
				 XIM_MODE_STS_DEFAULT)

#define XIM_SETIMDEFAULTS	(1L << 0)
#define XIM_SETIMVALUES		(1L << 1)
#define XIM_GETIMVALUES		(1L << 2)

#define XIM_SETICDEFAULTS	(1L << 0)
#define XIM_CREATEIC		(1L << 1)
#define XIM_SETICVALUES		(1L << 2)
#define XIM_GETICVALUES		(1L << 3)
#define XIM_PREEDIT_ATTR	(1L << 4)
#define XIM_STATUS_ATTR		(1L << 5)

#define XIM_CHECK_VALID		0
#define XIM_CHECK_INVALID	1
#define XIM_CHECK_ERROR		2

#define FILTERD         True
#define NOTFILTERD      False

#define XIMMODIFIER		"@im="

#define	XIM_TRUE	True
#define	XIM_FALSE	False
#define	XIM_OVERFLOW	(-1)

#define BRL_UC_ROW	0x2800

/*
 * Global symbols
 */

XPointer _XimGetLocaleCode (
    const char	*encoding_name
);

int _XimGetCharCode (
    XPointer		conv,
    KeySym		keysym,
    unsigned char	*buf,
    int			nbytes
);

unsigned int KeySymToUcs4 (
    KeySym		keysym
);

extern Bool _XimSetIMResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num
);

extern Bool _XimSetICResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num
);

extern Bool _XimSetInnerIMResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num
);

extern Bool _XimSetInnerICResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num
);

extern Bool _XimCheckCreateICValues(
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern XIMResourceList _XimGetResourceListRec(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    const char		*name
);

extern void _XimSetIMMode(
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern void _XimSetICMode(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XIMStyle		 style
);

extern int _XimCheckIMMode(
    XIMResourceList	 res_list,
    unsigned long	 mode
);

extern int _XimCheckICMode(
    XIMResourceList	 res_list,
    unsigned long	 mode
);

extern Bool _XimSetLocalIMDefaults(
    Xim			 im,
    XPointer		 top,
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern Bool _XimSetICDefaults(
    Xic			 ic,
    XPointer		 top,
    unsigned long	 mode,
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern Bool _XimEncodeLocalIMAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val
);

extern Bool _XimEncodeLocalICAttr(
    Xic			 ic,
    XIMResourceList	 res,
    XPointer		 top,
    XIMArg		*arg,
    unsigned long	 mode
);

extern Bool _XimCheckLocalInputStyle(
    Xic			 ic,
    XPointer		 top,
    XIMArg		*values,
    XIMStyles           *styles,
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern Bool _XimDecodeLocalIMAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val
);

extern Bool _XimDecodeLocalICAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val,
    unsigned long	mode
);

extern void _XimGetCurrentIMValues(
    Xim			 im,
    XimDefIMValues	*im_values
);

extern void _XimSetCurrentIMValues(
    Xim			 im,
    XimDefIMValues	*im_values
);

extern void _XimGetCurrentICValues(
    Xic			 ic,
    XimDefICValues	*ic_values
);

extern void _XimSetCurrentICValues(
    Xic			 ic,
    XimDefICValues	*ic_values
);

extern void _XimInitialResourceInfo(
    void
);

extern void	 _XimParseStringFile(
    FILE        *fp,
    Xim          im
);

extern Bool	 _XimCheckIfLocalProcessing(
    Xim		 im
);

extern Bool	 _XimCheckIfThaiProcessing(
    Xim		 im
);

extern Bool	 _XimLocalOpenIM(
    Xim		 im
);

extern Bool	 _XimThaiOpenIM(
    Xim		 im
);

extern Bool	 _XimProtoOpenIM(
    Xim		 im
);

extern void	 _XimLocalIMFree(
    Xim		 im
);

extern void	 _XimThaiIMFree(
    Xim	         im
);

extern void	 _XimProtoIMFree(
    Xim		 im
);

extern char *	 _XimSetIMValueData(
    Xim			 im,
    XPointer		 top,
    XIMArg		*arg,
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern char *	 _XimGetIMValueData(
    Xim			 im,
    XPointer		 top,
    XIMArg		*arg,
    XIMResourceList	 res_list,
    unsigned int	 list_num
);

extern char *	 _XimSetICValueData(
    Xic			 ic,
    XPointer		 top,
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XIMArg		*arg,
    unsigned long	 mode,
    Bool		 flag
);

extern char *	 _XimGetICValueData(
    Xic			 ic,
    XPointer		 top,
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XIMArg		*arg,
    unsigned long	 mode
);

extern char *	 _XimLocalSetIMValues(
    XIM		 im,
    XIMArg	*arg
);

extern char *	 _XimLocalGetIMValues(
    XIM		 im,
    XIMArg	*arg
);

extern XIC	 _XimLocalCreateIC(
    XIM		 im,
    XIMArg	*arg
);

extern Bool	_XimDispatchInit(
    Xim		 im
);

extern Bool	 _XimGetAttributeID(
    Xim		 im,
    CARD16	*buf
);

extern Bool	 _XimExtension(
    Xim		 im
);

extern void	_XimDestroyIMStructureList(
    Xim		 im
);

extern char *	_XimMakeIMAttrIDList(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    XIMArg		*arg,
    CARD16		*buf,
    INT16		*len,
    unsigned long        mode
);

extern char *	_XimMakeICAttrIDList(
    Xic                  ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    XIMArg		*arg,
    CARD16		*idList,
    INT16		*num,
    unsigned long	 mode
);

extern char *	_XimDecodeIMATTRIBUTE(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    CARD16		*buf,
    INT16		 buf_len,
    XIMArg		*arg,
    BITMASK32		 mode
);

extern char *	_XimDecodeICATTRIBUTE(
    Xic			 ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    CARD16		*buf,
    INT16		 buf_len,
    XIMArg		*arg,
    BITMASK32		 mode
);

extern Bool	_XimRegProtoIntrCallback(
    Xim		 im,
    CARD16	 major_code,
    CARD16	 minor_code,
    Bool	 (*proc)(
			Xim, INT16, XPointer, XPointer
			),
    XPointer	 call_data
);

extern Bool	_XimErrorCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern Bool	_XimError(
    Xim		 im,
    Xic		 ic,
    CARD16	 error_code,
    INT16	 detail_length,
    CARD16	 type,
    char	*detail
);

extern Bool	_XimRegisterTriggerKeysCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern Bool	_XimSetEventMaskCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern Bool	_XimForwardEventCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern Bool	_XimCommitCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern Bool	_XimSyncCallback(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data
);

extern void	_XimFreeProtoIntrCallback(
    Xim		 im
);

extern XIC	 _XimProtoCreateIC(
    XIM		 im,
    XIMArg	*arg
);

extern void	_XimRegisterServerFilter(
    Xim		 im
);

extern void	_XimUnregisterServerFilter(
    Xim		 im
);

extern Bool	_XimTriggerNotify(
    Xim		 im,
    Xic		 ic,
    int		 mode,
    CARD32	 idx
);

extern Bool	_XimProcSyncReply(
    Xim		 im,
    Xic		 ic
);

extern void	_XimSendSetFocus(
    Xim		 im,
    Xic		 ic
);

extern Bool	_XimForwardEvent(
    Xic		 ic,
    XEvent	*ev,
    Bool	 sync
);

extern void	_XimFreeRemakeArg(
    XIMArg	*arg
);

extern void	_XimServerDestroy(
    Xim			im
);

extern char *	_XimEncodeIMATTRIBUTE(
    Xim			  im,
    XIMResourceList	  res_list,
    unsigned int	  res_num,
    XIMArg               *arg,
    XIMArg		**arg_ret,
    char		 *buf,
    int			  size,
    int			 *ret_len,
    XPointer		  top,
    unsigned long	  mode
);

extern char *	_XimEncodeICATTRIBUTE(
    Xic			  ic,
    XIMResourceList	  res_list,
    unsigned int	  res_num,
    XIMArg               *arg,
    XIMArg		**arg_ret,
    char		 *buf,
    int			  size,
    int			 *ret_len,
    XPointer		  top,
    BITMASK32		 *flag,
    unsigned long	  mode
);

#ifdef EXT_MOVE
extern Bool	_XimExtenMove(
    Xim		 im,
    Xic		 ic,
    CARD32	 flag,
    CARD16	*buf,
    INT16	 length
);
#endif

extern int	_Ximctstombs(
    XIM		 im,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state
);

extern int	_Ximctstowcs(
    XIM		 im,
    char	*from,
    int		 from_len,
    wchar_t	*to,
    int		 to_len,
    Status	*state
);

extern int	_Ximctstoutf8(
    XIM		 im,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state
);

extern int	_XimLcctstombs(
    XIM		 im,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state
);

extern int	_XimLcctstowcs(
    XIM		 im,
    char	*from,
    int		 from_len,
    wchar_t	*to,
    int		 to_len,
    Status	*state
);

extern int	_XimLcctstoutf8(
    XIM		 im,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state
);

extern char _XimGetMyEndian(
    void
);

extern int	_XimCheckDataSize(
    XPointer	 buf,
    int		 len
);

extern void	_XimSetHeader(
    XPointer	 buf,
    CARD8	 major_opcode,
    CARD8	 minor_opcode,
    INT16	*len
);

extern Bool	_XimSync(
    Xim		 im,
    Xic		 ic
);

extern int	_XimProtoMbLookupString(
    XIC		 xic,
    XKeyEvent	*ev,
    char	*buffer,
    int		 bytes,
    KeySym	*keysym,
    Status	*state
);

extern int	_XimProtoWcLookupString(
    XIC		 xic,
    XKeyEvent	*ev,
    wchar_t	*buffer,
    int		 bytes,
    KeySym	*keysym,
    Status	*state
);

extern int	_XimProtoUtf8LookupString(
    XIC		 xic,
    XKeyEvent	*ev,
    char	*buffer,
    int		 bytes,
    KeySym	*keysym,
    Status	*state
);

extern void	_XimRegisterFilter(
    Xic		 ic
);

extern void	_XimUnregisterFilter(
    Xic		 ic
);

extern void	_XimReregisterFilter(
    Xic		 ic
);

extern Status	_XimProtoEventToWire(
    XEvent	*re,
    xEvent	*event,
    Bool	sw
);

extern Bool	_XimProtoWireToEvent(
    XEvent	*re,
    xEvent	*event,
    Bool	 sw
);

#ifdef EXT_FORWARD
extern Bool	_XimExtForwardKeyEvent(
    Xic		 ic,
    XKeyEvent	*ev,
    Bool	 sync
);
#endif

extern int	_XimLookupMBText(
    Xic			 ic,
    XKeyEvent		*event,
    char		*buffer,
    int			 nbytes,
    KeySym		*keysym,
    XComposeStatus	*status
);

extern int	_XimLookupWCText(
    Xic			 ic,
    XKeyEvent		*event,
    wchar_t		*buffer,
    int			 nbytes,
    KeySym		*keysym,
    XComposeStatus	*status
);

extern int	_XimLookupUTF8Text(
    Xic			 ic,
    XKeyEvent		*event,
    char		*buffer,
    int			 nbytes,
    KeySym		*keysym,
    XComposeStatus	*status
);

extern EVENTMASK	_XimGetWindowEventmask(
    Xic		 ic
);

extern Xic	_XimICOfXICID(
    Xim		im,
    XICID	icid
);

extern void	_XimResetIMInstantiateCallback(
    Xim         xim
);

extern Bool	_XimRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char	*res_class,
    XIDProc	 callback,
    XPointer	 client_data
);

extern Bool	_XimUnRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char	*res_class,
    XIDProc	 callback,
    XPointer	 client_data
);

extern void	_XimFreeCommitInfo(
    Xic		 ic
);

extern Bool	_XimConnect(
    Xim		 im
);

extern Bool	_XimShutdown(
    Xim		 im
);

extern Bool	_XimWrite(
    Xim		 im,
    INT16	 len,
    XPointer	 data
);

extern Bool	_XimRead(
    Xim		 im,
    INT16	*len,
    XPointer	 data,
    int		 data_len,
    Bool	 (*predicate)(
			      Xim, INT16, XPointer, XPointer
			      ),
    XPointer	 arg
);

extern void	_XimFlush(
    Xim		 im
);

extern Bool	_XimFilterWaitEvent(
    Xim		 im
);

extern void   _XimProcError(
    Xim		im,
    Xic		ic,
    XPointer	data
);

#ifdef EXT_MOVE
extern CARD32	_XimExtenArgCheck(
    XIMArg	*arg
);
#endif

extern Bool _XimCbDispatch(
    Xim im,
    INT16 len,
    XPointer data,
    XPointer call_data
);

extern Bool _XimLocalFilter(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 client_data
);

extern XIMResourceList _XimGetResourceListRecByQuark(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XrmQuark		 quark
);

extern Bool _XimReconnectModeCreateIC(
    Xic			 ic
);

extern char *_XimLocalSetICValues(
    XIC			 ic,
    XIMArg		*values
);

extern char * _XimLocalGetICValues(
    XIC			 ic,
    XIMArg		*values
);

extern int _XimLocalMbLookupString(
    XIC			 ic,
    XKeyEvent		*ev,
    char		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*status
);

extern int _XimLocalWcLookupString(
    XIC			 ic,
    XKeyEvent		*ev,
    wchar_t		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*status
);

extern int _XimLocalUtf8LookupString(
    XIC			 ic,
    XKeyEvent		*ev,
    char		*buffer,
    int			 bytes,
    KeySym		*keysym,
    Status		*status
);

extern Bool _XimThaiFilter(
    Display		*d,
    Window		 w,
    XEvent		*ev,
    XPointer		 client_data
);

extern XIC _XimThaiCreateIC(
    XIM			 im,
    XIMArg		*values
);

extern Status _XimThaiCloseIM(
    XIM			 xim
);

#ifdef XIM_CONNECTABLE
extern void _XimSetProtoResource(
    Xim im
);

extern Bool _XimConnectServer(
    Xim im
);

extern Bool _XimDelayModeSetAttr(
    Xim			 im
);

extern void	_XimServerReconectableDestroy(
    void
);

extern Bool _XimReCreateIC(
    Xic			ic
);

extern Bool _XimEncodeSavedIMATTRIBUTE(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    int			*idx,
    char		*buf,
    int			 size,
    int			*ret_len,
    XPointer		 top,
    unsigned long	 mode
);

extern Bool _XimEncodeSavedICATTRIBUTE(
    Xic			 ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    int			*idx,
    char		*buf,
    int			 size,
    int			*ret_len,
    XPointer		 top,
    unsigned long	 mode
);
#endif

extern Bool
_XimRegisterDispatcher(
    Xim          im,
    Bool         (*callback)(
                             Xim, INT16, XPointer, XPointer
                             ),
    XPointer     call_data);

extern Bool
_XimRespSyncReply(
    Xic          ic,
    BITMASK16    mode);

#endif /* _XIMINT_H */
