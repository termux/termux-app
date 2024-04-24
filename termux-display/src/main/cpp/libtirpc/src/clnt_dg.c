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
 * Implements a connectionless client side RPC.
 */
#include <pthread.h>
#include <reentrant.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <poll.h>

#include <sys/time.h>

#include <sys/ioctl.h>
#include <rpc/clnt.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <err.h>
#include "rpc_com.h"
#include "clnt_fd_locks.h"

#ifdef IP_RECVERR
#include <asm/types.h>
#include <linux/errqueue.h>
#include <sys/uio.h>
#endif


#define MAX_DEFAULT_FDS                 20000

static struct clnt_ops *clnt_dg_ops(void);
static bool_t time_not_ok(struct timeval *);
static enum clnt_stat clnt_dg_call(CLIENT *, rpcproc_t, xdrproc_t, void *,
	    xdrproc_t, void *, struct timeval);
static void clnt_dg_geterr(CLIENT *, struct rpc_err *);
static bool_t clnt_dg_freeres(CLIENT *, xdrproc_t, void *);
static void clnt_dg_abort(CLIENT *);
static bool_t clnt_dg_control(CLIENT *, u_int, void *);
static void clnt_dg_destroy(CLIENT *);


/*
 *	This machinery implements per-fd locks for MT-safety.  It is not
 *	sufficient to do per-CLIENT handle locks for MT-safety because a
 *	user may create more than one CLIENT handle with the same fd behind
 *	it.
 *
 *	We keep track of a list of per-fd locks, protected by the clnt_fd_lock
 *	mutex. Each per-fd lock consists of a predicate indicating whether is
 *	active or not: fd_lock->active == TRUE => a call is active on some
 *	CLIENT handle created for that fd. Each fd predicate is guarded by a
 *	condition variable so that the global mutex can be unlocked while
 *	waiting for the predicate to change.
 *
 *	The current implementation holds locks across the entire RPC and reply,
 *	including retransmissions.  Yes, this is silly, and as soon as this
 *	code is proven to work, this should be the first thing fixed.  One step
 *	at a time.
 */
static fd_locks_t *dg_fd_locks;
extern mutex_t clnt_fd_lock;
#define	release_fd_lock(fd_lock, mask) {	\
	mutex_lock(&clnt_fd_lock);	\
	fd_lock->active = FALSE;	\
	mutex_unlock(&clnt_fd_lock);	\
	thr_sigsetmask(SIG_SETMASK, &(mask), NULL); \
	cond_signal(&fd_lock->cv);	\
}

static const char mem_err_clnt_dg[] = "clnt_dg_create: out of memory";

/* VARIABLES PROTECTED BY clnt_fd_lock: dg_fd_locks, dg_cv */

/*
 * Private data kept per client handle
 */
struct cu_data {
	int			cu_fd;		/* connections fd */
	fd_lock_t 		*cu_fd_lock;
	bool_t			cu_closeit;	/* opened by library */
	struct sockaddr_storage	cu_raddr;	/* remote address */
	int			cu_rlen;
	struct timeval		cu_wait;	/* retransmit interval */
	struct timeval		cu_total;	/* total time for the call */
	struct rpc_err		cu_error;
	XDR			cu_outxdrs;
	u_int			cu_xdrpos;
	u_int			cu_sendsz;	/* send size */
	char			*cu_outbuf;
	u_int			cu_recvsz;	/* recv size */
	int			cu_async;
	int			cu_connect;	/* Use connect(). */
	int			cu_connected;	/* Have done connect(). */
	char			cu_inbuf[1];
};

/*
 * Connection less client creation returns with client handle parameters.
 * Default options are set, which the user can change using clnt_control().
 * fd should be open and bound.
 * NB: The rpch->cl_auth is initialized to null authentication.
 * 	Caller may wish to set this something more useful.
 *
 * sendsz and recvsz are the maximum allowable packet sizes that can be
 * sent and received. Normally they are the same, but they can be
 * changed to improve the program efficiency and buffer allocation.
 * If they are 0, use the transport default.
 *
 * If svcaddr is NULL, returns NULL.
 */
