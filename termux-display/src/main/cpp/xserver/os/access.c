/***********************************************************

Copyright 1987, 1998  The Open Group

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

X Window System is a trademark of The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * Copyright (c) 2004, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <X11/Xtrans/Xtrans.h>
#include <X11/Xauth.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include <errno.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <ctype.h>

#ifndef NO_LOCAL_CLIENT_CRED
#include <pwd.h>
#endif

#if defined(TCPCONN)
#include <netinet/in.h>
#endif                          /* TCPCONN || STREAMSCONN */

#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#ifdef __sun
#include <zone.h>
#endif
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#if defined(SVR4) ||  (defined(SYSV) && defined(__i386__)) || defined(__GNU__)
#include <sys/utsname.h>
#endif
#if defined(SYSV) &&  defined(__i386__)
#include <sys/stream.h>
#endif
#ifdef __GNU__
#undef SIOCGIFCONF
#include <netdb.h>
#else                           /*!__GNU__ */
#include <net/if.h>
#endif /*__GNU__ */

#ifdef SVR4
#include <sys/sockio.h>
#include <sys/stropts.h>
#endif

#include <netdb.h>

#ifdef CSRG_BASED
#include <sys/param.h>
#if (BSD >= 199103)
#define VARIABLE_IFREQ
#endif
#endif

#ifdef BSD44SOCKETS
#ifndef VARIABLE_IFREQ
#define VARIABLE_IFREQ
#endif
#endif

#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

/* Solaris provides an extended interface SIOCGLIFCONF.  Other systems
 * may have this as well, but the code has only been tested on Solaris
 * so far, so we only enable it there.  Other platforms may be added as
 * needed.
 *
 * Test for Solaris commented out  --  TSI @ UQV  2003.06.13
 */
#ifdef SIOCGLIFCONF
/* #if defined(__sun) */
#define USE_SIOCGLIFCONF
/* #endif */
#endif

#if defined(IPv6) && defined(AF_INET6)
#include <arpa/inet.h>
#endif

#endif                          /* WIN32 */

#if !defined(WIN32) || defined(__CYGWIN__)
#include <libgen.h>
#endif

#define X_INCLUDE_NETDB_H
#include <X11/Xos_r.h>

#include "dixstruct.h"
#include "osdep.h"

#include "xace.h"

Bool defeatAccessControl = FALSE;

#define addrEqual(fam, address, length, host) \
			 ((fam) == (host)->family &&\
			  (length) == (host)->len &&\
			  !memcmp (address, (host)->addr, length))

static int ConvertAddr(struct sockaddr * /*saddr */ ,
                       int * /*len */ ,
                       void ** /*addr */ );

static int CheckAddr(int /*family */ ,
                     const void * /*pAddr */ ,
                     unsigned /*length */ );

static Bool NewHost(int /*family */ ,
                    const void * /*addr */ ,
                    int /*len */ ,
                    int /* addingLocalHosts */ );

/* XFree86 bug #156: To keep track of which hosts were explicitly requested in
   /etc/X<display>.hosts, we've added a requested field to the HOST struct,
   and a LocalHostRequested variable.  These default to FALSE, but are set
   to TRUE in ResetHosts when reading in /etc/X<display>.hosts.  They are
   checked in DisableLocalHost(), which is called to disable the default
   local host entries when stronger authentication is turned on. */

typedef struct _host {
    short family;
    short len;
    unsigned char *addr;
    struct _host *next;
    int requested;
} HOST;

#define MakeHost(h,l)	(h)=malloc(sizeof *(h)+(l));\
			if (h) { \
			   (h)->addr=(unsigned char *) ((h) + 1);\
			   (h)->requested = FALSE; \
			}
#define FreeHost(h)	free(h)
static HOST *selfhosts = NULL;
static HOST *validhosts = NULL;
static int AccessEnabled = TRUE;
static int LocalHostEnabled = FALSE;
static int LocalHostRequested = FALSE;
static int UsingXdmcp = FALSE;

static enum {
    LOCAL_ACCESS_SCOPE_HOST = 0,
#ifndef NO_LOCAL_CLIENT_CRED
    LOCAL_ACCESS_SCOPE_USER,
#endif
} LocalAccessScope;

/* FamilyServerInterpreted implementation */
static Bool siAddrMatch(int family, void *addr, int len, HOST * host,
                        ClientPtr client);
static int siCheckAddr(const char *addrString, int length);
static void siTypesInitialize(void);

/*
 * called when authorization is not enabled to add the
 * local host to the access list
 */

void
EnableLocalAccess(void)
{
    switch (LocalAccessScope) {
        case LOCAL_ACCESS_SCOPE_HOST:
            EnableLocalHost();
            break;
#ifndef NO_LOCAL_CLIENT_CRED
        case LOCAL_ACCESS_SCOPE_USER:
            EnableLocalUser();
            break;
#endif
    }
}

void
EnableLocalHost(void)
{
    if (!UsingXdmcp) {
        LocalHostEnabled = TRUE;
        AddLocalHosts();
    }
}

/*
 * called when authorization is enabled to keep us secure
 */
void
DisableLocalAccess(void)
{
    switch (LocalAccessScope) {
        case LOCAL_ACCESS_SCOPE_HOST:
            DisableLocalHost();
            break;
#ifndef NO_LOCAL_CLIENT_CRED
        case LOCAL_ACCESS_SCOPE_USER:
            DisableLocalUser();
            break;
#endif
    }
}

void
DisableLocalHost(void)
{
    HOST *self;

    if (!LocalHostRequested)    /* Fix for XFree86 bug #156 */
        LocalHostEnabled = FALSE;
    for (self = selfhosts; self; self = self->next) {
        if (!self->requested)   /* Fix for XFree86 bug #156 */
            (void) RemoveHost((ClientPtr) NULL, self->family, self->len,
                              (void *) self->addr);
    }
}

#ifndef NO_LOCAL_CLIENT_CRED
static int GetLocalUserAddr(char **addr)
{
    static const char *type = "localuser";
    static const char delimiter = '\0';
    static const char *value;
    struct passwd *pw;
    int length = -1;

    pw = getpwuid(getuid());

    if (pw == NULL || pw->pw_name == NULL)
        goto out;

    value = pw->pw_name;

    length = asprintf(addr, "%s%c%s", type, delimiter, value);

    if (length == -1) {
        goto out;
    }

    /* Trailing NUL */
    length++;

out:
    return length;
}

void
EnableLocalUser(void)
{
    char *addr = NULL;
    int length = -1;

    length = GetLocalUserAddr(&addr);

    if (length == -1)
        return;

    NewHost(FamilyServerInterpreted, addr, length, TRUE);

    free(addr);
}

void
DisableLocalUser(void)
{
    char *addr = NULL;
    int length = -1;

    length = GetLocalUserAddr(&addr);

    if (length == -1)
        return;

    RemoveHost(NULL, FamilyServerInterpreted, length, addr);

    free(addr);
}

void
LocalAccessScopeUser(void)
{
    LocalAccessScope = LOCAL_ACCESS_SCOPE_USER;
}
#endif

/*
 * called at init time when XDMCP will be used; xdmcp always
 * adds local hosts manually when needed
 */

void
AccessUsingXdmcp(void)
{
    UsingXdmcp = TRUE;
    LocalHostEnabled = FALSE;
}

#if  defined(SVR4) && !defined(__sun)  && defined(SIOCGIFCONF) && !defined(USE_SIOCGLIFCONF)

/* Deal with different SIOCGIFCONF ioctl semantics on these OSs */

