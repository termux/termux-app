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
 *	Id: fontcache.h,v 1.6 1999/01/31 12:41:32 akiyama Exp $
 */
/* $XFree86: xc/include/extensions/fontcache.h,v 1.3 2001/08/01 00:44:35 tsi Exp $ */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _EXT_FONTCACHE_H_
#define _EXT_FONTCACHE_H_

#define X_FontCacheQueryVersion		0
#define X_FontCacheGetCacheSettings	1
#define X_FontCacheChangeCacheSettings	2
#define X_FontCacheGetCacheStatistics	3

#define FontCacheNumberEvents		0

#define FontCacheBadProtocol		0
#define FontCacheCannotAllocMemory	1
#define FontCacheNumberErrors		(FontCacheCannotAllocMemory + 1)

typedef struct {
    long	himark;
    long	lowmark;
    long	balance;
} FontCacheSettings, *FontCacheSettingsPtr;

struct cacheinfo {
    long	hits;
    long	misshits;
    long	purged;
    long	usage;
};

typedef struct {
    long		purge_runs;
    long		purge_stat;
    long		balance;
    struct cacheinfo	f;
    struct cacheinfo	v;
} FontCacheStatistics, *FontCacheStatisticsPtr;

#ifndef _FONTCACHE_SERVER_

#include <X11/Xlib.h>

_XFUNCPROTOBEGIN

Bool FontCacheQueryVersion(
    Display*		/* dpy */,
    int* 		/* majorVersion */,
    int* 		/* minorVersion */
);

Bool FontCacheQueryExtension(
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
);

Status FontCacheGetCacheSettings(
    Display*			/* dpy */,
    FontCacheSettings*		/* cache info */
);

Status FontCacheChangeCacheSettings(
    Display*			/* dpy */,
    FontCacheSettings*		/* cache info */
);

Status FontCacheGetCacheStatistics(
    Display*			/* dpy */,
    FontCacheStatistics*	/* cache statistics info */
);

_XFUNCPROTOEND

#endif /* !_FONTCACHE_SERVER_ */

#endif /* _EXT_FONTCACHE_H_ */
