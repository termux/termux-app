/*

Copyright 1988, 1998  The Open Group

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

/*
 * XDM-AUTHENTICATION-1 (XDMCP authentication) and
 * XDM-AUTHORIZATION-1 (client authorization) protocols
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <X11/Xtrans/Xtrans.h>
#include "os.h"
#include "osdep.h"
#include "dixstruct.h"

#ifdef HASXDMAUTH

static Bool authFromXDMCP;

#ifdef XDMCP
#include <X11/Xmd.h>
#undef REQUEST
#include <X11/Xdmcp.h>

/* XDM-AUTHENTICATION-1 */

static XdmAuthKeyRec privateKey;
static char XdmAuthenticationName[] = "XDM-AUTHENTICATION-1";

#define XdmAuthenticationNameLen (sizeof XdmAuthenticationName - 1)
static XdmAuthKeyRec global_rho;

static Bool
XdmAuthenticationValidator(ARRAY8Ptr privateData, ARRAY8Ptr incomingData,
                           xdmOpCode packet_type)
{
    XdmAuthKeyPtr incoming;

    XdmcpUnwrap(incomingData->data, (unsigned char *) &privateKey,
                incomingData->data, incomingData->length);
    if (packet_type == ACCEPT) {
        if (incomingData->length != 8)
            return FALSE;
        incoming = (XdmAuthKeyPtr) incomingData->data;
        XdmcpDecrementKey(incoming);
        return XdmcpCompareKeys(incoming, &global_rho);
    }
    return FALSE;
}

static Bool
XdmAuthenticationGenerator(ARRAY8Ptr privateData, ARRAY8Ptr outgoingData,
                           xdmOpCode packet_type)
{
    outgoingData->length = 0;
    outgoingData->data = 0;
    if (packet_type == REQUEST) {
        if (XdmcpAllocARRAY8(outgoingData, 8))
            XdmcpWrap((unsigned char *) &global_rho, (unsigned char *) &privateKey,
                      outgoingData->data, 8);
    }
    return TRUE;
}

static Bool
XdmAuthenticationAddAuth(int name_len, const char *name,
                         int data_len, char *data)
{
    Bool ret;

    XdmcpUnwrap((unsigned char *) data, (unsigned char *) &privateKey,
                (unsigned char *) data, data_len);
    authFromXDMCP = TRUE;
    ret = AddAuthorization(name_len, name, data_len, data);
    authFromXDMCP = FALSE;
    return ret;
}

#define atox(c)	('0' <= c && c <= '9' ? c - '0' : \
		 'a' <= c && c <= 'f' ? c - 'a' + 10 : \
		 'A' <= c && c <= 'F' ? c - 'A' + 10 : -1)

static int
HexToBinary(const char *in, char *out, int len)
{
    int top, bottom;

    while (len > 0) {
        top = atox(in[0]);
        if (top == -1)
            return 0;
        bottom = atox(in[1]);
        if (bottom == -1)
            return 0;
        *out++ = (top << 4) | bottom;
        in += 2;
        len -= 2;
    }
    if (len)
        return 0;
    *out++ = '\0';
    return 1;
}

void
XdmAuthenticationInit(const char *cookie, int cookie_len)
{
    memset(privateKey.data, 0, 8);
    if (!strncmp(cookie, "0x", 2) || !strncmp(cookie, "0X", 2)) {
        if (cookie_len > 2 + 2 * 8)
            cookie_len = 2 + 2 * 8;
        HexToBinary(cookie + 2, (char *) privateKey.data, cookie_len - 2);
    }
    else {
        if (cookie_len > 7)
            cookie_len = 7;
        memmove(privateKey.data + 1, cookie, cookie_len);
    }
    XdmcpGenerateKey(&global_rho);
    XdmcpRegisterAuthentication(XdmAuthenticationName, XdmAuthenticationNameLen,
                                (char *) &global_rho,
                                sizeof(global_rho),
                                (ValidatorFunc) XdmAuthenticationValidator,
                                (GeneratorFunc) XdmAuthenticationGenerator,
                                (AddAuthorFunc) XdmAuthenticationAddAuth);
}

#endif                          /* XDMCP */