static int
ifioctl(int fd, int cmd, char *arg)
{
    struct strioctl ioc;
    int ret;

    memset((char *) &ioc, 0, sizeof(ioc));
    ioc.ic_cmd = cmd;
    ioc.ic_timout = 0;
    if (cmd == SIOCGIFCONF) {
        ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
        ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
    }
    else {
        ioc.ic_len = sizeof(struct ifreq);
        ioc.ic_dp = arg;
    }
    ret = ioctl(fd, I_STR, (char *) &ioc);
    if (ret >= 0 && cmd == SIOCGIFCONF)
#ifdef SVR4
        ((struct ifconf *) arg)->ifc_len = ioc.ic_len;
#endif
    return ret;
}
#else
#define ifioctl ioctl
#endif

/*
 * DefineSelf (fd):
 *
 * Define this host for access control.  Find all the hosts the OS knows about
 * for this fd and add them to the selfhosts list.
 */

#if !defined(SIOCGIFCONF)
void
DefineSelf(int fd)
{
#if !defined(TCPCONN) && !defined(UNIXCONN)
    return;
#else
    register int n;
    int len;
    caddr_t addr;
    int family;
    register HOST *host;

#ifndef WIN32
    struct utsname name;
#else
    struct {
        char nodename[512];
    } name;
#endif

    register struct hostent *hp;

    union {
        struct sockaddr sa;
        struct sockaddr_in in;
#if defined(IPv6) && defined(AF_INET6)
        struct sockaddr_in6 in6;
#endif
    } saddr;

    struct sockaddr_in *inetaddr;
    struct sockaddr_in6 *inet6addr;
    struct sockaddr_in broad_addr;

#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
    _Xgethostbynameparams hparams;
#endif

    /* Why not use gethostname()?  Well, at least on my system, I've had to
     * make an ugly kernel patch to get a name longer than 8 characters, and
     * uname() lets me access to the whole string (it smashes release, you
     * see), whereas gethostname() kindly truncates it for me.
     */
#ifndef WIN32
    uname(&name);
#else
    gethostname(name.nodename, sizeof(name.nodename));
#endif

    hp = _XGethostbyname(name.nodename, hparams);
    if (hp != NULL) {
        saddr.sa.sa_family = hp->h_addrtype;
        switch (hp->h_addrtype) {
        case AF_INET:
            inetaddr = (struct sockaddr_in *) (&(saddr.sa));
            memcpy(&(inetaddr->sin_addr), hp->h_addr, hp->h_length);
            len = sizeof(saddr.sa);
            break;
#if defined(IPv6) && defined(AF_INET6)
        case AF_INET6:
            inet6addr = (struct sockaddr_in6 *) (&(saddr.sa));
            memcpy(&(inet6addr->sin6_addr), hp->h_addr, hp->h_length);
            len = sizeof(saddr.in6);
            break;
#endif
        default:
            goto DefineLocalHost;
        }
        family = ConvertAddr(&(saddr.sa), &len, (void **) &addr);
        if (family != -1 && family != FamilyLocal) {
            for (host = selfhosts;
                 host && !addrEqual(family, addr, len, host);
                 host = host->next);
            if (!host) {
                /* add this host to the host list.      */
                MakeHost(host, len)
                    if (host) {
                    host->family = family;
                    host->len = len;
                    memcpy(host->addr, addr, len);
                    host->next = selfhosts;
                    selfhosts = host;
                }
#ifdef XDMCP
                /*
                 *  If this is an Internet Address, but not the localhost
                 *  address (127.0.0.1), nor the bogus address (0.0.0.0),
                 *  register it.
                 */
                if (family == FamilyInternet &&
                    !(len == 4 &&
                      ((addr[0] == 127) ||
                       (addr[0] == 0 && addr[1] == 0 &&
                        addr[2] == 0 && addr[3] == 0)))
                    ) {
                    XdmcpRegisterConnection(family, (char *) addr, len);
                    broad_addr = *inetaddr;
                    ((struct sockaddr_in *) &broad_addr)->sin_addr.s_addr =
                        htonl(INADDR_BROADCAST);
                    XdmcpRegisterBroadcastAddress((struct sockaddr_in *)
                                                  &broad_addr);
                }
#if defined(IPv6) && defined(AF_INET6)
                else if (family == FamilyInternet6 &&
                         !(IN6_IS_ADDR_LOOPBACK((struct in6_addr *) addr))) {
                    XdmcpRegisterConnection(family, (char *) addr, len);
                }
#endif

#endif                          /* XDMCP */
            }
        }
    }
    /*
     * now add a host of family FamilyLocalHost...
     */
 DefineLocalHost:
    for (host = selfhosts;
         host && !addrEqual(FamilyLocalHost, "", 0, host); host = host->next);
    if (!host) {
        MakeHost(host, 0);
        if (host) {
            host->family = FamilyLocalHost;
            host->len = 0;
            /* Nothing to store in host->addr */
            host->next = selfhosts;
            selfhosts = host;
        }
    }
#endif                          /* !TCPCONN && !STREAMSCONN && !UNIXCONN */
}

#else

#ifdef USE_SIOCGLIFCONF
#define ifr_type    struct lifreq
#else
#define ifr_type    struct ifreq
#endif

#ifdef VARIABLE_IFREQ
#define ifr_size(p) (sizeof (struct ifreq) + \
		     (p->ifr_addr.sa_len > sizeof (p->ifr_addr) ? \
		      p->ifr_addr.sa_len - sizeof (p->ifr_addr) : 0))
#define ifraddr_size(a) (a.sa_len)
#else
#define ifr_size(p) (sizeof (ifr_type))
#define ifraddr_size(a) (sizeof (a))
#endif

#if defined(IPv6) && defined(AF_INET6)
static void
in6_fillscopeid(struct sockaddr_in6 *sin6)
{
#if defined(__KAME__)
    if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) && sin6->sin6_scope_id == 0) {
        sin6->sin6_scope_id =
            ntohs(*(u_int16_t *) &sin6->sin6_addr.s6_addr[2]);
        sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
    }
#endif
}
#endif

