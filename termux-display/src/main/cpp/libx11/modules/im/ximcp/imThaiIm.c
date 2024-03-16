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
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPublic.h"
#include "XlcPubI.h"
#include "Ximint.h"

static XIMMethodsRec      Xim_im_thai_methods = {
    _XimThaiCloseIM,           /* close */
    _XimLocalSetIMValues,      /* set_values */
    _XimLocalGetIMValues,      /* get_values */
    _XimThaiCreateIC,          /* create_ic */
    _XimLcctstombs,            /* ctstombs */
    _XimLcctstowcs,            /* ctstowcs */
    _XimLcctstoutf8            /* ctstoutf8 */
};

#define THAI_LANGUAGE_NAME 	"th"

Bool
_XimCheckIfThaiProcessing(Xim im)
{
    char *language;

    _XGetLCValues(im->core.lcd, XlcNLanguage, &language, NULL);
    if(strcmp(language, THAI_LANGUAGE_NAME) == 0 &&
       (strcmp(im->core.im_name, "") == 0 ||
        strcmp(im->core.im_name, "BasicCheck") == 0 ||
        strcmp(im->core.im_name, "Strict") == 0 ||
        strcmp(im->core.im_name, "Thaicat") == 0 ||
        strcmp(im->core.im_name, "Passthrough") == 0))
    {
	return(True);
    }
    return(False);
}

Bool
_XimThaiOpenIM(Xim im)
{
    XLCd		 lcd = im->core.lcd;
    XlcConv		 conv;
    XimDefIMValues	 im_values;
    XimLocalPrivateRec*  private = &im->private.local;

    _XimInitialResourceInfo();
    if(_XimSetIMResourceList(&im->core.im_resources,
		 		&im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    if(_XimSetICResourceList(&im->core.ic_resources,
				&im->core.ic_num_resources) == False) {
	goto Open_Error;
    }

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);

    _XimGetCurrentIMValues(im, &im_values);
    if(_XimSetLocalIMDefaults(im, (XPointer)&im_values,
		im->core.im_resources, im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    _XimSetCurrentIMValues(im, &im_values);

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNMultiByte)))
	goto Open_Error;
    private->ctom_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNWideChar)))
	goto Open_Error;
    private->ctow_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->ctoutf8_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNMultiByte)))
	goto Open_Error;
    private->cstomb_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNWideChar)))
	goto Open_Error;
    private->cstowc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->cstoutf8_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNChar)))
	goto Open_Error;
    private->ucstoc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->ucstoutf8_conv = conv;

    im->methods = &Xim_im_thai_methods;
    private->current_ic = (XIC)NULL;

    return(True);

Open_Error :
    _XimThaiIMFree(im);
    return(False);
}

void
_XimThaiIMFree(Xim im)
{
    if(im->core.im_resources) {
	Xfree(im->core.im_resources);
	im->core.im_resources = NULL;
    }
    if(im->core.ic_resources) {
	Xfree(im->core.ic_resources);
	im->core.ic_resources = NULL;
    }
    if(im->core.im_values_list) {
	Xfree(im->core.im_values_list);
	im->core.im_values_list = NULL;
    }
    if(im->core.ic_values_list) {
	Xfree(im->core.ic_values_list);
	im->core.ic_values_list = NULL;
    }
    if(im->core.styles) {
	Xfree(im->core.styles);
	im->core.styles = NULL;
    }
    if(im->core.res_name) {
	Xfree(im->core.res_name);
	im->core.res_name = NULL;
    }
    if(im->core.res_class) {
	Xfree(im->core.res_class);
	im->core.res_class = NULL;
    }
    if(im->core.im_name) {
	Xfree(im->core.im_name);
	im->core.im_name = NULL;
    }
    if (im->private.local.ctom_conv) {
	_XlcCloseConverter(im->private.local.ctom_conv);
	im->private.local.ctom_conv = NULL;
    }
    if (im->private.local.ctow_conv) {
	_XlcCloseConverter(im->private.local.ctow_conv);
	im->private.local.ctow_conv = NULL;
    }
    if (im->private.local.ctoutf8_conv) {
	_XlcCloseConverter(im->private.local.ctoutf8_conv);
	im->private.local.ctoutf8_conv = NULL;
    }
    if (im->private.local.cstomb_conv) {
	_XlcCloseConverter(im->private.local.cstomb_conv);
	im->private.local.cstomb_conv = NULL;
    }
    if (im->private.local.cstowc_conv) {
	_XlcCloseConverter(im->private.local.cstowc_conv);
	im->private.local.cstowc_conv = NULL;
    }
    if (im->private.local.cstoutf8_conv) {
	_XlcCloseConverter(im->private.local.cstoutf8_conv);
	im->private.local.cstoutf8_conv = NULL;
    }
    if (im->private.local.ucstoc_conv) {
	_XlcCloseConverter(im->private.local.ucstoc_conv);
	im->private.local.ucstoc_conv = NULL;
    }
    if (im->private.local.ucstoutf8_conv) {
	_XlcCloseConverter(im->private.local.ucstoutf8_conv);
	im->private.local.ucstoutf8_conv = NULL;
    }
    return;
}

Status
_XimThaiCloseIM(XIM xim)
{
    Xim		im = (Xim)xim;
    XIC		ic;
    XIC		next;

    ic = im->core.ic_chain;
    im->core.ic_chain = NULL;
    while (ic) {
	(*ic->methods->destroy) (ic);
	next = ic->core.next;
	Xfree (ic);
	ic = next;
    }
    _XimThaiIMFree(im);
    return(True);
}
