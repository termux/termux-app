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
 * clnt_perror.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>

static char *buf;

static char *_buf(void);
static char *auth_errmsg(enum auth_stat);
#define CLNT_PERROR_BUFLEN 256

static char *
_buf()
{

	if (buf == 0)
		buf = (char *)malloc(CLNT_PERROR_BUFLEN);
	return (buf);
}

/*
 * Print reply error info
 */
char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	const char *s;
{
	struct rpc_err e;
	char *err;
	char *str;
	char *strstart;
	size_t len, i;

	if (rpch == NULL || s == NULL)
		return(0);

	str = _buf(); /* side effect: sets CLNT_PERROR_BUFLEN */
	if (str == 0)
		return (0);
	len = CLNT_PERROR_BUFLEN;
	strstart = str;
	CLNT_GETERR(rpch, &e);

	if (snprintf(str, len, "%s: ", s) > 0) {
		i = strlen(str);
		str += i;
		len -= i;
	}

	(void)strncpy(str, clnt_sperrno(e.re_status), len - 1);
	i = strlen(str);
	str += i;
	len -= i;

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_PMAPFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		snprintf(str, len, "; errno = %s", strerror(e.re_errno)); 
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		break;

	case RPC_VERSMISMATCH:
		snprintf(str, len, "; low version = %u, high version = %u", 
			e.re_vers.low, e.re_vers.high);
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		snprintf(str, len, "; why = ");
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		if (err != NULL) {
			snprintf(str, len, "%s",err);
		} else {
			snprintf(str, len,
				"(unknown authentication error - %d)",
				(int) e.re_why);
		}
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		break;

	case RPC_PROGVERSMISMATCH:
		snprintf(str, len, "; low version = %u, high version = %u", 
			e.re_vers.low, e.re_vers.high);
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		break;

	default:	/* unknown */
		snprintf(str, len, "; s1 = %u, s2 = %u", 
			e.re_lb.s1, e.re_lb.s2);
		i = strlen(str);
		if (i > 0) {
			str += i;
			len -= i;
		}
		break;
	}
	strstart[CLNT_PERROR_BUFLEN-1] = '\0';
	return(strstart) ;
}

void
clnt_perror(rpch, s)
	CLIENT *rpch;
	const char *s;
{

	if (rpch == NULL || s == NULL)
		return;

	(void) fprintf(stderr, "%s\n", clnt_sperror(rpch,s));
}

static const char *const rpc_errlist[] = {
	"RPC: Success",				/*  0 - RPC_SUCCESS */
	"RPC: Can't encode arguments",		/*  1 - RPC_CANTENCODEARGS */
	"RPC: Can't decode result",		/*  2 - RPC_CANTDECODERES */
	"RPC: Unable to send",			/*  3 - RPC_CANTSEND */
	"RPC: Unable to receive",		/*  4 - RPC_CANTRECV */
	"RPC: Timed out",			/*  5 - RPC_TIMEDOUT */
	"RPC: Incompatible versions of RPC",	/*  6 - RPC_VERSMISMATCH */
	"RPC: Authentication error",		/*  7 - RPC_AUTHERROR */
	"RPC: Program unavailable",		/*  8 - RPC_PROGUNAVAIL */
	"RPC: Program/version mismatch",	/*  9 - RPC_PROGVERSMISMATCH */
	"RPC: Procedure unavailable",		/* 10 - RPC_PROCUNAVAIL */
	"RPC: Server can't decode arguments",	/* 11 - RPC_CANTDECODEARGS */
	"RPC: Remote system error",		/* 12 - RPC_SYSTEMERROR */
	"RPC: Unknown host",			/* 13 - RPC_UNKNOWNHOST */
	"RPC: Port mapper failure",		/* 14 - RPC_PMAPFAILURE */
	"RPC: Program not registered",		/* 15 - RPC_PROGNOTREGISTERED */
	"RPC: Failed (unspecified error)",	/* 16 - RPC_FAILED */
	"RPC: Unknown protocol"			/* 17 - RPC_UNKNOWNPROTO */
};


