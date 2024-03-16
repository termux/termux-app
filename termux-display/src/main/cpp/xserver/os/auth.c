/*

Copyright 1988, 1998  The Open Group

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

*/

/*
 * authorization hooks for the server
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include   <X11/X.h>
#include   <X11/Xauth.h>
#include   "misc.h"
#include   "osdep.h"
#include   "dixstruct.h"
#include   <sys/types.h>
#include   <sys/stat.h>
#include   <errno.h>
#ifdef WIN32
#include    <X11/Xw32defs.h>
#endif
#ifdef HAVE_LIBBSD
#include   <bsd/stdlib.h>       /* for arc4random_buf() */
#endif

struct protocol {
    unsigned short name_length;
    const char *name;
    AuthAddCFunc Add;           /* new authorization data */
    AuthCheckFunc Check;        /* verify client authorization data */
    AuthRstCFunc Reset;         /* delete all authorization data entries */
    AuthFromIDFunc FromID;      /* convert ID to cookie */
    AuthRemCFunc Remove;        /* remove a specific cookie */
#ifdef XCSECURITY
    AuthGenCFunc Generate;
#endif
};

static struct protocol protocols[] = {
    {(unsigned short) 18, "MIT-MAGIC-COOKIE-1",
     MitAddCookie, MitCheckCookie, MitResetCookie,
     MitFromID, MitRemoveCookie,
#ifdef XCSECURITY
     MitGenerateCookie
#endif
     },
#ifdef HASXDMAUTH
    {(unsigned short) 19, "XDM-AUTHORIZATION-1",
     XdmAddCookie, XdmCheckCookie, XdmResetCookie,
     XdmFromID, XdmRemoveCookie,
#ifdef XCSECURITY
     NULL
#endif
     },
#endif
#ifdef SECURE_RPC
    {(unsigned short) 9, "SUN-DES-1",
     SecureRPCAdd, SecureRPCCheck, SecureRPCReset,
     SecureRPCFromID, SecureRPCRemove,
#ifdef XCSECURITY
     NULL
#endif
     },
#endif
};

#define NUM_AUTHORIZATION  ARRAY_SIZE(protocols)

/*
 * Initialize all classes of authorization by reading the
 * specified authorization file
 */

static const char *authorization_file = NULL;

static Bool ShouldLoadAuth = TRUE;

void
InitAuthorization(const char *file_name)
{
    authorization_file = file_name;
}

static int
LoadAuthorization(void)
{
    FILE *f;
    Xauth *auth;
    int i;
    int count = 0;

    ShouldLoadAuth = FALSE;
    if (!authorization_file)
        return 0;

    errno = 0;
    f = Fopen(authorization_file, "r");
    if (!f) {
        LogMessageVerb(X_ERROR, 0,
                       "Failed to open authorization file \"%s\": %s\n",
                       authorization_file,
                       errno != 0 ? strerror(errno) : "Unknown error");
        return -1;
    }

    while ((auth = XauReadAuth(f)) != 0) {
        for (i = 0; i < NUM_AUTHORIZATION; i++) {
            if (protocols[i].name_length == auth->name_length &&
                memcmp(protocols[i].name, auth->name,
                       (int) auth->name_length) == 0 && protocols[i].Add) {
                ++count;
                (*protocols[i].Add) (auth->data_length, auth->data,
                                     FakeClientID(0));
            }
        }
        XauDisposeAuth(auth);
    }

    Fclose(f);
    return count;
}

#ifdef XDMCP
/*
 * XdmcpInit calls this function to discover all authorization
 * schemes supported by the display
 */
void
RegisterAuthorizations(void)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
        XdmcpRegisterAuthorization(protocols[i].name,
                                   (int) protocols[i].name_length);
}
#endif

