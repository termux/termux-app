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
 * NCR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NCR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYSTEMD_DAEMON
#include <systemd/sd-daemon.h>
#endif

/*
 * The transport table contains a definition for every transport (protocol)
 * family. All operations that can be made on the transport go through this
 * table.
 *
 * Each transport is assigned a unique transport id.
 *
 * New transports can be added by adding an entry in this table.
 * For compatiblity, the transport ids should never be renumbered.
 * Always add to the end of the list.
 */

#define TRANS_TLI_INET_INDEX		1
#define TRANS_TLI_TCP_INDEX		2
#define TRANS_TLI_TLI_INDEX		3
#define TRANS_SOCKET_UNIX_INDEX		4
#define TRANS_SOCKET_LOCAL_INDEX	5
#define TRANS_SOCKET_INET_INDEX		6
#define TRANS_SOCKET_TCP_INDEX		7
#define TRANS_DNET_INDEX		8
#define TRANS_LOCAL_LOCAL_INDEX		9
#define TRANS_LOCAL_PTS_INDEX		10
#define TRANS_LOCAL_NAMED_INDEX		11
/* 12 used to be ISC, but that's gone. */
#define TRANS_LOCAL_SCO_INDEX		13
#define TRANS_SOCKET_INET6_INDEX	14
#define TRANS_LOCAL_PIPE_INDEX		15


static
Xtransport_table Xtransports[] = {
#if defined(TCPCONN)
    { &TRANS(SocketTCPFuncs),	TRANS_SOCKET_TCP_INDEX },
#if defined(IPv6) && defined(AF_INET6)
    { &TRANS(SocketINET6Funcs),	TRANS_SOCKET_INET6_INDEX },
#endif /* IPv6 */
    { &TRANS(SocketINETFuncs),	TRANS_SOCKET_INET_INDEX },
#endif /* TCPCONN */
#if defined(UNIXCONN)
#if !defined(LOCALCONN)
    { &TRANS(SocketLocalFuncs),	TRANS_SOCKET_LOCAL_INDEX },
#endif /* !LOCALCONN */
    { &TRANS(SocketUNIXFuncs),	TRANS_SOCKET_UNIX_INDEX },
#endif /* UNIXCONN */
#if defined(LOCALCONN)
    { &TRANS(LocalFuncs),	TRANS_LOCAL_LOCAL_INDEX },
#ifndef __sun
    { &TRANS(PTSFuncs),		TRANS_LOCAL_PTS_INDEX },
#endif /* __sun */
#if defined(SVR4) || defined(__SVR4)
    { &TRANS(NAMEDFuncs),	TRANS_LOCAL_NAMED_INDEX },
#endif
#ifdef __sun
    { &TRANS(PIPEFuncs),	TRANS_LOCAL_PIPE_INDEX },
#endif /* __sun */
#if defined(__SCO__) || defined(__UNIXWARE__)
    { &TRANS(SCOFuncs),		TRANS_LOCAL_SCO_INDEX },
#endif /* __SCO__ || __UNIXWARE__ */
#endif /* LOCALCONN */
};

#define NUMTRANS	(sizeof(Xtransports)/sizeof(Xtransport_table))


#ifdef WIN32
#define ioctl ioctlsocket
#endif



/*
 * These are a few utility function used by the public interface functions.
 */

void
TRANS(FreeConnInfo) (XtransConnInfo ciptr)

{
    prmsg (3,"FreeConnInfo(%p)\n", ciptr);

    if (ciptr->addr)
	free (ciptr->addr);

    if (ciptr->peeraddr)
	free (ciptr->peeraddr);

    if (ciptr->port)
	free (ciptr->port);

    free (ciptr);
}


#define PROTOBUFSIZE	20

static Xtransport *
TRANS(SelectTransport) (const char *protocol)

