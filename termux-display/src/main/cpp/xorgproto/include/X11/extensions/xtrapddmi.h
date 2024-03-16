/* $XFree86$ */

#ifndef __XTRAPDDMI__
#define __XTRAPDDMI__

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991 by Digital Equipment Corp., Maynard, MA

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
 *      This header file is used by the XTrap server extension only
 *      (not used by clients or the XTrap Toolkit).  Information
 *      contained herein should *not* be visible to clients (xtrapdi.h
 *      is used for this).  The name is historical.
 */
#include <X11/X.h>
#include <X11/extensions/xtrapbits.h>
#include "dix.h"

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define XETrapNumEvents    1L  /* constants used for AddExtension */

/* Other constants used within the extension code */
#define XETrapMinRepSize        32L        /* Minimum number of longs */

/* This structure will be globally declared to provide storage to hold
 * the various extension wide configuration information.  Allocated on
 * a per-client basis.
 */
typedef struct
{
    ClientPtr            client;  /* Multi-client support and error handling */
    xXTrapGetCurReply    cur;    /* Struct of Miscellaneous state info */
    xXTrapGetStatsReply  *stats; /* Pointer to stat's, malloc'd if requested */
    CARD32 last_input_time;      /* last timestamp from input event */
    CARD16 protocol;             /* current communication protocol */
} XETrapEnv;

#define XETrapSetHeaderEvent(phdr)      ((phdr)->type = 0x1L)
#define XETrapSetHeaderRequest(phdr)    ((phdr)->type = 0x2L)
#define XETrapSetHeaderSpecial(phdr)    ((phdr)->type = 0x3L)
#define XETrapSetHeaderCursor(phdr)     ((phdr)->type = 0x4L)
#define XETrapSetHeaderReply(phdr)      ((phdr)->type = 0x5L)

#ifndef vaxc
#define globaldef
#define globalref extern
#endif

/* Extension platform identifier (conditionally defined) */
#if ( defined (__osf__) && defined(__alpha) )
# define XETrapPlatform    PF_DECOSF1
#endif
#ifdef ultrix
# define XETrapPlatform    PF_DECUltrix
#endif
#ifdef vms
#ifdef VAXELN
# define XETrapPlatform    PF_DECELN
#else
# define XETrapPlatform    PF_DECVMS
#endif
#endif
#ifdef VT1000
# define XETrapPlatform    PF_DECVT1000
#endif
#ifdef VXT
# define XETrapPlatform    PF_DECXTerm
#endif
#ifdef PC
# define XETrapPlatform    PF_IBMAT
#endif
#ifdef sun
# define XETrapPlatform    PF_SunSparc
#endif
#ifndef XETrapPlatform
# define XETrapPlatform    PF_Other
#endif  /* XETrapPlatform */

#endif /* __XTRAPDDMI__ */
