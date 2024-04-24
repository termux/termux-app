/******************************************************************

           Copyright 1993, 1994 by Sony Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sony Corporation
 not be used in advertising or publicity pertaining to distribution
 of the software without specific, written prior permission.
Sony Corporation makes no representations about the suitability of
 this software for any purpose. It is provided "as is" without
 express or implied warranty.

SONY CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL SONY CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author:   Makoto Wakamatsu   Sony Corporation
                               makoto@sm.sony.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include	<X11/Xatom.h>
#include	"Xlibint.h"
#include	"Xlcint.h"
#include	"XlcPublic.h"
#include	"Ximint.h"


typedef struct _XimInstCallback {
    Bool			 call;
    Bool			 destroy;
    Display			*display;
    XLCd			 lcd;
    char			 name[XIM_MAXLCNAMELEN];
    char			*modifiers;
    XrmDatabase			 rdb;
    char			*res_name;
    char			*res_class;
    XIDProc			 callback;
    XPointer			 client_data;
    struct _XimInstCallback	*next;
} XimInstCallbackRec, *XimInstCallback;


static XimInstCallback	callback_list	= NULL;
static Bool		lock		= False;


static void
MakeLocale( XLCd lcd, char locale[] )
{
    char	*language, *territory, *codeset;

    _XGetLCValues( lcd, XlcNLanguage, &language, XlcNTerritory, &territory,
		     XlcNCodeset, &codeset, NULL );

    strcpy( locale, language );
    if( territory  &&  *territory ) {
	strcat( locale, "_" );
	strcat( locale, territory );
    }
    if( codeset  &&  *codeset ) {
	strcat( locale, "." );
	strcat( locale, codeset );
    }
}


static Bool
_XimFilterPropertyNotify(
    Display	*display,
    Window	 window,
    XEvent	*event,
    XPointer	 client_data)
{
    Atom		ims, actual_type, *atoms;
    int			actual_format;
    unsigned long	nitems, bytes_after;
    int			ii;
    XIM			xim;
    Bool		flag = False;
    XimInstCallback	icb, picb, tmp;

    if( (ims = XInternAtom( display, XIM_SERVERS, True )) == None  ||
	event->xproperty.atom != ims  ||
	event->xproperty.state == PropertyDelete )
	return( False );

    if( XGetWindowProperty( display, RootWindow(display, 0), ims, 0L, 1000000L,
			    False, XA_ATOM, &actual_type, &actual_format,
			    &nitems, &bytes_after, (unsigned char **)&atoms )
	    != Success ) {
	return( False );
    }
    if( actual_type != XA_ATOM  ||  actual_format != 32 ) {
	XFree( atoms );
	return( False );
    }

    lock = True;
    for( ii = 0; ii < nitems; ii++ ) {
	if(XGetSelectionOwner (display, atoms[ii])) {
	    for( icb = callback_list; icb; icb = icb->next ) {
		if( !icb->call  &&  !icb->destroy ) {
		    xim = (*icb->lcd->methods->open_im)( icb->lcd, display,
							 icb->rdb,
							 icb->res_name,
							 icb->res_class );
		    if( xim ) {
			xim->methods->close( (XIM)xim );
			flag = True;
			icb->call = True;
			icb->callback( icb->display, icb->client_data, NULL );
		    }
		}
	    }
	    break;
	}
    }
    XFree( atoms );

    for( icb = callback_list, picb = NULL; icb; ) {
	if( icb->destroy ) {
	    if( picb )
		picb->next = icb->next;
	    else
		callback_list = icb->next;
	    tmp = icb;
	    icb = icb->next;
	    XFree( tmp );
	}
	else {
	    picb = icb;
	    icb = icb->next;
	}
    }
    lock = False;

    return( flag );
}


Bool
_XimRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char	*res_class,
    XIDProc	 callback,
    XPointer	 client_data)
{
    XimInstCallback	icb, tmp;
    XIM			xim;
    char		*modifiers = NULL;
    Window		root;
    XWindowAttributes	attr;

    if( lock )
	return( False );

    icb = Xmalloc(sizeof(XimInstCallbackRec));
    if( !icb )
	return( False );
    if (lcd->core->modifiers) {
	modifiers = strdup(lcd->core->modifiers);
	if (!modifiers) {
	    Xfree(icb);
	    return( False );
	}
    }
    icb->call = icb->destroy = False;
    icb->display = display;
    icb->lcd = lcd;
    MakeLocale( lcd, icb->name );
    icb->modifiers = modifiers;
    icb->rdb = rdb;
    icb->res_name = res_name;
    icb->res_class = res_class;
    icb->callback = callback;
    icb->client_data = client_data;
    icb->next = NULL;

    if( !callback_list )
	callback_list = icb;
    else {
	for( tmp = callback_list; tmp->next; tmp = tmp->next );
	tmp->next = icb;
    }

    xim = (*lcd->methods->open_im)( lcd, display, rdb, res_name, res_class );

    if( icb == callback_list ) {
	root = RootWindow( display, 0 );
	XGetWindowAttributes( display, root, &attr );
	_XRegisterFilterByType( display, root, PropertyNotify, PropertyNotify,
				_XimFilterPropertyNotify, (XPointer)NULL );
	XSelectInput( display, root,
		      attr.your_event_mask | PropertyChangeMask );
    }

    if( xim ) {
	lock = True;
	xim->methods->close( (XIM)xim );
	/* XIMs must be freed manually after being opened; close just
	   does the protocol to deinitialize the IM.  */
	XFree( xim );
	lock = False;
	icb->call = True;
	callback( display, client_data, NULL );
    }

    return( True );
}


