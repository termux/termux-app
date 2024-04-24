/*-
 * Copyright (c) 1998-1999 Shunsuke Akiyama <akiyama@jp.FreeBSD.org>.
 * All rights reserved.
 * Copyright (c) 1998-1999 X-TrueType Server Project, All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: fontcachstr.h,v 1.7 1999/01/31 14:58:40 akiyama Exp $
 */
/* $XFree86$ */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _FONTCACHESTR_H_
#define _FONTCACHESTR_H_

#include <X11/extensions/fontcache.h>

#define FONTCACHENAME		"FontCache"

#define FONTCACHE_MAJOR_VERSION	0	/* current version numbers */
#define FONTCACHE_MINOR_VERSION	1

typedef struct _FontCacheQueryVersion {
    CARD8	reqType;		/* always FontCacheReqCode */
    CARD8	fontcacheReqType;	/* always X_FontCacheQueryVersion */
    CARD16	length;
} xFontCacheQueryVersionReq;
#define sz_xFontCacheQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD16	majorVersion;		/* major version of Font-Cache */
    CARD16	minorVersion;		/* minor version of Font-Cache */
    CARD32	pad2;
    CARD32	pad3;
    CARD32	pad4;
    CARD32	pad5;
    CARD32	pad6;
} xFontCacheQueryVersionReply;
#define sz_xFontCacheQueryVersionReply	32

typedef struct _FontCacheGetCacheSettings {
    CARD8	reqType;		/* always FontCacheReqCode */
    CARD8	fontcacheReqType;	/* always X_FontCacheGetCacheSettings */
    CARD16	length;
} xFontCacheGetCacheSettingsReq;
#define sz_xFontCacheGetCacheSettingsReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	himark;
    CARD32	lowmark;
    CARD32	balance;
    CARD32	reserve0;
    CARD32	reserve1;
    CARD32	reserve2;
} xFontCacheGetCacheSettingsReply;
#define sz_xFontCacheGetCacheSettingsReply	32

typedef struct _FontCacheChangeCacheSettings {
    CARD8	reqType;		/* always FontCacheReqCode */
    CARD8	fontcacheReqType;	/* always X_FontCacheChangeCacheSettings */
    CARD16	length;
    CARD32	himark;
    CARD32	lowmark;
    CARD32	balance;
    CARD32	reserve0;
    CARD32	reserve1;
    CARD32	reserve2;
    CARD32	reserve3;
} xFontCacheChangeCacheSettingsReq;
#define sz_xFontCacheChangeCacheSettingsReq	32

typedef struct _FontCacheGetCacheStatistics {
    CARD8	reqType;		/* always FontCacheReqCode */
    CARD8	fontcacheReqType;	/* always X_FontCacheGetCacheStatistics */
    CARD16	length;
} xFontCacheGetCacheStatisticsReq;
#define sz_xFontCacheGetCacheStatisticsReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber;
    CARD32	length;
    CARD32	purge_runs;
    CARD32	purge_stat;
    CARD32	balance;
    CARD32	reserve0;
    CARD32	f_hits;
    CARD32	f_misshits;
    CARD32	f_purged;
    CARD32	f_usage;
    CARD32	f_reserve0;
    CARD32	v_hits;
    CARD32	v_misshits;
    CARD32	v_purged;
    CARD32	v_usage;
    CARD32	v_reserve0;
} xFontCacheGetCacheStatisticsReply;
#define sz_xFontCacheGetCacheStatisticsReply	64

#endif /* _FONTCACHESTR_H_ */