CLIENT *
clnt_dg_create(fd, svcaddr, program, version, sendsz, recvsz)
	int fd;				/* open file descriptor */
	const struct netbuf *svcaddr;	/* servers address */
	rpcprog_t program;		/* program number */
	rpcvers_t version;		/* version number */
	u_int sendsz;			/* buffer recv size */
	u_int recvsz;			/* buffer send size */
{
	CLIENT *cl = NULL;		/* client handle */
	struct cu_data *cu = NULL;	/* private data */
	struct timeval now;
	struct rpc_msg call_msg;
	sigset_t mask;
	sigset_t newmask;
	struct __rpc_sockinfo si;
	int one = 1;
	fd_lock_t *fd_lock;

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&clnt_fd_lock);
	if (dg_fd_locks == (fd_locks_t *) NULL) {
		dg_fd_locks = fd_locks_init();
		if (dg_fd_locks == (fd_locks_t *) NULL) {
			goto err1;
		}
	}
	fd_lock = fd_lock_create(fd, dg_fd_locks);
	if (fd_lock == (fd_lock_t *) NULL)
		goto err1;

	mutex_unlock(&clnt_fd_lock);
	thr_sigsetmask(SIG_SETMASK, &(mask), NULL);

	if (svcaddr == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNADDR;
		return (NULL);
	}

	if (!__rpc_fd2sockinfo(fd, &si)) {
		rpc_createerr.cf_stat = RPC_TLIERROR;
		rpc_createerr.cf_error.re_errno = 0;
		return (NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	sendsz = __rpc_get_t_size(si.si_af, si.si_proto, (int)sendsz);
	recvsz = __rpc_get_t_size(si.si_af, si.si_proto, (int)recvsz);
	if ((sendsz == 0) || (recvsz == 0)) {
		rpc_createerr.cf_stat = RPC_TLIERROR; /* XXX */
		rpc_createerr.cf_error.re_errno = 0;
		return (NULL);
	}

	if ((cl = mem_alloc(sizeof (CLIENT))) == NULL)
		goto err1;
	/*
	 * Should be multiple of 4 for XDR.
	 */
	sendsz = ((sendsz + 3) / 4) * 4;
	recvsz = ((recvsz + 3) / 4) * 4;
	cu = mem_alloc(sizeof (*cu) + sendsz + recvsz);
	if (cu == NULL)
		goto err1;
	(void) memcpy(&cu->cu_raddr, svcaddr->buf, (size_t)svcaddr->len);
	cu->cu_rlen = svcaddr->len;
	cu->cu_outbuf = &cu->cu_inbuf[recvsz];
	/* Other values can also be set through clnt_control() */
	cu->cu_wait.tv_sec = 15;	/* heuristically chosen */
	cu->cu_wait.tv_usec = 0;
	cu->cu_total.tv_sec = -1;
	cu->cu_total.tv_usec = -1;
	cu->cu_sendsz = sendsz;
	cu->cu_recvsz = recvsz;
	cu->cu_async = FALSE;
	cu->cu_connect = FALSE;
	cu->cu_connected = FALSE;
	(void) gettimeofday(&now, NULL);
	call_msg.rm_xid = __RPC_GETXID(&now);
	call_msg.rm_call.cb_prog = program;
	call_msg.rm_call.cb_vers = version;
	xdrmem_create(&(cu->cu_outxdrs), cu->cu_outbuf, sendsz, XDR_ENCODE);
	if (! xdr_callhdr(&(cu->cu_outxdrs), &call_msg)) {
		rpc_createerr.cf_stat = RPC_CANTENCODEARGS;  /* XXX */
		rpc_createerr.cf_error.re_errno = 0;
		goto err2;
	}
	cu->cu_xdrpos = XDR_GETPOS(&(cu->cu_outxdrs));

	/* XXX fvdl - do we still want this? */
#if 0
	(void)bindresvport_sa(fd, (struct sockaddr *)svcaddr->buf);
#endif
#ifdef IP_RECVERR
	{
	int on = 1;
	setsockopt(fd, SOL_IP, IP_RECVERR, &on, sizeof(on));
	}
#endif
	ioctl(fd, FIONBIO, (char *)(void *)&one);
	/*
	 * By default, closeit is always FALSE. It is users responsibility
	 * to do a close on it, else the user may use clnt_control
	 * to let clnt_destroy do it for him/her.
	 */
	cu->cu_closeit = FALSE;
	cu->cu_fd = fd;
	cu->cu_fd_lock = fd_lock;
	cl->cl_ops = clnt_dg_ops();
	cl->cl_private = (caddr_t)(void *)cu;
	cl->cl_auth = authnone_create();
	cl->cl_tp = NULL;
	cl->cl_netid = NULL;
	
	return (cl);
err1:
	warnx(mem_err_clnt_dg);
	rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = errno;
err2:
	if (cl) {
		mem_free(cl, sizeof (CLIENT));
		if (cu)
			mem_free(cu, sizeof (*cu) + sendsz + recvsz);
	}
	return (NULL);
}

static enum clnt_stat
clnt_dg_call(cl, proc, xargs, argsp, xresults, resultsp, utimeout)
	CLIENT	*cl;			/* client handle */
	rpcproc_t	proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	void		*argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	void		*resultsp;	/* pointer to results */
	struct timeval	utimeout;	/* seconds to wait before giving up */
{
	struct cu_data *cu = (struct cu_data *)cl->cl_private;
	XDR *xdrs;
	size_t outlen = 0;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	bool_t ok;
	int nrefreshes = 2;		/* number of times to refresh cred */
	struct timeval timeout;
        struct pollfd fd;
	int total_time, nextsend_time, tv=0;
	struct sockaddr *sa;
	sigset_t mask;
	sigset_t newmask;
	socklen_t salen;
	ssize_t recvlen = 0;
	u_int32_t xid, inval, outval;

	outlen = 0;
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&clnt_fd_lock);
	while (cu->cu_fd_lock->active)
		cond_wait(&cu->cu_fd_lock->cv, &clnt_fd_lock);
	cu->cu_fd_lock->active = TRUE;
	mutex_unlock(&clnt_fd_lock);
	if (cu->cu_total.tv_usec == -1) {
		timeout = utimeout;	/* use supplied timeout */
	} else {
		timeout = cu->cu_total;	/* use default timeout */
	}
	total_time = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
	nextsend_time = cu->cu_wait.tv_sec * 1000 + cu->cu_wait.tv_usec / 1000;

	if (cu->cu_connect && !cu->cu_connected) {
		if (connect(cu->cu_fd, (struct sockaddr *)&cu->cu_raddr,
		    cu->cu_rlen) < 0) {
			cu->cu_error.re_errno = errno;
			cu->cu_error.re_status = RPC_CANTSEND;
			goto out;
		}
		cu->cu_connected = 1;
	}
	if (cu->cu_connected) {
		sa = NULL;
		salen = 0;
	} else {
		sa = (struct sockaddr *)&cu->cu_raddr;
		salen = cu->cu_rlen;
	}

	/* Clean up in case the last call ended in a longjmp(3) call. */
call_again:
	xdrs = &(cu->cu_outxdrs);
	if (cu->cu_async == TRUE && xargs == NULL)
		goto get_reply;
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, cu->cu_xdrpos);
	/*
	 * the transaction is the first thing in the out buffer
	 * XXX Yes, and it's in network byte order, so we should to
	 * be careful when we increment it, shouldn't we.
	 */
	xid = ntohl(*(u_int32_t *)(void *)(cu->cu_outbuf));
	xid++;
	*(u_int32_t *)(void *)(cu->cu_outbuf) = htonl(xid);

	if ((! XDR_PUTINT32(xdrs, (int32_t *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs)) ||
	    (! AUTH_WRAP(cl->cl_auth, xdrs, xargs, argsp))) {
		cu->cu_error.re_status = RPC_CANTENCODEARGS;
		goto out;
	}
	outlen = (size_t)XDR_GETPOS(xdrs);

	/*
	 * Hack to provide rpc-based message passing
	 */
	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		cu->cu_error.re_status = RPC_TIMEDOUT;
		goto out;
	}

