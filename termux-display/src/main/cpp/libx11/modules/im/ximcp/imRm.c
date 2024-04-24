/******************************************************************

	  Copyright 1990, 1991, 1992,1993, 1994 by FUJITSU LIMITED
	  Copyright 1994                        by Sony Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
and Sony Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission. FUJITSU LIMITED and Sony Corporation make
no representations about the suitability of this software for any
purpose. It is provided "as is" without express or implied warranty.

FUJITSU LIMITED AND SONY CORPORATION DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJITSU LIMITED AND
SONY CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED
			       fujiwara@a80.tech.yk.fujitsu.co.jp
  Modifier: Makoto Wakamatsu   Sony Corporation
			       makoto@sm.sony.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xlib.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include "Xresource.h"

#define GET_NAME(x) name_table + x.name_offset

typedef struct _XimValueOffsetInfo {
    unsigned short name_offset;
    XrmQuark		 quark;
    unsigned int	 offset;
    Bool		 (*defaults)(
	struct _XimValueOffsetInfo *, XPointer, XPointer, unsigned long
			 );
    Bool		 (*encode)(
	struct _XimValueOffsetInfo *, XPointer, XPointer
			 );
    Bool		 (*decode)(
	struct _XimValueOffsetInfo *, XPointer, XPointer
			 );
} XimValueOffsetInfoRec, *XimValueOffsetInfo;

#ifdef XIM_CONNECTABLE
static Bool
_XimCheckBool(str)
    char	*str;
{
    if(!strcmp(str, "True") || !strcmp(str, "true") ||
       !strcmp(str, "Yes")  || !strcmp(str, "yes")  ||
       !strcmp(str, "ON")   || !strcmp(str, "on"))
	return True;
    return False;
}

void
_XimSetProtoResource(im)
    Xim		 im;
{
    char	res_name_buf[256];
    char*	res_name;
    size_t	res_name_len;
    char	res_class_buf[256];
    char*	res_class;
    size_t	res_class_len;
    char*	str_type;
    XrmValue	value;
    XIMStyle	preedit_style = 0;
    XIMStyle	status_style = 0;
    XIMStyles*	imstyles;
    char*	dotximdot = ".xim.";
    char*	ximdot = "xim.";
    char*	dotXimdot = ".Xim.";
    char*	Ximdot = "Xim.";

    if (!im->core.rdb)
	return;

    res_name_len = strlen (im->core.res_name);
    if (res_name_len < 200) {
	res_name = res_name_buf;
	res_name_len = sizeof(res_name_buf);
    }
    else {
	res_name_len += 50;
	res_name = Xmalloc (res_name_len);
    }
    res_class_len = strlen (im->core.res_class);
    if (res_class_len < 200) {
	res_class = res_class_buf;
	res_class_len = sizeof(res_class_buf);
    }
    else {
	res_class_len += 50;
	res_class = Xmalloc (res_class_len);
    }
    /* pretend malloc always works */

    (void) snprintf (res_name, res_name_len, "%s%s%s",
	im->core.res_name != NULL ? im->core.res_name : "*",
	im->core.res_name != NULL ? dotximdot : ximdot,
	"useAuth");
    (void) snprintf (res_class, res_class_len, "%s%s%s",
	im->core.res_class != NULL ? im->core.res_class : "*",
	im->core.res_class != NULL ? dotXimdot : Ximdot,
	"UseAuth");
    bzero(&value, sizeof(XrmValue));
    if(XrmGetResource(im->core.rdb, res_name, res_class, &str_type, &value)) {
	if(_XimCheckBool(value.addr)) {
	    MARK_USE_AUTHORIZATION_FUNC(im);
	}
    }

    (void) snprintf (res_name, res_name_len, "%s%s%s",
	im->core.res_name != NULL ? im->core.res_name : "*",
	im->core.res_name != NULL ? dotximdot : ximdot,
	"delaybinding");
    (void) snprintf (res_class, res_class_len, "%s%s%s",
	im->core.res_class != NULL ? im->core.res_class : "*",
	im->core.res_class != NULL ? dotXimdot : Ximdot,
	"Delaybinding");
    bzero(&value, sizeof(XrmValue));
    if(XrmGetResource(im->core.rdb, res_name, res_class, &str_type, &value)) {
	if(_XimCheckBool(value.addr)) {
	    MARK_DELAYBINDABLE(im);
	}
    }

    (void) snprintf (res_name, res_name_len, "%s%s%s",
	im->core.res_name != NULL ? im->core.res_name : "*",
	im->core.res_name != NULL ? dotximdot : ximdot,
	"reconnect");
    (void) snprintf (res_class, res_class_len, "%s%s%s",
	im->core.res_class != NULL ? im->core.res_class : "*",
	im->core.res_class != NULL ? dotXimdot : Ximdot,
	"Reconnect");
    bzero(&value, sizeof(XrmValue));
    if(XrmGetResource(im->core.rdb, res_name, res_class, &str_type, &value)) {
	if(_XimCheckBool(value.addr)) {
	    MARK_RECONNECTABLE(im);
	}
    }

    if(!IS_CONNECTABLE(im)) {
	if (res_name != res_name_buf) Xfree (res_name);
	if (res_class != res_class_buf) Xfree (res_class);
	return;
    }

    (void) snprintf (res_name, res_name_len, "%s%s%s",
	im->core.res_name != NULL ? im->core.res_name : "*",
	im->core.res_name != NULL ? dotximdot : ximdot,
	"preeditDefaultStyle");
    (void) snprintf (res_class, res_class_len, "%s%s%s",
	im->core.res_class != NULL ? im->core.res_class : "*",
	im->core.res_class != NULL ? dotXimdot : Ximdot,
	"PreeditDefaultStyle");
    if(XrmGetResource(im->core.rdb, res_name, res_class, &str_type, &value)) {
	if(!strcmp(value.addr, "XIMPreeditArea"))
	    preedit_style = XIMPreeditArea;
	else if(!strcmp(value.addr, "XIMPreeditCallbacks"))
	    preedit_style = XIMPreeditCallbacks;
	else if(!strcmp(value.addr, "XIMPreeditPosition"))
	    preedit_style = XIMPreeditPosition;
	else if(!strcmp(value.addr, "XIMPreeditNothing"))
	    preedit_style = XIMPreeditNothing;
	else if(!strcmp(value.addr, "XIMPreeditNone"))
	    preedit_style = XIMPreeditNone;
    }
    if(!preedit_style)
	preedit_style = XIMPreeditNothing;

    (void) snprintf (res_name, res_name_len, "%s%s%s",
	im->core.res_name != NULL ? im->core.res_name : "*",
	im->core.res_name != NULL ? dotximdot : ximdot,
	"statusDefaultStyle");
    (void) snprintf (res_class, res_class_len, "%s%s%s",
	im->core.res_class != NULL ? im->core.res_class : "*",
	im->core.res_class != NULL ? dotXimdot : Ximdot,
	"StatusDefaultStyle");
    if(XrmGetResource(im->core.rdb, res_name, res_class, &str_type, &value)) {
	if(!strcmp(value.addr, "XIMStatusArea"))
	    status_style = XIMStatusArea;
	else if(!strcmp(value.addr, "XIMStatusCallbacks"))
	    status_style = XIMStatusCallbacks;
	else if(!strcmp(value.addr, "XIMStatusNothing"))
	    status_style = XIMStatusNothing;
	else if(!strcmp(value.addr, "XIMStatusNone"))
	    status_style = XIMStatusNone;
    }
    if(!status_style)
	status_style = XIMStatusNothing;

    if(!(imstyles = Xmalloc(sizeof(XIMStyles) + sizeof(XIMStyle)))){
	if (res_name != res_name_buf) Xfree (res_name);
	if (res_class != res_class_buf) Xfree (res_class);
	return;
    }
    imstyles->count_styles = 1;
    imstyles->supported_styles =
			(XIMStyle *)((char *)imstyles + sizeof(XIMStyles));
    imstyles->supported_styles[0] = preedit_style | status_style;
    im->private.proto.default_styles = imstyles;
    if (res_name != res_name_buf) Xfree (res_name);
    if (res_class != res_class_buf) Xfree (res_class);
}
#endif /* XIM_CONNECTABLE */

static const char name_table[] =
    /*   0 */ XNQueryInputStyle"\0"
    /*  16 */ XNClientWindow"\0"
    /*  29 */ XNInputStyle"\0"
    /*  40 */ XNFocusWindow"\0"
    /*  52 */ XNResourceName"\0"
    /*  65 */ XNResourceClass"\0"
    /*  79 */ XNGeometryCallback"\0"
    /*  96 */ XNDestroyCallback"\0"
    /* 112 */ XNFilterEvents"\0"
    /* 125 */ XNPreeditStartCallback"\0"
    /* 146 */ XNPreeditDoneCallback"\0"
    /* 166 */ XNPreeditDrawCallback"\0"
    /* 186 */ XNPreeditCaretCallback"\0"
    /* 207 */ XNPreeditStateNotifyCallback"\0"
    /* 234 */ XNPreeditAttributes"\0"
    /* 252 */ XNStatusStartCallback"\0"
    /* 272 */ XNStatusDoneCallback"\0"
    /* 291 */ XNStatusDrawCallback"\0"
    /* 310 */ XNStatusAttributes"\0"
    /* 327 */ XNArea"\0"
    /* 332 */ XNAreaNeeded"\0"
    /* 343 */ XNSpotLocation"\0"
    /* 356 */ XNColormap"\0"
    /* 365 */ XNStdColormap"\0"
    /* 377 */ XNForeground"\0"
    /* 388 */ XNBackground"\0"
    /* 399 */ XNBackgroundPixmap"\0"
    /* 416 */ XNFontSet"\0"
    /* 424 */ XNLineSpace"\0"
    /* 434 */ XNCursor"\0"
    /* 441 */ XNQueryIMValuesList"\0"
    /* 459 */ XNQueryICValuesList"\0"
    /* 477 */ XNVisiblePosition"\0"
    /* 493 */ XNStringConversionCallback"\0"
    /* 518 */ XNStringConversion"\0"
    /* 535 */ XNResetState"\0"
    /* 546 */ XNHotKey"\0"
    /* 553 */ XNHotKeyState"\0"
    /* 565 */ XNPreeditState
;

