
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
 * rpcb_prot.c
 * XDR routines for the rpcbinder version 3.
 *
 * Copyright (C) 1984, 1988, Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/rpcb_prot.h>
#include "rpc_com.h"

bool_t
xdr_rpcb(xdrs, objp)
	XDR *xdrs;
	RPCB *objp;
{
	if (!xdr_u_int32_t(xdrs, &objp->r_prog)) {
		return (FALSE);
	}
	if (!xdr_u_int32_t(xdrs, &objp->r_vers)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_netid, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_addr, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_owner, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * rpcblist_ptr implements a linked list.  The RPCL definition from
 * rpcb_prot.x is:
 *
 * struct rpcblist {
 * 	rpcb		rpcb_map;
 *	struct rpcblist *rpcb_next;
 * };
 * typedef rpcblist *rpcblist_ptr;
 *
 * Recall that "pointers" in XDR are encoded as a boolean, indicating whether
 * there's any data behind the pointer, followed by the data (if any exists).
 * The boolean can be interpreted as ``more data follows me''; if FALSE then
 * nothing follows the boolean; if TRUE then the boolean is followed by an
 * actual struct rpcb, and another rpcblist_ptr (declared in RPCL as "struct
 * rpcblist *").
 *
 * This could be implemented via the xdr_pointer type, though this would
 * result in one recursive call per element in the list.  Rather than do that
 * we can ``unwind'' the recursion into a while loop and use xdr_reference to
 * serialize the rpcb elements.
 */

bool_t
xdr_rpcblist_ptr(xdrs, rp)
	XDR *xdrs;
	rpcblist_ptr *rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	rpcblist_ptr next;
	rpcblist_ptr next_copy;

	next = NULL;
	for (;;) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements)) {
			return (FALSE);
		}
		if (! more_elements) {
			return (TRUE);  /* we are done */
		}
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the next object
		 * before we free the current object ...
		 */
		if (freeing)
			next = (*rp)->rpcb_next;
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof (rpcblist), (xdrproc_t)xdr_rpcb)) {
			return (FALSE);
		}
		if (freeing) {
			next_copy = next;
			rp = &next_copy;
			/*
			 * Note that in the subsequent iteration, next_copy
			 * gets nulled out by the xdr_reference
			 * but next itself survives.
			 */
		} else {
			rp = &((*rp)->rpcb_next);
		}
	}
	/*NOTREACHED*/
}

/*
 * xdr_rpcblist() is specified to take a RPCBLIST **, but is identical in
 * functionality to xdr_rpcblist_ptr().
 */
bool_t
xdr_rpcblist(xdrs, rp)
	XDR *xdrs;
	RPCBLIST **rp;
{
	bool_t	dummy;

	dummy = xdr_rpcblist_ptr(xdrs, (rpcblist_ptr *)rp);
	return (dummy);
}


bool_t
xdr_rpcb_entry(xdrs, objp)
	XDR *xdrs;
	rpcb_entry *objp;
{
	if (!xdr_string(xdrs, &objp->r_maddr, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_netid, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_u_int32_t(xdrs, &objp->r_nc_semantics)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_protofmly, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_proto, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_rpcb_entry_list_ptr(xdrs, rp)
	XDR *xdrs;
	rpcb_entry_list_ptr *rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	rpcb_entry_list_ptr next;
	rpcb_entry_list_ptr next_copy;

	next = NULL;
	for (;;) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements)) {
			return (FALSE);
		}
		if (! more_elements) {
			return (TRUE);  /* we are done */
		}
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the next object
		 * before we free the current object ...
		 */
		if (freeing)
			next = (*rp)->rpcb_entry_next;
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof (rpcb_entry_list),
				    (xdrproc_t)xdr_rpcb_entry)) {
			return (FALSE);
		}
		if (freeing) {
			next_copy = next;
			rp = &next_copy;
			/*
			 * Note that in the subsequent iteration, next_copy
			 * gets nulled out by the xdr_reference
			 * but next itself survives.
			 */
		} else {
			rp = &((*rp)->rpcb_entry_next);
		}
	}
	/*NOTREACHED*/
}

/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rpcb_rmtcallargs(xdrs, p)
	XDR *xdrs;
	struct rpcb_rmtcallargs *p;
{
	struct r_rpcb_rmtcallargs *objp =
	    (struct r_rpcb_rmtcallargs *)(void *)p;
	u_int lenposition, argposition, position;
	int32_t *buf;

	buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT);
	if (buf == NULL) {
		if (!xdr_u_int32_t(xdrs, &objp->prog)) {
			return (FALSE);
		}
		if (!xdr_u_int32_t(xdrs, &objp->vers)) {
			return (FALSE);
		}
		if (!xdr_u_int32_t(xdrs, &objp->proc)) {
			return (FALSE);
		}
	} else {
		IXDR_PUT_U_INT32(buf, objp->prog);
		IXDR_PUT_U_INT32(buf, objp->vers);
		IXDR_PUT_U_INT32(buf, objp->proc);
	}

	/*
	 * All the jugglery for just getting the size of the arguments
	 */
	lenposition = XDR_GETPOS(xdrs);
	if (! xdr_u_int(xdrs, &(objp->args.args_len))) {
		return (FALSE);
	}
	argposition = XDR_GETPOS(xdrs);
	if (! (*objp->xdr_args)(xdrs, objp->args.args_val)) {
		return (FALSE);
	}
	position = XDR_GETPOS(xdrs);
	objp->args.args_len = (u_int)((u_long)position - (u_long)argposition);
	XDR_SETPOS(xdrs, lenposition);
	if (! xdr_u_int(xdrs, &(objp->args.args_len))) {
		return (FALSE);
	}
	XDR_SETPOS(xdrs, position);
	return (TRUE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rpcb_rmtcallres(xdrs, p)
	XDR *xdrs;
	struct rpcb_rmtcallres *p;
{
	bool_t dummy;
	struct r_rpcb_rmtcallres *objp = (struct r_rpcb_rmtcallres *)(void *)p;

	if (!xdr_string(xdrs, &objp->addr, RPC_MAXDATASIZE)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->results.results_len)) {
		return (FALSE);
	}
	dummy = (*(objp->xdr_res))(xdrs, objp->results.results_val);
	return (dummy);
}

bool_t
xdr_netbuf(xdrs, objp)
	XDR *xdrs;
	struct netbuf *objp;
{
	bool_t dummy;

	if (!xdr_u_int32_t(xdrs, (u_int32_t *) &objp->maxlen)) {
		return (FALSE);
	}

	if (objp->maxlen > RPC_MAXDATASIZE) {
		return (FALSE);
	}

	dummy = xdr_bytes(xdrs, (char **)&(objp->buf),
			(u_int *)&(objp->len), objp->maxlen);
	return (dummy);
}