{
#ifndef HAVE_STRCASECMP
    char 	protobuf[PROTOBUFSIZE];
#endif
    int		i;

    prmsg (3,"SelectTransport(%s)\n", protocol);

#ifndef HAVE_STRCASECMP
    /*
     * Force Protocol to be lowercase as a way of doing
     * a case insensitive match.
     */

    strncpy (protobuf, protocol, PROTOBUFSIZE - 1);
    protobuf[PROTOBUFSIZE-1] = '\0';

    for (i = 0; i < PROTOBUFSIZE && protobuf[i] != '\0'; i++)
	if (isupper ((unsigned char)protobuf[i]))
	    protobuf[i] = tolower ((unsigned char)protobuf[i]);
#endif

    /* Look at all of the configured protocols */

    for (i = 0; i < NUMTRANS; i++)
    {
#ifndef HAVE_STRCASECMP
	if (!strcmp (protobuf, Xtransports[i].transport->TransName))
#else
	if (!strcasecmp (protocol, Xtransports[i].transport->TransName))
#endif
	    return Xtransports[i].transport;
    }

    return NULL;
}

#ifndef TEST_t
static
#endif /* TEST_t */
int
TRANS(ParseAddress) (const char *address,
                     char **protocol, char **host, char **port)

{
    /*
     * For the font library, the address is a string formatted
     * as "protocol/host:port[/catalogue]".  Note that the catologue
     * is optional.  At this time, the catologue info is ignored, but
     * we have to parse it anyways.
     *
     * Other than fontlib, the address is a string formatted
     * as "protocol/host:port".
     *
     * If the protocol part is missing, then assume TCP.
     * If the protocol part and host part are missing, then assume local.
     * If a "::" is found then assume DNET.
     */

    char	*mybuf, *tmpptr;
    const char	*_protocol;
    char	*_host, *_port;
    char	hostnamebuf[256];
    int		_host_len;

    prmsg (3,"ParseAddress(%s)\n", address);

    /* Copy the string so it can be changed */

    tmpptr = mybuf = strdup (address);

    /* Parse the string to get each component */

    /* Get the protocol part */

    _protocol = mybuf;


   if ( ((mybuf = strchr (mybuf,'/')) == NULL) &&
      ((mybuf = strrchr (tmpptr,':')) == NULL) )
   {
	/* address is in a bad format */
	*protocol = NULL;
	*host = NULL;
	*port = NULL;
	free (tmpptr);
	return 0;
    }

    if (*mybuf == ':')
    {
	/*
	 * If there is a hostname, then assume tcp, otherwise
	 * it must be local.
	 */
	if (mybuf == tmpptr)
	{
	    /* There is neither a protocol or host specified */
	    _protocol = "local";
	}
	else
	{
	    /* There is a hostname specified */
	    _protocol = "tcp";
	    mybuf = tmpptr;	/* reset to the begining of the host ptr */
	}
    }
    else
    {
	/* *mybuf == '/' */

	*mybuf ++= '\0'; /* put a null at the end of the protocol */

	if (strlen(_protocol) == 0)
	{
	    /*
	     * If there is a hostname, then assume tcp, otherwise
	     * it must be local.
	     */
	    if (*mybuf != ':')
		_protocol = "tcp";
	    else
		_protocol = "local";
	}
    }

    /* Get the host part */

    _host = mybuf;

    if ((mybuf = strrchr (mybuf,':')) == NULL)
    {
	*protocol = NULL;
	*host = NULL;
	*port = NULL;
	free (tmpptr);
	return 0;
    }

    *mybuf ++= '\0';

    _host_len = strlen(_host);
    if (_host_len == 0)
    {
	TRANS(GetHostname) (hostnamebuf, sizeof (hostnamebuf));
	_host = hostnamebuf;
    }
#if defined(IPv6) && defined(AF_INET6)
    /* hostname in IPv6 [numeric_addr]:0 form? */
    else if ( (_host_len > 3) &&
      ((strcmp(_protocol, "tcp") == 0) || (strcmp(_protocol, "inet6") == 0))
      && (*_host == '[') && (*(_host + _host_len - 1) == ']') ) {
	struct sockaddr_in6 sin6;

	*(_host + _host_len - 1) = '\0';

	/* Verify address is valid IPv6 numeric form */
	if (inet_pton(AF_INET6, _host + 1, &sin6) == 1) {
	    /* It is. Use it as such. */
	    _host++;
	    _protocol = "inet6";
	} else {
	    /* It's not, restore it just in case some other code can use it. */
	    *(_host + _host_len - 1) = ']';
	}
    }
#endif


    /* Get the port */

    _port = mybuf;

#if defined(FONT_t) || defined(FS_t)
    /*
     * Is there an optional catalogue list?
     */

    if ((mybuf = strchr (mybuf,'/')) != NULL)
	*mybuf ++= '\0';

    /*
     * The rest, if any, is the (currently unused) catalogue list.
     *
     * _catalogue = mybuf;
     */
#endif

#ifdef HAVE_LAUNCHD
    /* launchd sockets will look like 'local//tmp/launch-XgkNns/:0' */
    if(address != NULL && strlen(address)>8 && (!strncmp(address,"local//",7))) {
      _protocol="local";
      _host="";
      _port=address+6;
    }
#endif

    /*
     * Now that we have all of the components, allocate new
     * string space for them.
     */

    if ((*protocol = strdup (_protocol)) == NULL)
    {
	/* Malloc failed */
	*port = NULL;
	*host = NULL;
	*protocol = NULL;
	free (tmpptr);
	return 0;
    }

    if ((*host = strdup (_host)) == NULL)
    {
	/* Malloc failed */
	*port = NULL;
	*host = NULL;
	free (*protocol);
	*protocol = NULL;
	free (tmpptr);
	return 0;
    }

    if ((*port = strdup (_port)) == NULL)
    {
	/* Malloc failed */
	*port = NULL;
	free (*host);
	*host = NULL;
	free (*protocol);
	*protocol = NULL;
	free (tmpptr);
	return 0;
    }

    free (tmpptr);

    return 1;
}


