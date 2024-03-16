/******************************************************************

                Copyright 1992,1993, 1994 by FUJITSU LIMITED

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
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"

char *
_XimLocalGetICValues(XIC xic, XIMArg *values)
{
    Xic			 ic = (Xic)xic;
    XimDefICValues	 ic_values;

    _XimGetCurrentICValues(ic, &ic_values);
    return(_XimGetICValueData(ic, (XPointer)&ic_values,
				ic->private.local.ic_resources,
				ic->private.local.ic_num_resources,
				values, XIM_GETICVALUES));
}
