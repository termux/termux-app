/******************************************************************

                Copyright 1993, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
FUJITSU LIMITED makes no representations about the suitability of
this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlib.h>
#include "Xlibint.h"
#include "Xutil.h"
#include "Xlcint.h"
#include "Ximint.h"


Bool
_XimRegProtoIntrCallback(
    Xim		 im,
    CARD16	 major_code,
    CARD16	 minor_code,
    Bool	(*proc)(
                        Xim, INT16, XPointer, XPointer
			),

    XPointer	 call_data)
{
    XimProtoIntrRec    *rec;

    if (!(rec = Xmalloc(sizeof(XimProtoIntrRec))))
        return False;
    rec->func       = proc;
    rec->major_code = major_code;
    rec->minor_code = minor_code;
    rec->call_data  = call_data;
    rec->next       = im->private.proto.intrproto;
    im->private.proto.intrproto = rec;
    return True;
}

void
_XimFreeProtoIntrCallback(Xim im)
{
    register XimProtoIntrRec *rec, *next;

    for (rec = im->private.proto.intrproto; rec;) {
	next = rec->next;
	Xfree(rec);
	rec = next;
    }
    im->private.proto.intrproto = NULL;
    return;
}

static Bool
_XimTransportIntr(
    Xim		 im,
    INT16	 len,
    XPointer	 data,
    XPointer	 call_data)
{
    Xim			 call_im = (Xim)call_data;
    XimProtoIntrRec	*rec = call_im->private.proto.intrproto;
    CARD8		 major_opcode = *((CARD8 *)data);
    CARD8		 minor_opcode = *((CARD8 *)data + 1);

    for (; rec; rec = rec->next) {
	if ((major_opcode == (CARD8)rec->major_code)
	 && (minor_opcode == (CARD8)rec->minor_code))
	    if ((*rec->func)(call_im, len, data, rec->call_data))
		return True;
    }
    return False;
}

Bool
_XimDispatchInit(Xim im)
{
    if (_XimRegisterDispatcher(im, _XimTransportIntr, (XPointer)im))
	return True;
    return False;
}