/*
 * TRANS(Open) does all of the real work opening a connection. The only
 * funny part about this is the type parameter which is used to decide which
 * type of open to perform.
 */

static XtransConnInfo
TRANS(Open) (int type, const char *address)

{
    char 		*protocol = NULL, *host = NULL, *port = NULL;
    XtransConnInfo	ciptr = NULL;
    Xtransport		*thistrans;

    prmsg (2,"Open(%d,%s)\n", type, address);

#if defined(WIN32) && defined(TCPCONN)
    if (TRANS(WSAStartup)())
    {
	prmsg (1,"Open: WSAStartup failed\n");
	return NULL;
    }
#endif

    /* Parse the Address */

    if (TRANS(ParseAddress) (address, &protocol, &host, &port) == 0)
    {
	prmsg (1,"Open: Unable to Parse address %s\n", address);
	return NULL;
    }

    /* Determine the transport type */

    if ((thistrans = TRANS(SelectTransport) (protocol)) == NULL)
    {
	prmsg (1,"Open: Unable to find transport for %s\n",
	       protocol);

	free (protocol);
	free (host);
	free (port);
	return NULL;
    }

    /* Open the transport */

    switch (type)
    {
    case XTRANS_OPEN_COTS_CLIENT:
#ifdef TRANS_CLIENT
	ciptr = thistrans->OpenCOTSClient(thistrans, protocol, host, port);
#endif /* TRANS_CLIENT */
	break;
    case XTRANS_OPEN_COTS_SERVER:
#ifdef TRANS_SERVER
	ciptr = thistrans->OpenCOTSServer(thistrans, protocol, host, port);
#endif /* TRANS_SERVER */
	break;
    default:
	prmsg (1,"Open: Unknown Open type %d\n", type);
    }

    if (ciptr == NULL)
    {
	if (!(thistrans->flags & TRANS_DISABLED))
	{
	    prmsg (1,"Open: transport open failed for %s/%s:%s\n",
	           protocol, host, port);
	}
	free (protocol);
	free (host);
	free (port);
	return NULL;
    }

    ciptr->transptr = thistrans;
    ciptr->port = port;			/* We need this for TRANS(Reopen) */

    free (protocol);
    free (host);

    return ciptr;
}


#ifdef TRANS_REOPEN

/*
 * We might want to create an XtransConnInfo object based on a previously
 * opened connection.  For example, the font server may clone itself and
 * pass file descriptors to the parent.
 */

static XtransConnInfo
TRANS(Reopen) (int type, int trans_id, int fd, const char *port)

