/******************************************************************

          Copyright 1992, 1993, 1994 by FUJITSU LIMITED
          Copyright 1993 by Digital Equipment Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED and
Digital Equipment Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.  FUJITSU LIMITED and Digital Equipment Corporation
makes no representations about the suitability of this software for
any purpose.  It is provided "as is" without express or implied
warranty.

FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

  Author:    Takashi Fujiwara     FUJITSU LIMITED
                               	  fujiwara@a80.tech.yk.fujitsu.co.jp
  Modifier:  Franky Ling          Digital Equipment Corporation
	                          frankyling@hgrd01.enet.dec.com

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"

static void
_XimThaiUnSetFocus(
    XIC	 xic)
{
    Xic  ic = (Xic)xic;
    ((Xim)ic->core.im)->private.local.current_ic = (XIC)NULL;

    if (ic->core.focus_window)
	_XUnregisterFilter(ic->core.im->core.display, ic->core.focus_window,
			_XimThaiFilter, (XPointer)ic);
    return;
}

static void
_XimThaiDestroyIC(
    XIC	 xic)
{
    Xic	 ic = (Xic)xic;
    DefTreeBase *b = &ic->private.local.base;

    if(((Xim)ic->core.im)->private.local.current_ic == (XIC)ic) {
	_XimThaiUnSetFocus(xic);
    }

    Xfree(ic->private.local.ic_resources);
    ic->private.local.ic_resources = NULL;

    Xfree (b->tree);
    b->tree = NULL;

    Xfree (b->mb);
    b->mb   = NULL;

    Xfree (b->wc);
    b->wc   = NULL;

    Xfree (b->utf8);
    b->utf8 = NULL;
    return;
}

static void
_XimThaiSetFocus(
    XIC	 xic)
{
    Xic	 ic = (Xic)xic;
    XIC	 current_ic = ((Xim)ic->core.im)->private.local.current_ic;

    if (current_ic == (XIC)ic)
	return;

    if (current_ic != (XIC)NULL) {
	_XimThaiUnSetFocus(current_ic);
    }
    ((Xim)ic->core.im)->private.local.current_ic = (XIC)ic;

    if (ic->core.focus_window)
	_XRegisterFilterByType(ic->core.im->core.display, ic->core.focus_window,
			KeyPress, KeyPress, _XimThaiFilter, (XPointer)ic);
    return;
}

static void
_XimThaiReset(
    XIC	 xic)
{
    Xic	 ic = (Xic)xic;
    DefTreeBase *b   = &ic->private.local.base;
    ic->private.local.thai.comp_state = 0;
    ic->private.local.thai.keysym = 0;
    b->mb[b->tree[ic->private.local.composed].mb] = '\0';
    b->wc[b->tree[ic->private.local.composed].wc] = '\0';
    b->utf8[b->tree[ic->private.local.composed].utf8] = '\0';
}

static char *
_XimThaiMbReset(
    XIC	 xic)
{
    _XimThaiReset(xic);
    return (char *)NULL;
}

static wchar_t *
_XimThaiWcReset(
    XIC	 xic)
{
    _XimThaiReset(xic);
    return (wchar_t *)NULL;
}

static XICMethodsRec Thai_ic_methods = {
    _XimThaiDestroyIC, 	/* destroy */
    _XimThaiSetFocus,  	/* set_focus */
    _XimThaiUnSetFocus,	/* unset_focus */
    _XimLocalSetICValues,	/* set_values */
    _XimLocalGetICValues,	/* get_values */
    _XimThaiMbReset,		/* mb_reset */
    _XimThaiWcReset,		/* wc_reset */
    _XimThaiMbReset,		/* utf8_reset */
    _XimLocalMbLookupString,	/* mb_lookup_string */
    _XimLocalWcLookupString,	/* wc_lookup_string */
    _XimLocalUtf8LookupString	/* utf8_lookup_string */
};

XIC
_XimThaiCreateIC(
    XIM			 im,
    XIMArg		*values)
{
    Xic			 ic;
    XimDefICValues	 ic_values;
    XIMResourceList	 res;
    unsigned int	 num;
    int			 len;
    DefTree             *tree;

    if((ic = Xcalloc(1, sizeof(XicRec))) == (Xic)NULL) {
	return ((XIC)NULL);
    }

    ic->methods = &Thai_ic_methods;
    ic->core.im = im;
    ic->core.filter_events = KeyPressMask;

    if (! (ic->private.local.base.tree = tree = Xmalloc(sizeof(DefTree)*3)) )
	goto Set_Error;
    if (! (ic->private.local.base.mb = Xmalloc(21)) )
	goto Set_Error;
    if (! (ic->private.local.base.wc = Xmalloc(sizeof(wchar_t)*21)) )
	goto Set_Error;
    if (! (ic->private.local.base.utf8 = Xmalloc(21)) )
	goto Set_Error;
    ic->private.local.context = 1;
    tree[1].mb   = 1;
    tree[1].wc   = 1;
    tree[1].utf8 = 1;
    ic->private.local.composed = 2;
    tree[2].mb   = 11;
    tree[2].wc   = 11;
    tree[2].utf8 = 11;

    ic->private.local.thai.comp_state = 0;
    ic->private.local.thai.keysym = 0;
    ic->private.local.thai.input_mode = 0;

    num = im->core.ic_num_resources;
    len = sizeof(XIMResource) * num;
    if((res = Xmalloc(len)) == (XIMResourceList)NULL) {
	goto Set_Error;
    }
    (void)memcpy((char *)res, (char *)im->core.ic_resources, len);
    ic->private.local.ic_resources     = res;
    ic->private.local.ic_num_resources = num;

    bzero((char *)&ic_values, sizeof(XimDefICValues));
    if(_XimCheckLocalInputStyle(ic, (XPointer)&ic_values, values,
				 im->core.styles, res, num) == False) {
	goto Set_Error;
    }

    _XimSetICMode(res, num, ic_values.input_style);

    if(_XimSetICValueData(ic, (XPointer)&ic_values,
			ic->private.local.ic_resources,
			ic->private.local.ic_num_resources,
			values, XIM_CREATEIC, True)) {
	goto Set_Error;
    }
    if(_XimSetICDefaults(ic, (XPointer)&ic_values,
				XIM_SETICDEFAULTS, res, num) == False) {
	goto Set_Error;
    }
    ic_values.filter_events = KeyPressMask;
    _XimSetCurrentICValues(ic, &ic_values);

    return ((XIC)ic);

Set_Error :
    if (ic->private.local.ic_resources) {
	Xfree(ic->private.local.ic_resources);
    }
    Xfree(ic);
    return((XIC)NULL);
}
