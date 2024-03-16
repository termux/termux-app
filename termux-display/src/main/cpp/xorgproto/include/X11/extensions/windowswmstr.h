/*
 * WindowsWM extension is based on AppleWM extension
 * Authors:	Kensuke Matsuzaki
 */
/**************************************************************************

Copyright (c) 2002 Apple Computer, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef _WINDOWSWMSTR_H_
#define _WINDOWSWMSTR_H_

#include <X11/extensions/windowswm.h>
#include <X11/X.h>
#include <X11/Xmd.h>

#define WINDOWSWMNAME "Windows-WM"

#define WINDOWS_WM_MAJOR_VERSION	1	/* current version numbers */
#define WINDOWS_WM_MINOR_VERSION	0
#define WINDOWS_WM_PATCH_VERSION	0

typedef struct _WindowsWMQueryVersion {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMQueryVersion */
    CARD16	length;
} xWindowsWMQueryVersionReq;
#define sz_xWindowsWMQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	majorVersion;		/* major version of WM protocol */
    CARD16	minorVersion;		/* minor version of WM protocol */
    CARD32	patchVersion;		/* patch version of WM protocol */
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xWindowsWMQueryVersionReply;
#define sz_xWindowsWMQueryVersionReply	32

typedef struct _WindowsWMDisableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMDisableUpdate */
    CARD16	length;
    CARD32	screen;
} xWindowsWMDisableUpdateReq;
#define sz_xWindowsWMDisableUpdateReq	8

typedef struct _WindowsWMReenableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMReenableUpdate */
    CARD16	length;
    CARD32	screen;
} xWindowsWMReenableUpdateReq;
#define sz_xWindowsWMReenableUpdateReq	8

typedef struct _WindowsWMSelectInput {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMSelectInput */
    CARD16	length;
    CARD32	mask;
} xWindowsWMSelectInputReq;
#define sz_xWindowsWMSelectInputReq	8

typedef struct _WindowsWMNotify {
	BYTE	type;		/* always eventBase + event type */
	BYTE	kind;
	CARD16	sequenceNumber;
	Window	window;
	Time	time;		/* time of change */
	CARD16	pad1;
	CARD32	arg;
	INT16	x;
	INT16	y;
	CARD16	w;
	CARD16	h;
} xWindowsWMNotifyEvent;
#define sz_xWindowsWMNotifyEvent	28

typedef struct _WindowsWMSetFrontProcess {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMSetFrontProcess */
    CARD16	length;
} xWindowsWMSetFrontProcessReq;
#define sz_xWindowsWMSetFrontProcessReq 4

typedef struct _WindowsWMFrameGetRect {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameGetRect */
    CARD16	length;
    CARD32	frame_style;
    CARD32	frame_style_ex;
    CARD16	frame_rect;
    INT16	ix;
    INT16	iy;
    CARD16	iw;
    CARD16	ih;
    CARD16	pad1;
} xWindowsWMFrameGetRectReq;
#define sz_xWindowsWMFrameGetRectReq	24

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    INT16	x;
    INT16	y;
    CARD16	w;
    CARD16	h;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xWindowsWMFrameGetRectReply;
#define sz_xWindowsWMFrameGetRectReply	32

typedef struct _WindowsWMFrameDraw {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameDraw */
    CARD16	length;
    CARD32	screen;
    CARD32	window;
    CARD32	frame_style;
    CARD32	frame_style_ex;
    INT16	ix;
    INT16	iy;
    CARD16	iw;
    CARD16	ih;
} xWindowsWMFrameDrawReq;
#define sz_xWindowsWMFrameDrawReq	28

typedef struct _WindowsWMFrameSetTitle {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameSetTitle */
    CARD16	length;
    CARD32	screen;
    CARD32	window;
    CARD32	title_length;
} xWindowsWMFrameSetTitleReq;
#define sz_xWindowsWMFrameSetTitleReq	16

#endif /* _WINDOWSWMSTR_H_ */