{
    XtransConnInfo	ciptr = NULL;
    Xtransport		*thistrans = NULL;
    char		*save_port;
    int			i;

    prmsg (2,"Reopen(%d,%d,%s)\n", trans_id, fd, port);

    /* Determine the transport type */

    for (i = 0; i < NUMTRANS; i++)
	if (Xtransports[i].transport_id == trans_id)
	{
	    thistrans = Xtransports[i].transport;
	    break;
	}

    if (thistrans == NULL)
    {
	prmsg (1,"Reopen: Unable to find transport id %d\n",
	       trans_id);

	return NULL;
    }

    if ((save_port = strdup (port)) == NULL)
    {
	prmsg (1,"Reopen: Unable to malloc port string\n");

	return NULL;
    }

    /* Get a new XtransConnInfo object */

    switch (type)
    {
    case XTRANS_OPEN_COTS_SERVER:
	ciptr = thistrans->ReopenCOTSServer(thistrans, fd, port);
	break;
    default:
	prmsg (1,"Reopen: Bad Open type %d\n", type);
    }

    if (ciptr == NULL)
    {
	prmsg (1,"Reopen: transport open failed\n");
	free (save_port);
	return NULL;
    }

    ciptr->transptr = thistrans;
    ciptr->port = save_port;

    return ciptr;
}

#endif /* TRANS_REOPEN */



/*
 * These are the public interfaces to this Transport interface.
 * These are the only functions that should have knowledge of the transport
 * table.
 */

#ifdef TRANS_CLIENT

XtransConnInfo
TRANS(OpenCOTSClient) (const char *address)

{
    prmsg (2,"OpenCOTSClient(%s)\n", address);
    return TRANS(Open) (XTRANS_OPEN_COTS_CLIENT, address);
}

#endif /* TRANS_CLIENT */


#ifdef TRANS_SERVER

XtransConnInfo
TRANS(OpenCOTSServer) (const char *address)

{
    prmsg (2,"OpenCOTSServer(%s)\n", address);
    return TRANS(Open) (XTRANS_OPEN_COTS_SERVER, address);
}

#endif /* TRANS_SERVER */


#ifdef TRANS_REOPEN

XtransConnInfo
TRANS(ReopenCOTSServer) (int trans_id, int fd, const char *port)

{
    prmsg (2,"ReopenCOTSServer(%d, %d, %s)\n", trans_id, fd, port);
    return TRANS(Reopen) (XTRANS_OPEN_COTS_SERVER, trans_id, fd, port);
}

int
TRANS(GetReopenInfo) (XtransConnInfo ciptr,
		      int *trans_id, int *fd, char **port)

{
    int i;

    for (i = 0; i < NUMTRANS; i++)
	if (Xtransports[i].transport == ciptr->transptr)
	{
	    *trans_id = Xtransports[i].transport_id;
	    *fd = ciptr->fd;

	    if ((*port = strdup (ciptr->port)) == NULL)
		return 0;
	    else
		return 1;
	}

    return 0;
}

#endif /* TRANS_REOPEN */


int
TRANS(SetOption) (XtransConnInfo ciptr, int option, int arg)

{
    int	fd = ciptr->fd;
    int	ret = 0;

    prmsg (2,"SetOption(%d,%d,%d)\n", fd, option, arg);

    /*
     * For now, all transport type use the same stuff for setting options.
     * As long as this is true, we can put the common code here. Once a more
     * complicated transport such as shared memory or an OSI implementation
     * that uses the session and application libraries is implemented, this
     * code may have to move to a transport dependent function.
     *
     * ret = ciptr->transptr->SetOption (ciptr, option, arg);
     */

    switch (option)
    {
    case TRANS_NONBLOCKING:
	switch (arg)
	{
	case 0:
	    /* Set to blocking mode */
	    break;
	case 1: /* Set to non-blocking mode */

#if defined(O_NONBLOCK) && !defined(SCO325)
	    ret = fcntl (fd, F_GETFL, 0);
	    if (ret != -1)
		ret = fcntl (fd, F_SETFL, ret | O_NONBLOCK);
#else
#ifdef FIOSNBIO
	{
	    int arg;
	    arg = 1;
	    ret = ioctl (fd, FIOSNBIO, &arg);
	}
#else
#if defined(WIN32)
	{
#ifdef WIN32
	    u_long arg;
#else
	    int arg;
#endif
	    arg = 1;
/* IBM TCP/IP understands this option too well: it causes TRANS(Read) to fail
 * eventually with EWOULDBLOCK */
	    ret = ioctl (fd, FIONBIO, &arg);
	}
#else
	    ret = fcntl (fd, F_GETFL, 0);
#ifdef FNDELAY
	    ret = fcntl (fd, F_SETFL, ret | FNDELAY);
#else
	    ret = fcntl (fd, F_SETFL, ret | O_NDELAY);
#endif
#endif /* AIXV3  || uniosu */
#endif /* FIOSNBIO */
#endif /* O_NONBLOCK */
	    break;
	default:
	    /* Unknown option */
	    break;
	}
	break;
    case TRANS_CLOSEONEXEC:
#ifdef F_SETFD
#ifdef FD_CLOEXEC
	ret = fcntl (fd, F_SETFD, FD_CLOEXEC);
#else
	ret = fcntl (fd, F_SETFD, 1);
#endif /* FD_CLOEXEC */
#endif /* F_SETFD */
	break;
    }

    return ret;
}

