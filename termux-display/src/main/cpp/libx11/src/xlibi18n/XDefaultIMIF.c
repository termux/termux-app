/*
Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions: The above copyright notice and this
permission notice shall be included in all copies or substantial
portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
EVEN IF ADVISED IN ADVANCE OF THE POSSIBILITY OF SUCH DAMAGES.


Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


X Window System is a trademark of The Open Group

OSF/1, OSF/Motif and Motif are registered trademarks, and OSF, the OSF
logo, LBX, X Window System, and Xinerama are trademarks of the Open
Group. All other trademarks and registered trademarks mentioned herein
are the property of their respective owners. No right, title or
interest in or to any trademark, service mark, logo or trade name of
Sun Microsystems, Inc. or its licensors is granted.

*/
/*
 * Copyright (c) 2000, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcGeneric.h"
#include "reallocarray.h"

#ifndef MAXINT
#define MAXINT          (~((unsigned int)1 << (8 * sizeof(int)) - 1))
#endif /* !MAXINT */

typedef struct _StaticXIM *StaticXIM;

typedef struct _XIMStaticXIMRec {
    /* for CT => MB,WC converter */
    XlcConv		 ctom_conv;
    XlcConv		 ctow_conv;
} XIMStaticXIMRec;

typedef enum {
    CREATE_IC = 1,
    SET_ICVAL = 2,
    GET_ICVAL = 3
} XICOp_t;

typedef struct _StaticXIM {
    XIMMethods		methods;
    XIMCoreRec		core;
    XIMStaticXIMRec	*private;
} StaticXIMRec;

static Status _CloseIM(
	XIM
);

static char *_SetIMValues(
	XIM, XIMArg *
);

static char *_GetIMValues(
	XIM, XIMArg*
);

static XIC _CreateIC(
	XIM, XIMArg*
);

static _Xconst XIMMethodsRec local_im_methods = {
    _CloseIM,		/* close */
    _SetIMValues,	/* set_values */
    _GetIMValues, 	/* get_values */
    _CreateIC,		/* create_ic */
    NULL,		/* ctstombs */
    NULL		/* ctstowcs */
};

static void _DestroyIC(
		       XIC
);
static void _SetFocus(
		      XIC
);
static void _UnsetFocus(
			XIC
);
static char* _SetICValues(
			 XIC, XIMArg *
);
static char* _GetICValues(
			 XIC, XIMArg *
);
static char *_MbReset(
		      XIC
);
static wchar_t *_WcReset(
			 XIC
);
static int _MbLookupString(
	XIC, XKeyEvent *, char *, int, KeySym *, Status *
);
static int _WcLookupString(
	XIC, XKeyEvent *, wchar_t *, int, KeySym *, Status *
);

static _Xconst XICMethodsRec local_ic_methods = {
    _DestroyIC, 	/* destroy */
    _SetFocus,		/* set_focus */
    _UnsetFocus,	/* unset_focus */
    _SetICValues,	/* set_values */
    _GetICValues,	/* get_values */
    _MbReset,		/* mb_reset */
    _WcReset,		/* wc_reset */
    NULL,		/* utf8_reset */		/* ??? */
    _MbLookupString,	/* mb_lookup_string */
    _WcLookupString,	/* wc_lookup_string */
    NULL		/* utf8_lookup_string */	/* ??? */
};

