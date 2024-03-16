/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "winauth.h"
#include "winmsg.h"

/* Includes for authorization */
#include "securitysrv.h"
#include "os/osdep.h"

#include <xcb/xcb.h>

/*
 * Constants
 */

#define AUTH_NAME	"MIT-MAGIC-COOKIE-1"

/*
 * Locals
 */

static XID g_authId = 0;
static unsigned int g_uiAuthDataLen = 0;
static char *g_pAuthData = NULL;
static xcb_auth_info_t auth_info;

/*
 * Code to generate a MIT-MAGIC-COOKIE-1, copied from under XCSECURITY
 */

#ifndef XCSECURITY
static XID
GenerateAuthorization(unsigned name_length,
                      const char *name,
                      unsigned data_length,
                      const char *data,
                      unsigned *data_length_return, char **data_return)
{
    return MitGenerateCookie(data_length, data,
                             FakeClientID(0), data_length_return, data_return);
}
#endif

/*
 * Generate authorization cookie for internal server clients
 */

BOOL
winGenerateAuthorization(void)
{
#ifdef XCSECURITY
    SecurityAuthorizationPtr pAuth = NULL;
#endif

    /* Call OS layer to generate authorization key */
    g_authId = GenerateAuthorization(strlen(AUTH_NAME),
                                     AUTH_NAME,
                                     0, NULL, &g_uiAuthDataLen, &g_pAuthData);
    if ((XID) ~0L == g_authId) {
        ErrorF("winGenerateAuthorization - GenerateAuthorization failed\n");
        return FALSE;
    }

    else {
        winDebug("winGenerateAuthorization - GenerateAuthorization success!\n"
                 "AuthDataLen: %d AuthData: %s\n",
                 g_uiAuthDataLen, g_pAuthData);
    }

    auth_info.name = strdup(AUTH_NAME);
    auth_info.namelen = strlen(AUTH_NAME);
    auth_info.data = g_pAuthData;
    auth_info.datalen = g_uiAuthDataLen;

#ifdef XCSECURITY
    /* Allocate structure for additional auth information */
    pAuth = (SecurityAuthorizationPtr)
        malloc(sizeof(SecurityAuthorizationRec));
    if (!(pAuth)) {
        ErrorF("winGenerateAuthorization - Failed allocating "
               "SecurityAuthorizationPtr.\n");
        return FALSE;
    }

    /* Fill in the auth fields */
    pAuth->id = g_authId;
    pAuth->timeout = 0;         /* live for x seconds after refcnt == 0 */
    pAuth->group = None;
    pAuth->trustLevel = XSecurityClientTrusted;
    pAuth->refcnt = 1;          /* this auth must stick around */
    pAuth->secondsRemaining = 0;
    pAuth->timer = NULL;
    pAuth->eventClients = NULL;

    /* Add the authorization to the server's auth list */
    if (!AddResource(g_authId, SecurityAuthorizationResType, pAuth)) {
        ErrorF("winGenerateAuthorization - AddResource failed for auth.\n");
        return FALSE;
    }
#endif

    return TRUE;
}

xcb_auth_info_t *
winGetXcbAuthInfo(void)
{
    if (g_pAuthData)
        return &auth_info;

    return NULL;
}