/*
 * This interface for use by clntrpc
 */
char *
clnt_sperrno(stat)
	enum clnt_stat stat;
{
	unsigned int errnum = stat;

	if (errnum < (sizeof(rpc_errlist)/sizeof(rpc_errlist[0])))
		/* LINTED interface problem */
		return (char *)rpc_errlist[errnum];

	return ("RPC: (unknown error code)");
}

void
clnt_perrno(num)
	enum clnt_stat num;
{
	(void) fprintf(stderr, "%s\n", clnt_sperrno(num));
}


char *
clnt_spcreateerror(s)
	const char *s;
{
	char *str, *err;
	size_t len, i;

	if (s == NULL)
		return(0);

	str = _buf(); /* side effect: sets CLNT_PERROR_BUFLEN */
	if (str == 0)
		return(0);
	len = CLNT_PERROR_BUFLEN;
	snprintf(str, len, "%s: ", s);
	i = strlen(str);
	if (i > 0)
		len -= i;
	(void)strncat(str, clnt_sperrno(rpc_createerr.cf_stat), len - 1);
	switch (rpc_createerr.cf_stat) {
	case RPC_PMAPFAILURE:
		(void) strncat(str, " - ", len - 1);
		 err = clnt_sperrno(rpc_createerr.cf_error.re_status);
		if (err)
			(void) strncat(str, err+5, len-5);
		switch(rpc_createerr.cf_error.re_status) {
		case RPC_CANTSEND:
		case RPC_CANTRECV:
			i = strlen(str);
			len -= i;
			snprintf(str+i, len, ": errno %d (%s)", 
				rpc_createerr.cf_error.re_errno,
				strerror(rpc_createerr.cf_error.re_errno)); 
			break;
		default:
			break;
		}
		break;

	case RPC_SYSTEMERROR:
		(void)strncat(str, " - ", len - 1);
		(void)strncat(str, strerror(rpc_createerr.cf_error.re_errno),
		    len - 4);
		break;

	case RPC_CANTSEND:
	case RPC_CANTDECODERES:
	case RPC_CANTENCODEARGS:
	case RPC_SUCCESS:
	case RPC_UNKNOWNPROTO:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
	case RPC_UNKNOWNHOST:
	case RPC_CANTDECODEARGS:
	case RPC_PROCUNAVAIL:
	case RPC_PROGVERSMISMATCH:
	case RPC_PROGUNAVAIL:
	case RPC_AUTHERROR:
	case RPC_VERSMISMATCH:
	case RPC_TIMEDOUT:
	case RPC_CANTRECV:
	default:
		break;
	}
	str[CLNT_PERROR_BUFLEN-1] = '\0';
	return (str);
}

void
clnt_pcreateerror(s)
	const char *s;
{

	if (s == NULL)
		return;

	(void) fprintf(stderr, "%s\n", clnt_spcreateerror(s));
}

static const char *const auth_errlist[] = {
	"Authentication OK",			/* 0 - AUTH_OK */
	"Invalid client credential",		/* 1 - AUTH_BADCRED */
	"Server rejected credential",		/* 2 - AUTH_REJECTEDCRED */
	"Invalid client verifier", 		/* 3 - AUTH_BADVERF */
	"Server rejected verifier", 		/* 4 - AUTH_REJECTEDVERF */
	"Client credential too weak",		/* 5 - AUTH_TOOWEAK */
	"Invalid server verifier",		/* 6 - AUTH_INVALIDRESP */
	"Failed (unspecified error)"		/* 7 - AUTH_FAILED */
};

static char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	unsigned int errnum = stat;

	if (errnum < (sizeof(auth_errlist)/sizeof(auth_errlist[0])))
		/* LINTED interface problem */
		return (char *)auth_errlist[errnum];

	return(NULL);
}