XID
CheckAuthorization(unsigned int name_length,
                   const char *name,
                   unsigned int data_length,
                   const char *data, ClientPtr client, const char **reason)
{                               /* failure message.  NULL for default msg */
    int i;
    struct stat buf;
    static time_t lastmod = 0;
    static Bool loaded = FALSE;

    if (!authorization_file || stat(authorization_file, &buf)) {
        if (lastmod != 0) {
            lastmod = 0;
            ShouldLoadAuth = TRUE;      /* stat lost, so force reload */
        }
    }
    else if (buf.st_mtime > lastmod) {
        lastmod = buf.st_mtime;
        ShouldLoadAuth = TRUE;
    }
    if (ShouldLoadAuth) {
        int loadauth = LoadAuthorization();

        /*
         * If the authorization file has at least one entry for this server,
         * disable local access. (loadauth > 0)
         *
         * If there are zero entries (either initially or when the
         * authorization file is later reloaded), or if a valid
         * authorization file was never loaded, enable local access.
         * (loadauth == 0 || !loaded)
         *
         * If the authorization file was loaded initially (with valid
         * entries for this server), and reloading it later fails, don't
         * change anything. (loadauth == -1 && loaded)
         */

        if (loadauth > 0) {
            DisableLocalAccess(); /* got at least one */
            loaded = TRUE;
        }
        else if (loadauth == 0 || !loaded)
            EnableLocalAccess();
    }
    if (name_length) {
        for (i = 0; i < NUM_AUTHORIZATION; i++) {
            if (protocols[i].name_length == name_length &&
                memcmp(protocols[i].name, name, (int) name_length) == 0) {
                return (*protocols[i].Check) (data_length, data, client,
                                              reason);
            }
            *reason = "Authorization protocol not supported by server\n";
        }
    }
    else
        *reason = "Authorization required, but no authorization protocol specified\n";
    return (XID) ~0L;
}

void
ResetAuthorization(void)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
        if (protocols[i].Reset)
            (*protocols[i].Reset) ();
    ShouldLoadAuth = TRUE;
}

int
AuthorizationFromID(XID id,
                    unsigned short *name_lenp,
                    const char **namep, unsigned short *data_lenp, char **datap)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
        if (protocols[i].FromID &&
            (*protocols[i].FromID) (id, data_lenp, datap)) {
            *name_lenp = protocols[i].name_length;
            *namep = protocols[i].name;
            return 1;
        }
    }
    return 0;
}

int
RemoveAuthorization(unsigned short name_length,
                    const char *name,
                    unsigned short data_length, const char *data)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
        if (protocols[i].name_length == name_length &&
            memcmp(protocols[i].name, name, (int) name_length) == 0 &&
            protocols[i].Remove) {
            return (*protocols[i].Remove) (data_length, data);
        }
    }
    return 0;
}

int
AddAuthorization(unsigned name_length, const char *name,
                 unsigned data_length, char *data)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
        if (protocols[i].name_length == name_length &&
            memcmp(protocols[i].name, name, (int) name_length) == 0 &&
            protocols[i].Add) {
            return (*protocols[i].Add) (data_length, data, FakeClientID(0));
        }
    }
    return 0;
}

#ifdef XCSECURITY

XID
GenerateAuthorization(unsigned name_length,
                      const char *name,
                      unsigned data_length,
                      const char *data,
                      unsigned *data_length_return, char **data_return)
{
    int i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
        if (protocols[i].name_length == name_length &&
            memcmp(protocols[i].name, name, (int) name_length) == 0 &&
            protocols[i].Generate) {
            return (*protocols[i].Generate) (data_length, data,
                                             FakeClientID(0),
                                             data_length_return, data_return);
        }
    }
    return -1;
}

#endif                          /* XCSECURITY */

void
GenerateRandomData(int len, char *buf)
{
#ifdef HAVE_ARC4RANDOM_BUF
    arc4random_buf(buf, len);
#else
    int fd;

    fd = open("/dev/urandom", O_RDONLY);
    read(fd, buf, len);
    close(fd);
#endif
}