#define OFFSET_XNQUERYINPUTSTYLE 0
#define OFFSET_XNCLIENTWINDOW 16
#define OFFSET_XNINPUTSTYLE 29
#define OFFSET_XNFOCUSWINDOW 40
#define OFFSET_XNRESOURCENAME 52
#define OFFSET_XNRESOURCECLASS 65
#define OFFSET_XNGEOMETRYCALLBACK 79
#define OFFSET_XNDESTROYCALLBACK 96
#define OFFSET_XNFILTEREVENTS 112
#define OFFSET_XNPREEDITSTARTCALLBACK 125
#define OFFSET_XNPREEDITDONECALLBACK 146
#define OFFSET_XNPREEDITDRAWCALLBACK 166
#define OFFSET_XNPREEDITCARETCALLBACK 186
#define OFFSET_XNPREEDITSTATENOTIFYCALLBACK 207
#define OFFSET_XNPREEDITATTRIBUTES 234
#define OFFSET_XNSTATUSSTARTCALLBACK 252
#define OFFSET_XNSTATUSDONECALLBACK 272
#define OFFSET_XNSTATUSDRAWCALLBACK 291
#define OFFSET_XNSTATUSATTRIBUTES 310
#define OFFSET_XNAREA 327
#define OFFSET_XNAREANEEDED 332
#define OFFSET_XNSPOTLOCATION 343
#define OFFSET_XNCOLORMAP 356
#define OFFSET_XNSTDCOLORMAP 365
#define OFFSET_XNFOREGROUND 377
#define OFFSET_XNBACKGROUND 388
#define OFFSET_XNBACKGROUNDPIXMAP 399
#define OFFSET_XNFONTSET 416
#define OFFSET_XNLINESPACE 424
#define OFFSET_XNCURSOR 434
#define OFFSET_XNQUERYIMVALUESLIST 441
#define OFFSET_XNQUERYICVALUESLIST 459
#define OFFSET_XNVISIBLEPOSITION 477
#define OFFSET_XNSTRINGCONVERSIONCALLBACK 493
#define OFFSET_XNSTRINGCONVERSION 518
#define OFFSET_XNRESETSTATE 535
#define OFFSET_XNHOTKEY 546
#define OFFSET_XNHOTKEYSTATE 553
#define OFFSET_XNPREEDITSTATE 565

/* offsets into name_table */
static const unsigned short supported_local_im_values_list[] = {
    OFFSET_XNQUERYINPUTSTYLE,
    OFFSET_XNRESOURCENAME,
    OFFSET_XNRESOURCECLASS,
    OFFSET_XNDESTROYCALLBACK,
    OFFSET_XNQUERYIMVALUESLIST,
    OFFSET_XNQUERYICVALUESLIST,
    OFFSET_XNVISIBLEPOSITION
};

/* offsets into name_table */
static const unsigned short supported_local_ic_values_list[] = {
    OFFSET_XNINPUTSTYLE,
    OFFSET_XNCLIENTWINDOW,
    OFFSET_XNFOCUSWINDOW,
    OFFSET_XNRESOURCENAME,
    OFFSET_XNRESOURCECLASS,
    OFFSET_XNGEOMETRYCALLBACK,
    OFFSET_XNFILTEREVENTS,
    OFFSET_XNDESTROYCALLBACK,
    OFFSET_XNSTRINGCONVERSIONCALLBACK,
    OFFSET_XNSTRINGCONVERSIONCALLBACK,
    OFFSET_XNRESETSTATE,
    OFFSET_XNHOTKEY,
    OFFSET_XNHOTKEYSTATE,
    OFFSET_XNPREEDITATTRIBUTES,
    OFFSET_XNSTATUSATTRIBUTES,
    OFFSET_XNAREA,
    OFFSET_XNAREANEEDED,
    OFFSET_XNSPOTLOCATION,
    OFFSET_XNCOLORMAP,
    OFFSET_XNSTDCOLORMAP,
    OFFSET_XNFOREGROUND,
    OFFSET_XNBACKGROUND,
    OFFSET_XNBACKGROUNDPIXMAP,
    OFFSET_XNFONTSET,
    OFFSET_XNLINESPACE,
    OFFSET_XNCURSOR,
    OFFSET_XNPREEDITSTARTCALLBACK,
    OFFSET_XNPREEDITDONECALLBACK,
    OFFSET_XNPREEDITDRAWCALLBACK,
    OFFSET_XNPREEDITCARETCALLBACK,
    OFFSET_XNSTATUSSTARTCALLBACK,
    OFFSET_XNSTATUSDONECALLBACK,
    OFFSET_XNSTATUSDRAWCALLBACK,
    OFFSET_XNPREEDITSTATE,
    OFFSET_XNPREEDITSTATENOTIFYCALLBACK
};

static XIMStyle const supported_local_styles[] = {
    XIMPreeditNone	| XIMStatusNone,
    XIMPreeditNothing	| XIMStatusNothing,
    0						/* dummy */
};

static Bool
_XimDefaultStyles(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,			/* unused */
    unsigned long	  mode)			/* unused */
{
    XIMStyles		 *styles;
    XIMStyles		**out;
    register int	  i;
    unsigned int	  n;
    int			  len;
    XPointer		  tmp;

    n = XIMNumber(supported_local_styles) - 1;
    len = sizeof(XIMStyles) + sizeof(XIMStyle) * n;
    if(!(tmp = Xcalloc(1, len))) {
	return False;
    }

    styles = (XIMStyles *)tmp;
    if (n > 0) {
	styles->count_styles = (unsigned short)n;
	styles->supported_styles =
				(XIMStyle *)((char *)tmp + sizeof(XIMStyles));
	for(i = 0; i < n; i++) {
	    styles->supported_styles[i] = supported_local_styles[i];
	}
    }

    out = (XIMStyles **)((char *)top + info->offset);
    *out = styles;
    return True;
}

static Bool
_XimDefaultIMValues(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,			/* unused */
    unsigned long	  mode)			/* unused */
{
    XIMValuesList	 *values_list;
    XIMValuesList	**out;
    register int	  i;
    unsigned int	  n;
    int			  len;
    XPointer		  tmp;

    n = XIMNumber(supported_local_im_values_list);
    len = sizeof(XIMValuesList) + sizeof(char **) * n;
    if(!(tmp = Xcalloc(1, len))) {
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    if (n > 0) {
	values_list->count_values = (unsigned short)n;
	values_list->supported_values
			 = (char **)((char *)tmp + sizeof(XIMValuesList));
	for(i = 0; i < n; i++) {
	    values_list->supported_values[i] =
		(char *)name_table + supported_local_im_values_list[i];
	}
    }

    out = (XIMValuesList **)((char *)top + info->offset);
    *out = values_list;
    return True;
}

static Bool
_XimDefaultICValues(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,			/* unused */
    unsigned long	  mode)			/* unused */
{
    XIMValuesList	 *values_list;
    XIMValuesList	**out;
    register int	  i;
    unsigned int	  n;
    int			  len;
    XPointer		  tmp;

    n = XIMNumber(supported_local_ic_values_list);
    len = sizeof(XIMValuesList) + sizeof(char **) * n;
    if(!(tmp = Xcalloc(1, len))) {
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    if (n > 0) {
	values_list->count_values = (unsigned short)n;
	values_list->supported_values
			 = (char **)((char *)tmp + sizeof(XIMValuesList));
	for(i = 0; i < n; i++) {
	    values_list->supported_values[i] =
		(char *)name_table + supported_local_ic_values_list[i];
	}
    }

    out = (XIMValuesList **)((char *)top + info->offset);
    *out = values_list;
    return True;
}

static Bool
_XimDefaultVisiblePos(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,			/* unused */
    unsigned long	  mode)			/* unused */
{
    Bool		*out;

    out = (Bool *)((char *)top + info->offset);
    *out = False;
    return True;
}

static Bool
_XimDefaultFocusWindow(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    Window		*out;

    if(ic->core.client_window == (Window)NULL) {
	return True;
    }

    out = (Window *)((char *)top + info->offset);
    *out = ic->core.client_window;
    return True;
}

static Bool
_XimDefaultResName(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,
    unsigned long	  mode)
{
    Xic			  ic = (Xic)parm;
    Xim			  im = (Xim)ic->core.im;
    char		**out;

    if(im->core.res_name == (char *)NULL) {
	return True;
    }

    out = (char **)((char *)top + info->offset);
    *out = im->core.res_name;
    return True;
}

static Bool
_XimDefaultResClass(
    XimValueOffsetInfo	   info,
    XPointer	 	   top,
    XPointer	 	   parm,
    unsigned long	   mode)
{
    Xic			  ic = (Xic)parm;
    Xim			  im = (Xim)ic->core.im;
    char		**out;

    if(im->core.res_class == (char *)NULL) {
	return True;
    }

    out = (char **)((char *)top + info->offset);
    *out = im->core.res_class;
    return True;
}

static Bool
_XimDefaultDestroyCB(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,
    unsigned long	  mode)
{
    Xic			 ic = (Xic)parm;
    Xim			 im = (Xim)ic->core.im;
    XIMCallback		*out;

    out = (XIMCallback *)((char *)top + info->offset);
    *out = im->core.destroy_callback;
    return True;
}

static Bool
_XimDefaultResetState(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,
    unsigned long	  mode)
{
    XIMResetState	*out;

    out = (XIMResetState *)((char *)top + info->offset);
    *out = XIMInitialState;
    return True;
}

static Bool
_XimDefaultHotKeyState(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  parm,
    unsigned long	  mode)
{
    XIMHotKeyState	*out;

    out = (XIMHotKeyState *)((char *)top + info->offset);
    *out = XIMHotKeyStateOFF;
    return True;
}

static Bool
_XimDefaultArea(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    Xim			 im = (Xim)ic->core.im;
    Window		 root_return;
    int			 x_return, y_return;
    unsigned int	 width_return, height_return;
    unsigned int	 border_width_return;
    unsigned int	 depth_return;
    XRectangle		 area;
    XRectangle		*out;

    if(ic->core.focus_window == (Window)NULL) {
	return True;
    }
    if(XGetGeometry(im->core.display, (Drawable)ic->core.focus_window,
		&root_return, &x_return, &y_return, &width_return,
		&height_return, &border_width_return, &depth_return)
		== (Status)Success) {
	return True;
    }
    area.x	= 0;
    area.y	= 0;
    area.width	= width_return;
    area.height	= height_return;

    out = (XRectangle *)((char *)top + info->offset);
    *out = area;
    return True;
}

static Bool
_XimDefaultColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    Xim			 im = (Xim)ic->core.im;
    XWindowAttributes	 win_attr;
    Colormap		*out;

    if(ic->core.client_window == (Window)NULL) {
	return True;
    }
    if(XGetWindowAttributes(im->core.display, ic->core.client_window,
					&win_attr) == (Status)Success) {
	return True;
    }

    out = (Colormap *)((char *)top + info->offset);
    *out = win_attr.colormap;
    return True;
}

static Bool
_XimDefaultStdColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Atom		*out;

    out = (Atom *)((char *)top + info->offset);
    *out = (Atom)0;
    return True;
}

