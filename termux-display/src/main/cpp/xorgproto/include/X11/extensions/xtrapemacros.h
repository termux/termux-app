/* $XFree86: xc/include/extensions/xtrapemacros.h,v 1.1 2001/11/02 23:29:26 dawes Exp $ */
#ifndef __XTRAPEMACROS__
#define __XTRAPEMACROS__ "@(#)xtrapemacros.h	1.9 - 90/09/18  "

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
 *      This include file is designed to provide the *visible*
 *      interface to XTrap data structures.  Fields can be set
 *      using these macros by client programs unless otherwise
 *      specified; however, use of Trap Context convenience
 *      routines is strongly encouraged (XETrapContext.c)
 */
#include <X11/extensions/xtrapbits.h>
#include <signal.h>

/* msleep macro to replace msleep() for portability reasons */
#define msleep(m)   usleep((m)*1000)

/* Copying TC's assumes that the new TC must be created */
#define XECopyTC(src,mask,dest) \
    (dest = XECreateTC(((src)->dpy), (mask), (&((src)->values))))

/* Expands to SET each element of the TCValues structure
 * Returns the TCValues Mask so that the Set can be entered
 * as an argument to the XEChangeTC() routine call
 */
/* Note: req_cb & evt_cb would only be used if you wanted to
 *       *share* callbacks between Trap Contexts.  Normally,
 *       XEAddRequestCB() and XEAddEventCB() would be used.
 */
#define XETrapSetCfgReqCB(tcv,x)      ((tcv)->req_cb = (x))
#define XETrapSetCfgEvtCB(tcv,x)      ((tcv)->evt_cb = (x))
#define XETrapSetCfgMaxPktSize(tcv,x) ((tcv)->v.max_pkt_size = (x))
#define XETrapSetCfgCmdKey(tcv,x)     ((tcv)->v.cmd_key = (x))
/* Note: e is only pertinent for "valid" or "data" */
#define XETrapSetCfgFlags(tcv,e,a)    \
    memcpy((tcv)->v.flags.e, (a), sizeof((tcv)->v.flags.e))
#define XETrapSetCfgFlagTimestamp(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapTimestamp, (x))
#define XETrapSetCfgFlagCmd(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapCmd, (x))
#define XETrapSetCfgFlagCmdKeyMod(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapCmdKeyMod, (x))
#define XETrapSetCfgFlagRequest(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapRequest, (x))
#define XETrapSetCfgFlagEvent(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapEvent, (x))
#define XETrapSetCfgFlagMaxPacket(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapMaxPacket, (x))
#define XETrapSetCfgFlagStatistics(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapStatistics, (x))
#define XETrapSetCfgFlagWinXY(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapWinXY, (x))
#define XETrapSetCfgFlagCursor(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapCursor, (x))
#define XETrapSetCfgFlagReq(tcv,request,x) \
    BitSet((tcv)->v.flags.req, (request), (x))
#define XETrapSetCfgFlagXInput(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapXInput, (x))
#define XETrapSetCfgFlagColorReplies(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapColorReplies, (x))
#define XETrapSetCfgFlagGrabServer(tcv,e,x) \
    BitSet((tcv)->v.flags.e, XETrapGrabServer, (x))
#define XETrapSetCfgFlagEvt(tcv,evt,x) \
    BitSet((tcv)->v.flags.event, (evt), (x))

#define XETrapSetValFlagDeltaTimes(tcv,x) \
    BitSet((tcv)->tc_flags, XETCDeltaTimes, (x))

/* Fields returned in the "GetAvailable" request */
#define XETrapGetAvailPFIdent(avail)      ((avail)->pf_ident)
#define XETrapGetAvailRelease(avail)      ((avail)->xtrap_release)
#define XETrapGetAvailVersion(avail)      ((avail)->xtrap_version)
#define XETrapGetAvailRevision(avail)     ((avail)->xtrap_revision)
#define XETrapGetAvailMaxPktSize(avail)   ((avail)->max_pkt_size)
#define XETrapGetAvailFlags(avail,a)  \
    memcpy((a), (avail)->valid, sizeof((avail)->valid))
