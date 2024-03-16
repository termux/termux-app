/* $XFree86: xc/include/extensions/xf86mscstr.h,v 3.12 2002/11/20 04:04:56 dawes Exp $ */

/*
 * Copyright (c) 1995, 1996  The XFree86 Project, Inc
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86MISCSTR_H_
#define _XF86MISCSTR_H_

#include <X11/extensions/xf86misc.h>

#define XF86MISCNAME		"XFree86-Misc"

#define XF86MISC_MAJOR_VERSION	0	/* current version numbers */
#define XF86MISC_MINOR_VERSION	9

typedef struct _XF86MiscQueryVersion {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscQueryVersion */
    CARD16	length;
} xXF86MiscQueryVersionReq;
#define sz_xXF86MiscQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	majorVersion;		/* major version of XFree86-Misc */
    CARD16	minorVersion;		/* minor version of XFree86-Misc */
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86MiscQueryVersionReply;
#define sz_xXF86MiscQueryVersionReply	32

#ifdef _XF86MISC_SAVER_COMPAT_
typedef struct _XF86MiscGetSaver {
    CARD8       reqType;                /* always XF86MiscReqCode */
    CARD8       xf86miscReqType;     /* always X_XF86MiscGetSaver */
    CARD16      length;
    CARD16      screen;
    CARD16      pad;
} xXF86MiscGetSaverReq;
#define sz_xXF86MiscGetSaverReq	8

typedef struct _XF86MiscSetSaver {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscSetSaver */
    CARD16	length;
    CARD16	screen;
    CARD16	pad;
    CARD32	suspendTime;
    CARD32	offTime;
} xXF86MiscSetSaverReq;
#define sz_xXF86MiscSetSaverReq	16

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	suspendTime;
    CARD32	offTime;
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
} xXF86MiscGetSaverReply;
#define sz_xXF86MiscGetSaverReply	32
#endif

typedef struct _XF86MiscGetMouseSettings {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscGetMouseSettings */
    CARD16	length;
} xXF86MiscGetMouseSettingsReq;
#define sz_xXF86MiscGetMouseSettingsReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	mousetype;
    CARD32	baudrate;
    CARD32	samplerate;
    CARD32	resolution;
    CARD32	buttons;
    BOOL	emulate3buttons;
    BOOL	chordmiddle;
    CARD16	pad2;
    CARD32	emulate3timeout;
    CARD32	flags;
    CARD32	devnamelen;		/* strlen(device)+1 */
} xXF86MiscGetMouseSettingsReply;
#define sz_xXF86MiscGetMouseSettingsReply	44

typedef struct _XF86MiscGetKbdSettings {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscGetKbdSettings */
    CARD16	length;
} xXF86MiscGetKbdSettingsReq;
#define sz_xXF86MiscGetKbdSettingsReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	kbdtype;
    CARD32	rate;
    CARD32	delay;
    BOOL	servnumlock;
    BOOL	pad2;
    CARD16	pad3;
    CARD32	pad4;
    CARD32	pad5;
} xXF86MiscGetKbdSettingsReply;
#define sz_xXF86MiscGetKbdSettingsReply	32

typedef struct _XF86MiscSetMouseSettings {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscSetMouseSettings */
    CARD16	length;
    CARD32	mousetype;
    CARD32	baudrate;
    CARD32	samplerate;
    CARD32	resolution;
    CARD32	buttons;
    BOOL	emulate3buttons;
    BOOL	chordmiddle;
    CARD16	devnamelen;
    CARD32	emulate3timeout;
    CARD32	flags;
} xXF86MiscSetMouseSettingsReq;
#define sz_xXF86MiscSetMouseSettingsReq	36

typedef struct _XF86MiscSetKbdSettings {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscSetKbdSettings */
    CARD16	length;
    CARD32	kbdtype;
    CARD32	rate;
    CARD32	delay;
    BOOL	servnumlock;
    BOOL	pad1;
    CARD16	pad2;
} xXF86MiscSetKbdSettingsReq;
#define sz_xXF86MiscSetKbdSettingsReq	20

typedef struct _XF86MiscSetGrabKeysState {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscSetKbdSettings */
    CARD16	length;
    BOOL	enable;
    BOOL	pad1;
    CARD16	pad2;
} xXF86MiscSetGrabKeysStateReq;
#define sz_xXF86MiscSetGrabKeysStateReq	8

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	status;
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86MiscSetGrabKeysStateReply;
#define sz_xXF86MiscSetGrabKeysStateReply	32

typedef struct _XF86MiscSetClientVersion {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;
    CARD16	length;
    CARD16	major;
    CARD16	minor;
} xXF86MiscSetClientVersionReq;
#define sz_xXF86MiscSetClientVersionReq	8

typedef struct _XF86MiscGetFilePaths {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscGetFilePaths */
    CARD16	length;
} xXF86MiscGetFilePathsReq;
#define sz_xXF86MiscGetFilePathsReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	configlen;
    CARD16	modulelen;
    CARD16	loglen;
    CARD16	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86MiscGetFilePathsReply;
#define sz_xXF86MiscGetFilePathsReply	32

typedef struct _XF86MiscPassMessage {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscPassMessage */
    CARD16	length;
    CARD16	typelen;
    CARD16	vallen;
    CARD16	screen;
    CARD16	pad;
} xXF86MiscPassMessageReq;
#define sz_xXF86MiscPassMessageReq	12

typedef struct {
    BYTE	type;			/* X_Reply */
    BYTE	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	mesglen;
    CARD16	pad2;
    CARD32	status;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xXF86MiscPassMessageReply;
#define sz_xXF86MiscPassMessageReply	32

#endif /* _XF86MISCSTR_H_ */
