/* $XFree86$ */
#ifndef __XTRAPDI__
#define __XTRAPDI__

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991, 1992, 1994 by Digital Equipment Corp.,
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
 *      This header file defines the common structures/constants
 *      between the XTrap extension and clients.  All protocol
 *      definitions between XTrap extension/clients can be found
 *      here.
 */

#define NEED_REPLIES
#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xmd.h>
#ifdef SMT
#define NEED_EVENTS
#define NEED_REPLIES
#endif
#include <X11/Xproto.h>
#include <X11/extensions/xtrapbits.h>
#define XTrapExtName "DEC-XTRAP"
/* Current Release, Version, and Revision of the XTrap Extension */
#define XETrapRelease      3L
#define XETrapVersion      4L
#ifndef XETrapRevision        /* Changed from the Makefile by users */
# define XETrapRevision    0L
#endif /* XETrapRevision */
#define XETrapProtocol     32L

#ifndef SIZEOF
# ifdef __STDC__
#  define SIZEOF(x) sz_##x
# else
#  define SIZEOF(x) sz_/**/x
# endif /* if ANSI C compiler else not */
#endif
#ifndef sz_CARD32
#define sz_CARD32   4L
#endif
#ifndef sz_CARD8
#define sz_CARD8    1L
#endif
#ifndef True
# define True  1L
# define False 0L
#endif

/* This is used as flags to indicate desired request traps
 * Note:  This has been padded to a CARD32 to keep structure aligned
 */
#define XETrapMaxRequest (((SIZEOF(CARD32)+((256L-1L) / \
    (BitsInByte*SIZEOF(CARD8))))/SIZEOF(CARD32))*SIZEOF(CARD32))
typedef CARD8 ReqFlags[XETrapMaxRequest];

/* This is used as flags to indicate desired event traps
 * Until events become *fully vectored*, we'll have to fake it
 * by defining an array of 5 events (KeyPress, KeyRelease,
 * ButtonPress, ButtonRelease, and MotionNotify.  The extra 2
 * are required as the event types start with "2" (errors and
 * replies are 0 & 1).  The event type is the index into the
 * bits.
 * Note:  This has been padded to a longword to keep structure aligned
 */
#ifndef VECTORED_EVENTS
#define XETrapCoreEvents    (2L+5L)
#else
#define XETrapCoreEvents    128L
#endif
#define XETrapMaxEvent (((SIZEOF(CARD32)+((XETrapCoreEvents-1L) / \
    (BitsInByte*SIZEOF(CARD8))))/SIZEOF(CARD32))*SIZEOF(CARD32))
typedef CARD8 EventFlags[XETrapMaxEvent];

/* This structure is used in a request to specify the types of
 * configuration information that should be changed or updated.
 */
typedef struct
{
    CARD8      valid[4L];  /* Bits TRUE indicates data field is used */
    CARD8      data[4L];   /* Bits looked at if corresponding valid bit set */
    ReqFlags   req;        /* Bits correspond to core requests */
    EventFlags event;      /* Bits correspond to core events */
} XETrapFlags;

/* Bit definitions for the above XETrapFlags structure. */
#define  XETrapTimestamp      0L          /* hdr timestamps desired */
#define  XETrapCmd            1L          /* command key specified */
#define  XETrapCmdKeyMod      2L          /* cmd key is a modifier  */
#define  XETrapRequest        3L          /* output requests array */
#define  XETrapEvent          4L          /*  future  output events array */
#define  XETrapMaxPacket      5L          /* Maximum packet length set */
#define  XETrapTransOut       6L          /* obsolete */
#define  XETrapStatistics     7L          /* collect counts on requests */
#define  XETrapWinXY          8L          /* Fill in Window (X,Y) in hdr */
#define  XETrapTransIn        9L          /* obsolete */
#define  XETrapCursor         10L         /* Trap cursor state changes */
#define  XETrapXInput         11L         /* Use XInput extension */
#define  XETrapVectorEvents   12L         /* Use Vectored Events (128) */
#define  XETrapColorReplies   13L         /* Return replies with Color Req's */
#define  XETrapGrabServer     14L         /* Disables client GrabServers */

typedef struct /* used by XEConfigRequest */
{
    XETrapFlags flags;            /* Flags to specify what should be chg'd */
    CARD16      max_pkt_size;     /* Maximum number of bytes in a packet */
    CARD8       cmd_key;       /* Keyboard command_key (KeyCode) */
/*
 * cmd_key is intentionally *not* defined KeyCode since it's definition is
 * ambiguous (int in Intrinsic.h and unsigned char in X.h.
 */
    CARD8      pad[1L];           /* pad out to a quadword */
} XETrapCfg;