send_again:
	if (total_time <= 0) {
		cu->cu_error.re_status = RPC_TIMEDOUT;
		goto out;
	}
	nextsend_time = cu->cu_wait.tv_sec * 1000 + cu->cu_wait.tv_usec / 1000;
	if (sendto(cu->cu_fd, cu->cu_outbuf, outlen, 0, sa, salen) != outlen) {
		cu->cu_error.re_errno = errno;
		cu->cu_error.re_status = RPC_CANTSEND;
		goto out;
	}

get_reply:

	/*
	 * sub-optimal code appears here because we have
	 * some clock time to spare while the packets are in flight.
	 * (We assume that this is actually only executed once.)
	 */
	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = NULL;
	reply_msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;

        fd.fd = cu->cu_fd;
        fd.events = POLLIN;
        fd.revents = 0;
	while (total_time > 0) {
		tv = total_time < nextsend_time ? total_time : nextsend_time;
                switch (poll(&fd, 1, tv)) {
                case 0:
		        total_time -= tv;
		        goto send_again;
                case -1:
                        if (errno == EINTR)
                                continue;
                        cu->cu_error.re_status = RPC_CANTRECV;
                        cu->cu_error.re_errno = errno;
                        goto out;
                }
                break;
        }
#ifdef IP_RECVERR
      if (fd.revents & POLLERR)
	{
	  struct msghdr msg;
	  struct cmsghdr *cmsg;
	  struct sock_extended_err *e;
	  struct sockaddr_in err_addr;
	  struct sockaddr_in *sin = (struct sockaddr_in *)&cu->cu_raddr;
	  struct iovec iov;
	  char *cbuf = (char *) mem_alloc((outlen + 256));
	  int ret;

	  if (cbuf == NULL) 
	  {
	  	cu->cu_error.re_errno = errno;
		return (cu->cu_error.re_status = RPC_CANTRECV);
	  }
	  iov.iov_base = cbuf + 256;
	  iov.iov_len = outlen;
	  msg.msg_name = (void *) &err_addr;
	  msg.msg_namelen = sizeof (err_addr);
	  msg.msg_iov = &iov;
	  msg.msg_iovlen = 1;
	  msg.msg_flags = 0;
	  msg.msg_control = cbuf;
	  msg.msg_controllen = 256;
	  ret = recvmsg (cu->cu_fd, &msg, MSG_ERRQUEUE);
	  if (ret >= 0
	      && memcmp (cbuf + 256, cu->cu_outbuf, ret) == 0
	      && (msg.msg_flags & MSG_ERRQUEUE)
	      && ((msg.msg_namelen == 0
		   && ret >= 12)
		  || (msg.msg_namelen == sizeof (err_addr)
		      && err_addr.sin_family == AF_INET
		      && memcmp (&err_addr.sin_addr, &sin->sin_addr,
				 sizeof (err_addr.sin_addr)) == 0
		      && err_addr.sin_port == sin->sin_port)))
	    for (cmsg = CMSG_FIRSTHDR (&msg); cmsg;
		 cmsg = CMSG_NXTHDR (&msg, cmsg))
	      if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR)
		{
		  mem_free(cbuf, (outlen + 256));
		  e = (struct sock_extended_err *) CMSG_DATA(cmsg);
		  cu->cu_error.re_errno = e->ee_errno;
		  release_fd_lock(cu->cu_fd_lock, mask);
		  return (cu->cu_error.re_status = RPC_CANTRECV);
		}
	  mem_free(cbuf, (outlen + 256));
	}