static Bool
_XimDefaultFg(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    Xim			 im = (Xim)ic->core.im;
    unsigned long	 fg;
    unsigned long	*out;

    fg = WhitePixel(im->core.display, DefaultScreen(im->core.display));
    out = (unsigned long *)((char *)top + info->offset);
    *out = fg;
    return True;
}

static Bool
_XimDefaultBg(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    Xim			 im = (Xim)ic->core.im;
    unsigned long	 bg;
    unsigned long	*out;

    bg = BlackPixel(im->core.display, DefaultScreen(im->core.display));
    out = (unsigned long *)((char *)top + info->offset);
    *out = bg;
    return True;
}

static Bool
_XimDefaultBgPixmap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Pixmap		*out;

    out = (Pixmap *)((char *)top + info->offset);
    *out = (Pixmap)0;
    return True;
}

static Bool
_XimDefaultFontSet(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    XFontSet		*out;

    out = (XFontSet *)((char *)top + info->offset);
    *out = 0;
    return True;
}

static Bool
_XimDefaultLineSpace(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Xic			 ic = (Xic)parm;
    XFontSet		 fontset;
    XFontSetExtents	*fset_extents;
    int			 line_space = 0;
    int			*out;

    if(mode & XIM_PREEDIT_ATTR) {
	fontset = ic->core.preedit_attr.fontset;
    } else if(mode & XIM_STATUS_ATTR) {
	fontset = ic->core.status_attr.fontset;
    } else {
	return True;
    }
    if (fontset) {
	fset_extents = XExtentsOfFontSet(fontset);
	line_space = fset_extents->max_logical_extent.height;
    }
    out = (int *)((char *)top + info->offset);
    *out = line_space;
    return True;
}

static Bool
_XimDefaultCursor(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    Cursor		*out;

    out = (Cursor *)((char *)top + info->offset);
    *out = (Cursor)0;
    return True;
}

static Bool
_XimDefaultPreeditState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    XIMPreeditState	*out;

    out = (XIMPreeditState *)((char *)top + info->offset);
    *out = XIMPreeditDisable;
    return True;
}

static Bool
_XimDefaultNest(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 parm,
    unsigned long	 mode)
{
    return True;
}

static Bool
_XimEncodeCallback(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMCallback		*out;

    out = (XIMCallback *)((char *)top + info->offset);
    *out = *((XIMCallback *)val);
    return True;
}

static Bool
_XimEncodeString(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  val)
{
    char		 *string;
    char		**out;

    if(val == (XPointer)NULL) {
	return False;
    }
    if (!(string = strdup((char *)val))) {
	return False;
    }

    out = (char **)((char *)top + info->offset);
    if(*out) {
	Xfree(*out);
    }
    *out = string;
    return True;
}

static Bool
_XimEncodeStyle(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMStyle		*out;

    out = (XIMStyle *)((char *)top + info->offset);
    *out = (XIMStyle)val;
    return True;
}

static Bool
_XimEncodeWindow(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Window		*out;

    out = (Window *)((char *)top + info->offset);
    *out = (Window)val;
    return True;
}

static Bool
_XimEncodeStringConv(
    XimValueOffsetInfo		 info,
    XPointer		 	 top,
    XPointer		 	 val)
{
    /*
     * Not yet
     */
    return True;
}

static Bool
_XimEncodeResetState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMResetState	*out;

    out = (XIMResetState *)((char *)top + info->offset);
    *out = (XIMResetState)val;
    return True;
}

static Bool
_XimEncodeHotKey(
    XimValueOffsetInfo	  info,
    XPointer	 	  top,
    XPointer	 	  val)
{
    XIMHotKeyTriggers	 *hotkey = (XIMHotKeyTriggers *)val;
    XIMHotKeyTriggers	**out;
    XIMHotKeyTriggers	 *key_list;
    XIMHotKeyTrigger	 *key;
    XPointer		  tmp;
    int			  num;
    int			  len;
    register int	  i;

    if(hotkey == (XIMHotKeyTriggers *)NULL) {
	return True;
    }

    if((num = hotkey->num_hot_key) == 0) {
	return True;
    }

    len = sizeof(XIMHotKeyTriggers) + sizeof(XIMHotKeyTrigger) * num;
    if(!(tmp = Xmalloc(len))) {
	return False;
    }

    key_list = (XIMHotKeyTriggers *)tmp;
    key = (XIMHotKeyTrigger *)((char *)tmp + sizeof(XIMHotKeyTriggers));

    for(i = 0; i < num; i++) {
	key[i] = hotkey->key[i];
    }

    key_list->num_hot_key = num;
    key_list->key = key;

    out = (XIMHotKeyTriggers **)((char *)top + info->offset);
    *out = key_list;
    return True;
}

static Bool
_XimEncodeHotKetState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMHotKeyState	*out;

    out = (XIMHotKeyState *)((char *)top + info->offset);
    *out = (XIMHotKeyState)val;
    return True;
}

static Bool
_XimEncodeRectangle(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XRectangle		*out;

    out = (XRectangle *)((char *)top + info->offset);
    *out = *((XRectangle *)val);
    return True;
}

static Bool
_XimEncodeSpot(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XPoint		*out;

    out = (XPoint *)((char *)top + info->offset);
    *out = *((XPoint *)val);
    return True;
}

static Bool
_XimEncodeColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Colormap		*out;

    out = (Colormap *)((char *)top + info->offset);
    *out = (Colormap)val;
    return True;
}

static Bool
_XimEncodeStdColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Atom		*out;

    out = (Atom *)((char *)top + info->offset);
    *out = (Atom)val;
    return True;
}

static Bool
_XimEncodeLong(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    unsigned long	*out;

    out = (unsigned long *)((char *)top + info->offset);
    *out = (unsigned long)val;
    return True;
}

static Bool
_XimEncodeBgPixmap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Pixmap		*out;

    out = (Pixmap *)((char *)top + info->offset);
    *out = (Pixmap)val;
    return True;
}

static Bool
_XimEncodeFontSet(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XFontSet		*out;

    out = (XFontSet *)((char *)top + info->offset);
    *out = (XFontSet)val;
    return True;
}

static Bool
_XimEncodeLineSpace(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    int			*out;

    out = (int *)((char *)top + info->offset);
    *out = (long)val;
    return True;
}

static Bool
_XimEncodeCursor(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Cursor		*out;

    out = (Cursor *)((char *)top + info->offset);
    *out = (Cursor)val;
    return True;
}

static Bool
_XimEncodePreeditState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMPreeditState	*out;

    out = (XIMPreeditState *)((char *)top + info->offset);
    *out = (XIMPreeditState)val;
    return True;
}

static Bool
_XimEncodeNest(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    return True;
}

static Bool
_XimDecodeStyles(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMStyles		*styles;
    XIMStyles		*out;
    register int	 i;
    unsigned int	 num;
    int			 len;
    XPointer		 tmp;

    if(val == (XPointer)NULL) {
	return False;
    }

    styles = *((XIMStyles **)((char *)top + info->offset));
    num = styles->count_styles;

    len = sizeof(XIMStyles) + sizeof(XIMStyle) * num;
    if(!(tmp = Xcalloc(1, len))) {
	return False;
    }

    out = (XIMStyles *)tmp;
    if(num >0) {
	out->count_styles = (unsigned short)num;
	out->supported_styles = (XIMStyle *)((char *)tmp + sizeof(XIMStyles));

	for(i = 0; i < num; i++) {
	    out->supported_styles[i] = styles->supported_styles[i];
	}
    }
    *((XIMStyles **)val) = out;
    return True;
}

static Bool
_XimDecodeValues(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMValuesList	*values_list;
    XIMValuesList	*out;
    register int	 i;
    unsigned int	 num;
    int			 len;
    XPointer		 tmp;

    if(val == (XPointer)NULL) {
	return False;
    }

    values_list = *((XIMValuesList **)((char *)top + info->offset));
    num = values_list->count_values;

    len = sizeof(XIMValuesList) + sizeof(char **) * num;
    if(!(tmp = Xcalloc(1, len))) {
	return False;
    }

    out = (XIMValuesList *)tmp;
    if(num) {
	out->count_values = (unsigned short)num;
	out->supported_values = (char **)((char *)tmp + sizeof(XIMValuesList));

	for(i = 0; i < num; i++) {
	    out->supported_values[i] = values_list->supported_values[i];
	}
    }
    *((XIMValuesList **)val) = out;
    return True;
}

static Bool
_XimDecodeCallback(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMCallback		*in;
    XIMCallback		*callback;

    in = (XIMCallback *)((char *)top + info->offset);
    if(!(callback = Xmalloc(sizeof(XIMCallback)))) {
	return False;
    }
    callback->client_data = in->client_data;
    callback->callback    = in->callback;

    *((XIMCallback **)val) = callback;
    return True;
}

static Bool
_XimDecodeString(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    char		*in;
    char		*string;

    in = *((char **)((char *)top + info->offset));
    if (in != NULL) {
	string = strdup(in);
    } else {
	string = Xcalloc(1, 1); /* strdup("") */
    }
    if (string == NULL) {
	return False;
    }
    *((char **)val) = string;
    return True;
}

static Bool
_XimDecodeBool(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Bool		*in;

    in = (Bool *)((char *)top + info->offset);
    *((Bool *)val) = *in;
    return True;
}

static Bool
_XimDecodeStyle(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMStyle		*in;

    in = (XIMStyle *)((char *)top + info->offset);
    *((XIMStyle *)val) = *in;
    return True;
}

static Bool
_XimDecodeWindow(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Window		*in;

    in = (Window *)((char *)top + info->offset);
    *((Window *)val) = *in;
    return True;
}

static Bool
_XimDecodeStringConv(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    /*
     * Not yet
     */
    return True;
}

static Bool
_XimDecodeResetState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMResetState	*in;

    in = (XIMResetState *)((char *)top + info->offset);
    *((XIMResetState *)val) = *in;
    return True;
}

static Bool
_XimDecodeHotKey(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMHotKeyTriggers	*in;
    XIMHotKeyTriggers	*hotkey;
    XIMHotKeyTrigger	*key;
    XPointer		 tmp;
    int			 num;
    int			 len;
    register int	 i;

    in = *((XIMHotKeyTriggers **)((char *)top + info->offset));
    num = in->num_hot_key;
    len = sizeof(XIMHotKeyTriggers) + sizeof(XIMHotKeyTrigger) * num;
    if(!(tmp = Xmalloc(len))) {
	return False;
    }

    hotkey = (XIMHotKeyTriggers *)tmp;
    key = (XIMHotKeyTrigger *)((char *)tmp + sizeof(XIMHotKeyTriggers));

    for(i = 0; i < num; i++) {
	key[i] = in->key[i];
    }
    hotkey->num_hot_key = num;
    hotkey->key = key;

    *((XIMHotKeyTriggers **)val) = hotkey;
    return True;
}