/* These structures are used within the Xtrap request structure for
 * the various types of xtrap request
 */
#ifndef _XINPUT
/* (see the definition of XEvent as a reference) */
typedef struct /* used by XESimulateXEventRequest for synthesizing core evts */
{
    CARD8    type;                 /* (must be first) as in XEvent */
    CARD8    detail;               /* Detail keycode/button as in XEvent */
    CARD8    screen;               /* screen number (0 to n) */
    CARD8    pad;                  /* pad to longword */
    INT16    x;                    /* X & Y coord as in XEvent */
    INT16    y;
} XETrapInputReq;
#endif

/* These are constants that refer to the extension request vector table.
 * A request will use these values as minor opcodes.
 */
#define XETrap_Reset          0L     /* set to steady state */
#define XETrap_GetAvailable   1L     /* get available funct from ext */
#define XETrap_Config         2L     /* configure extension */
#define XETrap_StartTrap      3L     /* use Trapping  */
#define XETrap_StopTrap       4L     /* stop using Trapping */
#define XETrap_GetCurrent     5L     /* get current info from ext */
#define XETrap_GetStatistics  6L     /* get count statistics from ext */
#ifndef _XINPUT
#define XETrap_SimulateXEvent 7L     /* async input simulation */
#endif
#define XETrap_GetVersion     8L     /* Get (Just) Version */
#define XETrap_GetLastInpTime 9L     /* Get Timestamp of last client input */

/* The following are formats of a request to the XTRAP
 * extension.  The data-less XTrap requests all use xXTrapReq
 */
typedef struct
{
    CARD8  reqType;
    CARD8  minor_opcode;
    CARD16 length;
    CARD32 pad;             /* Maintain quadword alignment */
} xXTrapReq;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapReq (sizeof(xXTrapReq))
/* For retrieving version/available info (passes lib-side protocol number) */
typedef struct
{
    CARD8  reqType;
    CARD8  minor_opcode;
    CARD16 length;
    CARD16 protocol;        /* The xtrap extension protocol number */
    CARD16 pad;             /* Maintain quadword alignment */
} xXTrapGetReq;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapGetReq (sizeof(xXTrapGetReq))

typedef struct
{
    CARD8     reqType;
    CARD8     minor_opcode;
    CARD16    length;
    /*
     * The following is done so that structure padding wont be
     * a problem.  The request structure contains a shadow for
     * the XETrapCfg structure. Since the XETrapCfg also has a
     * substructure (XETrapFlags) this structure is also shadowed.
     *
     * The following are a shadow of the XETrapFlags
     * structure.
     */
    CARD8       config_flags_valid[4L];
    CARD8       config_flags_data[4L];
    ReqFlags    config_flags_req;
    EventFlags  config_flags_event;
    /* End Shadow (XETrapFlags)*/
    CARD16      config_max_pkt_size;  /* Max number of bytes in a packet */
    CARD8       config_cmd_key;       /* Keyboard command_key (KeyCode) */
/*
 * cmd_key is intentionally *not* defined KeyCode since it's definition is
 * ambiguous (int in Intrinsic.h and unsigned char in X.h.
 */
    CARD8      config_pad[1L];           /* pad out to a quadword */
    /* End Shadow (XETrapCfg) */
    CARD32    pad;      /* Maintain quadword alignment */
} xXTrapConfigReq;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapConfigReq (sizeof(xXTrapConfigReq))

#ifndef _XINPUT
typedef struct
{
    CARD8            reqType;
    CARD8            minor_opcode;
    CARD16           length;
    CARD32           pad;       /* Maintain quadword alignment */
    XETrapInputReq   input;
} xXTrapInputReq;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapInputReq (sizeof(xXTrapInputReq))
#endif


/* The following structures are used by the server extension to send
 * information and replies to the client.
 */

/* header for all X replies */
typedef struct
{
    CARD8  type;
    CARD8  detail;
    CARD16 sequenceNumber;
    CARD32 length;
} XETrapRepHdr;

/* Structure of Get Available Functionality reply */
typedef struct
{
    CARD32  pf_ident;         /* Contains constant identifying the platform */
    CARD16  xtrap_release;    /* The xtrap extension release number */
    CARD16  xtrap_version;    /* The xtrap extension version number */
    CARD16  xtrap_revision;   /* The xtrap extension revision number */
    CARD16  max_pkt_size;     /* Maximum number of bytes in a packet */
    CARD8   valid[4];         /* What specific configuration flags are valid */
    CARD32  major_opcode;     /* The major opcode identifying xtrap */
    CARD32  event_base;       /* The event value we start at */
    CARD32  pad0;             /* obsolete field */
    CARD16  pad1, pad2, pad3; /* obsolete field */
    CARD16  xtrap_protocol;   /* The xtrap extension protocol number */
    INT16   cur_x;            /* Current X & Y coord for relative motion */
    INT16   cur_y;
} XETrapGetAvailRep;

