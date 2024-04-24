/* $XFree86: xc/include/extensions/xf86rushstr.h,v 1.4 2000/02/29 03:09:02 dawes Exp $ */
/*

Copyright (c) 1998  Daryll Strauss

*/

#ifndef _XF86RUSHSTR_H_
#define _XF86RUSHSTR_H_

#include <X11/extensions/xf86rush.h>

#define XF86RUSHNAME "XFree86-Rush"

#define XF86RUSH_MAJOR_VERSION	1	/* current version numbers */
#define XF86RUSH_MINOR_VERSION	1

typedef struct _XF86RushQueryVersion {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushQueryVersion */
    CARD16	length;
} xXF86RushQueryVersionReq;
#define sz_xXF86RushQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	majorVersion;		/* major version of Rush protocol */
    CARD16	minorVersion;		/* minor version of Rush protocol */
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86RushQueryVersionReply;
#define sz_xXF86RushQueryVersionReply	32

typedef struct _XF86RushLockPixmap {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushLockPixmap */
    CARD16	length;
    CARD16	screen;
    CARD16	pad;
    CARD32	pixmap;
} xXF86RushLockPixmapReq;
#define sz_xXF86RushLockPixmapReq	12

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	addr;
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86RushLockPixmapReply;
#define sz_xXF86RushLockPixmapReply	32

typedef struct _XF86RushUnlockPixmap {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushUnlockPixmap */
    CARD16	length;
    CARD16	screen;
    CARD16	pad;
    CARD32	pixmap;
} xXF86RushUnlockPixmapReq;
#define sz_xXF86RushUnlockPixmapReq	12

typedef struct _XF86RushUnlockAllPixmaps {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushUnlockAllPixmaps */
    CARD16      length;
} xXF86RushUnlockAllPixmapsReq;
#define sz_xXF86RushUnlockAllPixmapsReq   4

typedef struct _XF86RushSetCopyMode {
    CARD8       reqType;                /* always RushReqCode */
    CARD8       rushReqType;            /* always X_RushSetCopyMode */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
    CARD32      CopyMode;
} xXF86RushSetCopyModeReq;
#define sz_xXF86RushSetCopyModeReq      12


typedef struct _XF86RushSetPixelStride {
    CARD8       reqType;                /* always RushReqCode */
    CARD8       rushReqType;            /* always X_RushSetCopyMode */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
    CARD32      PixelStride;
} xXF86RushSetPixelStrideReq;
#define sz_xXF86RushSetPixelStrideReq      12

typedef struct {
    CARD8	reqType;               /* always RushReqCode */
    CARD8	rushReqType;           /* always X_RushOverlayPixmap */
    CARD16	length;
    XvPortID	port;
    Drawable	drawable;
    GContext	gc;
    Pixmap	pixmap;
    CARD32	id;
    INT16	src_x;
    INT16	src_y;
    CARD16	src_w;
    CARD16	src_h;
    INT16	drw_x;
    INT16	drw_y;
    CARD16	drw_w;
    CARD16	drw_h;
} xXF86RushOverlayPixmapReq;
#define sz_xXF86RushOverlayPixmapReq 40

typedef struct _XF86RushStatusRegOffset {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushStatusRegOffset */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
} xXF86RushStatusRegOffsetReq;
#define sz_xXF86RushStatusRegOffsetReq   8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	offset;			/* Offset of Rush status reg in mem */
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86RushStatusRegOffsetReply;
#define sz_xXF86RushStatusRegOffsetReply	32

typedef struct _XF86RushAT3DEnableRegs {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushAT3DEnableRegs */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
} xXF86RushAT3DEnableRegsReq;
#define sz_xXF86RushAT3DEnableRegsReq   8

typedef struct _XF86RushAT3DDisableRegs {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushAT3DDisableRegs */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
} xXF86RushAT3DDisableRegsReq;
#define sz_xXF86RushAT3DDisableRegsReq   8

#endif /* _XF86RUSHSTR_H_ */