void
DefineSelf(int fd)
{
#ifndef HAVE_GETIFADDRS
    char *cp, *cplim;

#ifdef USE_SIOCGLIFCONF
    struct sockaddr_storage buf[16];
    struct lifconf ifc;
    register struct lifreq *ifr;

#ifdef SIOCGLIFNUM
    struct lifnum ifn;
#endif
#else                           /* !USE_SIOCGLIFCONF */
    char buf[2048];
    struct ifconf ifc;
    register struct ifreq *ifr;
#endif
    void *bufptr = buf;
#else                           /* HAVE_GETIFADDRS */
    struct ifaddrs *ifap, *ifr;
#endif
    int len;
    unsigned char *addr;
    int family;
    register HOST *host;

#ifndef HAVE_GETIFADDRS

    len = sizeof(buf);

#ifdef USE_SIOCGLIFCONF

#ifdef SIOCGLIFNUM
    ifn.lifn_family = AF_UNSPEC;
    ifn.lifn_flags = 0;
    if (ioctl(fd, SIOCGLIFNUM, (char *) &ifn) < 0)
        ErrorF("Getting interface count: %s\n", strerror(errno));
    if (len < (ifn.lifn_count * sizeof(struct lifreq))) {
        len = ifn.lifn_count * sizeof(struct lifreq);
        bufptr = malloc(len);
    }
#endif

    ifc.lifc_family = AF_UNSPEC;
    ifc.lifc_flags = 0;
    ifc.lifc_len = len;
    ifc.lifc_buf = bufptr;

#define IFC_IOCTL_REQ SIOCGLIFCONF
#define IFC_IFC_REQ ifc.lifc_req
#define IFC_IFC_LEN ifc.lifc_len
#define IFR_IFR_ADDR ifr->lifr_addr
#define IFR_IFR_NAME ifr->lifr_name

#else                           /* Use SIOCGIFCONF */
    ifc.ifc_len = len;
    ifc.ifc_buf = bufptr;

#define IFC_IOCTL_REQ SIOCGIFCONF
#define IFC_IFC_REQ ifc.ifc_req
#define IFC_IFC_LEN ifc.ifc_len
#define IFR_IFR_ADDR ifr->ifr_addr
#define IFR_IFR_NAME ifr->ifr_name
#endif

    if (ifioctl(fd, IFC_IOCTL_REQ, (void *) &ifc) < 0)
        ErrorF("Getting interface configuration (4): %s\n", strerror(errno));

    cplim = (char *) IFC_IFC_REQ + IFC_IFC_LEN;

    for (cp = (char *) IFC_IFC_REQ; cp < cplim; cp += ifr_size(ifr)) {
        ifr = (ifr_type *) cp;
        len = ifraddr_size(IFR_IFR_ADDR);
        family = ConvertAddr((struct sockaddr *) &IFR_IFR_ADDR,
                             &len, (void **) &addr);
        if (family == -1 || family == FamilyLocal)
            continue;
#if defined(IPv6) && defined(AF_INET6)
        if (family == FamilyInternet6)
            in6_fillscopeid((struct sockaddr_in6 *) &IFR_IFR_ADDR);
#endif
        for (host = selfhosts;
             host && !addrEqual(family, addr, len, host); host = host->next);
        if (host)
            continue;
        MakeHost(host, len)
            if (host) {
            host->family = family;
            host->len = len;
            memcpy(host->addr, addr, len);
            host->next = selfhosts;
            selfhosts = host;
        }
#ifdef XDMCP
        {
#ifdef USE_SIOCGLIFCONF
            struct sockaddr_storage broad_addr;
#else
            struct sockaddr broad_addr;
#endif

            /*
             * If this isn't an Internet Address, don't register it.
             */
            if (family != FamilyInternet
#if defined(IPv6) && defined(AF_INET6)
                && family != FamilyInternet6
#endif
                )
                continue;

            /*
             * ignore 'localhost' entries as they're not useful
             * on the other end of the wire
             */
            if (family == FamilyInternet &&
                addr[0] == 127 && addr[1] == 0 && addr[2] == 0 && addr[3] == 1)
                continue;
#if defined(IPv6) && defined(AF_INET6)
            else if (family == FamilyInternet6 &&
                     IN6_IS_ADDR_LOOPBACK((struct in6_addr *) addr))
                continue;
#endif

            /*
             * Ignore '0.0.0.0' entries as they are
             * returned by some OSes for unconfigured NICs but they are
             * not useful on the other end of the wire.
             */
            if (len == 4 &&
                addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0)
                continue;

            XdmcpRegisterConnection(family, (char *) addr, len);

#if defined(IPv6) && defined(AF_INET6)
            /* IPv6 doesn't support broadcasting, so we drop out here */
            if (family == FamilyInternet6)
                continue;
#endif

            broad_addr = IFR_IFR_ADDR;

            ((struct sockaddr_in *) &broad_addr)->sin_addr.s_addr =
                htonl(INADDR_BROADCAST);
#if defined(USE_SIOCGLIFCONF) && defined(SIOCGLIFBRDADDR)
            {
                struct lifreq broad_req;

                broad_req = *ifr;
                if (ioctl(fd, SIOCGLIFFLAGS, (char *) &broad_req) != -1 &&
                    (broad_req.lifr_flags & IFF_BROADCAST) &&
                    (broad_req.lifr_flags & IFF_UP)
                    ) {
                    broad_req = *ifr;
                    if (ioctl(fd, SIOCGLIFBRDADDR, &broad_req) != -1)
                        broad_addr = broad_req.lifr_broadaddr;
                    else
                        continue;
                }
                else
                    continue;
            }

#elif defined(SIOCGIFBRDADDR)
            {
                struct ifreq broad_req;

                broad_req = *ifr;
                if (ifioctl(fd, SIOCGIFFLAGS, (void *) &broad_req) != -1 &&
                    (broad_req.ifr_flags & IFF_BROADCAST) &&
                    (broad_req.ifr_flags & IFF_UP)
                    ) {
                    broad_req = *ifr;
                    if (ifioctl(fd, SIOCGIFBRDADDR, (void *) &broad_req) != -1)
                        broad_addr = broad_req.ifr_addr;
                    else
                        continue;
                }
                else
                    continue;
            }
#endif                          /* SIOCGIFBRDADDR */
            XdmcpRegisterBroadcastAddress((struct sockaddr_in *) &broad_addr);
        }
#endif                          /* XDMCP */
    }
    if (bufptr != buf)
        free(bufptr);
#else                           /* HAVE_GETIFADDRS */
    if (getifaddrs(&ifap) < 0) {
        ErrorF("Warning: getifaddrs returns %s\n", strerror(errno));
        return;
    }
    for (ifr = ifap; ifr != NULL; ifr = ifr->ifa_next) {
        if (!ifr->ifa_addr)
            continue;
        len = sizeof(*(ifr->ifa_addr));
        family = ConvertAddr((struct sockaddr *) ifr->ifa_addr, &len,
                             (void **) &addr);
        if (family == -1 || family == FamilyLocal)
            continue;
#if defined(IPv6) && defined(AF_INET6)
        if (family == FamilyInternet6)
            in6_fillscopeid((struct sockaddr_in6 *) ifr->ifa_addr);
#endif

        for (host = selfhosts;
             host != NULL && !addrEqual(family, addr, len, host);
             host = host->next);
        if (host != NULL)
            continue;
        MakeHost(host, len);
        if (host != NULL) {
            host->family = family;
            host->len = len;
            memcpy(host->addr, addr, len);
            host->next = selfhosts;
            selfhosts = host;
        }
#ifdef XDMCP
        {
            /*
             * If this isn't an Internet Address, don't register it.
             */
            if (family != FamilyInternet
#if defined(IPv6) && defined(AF_INET6)
                && family != FamilyInternet6
#endif
                )
                continue;

            /*
             * ignore 'localhost' entries as they're not useful
             * on the other end of the wire
             */
            if (ifr->ifa_flags & IFF_LOOPBACK)
                continue;

            if (family == FamilyInternet &&
                addr[0] == 127 && addr[1] == 0 && addr[2] == 0 && addr[3] == 1)
                continue;

            /*
             * Ignore '0.0.0.0' entries as they are
             * returned by some OSes for unconfigured NICs but they are
             * not useful on the other end of the wire.
             */
            if (len == 4 &&
                addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0)
                continue;
#if defined(IPv6) && defined(AF_INET6)
            else if (family == FamilyInternet6 &&
                     IN6_IS_ADDR_LOOPBACK((struct in6_addr *) addr))
                continue;
#endif
            XdmcpRegisterConnection(family, (char *) addr, len);
#if defined(IPv6) && defined(AF_INET6)
            if (family == FamilyInternet6)
                /* IPv6 doesn't support broadcasting, so we drop out here */
                continue;
#endif
            if ((ifr->ifa_flags & IFF_BROADCAST) &&
                (ifr->ifa_flags & IFF_UP) && ifr->ifa_broadaddr)
                XdmcpRegisterBroadcastAddress((struct sockaddr_in *) ifr->
                                              ifa_broadaddr);
            else
                continue;
        }
#endif                          /* XDMCP */

    }                           /* for */
    freeifaddrs(ifap);
#endif                          /* HAVE_GETIFADDRS */

    /*
     * add something of FamilyLocalHost
     */
    for (host = selfhosts;
         host && !addrEqual(FamilyLocalHost, "", 0, host); host = host->next);
    if (!host) {
        MakeHost(host, 0);
        if (host) {
            host->family = FamilyLocalHost;
            host->len = 0;
            /* Nothing to store in host->addr */
            host->next = selfhosts;
            selfhosts = host;
        }
    }
}
#endif                          /* hpux && !HAVE_IFREQ */

