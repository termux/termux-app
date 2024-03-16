/*
 * Copyright © 2003 Philip Blundell
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Philip Blundell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Philip Blundell makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * PHILIP BLUNDELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL PHILIP BLUNDELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef XCALIBRATEPROTO_H
#define XCALIBRATEPROTO_H

/*
 * requests and replies
 */
typedef struct {
    CARD8   reqType;
    CARD8   xCalibrateReqType;
    CARD16  length;
    CARD32  majorVersion;
    CARD32  minorVersion;
} xXCalibrateQueryVersionReq;

#define sz_xXCalibrateQueryVersionReq   12

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  majorVersion;
    CARD32  minorVersion;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXCalibrateQueryVersionReply;

#define sz_xXCalibrateQueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xCalibrateReqType;
    CARD16  length;
    CARD16  on;
    CARD16  pad;
} xXCalibrateRawModeReq;

#define sz_xXCalibrateRawModeReq	8

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber;
    CARD32  status;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
    CARD32  pad7;
} xXCalibrateRawModeReply;

#define sz_xXCalibrateRawModeReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xCalibrateReqType;
    CARD16  length;
    CARD32  x;
    CARD32  y;
} xXCalibrateScreenToCoordReq;

#define sz_xXCalibrateScreenToCoordReq	12

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber;
    CARD32  x;
    CARD32  y;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} xXCalibrateScreenToCoordReply;

#define sz_xXCalibrateScreenToCoordReply	32

/* Events */

typedef struct {
    CARD8	type;
    CARD8	pad1;
    CARD16	sequenceNumber;
    CARD16	x;
    CARD16	y;
    CARD16	pressure;
    CARD16      pad2;
} xXCalibrateRawTouchscreenEvent;

#define sz_xXCalibrateRawTouchscreenEvent 12

#endif
