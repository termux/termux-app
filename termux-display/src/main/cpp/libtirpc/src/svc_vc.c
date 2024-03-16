
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
 * svc_vc.c, Server side for Connection Oriented based RPC. 
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */
#include <pthread.h>
#include <reentrant.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <poll.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>

#include "rpc_com.h"

#include <getpeereid.h>


extern rwlock_t svc_fd_lock;

static SVCXPRT *makefd_xprt(int, u_int, u_int);
static bool_t rendezvous_request(SVCXPRT *, struct rpc_msg *);
static enum xprt_stat rendezvous_stat(SVCXPRT *);
static void svc_vc_destroy(SVCXPRT *);
static void __svc_vc_dodestroy (SVCXPRT *);
static int read_vc(void *, void *, int);
static int write_vc(void *, void *, int);
static enum xprt_stat svc_vc_stat(SVCXPRT *);
static bool_t svc_vc_recv(SVCXPRT *, struct rpc_msg *);
static bool_t svc_vc_getargs(SVCXPRT *, xdrproc_t, void *);
static bool_t svc_vc_freeargs(SVCXPRT *, xdrproc_t, void *);
static bool_t svc_vc_reply(SVCXPRT *, struct rpc_msg *);
static void svc_vc_rendezvous_ops(SVCXPRT *);
static void svc_vc_ops(SVCXPRT *);
static bool_t svc_vc_control(SVCXPRT *xprt, const u_int rq, void *in);
static bool_t svc_vc_rendezvous_control (SVCXPRT *xprt, const u_int rq,
				   	     void *in);

struct cf_rendezvous { /* kept in xprt->xp_p1 for rendezvouser */
	u_int sendsize;
	u_int recvsize;
	int maxrec;
};

struct cf_conn {  /* kept in xprt->xp_p1 for actual connection */
	enum xprt_stat strm_stat;
	u_int32_t x_id;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
	u_int sendsize;
	u_int recvsize;
	int maxrec;
	bool_t nonblock;
	struct timeval last_recv_time;
};

/*
 * This is used to set xprt->xp_raddr in a way legacy
 * apps can deal with
 */
void
__xprt_set_raddr(SVCXPRT *xprt, const struct sockaddr_storage *ss)
{
	switch (ss->ss_family) {
	case AF_INET6:
		memcpy(&xprt->xp_raddr, ss, sizeof(struct sockaddr_in6));
		xprt->xp_addrlen = sizeof (struct sockaddr_in6);
		break;
	case AF_INET:
		memcpy(&xprt->xp_raddr, ss, sizeof(struct sockaddr_in));
		xprt->xp_addrlen = sizeof (struct sockaddr_in);
		break;
	default:
		xprt->xp_raddr.sin6_family = AF_UNSPEC;
		xprt->xp_addrlen = sizeof (struct sockaddr);
		break;
	}
}

/*
 * Usage:
 *	xprt = svc_vc_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * The filedescriptor passed in is expected to refer to a bound, but
 * not yet connected socket.
 *
 * Since streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
 */
SVCXPRT *
svc_vc_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *xprt = NULL;
	SVCXPRT_EXT *ext = NULL;
	struct cf_rendezvous *r = NULL;
	struct __rpc_sockinfo si;
	struct sockaddr_storage sslocal;
	socklen_t slen;

	r = mem_alloc(sizeof(*r));
	if (r == NULL) {
		warnx("svc_vc_create: out of memory");
		goto cleanup_svc_vc_create;
	}
	if (!__rpc_fd2sockinfo(fd, &si)) {
		warnx("svc_vc_create: __rpc_fd2sockinfo failed");
		goto cleanup_svc_vc_create;
	}
	r->sendsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)sendsize);
	r->recvsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)recvsize);
	r->maxrec = __svc_maxrec;
	xprt = mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		warnx("svc_vc_create: out of memory");
		goto cleanup_svc_vc_create;
	}
	ext = mem_alloc(sizeof (*ext));
	if (ext == NULL) {
		warnx("svc_vc_create: out of memory");
		goto cleanup_svc_vc_create;
	}
	memset(ext, 0, sizeof (*ext));
	xprt->xp_tp = NULL;
	xprt->xp_p1 = r;
	xprt->xp_p2 = NULL;
	xprt->xp_p3 = ext;
	xprt->xp_verf = _null_auth;
	svc_vc_rendezvous_ops(xprt);
	xprt->xp_port = (u_short)-1;	/* It is the rendezvouser */
	xprt->xp_fd = fd;

	slen = sizeof (struct sockaddr_storage);
	if (getsockname(fd, (struct sockaddr *)(void *)&sslocal, &slen) < 0) {
		warnx("svc_vc_create: could not retrieve local addr");
		goto cleanup_svc_vc_create;
	}

	if (!__rpc_set_netbuf(&xprt->xp_ltaddr, &sslocal, sizeof(sslocal))) {
		warnx("svc_vc_create: no mem for local addr");
		goto cleanup_svc_vc_create;
	}
	xprt_register(xprt);
	return (xprt);