#endif

	/* We have some data now */
	do {
		recvlen = recvfrom(cu->cu_fd, cu->cu_inbuf,
		    cu->cu_recvsz, 0, NULL, NULL);
	} while (recvlen < 0 && errno == EINTR);
	if (recvlen < 0 && errno != EWOULDBLOCK) {
		cu->cu_error.re_errno = errno;
		cu->cu_error.re_status = RPC_CANTRECV;
		goto out;
	}

	if (recvlen < sizeof(u_int32_t)) {
		total_time -= tv;
		goto send_again;
	}

	if (cu->cu_async == FALSE) {
		memcpy(&inval, cu->cu_inbuf, sizeof(u_int32_t));
		memcpy(&outval, cu->cu_outbuf, sizeof(u_int32_t));
		if (inval != outval) {
			total_time -= tv;
			goto send_again;
		}
	}

	/*
	 * now decode and validate the response
	 */

	xdrmem_create(&reply_xdrs, cu->cu_inbuf, (u_int)recvlen, XDR_DECODE);
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	/* XDR_DESTROY(&reply_xdrs);	save a few cycles on noop destroy */
	if (ok) {
		if ((reply_msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
			(reply_msg.acpted_rply.ar_stat == SUCCESS))
			cu->cu_error.re_status = RPC_SUCCESS;
		else
			_seterr_reply(&reply_msg, &(cu->cu_error));

		if (cu->cu_error.re_status == RPC_SUCCESS) {
			if (! AUTH_VALIDATE(cl->cl_auth,
					    &reply_msg.acpted_rply.ar_verf)) {
				cu->cu_error.re_status = RPC_AUTHERROR;
				cu->cu_error.re_why = AUTH_INVALIDRESP;
			} else if (! AUTH_UNWRAP(cl->cl_auth, &reply_xdrs,
						 xresults, resultsp)) {
				if (cu->cu_error.re_status == RPC_SUCCESS)
				     cu->cu_error.re_status = RPC_CANTDECODERES;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				xdrs->x_op = XDR_FREE;
				(void) xdr_opaque_auth(xdrs,
					&(reply_msg.acpted_rply.ar_verf));
			}
		}		/* end successful completion */
		/*
		 * If unsuccesful AND error is an authentication error
		 * then refresh credentials and try again, else break
		 */
		else if (cu->cu_error.re_status == RPC_AUTHERROR)
			/* maybe our credentials need to be refreshed ... */
			if (nrefreshes > 0 &&
			    AUTH_REFRESH(cl->cl_auth, &reply_msg)) {
				nrefreshes--;
				goto call_again;
			}
		/* end of unsuccessful completion */
	}	/* end of valid reply message */
	else {
		cu->cu_error.re_status = RPC_CANTDECODERES;

	}
out:
	release_fd_lock(cu->cu_fd_lock, mask);
	return (cu->cu_error.re_status);
}