static Bool
_XimDecodeHotKetState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMHotKeyState	*in;

    in = (XIMHotKeyState *)((char *)top + info->offset);
    *((XIMHotKeyState *)val) = *in;
    return True;
}

static Bool
_XimDecodeRectangle(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XRectangle		*in;
    XRectangle		*rect;

    in = (XRectangle *)((char *)top + info->offset);
    if(!(rect = Xmalloc(sizeof(XRectangle)))) {
	return False;
    }
    *rect = *in;
    *((XRectangle **)val) = rect;
    return True;
}

static Bool
_XimDecodeSpot(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XPoint		*in;
    XPoint		*spot;

    in = (XPoint *)((char *)top + info->offset);
    if(!(spot = Xmalloc(sizeof(XPoint)))) {
	return False;
    }
    *spot = *in;
    *((XPoint **)val) = spot;
    return True;
}

static Bool
_XimDecodeColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Colormap		*in;

    in = (Colormap *)((char *)top + info->offset);
    *((Colormap *)val) = *in;
    return True;
}

static Bool
_XimDecodeStdColormap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Atom		*in;

    in = (Atom *)((char *)top + info->offset);
    *((Atom *)val) = *in;
    return True;
}

static Bool
_XimDecodeLong(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    unsigned long	*in;

    in = (unsigned long *)((char *)top + info->offset);
    *((unsigned long *)val) = *in;
    return True;
}

static Bool
_XimDecodeBgPixmap(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Pixmap		*in;

    in = (Pixmap *)((char *)top + info->offset);
    *((Pixmap *)val) = *in;
    return True;
}

static Bool
_XimDecodeFontSet(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XFontSet		*in;

    in = (XFontSet *)((char *)top + info->offset);
    *((XFontSet *)val) = *in;
    return True;
}

static Bool
_XimDecodeLineSpace(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    int		*in;

    in = (int *)((char *)top + info->offset);
    *((int *)val) = *in;
    return True;
}

static Bool
_XimDecodeCursor(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    Cursor		*in;

    in = (Cursor *)((char *)top + info->offset);
    *((Cursor *)val) = *in;
    return True;
}

static Bool
_XimDecodePreeditState(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    XIMPreeditState	*in;

    in = (XIMPreeditState *)((char *)top + info->offset);
    *((XIMPreeditState *)val) = *in;
    return True;
}

static Bool
_XimDecodeNest(
    XimValueOffsetInfo	 info,
    XPointer	 	 top,
    XPointer	 	 val)
{
    return True;
}

static	XIMResource	im_resources[] = {
    {XNQueryInputStyle,		   0, XimType_XIMStyles,	0, 0, 0},
    {XNDestroyCallback,		   0,  0,			0, 0, 0},
    {XNResourceName,		   0, XimType_STRING8,		0, 0, 0},
    {XNResourceClass,		   0, XimType_STRING8,		0, 0, 0},
    {XNQueryIMValuesList,	   0, 0,			0, 0, 0},
    {XNQueryICValuesList,	   0, 0,			0, 0, 0},
    {XNVisiblePosition,		   0, 0,			0, 0, 0}
};

static	XIMResource	im_inner_resources[] = {
    {XNDestroyCallback,		   0, 0,			0, 0, 0},
    {XNResourceName,		   0, XimType_STRING8,		0, 0, 0},
    {XNResourceClass,		   0, XimType_STRING8,		0, 0, 0},
    {XNQueryIMValuesList,	   0, 0,			0, 0, 0},
    {XNQueryICValuesList,	   0, 0,			0, 0, 0},
    {XNVisiblePosition,		   0, 0,			0, 0, 0}
};

static	XIMResource	ic_resources[] = {
    {XNInputStyle,		   0, XimType_CARD32,		0, 0, 0},
    {XNClientWindow,		   0, XimType_Window,		0, 0, 0},
    {XNFocusWindow,		   0, XimType_Window,		0, 0, 0},
    {XNResourceName,		   0, XimType_STRING8,		0, 0, 0},
    {XNResourceClass,		   0, XimType_STRING8,		0, 0, 0},
    {XNGeometryCallback,	   0, 0,			0, 0, 0},
    {XNFilterEvents,		   0, XimType_CARD32,		0, 0, 0},
    {XNDestroyCallback,		   0, 0,			0, 0, 0},
    {XNStringConversionCallback,   0, 0,			0, 0, 0},
    {XNStringConversion,	   0, XimType_XIMStringConversion,0, 0, 0},
    {XNResetState,		   0, 0,			0, 0, 0},
    {XNHotKey,			   0, XimType_XIMHotKeyTriggers,0, 0, 0},
    {XNHotKeyState,		   0, XimType_XIMHotKeyState, 	0, 0, 0},
    {XNPreeditAttributes,	   0, XimType_NEST,		0, 0, 0},
    {XNStatusAttributes,	   0, XimType_NEST,		0, 0, 0},
    {XNArea,			   0, XimType_XRectangle,	0, 0, 0},
    {XNAreaNeeded,		   0, XimType_XRectangle,	0, 0, 0},
    {XNSpotLocation,		   0, XimType_XPoint,		0, 0, 0},
    {XNColormap,		   0, XimType_CARD32,		0, 0, 0},
    {XNStdColormap,		   0, XimType_CARD32,		0, 0, 0},
    {XNForeground,		   0, XimType_CARD32,		0, 0, 0},
    {XNBackground,		   0, XimType_CARD32,		0, 0, 0},
    {XNBackgroundPixmap,	   0, XimType_CARD32,		0, 0, 0},
    {XNFontSet,			   0, XimType_XFontSet,		0, 0, 0},
    {XNLineSpace,		   0, XimType_CARD32,		0, 0, 0},
    {XNCursor,			   0, XimType_CARD32,		0, 0, 0},
    {XNPreeditStartCallback,	   0, 0,			0, 0, 0},
    {XNPreeditDoneCallback,	   0, 0,			0, 0, 0},
    {XNPreeditDrawCallback,	   0, 0,			0, 0, 0},
    {XNPreeditCaretCallback,	   0, 0,			0, 0, 0},
    {XNStatusStartCallback,	   0, 0,			0, 0, 0},
    {XNStatusDoneCallback,	   0, 0,			0, 0, 0},
    {XNStatusDrawCallback,	   0, 0,			0, 0, 0},
    {XNPreeditState,		   0, 0,			0, 0, 0},
    {XNPreeditStateNotifyCallback, 0, 0,			0, 0, 0},
};

static	XIMResource	ic_inner_resources[] = {
    {XNResourceName,		   0, XimType_STRING8,		0, 0, 0},
    {XNResourceClass,		   0, XimType_STRING8,		0, 0, 0},
    {XNGeometryCallback,	   0, 0,			0, 0, 0},
    {XNDestroyCallback,		   0, 0,			0, 0, 0},
    {XNStringConversionCallback,   0, 0,			0, 0, 0},
    {XNPreeditStartCallback,	   0, 0,			0, 0, 0},
    {XNPreeditDoneCallback,	   0, 0,			0, 0, 0},
    {XNPreeditDrawCallback,	   0, 0,			0, 0, 0},
    {XNPreeditCaretCallback,	   0, 0,			0, 0, 0},
    {XNStatusStartCallback,	   0, 0,			0, 0, 0},
    {XNStatusDoneCallback,	   0, 0,			0, 0, 0},
    {XNStatusDrawCallback,	   0, 0,			0, 0, 0},
    {XNPreeditStateNotifyCallback, 0, 0,			0, 0, 0},
};

static XimValueOffsetInfoRec im_attr_info[] = {
    {OFFSET_XNQUERYINPUTSTYLE,		 0,
	XOffsetOf(XimDefIMValues, styles),
	_XimDefaultStyles,	 NULL,			_XimDecodeStyles},

    {OFFSET_XNDESTROYCALLBACK,		 0,
	XOffsetOf(XimDefIMValues, destroy_callback),
	NULL,		 	 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNRESOURCENAME,		 0,
	XOffsetOf(XimDefIMValues, res_name),
	NULL,		 	 _XimEncodeString,	_XimDecodeString},

    {OFFSET_XNRESOURCECLASS,		 0,
	XOffsetOf(XimDefIMValues, res_class),
	NULL,		 	 _XimEncodeString,	_XimDecodeString},

    {OFFSET_XNQUERYIMVALUESLIST,		 0,
	XOffsetOf(XimDefIMValues, im_values_list),
	_XimDefaultIMValues,	 NULL,			_XimDecodeValues},

    {OFFSET_XNQUERYICVALUESLIST,		 0,
	XOffsetOf(XimDefIMValues, ic_values_list),
	_XimDefaultICValues,	 NULL,			_XimDecodeValues},

    {OFFSET_XNVISIBLEPOSITION,			 0,
	XOffsetOf(XimDefIMValues, visible_position),
	_XimDefaultVisiblePos,	 NULL,			_XimDecodeBool}
};