#ifdef XDMCP
void
AugmentSelf(void *from, int len)
{
    int family;
    void *addr;
    register HOST *host;

    family = ConvertAddr(from, &len, (void **) &addr);
    if (family == -1 || family == FamilyLocal)
        return;
    for (host = selfhosts; host; host = host->next) {
        if (addrEqual(family, addr, len, host))
            return;
    }
    MakeHost(host, len)
        if (!host)
        return;
    host->family = family;
    host->len = len;
    memcpy(host->addr, addr, len);
    host->next = selfhosts;
    selfhosts = host;
}
#endif

void
AddLocalHosts(void)
{
    HOST *self;

    for (self = selfhosts; self; self = self->next)
        /* Fix for XFree86 bug #156: pass addingLocal = TRUE to
         * NewHost to tell that we are adding the default local
         * host entries and not to flag the entries as being
         * explicitly requested */
        (void) NewHost(self->family, self->addr, self->len, TRUE);
}

/* Reset access control list to initial hosts */
void
ResetHosts(const char *display)
{
    register HOST *host;
    char lhostname[120], ohostname[120];
    char *hostname = ohostname;
    char fname[PATH_MAX + 1];
    int fnamelen;
    FILE *fd;
    char *ptr;
    int i, hostlen;

#if defined(TCPCONN) &&  (!defined(IPv6) || !defined(AF_INET6))
    union {
        struct sockaddr sa;
#if defined(TCPCONN)
        struct sockaddr_in in;
#endif                          /* TCPCONN || STREAMSCONN */
    } saddr;
#endif
    int family = 0;
    void *addr = NULL;
    int len;

    siTypesInitialize();
    AccessEnabled = !defeatAccessControl;
    LocalHostEnabled = FALSE;
    while ((host = validhosts) != 0) {
        validhosts = host->next;
        FreeHost(host);
    }

#if defined WIN32 && defined __MINGW32__
#define ETC_HOST_PREFIX "X"
#else
#define ETC_HOST_PREFIX "/etc/X"
#endif
#define ETC_HOST_SUFFIX ".hosts"
    fnamelen = strlen(ETC_HOST_PREFIX) + strlen(ETC_HOST_SUFFIX) +
        strlen(display) + 1;
    if (fnamelen > sizeof(fname))
        FatalError("Display name `%s' is too long\n", display);
    snprintf(fname, sizeof(fname), ETC_HOST_PREFIX "%s" ETC_HOST_SUFFIX,
             display);

    if ((fd = fopen(fname, "r")) != 0) {
        while (fgets(ohostname, sizeof(ohostname), fd)) {
            family = FamilyWild;
            if (*ohostname == '#')
                continue;
            if ((ptr = strchr(ohostname, '\n')) != 0)
                *ptr = 0;
            hostlen = strlen(ohostname) + 1;
            for (i = 0; i < hostlen; i++)
                lhostname[i] = tolower(ohostname[i]);
            hostname = ohostname;
            if (!strncmp("local:", lhostname, 6)) {
                family = FamilyLocalHost;
                NewHost(family, "", 0, FALSE);
                LocalHostRequested = TRUE;      /* Fix for XFree86 bug #156 */
            }
#if defined(TCPCONN)
            else if (!strncmp("inet:", lhostname, 5)) {
                family = FamilyInternet;
                hostname = ohostname + 5;
            }
#if defined(IPv6) && defined(AF_INET6)
            else if (!strncmp("inet6:", lhostname, 6)) {
                family = FamilyInternet6;
                hostname = ohostname + 6;
            }
#endif
#endif
#ifdef SECURE_RPC
            else if (!strncmp("nis:", lhostname, 4)) {
                family = FamilyNetname;
                hostname = ohostname + 4;
            }
#endif
            else if (!strncmp("si:", lhostname, 3)) {
                family = FamilyServerInterpreted;
                hostname = ohostname + 3;
                hostlen -= 3;
            }

            if (family == FamilyServerInterpreted) {
                len = siCheckAddr(hostname, hostlen);
                if (len >= 0) {
                    NewHost(family, hostname, len, FALSE);
                }
            }
            else
#ifdef SECURE_RPC
            if ((family == FamilyNetname) || (strchr(hostname, '@'))) {
                SecureRPCInit();
                (void) NewHost(FamilyNetname, hostname, strlen(hostname),
                               FALSE);
            }
            else
#endif                          /* SECURE_RPC */
#if defined(TCPCONN)
            {
#if defined(IPv6) && defined(AF_INET6)
                if ((family == FamilyInternet) || (family == FamilyInternet6) ||
                    (family == FamilyWild)) {
                    struct addrinfo *addresses;
                    struct addrinfo *a;
                    int f;

                    if (getaddrinfo(hostname, NULL, NULL, &addresses) == 0) {
                        for (a = addresses; a != NULL; a = a->ai_next) {
                            len = a->ai_addrlen;
                            f = ConvertAddr(a->ai_addr, &len,
                                            (void **) &addr);
                            if (addr && ((family == f) ||
                                         ((family == FamilyWild) && (f != -1)))) {
                                NewHost(f, addr, len, FALSE);
                            }
                        }
                        freeaddrinfo(addresses);
                    }
                }
#else
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
                _Xgethostbynameparams hparams;
#endif
                register struct hostent *hp;

                /* host name */
                if ((family == FamilyInternet &&
                     ((hp = _XGethostbyname(hostname, hparams)) != 0)) ||
                    ((hp = _XGethostbyname(hostname, hparams)) != 0)) {
                    saddr.sa.sa_family = hp->h_addrtype;
                    len = sizeof(saddr.sa);
                    if ((family =
                         ConvertAddr(&saddr.sa, &len,
                                     (void **) &addr)) != -1) {
#ifdef h_addr                   /* new 4.3bsd version of gethostent */
                        char **list;

                        /* iterate over the addresses */
                        for (list = hp->h_addr_list; *list; list++)
                            (void) NewHost(family, (void *) *list, len, FALSE);
#else
                        (void) NewHost(family, (void *) hp->h_addr, len,
                                       FALSE);
#endif
                    }
                }
#endif                          /* IPv6 */
            }
#endif                          /* TCPCONN || STREAMSCONN */
            family = FamilyWild;
        }
        fclose(fd);
    }
}

static Bool
xtransLocalClient(ClientPtr client)
{
    int alen, family, notused;
    Xtransaddr *from = NULL;
    void *addr;
    register HOST *host;
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    if (!oc->trans_conn)
        return FALSE;

    if (!_XSERVTransGetPeerAddr(oc->trans_conn, &notused, &alen, &from)) {
        family = ConvertAddr((struct sockaddr *) from,
                             &alen, (void **) &addr);
        if (family == -1) {
            free(from);
            return FALSE;
        }
        if (family == FamilyLocal) {
            free(from);
            return TRUE;
        }
        for (host = selfhosts; host; host = host->next) {
            if (addrEqual(family, addr, alen, host)) {
                free(from);
                return TRUE;
            }
        }
        free(from);
    }
    return FALSE;
}