#ifdef TRANS_SERVER

int
TRANS(CreateListener) (XtransConnInfo ciptr, const char *port, unsigned int flags)

{
    return ciptr->transptr->CreateListener (ciptr, port, flags);
}

int
TRANS(Received) (const char * protocol)

{
   Xtransport *trans;
   int i = 0, ret = 0;

   prmsg (5, "Received(%s)\n", protocol);

   if ((trans = TRANS(SelectTransport)(protocol)) == NULL)
   {
	prmsg (1,"Received: unable to find transport: %s\n",
	       protocol);

	return -1;
   }
   if (trans->flags & TRANS_ALIAS) {
       if (trans->nolisten)
	   while (trans->nolisten[i]) {
	       ret |= TRANS(Received)(trans->nolisten[i]);
	       i++;
       }
   }

   trans->flags |= TRANS_RECEIVED;
   return ret;
}

int
TRANS(NoListen) (const char * protocol)

{
   Xtransport *trans;
   int i = 0, ret = 0;

   if ((trans = TRANS(SelectTransport)(protocol)) == NULL)
   {
	prmsg (1,"TransNoListen: unable to find transport: %s\n",
	       protocol);

	return -1;
   }
   if (trans->flags & TRANS_ALIAS) {
       if (trans->nolisten)
	   while (trans->nolisten[i]) {
	       ret |= TRANS(NoListen)(trans->nolisten[i]);
	       i++;
       }
   }

   trans->flags |= TRANS_NOLISTEN;
   return ret;
}

int
TRANS(Listen) (const char * protocol)
{
   Xtransport *trans;
   int i = 0, ret = 0;

   if ((trans = TRANS(SelectTransport)(protocol)) == NULL)
   {
	prmsg (1,"TransListen: unable to find transport: %s\n",
	       protocol);

	return -1;
   }
   if (trans->flags & TRANS_ALIAS) {
       if (trans->nolisten)
	   while (trans->nolisten[i]) {
	       ret |= TRANS(Listen)(trans->nolisten[i]);
	       i++;
       }
   }

   trans->flags &= ~TRANS_NOLISTEN;
   return ret;
}

int
TRANS(IsListening) (const char * protocol)
{
   Xtransport *trans;

   if ((trans = TRANS(SelectTransport)(protocol)) == NULL)
   {
	prmsg (1,"TransIsListening: unable to find transport: %s\n",
	       protocol);

	return 0;
   }

   return !(trans->flags & TRANS_NOLISTEN);
}

int
TRANS(ResetListener) (XtransConnInfo ciptr)

{
    if (ciptr->transptr->ResetListener)
	return ciptr->transptr->ResetListener (ciptr);
    else
	return TRANS_RESET_NOOP;
}


XtransConnInfo
TRANS(Accept) (XtransConnInfo ciptr, int *status)

{
    XtransConnInfo	newciptr;

    prmsg (2,"Accept(%d)\n", ciptr->fd);

    newciptr = ciptr->transptr->Accept (ciptr, status);

    if (newciptr)
	newciptr->transptr = ciptr->transptr;

    return newciptr;
}

#endif /* TRANS_SERVER */


#ifdef TRANS_CLIENT

int
TRANS(Connect) (XtransConnInfo ciptr, const char *address)

{
    char	*protocol;
    char	*host;
    char	*port;
    int		ret;

    prmsg (2,"Connect(%d,%s)\n", ciptr->fd, address);

    if (TRANS(ParseAddress) (address, &protocol, &host, &port) == 0)
    {
	prmsg (1,"Connect: Unable to Parse address %s\n",
	       address);
	return -1;
    }

#ifdef HAVE_LAUNCHD
    if (!host) host=strdup("");
#endif

    if (!port || !*port)
    {
	prmsg (1,"Connect: Missing port specification in %s\n",
	      address);
	if (protocol) free (protocol);
	if (host) free (host);
	return -1;
    }

    ret = ciptr->transptr->Connect (ciptr, host, port);

    if (protocol) free (protocol);
    if (host) free (host);
    if (port) free (port);

    return ret;
}