#define XETrapGetAvailFlagTimestamp(avail) \
    (BitValue((avail)->valid, XETrapTimestamp))
#define XETrapGetAvailFlagCmd(avail) \
    (BitValue((avail)->valid, XETrapCmd))
#define XETrapGetAvailFlagCmdKeyMod(avail) \
    (BitValue((avail)->valid, XETrapCmdKeyMod))
#define XETrapGetAvailFlagRequest(avail) \
    (BitValue((avail)->valid, XETrapRequest))
#define XETrapGetAvailFlagEvent(avail) \
    (BitValue((avail)->valid, XETrapEvent))
#define XETrapGetAvailFlagMaxPacket(avail) \
    (BitValue((avail)->valid, XETrapMaxPacket))
#define XETrapGetAvailFlagStatistics(avail) \
    (BitValue((avail)->valid, XETrapStatistics))
#define XETrapGetAvailFlagWinXY(avail) \
    (BitValue((avail)->valid, XETrapWinXY))
#define XETrapGetAvailFlagCursor(avail) \
    (BitValue((avail)->valid, XETrapCursor))
#define XETrapGetAvailFlagXInput(avail) \
    (BitValue((avail)->valid, XETrapXInput))
#define XETrapGetAvailFlagVecEvt(avail) \
    (BitValue((avail)->valid, XETrapVectorEvents))
#define XETrapGetAvailFlagColorReplies(avail) \
    (BitValue((avail)->valid, XETrapColorReplies))
#define XETrapGetAvailFlagGrabServer(avail) \
    (BitValue((avail)->valid, XETrapGrabServer))
#define XETrapGetAvailOpCode(avail)         ((avail)->major_opcode)
/* Macro's for creating current request and trap context macros */
#define XETrapGetCfgMaxPktSize(cfg) ((cfg)->max_pkt_size)
#define XETrapGetCfgCmdKey(cfg)     ((cfg)->cmd_key)
#define XETrapGetCfgFlags(cfg,e,a)  \
    memcpy((a), (cfg)->flags.e, sizeof((cfg)->flags.e))
#define XETrapGetCfgFlagTimestamp(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapTimestamp))
#define XETrapGetCfgFlagCmd(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapCmd))
#define XETrapGetCfgFlagCmdKeyMod(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapCmdKeyMod))
#define XETrapGetCfgFlagRequest(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapRequest))
#define XETrapGetCfgFlagEvent(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapEvent))
#define XETrapGetCfgFlagMaxPacket(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapMaxPacket))
#define XETrapGetCfgFlagStatistics(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapStatistics))
#define XETrapGetCfgFlagWinXY(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapWinXY))
#define XETrapGetCfgFlagCursor(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapCursor))
#define XETrapGetCfgFlagXInput(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapXInput))
#define XETrapGetCfgFlagColorReplies(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapColorReplies))
#define XETrapGetCfgFlagGrabServer(cfg,e) \
    (BitValue((cfg)->flags.e, XETrapGrabServer))
/* Request values are in "Xproto.h" of the flavor X_RequestType */
#define XETrapGetCfgFlagReq(cfg,request) \
    (BitValue((cfg)->flags.req, (request)))
/* Event types are in "X.h" of the flavor EventType (e.g. KeyPress) */
#define XETrapGetCfgFlagEvt(cfg,evt) \
    (BitValue((cfg)->flags.event, (evt)))

/* Fields returned int the "GetCurrent" Request */
#define XETrapGetCurX(avail)              ((avail)->cur_x)
#define XETrapGetCurY(avail)              ((avail)->cur_y)
#define XETrapGetCurSFlags(cur,a) \
    memcpy((a), (cur)->state_flags, sizeof((cur)->state_flags))
