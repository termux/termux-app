
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

#ifndef _XATOMTYPE_H_
#define _XATOMTYPE_H_

/*
 * This files defines crock C structures for calling XGetWindowProperty and
 * XChangeProperty.  All fields must be longs as the semantics of property
 * routines will handle conversion to and from actual 32 bit objects.  If your
 * compiler doesn't treat &structoflongs the same as &arrayoflongs[0], you
 * will have some work to do.
 */

#define BOOL long
#define SIGNEDINT long
#define UNSIGNEDINT unsigned long
#define RESOURCEID unsigned long


/* this structure may be extended, but do not change the order */
typedef struct {
    UNSIGNEDINT flags;
    SIGNEDINT x, y, width, height;	/* need to cvt; only for pre-ICCCM */
    SIGNEDINT minWidth, minHeight;	/* need to cvt */
    SIGNEDINT maxWidth, maxHeight;	/* need to cvt */
    SIGNEDINT widthInc, heightInc;	/* need to cvt */
    SIGNEDINT minAspectX, minAspectY;	/* need to cvt */
    SIGNEDINT maxAspectX, maxAspectY;	/* need to cvt */
    SIGNEDINT baseWidth,baseHeight;	/* need to cvt; ICCCM version 1 */
    SIGNEDINT winGravity;		/* need to cvt; ICCCM version 1 */
} xPropSizeHints;
#define OldNumPropSizeElements 15	/* pre-ICCCM */
#define NumPropSizeElements 18		/* ICCCM version 1 */

/* this structure may be extended, but do not change the order */
/* RGB properties */
typedef struct {
	RESOURCEID colormap;
	UNSIGNEDINT red_max;
	UNSIGNEDINT red_mult;
	UNSIGNEDINT green_max;
	UNSIGNEDINT green_mult;
	UNSIGNEDINT blue_max;
	UNSIGNEDINT blue_mult;
	UNSIGNEDINT base_pixel;
	RESOURCEID visualid;		/* ICCCM version 1 */
	RESOURCEID killid;		/* ICCCM version 1 */
} xPropStandardColormap;
#define OldNumPropStandardColormapElements 8  /* pre-ICCCM */
#define NumPropStandardColormapElements 10  /* ICCCM version 1 */


/* this structure may be extended, but do not change the order */
typedef struct {
    UNSIGNEDINT flags;
    BOOL input;				/* need to convert */
    SIGNEDINT initialState;		/* need to cvt */
    RESOURCEID iconPixmap;
    RESOURCEID iconWindow;
    SIGNEDINT  iconX;			/* need to cvt */
    SIGNEDINT  iconY;			/* need to cvt */
    RESOURCEID iconMask;
    UNSIGNEDINT windowGroup;
  } xPropWMHints;
#define NumPropWMHintsElements 9 /* number of elements in this structure */

/* this structure defines the icon size hints information */
typedef struct {
    SIGNEDINT minWidth, minHeight;	/* need to cvt */
    SIGNEDINT maxWidth, maxHeight;	/* need to cvt */
    SIGNEDINT widthInc, heightInc;	/* need to cvt */
  } xPropIconSize;
#define NumPropIconSizeElements 6 /* number of elements in this structure */

/* this structure defines the window manager state information */
typedef struct {
    SIGNEDINT state;			/* need to cvt */
    RESOURCEID iconWindow;
} xPropWMState;
#define NumPropWMStateElements 2	/* number of elements in struct */

#undef BOOL
#undef SIGNEDINT
#undef UNSIGNEDINT
#undef RESOURCEID

#endif /* _XATOMTYPE_H_ */