#endif /* TRANS_CLIENT */


int
TRANS(BytesReadable) (XtransConnInfo ciptr, BytesReadable_t *pend)

{
    return ciptr->transptr->BytesReadable (ciptr, pend);
}

int
TRANS(Read) (XtransConnInfo ciptr, char *buf, int size)

{
    return ciptr->transptr->Read (ciptr, buf, size);
}

int
TRANS(Write) (XtransConnInfo ciptr, char *buf, int size)

{
    return ciptr->transptr->Write (ciptr, buf, size);
}

int
TRANS(Readv) (XtransConnInfo ciptr, struct iovec *buf, int size)

{
    return ciptr->transptr->Readv (ciptr, buf, size);
}

int
TRANS(Writev) (XtransConnInfo ciptr, struct iovec *buf, int size)

{
    return ciptr->transptr->Writev (ciptr, buf, size);
}

#if XTRANS_SEND_FDS
int
TRANS(SendFd) (XtransConnInfo ciptr, int fd, int do_close)
{
    return ciptr->transptr->SendFd(ciptr, fd, do_close);
}

int
TRANS(RecvFd) (XtransConnInfo ciptr)
{
    return ciptr->transptr->RecvFd(ciptr);
}
#endif

int
TRANS(Disconnect) (XtransConnInfo ciptr)

{
    return ciptr->transptr->Disconnect (ciptr);
}

int
TRANS(Close) (XtransConnInfo ciptr)

{
    int ret;

    prmsg (2,"Close(%d)\n", ciptr->fd);

    ret = ciptr->transptr->Close (ciptr);

    TRANS(FreeConnInfo) (ciptr);

    return ret;
}

int
TRANS(CloseForCloning) (XtransConnInfo ciptr)

{
    int ret;

    prmsg (2,"CloseForCloning(%d)\n", ciptr->fd);

    ret = ciptr->transptr->CloseForCloning (ciptr);

    TRANS(FreeConnInfo) (ciptr);

    return ret;
}

int
TRANS(IsLocal) (XtransConnInfo ciptr)

{
    return (ciptr->family == AF_UNIX);
}

int
TRANS(GetPeerAddr) (XtransConnInfo ciptr, int *familyp, int *addrlenp,
		    Xtransaddr **addrp)

{
    prmsg (2,"GetPeerAddr(%d)\n", ciptr->fd);

    *familyp = ciptr->family;
    *addrlenp = ciptr->peeraddrlen;

    if ((*addrp = malloc (ciptr->peeraddrlen)) == NULL)
    {
	prmsg (1,"GetPeerAddr: malloc failed\n");
	return -1;
    }
    memcpy(*addrp, ciptr->peeraddr, ciptr->peeraddrlen);

    return 0;
}


int
TRANS(GetConnectionNumber) (XtransConnInfo ciptr)

{
    return ciptr->fd;
}


/*
 * These functions are really utility functions, but they require knowledge
 * of the internal data structures, so they have to be part of the Transport
 * Independant API.
 */

#ifdef TRANS_SERVER

static int
complete_network_count (void)

{
    int count = 0;
    int found_local = 0;
    int i;

    /*
     * For a complete network, we only need one LOCALCONN transport to work
     */

    for (i = 0; i < NUMTRANS; i++)
    {
	if (Xtransports[i].transport->flags & TRANS_ALIAS
   	 || Xtransports[i].transport->flags & TRANS_NOLISTEN)
	    continue;

	if (Xtransports[i].transport->flags & TRANS_LOCAL)
	    found_local = 1;
	else
	    count++;
    }

    return (count + found_local);
}


static int
receive_listening_fds(const char* port, XtransConnInfo* temp_ciptrs,
                      int* count_ret)

