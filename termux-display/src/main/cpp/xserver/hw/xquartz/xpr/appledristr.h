/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright 2000 VA Linux Systems, Inc.
   Copyright (c) 2002-2012 Apple Computer, Inc.
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

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Fiath <faith@valinux.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 *
 */

#ifndef _APPLEDRISTR_H_
#define _APPLEDRISTR_H_

#include "appledri.h"

#define APPLEDRINAME            "Apple-DRI"

#define APPLE_DRI_MAJOR_VERSION 1       /* current version numbers */
#define APPLE_DRI_MINOR_VERSION 0
#define APPLE_DRI_PATCH_VERSION 0

typedef struct _AppleDRIQueryVersion {
    CARD8 reqType;               /* always DRIReqCode */
    CARD8 driReqType;            /* always X_DRIQueryVersion */
    CARD16 length;
} xAppleDRIQueryVersionReq;
#define sz_xAppleDRIQueryVersionReq 4

typedef struct {
    BYTE type;                   /* X_Reply */
    BOOL pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD16 majorVersion;         /* major version of DRI protocol */
    CARD16 minorVersion;         /* minor version of DRI protocol */
    CARD32 patchVersion;         /* patch version of DRI protocol */
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xAppleDRIQueryVersionReply;
#define sz_xAppleDRIQueryVersionReply 32

typedef struct _AppleDRIQueryDirectRenderingCapable {
    CARD8 reqType;               /* always DRIReqCode */
    CARD8 driReqType;            /* X_DRIQueryDirectRenderingCapable */
    CARD16 length;
    CARD32 screen;
} xAppleDRIQueryDirectRenderingCapableReq;
#define sz_xAppleDRIQueryDirectRenderingCapableReq 8

typedef struct {
    BYTE type;                   /* X_Reply */
    BOOL pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    BOOL isCapable;
    BOOL pad2;
    BOOL pad3;
    BOOL pad4;
    CARD32 pad5;
    CARD32 pad6;
    CARD32 pad7;
    CARD32 pad8;
    CARD32 pad9;
} xAppleDRIQueryDirectRenderingCapableReply;
#define sz_xAppleDRIQueryDirectRenderingCapableReply 32

typedef struct _AppleDRIAuthConnection {
    CARD8 reqType;               /* always DRIReqCode */
    CARD8 driReqType;            /* always X_DRICloseConnection */
    CARD16 length;
    CARD32 screen;
    CARD32 magic;
} xAppleDRIAuthConnectionReq;
#define sz_xAppleDRIAuthConnectionReq 12

typedef struct {
    BYTE type;
    BOOL pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 authenticated;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xAppleDRIAuthConnectionReply;
#define zx_xAppleDRIAuthConnectionReply 32

typedef struct _AppleDRICreateSurface {
    CARD8 reqType;               /* always DRIReqCode */
    CARD8 driReqType;            /* always X_DRICreateSurface */
    CARD16 length;
    CARD32 screen;
    CARD32 drawable;
    CARD32 client_id;
} xAppleDRICreateSurfaceReq;
#define sz_xAppleDRICreateSurfaceReq 16

typedef struct {
    BYTE type;                   /* X_Reply */
    BOOL pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 key_0;
    CARD32 key_1;
    CARD32 uid;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xAppleDRICreateSurfaceReply;
#define sz_xAppleDRICreateSurfaceReply 32

typedef struct _AppleDRIDestroySurface {
    CARD8 reqType;               /* always DRIReqCode */
    CARD8 driReqType;            /* always X_DRIDestroySurface */
    CARD16 length;
    CARD32 screen;
    CARD32 drawable;
} xAppleDRIDestroySurfaceReq;
#define sz_xAppleDRIDestroySurfaceReq 12

typedef struct _AppleDRINotify {
    BYTE type;                   /* always eventBase + event type */
    BYTE kind;
    CARD16 sequenceNumber;
    CARD32 time;                 /* time of change */
    CARD32 pad1;
    CARD32 arg;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xAppleDRINotifyEvent;
#define sz_xAppleDRINotifyEvent 32

typedef struct {
    CARD8 reqType;
    CARD8 driReqType;
    CARD16 length;
    CARD32 screen;
    CARD32 drawable;
    BOOL doubleSwap;
    CARD8 pad1, pad2, pad3;
} xAppleDRICreateSharedBufferReq;

#define sz_xAppleDRICreateSharedBufferReq 16

typedef struct {
    BYTE type;
    BYTE data1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 stringLength;         /* 0 on error */
    CARD32 width;
    CARD32 height;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
} xAppleDRICreateSharedBufferReply;

#define sz_xAppleDRICreateSharedBufferReply 32

typedef struct {
    CARD8 reqType;
    CARD8 driReqType;
    CARD16 length;
    CARD32 screen;
    CARD32 drawable;
} xAppleDRISwapBuffersReq;

#define sz_xAppleDRISwapBuffersReq 12

typedef struct {
    CARD8 reqType;               /*1 */
    CARD8 driReqType;            /*2 */
    CARD16 length;               /*4 */
    CARD32 screen;               /*8 */
    CARD32 drawable;             /*12 */
} xAppleDRICreatePixmapReq;

#define sz_xAppleDRICreatePixmapReq 12

typedef struct {
    BYTE type;                   /*1 */
    BOOL pad1;                   /*2 */
    CARD16 sequenceNumber;       /*4 */
    CARD32 length;               /*8 */
    CARD32 width;                /*12 */
    CARD32 height;               /*16 */
    CARD32 pitch;                /*20 */
    CARD32 bpp;                  /*24 */
    CARD32 size;                 /*28 */
    CARD32 stringLength;         /*32 */
} xAppleDRICreatePixmapReply;

#define sz_xAppleDRICreatePixmapReply 32

typedef struct {
    CARD8 reqType;               /*1 */
    CARD8 driReqType;            /*2 */
    CARD16 length;               /*4 */
    CARD32 drawable;             /*8 */
} xAppleDRIDestroyPixmapReq;

#define sz_xAppleDRIDestroyPixmapReq 8

#ifdef _APPLEDRI_SERVER_

void AppleDRISendEvent(
#if NeedFunctionPrototypes
    int /* type */,
    unsigned int /* mask */,
    int /* which */,
    int                       /* arg */
#endif
    );

#endif /* _APPLEDRI_SERVER_ */
#endif /* _APPLEDRISTR_H_ */