#define XETrapGetCurMaxPktSize(cur) (XETrapGetCfgMaxPktSize(&((cur)->config)))
#define XETrapGetCurCmdKey(cur)     (XETrapGetCfgCmdKey(&((cur)->config)))
/* Note: e is only pertinent for "valid" or "data" */
#define XETrapGetCurCFlags(cur,e,a) (XETrapGetCfgFlags(&((cur)->config),e,a))
#define XETrapGetCurFlagTimestamp(cur,e) \
    (XETrapGetCfgFlagTimestamp(&((cur)->config),e))
#define XETrapGetCurFlagCmd(cur,e)  (XETrapGetCfgFlagCmd(&((cur)->config),e))
#define XETrapGetCurFlagCmdKeyMod(cur,e) \
    (XETrapGetCfgFlagCmdKeyMod(&((cur)->config),e))
#define XETrapGetCurFlagRequest(cur,r) \
    (XETrapGetCfgFlagRequest(&((cur)->config),r))
#define XETrapGetCurFlagEvent(cur,e) \
    (XETrapGetCfgFlagEvent(&((cur)->config),e))
#define XETrapGetCurFlagMaxPacket(cur,e) \
    (XETrapGetCfgFlagMaxPacket(&((cur)->config),e))
#define XETrapGetCurFlagStatistics(cur,e) \
    (XETrapGetCfgFlagStatistics(&((cur)->config),e))
#define XETrapGetCurFlagWinXY(cur,e) \
    (XETrapGetCfgFlagWinXY(&((cur)->config),e))
#define XETrapGetCurFlagCursor(cur,e) \
    (XETrapGetCfgFlagCursor(&((cur)->config),e))
#define XETrapGetCurFlagXInput(cur,e) \
    (XETrapGetCfgFlagXInput(&((cur)->config),e))
#define XETrapGetCurFlagColorReplies(cur,e) \
    (XETrapGetCfgFlagColorReplies(&((cur)->config),e))
#define XETrapGetCurFlagGrabServer(cur,e) \
    (XETrapGetCfgFlagGrabServer(&((cur)->config),e))
/* Request values are in "Xproto.h" of the flavor X_RequestType */
#define XETrapGetCurFlagReq(cur,r)  (XETrapGetCfgFlagReq(&((cur)->config),r))
/* Event types are in "X.h" of the flavor EventType (e.g. KeyPress) */
#define XETrapGetCurFlagEvt(cur,e)  (XETrapGetCfgFlagEvt(&((cur)->config),e))

/* Fields returned int the "GetStatistics" Request */
#define XETrapGetStatsReq(stat,e)   ((stat)->requests[(e)])
#define XETrapGetStatsEvt(stat,e)   ((stat)->events[(e)])

/* Fields returned in the "GetVersion" request */
#define XETrapGetVersRelease(vers)      ((vers)->xtrap_release)
#define XETrapGetVersVersion(vers)      ((vers)->xtrap_version)
#define XETrapGetVersRevision(vers)     ((vers)->xtrap_revision)

/* Fields returned in the "GetLastInpTime" request */
#define XETrapGetLastInpTime(time_rep)      ((time_rep)->last_time)

/* Expands to GET each element of the TCValues structure */
#define XETrapGetTCReqCB(tc)      ((tc)->values.req_cb)
#define XETrapGetTCEvtCB(tc)      ((tc)->values.evt_cb)
#define XETrapGetTCTime(tc)       ((tc)->values.last_time)
/* TC specific flags */
#define XETrapGetTCLFlags(tc,a)  \
    memcpy((a), (tc)->values.tc_flags, sizeof((tc)->values.tc_flags))
#define XETrapGetTCFlagDeltaTimes(tc) \
    (BitValue((tc)->values.tc_flags, XETCDeltaTimes))
#define XETrapGetTCFlagTrapActive(tc) \
    (BitValue((tc)->values.tc_flags, XETCTrapActive))
#define XETrapGetTCMaxPktSize(tc) (XETrapGetCfgMaxPktSize(&((tc)->values.v)))
#define XETrapGetTCCmdKey(tc)     (XETrapGetCfgCmdKey(&((tc)->values.v)))
/* Note: e is only pertinent for "valid" or "data" */
#define XETrapGetTCFlags(tc,e,a)  (XETrapGetCfgFlags(&((tc)->values.v),e,a))
#define XETrapGetTCFlagTimestamp(tc,e) \
    (XETrapGetCfgFlagTimestamp(&((tc)->values.v),e))
