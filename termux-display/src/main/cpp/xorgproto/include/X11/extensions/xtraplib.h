/* $XFree86$ */
#ifndef __XTRAPLIB__
#define __XTRAPLIB__


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
 *      This header file describes all the structures/constants required
 *      for interfacing with the client toolkit *except* the common
 *      client/extension definitions in xtrapdi.h.  Namely, *no* extension-
 *      only information or client/extension information can be found here.
 */
#ifdef SMT
#define NEED_EVENTS
#define NEED_REPLIES
#endif
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/xtrapdi.h>
#include <X11/extensions/xtrapemacros.h>

typedef struct _XETC XETC;

typedef int  (*int_function)(XETC *tc, XETrapDatum *pdatum, BYTE *userp);
typedef void (*void_function)(XETC *tc, XETrapDatum *pdatum, BYTE *userp);

typedef struct  /* Callback structure */
{
    void_function func;
    BYTE          *data;
} XETrapCB;

/* Data structure for setting trap context */
typedef struct
{
    CARD8       tc_flags[2L];  /* Toolkit-side specific flags */
    XETrapCfg   v;             /* XTrap configuration values */
    XETrapCB    *req_cb;       /* Pointer to Request Callbacks */
    XETrapCB    *evt_cb;       /* Pointer to Event Callbacks (starting at 2) */
    CARD32      last_time;     /* Last (delta) timestamp */
} XETCValues;

    /* bits 0 thru 6 are formerly "families" (now obsolete) */
#define XETCDeltaTimes             7
#define XETCTrapActive             8
    /* bits 9 thru 15 are reserved for future expansion */

/* Values bit masks (used when determining what's dirty */
#define TCStatistics               (1L<<0L)
#define TCRequests                 (1L<<1L)
#define TCEvents                   (1L<<2L)
#define TCMaxPacket                (1L<<3L)
#define TCCmdKey                   (1L<<4L)
#define TCTimeStamps               (1L<<5L)
#define TCWinXY                    (1L<<6L)
#define TCXInput                   (1L<<7L)
#define TCReqCBs                   (1L<<8L)
#define TCEvtCBs                   (1L<<9L)
#define TCCursor                   (1L<<10L)
#define TCColorReplies             (1L<<11L)
#define TCGrabServer               (1L<<12L)

/* This is the representation we use in the library code for XLib transport */
typedef struct {
    int type;
    unsigned long serial;
    Bool synthetic;
    Display *display;
    int detail;
    unsigned long idx;
    unsigned char data[sz_EventData];
} XETrapDataEvent;

/* Trap Context structure for maintaining XTrap State for client */
struct _XETC
{
    struct _XETC *next;         /* Ptr to next linked-listed TC */
    Display      *dpy;          /* Display ptr of current TC */
    INT32        eventBase /*B32*/; /* First event value */
    INT32        errorBase /*B32*/; /* First error value */
    INT32        extOpcode /*B32*/; /* Major opcode of the extension */
    BYTE         *xbuff;        /* Pointer to buffer for XLib Communications */
    CARD16       xmax_size /*B16*/; /* Max Size of a request */
    XExtData     *ext_data;     /* hook for extension to hang data */
    /*
     *  The following are initialized with the client-side version number
     *  However, when either a GetAvailable or GetVersion reply is received,
     *  these values are updated with the *oldest* version numbers.
     */
    CARD16       release /*B16*/;   /* The extension release number */
    CARD16       version /*B16*/;   /* The xtrap extension version number */
    CARD16       revision /*B16*/;  /* The xtrap extension revision number */
    CARD16       protocol /*B16*/;  /* The xtrap extension protocol number */
    unsigned     dirty /*B32*/;     /* cache dirty bits */
    XETCValues   values;        /* shadow structure of values */
    Boolean      (*eventFunc[XETrapNumberEvents])(XETrapDataEvent *event, struct _XETC *tc);
};


#endif /* __XTRAPLIB__ */
