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
 * Copyright (c) 1988 by Sun Microsystems, Inc.

 */

/*
 * rtime - get time from remote machine
 *
 * gets time, obtaining value from host
 * on the udp/time socket.  Since timeserver returns
 * with time of day in seconds since Jan 1, 1900,  must
 * subtract seconds before Jan 1, 1970 to get
 * what unix uses.
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/select.h>

extern int _rpc_dtablesize( void );

#define NYEARS	(unsigned long)(1970 - 1900)
#define TOFFSET (unsigned long)(60*60*24*(365*NYEARS + (NYEARS/4)))

static void do_close( int );

int
rtime(addrp, timep, timeout)
	struct sockaddr_in *addrp;
	struct timeval *timep;
	struct timeval *timeout;
{
	int s;
	struct pollfd fd;
	int milliseconds;
	int res;
	unsigned long thetime;
	struct sockaddr_in from;
	socklen_t fromlen;
	int type;
	struct servent *serv;

	if (timeout == NULL) {
		type = SOCK_STREAM;
	} else {
		type = SOCK_DGRAM;
	}
	s = socket(AF_INET, type, 0);
	if (s < 0) {
		return(-1);
	}
	addrp->sin_family = AF_INET;

	/* TCP and UDP port are the same in this case */
	if ((serv = getservbyname("time", "tcp")) == NULL) {
		do_close(s);
		return(-1);
	}

	addrp->sin_port = serv->s_port;

	if (type == SOCK_DGRAM) {
		res = sendto(s, (char *)&thetime, sizeof(thetime), 0,
			     (struct sockaddr *)addrp, sizeof(*addrp));
		if (res < 0) {
			do_close(s);
			return(-1);
		}

		milliseconds = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);
		fd.fd = s;
		fd.events = POLLIN;
		do
		  res = poll (&fd, 1, milliseconds);
		while (res < 0 && errno == EINTR);
		if (res <= 0) {
			if (res == 0) {
				errno = ETIMEDOUT;
			}
			do_close(s);
			return(-1);
		}
		fromlen = sizeof(from);
		res = recvfrom(s, (char *)&thetime, sizeof(thetime), 0,
			       (struct sockaddr *)&from, &fromlen);
		do_close(s);
		if (res < 0) {
			return(-1);
		}
	} else {
		if (connect(s, (struct sockaddr *)addrp, sizeof(*addrp)) < 0) {
			do_close(s);
			return(-1);
		}
		res = read(s, (char *)&thetime, sizeof(thetime));
		do_close(s);
		if (res < 0) {
			return(-1);
		}
	}
	if (res != sizeof(thetime)) {
		errno = EIO;
		return(-1);	
	}
	thetime = ntohl(thetime);
	timep->tv_sec = thetime - TOFFSET;
	timep->tv_usec = 0;
	return(0);
}

static void
do_close(s)
	int s;
{
	int save;

	save = errno;
	(void)close(s);
	errno = save;
}
