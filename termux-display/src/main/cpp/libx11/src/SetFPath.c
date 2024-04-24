/*

Copyright 1986, 1998  The Open Group

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
#include <limits.h>
#include "Xlibint.h"

#define safestrlen(s) ((s) ? strlen(s) : 0)

int
XSetFontPath (
    register Display *dpy,
    char **directories,
    int ndirs)
{
	register size_t n = 0;
	register int i;
	register int nbytes;
	char *p;
	register xSetFontPathReq *req;
	int retCode;

        LockDisplay(dpy);
	GetReq (SetFontPath, req);
	req->nFonts = ndirs;
	for (i = 0; i < ndirs; i++) {
		n = n + (safestrlen (directories[i]) + 1);
		if (n >= USHRT_MAX) {
			UnlockDisplay(dpy);
			SyncHandle();
			return 0;
		}
	}
	nbytes = (n + 3) & ~3;
	req->length += nbytes >> 2;
	if ((p = Xmalloc (nbytes))) {
		/*
	 	 * pack into counted strings.
	 	 */
		char	*tmp = p;

		for (i = 0; i < ndirs; i++) {
			size_t length = safestrlen (directories[i]);
			*p = length;
			memcpy (p + 1, directories[i], length);
			p += length + 1;
		}
		Data (dpy, tmp, nbytes);
		Xfree (tmp);
		retCode = 1;
	}
	else
		retCode = 0;

        UnlockDisplay(dpy);
	SyncHandle();
	return (retCode);
}