cleanup_svc_vc_create:
	if (r != NULL)
		mem_free(r, sizeof(*r));
	if (xprt != NULL)
		mem_free(xprt, sizeof(*xprt));
	if (ext != NULL)
		mem_free(ext, sizeof(*ext));
	return (NULL);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */
SVCXPRT *
svc_fd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	struct sockaddr_storage ss;
	socklen_t slen;
	SVCXPRT *ret;

	assert(fd != -1);

	ret = makefd_xprt(fd, sendsize, recvsize);
	if (ret == NULL)
		return NULL;

	slen = sizeof (struct sockaddr_storage);
	if (getsockname(fd, (struct sockaddr *)(void *)&ss, &slen) < 0) {
		warnx("svc_fd_create: could not retrieve local addr");
		goto freedata;
	}
	if (!__rpc_set_netbuf(&ret->xp_ltaddr, &ss, sizeof(ss))) {
		warnx("svc_fd_create: no mem for local addr");
		goto freedata;
	}

	slen = sizeof (struct sockaddr_storage);
	if (getpeername(fd, (struct sockaddr *)(void *)&ss, &slen) < 0) {
		warnx("svc_fd_create: could not retrieve remote addr");
		goto freedata;
	}
	if (!__rpc_set_netbuf(&ret->xp_rtaddr, &ss, sizeof(ss))) {
		warnx("svc_fd_create: no mem for remote addr");
		goto freedata;
	}

	/* Set xp_raddr for compatibility */
	__xprt_set_raddr(ret, &ss);

	return ret;

freedata:
	if (ret->xp_ltaddr.buf != NULL) {
		mem_free(ret->xp_ltaddr.buf, rep->xp_ltaddr.maxlen);
		ret->xp_ltaddr.buf = NULL;
	}
	return NULL;
}

static SVCXPRT *
makefd_xprt(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *xprt;
	SVCXPRT_EXT *ext;
	struct cf_conn *cd;
	const char *netid;
	struct __rpc_sockinfo si;

	assert(fd != -1);

	xprt = mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		warnx("svc_vc: makefd_xprt: out of memory");
		goto done;
	}
	memset(xprt, 0, sizeof *xprt);
	ext = mem_alloc(sizeof (*ext));
	if (ext == NULL) {
		warnx("svc_vc: makefd_xprt: out of memory");
		mem_free(xprt, sizeof(SVCXPRT));
		xprt = NULL;
		goto done;
	}
	memset(ext, 0, sizeof (*ext));
	cd = mem_alloc(sizeof(struct cf_conn));
	if (cd == NULL) {
		warnx("svc_vc: makefd_xprt: out of memory");
		mem_free(ext, sizeof(*ext));
		mem_free(xprt, sizeof(SVCXPRT));
		xprt = NULL;
		goto done;
	}
	cd->strm_stat = XPRT_IDLE;
	xdrrec_create(&(cd->xdrs), sendsize, recvsize,
	    xprt, read_vc, write_vc);
	xprt->xp_p1 = cd;
	xprt->xp_p3 = ext;
	xprt->xp_verf.oa_base = cd->verf_body;
	svc_vc_ops(xprt);  /* truely deals with calls */
	xprt->xp_port = 0;  /* this is a connection, not a rendezvouser */
	xprt->xp_fd = fd;
        if (__rpc_fd2sockinfo(fd, &si) && __rpc_sockinfo2netid(&si, &netid))
		xprt->xp_netid = strdup(netid);

	xprt_register(xprt);
done:
	return (xprt);
}

