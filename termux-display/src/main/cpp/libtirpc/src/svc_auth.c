
/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1986-1991 by Sun Microsystems Inc. 
 */

/*
 * svc_auth.c, Server-side rpc authenticator interface.
 *
 */
#include <pthread.h>
#include <reentrant.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpc/auth_des.h>
#include <stdlib.h>

/*
 * svcauthsw is the bdevsw of server side authentication.
 *
 * Server side authenticators are called from authenticate by
 * using the client auth struct flavor field to index into svcauthsw.
 * The server auth flavors must implement a routine that looks
 * like:
 *
 *	enum auth_stat
 *	flavorx_auth(rqst, msg)
 *		struct svc_req *rqst;
 *		struct rpc_msg *msg;
 *
 */

/* declarations to allow servers to specify new authentication flavors */
struct authsvc {
	int	flavor;
	enum	auth_stat (*handler)(struct svc_req *, struct rpc_msg *);
	struct	authsvc	  *next;
};
static struct authsvc *Auths = NULL;

extern SVCAUTH svc_auth_none;

/*
 * The call rpc message, msg has been obtained from the wire.  The msg contains
 * the raw form of credentials and verifiers.  authenticate returns AUTH_OK
 * if the msg is successfully authenticated.  If AUTH_OK then the routine also
 * does the following things:
 * set rqst->rq_xprt->verf to the appropriate response verifier;
 * sets rqst->rq_client_cred to the "cooked" form of the credentials.
 *
 * NB: rqst->rq_cxprt->verf must be pre-alloctaed;
 * its length is set appropriately.
 *
 * The caller still owns and is responsible for msg->u.cmb.cred and
 * msg->u.cmb.verf.  The authentication system retains ownership of
 * rqst->rq_client_cred, the cooked credentials.
 *
 * There is an assumption that any flavour less than AUTH_NULL is
 * invalid.
 */
enum auth_stat
_gss_authenticate(rqst, msg, no_dispatch)
	struct svc_req *rqst;
	struct rpc_msg *msg;
	bool_t *no_dispatch;
{
	int cred_flavor;
	struct authsvc *asp;
	enum auth_stat dummy;
	extern mutex_t authsvc_lock;

/* VARIABLES PROTECTED BY authsvc_lock: asp, Auths */

	rqst->rq_cred = msg->rm_call.cb_cred;
	SVC_XP_AUTH(rqst->rq_xprt).svc_ah_ops = svc_auth_none.svc_ah_ops;
	SVC_XP_AUTH(rqst->rq_xprt).svc_ah_private = NULL;
	rqst->rq_xprt->xp_verf.oa_flavor = _null_auth.oa_flavor;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	cred_flavor = rqst->rq_cred.oa_flavor;
	*no_dispatch = FALSE;
	switch (cred_flavor) {
	case AUTH_NONE:
		dummy = _svcauth_none(rqst, msg);
		return (dummy);
	case AUTH_SYS:
		dummy = _svcauth_unix(rqst, msg);
		return (dummy);
	case AUTH_SHORT:
		dummy = _svcauth_short(rqst, msg);
		return (dummy);
	case AUTH_DES:
#ifdef AUTHDES_SUPPORT
		dummy = _svcauth_des(rqst, msg);
		return (dummy);
#else
		return (AUTH_FAILED);
#endif
#ifdef HAVE_RPCSEC_GSS
	case RPCSEC_GSS:
		dummy = _svcauth_gss(rqst, msg, no_dispatch);
		return (dummy);
#endif
	default:
		break;
	}

	/* flavor doesn't match any of the builtin types, so try new ones */
	mutex_lock(&authsvc_lock);
	for (asp = Auths; asp; asp = asp->next) {
		if (asp->flavor == cred_flavor) {
			enum auth_stat as;

			as = (*asp->handler)(rqst, msg);
			mutex_unlock(&authsvc_lock);
			return (as);
		}
	}
	mutex_unlock(&authsvc_lock);

	return (AUTH_REJECTEDCRED);
}

enum auth_stat
_authenticate(struct svc_req *rqst, struct rpc_msg *msg)
{
	bool_t no_dispatch;
	return _gss_authenticate(rqst, msg, &no_dispatch);
}

/*
 *  Allow the rpc service to register new authentication types that it is
 *  prepared to handle.  When an authentication flavor is registered,
 *  the flavor is checked against already registered values.  If not
 *  registered, then a new Auths entry is added on the list.
 *
 *  There is no provision to delete a registration once registered.
 *
 *  This routine returns:
 *	 0 if registration successful
 *	 1 if flavor already registered
 *	-1 if can't register (errno set)
 */

int
svc_auth_reg(cred_flavor, handler)
	int cred_flavor;
	enum auth_stat (*handler)(struct svc_req *, struct rpc_msg *);
{
	struct authsvc *asp;
	extern mutex_t authsvc_lock;

	switch (cred_flavor) {
	    case AUTH_NULL:
	    case AUTH_SYS:
	    case AUTH_SHORT:
	    case AUTH_DES:
#ifndef AUTHDES_SUPPORT
		return(-1);
#endif
#ifdef HAVE_RPCSEC_GSS
	    case RPCSEC_GSS:
#endif
		/* already registered */
		return (1);

	    default:
		mutex_lock(&authsvc_lock);
		for (asp = Auths; asp; asp = asp->next) {
			if (asp->flavor == cred_flavor) {
				/* already registered */
				mutex_unlock(&authsvc_lock);
				return (1);
			}
		}

		/* this is a new one, so go ahead and register it */
		asp = mem_alloc(sizeof (*asp));
		if (asp == NULL) {
			mutex_unlock(&authsvc_lock);
			return (-1);
		}
		asp->flavor = cred_flavor;
		asp->handler = handler;
		asp->next = Auths;
		Auths = asp;
		mutex_unlock(&authsvc_lock);
		break;
	}
	return (0);
}
