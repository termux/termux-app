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
 * *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

#ifdef HASXDMAUTH

/*
 * The following function exists only to demonstrate the
 * desired functional interface for this routine.  You will
 * need to add the appropriate algorithm if you wish to
 * use XDM-AUTHENTICATION-1/XDM-AUTHORIZATION-1.
 *
 * Examine the XDMCP specification for the correct algorithm
 */

#include "Wrap.h"

void
XdmcpWrap (
    unsigned char	*input,
    unsigned char	*wrapper,
    unsigned char	*output,
    int			bytes)
{
    int			i, j;
    int			len;
    unsigned char	tmp[8];
    unsigned char	expand_wrapper[8];
    auth_wrapper_schedule	schedule;

    _XdmcpWrapperToOddParity (wrapper, expand_wrapper);
    _XdmcpAuthSetup (expand_wrapper, schedule);
    for (j = 0; j < bytes; j += 8)
    {
	len = 8;
	if (bytes - j < len)
	    len = bytes - j;
	/* block chaining */
	for (i = 0; i < len; i++)
	{
	    if (j == 0)
		tmp[i] = input[i];
	    else
		tmp[i] = input[j + i] ^ output[j - 8 + i];
	}
	for (; i < 8; i++)
	{
	    if (j == 0)
		tmp[i] = 0;
	    else
		tmp[i] = 0 ^ output[j - 8 + i];
	}
	_XdmcpAuthDoIt (tmp, (output + j), schedule, 1);
    }
}

/*
 * Given a 56 bit wrapper in XDMCP format, create a 56
 * bit wrapper in 7-bits + odd parity format
 */

static int
OddParity (unsigned char c)
{
    c = c ^ (c >> 4);
    c = c ^ (c >> 2);
    c = c ^ (c >> 1);
    return ~c & 0x1;
}

/*
 * Spread the 56 bit wrapper among 8 bytes, using the upper 7 bits
 * of each byte, and storing an odd parity bit in the low bit
 */

void
_XdmcpWrapperToOddParity (
    unsigned char   *in,
    unsigned char   *out)
{
    int		    ashift, bshift;
    int		    i;
    unsigned char   c;

    ashift = 7;
    bshift = 1;
    for (i = 0; i < 7; i++)
    {
	c = ((in[i] << ashift) | (in[i+1] >> bshift)) & 0x7f;
	out[i] = (c << 1) | OddParity (c);
	ashift--;
	bshift++;
    }
    c = in[i];
    out[i] = (c << 1) | OddParity(c);
}

#endif
