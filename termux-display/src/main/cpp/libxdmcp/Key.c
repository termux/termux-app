/*
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

#ifndef HAVE_ARC4RANDOM_BUF
static void
getbits (long data, unsigned char *dst)
{
    dst[0] = (data      ) & 0xff;
    dst[1] = (data >>  8) & 0xff;
    dst[2] = (data >> 16) & 0xff;
    dst[3] = (data >> 24) & 0xff;
}
#endif

#define Time_t time_t

#include <stdlib.h>

#if defined(HAVE_LRAND48) && defined(HAVE_SRAND48)
#define srandom srand48
#define random lrand48
#endif
#ifdef WIN32
#include <process.h>
#define srandom srand
#define random rand
#define getpid(x) _getpid(x)
#endif

#ifndef HAVE_ARC4RANDOM_BUF

/* Solaris 11.3.0 - 11.4.15 only define getentropy() in <sys/random.h> */
#if HAVE_GETENTROPY && HAVE_SYS_RANDOM_H
# include <sys/random.h>
#endif

static void
insecure_getrandom_buf (unsigned char *auth, int len)
{
    long    lowbits, highbits;

    srandom ((int)getpid() ^ time((Time_t *)0));
    lowbits = random ();
    highbits = random ();
    getbits (lowbits, auth);
    getbits (highbits, auth + 4);
}

static void
arc4random_buf (void *auth, int len)
{
#if HAVE_GETENTROPY
    int	    ret;

    /* weak emulation of arc4random through the getentropy libc call */
    ret = getentropy (auth, len);
    if (ret == 0)
	return;
#endif /* HAVE_GETENTROPY */

    insecure_getrandom_buf (auth, len);
}

#endif /* !defined(HAVE_ARC4RANDOM_BUF) */

void
XdmcpGenerateKey (XdmAuthKeyPtr key)
{
    arc4random_buf(key->data, 8);
}

int
XdmcpCompareKeys (const XdmAuthKeyPtr a, const XdmAuthKeyPtr b)
{
    int	i;

    for (i = 0; i < 8; i++)
	if (a->data[i] != b->data[i])
	    return FALSE;
    return TRUE;
}

void
XdmcpIncrementKey (XdmAuthKeyPtr key)
{
    int	i;

    i = 7;
    while (++key->data[i] == 0)
	if (--i < 0)
	    break;
}

void
XdmcpDecrementKey (XdmAuthKeyPtr key)
{
    int	i;

    i = 7;
    while (key->data[i]-- == 0)
	if (--i < 0)
	    break;
}