XIM
_XDefaultOpenIM(
    XLCd                lcd,
    Display             *dpy,
    XrmDatabase         rdb,
    char                *res_name,
    char                *res_class)
{
    StaticXIM im;
    int i;
    char *mod;
    char buf[BUFSIZ];

    if ((im = Xcalloc(1, sizeof(StaticXIMRec))) == NULL)
        return NULL;

    if ((im->private = Xcalloc(1, sizeof(XIMStaticXIMRec))) == NULL)
        goto Error;

    if ((im->private->ctom_conv = _XlcOpenConverter(lcd, XlcNCompoundText,
                                                    lcd, XlcNMultiByte))
        == NULL)
        goto Error;

    if ((im->private->ctow_conv = _XlcOpenConverter(lcd, XlcNCompoundText,
                                                    lcd, XlcNWideChar))
        == NULL)
        goto Error;

    buf[0] = '\0';
    i = 0;
    if ((lcd->core->modifiers) && (*lcd->core->modifiers)) {
#define	MODIFIER "@im="
	mod = strstr(lcd->core->modifiers, MODIFIER);
	if (mod) {
	    mod += strlen(MODIFIER);
	    while (*mod && *mod != '@' && i < BUFSIZ - 1) {
		buf[i++] = *mod++;
	    }
	    buf[i] = '\0';
	}
    }
#undef MODIFIER
    if ((im->core.im_name = strdup(buf)) == NULL)
	goto Error;

    im->methods        = (XIMMethods)&local_im_methods;
    im->core.lcd       = lcd;
    im->core.ic_chain  = (XIC)NULL;
    im->core.display   = dpy;
    im->core.rdb       = rdb;
    im->core.res_name  = NULL;
    im->core.res_class = NULL;

    if ((res_name != NULL) && (*res_name != '\0')){
	im->core.res_name  = strdup(res_name);
    }
    if ((res_class != NULL) && (*res_class != '\0')){
	im->core.res_class = strdup(res_class);
    }

    return (XIM)im;

  Error:
    _CloseIM((XIM)im);
    Xfree(im);
    return(NULL);
}

static Status
_CloseIM(XIM xim)
{
    StaticXIM im = (StaticXIM)xim;

    if (im->private->ctom_conv != NULL)
        _XlcCloseConverter(im->private->ctom_conv);
    if (im->private->ctow_conv != NULL)
        _XlcCloseConverter(im->private->ctow_conv);
    XFree(im->private);
    XFree(im->core.im_name);
    XFree(im->core.res_name);
    XFree(im->core.res_class);
    return 1;
}

static char *
_SetIMValues(
    XIM xim,
    XIMArg *arg)
{
    return(arg->name);		/* evil */
}

static char *
_GetIMValues(
    XIM xim,
    XIMArg *values)
{
    XIMArg *p;
    XIMStyles *styles;

    for (p = values; p->name != NULL; p++) {
	if (strcmp(p->name, XNQueryInputStyle) == 0) {
	    styles = Xmalloc(sizeof(XIMStyles));
	    *(XIMStyles **)p->value = styles;
	    styles->count_styles = 1;
	    styles->supported_styles =
		Xmallocarray(styles->count_styles, sizeof(XIMStyle));
	    styles->supported_styles[0] = (XIMPreeditNone | XIMStatusNone);
	} else {
	    break;
	}
    }
    return (p->name);
}

static char*
_SetICValueData(XIC ic, XIMArg *values, XICOp_t mode)
{
    XIMArg *p;
    char *return_name = NULL;

    for (p = values; p != NULL && p->name != NULL; p++) {
	if(strcmp(p->name, XNInputStyle) == 0) {
	    if (mode == CREATE_IC)
		ic->core.input_style = (XIMStyle)p->value;
	} else if (strcmp(p->name, XNClientWindow) == 0) {
	    ic->core.client_window = (Window)p->value ;
	} else if (strcmp(p->name, XNFocusWindow) == 0) {
	    ic->core.focus_window = (Window)p->value ;
	} else if (strcmp(p->name, XNPreeditAttributes) == 0
		   || strcmp(p->name, XNStatusAttributes) == 0) {
            return_name = _SetICValueData(ic, (XIMArg*)p->value, mode);
            if (return_name) break;
        } else {
            return_name = p->name;
            break;
        }
    }
    return(return_name);
}