/*ARGSUSED*/
static bool_t
rendezvous_request(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	int sock, flags;
	struct cf_rendezvous *r;
	struct cf_conn *cd;
	struct sockaddr_storage addr;
	socklen_t len;
	struct __rpc_sockinfo si;
	SVCXPRT *newxprt;

	assert(xprt != NULL);
	assert(msg != NULL);

	r = (struct cf_rendezvous *)xprt->xp_p1;
again:
	len = sizeof addr;
	sock = accept(xprt->xp_fd, (struct sockaddr *)(void *)&addr, &len);
	if (sock < 0) {
		if (errno == EINTR)
			goto again;
		return (FALSE);
	}
	/*
	 * make a new transporter (re-uses xprt)
	 */

	newxprt = makefd_xprt(sock, r->sendsize, r->recvsize);
	if (!newxprt)
		return (FALSE);

	if (!__rpc_set_netbuf(&newxprt->xp_rtaddr, &addr, len))
		return (FALSE);

	__xprt_set_raddr(newxprt, &addr);

	if (__rpc_fd2sockinfo(sock, &si) && si.si_proto == IPPROTO_TCP) {
		len = 1;
		/* XXX fvdl - is this useful? */
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &len, sizeof (len));
	}

	cd = (struct cf_conn *)newxprt->xp_p1;

	cd->recvsize = r->recvsize;
	cd->sendsize = r->sendsize;
	cd->maxrec = r->maxrec;

	if (cd->maxrec != 0) {
		flags = fcntl(sock, F_GETFL, 0);
		if (flags  == -1)
			return (FALSE);
		if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
			return (FALSE);
		if (cd->recvsize > cd->maxrec)
			cd->recvsize = cd->maxrec;
		cd->nonblock = TRUE;
		__xdrrec_setnonblock(&cd->xdrs, cd->maxrec);
	} else
		cd->nonblock = FALSE;

	gettimeofday(&cd->last_recv_time, NULL);

	return (FALSE); /* there is never an rpc msg to be processed */
}

/*ARGSUSED*/
static enum xprt_stat
rendezvous_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE);
}

static void
svc_vc_destroy(xprt)
	SVCXPRT *xprt;
{
	assert(xprt != NULL);
	
	xprt_unregister(xprt);
	__svc_vc_dodestroy(xprt);
}

static bool_t
__svc_rendezvous_socket(xprt)
	SVCXPRT *xprt;
{
	return (xprt->xp_ops->xp_recv == rendezvous_request);
}
static void
__svc_vc_dodestroy(xprt)
	SVCXPRT *xprt;
{
	SVCXPRT_EXT *ext = SVCEXT(xprt);
	struct cf_conn *cd;
	struct cf_rendezvous *r;

	cd = (struct cf_conn *)xprt->xp_p1;

	if (xprt->xp_fd != RPC_ANYFD)
		(void)close(xprt->xp_fd);
	if (__svc_rendezvous_socket(xprt)) {
		/* a rendezvouser socket */
		r = (struct cf_rendezvous *)xprt->xp_p1;
		mem_free(r, sizeof (struct cf_rendezvous));
		xprt->xp_port = 0;
	} else {
		/* an actual connection socket */
		XDR_DESTROY(&(cd->xdrs));
		mem_free(cd, sizeof(struct cf_conn));
	}
	if (ext)
		mem_free(ext, sizeof (*ext));
	if (xprt->xp_rtaddr.buf)
		mem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.maxlen);
	if (xprt->xp_ltaddr.buf)
		mem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);
	if (xprt->xp_tp)
		free(xprt->xp_tp);
	if (xprt->xp_netid)
		free(xprt->xp_netid);
	mem_free(xprt, sizeof(SVCXPRT));
}

/*ARGSUSED*/
static bool_t
svc_vc_control(xprt, rq, in)
	SVCXPRT *xprt;
	const u_int rq;
	void *in;
{
	return (FALSE);
}

static bool_t
svc_vc_rendezvous_control(xprt, rq, in)
	SVCXPRT *xprt;
	const u_int rq;
	void *in;
{
	struct cf_rendezvous *cfp;

	cfp = (struct cf_rendezvous *)xprt->xp_p1;
	if (cfp == NULL)
		return (FALSE);
	switch (rq) {
		case SVCGET_CONNMAXREC:
			*(int *)in = cfp->maxrec;
			break;
		case SVCSET_CONNMAXREC:
			cfp->maxrec = *(int *)in;
			break;
		default:
			return (FALSE);
	}
	return (TRUE);
}

/*
 * reads data from the tcp or uip connection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 * All read operations timeout after 35 seconds.  A timeout is
 * fatal for the connection.
 */
