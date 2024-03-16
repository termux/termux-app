
 /*
  * Copyright 1990, 1991 by OMRON Corporation
  *
  * Permission to use, copy, modify, distribute, and sell this software and its
  * documentation for any purpose is hereby granted without fee, provided that
  * the above copyright notice appear in all copies and that both that
  * copyright notice and this permission notice appear in supporting
  * documentation, and that the name OMRON not be used in
  * advertising or publicity pertaining to distribution of the software without
  * specific, written prior permission.  OMRON makes no representations
  * about the suitability of this software for any purpose.  It is provided
  * "as is" without express or implied warranty.
  *
  * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
  * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
  * EVENT SHALL OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
  * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
  * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
  * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
  * PERFORMANCE OF THIS SOFTWARE.
  *
  *	Author:	Seiji Kuwari	OMRON Corporation
  *				kuwa@omron.co.jp
  *				kuwa%omron.co.jp@uunet.uu.net
  */

/*

Copyright 1991, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define XLOCALE 1
#endif
#include "Xlibint.h"
#if XLOCALE
#include "Xlcint.h"
#endif

extern long const _Xevent_to_mask[];

/*
 * Look up if there is a specified filter for the event.
 */
Bool
XFilterEvent(
    XEvent *ev,
    Window window)
{
#if XLOCALE
    XFilterEventList	p;
    Window		win;
    long		mask;
    Bool		ret;

    if (window)
	win = window;
    else
	win = ev->xany.window;
    if (ev->type >= LASTEvent)
	mask = 0;
    else
	mask = _Xevent_to_mask[ev->type];

    LockDisplay(ev->xany.display);
    for (p = ev->xany.display->im_filters; p != NULL; p = p->next) {
	if (win == p->window) {
	    if ((mask & p->event_mask) ||
		(ev->type >= p->start_type && ev->type <= p->end_type)) {
		UnlockDisplay(ev->xany.display);
		ret = (*(p->filter))(ev->xany.display, p->window, ev,
				      p->client_data);
		return(ret);
	    }
	}
    }
    UnlockDisplay(ev->xany.display);
#endif
    return(False);
}