/* Is client on the local host */
Bool
ComputeLocalClient(ClientPtr client)
{
    const char *cmdname = GetClientCmdName(client);

    if (!xtransLocalClient(client))
        return FALSE;

    /* If the executable name is "ssh", assume that this client connection
     * is forwarded from another host via SSH
     */
    if (cmdname) {
        char *cmd = strdup(cmdname);
        Bool ret;

        /* Cut off any colon and whatever comes after it, see
         * https://lists.freedesktop.org/archives/xorg-devel/2015-December/048164.html
         */
        char *tok = strtok(cmd, ":");

#if !defined(WIN32) || defined(__CYGWIN__)
        ret = strcmp(basename(tok), "ssh") != 0;
#else
        ret = strcmp(tok, "ssh") != 0;
#endif

        free(cmd);

        return ret;
    }

    return TRUE;
}

/*
 * Return the uid and all gids of a connected local client
 * Allocates a LocalClientCredRec - caller must call FreeLocalClientCreds
 *
 * Used by localuser & localgroup ServerInterpreted access control forms below
 * Used by AuthAudit to log who local connections came from
 */
int
GetLocalClientCreds(ClientPtr client, LocalClientCredRec ** lccp)
{
#if defined(HAVE_GETPEEREID) || defined(HAVE_GETPEERUCRED) || defined(SO_PEERCRED)
    int fd;
    XtransConnInfo ci;
    LocalClientCredRec *lcc;

#if defined(HAVE_GETPEERUCRED)
    ucred_t *peercred = NULL;
    const gid_t *gids;
#elif defined(SO_PEERCRED)
    struct ucred peercred;
    socklen_t so_len = sizeof(peercred);
#elif defined(HAVE_GETPEEREID)
    uid_t uid;
    gid_t gid;
#if defined(LOCAL_PEERPID)
    pid_t pid;
    socklen_t so_len = sizeof(pid);
#endif
#endif

    if (client == NULL)
        return -1;
    ci = ((OsCommPtr) client->osPrivate)->trans_conn;
#if !(defined(__sun) && defined(HAVE_GETPEERUCRED))
    /* Most implementations can only determine peer credentials for Unix
     * domain sockets - Solaris getpeerucred can work with a bit more, so
     * we just let it tell us if the connection type is supported or not
     */
    if (!_XSERVTransIsLocal(ci)) {
        return -1;
    }
#endif

    *lccp = calloc(1, sizeof(LocalClientCredRec));
    if (*lccp == NULL)
        return -1;
    lcc = *lccp;

    fd = _XSERVTransGetConnectionNumber(ci);
#if defined(HAVE_GETPEERUCRED)
    if (getpeerucred(fd, &peercred) < 0) {
        FreeLocalClientCreds(lcc);
        return -1;
    }
    lcc->euid = ucred_geteuid(peercred);
    if (lcc->euid != -1)
        lcc->fieldsSet |= LCC_UID_SET;
    lcc->egid = ucred_getegid(peercred);
    if (lcc->egid != -1)
        lcc->fieldsSet |= LCC_GID_SET;
    lcc->pid = ucred_getpid(peercred);
    if (lcc->pid != -1)
        lcc->fieldsSet |= LCC_PID_SET;
#ifdef HAVE_GETZONEID
    lcc->zoneid = ucred_getzoneid(peercred);
    if (lcc->zoneid != -1)
        lcc->fieldsSet |= LCC_ZID_SET;
#endif
    lcc->nSuppGids = ucred_getgroups(peercred, &gids);
    if (lcc->nSuppGids > 0) {
        lcc->pSuppGids = calloc(lcc->nSuppGids, sizeof(int));
        if (lcc->pSuppGids == NULL) {
            lcc->nSuppGids = 0;
        }
        else {
            int i;

            for (i = 0; i < lcc->nSuppGids; i++) {
                (lcc->pSuppGids)[i] = (int) gids[i];
            }
        }
    }
    else {
        lcc->nSuppGids = 0;
    }
    ucred_free(peercred);
    return 0;
#elif defined(SO_PEERCRED)
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peercred, &so_len) == -1) {
        FreeLocalClientCreds(lcc);
        return -1;
    }
    lcc->euid = peercred.uid;
    lcc->egid = peercred.gid;
    lcc->pid = peercred.pid;
    lcc->fieldsSet = LCC_UID_SET | LCC_GID_SET | LCC_PID_SET;
    return 0;
#elif defined(HAVE_GETPEEREID)
    if (getpeereid(fd, &uid, &gid) == -1) {
        FreeLocalClientCreds(lcc);
        return -1;
    }
    lcc->euid = uid;
    lcc->egid = gid;
    lcc->fieldsSet = LCC_UID_SET | LCC_GID_SET;

#if defined(LOCAL_PEERPID)
    if (getsockopt(fd, SOL_LOCAL, LOCAL_PEERPID, &pid, &so_len) != 0) {
        ErrorF("getsockopt failed to determine pid of socket %d: %s\n", fd, strerror(errno));
    } else {
        lcc->pid = pid;
        lcc->fieldsSet |= LCC_PID_SET;
    }
#endif

    return 0;
#endif
#else
    /* No system call available to get the credentials of the peer */
    return -1;
#endif
}

void
FreeLocalClientCreds(LocalClientCredRec * lcc)
{
    if (lcc != NULL) {
        if (lcc->nSuppGids > 0) {
            free(lcc->pSuppGids);
        }
        free(lcc);
    }
}

static int
AuthorizedClient(ClientPtr client)
{
    int rc;

    if (!client || defeatAccessControl)
        return Success;

    /* untrusted clients can't change host access */
    rc = XaceHook(XACE_SERVER_ACCESS, client, DixManageAccess);
    if (rc != Success)
        return rc;

    return client->local ? Success : BadAccess;
}

/* Add a host to the access control list.  This is the external interface
 * called from the dispatcher */

int
AddHost(ClientPtr client, int family, unsigned length,  /* of bytes in pAddr */
        const void *pAddr)
{
    int rc, len;

    rc = AuthorizedClient(client);
    if (rc != Success)
        return rc;
    switch (family) {
    case FamilyLocalHost:
        len = length;
        LocalHostEnabled = TRUE;
        break;
#ifdef SECURE_RPC
    case FamilyNetname:
        len = length;
        SecureRPCInit();
        break;
#endif
    case FamilyInternet:
#if defined(IPv6) && defined(AF_INET6)
    case FamilyInternet6:
#endif
    case FamilyDECnet:
    case FamilyChaos:
    case FamilyServerInterpreted:
        if ((len = CheckAddr(family, pAddr, length)) < 0) {
            client->errorValue = length;
            return BadValue;
        }
        break;
    case FamilyLocal:
    default:
        client->errorValue = family;
        return BadValue;
    }
    if (NewHost(family, pAddr, len, FALSE))
        return Success;
    return BadAlloc;
}

Bool
ForEachHostInFamily(int family, Bool (*func) (unsigned char *addr,
                                              short len,
                                              void *closure),
                    void *closure)
{
    HOST *host;

    for (host = validhosts; host; host = host->next)
        if (family == host->family && func(host->addr, host->len, closure))
            return TRUE;
    return FALSE;
}

/* Add a host to the access control list. This is the internal interface
 * called when starting or resetting the server */
