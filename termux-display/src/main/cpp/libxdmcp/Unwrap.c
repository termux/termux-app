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
 * The interface for this routine is quite simple.  All three
 * arguments are arrays of 8 unsigned characters, the first two
 * are 64 bits of useful data, the last is 56 bits of useful
 * data packed into 8 bytes, using the low 7 bits of each
 * byte, filling the high bit with odd parity.
 *
 * Examine the XDMCP specification for the correct algorithm
 */

#include "Wrap.h"

void
XdmcpUnwrap (
    unsigned char	*input,
    unsigned char	*wrapper,
    unsigned char	*output,
    int			bytes)
{
    int			i, j, k;
    unsigned char	tmp[8];
    unsigned char	blocks[2][8];
    unsigned char	expand_wrapper[8];
    auth_wrapper_schedule	schedule;

    _XdmcpWrapperToOddParity (wrapper, expand_wrapper);
    _XdmcpAuthSetup (expand_wrapper, schedule);

    k = 0;
    for (j = 0; j < bytes; j += 8)
    {
	if (bytes - j < 8)
	    return; /* bad input length */
	for (i = 0; i < 8; i++)
	    blocks[k][i] = input[j + i];
	_XdmcpAuthDoIt ((unsigned char *) (input + j), (unsigned char *) tmp, schedule, 0);
	/* block chaining */
	k = (k == 0) ? 1 : 0;
	for (i = 0; i < 8; i++)
	{
	    if (j == 0)
		output[j + i] = tmp[i];
	    else
		output[j + i] = tmp[i] ^ blocks[k][i];
	}
    }
}

#endif /* HASXDMAUTH */
