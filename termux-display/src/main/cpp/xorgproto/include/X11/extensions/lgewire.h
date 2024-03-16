/************************************************************

Copyright (c) 2004, Sun Microsystems, Inc.

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

********************************************************/

/*
 * lge.h - Looking Glass Extension Definitions
 */

#ifndef _LGEWIRE_H
#define _LGEWIRE_H

#include "X11/Xfuncproto.h"

#define LGE_NAME "LGE"

/* Current interface version numbers */
#define LGE_MAJOR_VERSION   5
#define LGE_MINOR_VERSION   0

/* Display Server is alive */
#define  X_LgeQueryVersion	     0
#define  X_LgeRegisterClient         1
#define  X_LgeRegisterScreen         2
#define  X_LgeControlLgMode          3
#define  X_LgeSendEvent              4

/* Arguments to XLgeRegisterClient */
#define LGE_CLIENT_GENERIC	     0
#define LGE_CLIENT_PICKER            1
#define LGE_CLIENT_EVENT_DELIVERER   2

typedef struct {
    CARD8       reqType;
    CARD8       lgeReqType;
    CARD16	length;
} xLgeQueryVersionReq;

#define sz_xLgeQueryVersionReq	sizeof(xLgeQueryVersionReq)

typedef struct {
    /* Always X_Reply */
    BYTE        type;
    CARD8       unused;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	majorVersion;
    CARD32	minorVersion;
    CARD32	implementation;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
} xLgeQueryVersionReply;

#define sz_xLgeQueryVersionReply sizeof(xLgeQueryVersionReply)

typedef struct {
    CARD8  reqType;
    CARD8  lgeReqType;
    CARD16 length;
    CARD8  clientType;
    BOOL   sendEventDirect;
    CARD16 pad2;
} xLgeRegisterClientReq;

#define sz_xLgeRegisterClientReq sizeof(xLgeRegisterClientReq)

typedef struct {
    CARD8  reqType;
    CARD8  lgeReqType;
    CARD16 length;
    /* The pseudo-root window of the screen */
    Window prw;
} xLgeRegisterScreenReq;

#define sz_xLgeRegisterScreenReq sizeof(xLgeRegisterScreenReq)

typedef struct {
    CARD8     reqType;
    CARD8     lgeReqType;
    CARD16    length;
    BOOL      enable;
    CARD8     pad1;
    CARD16    pad2;
} xLgeControlLgModeReq;

#define sz_xLgeControlLgModeReq sizeof(xLgeControlLgModeReq)

typedef struct {
    CARD8     reqType;
    CARD8     lgeReqType;
    CARD16    length;
    xEvent    event;
} xLgeSendEventReq;

#define sz_xLgeSendEventReq sizeof(xLgeSendEventReq)

#endif /* LGEWIRE_H */