static Bool
NewHost(int family, const void *addr, int len, int addingLocalHosts)
{
    register HOST *host;

    for (host = validhosts; host; host = host->next) {
        if (addrEqual(family, addr, len, host))
            return TRUE;
    }
    if (!addingLocalHosts) {    /* Fix for XFree86 bug #156 */
        for (host = selfhosts; host; host = host->next) {
            if (addrEqual(family, addr, len, host)) {
                host->requested = TRUE;
                break;
            }
        }
    }
    MakeHost(host, len)
        if (!host)
        return FALSE;
    host->family = family;
    host->len = len;
    memcpy(host->addr, addr, len);
    host->next = validhosts;
    validhosts = host;
    return TRUE;
}

/* Remove a host from the access control list */

int
RemoveHost(ClientPtr client, int family, unsigned length,       /* of bytes in pAddr */
           void *pAddr)
{
    int rc, len;
    register HOST *host, **prev;

    rc = AuthorizedClient(client);
    if (rc != Success)
        return rc;
    switch (family) {
    case FamilyLocalHost:
        len = length;
        LocalHostEnabled = FALSE;
        break;
#ifdef SECURE_RPC
    case FamilyNetname:
        len = length;
        break;
#endif
    case FamilyInternet:
#if defined(IPv6) && defined(AF_INET6)
    case FamilyInternet6:
#endif
    case FamilyDECnet:
    case FamilyChaos:
    case FamilyServerInterpreted:
        if ((len = CheckAddr(family, pAddr, length)) < 0) {
            client->errorValue = length;
            return BadValue;
        }
        break;
    case FamilyLocal:
    default:
        client->errorValue = family;
        return BadValue;
    }
    for (prev = &validhosts;
         (host = *prev) && (!addrEqual(family, pAddr, len, host));
         prev = &host->next);
    if (host) {
        *prev = host->next;
        FreeHost(host);
    }
    return Success;
}

/* Get all hosts in the access control list */
int
GetHosts(void **data, int *pnHosts, int *pLen, BOOL * pEnabled)
{
    int len;
    register int n = 0;
    register unsigned char *ptr;
    register HOST *host;
    int nHosts = 0;

    *pEnabled = AccessEnabled ? EnableAccess : DisableAccess;
    for (host = validhosts; host; host = host->next) {
        nHosts++;
        n += pad_to_int32(host->len) + sizeof(xHostEntry);
        /* Could check for INT_MAX, but in reality having more than 1mb of
           hostnames in the access list is ridiculous */
        if (n >= 1048576)
            break;
    }
    if (n) {
        *data = ptr = malloc(n);
        if (!ptr) {
            return BadAlloc;
        }
        for (host = validhosts; host; host = host->next) {
            len = host->len;
            if ((ptr + sizeof(xHostEntry) + len) > ((unsigned char *) *data + n))
                break;
            ((xHostEntry *) ptr)->family = host->family;
            ((xHostEntry *) ptr)->length = len;
            ptr += sizeof(xHostEntry);
            memcpy(ptr, host->addr, len);
            ptr += pad_to_int32(len);
        }
    }
    else {
        *data = NULL;
    }
    *pnHosts = nHosts;
    *pLen = n;
    return Success;
}

/* Check for valid address family and length, and return address length. */

 /*ARGSUSED*/ static int
CheckAddr(int family, const void *pAddr, unsigned length)
{
    int len;

    switch (family) {
#if defined(TCPCONN)
    case FamilyInternet:
        if (length == sizeof(struct in_addr))
            len = length;
        else
            len = -1;
        break;
#if defined(IPv6) && defined(AF_INET6)
    case FamilyInternet6:
        if (length == sizeof(struct in6_addr))
            len = length;
        else
            len = -1;
        break;
#endif
#endif
    case FamilyServerInterpreted:
        len = siCheckAddr(pAddr, length);
        break;
    default:
        len = -1;
    }
    return len;
}

/* Check if a host is not in the access control list.
 * Returns 1 if host is invalid, 0 if we've found it. */

int
InvalidHost(register struct sockaddr *saddr, int len, ClientPtr client)
{
    int family;
    void *addr = NULL;
    register HOST *selfhost, *host;

    if (!AccessEnabled)         /* just let them in */
        return 0;
    family = ConvertAddr(saddr, &len, (void **) &addr);
    if (family == -1)
        return 1;
    if (family == FamilyLocal) {
        if (!LocalHostEnabled) {
            /*
             * check to see if any local address is enabled.  This
             * implicitly enables local connections.
             */
            for (selfhost = selfhosts; selfhost; selfhost = selfhost->next) {
                for (host = validhosts; host; host = host->next) {
                    if (addrEqual(selfhost->family, selfhost->addr,
                                  selfhost->len, host))
                        return 0;
                }
            }
        }
        else
            return 0;
    }
    for (host = validhosts; host; host = host->next) {
        if (host->family == FamilyServerInterpreted) {
            if (siAddrMatch(family, addr, len, host, client)) {
                return 0;
            }
        }
        else {
            if (addr && addrEqual(family, addr, len, host))
                return 0;
        }

    }
    return 1;
}

static int
ConvertAddr(register struct sockaddr *saddr, int *len, void **addr)
{
    if (*len == 0)
        return FamilyLocal;
    switch (saddr->sa_family) {
    case AF_UNSPEC:
#if defined(UNIXCONN) || defined(LOCALCONN)
    case AF_UNIX:
#endif
        return FamilyLocal;
#if defined(TCPCONN)
    case AF_INET:
#ifdef WIN32
        if (16777343 == *(long *) &((struct sockaddr_in *) saddr)->sin_addr)
            return FamilyLocal;
#endif
        *len = sizeof(struct in_addr);
        *addr = (void *) &(((struct sockaddr_in *) saddr)->sin_addr);
        return FamilyInternet;
#if defined(IPv6) && defined(AF_INET6)
    case AF_INET6:
    {
        struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) saddr;

        if (IN6_IS_ADDR_V4MAPPED(&(saddr6->sin6_addr))) {
            *len = sizeof(struct in_addr);
            *addr = (void *) &(saddr6->sin6_addr.s6_addr[12]);
            return FamilyInternet;
        }
        else {
            *len = sizeof(struct in6_addr);
            *addr = (void *) &(saddr6->sin6_addr);
            return FamilyInternet6;
        }
    }
#endif
#endif
    default:
        return -1;
    }
}

int
ChangeAccessControl(ClientPtr client, int fEnabled)
{
    int rc = AuthorizedClient(client);

    if (rc != Success)
        return rc;
    AccessEnabled = fEnabled;
    return Success;
}

/* returns FALSE if xhost + in effect, else TRUE */
int
GetAccessControl(void)
{
    return AccessEnabled;
}

int
GetClientFd(ClientPtr client)
{
    return ((OsCommPtr) client->osPrivate)->fd;
}

Bool
ClientIsLocal(ClientPtr client)
{
    XtransConnInfo ci = ((OsCommPtr) client->osPrivate)->trans_conn;

    return _XSERVTransIsLocal(ci);
}

/*****************************************************************************
 * FamilyServerInterpreted host entry implementation
 *
 * Supports an extensible system of host types which the server can interpret
 * See the IPv6 extensions to the X11 protocol spec for the definition.
 *
 * Currently supported schemes:
 *
 * hostname	- hostname as defined in IETF RFC 2396
 * ipv6		- IPv6 literal address as defined in IETF RFC's 3513 and <TBD>
 *
 * See xc/doc/specs/SIAddresses for formal definitions of each type.
 */

/* These definitions and the siTypeAdd function could be exported in the
 * future to enable loading additional host types, but that was not done for
 * the initial implementation.
 */
typedef Bool (*siAddrMatchFunc) (int family, void *addr, int len,
                                 const char *siAddr, int siAddrlen,
                                 ClientPtr client, void *siTypePriv);