typedef struct
{
    CARD16  xtrap_release;    /* The xtrap extension release number */
    CARD16  xtrap_version;    /* The xtrap extension version number */
    CARD16  xtrap_revision;   /* The xtrap extension revision number */
    CARD16  xtrap_protocol;   /* The xtrap extension protocol number */
} XETrapGetVersRep;

typedef struct
{
    CARD32 last_time;         /* Timestamp of last input time */
} XETrapGetLastInpTimeRep;

/* Structure of Get Current Configuration Information reply */
typedef struct
{
    CARD8       state_flags[2]; /* Miscellaneous flags, see below #define's */
    CARD16      pad0;           /* Assure quadword alignment */
    XETrapCfg   config;         /* Current Config information */
    CARD32      pad1;
} XETrapGetCurRep;

/* Mask definitions for the above flags. */
#define XETrapTrapActive     0L    /* If sending/receiving between client/ext */

/* Structure of Get Statistics Information reply */
typedef struct
{
    CARD32   requests[256L]; /* Array containing request counts if trapped */
    CARD32   events[XETrapCoreEvents];  /* Array containing event stats */
#ifndef VECTORED_EVENTS
    CARD32   pad;            /* Pad out to a quadword */
#endif
} XETrapGetStatsRep;

#define PF_Other         0L       /* server not one of the below */
#define PF_Apollo        10L      /* server on Apollo system */
#define PF_ATT           20L      /* server on AT&T system */
#define PF_Cray1         30L      /* server on Cray 1 system */
#define PF_Cray2         31L      /* server on Cray 2 system */
#define PF_DECUltrix     40L      /* server on DEC ULTRIX system */
#define PF_DECVMS        41L      /* server on DEC VMS system */
#define PF_DECVT1000     42L      /* server on DEC-VT1000-terminal */
#define PF_DECXTerm      43L      /* server on DEC-X-terminal */
#define PF_DECELN	 44L      /* server on DEC VAXELN X terminal */
#define PF_DECOSF1       45L      /* server on DEC's OSF/1 system */
#define PF_HP9000s800    50L      /* server on HP 9000/800 system */
#define PF_HP9000s300    51L      /* server on HP 9000/300 system */
#define PF_IBMAT         60L      /* server on IBM/AT system */
#define PF_IBMRT         61L      /* server on IBM/RT system */
#define PF_IBMPS2        62L      /* server on IBM/PS2 system */
#define PF_IBMRS         63L      /* server on IBM/RS system */
#define PF_MacII         70L      /* server on Mac II system */
#define PF_Pegasus       80L      /* server on Tektronix Pegasus system */
#define PF_SGI           90L      /* server on Silicon Graphcis system */
#define PF_Sony          100L     /* server on Sony system */
#define PF_Sun3          110L     /* server on Sun 3 system */
#define PF_Sun386i       111L     /* server on Sun 386i system */
#define PF_SunSparc      112L     /* server on Sun Sparc system */

/* reply sent back by XETrapGetAvailable request */
typedef struct
{
    XETrapRepHdr       hdr;
    XETrapGetAvailRep  data;
} xXTrapGetAvailReply;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapGetAvailReply  sizeof(xXTrapGetAvailReply)

/* reply sent back by XETrapGetVersion request */
typedef struct
{
    XETrapRepHdr       hdr;
    XETrapGetVersRep  data;
    CARD32 pad0;        /* pad out to 32 bytes */
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
} xXTrapGetVersReply;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapGetVersReply  sizeof(xXTrapGetVersReply)

/* reply sent back by XETrapGetLastInpTime request */
typedef struct
{
    XETrapRepHdr            hdr;
    /*
     * The following is a shadow of the XETrapGetLastInpTimeRep
     * structure.  This is done to avoid structure padding.
     */
    CARD32 data_last_time;    /* Timestamp of last input time */
    CARD32 pad0;              /* pad out to 32 bytes */
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
} xXTrapGetLITimReply;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapGetLITimReply  sizeof(xXTrapGetLITimReply)