static XimValueOffsetInfoRec ic_attr_info[] = {
    {OFFSET_XNINPUTSTYLE,		 0,
	XOffsetOf(XimDefICValues, input_style),
	NULL,			 _XimEncodeStyle,	_XimDecodeStyle},

    {OFFSET_XNCLIENTWINDOW,		 0,
	XOffsetOf(XimDefICValues, client_window),
	NULL,			 _XimEncodeWindow,	_XimDecodeWindow},

    {OFFSET_XNFOCUSWINDOW,		 0,
	XOffsetOf(XimDefICValues, focus_window),
	_XimDefaultFocusWindow,  _XimEncodeWindow,	_XimDecodeWindow},

    {OFFSET_XNRESOURCENAME,		 0,
	XOffsetOf(XimDefICValues, res_name),
	_XimDefaultResName,	 _XimEncodeString,	_XimDecodeString},

    {OFFSET_XNRESOURCECLASS,		 0,
	XOffsetOf(XimDefICValues, res_class),
	_XimDefaultResClass,	 _XimEncodeString,	_XimDecodeString},

    {OFFSET_XNGEOMETRYCALLBACK,	 0,
	XOffsetOf(XimDefICValues, geometry_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNFILTEREVENTS,		 0,
	XOffsetOf(XimDefICValues, filter_events),
	NULL,			 NULL,			_XimDecodeLong},

    {OFFSET_XNDESTROYCALLBACK,		 0,
	XOffsetOf(XimDefICValues, destroy_callback),
	_XimDefaultDestroyCB,	 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNSTRINGCONVERSIONCALLBACK, 0,
	XOffsetOf(XimDefICValues, string_conversion_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNSTRINGCONVERSION,	 0,
	XOffsetOf(XimDefICValues, string_conversion),
	NULL,			 _XimEncodeStringConv,	_XimDecodeStringConv},

    {OFFSET_XNRESETSTATE,		 0,
	XOffsetOf(XimDefICValues, reset_state),
	_XimDefaultResetState,	 _XimEncodeResetState,	_XimDecodeResetState},

    {OFFSET_XNHOTKEY,			 0,
	XOffsetOf(XimDefICValues, hotkey),
	NULL,			 _XimEncodeHotKey,	_XimDecodeHotKey},

    {OFFSET_XNHOTKEYSTATE,		 0,
	XOffsetOf(XimDefICValues, hotkey_state),
	_XimDefaultHotKeyState,	 _XimEncodeHotKetState,	_XimDecodeHotKetState},

    {OFFSET_XNPREEDITATTRIBUTES,	 0,
	XOffsetOf(XimDefICValues, preedit_attr),
	_XimDefaultNest,	 _XimEncodeNest,	_XimDecodeNest},

    {OFFSET_XNSTATUSATTRIBUTES,	 0,
	XOffsetOf(XimDefICValues, status_attr),
	_XimDefaultNest,	 _XimEncodeNest,	_XimDecodeNest},
};

static XimValueOffsetInfoRec ic_pre_attr_info[] = {
    {OFFSET_XNAREA,			 0,
	XOffsetOf(ICPreeditAttributes, area),
	_XimDefaultArea,	 _XimEncodeRectangle,	_XimDecodeRectangle},

    {OFFSET_XNAREANEEDED,		 0,
	XOffsetOf(ICPreeditAttributes, area_needed),
	NULL,			 _XimEncodeRectangle,	_XimDecodeRectangle},

    {OFFSET_XNSPOTLOCATION,		 0,
	XOffsetOf(ICPreeditAttributes, spot_location),
	NULL,			 _XimEncodeSpot,	_XimDecodeSpot},

    {OFFSET_XNCOLORMAP,		 0,
	XOffsetOf(ICPreeditAttributes, colormap),
	_XimDefaultColormap,	 _XimEncodeColormap,	_XimDecodeColormap},

    {OFFSET_XNSTDCOLORMAP,		 0,
	XOffsetOf(ICPreeditAttributes, std_colormap),
	_XimDefaultStdColormap,	 _XimEncodeStdColormap,	_XimDecodeStdColormap},

    {OFFSET_XNFOREGROUND,		 0,
	XOffsetOf(ICPreeditAttributes, foreground),
	_XimDefaultFg,		 _XimEncodeLong,	_XimDecodeLong},

    {OFFSET_XNBACKGROUND,		 0,
	XOffsetOf(ICPreeditAttributes, background),
	_XimDefaultBg,		 _XimEncodeLong,	_XimDecodeLong},

    {OFFSET_XNBACKGROUNDPIXMAP,	 0,
	XOffsetOf(ICPreeditAttributes, background_pixmap),
	_XimDefaultBgPixmap, 	 _XimEncodeBgPixmap,	_XimDecodeBgPixmap},

    {OFFSET_XNFONTSET,			 0,
	XOffsetOf(ICPreeditAttributes, fontset),
	_XimDefaultFontSet,	 _XimEncodeFontSet,	_XimDecodeFontSet},

    {OFFSET_XNLINESPACE,		 0,
	XOffsetOf(ICPreeditAttributes, line_spacing),
	_XimDefaultLineSpace,	 _XimEncodeLineSpace,	_XimDecodeLineSpace},

    {OFFSET_XNCURSOR,			 0,
	XOffsetOf(ICPreeditAttributes, cursor),
	_XimDefaultCursor,	 _XimEncodeCursor,	_XimDecodeCursor},

    {OFFSET_XNPREEDITSTARTCALLBACK,	 0,
	XOffsetOf(ICPreeditAttributes, start_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNPREEDITDONECALLBACK,	 0,
	XOffsetOf(ICPreeditAttributes, done_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNPREEDITDRAWCALLBACK,	 0,
	XOffsetOf(ICPreeditAttributes, draw_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNPREEDITCARETCALLBACK,	 0,
	XOffsetOf(ICPreeditAttributes, caret_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNPREEDITSTATE,		 0,
	XOffsetOf(ICPreeditAttributes, preedit_state),
	_XimDefaultPreeditState, _XimEncodePreeditState,_XimDecodePreeditState},

    {OFFSET_XNPREEDITSTATENOTIFYCALLBACK, 0,
	XOffsetOf(ICPreeditAttributes, state_notify_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},
};

static XimValueOffsetInfoRec ic_sts_attr_info[] = {
    {OFFSET_XNAREA,			 0,
	XOffsetOf(ICStatusAttributes, area),
	_XimDefaultArea,	 _XimEncodeRectangle,	_XimDecodeRectangle},

    {OFFSET_XNAREANEEDED,		 0,
	XOffsetOf(ICStatusAttributes, area_needed),
	NULL,			 _XimEncodeRectangle,	_XimDecodeRectangle},

    {OFFSET_XNCOLORMAP,		 0,
	XOffsetOf(ICStatusAttributes, colormap),
	_XimDefaultColormap,	 _XimEncodeColormap,	_XimDecodeColormap},

    {OFFSET_XNSTDCOLORMAP,		 0,
	XOffsetOf(ICStatusAttributes, std_colormap),
	_XimDefaultStdColormap,	 _XimEncodeStdColormap,	_XimDecodeStdColormap},

    {OFFSET_XNFOREGROUND,		 0,
	XOffsetOf(ICStatusAttributes, foreground),
	_XimDefaultFg,		 _XimEncodeLong,	_XimDecodeLong},

    {OFFSET_XNBACKGROUND,		 0,
	XOffsetOf(ICStatusAttributes, background),
	_XimDefaultBg,		 _XimEncodeLong,	_XimDecodeLong},

    {OFFSET_XNBACKGROUNDPIXMAP,	 0,
	XOffsetOf(ICStatusAttributes, background_pixmap),
	_XimDefaultBgPixmap, 	 _XimEncodeBgPixmap,	_XimDecodeBgPixmap},

    {OFFSET_XNFONTSET,			 0,
	XOffsetOf(ICStatusAttributes, fontset),
	_XimDefaultFontSet,	 _XimEncodeFontSet,	_XimDecodeFontSet},

    {OFFSET_XNLINESPACE,		 0,
	XOffsetOf(ICStatusAttributes, line_spacing),
	_XimDefaultLineSpace,	 _XimEncodeLineSpace,	_XimDecodeLineSpace},

    {OFFSET_XNCURSOR,			 0,
	XOffsetOf(ICStatusAttributes, cursor),
	_XimDefaultCursor,	 _XimEncodeCursor,	_XimDecodeCursor},

    {OFFSET_XNSTATUSSTARTCALLBACK,	 0,
	XOffsetOf(ICStatusAttributes, start_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNSTATUSDONECALLBACK,	 0,
	XOffsetOf(ICStatusAttributes, done_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback},

    {OFFSET_XNSTATUSDRAWCALLBACK,	 0,
	XOffsetOf(ICStatusAttributes, draw_callback),
	NULL,			 _XimEncodeCallback,	_XimDecodeCallback}
};

typedef struct _XimIMMode {
    unsigned short name_offset;
    unsigned short	 mode;
} XimIMMode;

static const XimIMMode im_mode[] = {
    {OFFSET_XNQUERYINPUTSTYLE,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_GET)},
    {OFFSET_XNDESTROYCALLBACK,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_SET | XIM_MODE_IM_GET)},
    {OFFSET_XNRESOURCENAME,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_SET | XIM_MODE_IM_GET)},
    {OFFSET_XNRESOURCECLASS,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_SET | XIM_MODE_IM_GET)},
    {OFFSET_XNQUERYIMVALUESLIST,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_GET)},
    {OFFSET_XNQUERYICVALUESLIST,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_GET)},
    {OFFSET_XNVISIBLEPOSITION,
		(XIM_MODE_IM_DEFAULT | XIM_MODE_IM_GET)}
};

typedef struct _XimICMode {
    unsigned short name_offset;
    unsigned short	 preedit_callback_mode;
    unsigned short	 preedit_position_mode;
    unsigned short	 preedit_area_mode;
    unsigned short	 preedit_nothing_mode;
    unsigned short	 preedit_none_mode;
    unsigned short	 status_callback_mode;
    unsigned short	 status_area_mode;
    unsigned short	 status_nothing_mode;
    unsigned short	 status_none_mode;
} XimICMode;

static const XimICMode ic_mode[] = {
    {OFFSET_XNINPUTSTYLE,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_GET),
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_GET),
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_GET),
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_GET),
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_GET)},
    {OFFSET_XNCLIENTWINDOW,
		(XIM_MODE_PRE_ONCE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_ONCE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_ONCE | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_ONCE | XIM_MODE_PRE_GET),
		0,
		(XIM_MODE_STS_ONCE | XIM_MODE_STS_GET),
		(XIM_MODE_STS_ONCE | XIM_MODE_STS_GET),
		(XIM_MODE_STS_ONCE | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNFOCUSWINDOW,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNRESOURCENAME,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNRESOURCECLASS,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNGEOMETRYCALLBACK,
		0,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0},
    {OFFSET_XNFILTEREVENTS,
		XIM_MODE_PRE_GET,
		XIM_MODE_PRE_GET,
		XIM_MODE_PRE_GET,
		XIM_MODE_PRE_GET,
		0,
		XIM_MODE_STS_GET,
		XIM_MODE_STS_GET,
		XIM_MODE_STS_GET,
		XIM_MODE_STS_GET},
    {OFFSET_XNDESTROYCALLBACK,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0},
    {OFFSET_XNSTRINGCONVERSIONCALLBACK,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0},
    {OFFSET_XNSTRINGCONVERSION,
		XIM_MODE_PRE_SET,
		XIM_MODE_PRE_SET,
		XIM_MODE_PRE_SET,
		XIM_MODE_PRE_SET,
		XIM_MODE_PRE_SET,
		0,
		0,
		0,
		0},
    {OFFSET_XNRESETSTATE,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNHOTKEY,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNHOTKEYSTATE,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITATTRIBUTES,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNSTATUSATTRIBUTES,
		0,
		0,
		0,
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNAREA,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0},
    {OFFSET_XNAREANEEDED,
		0,
		0,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		(XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0},
    {OFFSET_XNSPOTLOCATION,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0},
    {OFFSET_XNCOLORMAP,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNSTDCOLORMAP,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNFOREGROUND,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNBACKGROUND,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNBACKGROUNDPIXMAP,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNFONTSET,
		0,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNLINESPACE,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNCURSOR,
		0,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		(XIM_MODE_STS_DEFAULT | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0},
    {OFFSET_XNPREEDITSTARTCALLBACK,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITDONECALLBACK,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITDRAWCALLBACK,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITCARETCALLBACK,
		(XIM_MODE_PRE_CREATE | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITSTATE,
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_DEFAULT | XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNPREEDITSTATENOTIFYCALLBACK,
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		(XIM_MODE_PRE_SET | XIM_MODE_PRE_GET),
		0,
		0,
		0,
		0,
		0},
    {OFFSET_XNSTATUSSTARTCALLBACK,
		0,
		0,
		0,
		0,
		0,
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0,
		0},
    {OFFSET_XNSTATUSDONECALLBACK,
		0,
		0,
		0,
		0,
		0,
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0,
		0},
    {OFFSET_XNSTATUSDRAWCALLBACK,
		0,
		0,
		0,
		0,
		0,
		(XIM_MODE_STS_CREATE | XIM_MODE_STS_SET | XIM_MODE_STS_GET),
		0,
		0,
		0}
};

/* the quarks are separated from im_mode/ic_mode so those arrays
 * can be const.
 */
static XrmQuark im_mode_quark[sizeof(im_mode) / sizeof(im_mode[0])];
static XrmQuark ic_mode_quark[sizeof(ic_mode) / sizeof(ic_mode[0])];

static Bool
_XimSetResourceList(
    XIMResourceList	 *res_list,
    unsigned int	 *list_num,
    XIMResourceList	  resource,
    unsigned int	  num_resource,
    unsigned short	  id)
{
    register int	  i;
    int			  len;
    XIMResourceList	  res;

    len = sizeof(XIMResource) * num_resource;
    if(!(res = Xcalloc(1, len))) {
	return False;
    }

    for(i = 0; i < num_resource; i++, id++) {
	res[i]    = resource[i];
	res[i].id = id;
    }

    _XIMCompileResourceList(res, num_resource);
    *res_list  = res;
    *list_num  = num_resource;
    return True;
}

Bool
_XimSetIMResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num)
{
    return _XimSetResourceList(res_list, list_num,
				im_resources, XIMNumber(im_resources), 100);
}

Bool
_XimSetICResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num)
{
    return _XimSetResourceList(res_list, list_num,
				ic_resources, XIMNumber(ic_resources), 200);
}

Bool
_XimSetInnerIMResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num)
{
    return _XimSetResourceList(res_list, list_num,
		im_inner_resources, XIMNumber(im_inner_resources), 100);
}