static void
clnt_dg_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	struct cu_data *cu = (struct cu_data *)cl->cl_private;

	*errp = cu->cu_error;
}

static bool_t
clnt_dg_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	void *res_ptr;
{
	struct cu_data *cu = (struct cu_data *)cl->cl_private;
	XDR *xdrs = &(cu->cu_outxdrs);
	bool_t dummy;
	sigset_t mask;
	sigset_t newmask;

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&clnt_fd_lock);
	while (cu->cu_fd_lock->active)
		cond_wait(&cu->cu_fd_lock->cv, &clnt_fd_lock);
	cu->cu_fd_lock->active = TRUE;
	xdrs->x_op = XDR_FREE;
	dummy = (*xdr_res)(xdrs, res_ptr);
	mutex_unlock(&clnt_fd_lock);
	thr_sigsetmask(SIG_SETMASK, &mask, NULL);
	cond_signal(&cu->cu_fd_lock->cv);
	return (dummy);
}

/*ARGSUSED*/
static void
clnt_dg_abort(h)
	CLIENT *h;
{
}

static bool_t
clnt_dg_control(cl, request, info)
	CLIENT *cl;
	u_int request;
	void *info;
{
	struct cu_data *cu = (struct cu_data *)cl->cl_private;
	struct netbuf *addr;
	sigset_t mask;
	sigset_t newmask;

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&clnt_fd_lock);
	while (cu->cu_fd_lock->active)
		cond_wait(&cu->cu_fd_lock->cv, &clnt_fd_lock);
	cu->cu_fd_lock->active = TRUE;
	mutex_unlock(&clnt_fd_lock);
	switch (request) {
	case CLSET_FD_CLOSE:
		cu->cu_closeit = TRUE;
		release_fd_lock(cu->cu_fd_lock, mask);
		return (TRUE);
	case CLSET_FD_NCLOSE:
		cu->cu_closeit = FALSE;
		release_fd_lock(cu->cu_fd_lock, mask);
		return (TRUE);
	}

	/* for other requests which use info */
	if (info == NULL) {
		release_fd_lock(cu->cu_fd_lock, mask);
		return (FALSE);
	}
	switch (request) {
	case CLSET_TIMEOUT:
		if (time_not_ok((struct timeval *)info)) {
			release_fd_lock(cu->cu_fd_lock, mask);
			return (FALSE);
		}
		cu->cu_total = *(struct timeval *)info;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *)info = cu->cu_total;
		break;
	case CLGET_SERVER_ADDR:		/* Give him the fd address */
		/* Now obsolete. Only for backward compatibility */
		(void) memcpy(info, &cu->cu_raddr, (size_t)cu->cu_rlen);
		break;
	case CLSET_RETRY_TIMEOUT:
		if (time_not_ok((struct timeval *)info)) {
			release_fd_lock(cu->cu_fd_lock, mask);
			return (FALSE);
		}
		cu->cu_wait = *(struct timeval *)info;
		break;
	case CLGET_RETRY_TIMEOUT:
		*(struct timeval *)info = cu->cu_wait;
		break;
	case CLGET_FD:
		*(int *)info = cu->cu_fd;
		break;
	case CLGET_SVC_ADDR:
		addr = (struct netbuf *)info;
		addr->buf = &cu->cu_raddr;
		addr->len = cu->cu_rlen;
		addr->maxlen = sizeof cu->cu_raddr;
		break;
	case CLSET_SVC_ADDR:		/* set to new address */
		addr = (struct netbuf *)info;
		if (addr->len < sizeof cu->cu_raddr) {
			release_fd_lock(cu->cu_fd_lock, mask);
			return (FALSE);
		}
		(void) memcpy(&cu->cu_raddr, addr->buf, addr->len);
		cu->cu_rlen = addr->len;
		break;
	case CLGET_XID:
		/*
		 * use the knowledge that xid is the
		 * first element in the call structure *.
		 * This will get the xid of the PREVIOUS call
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)cu->cu_outbuf);
		break;

	case CLSET_XID:
		/* This will set the xid of the NEXT call */
		*(u_int32_t *)(void *)cu->cu_outbuf =
		    htonl(*(u_int32_t *)info - 1);
		/* decrement by 1 as clnt_dg_call() increments once */
		break;

	case CLGET_VERS:
		/*
		 * This RELIES on the information that, in the call body,
		 * the version number field is the fifth field from the
		 * begining of the RPC header. MUST be changed if the
		 * call_struct is changed
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)(cu->cu_outbuf +
		    4 * BYTES_PER_XDR_UNIT));
		break;

	case CLSET_VERS:
		*(u_int32_t *)(void *)(cu->cu_outbuf + 4 * BYTES_PER_XDR_UNIT)
			= htonl(*(u_int32_t *)info);
		break;

	case CLGET_PROG:
		/*
		 * This RELIES on the information that, in the call body,
		 * the program number field is the fourth field from the
		 * begining of the RPC header. MUST be changed if the
		 * call_struct is changed
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)(cu->cu_outbuf +
		    3 * BYTES_PER_XDR_UNIT));
		break;

	case CLSET_PROG:
		*(u_int32_t *)(void *)(cu->cu_outbuf + 3 * BYTES_PER_XDR_UNIT)
			= htonl(*(u_int32_t *)info);
		break;
	case CLSET_ASYNC:
		cu->cu_async = *(int *)info;
		break;
	case CLSET_CONNECT:
		cu->cu_connect = *(int *)info;
		break;
	default:
		release_fd_lock(cu->cu_fd_lock, mask);
		return (FALSE);
	}
	release_fd_lock(cu->cu_fd_lock, mask);
	return (TRUE);
}

static void
clnt_dg_destroy(cl)
	CLIENT *cl;
{
	struct cu_data *cu = (struct cu_data *)cl->cl_private;
	int cu_fd = cu->cu_fd;
	fd_lock_t *cu_fd_lock = cu->cu_fd_lock;
	sigset_t mask;
	sigset_t newmask;

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&clnt_fd_lock);
	while (cu_fd_lock->active)
		cond_wait(&cu_fd_lock->cv, &clnt_fd_lock);
	if (cu->cu_closeit)
		(void)close(cu_fd);
	XDR_DESTROY(&(cu->cu_outxdrs));
	mem_free(cu, (sizeof (*cu) + cu->cu_sendsz + cu->cu_recvsz));
	if (cl->cl_netid && cl->cl_netid[0])
		mem_free(cl->cl_netid, strlen(cl->cl_netid) +1);
	if (cl->cl_tp && cl->cl_tp[0])
		mem_free(cl->cl_tp, strlen(cl->cl_tp) +1);
	mem_free(cl, sizeof (CLIENT));
	cond_signal(&cu_fd_lock->cv);
	fd_lock_destroy(cu_fd, cu_fd_lock, dg_fd_locks);
	mutex_unlock(&clnt_fd_lock);
	thr_sigsetmask(SIG_SETMASK, &mask, NULL);
}

static struct clnt_ops *
clnt_dg_ops()
{
	static struct clnt_ops ops;
	extern mutex_t	ops_lock;
	sigset_t mask;
	sigset_t newmask;

/* VARIABLES PROTECTED BY ops_lock: ops */

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
	mutex_lock(&ops_lock);
	if (ops.cl_call == NULL) {
		ops.cl_call = clnt_dg_call;
		ops.cl_abort = clnt_dg_abort;
		ops.cl_geterr = clnt_dg_geterr;
		ops.cl_freeres = clnt_dg_freeres;
		ops.cl_destroy = clnt_dg_destroy;
		ops.cl_control = clnt_dg_control;
	}
	mutex_unlock(&ops_lock);
	thr_sigsetmask(SIG_SETMASK, &mask, NULL);
	return (&ops);
}

/*
 * Make sure that the time is not garbage.  -1 value is allowed.
 */
static bool_t
time_not_ok(t)
	struct timeval *t;
{
	return (t->tv_sec < -1 || t->tv_sec > 100000000 ||
		t->tv_usec < -1 || t->tv_usec > 1000000);
}

