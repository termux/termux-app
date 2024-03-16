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
#include "reallocarray.h"

/*
 * XTextPropertyToStringList - set list and count to contain data stored in
 * null-separated STRING property.
 */

Status XTextPropertyToStringList (
    XTextProperty *tp,
    char ***list_return,
    int *count_return)
{
    char **list;			/* return value */
    int nelements;			/* return value */
    register char *cp;			/* temp variable */
    char *start;			/* start of thing to copy */
    int i, j;				/* iterator variables */
    int datalen = (int) tp->nitems;	/* for convenience */

    /*
     * make sure we understand how to do it
     */
    if (tp->encoding != XA_STRING ||  tp->format != 8) return False;

    if (datalen == 0) {
	*list_return = NULL;
	*count_return = 0;
	return True;
    }

    /*
     * walk the list to figure out how many elements there are
     */
    nelements = 1;			/* since null-separated */
    for (cp = (char *) tp->value, i = datalen; i > 0; cp++, i--) {
	if (*cp == '\0') nelements++;
    }

    /*
     * allocate list and duplicate
     */
    list = Xmallocarray (nelements, sizeof (char *));
    if (!list) return False;

    start = Xmalloc ((datalen + 1) * sizeof (char));	/* for <NUL> */
    if (!start) {
	Xfree (list);
	return False;
    }

    /*
     * copy data
     */
    memcpy (start, (char *) tp->value, tp->nitems);
    start[datalen] = '\0';

    /*
     * walk down list setting value
     */
    for (cp = start, i = datalen + 1, j = 0; i > 0; cp++, i--) {
	if (*cp == '\0') {
	    list[j] = start;
	    start = (cp + 1);
	    j++;
	}
    }

    /*
     * append final null pointer and then return data
     */
    *list_return = list;
    *count_return = nelements;
    return True;
}


void XFreeStringList (char **list)
{
    if (list) {
	if (list[0]) Xfree (list[0]);
	Xfree (list);
	list = NULL;
    }
}

