/*

Copyright 1993, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

 * Copyright 1993, 1994 NCR Corporation - Dayton, Ohio, USA
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name NCR not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCR makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCRS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NCR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * These are some utility functions created for convenience or to provide
 * an interface that is similar to an existing interface. These are built
 * only using the Transport Independant API, and have no knowledge of
 * the internal implementation.
 */

#ifdef XTHREADS
#include <X11/Xthreads.h>
#endif
#ifdef WIN32
#include <X11/Xlibint.h>
#include <X11/Xwinsock.h>
#endif

#ifdef X11_t

/*
 * These values come from X.h and Xauth.h, and MUST match them. Some
 * of these values are also defined by the ChangeHost protocol message.
 */

#define FamilyInternet		0	/* IPv4 */
#define FamilyDECnet		1
#define FamilyChaos		2
#define FamilyInternet6		6
#define FamilyAmoeba		33
#define FamilyLocalHost		252
#define FamilyKrb5Principal	253
#define FamilyNetname		254
#define FamilyLocal		256
#define FamilyWild		65535

/*
 * TRANS(ConvertAddress) converts a sockaddr based address to an
 * X authorization based address. Some of this is defined as part of
 * the ChangeHost protocol. The rest is just done in a consistent manner.
 */

int
TRANS(ConvertAddress)(int *familyp, int *addrlenp, Xtransaddr **addrp)

{

    prmsg(2,"ConvertAddress(%d,%d,%p)\n",*familyp,*addrlenp,*addrp);

    switch( *familyp )
    {
#if defined(TCPCONN)
    case AF_INET:
    {
	/*
	 * Check for the BSD hack localhost address 127.0.0.1.
	 * In this case, we are really FamilyLocal.
	 */

	struct sockaddr_in saddr;
	int len = sizeof(saddr.sin_addr.s_addr);
	char *cp = (char *) &saddr.sin_addr.s_addr;

	memcpy (&saddr, *addrp, sizeof (struct sockaddr_in));

	if ((len == 4) && (cp[0] == 127) && (cp[1] == 0) &&
	    (cp[2] == 0) && (cp[3] == 1))
	{
	    *familyp=FamilyLocal;
	}
	else
	{
	    *familyp=FamilyInternet;
	    *addrlenp=len;
	    memcpy(*addrp,&saddr.sin_addr,len);
	}
	break;
    }

#if defined(IPv6) && defined(AF_INET6)
    case AF_INET6:
    {
	struct sockaddr_in6 saddr6;

	memcpy (&saddr6, *addrp, sizeof (struct sockaddr_in6));

	if (IN6_IS_ADDR_LOOPBACK(&saddr6.sin6_addr))
	{
	    *familyp=FamilyLocal;
	}
	else if (IN6_IS_ADDR_V4MAPPED(&(saddr6.sin6_addr))) {
	    char *cp = (char *) &saddr6.sin6_addr.s6_addr[12];

	    if ((cp[0] == 127) && (cp[1] == 0) &&
	      (cp[2] == 0) && (cp[3] == 1))
	    {
		*familyp=FamilyLocal;
	    }
	    else
	    {
		*familyp=FamilyInternet;
		*addrlenp = sizeof (struct in_addr);
		memcpy(*addrp,cp,*addrlenp);
	    }
	}
	else
	{
	    *familyp=FamilyInternet6;
	    *addrlenp=sizeof(saddr6.sin6_addr);
	    memcpy(*addrp,&saddr6.sin6_addr,sizeof(saddr6.sin6_addr));
	}
	break;
    }
#endif /* IPv6 */
#endif /* defined(TCPCONN) */


#if defined(UNIXCONN) || defined(LOCALCONN)
    case AF_UNIX:
    {
	*familyp=FamilyLocal;
	break;
    }
#endif /* defined(UNIXCONN) || defined(LOCALCONN) */

#if (defined(__SCO__) || defined(__UNIXWARE__)) && defined(LOCALCONN)
    case 0:
    {
	*familyp=FamilyLocal;
	break;
    }
#endif

    default:
	prmsg(1,"ConvertAddress: Unknown family type %d\n",
	      *familyp);
	return -1;
    }


    if (*familyp == FamilyLocal)
    {
	/*
	 * In the case of a local connection, we need to get the
	 * host name for authentication.
	 */

	char hostnamebuf[256];
	int len = TRANS(GetHostname) (hostnamebuf, sizeof hostnamebuf);

	if (len > 0) {
	    if (*addrp && *addrlenp < (len + 1))
	    {
		free (*addrp);
		*addrp = NULL;
	    }
	    if (!*addrp)
		*addrp = malloc (len + 1);
	    if (*addrp) {
		strcpy ((char *) *addrp, hostnamebuf);
		*addrlenp = len;
	    } else {
		*addrlenp = 0;
	    }
	}
	else
	{
	    if (*addrp)
		free (*addrp);
	    *addrp = NULL;
	    *addrlenp = 0;
	}
    }

    return 0;
}

#endif /* X11_t */

#ifdef ICE_t

