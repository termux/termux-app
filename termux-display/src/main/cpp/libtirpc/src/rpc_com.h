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
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * rpc_com.h, Common definitions for both the server and client side.
 * All for the topmost layer of rpc
 *
 * In Sun's tirpc distribution, this was installed as <rpc/rpc_com.h>,
 * but as it contains only non-exported interfaces, it was moved here.
 */

#ifndef _TIRPC_RPCCOM_H
#define	_TIRPC_RPCCOM_H

#include <rpc/rpc_com.h>

#ifdef __cplusplus
extern "C" {
#endif

struct netbuf *__rpc_set_netbuf(struct netbuf *, const void *, size_t);

struct netbuf *__rpcb_findaddr_timed(rpcprog_t, rpcvers_t,
    const struct netconfig *, const char *host, CLIENT **clpp,
    struct timeval *tp);

bool_t __rpc_control(int,void *);

bool_t __svc_clean_idle(fd_set *, int, bool_t);
bool_t __xdrrec_setnonblock(XDR *, int);
bool_t __xdrrec_getrec(XDR *, enum xprt_stat *, bool_t);
void __xprt_unregister_unlocked(SVCXPRT *);
void __xprt_set_raddr(SVCXPRT *, const struct sockaddr_storage *);


extern int __svc_maxrec;

#ifdef __cplusplus
}
#endif

#endif /* _TIRPC_RPCCOM_H */
