/* $XFree86$ */
#ifndef __XTRAPLIBP__
#define __XTRAPLIBP__


/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991, 1994 by Digital Equipment Corp.,
Maynard, MA

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

*****************************************************************************/
/*
 *
 *  CONTRIBUTORS:
 *
 *      Dick Annicchiarico
 *      Robert Chesler
 *      Dan Coutu
 *      Gene Durso
 *      Marc Evans
 *      Alan Jamison
 *      Mark Henry
 *      Ken Miller
 *
 *  DESCRIPTION:
 *      This header file contains the function prototypes for client/toolkit
 *      routines sorted by module (globally defined routines *only*).
 */

#include <X11/Xfuncproto.h>

#include <stdio.h>

_XFUNCPROTOBEGIN

/* XEConTxt.c */
XETC *XECreateTC (Display *dpy , CARD32 valuemask , XETCValues *value );
int XEChangeTC (XETC *tc , CARD32 mask , XETCValues *values );
void XEFreeTC (XETC *tc );
int XETrapSetMaxPacket (XETC *tc , Bool set_flag , CARD16 size );
int XETrapSetCommandKey (XETC *tc , Bool set_flag , KeySym cmd_key ,
    Bool mod_flag );
int XETrapSetTimestamps (XETC *tc , Bool set_flag , Bool delta_flag );
int XETrapSetWinXY (XETC *tc , Bool set_flag );
int XETrapSetCursor (XETC *tc , Bool set_flag );
int XETrapSetXInput (XETC *tc , Bool set_flag );
int XETrapSetColorReplies (XETC *tc , Bool set_flag );
int XETrapSetGrabServer (XETC *tc , Bool set_flag );
int XETrapSetStatistics (XETC *tc , Bool set_flag );
int XETrapSetRequests (XETC *tc , Bool set_flag , ReqFlags requests );
int XETrapSetEvents (XETC *tc , Bool set_flag , EventFlags events );
Bool XESetCmdGateState (XETC *tc , CARD8 type, Bool *gate_closed ,
    CARD8 *next_key , Bool *key_ignore );

/* XERqsts.c */
int XEFlushConfig (XETC *tc );
int XEResetRequest (XETC *tc );
int XEGetVersionRequest (XETC *tc , XETrapGetVersRep *ret );
int XEGetLastInpTimeRequest (XETC *tc , XETrapGetLastInpTimeRep *ret );
int XEGetAvailableRequest (XETC *tc , XETrapGetAvailRep *ret );
int XEStartTrapRequest (XETC *tc );
int XEStopTrapRequest (XETC *tc );
int XESimulateXEventRequest (XETC *tc , CARD8 type , CARD8 detail ,
    CARD16 x , CARD16 y , CARD8 screen );
int XEGetCurrentRequest (XETC *tc , XETrapGetCurRep *ret );
int XEGetStatisticsRequest (XETC *tc , XETrapGetStatsRep *ret );

/* XECallBcks.c */
int XEAddRequestCB (XETC *tc , CARD8 req , void_function func , BYTE *data );
int XEAddRequestCBs (XETC *tc , ReqFlags req_flags , void_function func ,
    BYTE *data );
int XEAddEventCB (XETC *tc , CARD8 evt , void_function func , BYTE *data );
int XEAddEventCBs (XETC *tc , EventFlags evt_flags , void_function func ,
    BYTE *data );

/* The following seem to never be used.  Perhaps they should be removed */
void XERemoveRequestCB (XETC *tc, CARD8 req);
void XERemoveRequestCBs (XETC *tc, ReqFlags req_flags);
void XERemoveAllRequestCBs (XETC *tc);
void XERemoveEventCB (XETC *tc, CARD8 evt);
void XERemoveEventCBs (XETC *tc, EventFlags evt_flags);
void XERemoveAllEventCBs (XETC *tc);


/* XEDsptch.c */
Boolean XETrapDispatchXLib (XETrapDataEvent *event , XETC *tc);

/* XEWrappers.c */
Boolean XETrapDispatchEvent (XEvent *pevent , XETC *tc );
XtInputMask XETrapAppPending (XtAppContext app);
void XETrapAppMainLoop (XtAppContext app , XETC *tc );
int XETrapAppWhileLoop (XtAppContext app , XETC *tc , Bool *done );
int XETrapWaitForSomething (XtAppContext app );
Boolean (*XETrapGetEventHandler(XETC *tc, CARD32 id))(XETrapDataEvent *event, XETC *tc);
Boolean (*XETrapSetEventHandler(XETC *tc, CARD32 id, Boolean (*pfunc)(XETrapDataEvent *event, XETC *tc))) (XETrapDataEvent *event, XETC *tc);

/* XEPrInfo.c */
void XEPrintRelease (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintTkRelease ( FILE *ofp, XETC *tc);
void XEPrintPlatform (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintAvailFlags (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintAvailPktSz (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintStateFlags (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintMajOpcode (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintCurXY (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintTkFlags (FILE *ofp , XETC *tc );
void XEPrintLastTime (FILE *ofp , XETC *tc );
void XEPrintCfgFlags (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintRequests (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintEvents (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintCurPktSz (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintCmdKey (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintEvtStats (FILE *ofp , XETrapGetStatsRep *pstats , XETC *tc);
void XEPrintReqStats (FILE *ofp , XETrapGetStatsRep *pstats , XETC *tc);
void XEPrintAvail (FILE *ofp , XETrapGetAvailRep *pavail );
void XEPrintTkState (FILE *ofp , XETC *tc );
void XEPrintCurrent (FILE *ofp , XETrapGetCurRep *pcur );
void XEPrintStatistics (FILE *ofp , XETrapGetStatsRep *pstats, XETC *tc );

/* XEStrMap.c */
INT16 XEEventStringToID (char *string );
INT16 XERequestStringToID (char *string );
CARD32 XEPlatformStringToID (char *string );
char *XEEventIDToString (CARD8 id , XETC *tc);
char *XERequestIDToExtString (register CARD8 id , XETC *tc);
char *XERequestIDToString (CARD8 id , XETC *tc);
char *XEPlatformIDToString (CARD32 id );

/* XETrapInit.c */
Bool XETrapQueryExtension (Display *dpy,INT32 *event_base_return,
			  INT32 *error_base_return, INT32 *opcode_return);


_XFUNCPROTOEND

#endif /* __XTRAPLIBP__ */