static int
read_vc(xprtp, buf, len)
	void *xprtp;
	void *buf;
	int len;
{
	SVCXPRT *xprt;
	int sock;
	int milliseconds = 35 * 1000;
	struct pollfd pollfd;
	struct cf_conn *cfp;

	xprt = (SVCXPRT *)xprtp;
	assert(xprt != NULL);

	sock = xprt->xp_fd;

	cfp = (struct cf_conn *)xprt->xp_p1;

	if (cfp->nonblock) {
		/* Since len == 0 is returned on zero length
		 * read or EOF errno needs to be reset before
		 * the read
		 */
		errno = 0;
		len = read(sock, buf, (size_t)len);
		if (len < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				len = 0;
			else
				goto fatal_err;
		}
		if (len != 0)
			gettimeofday(&cfp->last_recv_time, NULL);
		return len;
	}

	do {
		pollfd.fd = sock;
		pollfd.events = POLLIN;
		pollfd.revents = 0;
		switch (poll(&pollfd, 1, milliseconds)) {
		case -1:
			if (errno == EINTR)
				continue;
			/*FALLTHROUGH*/
		case 0:
			goto fatal_err;

		default:
			break;
		}
	} while ((pollfd.revents & POLLIN) == 0);

	if ((len = read(sock, buf, (size_t)len)) > 0) {
		gettimeofday(&cfp->last_recv_time, NULL);
		return (len);
	}

fatal_err:
	((struct cf_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
static int
write_vc(xprtp, buf, len)
	void *xprtp;
	void *buf;
	int len;
{
	SVCXPRT *xprt;
	int i, cnt;
	struct cf_conn *cd;
	struct timeval tv0, tv1;

	xprt = (SVCXPRT *)xprtp;
	assert(xprt != NULL);

	cd = (struct cf_conn *)xprt->xp_p1;

	if (cd->nonblock)
		gettimeofday(&tv0, NULL);
	
	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		i = write(xprt->xp_fd, buf, (size_t)cnt);
		if (i  < 0) {
			if (errno != EAGAIN || !cd->nonblock) {
				cd->strm_stat = XPRT_DIED;
				return (-1);
			}
			/*
			 * For non-blocking connections, do not
			 * take more than 2 seconds writing the
			 * data out.
			 *
			 * XXX 2 is an arbitrary amount.
			 */
			gettimeofday(&tv1, NULL);
			if (tv1.tv_sec - tv0.tv_sec >= 2) {
				cd->strm_stat = XPRT_DIED;
				return (-1);
			}
			i = 0; /* Don't change buf and cnt */
		}
	}

	return (len);
}

static enum xprt_stat
svc_vc_stat(xprt)
	SVCXPRT *xprt;
{
	struct cf_conn *cd;

	assert(xprt != NULL);

	cd = (struct cf_conn *)(xprt->xp_p1);

	if (cd->strm_stat == XPRT_DIED)
		return (XPRT_DIED);
	if (! xdrrec_eof(&(cd->xdrs)))
		return (XPRT_MOREREQS);
	return (XPRT_IDLE);
}

static bool_t
svc_vc_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct cf_conn *cd;
	XDR *xdrs;

	assert(xprt != NULL);
	assert(msg != NULL);

	cd = (struct cf_conn *)(xprt->xp_p1);
	xdrs = &(cd->xdrs);

	if (cd->nonblock) {
		if (!__xdrrec_getrec(xdrs, &cd->strm_stat, TRUE))
			return FALSE;
	}

	xdrs->x_op = XDR_DECODE;
	/*
	 * No need skip records with nonblocking connections
	 */
	if (cd->nonblock == FALSE)
		(void)xdrrec_skiprecord(xdrs);
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		return (TRUE);
	}
	cd->strm_stat = XPRT_DIED;
	return (FALSE);
}

static bool_t
svc_vc_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	void *args_ptr;
{

	assert(xprt != NULL);
	/* args_ptr may be NULL */

	if (!SVCAUTH_UNWRAP(&SVC_XP_AUTH(xprt),
			    &(((struct cf_conn *)(xprt->xp_p1))->xdrs),
			    xdr_args, args_ptr)) {
		return FALSE;  
	}
	return TRUE;
}