/* XDM-AUTHORIZATION-1 */
typedef struct _XdmAuthorization {
    struct _XdmAuthorization *next;
    XdmAuthKeyRec rho;
    XdmAuthKeyRec key;
    XID id;
} XdmAuthorizationRec, *XdmAuthorizationPtr;

static XdmAuthorizationPtr xdmAuth;

typedef struct _XdmClientAuth {
    struct _XdmClientAuth *next;
    XdmAuthKeyRec rho;
    char client[6];
    long time;
} XdmClientAuthRec, *XdmClientAuthPtr;

static XdmClientAuthPtr xdmClients;
static long clockOffset;
static Bool gotClock;

#define TwentyMinutes	(20 * 60)
#define TwentyFiveMinutes (25 * 60)

static Bool
XdmClientAuthCompare(const XdmClientAuthPtr a, const XdmClientAuthPtr b)
{
    int i;

    if (!XdmcpCompareKeys(&a->rho, &b->rho))
        return FALSE;
    for (i = 0; i < 6; i++)
        if (a->client[i] != b->client[i])
            return FALSE;
    return a->time == b->time;
}

static void
XdmClientAuthDecode(const unsigned char *plain, XdmClientAuthPtr auth)
{
    int i, j;

    j = 0;
    for (i = 0; i < 8; i++) {
        auth->rho.data[i] = plain[j];
        ++j;
    }
    for (i = 0; i < 6; i++) {
        auth->client[i] = plain[j];
        ++j;
    }
    auth->time = 0;
    for (i = 0; i < 4; i++) {
        auth->time |= plain[j] << ((3 - i) << 3);
        j++;
    }
}

static void
XdmClientAuthTimeout(long now)
{
    XdmClientAuthPtr client, next, prev;

    prev = 0;
    for (client = xdmClients; client; client = next) {
        next = client->next;
        if (labs(now - client->time) > TwentyFiveMinutes) {
            if (prev)
                prev->next = next;
            else
                xdmClients = next;
            free(client);
        }
        else
            prev = client;
    }
}

static XdmClientAuthPtr
XdmAuthorizationValidate(unsigned char *plain, int length,
                         XdmAuthKeyPtr rho, ClientPtr xclient,
                         const char **reason)
{
    XdmClientAuthPtr client, existing;
    long now;
    int i;

    if (length != (192 / 8)) {
        if (reason)
            *reason = "Bad XDM authorization key length";
        return NULL;
    }
    client = malloc(sizeof(XdmClientAuthRec));
    if (!client)
        return NULL;
    XdmClientAuthDecode(plain, client);
    if (!XdmcpCompareKeys(&client->rho, rho)) {
        free(client);
        if (reason)
            *reason = "Invalid XDM-AUTHORIZATION-1 key (failed key comparison)";
        return NULL;
    }
    for (i = 18; i < 24; i++)
        if (plain[i] != 0) {
            free(client);
            if (reason)
                *reason = "Invalid XDM-AUTHORIZATION-1 key (failed NULL check)";
            return NULL;
        }
    if (xclient) {
        int family, addr_len;
        Xtransaddr *addr;

        if (_XSERVTransGetPeerAddr(((OsCommPtr) xclient->osPrivate)->trans_conn,
                                   &family, &addr_len, &addr) == 0
            && _XSERVTransConvertAddress(&family, &addr_len, &addr) == 0) {
#if defined(TCPCONN)
            if (family == FamilyInternet &&
                memcmp((char *) addr, client->client, 4) != 0) {
                free(client);
                free(addr);
                if (reason)
                    *reason =
                        "Invalid XDM-AUTHORIZATION-1 key (failed address comparison)";
                return NULL;

            }
#endif
            free(addr);
        }
    }
    now = time(0);
    if (!gotClock) {
        clockOffset = client->time - now;
        gotClock = TRUE;
    }
    now += clockOffset;
    XdmClientAuthTimeout(now);
    if (labs(client->time - now) > TwentyMinutes) {
        free(client);
        if (reason)
            *reason = "Excessive XDM-AUTHORIZATION-1 time offset";
        return NULL;
    }
    for (existing = xdmClients; existing; existing = existing->next) {
        if (XdmClientAuthCompare(existing, client)) {
            free(client);
            if (reason)
                *reason = "XDM authorization key matches an existing client!";
            return NULL;
        }
    }
    return client;
}

