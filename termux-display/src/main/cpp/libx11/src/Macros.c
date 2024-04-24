/*

Copyright 1987, 1998  The Open Group

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
#define XUTIL_DEFINE_FUNCTIONS
#include "Xutil.h"
#include "Xxcbint.h"

/*
 * This file makes full definitions of routines for each macro.
 * We do not expect C programs to use these, but other languages may
 * need them.
 */

int XConnectionNumber(Display *dpy) { return (ConnectionNumber(dpy)); }

Window XRootWindow (Display *dpy, int scr)
{
    return (RootWindow(dpy,scr));
}

int XDefaultScreen(Display *dpy) { return (DefaultScreen(dpy)); }

Window XDefaultRootWindow (Display *dpy)
{
    return (RootWindow(dpy,DefaultScreen(dpy)));
}

Visual *XDefaultVisual(Display *dpy, int scr)
{
    return (DefaultVisual(dpy, scr));
}

GC XDefaultGC(Display *dpy, int scr)
{
    return (DefaultGC(dpy,scr));
}

unsigned long XBlackPixel(Display *dpy, int scr)
{
    return (BlackPixel(dpy, scr));
}

unsigned long XWhitePixel(Display *dpy, int scr)
{
    return (WhitePixel(dpy,scr));
}

unsigned long XAllPlanes(void) { return AllPlanes; }

int XQLength(Display *dpy) { return (QLength(dpy)); }

int XDisplayWidth(Display *dpy, int scr)
{
    return (DisplayWidth(dpy,scr));
}

int XDisplayHeight(Display *dpy, int scr)
{
    return (DisplayHeight(dpy, scr));
}

int XDisplayWidthMM(Display *dpy, int scr)
{
    return (DisplayWidthMM(dpy, scr));
}

int XDisplayHeightMM(Display *dpy, int scr)
{
    return (DisplayHeightMM(dpy, scr));
}

int XDisplayPlanes(Display *dpy, int scr)
{
    return (DisplayPlanes(dpy, scr));
}

int XDisplayCells(Display *dpy, int scr)
{
    return (DisplayCells (dpy, scr));
}

int XScreenCount(Display *dpy) { return (ScreenCount(dpy)); }

char *XServerVendor(Display *dpy) { return (ServerVendor(dpy)); }

int XProtocolVersion(Display *dpy) { return (ProtocolVersion(dpy)); }

int XProtocolRevision(Display *dpy) { return (ProtocolRevision(dpy));}

int XVendorRelease(Display *dpy) { return (VendorRelease(dpy)); }

char *XDisplayString(Display *dpy) { return (DisplayString(dpy)); }

int XDefaultDepth(Display *dpy, int scr)
{
    return(DefaultDepth(dpy, scr));
}

Colormap XDefaultColormap(Display *dpy, int scr)
{
    return (DefaultColormap(dpy, scr));
}

int XBitmapUnit(Display *dpy) { return (BitmapUnit(dpy)); }

int XBitmapBitOrder(Display *dpy) { return (BitmapBitOrder(dpy)); }

int XBitmapPad(Display *dpy) { return (BitmapPad(dpy)); }

int XImageByteOrder(Display *dpy) { return (ImageByteOrder(dpy)); }

/* XNextRequest() differs from the rest of the functions here because it is
 * no longer a macro wrapper - when libX11 is being used mixed together
 * with direct use of xcb, the next request field of the Display structure will
 * not be updated. We can't fix the NextRequest() macro in any easy way,
 * but we can at least make XNextRequest() do the right thing.
 */
unsigned long XNextRequest(Display *dpy)
{
    unsigned long next_request;
    LockDisplay(dpy);
    next_request = _XNextRequest(dpy);
    UnlockDisplay(dpy);

    return next_request;
}

unsigned long XLastKnownRequestProcessed(Display *dpy)
{
    return (LastKnownRequestProcessed(dpy));
}

/* screen oriented macros (toolkit) */
Screen *XScreenOfDisplay(Display *dpy, int scr)
{
    return (ScreenOfDisplay(dpy, scr));
}

Screen *XDefaultScreenOfDisplay(Display *dpy)
{
    return (DefaultScreenOfDisplay(dpy));
}

Display *XDisplayOfScreen(Screen *s) { return (DisplayOfScreen(s)); }

Window XRootWindowOfScreen(Screen *s) { return (RootWindowOfScreen(s)); }

unsigned long XBlackPixelOfScreen(Screen *s)
{
    return (BlackPixelOfScreen(s));
}

unsigned long XWhitePixelOfScreen(Screen *s)
{
    return (WhitePixelOfScreen(s));
}

Colormap XDefaultColormapOfScreen(Screen *s)
{
    return (DefaultColormapOfScreen(s));
}

int XDefaultDepthOfScreen(Screen *s)
{
    return (DefaultDepthOfScreen(s));
}

GC XDefaultGCOfScreen(Screen *s)
{
    return (DefaultGCOfScreen(s));
}

Visual *XDefaultVisualOfScreen(Screen *s)
{
    return (DefaultVisualOfScreen(s));
}

int XWidthOfScreen(Screen *s) { return (WidthOfScreen(s)); }

int XHeightOfScreen(Screen *s) { return (HeightOfScreen(s)); }

int XWidthMMOfScreen(Screen *s) { return (WidthMMOfScreen(s)); }

int XHeightMMOfScreen(Screen *s) { return (HeightMMOfScreen(s)); }

int XPlanesOfScreen(Screen *s) { return (PlanesOfScreen(s)); }

int XCellsOfScreen(Screen *s) { return (CellsOfScreen(s)); }

int XMinCmapsOfScreen(Screen *s) { return (MinCmapsOfScreen(s)); }

int XMaxCmapsOfScreen(Screen *s) { return (MaxCmapsOfScreen(s)); }

Bool XDoesSaveUnders(Screen *s) { return (DoesSaveUnders(s)); }

int XDoesBackingStore(Screen *s) { return (DoesBackingStore(s)); }

long XEventMaskOfScreen(Screen *s) { return (EventMaskOfScreen(s)); }

int XScreenNumberOfScreen (register Screen *scr)
{
    register Display *dpy = scr->display;
    register Screen *dpyscr = dpy->screens;
    register int i;

    for (i = 0; i < dpy->nscreens; i++, dpyscr++) {
	if (scr == dpyscr) return i;
    }
    return -1;
}

/*
 * These macros are used to give some sugar to the image routines so that
 * naive people are more comfortable with them.
 */
#undef XDestroyImage
int
XDestroyImage(
	XImage *ximage)
{
	return((*((ximage)->f.destroy_image))((ximage)));
}
#undef XGetPixel
unsigned long XGetPixel(
	XImage *ximage,
	int x, int y)
{
	return ((*((ximage)->f.get_pixel))((ximage), (x), (y)));
}
#undef XPutPixel
int XPutPixel(
	XImage *ximage,
	int x, int y,
	unsigned long pixel)
{
	return((*((ximage)->f.put_pixel))((ximage), (x), (y), (pixel)));
}
#undef XSubImage
XImage *XSubImage(
	XImage *ximage,
	int x, int y,
	unsigned int width, unsigned int height)
{
	return((*((ximage)->f.sub_image))((ximage), (x),
		(y), (width), (height)));
}
#undef XAddPixel
int XAddPixel(
	XImage *ximage,
	long value)
{
	return((*((ximage)->f.add_pixel))((ximage), (value)));
}


int
XNoOp (register Display *dpy)
{
    _X_UNUSED register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(NoOperation, req);

    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
