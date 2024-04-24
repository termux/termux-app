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
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>
#ifdef YP
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <libc_private.h>
#endif

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_GETRPCBYNAME || !HAVE_GETRPCBYNUMBER || \
	!HAVE_SETRPCENT || !HAVE_ENDRPCENT || !HAVE_GETRPCENT

/*
 * Internet version.
 */
static struct rpcdata {
	FILE	*rpcf;
	int	stayopen;
#define	MAXALIASES	35
	char	*rpc_aliases[MAXALIASES];
	struct	rpcent rpc;
	char	line[BUFSIZ+1];
#ifdef	YP
	char	*domain;
	char	*current;
	int	currentlen;
#endif
} *rpcdata;

static	struct rpcent *interpret(char *val, size_t len);

#ifdef	YP
static int	__yp_nomap = 0;
#endif	/* YP */

#define	RPCDB	"/etc/rpc"

static struct rpcdata *_rpcdata(void);

static struct rpcdata *
_rpcdata()
{
	struct rpcdata *d = rpcdata;

	if (d == 0) {
		d = (struct rpcdata *)calloc(1, sizeof (struct rpcdata));
		rpcdata = d;
	}
	return (d);
}

#if !HAVE_GETRPCBYNUMBER
struct rpcent *
getrpcbynumber(number)
	int number;
{
#ifdef	YP
	int reason;
	char adrstr[16];
#endif
	struct rpcent *p;
	struct rpcdata *d = _rpcdata();

	if (d == 0)
		return (0);
#ifdef	YP
        if (!__yp_nomap && __yp_check(&d->domain)) {
                sprintf(adrstr, "%d", number);
                reason = yp_match(d->domain, "rpc.bynumber", adrstr, strlen(adrstr),
                                  &d->current, &d->currentlen);
                switch(reason) {
                case 0:
                        break;
                case YPERR_MAP:
                        __yp_nomap = 1;
                        goto no_yp;
                        break;
                default:
                        return(0);
                        break;
                }
                d->current[d->currentlen] = '\0';
                p = interpret(d->current, d->currentlen);
                (void) free(d->current);
                return p;
        }
no_yp:
#endif	/* YP */

	setrpcent(0);
	while ((p = getrpcent()) != NULL) {
		if (p->r_number == number)
			break;
	}
	endrpcent();
	return (p);
}
#endif /* !HAVE_GETRPCBYNUMBER */

#if !HAVE_GETRPCBYNAME
struct rpcent *
getrpcbyname(name)
	const char *name;
{
	struct rpcent *rpc = NULL;
	char **rp;

	assert(name != NULL);

	setrpcent(0);
	while ((rpc = getrpcent()) != NULL) {
		if (strcmp(rpc->r_name, name) == 0)
			goto done;
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0)
				goto done;
		}
	}
done:
	endrpcent();
	return (rpc);
}
#endif /* !HAVE_GETRPCBYNAME */

#if !HAVE_SETRPCENT
void
setrpcent(f)
	int f;
{
	struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
#ifdef	YP
        if (!__yp_nomap && __yp_check(NULL)) {
                if (d->current)
                        free(d->current);
                d->current = NULL;
                d->currentlen = 0;
                return;
        }
        __yp_nomap = 0;
#endif	/* YP */
	if (d->rpcf == NULL)
		d->rpcf = fopen(RPCDB, "r");
	else
		rewind(d->rpcf);
	d->stayopen |= f;
}
#endif

#if !HAVE_ENDRPCENT
void
endrpcent()
{
	struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
#ifdef	YP
        if (!__yp_nomap && __yp_check(NULL)) {
        	if (d->current && !d->stayopen)
                        free(d->current);
                d->current = NULL;
                d->currentlen = 0;
                return;
        }
        __yp_nomap = 0;
#endif	/* YP */
	if (d->rpcf && !d->stayopen) {
		fclose(d->rpcf);
		d->rpcf = NULL;
	}
}
#endif

#if !HAVE_GETRPCENT
struct rpcent *
getrpcent()
{
	struct rpcdata *d = _rpcdata();
#ifdef	YP
	struct rpcent *hp;
	int reason;
	char *val = NULL;
	int vallen;
#endif

	if (d == 0)
		return(NULL);
#ifdef	YP
        if (!__yp_nomap && __yp_check(&d->domain)) {
                if (d->current == NULL && d->currentlen == 0) {
                        reason = yp_first(d->domain, "rpc.bynumber",
                                          &d->current, &d->currentlen,
                                          &val, &vallen);
                } else {
                        reason = yp_next(d->domain, "rpc.bynumber",
                                         d->current, d->currentlen,
                                         &d->current, &d->currentlen,
                                         &val, &vallen);
                }
                switch(reason) {
                case 0:
                        break;
                case YPERR_MAP:
                        __yp_nomap = 1;
                        goto no_yp;
                        break;
                default:
                        return(0);
                        break;
                }
                val[vallen] = '\0';
                hp = interpret(val, vallen);
                (void) free(val);
                return hp;
        }
no_yp:
#endif	/* YP */
	if (d->rpcf == NULL && (d->rpcf = fopen(RPCDB, "r")) == NULL)
		return (NULL);
	/* -1 so there is room to append a \n below */
        if (fgets(d->line, BUFSIZ - 1, d->rpcf) == NULL)
		return (NULL);
	return (interpret(d->line, strlen(d->line)));
}
#endif

static struct rpcent *
interpret(val, len)
	char *val;
	size_t len;
{
	struct rpcdata *d = _rpcdata();
	char *p;
	char *cp, **q;

	assert(val != NULL);

	if (d == 0)
		return (0);
	(void) strncpy(d->line, val, BUFSIZ);
	d->line[BUFSIZ] = '\0';
	p = d->line;
	p[len] = '\n';
	if (*p == '#')
		return (getrpcent());
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (getrpcent());
	*cp = '\0';
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (getrpcent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->rpc.r_name = d->line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->rpc.r_number = atoi(cp);
	q = d->rpc.r_aliases = d->rpc_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->rpc_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->rpc);
}

#endif