int
XdmAddCookie(unsigned short data_length, const char *data, XID id)
{
    XdmAuthorizationPtr new;
    unsigned char *rho_bits, *key_bits;

    switch (data_length) {
    case 16:                   /* auth from files is 16 bytes long */
#ifdef XDMCP
        if (authFromXDMCP) {
            /* R5 xdm sent bogus authorization data in the accept packet,
             * but we can recover */
            rho_bits = global_rho.data;
            key_bits = (unsigned char *) data;
            key_bits[0] = '\0';
        }
        else
#endif
        {
            rho_bits = (unsigned char *) data;
            key_bits = (unsigned char *) (data + 8);
        }
        break;
#ifdef XDMCP
    case 8:                    /* auth from XDMCP is 8 bytes long */
        rho_bits = global_rho.data;
        key_bits = (unsigned char *) data;
        break;
#endif
    default:
        return 0;
    }
    /* the first octet of the key must be zero */
    if (key_bits[0] != '\0')
        return 0;
    new = malloc(sizeof(XdmAuthorizationRec));
    if (!new)
        return 0;
    new->next = xdmAuth;
    xdmAuth = new;
    memmove(new->key.data, key_bits, (int) 8);
    memmove(new->rho.data, rho_bits, (int) 8);
    new->id = id;
    return 1;
}

XID
XdmCheckCookie(unsigned short cookie_length, const char *cookie,
               ClientPtr xclient, const char **reason)
{
    XdmAuthorizationPtr auth;
    XdmClientAuthPtr client;
    unsigned char *plain;

    /* Auth packets must be a multiple of 8 bytes long */
    if (cookie_length & 7)
        return (XID) -1;
    plain = malloc(cookie_length);
    if (!plain)
        return (XID) -1;
    for (auth = xdmAuth; auth; auth = auth->next) {
        XdmcpUnwrap((unsigned char *) cookie, (unsigned char *) &auth->key,
                    plain, cookie_length);
        if ((client =
             XdmAuthorizationValidate(plain, cookie_length, &auth->rho, xclient,
                                      reason)) != NULL) {
            client->next = xdmClients;
            xdmClients = client;
            free(plain);
            return auth->id;
        }
    }
    free(plain);
    return (XID) -1;
}

int
XdmResetCookie(void)
{
    XdmAuthorizationPtr auth, next_auth;
    XdmClientAuthPtr client, next_client;

    for (auth = xdmAuth; auth; auth = next_auth) {
        next_auth = auth->next;
        free(auth);
    }
    xdmAuth = 0;
    for (client = xdmClients; client; client = next_client) {
        next_client = client->next;
        free(client);
    }
    xdmClients = (XdmClientAuthPtr) 0;
    return 1;
}

int
XdmFromID(XID id, unsigned short *data_lenp, char **datap)
{
    XdmAuthorizationPtr auth;

    for (auth = xdmAuth; auth; auth = auth->next) {
        if (id == auth->id) {
            *data_lenp = 16;
            *datap = (char *) &auth->rho;
            return 1;
        }
    }
    return 0;
}

int
XdmRemoveCookie(unsigned short data_length, const char *data)
{
    XdmAuthorizationPtr auth;
    XdmAuthKeyPtr key_bits, rho_bits;

    switch (data_length) {
    case 16:
        rho_bits = (XdmAuthKeyPtr) data;
        key_bits = (XdmAuthKeyPtr) (data + 8);
        break;
#ifdef XDMCP
    case 8:
        rho_bits = &global_rho;
        key_bits = (XdmAuthKeyPtr) data;
        break;
#endif
    default:
        return 0;
    }
    for (auth = xdmAuth; auth; auth = auth->next) {
        if (XdmcpCompareKeys(rho_bits, &auth->rho) &&
            XdmcpCompareKeys(key_bits, &auth->key)) {
            xdmAuth = auth->next;
            free(auth);
            return 1;
        }
    }
    return 0;
}

#endif