static bool_t
svc_vc_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	void *args_ptr;
{
	XDR *xdrs;

	assert(xprt != NULL);
	/* args_ptr may be NULL */

	xdrs = &(((struct cf_conn *)(xprt->xp_p1))->xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static bool_t
svc_vc_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct cf_conn *cd;
	XDR *xdrs;
	bool_t rstat;

	xdrproc_t xdr_results;
	caddr_t xdr_location;
	bool_t has_args;

	assert(xprt != NULL);
	assert(msg != NULL);

	cd = (struct cf_conn *)(xprt->xp_p1);
	xdrs = &(cd->xdrs);

	if (msg->rm_reply.rp_stat == MSG_ACCEPTED &&
	    msg->rm_reply.rp_acpt.ar_stat == SUCCESS) {
		has_args = TRUE;
		xdr_results = msg->acpted_rply.ar_results.proc;
		xdr_location = msg->acpted_rply.ar_results.where;

		msg->acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
		msg->acpted_rply.ar_results.where = NULL;
	} else
		has_args = FALSE;

	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	rstat = FALSE;
	if (xdr_replymsg(xdrs, msg) &&
	    (!has_args ||
	     SVCAUTH_WRAP(&SVC_XP_AUTH(xprt),
			  xdrs, xdr_results, xdr_location))) {
		rstat = TRUE;
	}
	(void)xdrrec_endofrecord(xdrs, TRUE);
	return (rstat);
}

static void
svc_vc_ops(xprt)
	SVCXPRT *xprt;
{
	static struct xp_ops ops;
	static struct xp_ops2 ops2;
	extern mutex_t ops_lock;

/* VARIABLES PROTECTED BY ops_lock: ops, ops2 */

	mutex_lock(&ops_lock);
	if (ops.xp_recv == NULL) {
		ops.xp_recv = svc_vc_recv;
		ops.xp_stat = svc_vc_stat;
		ops.xp_getargs = svc_vc_getargs;
		ops.xp_reply = svc_vc_reply;
		ops.xp_freeargs = svc_vc_freeargs;
		ops.xp_destroy = svc_vc_destroy;
		ops2.xp_control = svc_vc_control;
	}
	xprt->xp_ops = &ops;
	xprt->xp_ops2 = &ops2;
	mutex_unlock(&ops_lock);
}

static void
svc_vc_rendezvous_ops(xprt)
	SVCXPRT *xprt;
{
	static struct xp_ops ops;
	static struct xp_ops2 ops2;
	extern mutex_t ops_lock;

	mutex_lock(&ops_lock);
	if (ops.xp_recv == NULL) {
		ops.xp_recv = rendezvous_request;
		ops.xp_stat = rendezvous_stat;
		ops.xp_getargs =
		    (bool_t (*)(SVCXPRT *, xdrproc_t, void *))abort;
		ops.xp_reply =
		    (bool_t (*)(SVCXPRT *, struct rpc_msg *))abort;
		ops.xp_freeargs =
		    (bool_t (*)(SVCXPRT *, xdrproc_t, void *))abort,
		ops.xp_destroy = svc_vc_destroy;
		ops2.xp_control = svc_vc_rendezvous_control;
	}
	xprt->xp_ops = &ops;
	xprt->xp_ops2 = &ops2;
	mutex_unlock(&ops_lock);
}

/*
 * Get the effective UID of the sending process. Used by rpcbind, keyserv
 * and rpc.yppasswdd on AF_LOCAL.
 */
int
__rpc_get_local_uid(SVCXPRT *transp, uid_t *uid) {
	int sock, ret;
	gid_t egid;
	uid_t euid;
	struct sockaddr *sa;

	sock = transp->xp_fd;
	sa = (struct sockaddr *)transp->xp_rtaddr.buf;
	if (sa->sa_family == AF_LOCAL) {
		ret = getpeereid(sock, &euid, &egid);
		if (ret == 0)
			*uid = euid;
		return (ret);
	} else
		return (-1);
}

/*
 * Destroy xprts that have not have had any activity in 'timeout' seconds.
 * If 'cleanblock' is true, blocking connections (the default) are also
 * cleaned. If timeout is 0, the least active connection is picked.
 *
 * Though this is not a publicly documented interface, some versions of
 * rpcbind are known to call this function.  Do not alter or remove this
 * API without changing the library's sonum.
 */
/* Since this is an exported interface used by rpcbind, we cannot
   remove it. But since poll() can handle much more and much higher
   file descriptors, this code doesn't really work anymore, too.
   So for now, keep it as dummy function and do nothing to not break
   existing binaries. If we have ported rpcbind to the poll() interface
   and find out, that we really need this cleanup stuff (but nobody
   besides FreeBSD has this), we need to re-implement it using poll().
   But this means a new function name with different parameters. For
   ABI/API compatibility, we cannot reuse this one. */
bool_t
__svc_clean_idle(fd_set *fds, int timeout, bool_t cleanblock)
{
	return FALSE;
}
