
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
 * svc_raw.c,   This a toy for simple testing and timing.
 * Interface to create an rpc client and server in the same UNIX process.
 * This lets us similate rpc and get rpc (round trip) overhead, without
 * any interference from the kernel.
 *
 */
#include <pthread.h>
#include <reentrant.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <rpc/raw.h>
#include <stdlib.h>

#ifndef UDPMSGSIZE
#define	UDPMSGSIZE 8800
#endif

/*
 * This is the "network" that we will be moving data over
 */
static struct svc_raw_private {
	char	*raw_buf;	/* should be shared with the cl handle */
	SVCXPRT	server;
	XDR	xdr_stream;
	char	verf_body[MAX_AUTH_BYTES];
} *svc_raw_private;

extern mutex_t	svcraw_lock;

static enum xprt_stat svc_raw_stat(SVCXPRT *);
static bool_t svc_raw_recv(SVCXPRT *, struct rpc_msg *);
static bool_t svc_raw_reply(SVCXPRT *, struct rpc_msg *);
static bool_t svc_raw_getargs(SVCXPRT *, xdrproc_t, void *);
static bool_t svc_raw_freeargs(SVCXPRT *, xdrproc_t, void *);
static void svc_raw_destroy(SVCXPRT *);
static void svc_raw_ops(SVCXPRT *);
static bool_t svc_raw_control(SVCXPRT *, const u_int, void *);

char *__rpc_rawcombuf = NULL;

SVCXPRT *
svc_raw_create()
{
	struct svc_raw_private *srp;
	SVCXPRT_EXT *ext = NULL;
/* VARIABLES PROTECTED BY svcraw_lock: svc_raw_private, srp */

	mutex_lock(&svcraw_lock);
	srp = svc_raw_private;
	if (srp == NULL) {
		srp = (struct svc_raw_private *)calloc(1, sizeof (*srp));
		if (srp == NULL) {
			mutex_unlock(&svcraw_lock);
			return (NULL);
		}
		ext = mem_alloc(sizeof (*ext));
		if (ext == NULL) {
			free(srp);
			mutex_unlock(&svcraw_lock);
			return (NULL);
		}
		memset(ext, 0, sizeof (*ext));
		srp->server.xp_p3 = ext;
		if (__rpc_rawcombuf == NULL)
			__rpc_rawcombuf = calloc(UDPMSGSIZE, sizeof (char));
		srp->raw_buf = __rpc_rawcombuf; /* Share it with the client */
		svc_raw_private = srp;
	}
	srp->server.xp_fd = FD_SETSIZE;
	srp->server.xp_port = 0;
	svc_flags(&srp->server) = 0;
	svc_raw_ops(&srp->server);
	srp->server.xp_verf.oa_base = srp->verf_body;
	xdrmem_create(&srp->xdr_stream, srp->raw_buf, UDPMSGSIZE, XDR_DECODE);
	xprt_register(&srp->server);
	mutex_unlock(&svcraw_lock);
	return (&srp->server);
}

/*ARGSUSED*/
static enum xprt_stat
svc_raw_stat(xprt)
SVCXPRT *xprt; /* args needed to satisfy ANSI-C typechecking */
{
	return (XPRT_IDLE);
}

/*ARGSUSED*/
static bool_t
svc_raw_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct svc_raw_private *srp;
	XDR *xdrs;

	mutex_lock(&svcraw_lock);
	srp = svc_raw_private;
	if (srp == NULL) {
		mutex_unlock(&svcraw_lock);
		return (FALSE);
	}
	mutex_unlock(&svcraw_lock);

	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_DECODE;
	(void) XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg)) {
		return (FALSE);
	}
	return (TRUE);
}

/*ARGSUSED*/
static bool_t
svc_raw_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct svc_raw_private *srp;
	XDR *xdrs;

	mutex_lock(&svcraw_lock);
	srp = svc_raw_private;
	if (srp == NULL) {
		mutex_unlock(&svcraw_lock);
		return (FALSE);
	}
	mutex_unlock(&svcraw_lock);

	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_ENCODE;
	(void) XDR_SETPOS(xdrs, 0);
	if (! xdr_replymsg(xdrs, msg)) {
		return (FALSE);
	}
	(void) XDR_GETPOS(xdrs);  /* called just for overhead */
	return (TRUE);
}

/*ARGSUSED*/
static bool_t
svc_raw_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	void *args_ptr;
{
	struct svc_raw_private *srp;

	mutex_lock(&svcraw_lock);
	srp = svc_raw_private;
	if (srp == NULL) {
		mutex_unlock(&svcraw_lock);
		return (FALSE);
	}
	mutex_unlock(&svcraw_lock);
	return (*xdr_args)(&srp->xdr_stream, args_ptr);
}

/*ARGSUSED*/
static bool_t
svc_raw_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	void *args_ptr;
{
	struct svc_raw_private *srp;
	XDR *xdrs;

	mutex_lock(&svcraw_lock);
	srp = svc_raw_private;
	if (srp == NULL) {
		mutex_unlock(&svcraw_lock);
		return (FALSE);
	}
	mutex_unlock(&svcraw_lock);

	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_FREE;
	return (*xdr_args)(xdrs, args_ptr);
}

/*ARGSUSED*/
static void
svc_raw_destroy(xprt)
SVCXPRT *xprt;
{
}

/*ARGSUSED*/
static bool_t
svc_raw_control(xprt, rq, in)
	SVCXPRT *xprt;
	const u_int	rq;
	void		*in;
{
	return (FALSE);
}

static void
svc_raw_ops(xprt)
	SVCXPRT *xprt;
{
	static struct xp_ops ops;
	static struct xp_ops2 ops2;
	extern mutex_t ops_lock;

/* VARIABLES PROTECTED BY ops_lock: ops */

	mutex_lock(&ops_lock);
	if (ops.xp_recv == NULL) {
		ops.xp_recv = svc_raw_recv;
		ops.xp_stat = svc_raw_stat;
		ops.xp_getargs = svc_raw_getargs;
		ops.xp_reply = svc_raw_reply;
		ops.xp_freeargs = svc_raw_freeargs;
		ops.xp_destroy = svc_raw_destroy;
		ops2.xp_control = svc_raw_control;
	}
	xprt->xp_ops = &ops;
	xprt->xp_ops2 = &ops2;
	mutex_unlock(&ops_lock);
}