#define XETrapGetTCFlagCmd(tc,e) \
    (XETrapGetCfgFlagCmd(&((tc)->values.v),e))
#define XETrapGetTCFlagCmdKeyMod(tc,e) \
    (XETrapGetCfgFlagCmdKeyMod(&((tc)->values.v),e))
#define XETrapGetTCFlagRequest(tc,r) \
    (XETrapGetCfgFlagRequest(&((tc)->values.v),r))
#define XETrapGetTCFlagEvent(tc,e) \
    (XETrapGetCfgFlagEvent(&((tc)->values.v),e))
#define XETrapGetTCFlagMaxPacket(tc,e) \
    (XETrapGetCfgFlagMaxPacket(&((tc)->values.v),e))
#define XETrapGetTCFlagStatistics(tc,e) \
    (XETrapGetCfgFlagStatistics(&((tc)->values.v),e))
#define XETrapGetTCFlagWinXY(tc,e) \
    (XETrapGetCfgFlagWinXY(&((tc)->values.v),e))
#define XETrapGetTCFlagCursor(tc,e) \
    (XETrapGetCfgFlagCursor(&((tc)->values.v),e))
#define XETrapGetTCFlagXInput(tc,e) \
    (XETrapGetCfgFlagXInput(&((tc)->values.v),e))
#define XETrapGetTCFlagColorReplies(tc,e) \
    (XETrapGetCfgFlagColorReplies(&((tc)->values.v),e))
#define XETrapGetTCFlagGrabServer(tc,e) \
    (XETrapGetCfgFlagGrabServer(&((tc)->values.v),e))
/* Request values are in "Xproto.h" of the flavor X_RequestType */
#define XETrapGetTCFlagReq(tc,r) \
    (XETrapGetCfgFlagReq(&((tc)->values.v),r))
/* Event types are in "X.h" of the flavor EventType (e.g. KeyPress) */
#define XETrapGetTCFlagEvt(tc,e) \
    (XETrapGetCfgFlagEvt(&((tc)->values.v),e))
/* The following can/should *not* be set directly! */
#define XETrapGetNext(tc)        ((tc)->next)
#define XETrapGetDpy(tc)         ((tc)->dpy)
#define XETrapGetEventBase(tc)   ((tc)->eventBase)
#define XETrapGetErrorBase(tc)   ((tc)->errorBase)
#define XETrapGetExtOpcode(tc)   ((tc)->extOpcode)
#define XETrapGetXBuff(tc)       ((tc)->xbuff)
#define XETrapGetXMaxSize(tc)    ((tc)->xmax_size)
#define XETrapGetExt(tc)         ((tc)->ext_data)
#define XETrapGetDirty(tc)       ((tc)->dirty)
#define XETrapGetValues(tc)      memcpy((x),(tc)->values,sizeof((tc)->values))
#define XETrapGetEventFunc(tc)   ((tc)->eventFunc)

#define XETrapGetHeaderCount(phdr)      ((phdr)->count)
#define XETrapGetHeaderTimestamp(phdr)  ((phdr)->timestamp)
#define XETrapGetHeaderType(phdr)       ((phdr)->type)
#define XETrapGetHeaderScreen(phdr)     ((phdr)->screen)
#define XETrapGetHeaderWindowX(phdr)    ((phdr)->win_x)
#define XETrapGetHeaderWindowY(phdr)    ((phdr)->win_y)
#define XETrapGetHeaderClient(phdr)     ((phdr)->client)

#define XEGetRelease(tc)        ((tc)->release)
#define XEGetVersion(tc)        ((tc)->version)
#define XEGetRevision(tc)       ((tc)->revision)

/*  Condition handling macros */
#if !defined(vms) && \
    (!defined(_InitExceptionHandling) || !defined(_ClearExceptionHandling))