typedef int (*siCheckAddrFunc) (const char *addrString, int length,
                                void *siTypePriv);

struct siType {
    struct siType *next;
    const char *typeName;
    siAddrMatchFunc addrMatch;
    siCheckAddrFunc checkAddr;
    void *typePriv;             /* Private data for type routines */
};

static struct siType *siTypeList;

static int
siTypeAdd(const char *typeName, siAddrMatchFunc addrMatch,
          siCheckAddrFunc checkAddr, void *typePriv)
{
    struct siType *s, *p;

    if ((typeName == NULL) || (addrMatch == NULL) || (checkAddr == NULL))
        return BadValue;

    for (s = siTypeList, p = NULL; s != NULL; p = s, s = s->next) {
        if (strcmp(typeName, s->typeName) == 0) {
            s->addrMatch = addrMatch;
            s->checkAddr = checkAddr;
            s->typePriv = typePriv;
            return Success;
        }
    }

    s = malloc(sizeof(struct siType));
    if (s == NULL)
        return BadAlloc;

    if (p == NULL)
        siTypeList = s;
    else
        p->next = s;

    s->next = NULL;
    s->typeName = typeName;
    s->addrMatch = addrMatch;
    s->checkAddr = checkAddr;
    s->typePriv = typePriv;
    return Success;
}

/* Checks to see if a host matches a server-interpreted host entry */
static Bool
siAddrMatch(int family, void *addr, int len, HOST * host, ClientPtr client)
{
    Bool matches = FALSE;
    struct siType *s;
    const char *valueString;
    int addrlen;

    valueString = (const char *) memchr(host->addr, '\0', host->len);
    if (valueString != NULL) {
        for (s = siTypeList; s != NULL; s = s->next) {
            if (strcmp((char *) host->addr, s->typeName) == 0) {
                addrlen = host->len - (strlen((char *) host->addr) + 1);
                matches = s->addrMatch(family, addr, len,
                                       valueString + 1, addrlen, client,
                                       s->typePriv);
                break;
            }
        }
#ifdef FAMILY_SI_DEBUG
        ErrorF("Xserver: siAddrMatch(): type = %s, value = %*.*s -- %s\n",
               host->addr, addrlen, addrlen, valueString + 1,
               (matches) ? "accepted" : "rejected");
#endif
    }
    return matches;
}

static int
siCheckAddr(const char *addrString, int length)
{
    const char *valueString;
    int addrlen, typelen;
    int len = -1;
    struct siType *s;

    /* Make sure there is a \0 byte inside the specified length
       to separate the address type from the address value. */
    valueString = (const char *) memchr(addrString, '\0', length);
    if (valueString != NULL) {
        /* Make sure the first string is a recognized address type,
         * and the second string is a valid address of that type.
         */
        typelen = strlen(addrString) + 1;
        addrlen = length - typelen;

        for (s = siTypeList; s != NULL; s = s->next) {
            if (strcmp(addrString, s->typeName) == 0) {
                len = s->checkAddr(valueString + 1, addrlen, s->typePriv);
                if (len >= 0) {
                    len += typelen;
                }
                break;
            }
        }
#ifdef FAMILY_SI_DEBUG
        {
            const char *resultMsg;

            if (s == NULL) {
                resultMsg = "type not registered";
            }
            else {
                if (len == -1)
                    resultMsg = "rejected";
                else
                    resultMsg = "accepted";
            }

            ErrorF
                ("Xserver: siCheckAddr(): type = %s, value = %*.*s, len = %d -- %s\n",
                 addrString, addrlen, addrlen, valueString + 1, len, resultMsg);
        }
#endif
    }
    return len;
}

/***
 * Hostname server-interpreted host type
 *
 * Stored as hostname string, explicitly defined to be resolved ONLY
 * at access check time, to allow for hosts with dynamic addresses
 * but static hostnames, such as found in some DHCP & mobile setups.
 *
 * Hostname must conform to IETF RFC 2396 sec. 3.2.2, which defines it as:
 * 	hostname     = *( domainlabel "." ) toplabel [ "." ]
 *	domainlabel  = alphanum | alphanum *( alphanum | "-" ) alphanum
 *	toplabel     = alpha | alpha *( alphanum | "-" ) alphanum
 */

#ifdef NI_MAXHOST
#define SI_HOSTNAME_MAXLEN NI_MAXHOST
#else
#ifdef MAXHOSTNAMELEN
#define SI_HOSTNAME_MAXLEN MAXHOSTNAMELEN
#else
#define SI_HOSTNAME_MAXLEN 256
#endif
#endif

static Bool
siHostnameAddrMatch(int family, void *addr, int len,
                    const char *siAddr, int siAddrLen, ClientPtr client,
                    void *typePriv)
{
    Bool res = FALSE;

/* Currently only supports checking against IPv4 & IPv6 connections, but
 * support for other address families, such as DECnet, could be added if
 * desired.
 */
#if defined(IPv6) && defined(AF_INET6)
    if ((family == FamilyInternet) || (family == FamilyInternet6)) {
        char hostname[SI_HOSTNAME_MAXLEN];
        struct addrinfo *addresses;
        struct addrinfo *a;
        int f, hostaddrlen;
        void *hostaddr = NULL;

        if (siAddrLen >= sizeof(hostname))
            return FALSE;

        strlcpy(hostname, siAddr, siAddrLen + 1);

        if (getaddrinfo(hostname, NULL, NULL, &addresses) == 0) {
            for (a = addresses; a != NULL; a = a->ai_next) {
                hostaddrlen = a->ai_addrlen;
                f = ConvertAddr(a->ai_addr, &hostaddrlen, &hostaddr);
                if ((f == family) && (len == hostaddrlen) && hostaddr &&
                    (memcmp(addr, hostaddr, len) == 0)) {
                    res = TRUE;
                    break;
                }
            }
            freeaddrinfo(addresses);
        }
    }
#else                           /* IPv6 not supported, use gethostbyname instead for IPv4 */
    if (family == FamilyInternet) {
        register struct hostent *hp;

#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
        _Xgethostbynameparams hparams;
#endif
        char hostname[SI_HOSTNAME_MAXLEN];
        int f, hostaddrlen;
        void *hostaddr;
        const char **addrlist;

        if (siAddrLen >= sizeof(hostname))
            return FALSE;

        strlcpy(hostname, siAddr, siAddrLen + 1);

        if ((hp = _XGethostbyname(hostname, hparams)) != NULL) {
#ifdef h_addr                   /* new 4.3bsd version of gethostent */
            /* iterate over the addresses */
            for (addrlist = hp->h_addr_list; *addrlist; addrlist++)
#else
            addrlist = &hp->h_addr;
#endif
            {
                struct sockaddr_in sin;

                sin.sin_family = hp->h_addrtype;
                memcpy(&(sin.sin_addr), *addrlist, hp->h_length);
                hostaddrlen = sizeof(sin);
                f = ConvertAddr((struct sockaddr *) &sin,
                                &hostaddrlen, &hostaddr);
                if ((f == family) && (len == hostaddrlen) &&
                    (memcmp(addr, hostaddr, len) == 0)) {
                    res = TRUE;
                    break;
                }
            }
        }
    }
#endif
    return res;
}

