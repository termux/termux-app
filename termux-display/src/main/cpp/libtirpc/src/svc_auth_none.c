/*
  svc_auth_none.c

  Copyright (c) 2000 The Regents of the University of Michigan.
  All rights reserved.

  Copyright (c) 2000 Dug Song <dugsong@UMICH.EDU>.
  All rights reserved, all wrongs reversed.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the University nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  $Id: svc_auth_none.c,v 1.1 2004/10/22 17:24:30 bfields Exp $
 */

#include <rpc/rpc.h>

static bool_t	svcauth_none_destroy();
static bool_t   svcauth_none_wrap();

struct svc_auth_ops svc_auth_none_ops = {
	svcauth_none_wrap,
	svcauth_none_wrap,
	svcauth_none_destroy
};

SVCAUTH svc_auth_none = {
	&svc_auth_none_ops,
	NULL,
};

static bool_t
svcauth_none_destroy(SVCAUTH *auth)
{
	return (TRUE);
}

static bool_t
svcauth_none_wrap(SVCAUTH *auth, XDR *xdrs, xdrproc_t xdr_func,
		  caddr_t xdr_ptr)
{
	return ((*xdr_func)(xdrs, xdr_ptr));
}

enum auth_stat
_svcauth_none(struct svc_req *rqst, struct rpc_msg *msg)
{
	return (AUTH_OK);
}