# ifndef _SetSIGBUSHandling
#  ifdef SIGBUS
#   define _SetSIGBUSHandling(rtn)  (void)signal(SIGBUS, rtn)
#  else
#   define _SetSIGBUSHandling(rtn)  /* */
#  endif
# endif
# ifndef _SetSIGSEGVHandling
#  ifdef SIGSEGV
#   define _SetSIGSEGVHandling(rtn) (void)signal(SIGSEGV, rtn)
#  else
#   define _SetSIGSEGVHandling(rtn) /* */
#  endif
# endif
# ifndef _SetSIGFPEHandling
#  ifdef SIGFPE
#   define _SetSIGFPEHandling(rtn)  (void)signal(SIGFPE, rtn)
#  else
#   define _SetSIGFPEHandling(rtn)  /* */
#  endif
# endif
# ifndef _SetSIGILLHandling
#  ifdef SIGILL
#   define _SetSIGILLHandling(rtn)  (void)signal(SIGILL, rtn)
#  else
#   define _SetSIGILLHandling(rtn)  /* */
#  endif
# endif
# ifndef _SetSIGSYSHandling
#  ifdef SIGSYS
#   define _SetSIGSYSHandling(rtn)  (void)signal(SIGSYS, rtn)
#  else
#   define _SetSIGSYSHandling(rtn)  /* */
#  endif
# endif
# ifndef _SetSIGHUPHandling
#  ifdef SIGHUP
#   define _SetSIGHUPHandling(rtn)  (void)signal(SIGHUP, rtn)
#  else
#   define _SetSIGHUPHandling(rtn)  /* */
#  endif
# endif
# ifndef _SetSIGPIPEHandling
#  ifdef SIGPIPE
#   define _SetSIGPIPEHandling(rtn) (void)signal(SIGPIPE, rtn)
#  else
#   define _SetSIGPIPEHandling(rtn) /* */
#  endif
# endif
# ifndef _SetSIGTERMHandling
#  ifdef SIGTERM
#   define _SetSIGTERMHandling(rtn) (void)signal(SIGTERM, rtn)
#  else
#   define _SetSIGTERMHandling(rtn) /* */
#  endif
# endif
#endif
#ifndef _InitExceptionHandling
#ifdef vms
#define _InitExceptionHandling(rtn)                        \
    VAXC$ESTABLISH(rtn)   /* VMS exception handler */
#else /* vms */
#define _InitExceptionHandling(rtn)                           \
    _SetSIGBUSHandling(rtn);  /* Bus error */                 \
    _SetSIGSEGVHandling(rtn); /* Accvio/Segment error */      \
    _SetSIGFPEHandling(rtn);  /* Floating point exception */  \
    _SetSIGILLHandling(rtn);  /* Illegal instruction */       \
    _SetSIGSYSHandling(rtn);  /* Param error in sys call */   \
    _SetSIGHUPHandling(rtn);                                  \
    _SetSIGPIPEHandling(rtn);                                 \
    _SetSIGTERMHandling(rtn)
#endif /* vms */
#endif /* _InitExceptionHandling */

#ifndef _ClearExceptionHandling
#ifdef vms
#define _ClearExceptionHandling() \
    LIB$REVERT()
#else
#define _ClearExceptionHandling() \
    _SetSIGBUSHandling(SIG_DFL);   /* Bus error */                 \
    _SetSIGSEGVHandling(SIG_DFL);  /* Accvio/Segment error */      \
    _SetSIGFPEHandling(SIG_DFL);   /* Floating point exception */  \
    _SetSIGILLHandling(SIG_DFL);   /* Illegal instruction */       \
    _SetSIGSYSHandling(SIG_DFL);   /* Param error in sys call */   \
    _SetSIGHUPHandling(SIG_DFL);                                   \
    _SetSIGPIPEHandling(SIG_DFL);                                  \
    _SetSIGTERMHandling(SIG_DFL)
#endif /* vms */
#endif /* _ClearExceptionHandling */

#endif /* __XTRAPEMACROS__ */