/* Needed for _XGethostbyaddr usage in TRANS(GetPeerNetworkId) */
# if defined(TCPCONN) || defined(UNIXCONN)
#  define X_INCLUDE_NETDB_H
#  define XOS_USE_NO_LOCKING
#  include <X11/Xos_r.h>
# endif

#include <signal.h>

char *
TRANS(GetMyNetworkId) (XtransConnInfo ciptr)

{
    int		family = ciptr->family;
    char 	*addr = ciptr->addr;
    char	hostnamebuf[256];
    char 	*networkId = NULL;
    const char	*transName = ciptr->transptr->TransName;

    if (gethostname (hostnamebuf, sizeof (hostnamebuf)) < 0)
    {
	return (NULL);
    }

    switch (family)
    {
#if defined(UNIXCONN) || defined(LOCALCONN)
    case AF_UNIX:
    {
	struct sockaddr_un *saddr = (struct sockaddr_un *) addr;
	networkId = malloc (3 + strlen (transName) +
	    strlen (hostnamebuf) + strlen (saddr->sun_path));
	sprintf (networkId, "%s/%s:%s", transName,
	    hostnamebuf, saddr->sun_path);
	break;
    }
#endif /* defined(UNIXCONN) || defined(LOCALCONN) */

#if defined(TCPCONN)
    case AF_INET:
#if defined(IPv6) && defined(AF_INET6)
    case AF_INET6:
#endif
    {
	struct sockaddr_in *saddr = (struct sockaddr_in *) addr;
#if defined(IPv6) && defined(AF_INET6)
	struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) addr;
#endif
	int portnum;
	char portnumbuf[10];


#if defined(IPv6) && defined(AF_INET6)
	if (family == AF_INET6)
	    portnum = ntohs (saddr6->sin6_port);
	else
#endif
	    portnum = ntohs (saddr->sin_port);

	snprintf (portnumbuf, sizeof(portnumbuf), "%d", portnum);
	networkId = malloc (3 + strlen (transName) +
	    strlen (hostnamebuf) + strlen (portnumbuf));
	sprintf (networkId, "%s/%s:%s", transName, hostnamebuf, portnumbuf);
	break;
    }
#endif /* defined(TCPCONN) */


    default:
	break;
    }

    return (networkId);
}

#include <setjmp.h>
static jmp_buf env;

#ifdef SIGALRM
static volatile int nameserver_timedout = 0;

static void
nameserver_lost(int sig _X_UNUSED)
{
  nameserver_timedout = 1;
  longjmp (env, -1);
  /* NOTREACHED */
}
#endif /* SIGALARM */


char *
TRANS(GetPeerNetworkId) (XtransConnInfo ciptr)

{
    int		family = ciptr->family;
    char	*peer_addr = ciptr->peeraddr;
    char	*hostname;
    char	addrbuf[256];
    const char	*addr = NULL;

    switch (family)
    {
    case AF_UNSPEC:
#if defined(UNIXCONN) || defined(LOCALCONN)
    case AF_UNIX:
    {
	if (gethostname (addrbuf, sizeof (addrbuf)) == 0)
	    addr = addrbuf;
	break;
    }
#endif /* defined(UNIXCONN) || defined(LOCALCONN) */

#if defined(TCPCONN)
    case AF_INET:
#if defined(IPv6) && defined(AF_INET6)
    case AF_INET6:
#endif
    {
	struct sockaddr_in *saddr = (struct sockaddr_in *) peer_addr;
#if defined(IPv6) && defined(AF_INET6)
	struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) peer_addr;
#endif
	char *address;
	int addresslen;
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
	_Xgethostbynameparams hparams;
#endif
	struct hostent * volatile hostp = NULL;

#if defined(IPv6) && defined(AF_INET6)
	if (family == AF_INET6)
	{
	    address = (char *) &saddr6->sin6_addr;
	    addresslen = sizeof (saddr6->sin6_addr);
	}
	else
#endif
	{
	    address = (char *) &saddr->sin_addr;
	    addresslen = sizeof (saddr->sin_addr);
	}

#ifdef SIGALRM
	/*
	 * gethostbyaddr can take a LONG time if the host does not exist.
	 * Assume that if it does not respond in NAMESERVER_TIMEOUT seconds
	 * that something is wrong and do not make the user wait.
	 * gethostbyaddr will continue after a signal, so we have to
	 * jump out of it.
	 */

	nameserver_timedout = 0;
	signal (SIGALRM, nameserver_lost);
	alarm (4);
	if (setjmp(env) == 0) {
#endif
	    hostp = _XGethostbyaddr (address, addresslen, family, hparams);
#ifdef SIGALRM
	}
	alarm (0);
#endif
	if (hostp != NULL)
	  addr = hostp->h_name;
	else
#if defined(IPv6) && defined(AF_INET6)
	  addr = inet_ntop (family, address, addrbuf, sizeof (addrbuf));
#else
	  addr = inet_ntoa (saddr->sin_addr);
#endif
	break;
    }

#endif /* defined(TCPCONN) */


    default:
	return (NULL);
    }


    hostname = malloc (strlen (ciptr->transptr->TransName) + strlen (addr) + 2);
    strcpy (hostname, ciptr->transptr->TransName);
    strcat (hostname, "/");
    if (addr)
	strcat (hostname, addr);

    return (hostname);
}