static char*
_GetICValueData(XIC ic, XIMArg *values, XICOp_t mode)
{
    XIMArg *p;
    char *return_name = NULL;

    for (p = values; p->name != NULL; p++) {
	if(strcmp(p->name, XNInputStyle) == 0) {
	    *((XIMStyle *)(p->value)) = ic->core.input_style;
	} else if (strcmp(p->name, XNClientWindow) == 0) {
	    *((Window *)(p->value)) = ic->core.client_window;
	} else if (strcmp(p->name, XNFocusWindow) == 0) {
	    *((Window *)(p->value)) = ic->core.focus_window;
	} else if (strcmp(p->name, XNFilterEvents) == 0) {
	    *((unsigned long *)(p->value))= ic->core.filter_events;
	} else if (strcmp(p->name, XNPreeditAttributes) == 0
		   || strcmp(p->name, XNStatusAttributes) == 0) {
	    return_name = _GetICValueData(ic, (XIMArg*)p->value, mode);
	    if (return_name) break;
	} else {
	    return_name = p->name;
	    break;
	}
    }
    return(return_name);
}

static XIC
_CreateIC(XIM im, XIMArg *arg)
{
    XIC ic;

    if ((ic = Xcalloc(1, sizeof(XICRec))) == (XIC)NULL) {
	return ((XIC)NULL);
    }

    ic->methods = (XICMethods)&local_ic_methods;
    ic->core.im = im;
    ic->core.filter_events = KeyPressMask;

    if (_SetICValueData(ic, arg, CREATE_IC) != NULL)
	goto err_return;
    if (!(ic->core.input_style))
	goto err_return;

    return (XIC)ic;
err_return:
    XFree(ic);
    return ((XIC)NULL);
}

static void
_DestroyIC(XIC ic)
{
/*BugId4255571. This Xfree() should be removed because XDestroyIC() still need ic after invoking _DestroyIC() and there is a XFree(ic) at the end of XDestroyIC() already.
   if(ic)
   	XFree(ic); */
}

static void
_SetFocus(XIC ic)
{
}

static void
_UnsetFocus(XIC ic)
{
}

static char*
_SetICValues(XIC ic, XIMArg *args)
{
    char *ret = NULL;
    if (!ic) {
        return (args->name);
    }
    ret = _SetICValueData(ic, args, SET_ICVAL);
    return(ret);
}

static char*
_GetICValues(XIC ic, XIMArg *args)
{
    char *ret = NULL;
    if (!ic) {
        return (args->name);
    }
    ret = _GetICValueData(ic, args, GET_ICVAL);
    return(ret);
}

static char *
_MbReset(XIC xic)
{
    return(NULL);
}

static wchar_t *
_WcReset(XIC xic)
{
    return(NULL);
}

static int
_MbLookupString(
    XIC xic,
    XKeyEvent *ev,
    char * buffer,
    int bytes,
    KeySym *keysym,
    Status *status)
{
    XComposeStatus NotSupportedYet ;
    int length;

    length = XLookupString(ev, buffer, bytes, keysym, &NotSupportedYet);

    if (keysym && *keysym == NoSymbol){
	*status = XLookupNone;
    } else if (length > 0) {
	*status = XLookupBoth;
    } else {
	*status = XLookupKeySym;
    }
    return(length);
}

static int
_WcLookupString(
    XIC xic,
    XKeyEvent *ev,
    wchar_t * buffer,
    int wlen,
    KeySym *keysym,
    Status *status)
{
    XComposeStatus NotSupportedYet ;
    int length;
    /* In single-byte, mb_len = wc_len */
    char *mb_buf = Xmalloc(wlen);

    length = XLookupString(ev, mb_buf, wlen, keysym, &NotSupportedYet);

    if (keysym && *keysym == NoSymbol){
	*status = XLookupNone;
    } else if (length > 0) {
	*status = XLookupBoth;
    } else {
	*status = XLookupKeySym;
    }
    mbstowcs(buffer, mb_buf, (size_t) length);
    XFree(mb_buf);
    return(length);
}