Bool
_XimUnRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char	*res_class,
    XIDProc	 callback,
    XPointer	 client_data)
{
    char		locale[XIM_MAXLCNAMELEN];
    XimInstCallback	icb, picb;

    if( !callback_list )
	return( False );

    MakeLocale( lcd, locale );

    for( icb = callback_list, picb = NULL; icb; picb = icb, icb = icb->next ) {
	if( !strcmp( locale, icb->name )  &&
	    (lcd->core->modifiers == icb->modifiers  ||		/* XXXXX */
	     (lcd->core->modifiers  &&  icb->modifiers  &&
	      !strcmp( lcd->core->modifiers, icb->modifiers )))  &&
	    rdb == icb->rdb  &&					/* XXXXX */
	    ((res_name == NULL  &&  icb->res_name == NULL)  ||
	     (res_name != NULL  &&  icb->res_name != NULL  &&
	      !strcmp( res_name, icb->res_name )))  &&
	    ((res_class == NULL  &&  icb->res_class == NULL)  ||
	     (res_class != NULL  &&  icb->res_class != NULL  &&
	      !strcmp( res_class, icb->res_class )))  &&
	    (callback == icb->callback)  &&
	    (client_data  ==  icb->client_data)  &&		/* XXXXX */
	    !icb->destroy ) {
	    if( lock )
		icb->destroy = True;
	    else {
		if( !picb ) {
		    callback_list = icb->next;
		    _XUnregisterFilter( display, RootWindow(display, 0),
					_XimFilterPropertyNotify,
					(XPointer)NULL );
		}
		else
		    picb->next = icb->next;
		_XCloseLC( icb->lcd );
		XFree( icb->modifiers );
		XFree( icb );
	    }
	    return( True );
	}
    }
    return( False );
}


void
_XimResetIMInstantiateCallback(Xim xim)
{
    char		 locale[XIM_MAXLCNAMELEN];
    XimInstCallback	 icb;
    XLCd	 	 lcd = xim->core.lcd;

    if( !callback_list  &&  lock )
	return;

    MakeLocale( lcd, locale );

    for( icb = callback_list; icb; icb = icb->next )
	if( !strcmp( locale, icb->name )  &&
	    (lcd->core->modifiers == icb->modifiers  ||
	     (lcd->core->modifiers  &&  icb->modifiers  &&
	      !strcmp( lcd->core->modifiers, icb->modifiers ))) )
	    icb->call = False;
}