Bool
_XimSetInnerICResourceList(
    XIMResourceList	*res_list,
    unsigned int	*list_num)
{
    return _XimSetResourceList(res_list, list_num,
		ic_inner_resources, XIMNumber(ic_inner_resources), 200);
}

static XIMResourceList
_XimGetResourceListRecByMode(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    unsigned short	 mode)
{
    register int	 i;

    for(i = 0; i < list_num; i++) {
	if (res_list[i].mode & mode) {
	    return (XIMResourceList)&res_list[i];
	}
    }
    return (XIMResourceList)NULL;
}

Bool
_XimCheckCreateICValues(
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    if (!_XimGetResourceListRecByMode(res_list, list_num, XIM_MODE_IC_CREATE)) {
	return True;
    }
    return False;
}

XIMResourceList
_XimGetResourceListRecByQuark(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XrmQuark		 quark)
{
    register int	 i;

    for(i = 0; i < list_num; i++) {
	if (res_list[i].xrm_name == quark) {
	    return (XIMResourceList)&res_list[i];
	}
    }
    return (XIMResourceList)NULL;
}

XIMResourceList
_XimGetResourceListRec(
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    const char		*name)
{
    XrmQuark		 quark = XrmStringToQuark(name);

    return _XimGetResourceListRecByQuark(res_list, list_num, quark);
}

char *
_XimSetIMValueData(
    Xim			 im,
    XPointer		 top,
    XIMArg		*values,
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;

    for(p = values; p->name != NULL; p++) {
	if(!(res = _XimGetResourceListRec(res_list, list_num, p->name))) {
	    return p->name;
	}
	check = _XimCheckIMMode(res, XIM_SETIMVALUES);
	if(check == XIM_CHECK_INVALID) {
	    continue;
	} else if (check == XIM_CHECK_ERROR) {
	    return p->name;
	}

	if(!_XimEncodeLocalIMAttr(res, top, p->value)) {
	    return p->name;
	}
    }
    return NULL;
}

char *
_XimGetIMValueData(
    Xim			 im,
    XPointer		 top,
    XIMArg		*values,
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;

    for(p = values; p->name != NULL; p++) {
	if(!(res = _XimGetResourceListRec(res_list, list_num, p->name))) {
	    return p->name;
	}
	check = _XimCheckIMMode(res, XIM_GETIMVALUES);
	if(check == XIM_CHECK_INVALID) {
	    continue;
	} else if (check == XIM_CHECK_ERROR) {
	    return p->name;
	}

	if(!_XimDecodeLocalIMAttr(res, top, p->value)) {
	    return p->name;
	}
    }
    return NULL;
}

void
_XimSetIMMode(
    XIMResourceList	res_list,
    unsigned int	list_num)
{
    XIMResourceList	res;
    unsigned int	n = XIMNumber(im_mode);
    register int	i;

    for(i = 0; i < n; i++) {
	if(!(res = _XimGetResourceListRecByQuark(res_list,
						list_num, im_mode_quark[i]))) {
	    continue;
	}
	res->mode = im_mode[i].mode;
    }
    return;
}

static int
_XimCheckSetIMDefaultsMode(
    XIMResourceList	res)
{
    if(res->mode & XIM_MODE_IM_DEFAULT) {
	return XIM_CHECK_VALID;
    }
    return XIM_CHECK_INVALID;
}

static int
_XimCheckSetIMValuesMode(
    XIMResourceList	res)
{
    if(res->mode & XIM_MODE_IM_SET) {
	return XIM_CHECK_VALID;
    }
    return XIM_CHECK_INVALID;
}

static int
 _XimCheckGetIMValuesMode(
    XIMResourceList	res)
{
    if(res->mode & XIM_MODE_IM_GET) {
	return XIM_CHECK_VALID;
    }
    return XIM_CHECK_INVALID;
}

int
 _XimCheckIMMode(
    XIMResourceList	res,
    unsigned long	mode)
{
    if(res->mode == 0) {
	return XIM_CHECK_INVALID;
    }
    if(mode & XIM_SETIMDEFAULTS) {
	return _XimCheckSetIMDefaultsMode(res);
    } else if (mode & XIM_SETIMVALUES) {
	return _XimCheckSetIMValuesMode(res);
    } else if (mode & XIM_GETIMVALUES) {
	return _XimCheckGetIMValuesMode(res);
    } else {
	return XIM_CHECK_ERROR;
    }
}

void
_XimSetICMode(XIMResourceList res_list, unsigned int list_num, XIMStyle style)
{
    XIMResourceList	res;
    unsigned int	n = XIMNumber(ic_mode);
    register int	i;
    unsigned int	pre_offset;
    unsigned int	sts_offset;

    if(style & XIMPreeditArea) {
	pre_offset = XOffsetOf(XimICMode, preedit_area_mode);
    } else if(style & XIMPreeditCallbacks) {
	pre_offset = XOffsetOf(XimICMode, preedit_callback_mode);
    } else if(style & XIMPreeditPosition) {
	pre_offset = XOffsetOf(XimICMode, preedit_position_mode);
    } else if(style & XIMPreeditNothing) {
	pre_offset = XOffsetOf(XimICMode, preedit_nothing_mode);
    } else {
	pre_offset = XOffsetOf(XimICMode, preedit_none_mode);
    }

    if(style & XIMStatusArea) {
	sts_offset = XOffsetOf(XimICMode, status_area_mode);
    } else if(style & XIMStatusCallbacks) {
	sts_offset = XOffsetOf(XimICMode, status_callback_mode);
    } else if(style & XIMStatusNothing) {
	sts_offset = XOffsetOf(XimICMode, status_nothing_mode);
    } else {
	sts_offset = XOffsetOf(XimICMode, status_none_mode);
    }

    for(i = 0; i < n; i++) {
	if(!(res = _XimGetResourceListRecByQuark(res_list,
						list_num, ic_mode_quark[i]))) {
	    continue;
	}
	res->mode = ( (*(const unsigned short *)((const char *)&ic_mode[i] + pre_offset))
		    | (*(const unsigned short *)((const char *)&ic_mode[i] + sts_offset)));
    }
    return;
}

