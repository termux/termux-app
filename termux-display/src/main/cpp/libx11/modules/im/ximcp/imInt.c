/******************************************************************

           Copyright 1992, 1993, 1994 by FUJITSU LIMITED

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
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include "XimImSw.h"

static Xim 		*_XimCurrentIMlist  = (Xim *)NULL;
static int		 _XimCurrentIMcount = 0;

static Bool
_XimSetIMStructureList(
    Xim		  im)
{
    register int  i;
    Xim		 *xim;

    if(!(_XimCurrentIMlist)) {
	if(!(_XimCurrentIMlist = Xmalloc(sizeof(Xim))))
	    return False;
	_XimCurrentIMlist[0] = im;
	_XimCurrentIMcount   = 1;
    }
    else {
	for(i = 0; i < _XimCurrentIMcount; i++) {
	    if(!( _XimCurrentIMlist[i])) {
		_XimCurrentIMlist[i] = im;
		break;
	    }
	}
	if(i >= _XimCurrentIMcount) {
	    if(!(xim = Xrealloc(_XimCurrentIMlist,
					 ((i + 1) * sizeof(Xim)))))
		return False;
	    _XimCurrentIMlist			  = xim;
	    _XimCurrentIMlist[_XimCurrentIMcount] = im;
	    _XimCurrentIMcount++;
	}
    }
    return True;
}

void
_XimDestroyIMStructureList(Xim im)
{
    register int  i;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(_XimCurrentIMlist[i] == im) {
	    _XimCurrentIMlist[i] = NULL;
	    break;
	}
    }
    return;
}

void
_XimServerDestroy(Xim im_2_destroy)
{
    register int  i;
    Xim		  im;
    XIC		  ic;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(!(im = _XimCurrentIMlist[i]))
	    continue;
	/*
	 * Only continue if this im is the one to be destroyed.
	 */
	if (im != im_2_destroy)
	    continue;

	if (im->core.destroy_callback.callback)
	    (*im->core.destroy_callback.callback)((XIM)im,
			im->core.destroy_callback.client_data, NULL);
	for (ic = im->core.ic_chain; ic; ic = ic->core.next) {
	    if (ic->core.destroy_callback.callback) {
		(*ic->core.destroy_callback.callback)(ic,
			ic->core.destroy_callback.client_data, NULL);
	    }
	}
	_XimResetIMInstantiateCallback( im );
	(void)im->methods->close((XIM)im);
	Xfree(im);
	_XimCurrentIMlist[i] = NULL;
	return;
    }
}

#ifdef XIM_CONNECTABLE
void
_XimServerReconectableDestroy(void)
{
    register int  i;
    Xim		  im;
    XIC		  ic;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(!(im = _XimCurrentIMlist[i]))
	    continue;

	if (im->core.destroy_callback.callback)
	    (*im->core.destroy_callback.callback)(im,
			im->core.destroy_callback.client_data, NULL);
	for (ic = im->core.ic_chain; ic; ic = ic->core.next) {
	    if (ic->core.destroy_callback.callback) {
		(*ic->core.destroy_callback.callback)(ic,
			ic->core.destroy_callback.client_data, NULL);
	    }
	}
	_XimResetIMInstantiateCallback( im );
	(void)im->methods->close((XIM)im);
    }
    return;
}
#endif /* XIM_CONNECTABLE */

static const char *
_XimStrstr(
    register const char *src,
    register const char *dest)
{
    int			 len;

    len = strlen(dest);
    while((src = strchr(src, *dest))) {
	if(!strncmp(src, dest, len))
	    return src;
	src++;
    }
    return NULL;
}

static char *
_XimMakeImName(
    XLCd	   lcd)
{
    const char* begin = NULL;
    const char* end = NULL;
    char* ret = NULL;
    const char* ximmodifier = XIMMODIFIER;

    if(lcd->core->modifiers != NULL && *lcd->core->modifiers != '\0') {
	begin = _XimStrstr(lcd->core->modifiers, ximmodifier);
	if (begin != NULL) {
	    end = begin += strlen(ximmodifier);
	    while (*end && *end != '@')
		end++;
	}
    }
    ret = Xmalloc(end - begin + 1);
    if (ret != NULL) {
	if (begin != NULL && end != NULL) {
	    (void)strncpy(ret, begin, end - begin);
	    ret[end - begin] = '\0';
	} else {
	    ret[0] = '\0';
	}
    }

    return ret;
}

XIM
_XimOpenIM(
    XLCd		 lcd,
    Display		*dpy,
    XrmDatabase		 rdb,
    char		*res_name,
    char		*res_class)
{
    Xim			 im;
    register int	 i;

    if (!(im = Xcalloc(1, sizeof(XimRec))))
	return (XIM)NULL;

    im->core.lcd       = lcd;
    im->core.ic_chain  = (XIC)NULL;
    im->core.display   = dpy;
    im->core.rdb       = rdb;
    im->core.res_name  = NULL;
    im->core.res_class = NULL;
    if((res_name != NULL) && (*res_name != '\0')){
	if(!(im->core.res_name  = strdup(res_name)))
	    goto Error1;
    }
    if((res_class != NULL) && (*res_class != '\0')){
	if(!(im->core.res_class = strdup(res_class)))
	    goto Error2;
    }
    if(!(im->core.im_name = _XimMakeImName(lcd)))
	goto Error3;

    for(i= 0; ; i++) {
	if(_XimImSportRec[i].checkprocessing(im)) {
	    if(!(_XimImSportRec[i].im_open(im)))
		goto Error4;
	    if(!_XimSetIMStructureList(im))
		goto Error4;
	    return (XIM)im;
	}
    }

Error4 :
    _XimImSportRec[i].im_free(im);
    Xfree(im);
    return NULL;
Error3 :
    Xfree(im->core.im_name);
Error2:
    Xfree(im->core.res_class);
Error1:
    Xfree(im->core.res_name);
    Xfree(im);
    return NULL;
}

Bool
_XInitIM(XLCd lcd)
{
    if(lcd == (XLCd)NULL)
	return False;
    lcd->methods->open_im = _XimOpenIM;
    lcd->methods->register_callback = _XimRegisterIMInstantiateCallback;
    lcd->methods->unregister_callback = _XimUnRegisterIMInstantiateCallback;
    return True;
}
