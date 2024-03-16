/*

Copyright 1986, 1998  The Open Group

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
#include "Xlibint.h"

#ifdef USE_DYNAMIC_XCURSOR

#ifdef __UNIXOS2__
#define RTLD_LAZY 1
#define LIBXCURSOR "Xcursor.dll"
#endif
#include <stdio.h>
#include <string.h>
#if defined(hpux)
#include <dl.h>
#else
#include <dlfcn.h>
#endif
#include "Cr.h"

#ifdef __CYGWIN__
#define LIBXCURSOR "cygXcursor-1.dll"
#endif

#if defined(hpux)
typedef shl_t	XModuleType;
#else
typedef void *XModuleType;
#endif

#ifndef LIBXCURSOR
#define LIBXCURSOR "libXcursor.so.1"
#endif

static char libraryName[] = LIBXCURSOR;

static XModuleType
open_library (void)
{
    char	*library = libraryName;
    char	*dot;
    XModuleType	module;
    for (;;)
    {
#if defined(hpux)
	module = shl_load(library, BIND_DEFERRED, 0L);
#else
	module =  dlopen(library, RTLD_LAZY);
#endif
	if (module)
	    return module;
	dot = strrchr (library, '.');
	if (!dot)
	    break;
	*dot = '\0';
    }
    return NULL;
}

static void *
fetch_symbol (XModuleType module, const char *under_symbol)
{
    void *result = NULL;
    const char *symbol = under_symbol + 1;
#if defined(hpux)
    int getsyms_cnt, i;
    struct shl_symbol *symbols;

    getsyms_cnt = shl_getsymbols(module, TYPE_PROCEDURE,
				 EXPORT_SYMBOLS, malloc, &symbols);

    for(i=0; i<getsyms_cnt; i++) {
        if(!strcmp(symbols[i].name, symbol)) {
	    result = symbols[i].value;
	    break;
         }
    }

    if(getsyms_cnt > 0) {
        free(symbols);
    }
#else
    result = dlsym (module, symbol);
    if (!result)
	result = dlsym (module, under_symbol);
#endif
    return result;
}

typedef void	(*NoticeCreateBitmapFunc) (Display	    *dpy,
					   Pixmap	    pid,
					   unsigned int width,
					   unsigned int height);

typedef void	(*NoticePutBitmapFunc) (Display	    *dpy,
					Drawable    draw,
					XImage	    *image);

typedef Cursor	(*TryShapeBitmapCursorFunc) (Display	    *dpy,
					     Pixmap	    source,
					     Pixmap	    mask,
					     XColor	    *foreground,
					     XColor	    *background,
					     unsigned int   x,
					     unsigned int   y);

typedef Cursor	(*TryShapeCursorFunc) (Display	    *dpy,
				       Font	    source_font,
				       Font	    mask_font,
				       unsigned int source_char,
				       unsigned int mask_char,
				       XColor _Xconst *foreground,
				       XColor _Xconst *background);

static XModuleType  _XcursorModule;
static Bool	    _XcursorModuleTried;

#define GetFunc(type,name,ret) {\
    static Bool	    been_here; \
    static type	    staticFunc; \
     \
    _XLockMutex (_Xglobal_lock); \
    if (!been_here) \
    { \
	been_here = True; \
	if (!_XcursorModuleTried) \
	{ \
	    _XcursorModuleTried = True; \
	    _XcursorModule = open_library (); \
	} \
	if (_XcursorModule) \
	    staticFunc = (type) fetch_symbol (_XcursorModule, "_" name); \
    } \
    ret = staticFunc; \
    _XUnlockMutex (_Xglobal_lock); \
}

static Cursor
_XTryShapeCursor (Display	    *dpy,
		  Font		    source_font,
		  Font		    mask_font,
		  unsigned int	    source_char,
		  unsigned int	    mask_char,
		  XColor _Xconst    *foreground,
		  XColor _Xconst    *background)
{
    TryShapeCursorFunc		func;

    GetFunc (TryShapeCursorFunc, "XcursorTryShapeCursor", func);
    if (func)
	return (*func) (dpy, source_font, mask_font, source_char, mask_char,
			foreground, background);
    return None;
}

void
_XNoticeCreateBitmap (Display	    *dpy,
		      Pixmap	    pid,
		      unsigned int  width,
		      unsigned int  height)
{
    NoticeCreateBitmapFunc  func;

    GetFunc (NoticeCreateBitmapFunc, "XcursorNoticeCreateBitmap", func);
    if (func)
	(*func) (dpy, pid, width, height);
}

void
_XNoticePutBitmap (Display	*dpy,
		   Drawable	draw,
		   XImage	*image)
{
    NoticePutBitmapFunc	func;

    GetFunc (NoticePutBitmapFunc, "XcursorNoticePutBitmap", func);
    if (func)
	(*func) (dpy, draw, image);
}

Cursor
_XTryShapeBitmapCursor (Display		*dpy,
			Pixmap		source,
			Pixmap		mask,
			XColor		*foreground,
			XColor		*background,
			unsigned int	x,
			unsigned int	y)
{
    TryShapeBitmapCursorFunc	func;

    GetFunc (TryShapeBitmapCursorFunc, "XcursorTryShapeBitmapCursor", func);
    if (func)
	return (*func) (dpy, source, mask, foreground, background, x, y);
    return None;
}
#endif

Cursor XCreateGlyphCursor(
     register Display *dpy,
     Font source_font,
     Font mask_font,
     unsigned int source_char,
     unsigned int mask_char,
     XColor _Xconst *foreground,
     XColor _Xconst *background)
{
    Cursor cid;
    register xCreateGlyphCursorReq *req;

#ifdef USE_DYNAMIC_XCURSOR
    cid = _XTryShapeCursor (dpy, source_font, mask_font,
			    source_char, mask_char, foreground, background);
    if (cid)
	return cid;
#endif
    LockDisplay(dpy);
    GetReq(CreateGlyphCursor, req);
    cid = req->cid = XAllocID(dpy);
    req->source = source_font;
    req->mask = mask_font;
    req->sourceChar = source_char;
    req->maskChar = mask_char;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
    return (cid);
}