static int
_XimCheckSetICDefaultsMode(
    XIMResourceList	res,
    unsigned long	mode)
{
    if(mode & XIM_PREEDIT_ATTR) {
	if(!(res->mode & XIM_MODE_PRE_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_PRE_CREATE) {
	    return XIM_CHECK_ERROR;
	} else if (!(res->mode & XIM_MODE_PRE_DEFAULT)) {
	    return XIM_CHECK_INVALID;
	}

    } else if(mode & XIM_STATUS_ATTR) {
	if(!(res->mode & XIM_MODE_STS_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_STS_CREATE) {
	    return XIM_CHECK_ERROR;
	}
	if(!(res->mode & XIM_MODE_STS_DEFAULT)) {
	    return XIM_CHECK_INVALID;
	}

    } else {
	if(!res->mode) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_IC_CREATE) {
	    return XIM_CHECK_ERROR;
	}
	if(!(res->mode & XIM_MODE_IC_DEFAULT)) {
	    return XIM_CHECK_INVALID;
	}
    }
    return XIM_CHECK_VALID;
}

static int
_XimCheckCreateICMode(
    XIMResourceList	res,
    unsigned long	mode)
{
    if(mode & XIM_PREEDIT_ATTR) {
	if(!(res->mode & XIM_MODE_PRE_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_PRE_CREATE) {
	    res->mode &= ~XIM_MODE_PRE_CREATE;
	} else if(res->mode & XIM_MODE_PRE_ONCE) {
	    res->mode &= ~XIM_MODE_PRE_ONCE;
	} else if(res->mode & XIM_MODE_PRE_DEFAULT) {
	    res->mode &= ~XIM_MODE_PRE_DEFAULT;
	} else if (!(res->mode & XIM_MODE_PRE_SET)) {
	    return XIM_CHECK_ERROR;
	}

    } else if(mode & XIM_STATUS_ATTR) {
	if(!(res->mode & XIM_MODE_STS_MASK)) {
	    return  XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_STS_CREATE) {
	    res->mode &= ~XIM_MODE_STS_CREATE;
	} else if(res->mode & XIM_MODE_STS_ONCE) {
	    res->mode &= ~XIM_MODE_STS_ONCE;
	} else if(res->mode & XIM_MODE_STS_DEFAULT) {
	    res->mode &= ~XIM_MODE_STS_DEFAULT;
	} else if (!(res->mode & XIM_MODE_STS_SET)) {
	    return XIM_CHECK_ERROR;
	}

    } else {
	if(!res->mode) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_IC_CREATE) {
	    res->mode &= ~XIM_MODE_IC_CREATE;
	} else if(res->mode & XIM_MODE_IC_ONCE) {
	    res->mode &= ~XIM_MODE_IC_ONCE;
	} else if(res->mode & XIM_MODE_IC_DEFAULT) {
	    res->mode &= ~XIM_MODE_IC_DEFAULT;
	} else if (!(res->mode & XIM_MODE_IC_SET)) {
	    return XIM_CHECK_ERROR;
	}
    }
    return XIM_CHECK_VALID;
}

static int
_XimCheckSetICValuesMode(
    XIMResourceList	res,
    unsigned long	mode)
{
    if(mode & XIM_PREEDIT_ATTR) {
	if(!(res->mode & XIM_MODE_PRE_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_PRE_ONCE) {
	    res->mode &= ~XIM_MODE_PRE_ONCE;
	} else if(!(res->mode & XIM_MODE_PRE_SET)) {
	    return XIM_CHECK_ERROR;
	}

    } else if(mode & XIM_STATUS_ATTR) {
	if(!(res->mode & XIM_MODE_STS_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_STS_ONCE) {
	    res->mode &= ~XIM_MODE_STS_ONCE;
	} else if(!(res->mode & XIM_MODE_STS_SET)) {
	    return XIM_CHECK_ERROR;
	}

    } else {
	if(!res->mode) {
	    return XIM_CHECK_INVALID;
	}

	if(res->mode & XIM_MODE_IC_ONCE) {
	    res->mode &= ~XIM_MODE_IC_ONCE;
	} else if(!(res->mode & XIM_MODE_IC_SET)) {
	    return XIM_CHECK_ERROR;
	}
    }
    return XIM_CHECK_VALID;
}

static int
_XimCheckGetICValuesMode(
    XIMResourceList	res,
    unsigned long	mode)
{
    if(mode & XIM_PREEDIT_ATTR) {
	if(!(res->mode & XIM_MODE_PRE_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(!(res->mode & XIM_MODE_PRE_GET)) {
	    return XIM_CHECK_ERROR;
	}

    } else if(mode & XIM_STATUS_ATTR) {
	if(!(res->mode & XIM_MODE_STS_MASK)) {
	    return XIM_CHECK_INVALID;
	}

	if(!(res->mode & XIM_MODE_STS_GET)) {
	    return XIM_CHECK_ERROR;
	}

    } else {
	if(!res->mode) {
	    return XIM_CHECK_INVALID;
	}

	if(!(res->mode & XIM_MODE_IC_GET)) {
	    return XIM_CHECK_ERROR;
	}
    }
    return XIM_CHECK_VALID;
}

int
 _XimCheckICMode(
    XIMResourceList     res,
    unsigned long       mode)
{
    if(mode &XIM_SETICDEFAULTS) {
	return _XimCheckSetICDefaultsMode(res, mode);
    } else if (mode & XIM_CREATEIC) {
	return _XimCheckCreateICMode(res, mode);
    } else if (mode & XIM_SETICVALUES) {
	return _XimCheckSetICValuesMode(res, mode);
    } else if (mode & XIM_GETICVALUES) {
	return _XimCheckGetICValuesMode(res, mode);
    } else {
	return XIM_CHECK_ERROR;
    }
}

Bool
_XimSetLocalIMDefaults(
    Xim			 im,
    XPointer		 top,
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    XimValueOffsetInfo	 info;
    unsigned int	 num;
    register int	 i;
    XIMResourceList	 res;
    int			 check;

    info = im_attr_info;
    num  = XIMNumber(im_attr_info);

    for(i = 0; i < num; i++) {
	if((res = _XimGetResourceListRecByQuark( res_list, list_num,
				info[i].quark)) == (XIMResourceList)NULL) {
	    return False;
	}

	check = _XimCheckIMMode(res, XIM_SETIMDEFAULTS);
	if(check == XIM_CHECK_INVALID) {
	    continue;
	} else if (check == XIM_CHECK_ERROR) {
	    return False;
	}

	if(!info[i].defaults) {
	    continue;
	}
	if(!(info[i].defaults(&info[i], top, (XPointer)NULL, 0))) {
	    return False;
	}
    }
    return True;
}

Bool
_XimSetICDefaults(
    Xic			 ic,
    XPointer		 top,
    unsigned long	 mode,
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    unsigned int	 num;
    XimValueOffsetInfo	 info;
    register int	 i;
    XIMResourceList	 res;
    int			 check;
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    if(mode & XIM_PREEDIT_ATTR) {
	info = ic_pre_attr_info;
	num  = XIMNumber(ic_pre_attr_info);
    } else if(mode & XIM_STATUS_ATTR) {
	info = ic_sts_attr_info;
	num  = XIMNumber(ic_sts_attr_info);
    } else {
	info = ic_attr_info;
	num  = XIMNumber(ic_attr_info);
    }

    for(i = 0; i < num; i++) {
	if(info[i].quark == pre_quark) {
	    if(!_XimSetICDefaults(ic, (XPointer)((char *)top + info[i].offset),
			(mode | XIM_PREEDIT_ATTR), res_list, list_num)) {
		return False;
	    }
	} else if (info[i].quark == sts_quark) {
	    if(!_XimSetICDefaults(ic, (XPointer)((char *)top + info[i].offset),
			(mode | XIM_STATUS_ATTR), res_list, list_num)) {
		return False;
	    }
	} else {
	    if(!(res = _XimGetResourceListRecByQuark(res_list, list_num,
							info[i].quark))) {
		return False;
	    }

	    check = _XimCheckICMode(res, mode);
	    if (check == XIM_CHECK_INVALID) {
		continue;
	    } else if (check == XIM_CHECK_ERROR) {
		return False;
	    }

	    if (!info[i].defaults) {
		continue;
	    }
	    if (!(info[i].defaults(&info[i], top, (XPointer)ic, mode))) {
		return False;
	    }
	}
    }
    return True;
}

static Bool
_XimEncodeAttr(
    XimValueOffsetInfo	 info,
    unsigned int	 num,
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val)
{
    register int	 i;

    for(i = 0; i < num; i++ ) {
	if(info[i].quark == res->xrm_name) {
	    if(!info[i].encode) {
		return False;
	    }
	    return (*info[i].encode)(&info[i], top, val);
	}
    }
    return False;
}

Bool
_XimEncodeLocalIMAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val)
{
    return _XimEncodeAttr(im_attr_info, XIMNumber(im_attr_info),
					res, top, val);
}

Bool
_XimEncodeLocalICAttr(
    Xic			 ic,
    XIMResourceList	 res,
    XPointer		 top,
    XIMArg		*arg,
    unsigned long	 mode)
{
    unsigned int	 num;
    XimValueOffsetInfo	 info;

    if(mode & XIM_PREEDIT_ATTR) {
	info = ic_pre_attr_info;
	num  = XIMNumber(ic_pre_attr_info);
    } else if(mode & XIM_STATUS_ATTR) {
	info = ic_sts_attr_info;
	num  = XIMNumber(ic_sts_attr_info);
    } else {
	info = ic_attr_info;
	num  = XIMNumber(ic_attr_info);
    }

    return _XimEncodeAttr(info, num, res, top, arg->value);
}

static Bool
_XimEncodeLocalTopValue(
    Xic			 ic,
    XIMResourceList	 res,
    XPointer		 val,
    Bool		 flag)
{
    XIMArg		*p = (XIMArg *)val;

    if (res->xrm_name == XrmStringToQuark(XNClientWindow)) {
	ic->core.client_window = (Window)p->value;
	if (ic->core.focus_window == (Window)0)
	    ic->core.focus_window = ic->core.client_window;
	if (flag) {
	    _XRegisterFilterByType(ic->core.im->core.display,
			ic->core.focus_window,
			KeyPress, KeyRelease, _XimLocalFilter, (XPointer)ic);
	}
    } else if (res->xrm_name == XrmStringToQuark(XNFocusWindow)) {
	if (ic->core.client_window) {
	    if (flag) {
	        _XUnregisterFilter(ic->core.im->core.display,
			ic->core.focus_window, _XimLocalFilter, (XPointer)ic);
	    }
	    ic->core.focus_window = (Window)p->value;
	    if (flag) {
	        _XRegisterFilterByType(ic->core.im->core.display,
			ic->core.focus_window, KeyPress, KeyRelease,
			_XimLocalFilter, (XPointer)ic);
	    }
	} else
	    ic->core.focus_window = (Window)p->value;
    }
    return True;
}

static Bool
_XimEncodeLocalPreeditValue(
    Xic			 ic,
    XIMResourceList	 res,
    XPointer		 val)
{
    XIMArg		*p = (XIMArg *)val;

    if (res->xrm_name == XrmStringToQuark(XNStdColormap)) {
	XStandardColormap	*colormap_ret;
	int			 count;

	if (!(XGetRGBColormaps(ic->core.im->core.display,
				ic->core.focus_window, &colormap_ret,
				&count, (Atom)p->value)))
	    return False;

	Xfree(colormap_ret);
    }
    return True;
}

static Bool
_XimEncodeLocalStatusValue(
    Xic			 ic,
    XIMResourceList	 res,
    XPointer		 val)
{
    XIMArg		*p = (XIMArg *)val;

    if (res->xrm_name == XrmStringToQuark(XNStdColormap)) {
	XStandardColormap	*colormap_ret;
	int			 count;

	if (!(XGetRGBColormaps(ic->core.im->core.display,
				ic->core.focus_window, &colormap_ret,
				&count, (Atom)p->value)))
	    return False;

	Xfree(colormap_ret);
    }
    return True;
}

char *
_XimSetICValueData(
    Xic			 ic,
    XPointer		 top,
    XIMResourceList	 res_list,
    unsigned int	 list_num,
    XIMArg		*values,
    unsigned long	 mode,
    Bool		 flag)
{
    register  XIMArg	*p;
    XIMResourceList	 res;
    char		*name;
    int			 check;
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    for(p = values; p->name != NULL; p++) {
	if((res = _XimGetResourceListRec(res_list, list_num,
					p->name)) == (XIMResourceList)NULL) {
	    return p->name;
	}
	if(res->xrm_name == pre_quark) {
	    if(((name = _XimSetICValueData(ic,
			(XPointer)(&((XimDefICValues *)top)->preedit_attr),
			res_list, list_num, (XIMArg *)p->value,
			(mode | XIM_PREEDIT_ATTR), flag)))) {
		return name;
	    }
	} else if(res->xrm_name == sts_quark) {
	    if(((name = _XimSetICValueData(ic,
			(XPointer)(&((XimDefICValues *)top)->status_attr),
			res_list, list_num, (XIMArg *)p->value,
			(mode | XIM_STATUS_ATTR), flag)))) {
		return name;
	    }
	} else {
	    check = _XimCheckICMode(res, mode);
	    if(check == XIM_CHECK_INVALID) {
		continue;
	    } else if(check == XIM_CHECK_ERROR) {
		return p->name;
	    }

	    if(mode & XIM_PREEDIT_ATTR) {
		if (!_XimEncodeLocalPreeditValue(ic, res, (XPointer)p))
		    return p->name;
    	    } else if(mode & XIM_STATUS_ATTR) {
		if (!_XimEncodeLocalStatusValue(ic, res, (XPointer)p))
		    return p->name;
    	    } else {
		if (!_XimEncodeLocalTopValue(ic, res, (XPointer)p, flag))
		    return p->name;
    	    }
	    if(_XimEncodeLocalICAttr(ic, res, top, p, mode) == False) {
		return p->name;
	    }
	}
    }
    return NULL;
}

static Bool
_XimCheckInputStyle(
    XIMStyles		*styles,
    XIMStyle		 style)
{
    int			 num = styles->count_styles;
    register int	 i;

    for(i = 0; i < num; i++) {
	if(styles->supported_styles[i] == style) {
	    return True;
	}
    }
    return False;
}

Bool
_XimCheckLocalInputStyle(
    Xic			 ic,
    XPointer		 top,
    XIMArg		*values,
    XIMStyles		*styles,
    XIMResourceList	 res_list,
    unsigned int	 list_num)
{
    XrmQuark		 quark = XrmStringToQuark(XNInputStyle);
    register XIMArg	*p;
    XIMResourceList	 res;

    for(p = values; p && p->name != NULL; p++) {
	if(quark == XrmStringToQuark(p->name)) {
	    if(!(res = _XimGetResourceListRec(res_list, list_num, p->name))) {
		return False;
	    }
	    if(!_XimEncodeLocalICAttr(ic, res, top, p, 0)) {
		return False;
	    }
	    if (_XimCheckInputStyle(styles,
			((XimDefICValues *)top)->input_style)) {
		return True;
	    }
	    return False;
	}
    }
    return False;
}

static Bool
_XimDecodeAttr(
    XimValueOffsetInfo	 info,
    unsigned int	 num,
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val)
{
    register int	 i;

    for(i = 0; i < num; i++ ) {
	if(info[i].quark == res->xrm_name) {
	    if(!info[i].decode) {
		return False;
	    }
	    return (*info[i].decode)(&info[i], top, val);
	}
    }
    return False;
}

Bool
_XimDecodeLocalIMAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val)
{
    return _XimDecodeAttr(im_attr_info, XIMNumber(im_attr_info),
					res, top, val);
}

Bool
_XimDecodeLocalICAttr(
    XIMResourceList	 res,
    XPointer		 top,
    XPointer		 val,
    unsigned long	 mode)
{
    unsigned int	 num;
    XimValueOffsetInfo	 info;

    if(mode & XIM_PREEDIT_ATTR) {
	info = ic_pre_attr_info;
	num  = XIMNumber(ic_pre_attr_info);
    } else if(mode & XIM_STATUS_ATTR) {
	info = ic_sts_attr_info;
	num  = XIMNumber(ic_sts_attr_info);
    } else {
	info = ic_attr_info;
	num  = XIMNumber(ic_attr_info);
    }

    return _XimDecodeAttr(info, num, res, top, val);
}

char *
_XimGetICValueData(Xic ic, XPointer top, XIMResourceList res_list,
		   unsigned int	 list_num, XIMArg *values, unsigned long mode)
{
    register  XIMArg	*p;
    XIMResourceList	 res;
    char		*name;
    int			 check;
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    for(p = values; p->name != NULL; p++) {
	if((res = _XimGetResourceListRec(res_list, list_num,
					p->name)) == (XIMResourceList)NULL) {
	    return p->name;
	}
	if(res->xrm_name == pre_quark) {
	    if((name = _XimGetICValueData(ic,
			(XPointer)(&((XimDefICValues *)top)->preedit_attr),
			res_list, list_num, (XIMArg *)p->value,
			(mode | XIM_PREEDIT_ATTR)))) {
		return name;
	    }
	} else if(res->xrm_name == sts_quark) {
	    if((name = _XimGetICValueData(ic,
			(XPointer)(&((XimDefICValues *)top)->status_attr),
			res_list, list_num, (XIMArg *)p->value,
			(mode | XIM_STATUS_ATTR)))) {
		return name;
	    }
	} else {
	    check = _XimCheckICMode(res, mode);
	    if(check == XIM_CHECK_INVALID) {
		continue;
	    } else if(check == XIM_CHECK_ERROR) {
		return p->name;
	    }

	    if(_XimDecodeLocalICAttr(res, top, p->value, mode) == False) {
		return p->name;
	    }
	}
    }
    return NULL;
}

void
_XimGetCurrentIMValues(Xim im, XimDefIMValues *im_values)
{
    bzero((char *)im_values, sizeof(XimDefIMValues));

    im_values->styles		= im->core.styles;
    im_values->im_values_list	= im->core.im_values_list;
    im_values->ic_values_list	= im->core.ic_values_list;
    im_values->destroy_callback = im->core.destroy_callback;
    im_values->res_name		= im->core.res_name;
    im_values->res_class	= im->core.res_class;
    im_values->visible_position	= im->core.visible_position;
}

void
_XimSetCurrentIMValues(Xim im, XimDefIMValues *im_values)
{
    im->core.styles		= im_values->styles;
    im->core.im_values_list	= im_values->im_values_list;
    im->core.ic_values_list	= im_values->ic_values_list;
    im->core.destroy_callback	= im_values->destroy_callback;
    im->core.res_name		= im_values->res_name;
    im->core.res_class		= im_values->res_class;
    im->core.visible_position	= im_values->visible_position;
}

void
_XimGetCurrentICValues(Xic ic, XimDefICValues *ic_values)
{
    bzero((char *)ic_values, sizeof(XimDefICValues));

    ic_values->input_style	 = ic->core.input_style;
    ic_values->client_window	 = ic->core.client_window;
    ic_values->focus_window	 = ic->core.focus_window;
    ic_values->filter_events	 = ic->core.filter_events;
    ic_values->geometry_callback = ic->core.geometry_callback;
    ic_values->res_name		 = ic->core.res_name;
    ic_values->res_class	 = ic->core.res_class;
    ic_values->destroy_callback	 = ic->core.destroy_callback;
    ic_values->string_conversion_callback
				 = ic->core.string_conversion_callback;
    ic_values->string_conversion = ic->core.string_conversion;
    ic_values->reset_state	 = ic->core.reset_state;
    ic_values->hotkey		 = ic->core.hotkey;
    ic_values->hotkey_state	 = ic->core.hotkey_state;
    ic_values->preedit_attr	 = ic->core.preedit_attr;
    ic_values->status_attr	 = ic->core.status_attr;
}

void
_XimSetCurrentICValues(
    Xic			 ic,
    XimDefICValues	*ic_values)
{
    ic->core.input_style	= ic_values->input_style;
    ic->core.client_window	= ic_values->client_window;
    if (ic_values->focus_window)
	ic->core.focus_window	= ic_values->focus_window;
    ic->core.filter_events	= ic_values->filter_events;
    ic->core.geometry_callback	= ic_values->geometry_callback;
    ic->core.res_name		= ic_values->res_name;
    ic->core.res_class		= ic_values->res_class;
    ic->core.destroy_callback 	= ic_values->destroy_callback;
    ic->core.string_conversion_callback
				= ic_values->string_conversion_callback;
    ic->core.string_conversion	= ic_values->string_conversion;
    ic->core.reset_state	= ic_values->reset_state;
    ic->core.hotkey		= ic_values->hotkey;
    ic->core.hotkey_state	= ic_values->hotkey_state;
    ic->core.preedit_attr	= ic_values->preedit_attr;
    ic->core.status_attr	= ic_values->status_attr;
}

static void
_XimInitialIMOffsetInfo(void)
{
    unsigned int	 n = XIMNumber(im_attr_info);
    register int	 i;

    for(i = 0; i < n; i++) {
	im_attr_info[i].quark = XrmStringToQuark(GET_NAME(im_attr_info[i]));
    }
}

static void
_XimInitialICOffsetInfo(void)
{
    unsigned int	 n;
    register int	 i;

    n = XIMNumber(ic_attr_info);
    for(i = 0; i < n; i++) {
	ic_attr_info[i].quark = XrmStringToQuark(GET_NAME(ic_attr_info[i]));
    }

    n = XIMNumber(ic_pre_attr_info);
    for(i = 0; i < n; i++) {
	ic_pre_attr_info[i].quark = XrmStringToQuark(GET_NAME(ic_pre_attr_info[i]));
    }

    n = XIMNumber(ic_sts_attr_info);
    for(i = 0; i < n; i++) {
	ic_sts_attr_info[i].quark = XrmStringToQuark(GET_NAME(ic_sts_attr_info[i]));
    }
}

static void
_XimInitialIMMode(void)
{
    unsigned int	n = XIMNumber(im_mode);
    register int	i;

    for(i = 0; i < n; i++) {
	im_mode_quark[i] = XrmStringToQuark(GET_NAME(im_mode[i]));
    }
}

static void
_XimInitialICMode(void)
{
    unsigned int	n = XIMNumber(ic_mode);
    register int	i;

    for(i = 0; i < n; i++) {
	ic_mode_quark[i] = XrmStringToQuark(GET_NAME(ic_mode[i]));
    }
}

void
_XimInitialResourceInfo(void)
{
    static Bool	init_flag = False;

    if(init_flag == True) {
	return;
    }
    _XimInitialIMOffsetInfo();
    _XimInitialICOffsetInfo();
    _XimInitialIMMode();
    _XimInitialICMode();
    init_flag = True;
}