{
#ifdef HAVE_SYSTEMD_DAEMON
    XtransConnInfo ciptr;
    int i, systemd_listen_fds;

    systemd_listen_fds = sd_listen_fds(1);
    if (systemd_listen_fds < 0)
    {
        prmsg (1, "receive_listening_fds: sd_listen_fds error: %s\n",
               strerror(-systemd_listen_fds));
        return -1;
    }

    for (i = 0; i < systemd_listen_fds && *count_ret < NUMTRANS; i++)
    {
        struct sockaddr_storage a;
        int ti;
        const char* tn;
        socklen_t al;

        al = sizeof(a);
        if (getsockname(i + SD_LISTEN_FDS_START, (struct sockaddr*)&a, &al) < 0) {
            prmsg (1, "receive_listening_fds: getsockname error: %s\n",
                   strerror(errno));
            return -1;
        }

        switch (a.ss_family)
        {
        case AF_UNIX:
            ti = TRANS_SOCKET_UNIX_INDEX;
            if (*((struct sockaddr_un*)&a)->sun_path == '\0' &&
                al > sizeof(sa_family_t))
                tn = "local";
            else
                tn = "unix";
            break;
        case AF_INET:
            ti = TRANS_SOCKET_INET_INDEX;
            tn = "inet";
            break;
#if defined(IPv6) && defined(AF_INET6)
        case AF_INET6:
            ti = TRANS_SOCKET_INET6_INDEX;
            tn = "inet6";
            break;
#endif /* IPv6 */
        default:
            prmsg (1, "receive_listening_fds:"
                   "Got unknown socket address family\n");
            return -1;
        }

        ciptr = TRANS(ReopenCOTSServer)(ti, i + SD_LISTEN_FDS_START, port);
        if (!ciptr)
        {
            prmsg (1, "receive_listening_fds:"
                   "Got NULL while trying to reopen socket received from systemd.\n");
            return -1;
        }

        prmsg (5, "receive_listening_fds: received listener for %s, %d\n",
               tn, ciptr->fd);
        temp_ciptrs[(*count_ret)++] = ciptr;
        TRANS(Received)(tn);
    }
#endif /* HAVE_SYSTEMD_DAEMON */
    return 0;
}

#ifdef XQUARTZ_EXPORTS_LAUNCHD_FD
extern int xquartz_launchd_fd;
#endif

int
TRANS(MakeAllCOTSServerListeners) (const char *port, int *partial,
                                   int *count_ret, XtransConnInfo **ciptrs_ret)

{
    char		buffer[256]; /* ??? What size ?? */
    XtransConnInfo	ciptr, temp_ciptrs[NUMTRANS];
    int			status, i, j;

#if defined(IPv6) && defined(AF_INET6)
    int		ipv6_succ = 0;
#endif
    prmsg (2,"MakeAllCOTSServerListeners(%s,%p)\n",
	   port ? port : "NULL", ciptrs_ret);

    *count_ret = 0;

#ifdef XQUARTZ_EXPORTS_LAUNCHD_FD
    fprintf(stderr, "Launchd socket fd: %d\n", xquartz_launchd_fd);
    if(xquartz_launchd_fd != -1) {
        if((ciptr = TRANS(ReopenCOTSServer(TRANS_SOCKET_LOCAL_INDEX,
                                           xquartz_launchd_fd, getenv("DISPLAY"))))==NULL)
            fprintf(stderr,"Got NULL while trying to Reopen launchd port\n");
        else
            temp_ciptrs[(*count_ret)++] = ciptr;
    }
#endif

    if (receive_listening_fds(port, temp_ciptrs, count_ret) < 0)
	return -1;

    for (i = 0; i < NUMTRANS; i++)
    {
	Xtransport *trans = Xtransports[i].transport;
	unsigned int flags = 0;

	if (trans->flags&TRANS_ALIAS || trans->flags&TRANS_NOLISTEN ||
	    trans->flags&TRANS_RECEIVED)
	    continue;

	snprintf(buffer, sizeof(buffer), "%s/:%s",
		 trans->TransName, port ? port : "");

	prmsg (5,"MakeAllCOTSServerListeners: opening %s\n",
	       buffer);

	if ((ciptr = TRANS(OpenCOTSServer(buffer))) == NULL)
	{
	    if (trans->flags & TRANS_DISABLED)
		continue;

	    prmsg (1,
	  "MakeAllCOTSServerListeners: failed to open listener for %s\n",
		  trans->TransName);
	    continue;
	}
#if defined(IPv6) && defined(AF_INET6)
		if ((Xtransports[i].transport_id == TRANS_SOCKET_INET_INDEX
		     && ipv6_succ))
		    flags |= ADDR_IN_USE_ALLOWED;
#endif

	if ((status = TRANS(CreateListener (ciptr, port, flags))) < 0)
	{
	    if (status == TRANS_ADDR_IN_USE)
	    {
		/*
		 * We failed to bind to the specified address because the
		 * address is in use.  It must be that a server is already
		 * running at this address, and this function should fail.
		 */

		prmsg (1,
		"MakeAllCOTSServerListeners: server already running\n");

		for (j = 0; j < *count_ret; j++)
		    TRANS(Close) (temp_ciptrs[j]);

		*count_ret = 0;
		*ciptrs_ret = NULL;
		*partial = 0;
		return -1;
	    }
	    else
	    {
		prmsg (1,
	"MakeAllCOTSServerListeners: failed to create listener for %s\n",
		  trans->TransName);

		continue;
	    }
	}

#if defined(IPv6) && defined(AF_INET6)
	if (Xtransports[i].transport_id == TRANS_SOCKET_INET6_INDEX)
	    ipv6_succ = 1;
#endif

	prmsg (5,
	      "MakeAllCOTSServerListeners: opened listener for %s, %d\n",
	      trans->TransName, ciptr->fd);

	temp_ciptrs[*count_ret] = ciptr;
	(*count_ret)++;
    }

    *partial = (*count_ret < complete_network_count());

    prmsg (5,
     "MakeAllCOTSServerListeners: partial=%d, actual=%d, complete=%d \n",
	*partial, *count_ret, complete_network_count());

    if (*count_ret > 0)
    {
	if ((*ciptrs_ret = malloc (
	    *count_ret * sizeof (XtransConnInfo))) == NULL)
	{
	    return -1;
	}

	for (i = 0; i < *count_ret; i++)
	{
	    (*ciptrs_ret)[i] = temp_ciptrs[i];
	}
    }
    else
	*ciptrs_ret = NULL;

    return 0;
}