#endif /* ICE_t */


#if defined(WIN32) && defined(TCPCONN)
int
TRANS(WSAStartup) (void)
{
    static WSADATA wsadata;

    prmsg (2,"WSAStartup()\n");

    if (!wsadata.wVersion && WSAStartup(MAKEWORD(2,2), &wsadata))
        return 1;
    return 0;
}
#endif

#include <ctype.h>

static int
is_numeric (const char *str)
{
    int i;

    for (i = 0; i < (int) strlen (str); i++)
	if (!isdigit (str[i]))
	    return (0);

    return (1);
}

#ifdef TRANS_SERVER
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if !defined(S_IFLNK) && !defined(S_ISLNK)
#undef lstat
#define lstat(a,b) stat(a,b)
#endif

#define FAIL_IF_NOMODE  1
#define FAIL_IF_NOT_ROOT 2
#define WARN_NO_ACCESS 4

/*
 * We make the assumption that when the 'sticky' (t) bit is requested
 * it's not save if the directory has non-root ownership or the sticky
 * bit cannot be set and fail.
 */
static int
trans_mkdir(const char *path, int mode)
{
    struct stat buf;

    if (lstat(path, &buf) != 0) {
	if (errno != ENOENT) {
	    prmsg(1, "mkdir: ERROR: (l)stat failed for %s (%d)\n",
		  path, errno);
	    return -1;
	}
	/* Dir doesn't exist. Try to create it */

#ifndef WIN32
	if (mkdir(path, mode) == 0) {
	    if (chmod(path, mode)) {
		prmsg(1, "mkdir: ERROR: Mode of %s should be set to %04o\n",
		      path, mode);
#ifdef FAIL_HARD
		return -1;
#endif
	    }
#else
	if (mkdir(path) == 0) {
#endif
	} else {
	    prmsg(1, "mkdir: ERROR: Cannot create %s\n",
		  path);
	    return -1;
	}

	return 0;

    } else {
	if (S_ISDIR(buf.st_mode)) {
	    int updateMode = 0;
	    int updatedMode = 0;
	    int status = 0;

	    /*
	     * Check if the directory's mode is OK.  An exact match isn't
	     * required, just a mode that isn't more permissive than the
	     * one requested.
	     */
	    if ((~mode) & 0077 & buf.st_mode)
		updateMode = 1;

	    /*
	     * If the directory is not writeable not everybody may
	     * be able to create sockets. Therefore warn if mode
	     * cannot be fixed.
	     */
	    if ((~buf.st_mode) & 0022 & mode) {
		updateMode = 1;
		status |= WARN_NO_ACCESS;
	    }

	    /*
	     * If 'sticky' bit is requested fail if owner isn't root
	     * as we assume the caller makes certain security implications
	     */
	    if (mode & 01000) {
		status |= FAIL_IF_NOT_ROOT;
		if (!(buf.st_mode & 01000)) {
		    status |= FAIL_IF_NOMODE;
		    updateMode = 1;
		}
	    }

#ifdef HAS_FCHOWN
	    /*
	     * If fchown(2) and fchmod(2) are available, try to correct the
	     * directory's owner and mode.  Otherwise it isn't safe to attempt
	     * to do this.
	     */
	    if (updateMode) {
		int fd = -1;
		struct stat fbuf;
		if ((fd = open(path, O_RDONLY)) != -1) {
		    if (fstat(fd, &fbuf) == -1) {
			prmsg(1, "mkdir: ERROR: fstat failed for %s (%d)\n",
			      path, errno);
			close(fd);
			return -1;
		    }
		    /*
		     * Verify that we've opened the same directory as was
		     * checked above.
		     */
		    if (!S_ISDIR(fbuf.st_mode) ||
			buf.st_dev != fbuf.st_dev ||
			buf.st_ino != fbuf.st_ino) {
			prmsg(1, "mkdir: ERROR: inode for %s changed\n",
			      path);
			close(fd);
			return -1;
		    }
		    if (updateMode && fchmod(fd, mode) == 0)
			updatedMode = 1;
		    close(fd);
		}
	    }
#endif

	    if (updateMode && !updatedMode) {
#ifdef FAIL_HARD
		if (status & FAIL_IF_NOMODE) {
		    prmsg(1, "mkdir: ERROR: Mode of %s must be set to %04o\n",
			  path, mode);
		    return -1;
		}
#endif
		prmsg(1, "mkdir: Mode of %s should be set to %04o\n",
		      path, mode);
		if (status & WARN_NO_ACCESS) {
		    prmsg(1, "mkdir: this may cause subsequent errors\n");
		}
	    }
	    return 0;
	}
	return -1;
    }

    /* In all other cases, fail */
    return -1;
}

#endif /* TRANS_SERVER */
