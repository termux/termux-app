/*
Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/

/* Xserver internals for Security extension - moved here from
   _SECURITY_SERVER section of <X11/extensions/security.h> */

#ifndef _SECURITY_SRV_H
#define _SECURITY_SRV_H

/* Allow client side portions of <X11/extensions/security.h> to compile */
#ifndef Status
#define Status int
#define NEED_UNDEF_Status
#endif
#ifndef Display
#define Display void
#define NEED_UNDEF_Display
#endif

#include <X11/extensions/secur.h>

#ifdef NEED_UNDEF_Status
#undef Status
#undef NEED_UNDEF_Status
#endif
#ifdef NEED_UNDEF_Display
#undef Display
#undef NEED_UNDEF_Display
#endif

#include "input.h"              /* for DeviceIntPtr */
#include "property.h"           /* for PropertyPtr */
#include "pixmap.h"             /* for DrawablePtr */
#include "resource.h"           /* for RESTYPE */

/* resource type to pass in LookupIDByType for authorizations */
extern RESTYPE SecurityAuthorizationResType;

/* this is what we store for an authorization */
typedef struct {
    XID id;                     /* resource ID */
    CARD32 timeout;             /* how long to live in seconds after refcnt == 0 */
    unsigned int trustLevel;    /* trusted/untrusted */
    XID group;                  /* see embedding extension */
    unsigned int refcnt;        /* how many clients connected with this auth */
    unsigned int secondsRemaining;      /* overflow time amount for >49 days */
    OsTimerPtr timer;           /* timer for this auth */
    struct _OtherClients *eventClients; /* clients wanting events */
} SecurityAuthorizationRec, *SecurityAuthorizationPtr;

typedef struct {
    XID group;                  /* the group that was sent in GenerateAuthorization */
    Bool valid;                 /* did anyone recognize it? if so, set to TRUE */
} SecurityValidateGroupInfoRec;

/* Give this value or higher to the -audit option to get security messages */
#define SECURITY_AUDIT_LEVEL 4

#endif                          /* _SECURITY_SRV_H */
