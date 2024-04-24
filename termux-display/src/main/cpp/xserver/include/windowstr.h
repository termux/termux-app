/***********************************************************

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef WINDOWSTRUCT_H
#define WINDOWSTRUCT_H

#include "window.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "cursor.h"
#include "property.h"
#include "resource.h"           /* for ROOT_WINDOW_ID_BASE */
#include "dix.h"
#include "privates.h"
#include "miscstruct.h"
#include <X11/Xprotostr.h>
#include "opaque.h"

#define GuaranteeNothing	0
#define GuaranteeVisBack	1

#define SameBackground(as, a, bs, b)				\
    ((as) == (bs) && ((as) == None ||				\
		      (as) == ParentRelative ||			\
 		      SamePixUnion(a,b,as == BackgroundPixel)))

#define SameBorder(as, a, bs, b)				\
    EqualPixUnion(as, a, bs, b)

/* used as NULL-terminated list */
typedef struct _DevCursorNode {
    CursorPtr cursor;
    DeviceIntPtr dev;
    struct _DevCursorNode *next;
} DevCursNodeRec, *DevCursNodePtr, *DevCursorList;

typedef struct _WindowOpt {
    CursorPtr cursor;           /* default: window.cursorNone */
    VisualID visual;            /* default: same as parent */
    Colormap colormap;          /* default: same as parent */
    Mask dontPropagateMask;     /* default: window.dontPropagate */
    Mask otherEventMasks;       /* default: 0 */
    struct _OtherClients *otherClients; /* default: NULL */
    struct _GrabRec *passiveGrabs;      /* default: NULL */
    PropertyPtr userProps;      /* default: NULL */
    CARD32 backingBitPlanes;    /* default: ~0L */
    CARD32 backingPixel;        /* default: 0 */
    RegionPtr boundingShape;    /* default: NULL */
    RegionPtr clipShape;        /* default: NULL */
    RegionPtr inputShape;       /* default: NULL */
    struct _OtherInputMasks *inputMasks;        /* default: NULL */
    DevCursorList deviceCursors;        /* default: NULL */
} WindowOptRec, *WindowOptPtr;

#define BackgroundPixel	    2L
#define BackgroundPixmap    3L

/*
 * The redirectDraw field can have one of three values:
 *
 *  RedirectDrawNone
 *	A normal window; painted into the same pixmap as the parent
 *	and clipping parent and siblings to its geometry. These
 *	windows get a clip list equal to the intersection of their
 *	geometry with the parent geometry, minus the geometry
 *	of overlapping None and Clipped siblings.
 *  RedirectDrawAutomatic
 *	A redirected window which clips parent and sibling drawing.
 *	Contents for these windows are manage inside the server.
 *	These windows get an internal clip list equal to their
 *	geometry.
 *  RedirectDrawManual
 *	A redirected window which does not clip parent and sibling
 *	drawing; the window must be represented within the parent
 *	geometry by the client performing the redirection management.
 *	Contents for these windows are managed outside the server.
 *	These windows get an internal clip list equal to their
 *	geometry.
 */

#define RedirectDrawNone	0
#define RedirectDrawAutomatic	1
#define RedirectDrawManual	2

typedef struct _Window {
    DrawableRec drawable;
    PrivateRec *devPrivates;
    WindowPtr parent;           /* ancestor chain */
    WindowPtr nextSib;          /* next lower sibling */
    WindowPtr prevSib;          /* next higher sibling */
    WindowPtr firstChild;       /* top-most child */
    WindowPtr lastChild;        /* bottom-most child */
    RegionRec clipList;         /* clipping rectangle for output */
    RegionRec borderClip;       /* NotClippedByChildren + border */
    union _Validate *valdata;
    RegionRec winSize;
    RegionRec borderSize;
    DDXPointRec origin;         /* position relative to parent */
    unsigned short borderWidth;
    unsigned short deliverableEvents;   /* all masks from all clients */
    Mask eventMask;             /* mask from the creating client */
    PixUnion background;
    PixUnion border;
    WindowOptPtr optional;
    unsigned backgroundState:2; /* None, Relative, Pixel, Pixmap */
    unsigned borderIsPixel:1;
    unsigned cursorIsNone:1;    /* else real cursor (might inherit) */
    unsigned backingStore:2;
    unsigned saveUnder:1;
    unsigned bitGravity:4;
    unsigned winGravity:4;
    unsigned overrideRedirect:1;
    unsigned visibility:2;
    unsigned mapped:1;
    unsigned realized:1;        /* ancestors are all mapped */
    unsigned viewable:1;        /* realized && InputOutput */
    unsigned dontPropagate:3;   /* index into DontPropagateMasks */
    unsigned redirectDraw:2;    /* COMPOSITE rendering redirect */
    unsigned forcedBG:1;        /* must have an opaque background */
    unsigned unhittable:1;      /* doesn't hit-test, for rootless */
#ifdef COMPOSITE
    unsigned damagedDescendants:1;      /* some descendants are damaged */
    unsigned inhibitBGPaint:1;  /* paint the background? */
#endif
} WindowRec;

/*
 * Ok, a bunch of macros for accessing the optional record
 * fields (or filling the appropriate default value)
 */

extern _X_EXPORT Mask DontPropagateMasks[];

#define wTrackParent(w,field)	((w)->optional ? \
				    (w)->optional->field \
 				 : FindWindowWithOptional(w)->optional->field)
#define wUseDefault(w,field,def)	((w)->optional ? \
				    (w)->optional->field \
				 : def)

#define wVisual(w)		wTrackParent(w, visual)
#define wCursor(w)		((w)->cursorIsNone ? None : wTrackParent(w, cursor))
#define wColormap(w)		((w)->drawable.class == InputOnly ? None : wTrackParent(w, colormap))
#define wDontPropagateMask(w)	wUseDefault(w, dontPropagateMask, DontPropagateMasks[(w)->dontPropagate])
#define wOtherEventMasks(w)	wUseDefault(w, otherEventMasks, 0)
#define wOtherClients(w)	wUseDefault(w, otherClients, NULL)
#define wOtherInputMasks(w)	wUseDefault(w, inputMasks, NULL)
#define wPassiveGrabs(w)	wUseDefault(w, passiveGrabs, NULL)
#define wUserProps(w)		wUseDefault(w, userProps, NULL)
#define wBackingBitPlanes(w)	wUseDefault(w, backingBitPlanes, ~0L)
#define wBackingPixel(w)	wUseDefault(w, backingPixel, 0)
#define wBoundingShape(w)	wUseDefault(w, boundingShape, NULL)
#define wClipShape(w)		wUseDefault(w, clipShape, NULL)
#define wInputShape(w)          wUseDefault(w, inputShape, NULL)
#define wClient(w)		(clients[CLIENT_ID((w)->drawable.id)])
#define wBorderWidth(w)		((int) (w)->borderWidth)

/* true when w needs a border drawn. */

#define HasBorder(w)	((w)->borderWidth || wClipShape(w))

typedef struct _ScreenSaverStuff *ScreenSaverStuffPtr;

#define SCREEN_IS_BLANKED   0
#define SCREEN_ISNT_SAVED   1
#define SCREEN_IS_TILED     2
#define SCREEN_IS_BLACK	    3

#define HasSaverWindow(pScreen)   (pScreen->screensaver.pWindow != NullWindow)

extern _X_EXPORT int screenIsSaved;

#endif                          /* WINDOWSTRUCT_H */
