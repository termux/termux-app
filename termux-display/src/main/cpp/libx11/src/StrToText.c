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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

/*
 * XStringListToTextProperty - fill in TextProperty structure with
 * concatenated list of null-separated strings.  Return True if successful
 * else False.  Allocate room on end for trailing NULL, but don't include in
 * count.
 */

Status XStringListToTextProperty (
    char **argv,
    int argc,
    XTextProperty *textprop)
{
    register int i;
    register unsigned int nbytes;
    XTextProperty proto;

    /* figure out how much space we'll need for this list */
    for (i = 0, nbytes = 0; i < argc; i++) {
	nbytes += (unsigned) ((argv[i] ? strlen (argv[i]) : 0) + 1);
    }

    /* fill in a prototype containing results so far */
    proto.encoding = XA_STRING;
    proto.format = 8;
    if (nbytes)
	proto.nitems = nbytes - 1;	/* subtract one for trailing <NUL> */
    else
	proto.nitems = 0;
    proto.value = NULL;

    /* build concatenated list of strings */
    if (nbytes > 0) {
	register char *buf = Xmalloc (nbytes);
	if (!buf) return False;

	proto.value = (unsigned char *) buf;
	for (i = 0; i < argc; i++) {
	    char *arg = argv[i];

	    if (arg) {
		(void) strcpy (buf, arg);
		buf += (strlen (arg) + 1);
	    } else {
		*buf++ = '\0';
	    }
	}
    } else {
	proto.value = Xmalloc (1);		/* easier for client */
	if (!proto.value) return False;

	proto.value[0] = '\0';
    }

    /* we were successful, so set return value */
    *textprop = proto;
    return True;
}