/* reply sent back by XETrapGetCurrent request */
typedef struct
{
    XETrapRepHdr     hdr;
    /*
     * The following is a shadow of the XETrapGetCurRep
     * structure.  This is done to avoid structure padding.
     * Since the XETrapGetCurRep structure contains a sub-structure
     * (XETrapCfg) there is a shadow for that as well.*/
    CARD8       data_state_flags[2]; /* Misc flags, see below #define's */
    CARD16      data_pad0;           /* Assure quadword alignment */
    /* XETrapCfg Shadow Starts */
    CARD8       data_config_flags_valid[4L];
    CARD8       data_config_flags_data[4L];
    ReqFlags    data_config_flags_req;
    EventFlags  data_config_flags_event;
    CARD16      data_config_max_pkt_size;  /* Max num of bytes in a pkt */
    CARD8       data_config_cmd_key;       /* Keyboard cmd_key (KeyCode) */
/*
 * cmd_key is intentionally *not* defined KeyCode since it's definition is
 * ambiguous (int in Intrinsic.h and unsigned char in X.h.
 */
    CARD8      data_config_pad[1L];           /* pad out to a quadword */
    /* End Shadow (XETrapCfg) */
    CARD32      pad1;
} xXTrapGetCurReply;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_xXTrapGetCurReply  sizeof(xXTrapGetCurReply)

/* reply sent back by XETrapGetStatistics request */
/* Note:
 * The following does *not* use the standard XETrapRepHdr, but instead
 * one which is padded out to 32-bytes.  This is because Cray's have a problem
 * reading arrays of CARD32s without using the _Read32 macro (see XERqsts.c).
 * This requires that none of the data be in the _Reply area.
 */
typedef struct
{
    CARD8  type;
    CARD8  detail;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad0;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    XETrapGetStatsRep  data;
} xXTrapGetStatsReply;
#define sz_xXTrapGetStatsReply  1088

typedef struct /* the XTrap Output header (for output from ext to client) */
{   /* this must be quadword aligned for portability */
    CARD32   count;               /* Length including this header */
    CARD32   timestamp;           /* timestamp if desired */
    CARD8    type;                /* event id, request id, special id */
    CARD8    screen;              /* screen number (0 to n) */
    INT16    win_x;               /* X coord of drawable, if any */
    INT16    win_y;               /* X coord of drawable, if any */
    CARD16   client;              /* to distinguish requests */
} XETrapHeader;
/* the following works because all fields are defined as bit (Bnn) fields */
#define sz_XETrapHeader   sizeof(XETrapHeader)

#define XETrapHeaderIsEvent(phdr)       (XETrapGetHeaderType(phdr) == 0x1L)
#define XETrapHeaderIsRequest(phdr)     (XETrapGetHeaderType(phdr) == 0x2L)
#define XETrapHeaderIsSpecial(phdr)     (XETrapGetHeaderType(phdr) == 0x3L)
#define XETrapHeaderIsCursor(phdr)      (XETrapGetHeaderType(phdr) == 0x4L)
#define XETrapHeaderIsReply(phdr)       (XETrapGetHeaderType(phdr) == 0x5L)

/* Define a structure used for reading/writing datum of type XTrap */
typedef struct
{
    XETrapHeader hdr;
    union
    {
        xEvent          event;
        xResourceReq    req;
        xGenericReply   reply;
        /* special? */
    } u;
} XETrapDatum;

/* this doesn't get picked up for VMS server builds (different Xproto.h) */
#ifndef sz_xEvent
#define sz_xEvent 32
#endif
/* Minimum size of a packet from the server extension */
#define XETrapMinPktSize    (SIZEOF(XETrapHeader) + SIZEOF(xEvent))

/* Constants used with the XLIB transport */
#define XETrapDataStart      0L    /* Used in the detail field */
#define XETrapDataContinued  1L    /* Used in the detail field */
#define XETrapDataLast       2L    /* Used in the detail field */
#define XETrapData           0L    /* Used in the type field */
#define XETrapNumberEvents   1L
/* This is the representation on the wire(see also XLib.h) */
#define sz_EventData         24L   /* 32 bytes - type, detail, seq, index */
typedef struct {
    CARD8  type;
    CARD8  detail;
    CARD16 sequenceNumber;
    CARD32 idx;
    CARD8  data[sz_EventData];
} xETrapDataEvent;

/* Error message indexes added to X for extension */
#define BadIO               2L      /* Can't read/write */
#define BadStatistics       4L      /* Stat's not avail. */
#define BadDevices          5L      /* Devices not vectored */
#define BadScreen           7L      /* Can't send event to given screen */
#define BadSwapReq          8L      /* Can't send swapped extension requests */
#define XETrapNumErrors     (BadSwapReq + 1)


#define XEKeyIsClear       0
#define XEKeyIsEcho        1
#define XEKeyIsOther       2

#endif /* __XTRAPDI__ */