static int
siHostnameCheckAddr(const char *valueString, int length, void *typePriv)
{
    /* Check conformance of hostname to RFC 2396 sec. 3.2.2 definition.
     * We do not use ctype functions here to avoid locale-specific
     * character sets.  Hostnames must be pure ASCII.
     */
    int len = length;
    int i;
    Bool dotAllowed = FALSE;
    Bool dashAllowed = FALSE;

    if ((length <= 0) || (length >= SI_HOSTNAME_MAXLEN)) {
        len = -1;
    }
    else {
        for (i = 0; i < length; i++) {
            char c = valueString[i];

            if (c == 0x2E) {    /* '.' */
                if (dotAllowed == FALSE) {
                    len = -1;
                    break;
                }
                else {
                    dotAllowed = FALSE;
                    dashAllowed = FALSE;
                }
            }
            else if (c == 0x2D) {       /* '-' */
                if (dashAllowed == FALSE) {
                    len = -1;
                    break;
                }
                else {
                    dotAllowed = FALSE;
                }
            }
            else if (((c >= 0x30) && (c <= 0x3A)) /* 0-9 */ ||
                     ((c >= 0x61) && (c <= 0x7A)) /* a-z */ ||
                     ((c >= 0x41) && (c <= 0x5A)) /* A-Z */ ) {
                dotAllowed = TRUE;
                dashAllowed = TRUE;
            }
            else {              /* Invalid character */
                len = -1;
                break;
            }
        }
    }
    return len;
}

#if defined(IPv6) && defined(AF_INET6)
/***
 * "ipv6" server interpreted type
 *
 * Currently supports only IPv6 literal address as specified in IETF RFC 3513
 *
 * Once draft-ietf-ipv6-scoping-arch-00.txt becomes an RFC, support will be
 * added for the scoped address format it specifies.
 */

/* Maximum length of an IPv6 address string - increase when adding support
 * for scoped address qualifiers.  Includes room for trailing NUL byte.
 */
#define SI_IPv6_MAXLEN INET6_ADDRSTRLEN

static Bool
siIPv6AddrMatch(int family, void *addr, int len,
                const char *siAddr, int siAddrlen, ClientPtr client,
                void *typePriv)
{
    struct in6_addr addr6;
    char addrbuf[SI_IPv6_MAXLEN];

    if ((family != FamilyInternet6) || (len != sizeof(addr6)))
        return FALSE;

    memcpy(addrbuf, siAddr, siAddrlen);
    addrbuf[siAddrlen] = '\0';

    if (inet_pton(AF_INET6, addrbuf, &addr6) != 1) {
        perror("inet_pton");
        return FALSE;
    }

    if (memcmp(addr, &addr6, len) == 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static int
siIPv6CheckAddr(const char *addrString, int length, void *typePriv)
{
    int len;

    /* Minimum length is 3 (smallest legal address is "::1") */
    if (length < 3) {
        /* Address is too short! */
        len = -1;
    }
    else if (length >= SI_IPv6_MAXLEN) {
        /* Address is too long! */
        len = -1;
    }
    else {
        /* Assume inet_pton is sufficient validation */
        struct in6_addr addr6;
        char addrbuf[SI_IPv6_MAXLEN];

        memcpy(addrbuf, addrString, length);
        addrbuf[length] = '\0';

        if (inet_pton(AF_INET6, addrbuf, &addr6) != 1) {
            perror("inet_pton");
            len = -1;
        }
        else {
            len = length;
        }
    }
    return len;
}
#endif                          /* IPv6 */

#if !defined(NO_LOCAL_CLIENT_CRED)
/***
 * "localuser" & "localgroup" server interpreted types
 *
 * Allows local connections from a given local user or group
 */

#include <pwd.h>
#include <grp.h>

#define LOCAL_USER 1
#define LOCAL_GROUP 2

typedef struct {
    int credType;
} siLocalCredPrivRec, *siLocalCredPrivPtr;

static siLocalCredPrivRec siLocalUserPriv = { LOCAL_USER };
static siLocalCredPrivRec siLocalGroupPriv = { LOCAL_GROUP };

static Bool
siLocalCredGetId(const char *addr, int len, siLocalCredPrivPtr lcPriv, int *id)
{
    Bool parsedOK = FALSE;
    char *addrbuf = malloc(len + 1);

    if (addrbuf == NULL) {
        return FALSE;
    }

    memcpy(addrbuf, addr, len);
    addrbuf[len] = '\0';

    if (addr[0] == '#') {       /* numeric id */
        char *cp;

        errno = 0;
        *id = strtol(addrbuf + 1, &cp, 0);
        if ((errno == 0) && (cp != (addrbuf + 1))) {
            parsedOK = TRUE;
        }
    }
    else {                      /* non-numeric name */
        if (lcPriv->credType == LOCAL_USER) {
            struct passwd *pw = getpwnam(addrbuf);

            if (pw != NULL) {
                *id = (int) pw->pw_uid;
                parsedOK = TRUE;
            }
        }
        else {                  /* group */
            struct group *gr = getgrnam(addrbuf);

            if (gr != NULL) {
                *id = (int) gr->gr_gid;
                parsedOK = TRUE;
            }
        }
    }

    free(addrbuf);
    return parsedOK;
}

static Bool
siLocalCredAddrMatch(int family, void *addr, int len,
                     const char *siAddr, int siAddrlen, ClientPtr client,
                     void *typePriv)
{
    int siAddrId;
    LocalClientCredRec *lcc;
    siLocalCredPrivPtr lcPriv = (siLocalCredPrivPtr) typePriv;

    if (GetLocalClientCreds(client, &lcc) == -1) {
        return FALSE;
    }

#ifdef HAVE_GETZONEID           /* Ensure process is in the same zone */
    if ((lcc->fieldsSet & LCC_ZID_SET) && (lcc->zoneid != getzoneid())) {
        FreeLocalClientCreds(lcc);
        return FALSE;
    }
#endif

    if (siLocalCredGetId(siAddr, siAddrlen, lcPriv, &siAddrId) == FALSE) {
        FreeLocalClientCreds(lcc);
        return FALSE;
    }

    if (lcPriv->credType == LOCAL_USER) {
        if ((lcc->fieldsSet & LCC_UID_SET) && (lcc->euid == siAddrId)) {
            FreeLocalClientCreds(lcc);
            return TRUE;
        }
    }
    else {
        if ((lcc->fieldsSet & LCC_GID_SET) && (lcc->egid == siAddrId)) {
            FreeLocalClientCreds(lcc);
            return TRUE;
        }
        if (lcc->pSuppGids != NULL) {
            int i;

            for (i = 0; i < lcc->nSuppGids; i++) {
                if (lcc->pSuppGids[i] == siAddrId) {
                    FreeLocalClientCreds(lcc);
                    return TRUE;
                }
            }
        }
    }
    FreeLocalClientCreds(lcc);
    return FALSE;
}

static int
siLocalCredCheckAddr(const char *addrString, int length, void *typePriv)
{
    int len = length;
    int id;

    if (siLocalCredGetId(addrString, length,
                         (siLocalCredPrivPtr) typePriv, &id) == FALSE) {
        len = -1;
    }
    return len;
}
#endif                          /* localuser */

static void
siTypesInitialize(void)
{
    siTypeAdd("hostname", siHostnameAddrMatch, siHostnameCheckAddr, NULL);
#if defined(IPv6) && defined(AF_INET6)
    siTypeAdd("ipv6", siIPv6AddrMatch, siIPv6CheckAddr, NULL);
#endif
#if !defined(NO_LOCAL_CLIENT_CRED)
    siTypeAdd("localuser", siLocalCredAddrMatch, siLocalCredCheckAddr,
              &siLocalUserPriv);
    siTypeAdd("localgroup", siLocalCredAddrMatch, siLocalCredCheckAddr,
              &siLocalGroupPriv);
#endif
}