#endif /* TRANS_SERVER */



/*
 * These routines are not part of the X Transport Interface, but they
 * may be used by it.
 */


#ifdef WIN32

/*
 * emulate readv
 */

static int TRANS(ReadV) (XtransConnInfo ciptr, struct iovec *iov, int iovcnt)

{
    int i, len, total;
    char *base;

    ESET(0);
    for (i = 0, total = 0;  i < iovcnt;  i++, iov++) {
	len = iov->iov_len;
	base = iov->iov_base;
	while (len > 0) {
	    register int nbytes;
	    nbytes = TRANS(Read) (ciptr, base, len);
	    if (nbytes < 0 && total == 0)  return -1;
	    if (nbytes <= 0)  return total;
	    ESET(0);
	    len   -= nbytes;
	    total += nbytes;
	    base  += nbytes;
	}
    }
    return total;
}


/*
 * emulate writev
 */

static int TRANS(WriteV) (XtransConnInfo ciptr, struct iovec *iov, int iovcnt)

{
    int i, len, total;
    char *base;

    ESET(0);
    for (i = 0, total = 0;  i < iovcnt;  i++, iov++) {
	len = iov->iov_len;
	base = iov->iov_base;
	while (len > 0) {
	    register int nbytes;
	    nbytes = TRANS(Write) (ciptr, base, len);
	    if (nbytes < 0 && total == 0)  return -1;
	    if (nbytes <= 0)  return total;
	    ESET(0);
	    len   -= nbytes;
	    total += nbytes;
	    base  += nbytes;
	}
    }
    return total;
}

#endif /* WIN32 */


#if defined(_POSIX_SOURCE) || defined(USG) || defined(SVR4) || defined(__SVR4) || defined(__SCO__)
#ifndef NEED_UTSNAME
#define NEED_UTSNAME
#endif
#include <sys/utsname.h>
#endif

/*
 * TRANS(GetHostname) - similar to gethostname but allows special processing.
 */

int TRANS(GetHostname) (char *buf, int maxlen)

{
    int len;

#ifdef NEED_UTSNAME
    struct utsname name;

    uname (&name);
    len = strlen (name.nodename);
    if (len >= maxlen) len = maxlen - 1;
    strncpy (buf, name.nodename, len);
    buf[len] = '\0';
#else
    buf[0] = '\0';
    (void) gethostname (buf, maxlen);
    buf [maxlen - 1] = '\0';
    len = strlen(buf);
#endif /* NEED_UTSNAME */
    return len;
}
