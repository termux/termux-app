/*

Copyright 1991, 1998  The Open Group

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
 * SUN-DES-1 authentication mechanism
 * Author:  Mayank Choudhary, Sun Microsystems
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef SECURE_RPC

#include <X11/X.h>
#include <X11/Xauth.h>
#include "misc.h"
#include "os.h"
#include "osdep.h"
#include "dixstruct.h"

#include <rpc/rpc.h>

#ifdef __sun
/* <rpc/auth.h> only includes this if _KERNEL is #defined... */
extern bool_t xdr_opaque_auth(XDR *, struct opaque_auth *);
#endif

static enum auth_stat why;

static char *
authdes_ezdecode(const char *inmsg, int len)
{
    struct rpc_msg msg;
    char cred_area[MAX_AUTH_BYTES];
    char verf_area[MAX_AUTH_BYTES];
    char *temp_inmsg;
    struct svc_req r;
    bool_t res0, res1;
    XDR xdr;
    SVCXPRT xprt;

    temp_inmsg = malloc(len);
    if (temp_inmsg == NULL) {
        why = AUTH_FAILED; /* generic error, since there is no AUTH_BADALLOC */
        return NULL;
    }
    memmove(temp_inmsg, inmsg, len);

    memset((char *) &msg, 0, sizeof(msg));
    memset((char *) &r, 0, sizeof(r));
    memset(cred_area, 0, sizeof(cred_area));
    memset(verf_area, 0, sizeof(verf_area));

    msg.rm_call.cb_cred.oa_base = cred_area;
    msg.rm_call.cb_verf.oa_base = verf_area;
    why = AUTH_FAILED;
    xdrmem_create(&xdr, temp_inmsg, len, XDR_DECODE);

    if ((r.rq_clntcred = malloc(MAX_AUTH_BYTES)) == NULL)
        goto bad1;
    r.rq_xprt = &xprt;

    /* decode into msg */
    res0 = xdr_opaque_auth(&xdr, &(msg.rm_call.cb_cred));
    res1 = xdr_opaque_auth(&xdr, &(msg.rm_call.cb_verf));
    if (!(res0 && res1))
        goto bad2;

    /* do the authentication */

    r.rq_cred = msg.rm_call.cb_cred;    /* read by opaque stuff */
    if (r.rq_cred.oa_flavor != AUTH_DES) {
        why = AUTH_TOOWEAK;
        goto bad2;
    }
#ifdef SVR4
    if ((why = __authenticate(&r, &msg)) != AUTH_OK) {
#else
    if ((why = _authenticate(&r, &msg)) != AUTH_OK) {
#endif
        goto bad2;
    }
    return (((struct authdes_cred *) r.rq_clntcred)->adc_fullname.name);

 bad2:
    free(r.rq_clntcred);
 bad1:
    return ((char *) 0);        /* ((struct authdes_cred *) NULL); */
}

static XID rpc_id = (XID) ~0L;

static Bool
CheckNetName(unsigned char *addr, short len, void *closure)
{
    return (len == strlen((char *) closure) &&
            strncmp((char *) addr, (char *) closure, len) == 0);
}

static char rpc_error[MAXNETNAMELEN + 50];

_X_HIDDEN XID
SecureRPCCheck(unsigned short data_length, const char *data,
               ClientPtr client, const char **reason)
{
    char *fullname;

    if (rpc_id == (XID) ~0L) {
        *reason = "Secure RPC authorization not initialized";
    }
    else {
        fullname = authdes_ezdecode(data, data_length);
        if (fullname == (char *) 0) {
            snprintf(rpc_error, sizeof(rpc_error),
                     "Unable to authenticate secure RPC client (why=%d)", why);
            *reason = rpc_error;
        }
        else {
            if (ForEachHostInFamily(FamilyNetname, CheckNetName, fullname))
                return rpc_id;
            snprintf(rpc_error, sizeof(rpc_error),
                     "Principal \"%s\" is not authorized to connect", fullname);
            *reason = rpc_error;
        }
    }
    return (XID) ~0L;
}

_X_HIDDEN void
SecureRPCInit(void)
{
    if (rpc_id == ~0L)
        AddAuthorization(9, "SUN-DES-1", 0, (char *) 0);
}

_X_HIDDEN int
SecureRPCAdd(unsigned short data_length, const char *data, XID id)
{
    if (data_length)
        AddHost((void *) 0, FamilyNetname, data_length, data);
    rpc_id = id;
    return 1;
}

_X_HIDDEN int
SecureRPCReset(void)
{
    rpc_id = (XID) ~0L;
    return 1;
}

_X_HIDDEN int
SecureRPCFromID(XID id, unsigned short *data_lenp, char **datap)
{
    return 0;
}

_X_HIDDEN int
SecureRPCRemove(unsigned short data_length, const char *data)
{
    return 0;
}
#endif                          /* SECURE_RPC */
