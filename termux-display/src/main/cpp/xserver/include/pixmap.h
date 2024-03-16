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

#ifndef PIXMAP_H
#define PIXMAP_H

#include "misc.h"
#include "screenint.h"
#include "regionstr.h"
#include <X11/extensions/randr.h>
/* types for Drawable */
#define DRAWABLE_WINDOW 0
#define DRAWABLE_PIXMAP 1
#define UNDRAWABLE_WINDOW 2

/* corresponding type masks for dixLookupDrawable() */
#define M_DRAWABLE_WINDOW	(1<<0)
#define M_DRAWABLE_PIXMAP	(1<<1)
#define M_UNDRAWABLE_WINDOW	(1<<2)
#define M_ANY			(-1)
#define M_WINDOW	(M_DRAWABLE_WINDOW|M_UNDRAWABLE_WINDOW)
#define M_DRAWABLE	(M_DRAWABLE_WINDOW|M_DRAWABLE_PIXMAP)
#define M_UNDRAWABLE	(M_UNDRAWABLE_WINDOW)

/* flags to PaintWindow() */
#define PW_BACKGROUND 0
#define PW_BORDER 1

#define NullPixmap ((PixmapPtr)0)

typedef struct _Drawable *DrawablePtr;
typedef struct _Pixmap *PixmapPtr;

typedef struct _PixmapDirtyUpdate *PixmapDirtyUpdatePtr;

typedef union _PixUnion {
    PixmapPtr pixmap;
    unsigned long pixel;
} PixUnion;

#define SamePixUnion(a,b,isPixel)\
    ((isPixel) ? (a).pixel == (b).pixel : (a).pixmap == (b).pixmap)

#define EqualPixUnion(as, a, bs, b)				\
    ((as) == (bs) && (SamePixUnion (a, b, as)))

#define OnScreenDrawable(type) \
	(type == DRAWABLE_WINDOW)

#define WindowDrawable(type) \
	((type == DRAWABLE_WINDOW) || (type == UNDRAWABLE_WINDOW))

extern _X_EXPORT PixmapPtr GetScratchPixmapHeader(ScreenPtr pScreen,
                                                  int width,
                                                  int height,
                                                  int depth,
                                                  int bitsPerPixel,
                                                  int devKind,
                                                  void *pPixData);

extern _X_EXPORT void FreeScratchPixmapHeader(PixmapPtr /*pPixmap */ );

extern _X_EXPORT Bool CreateScratchPixmapsForScreen(ScreenPtr /*pScreen */ );

extern _X_EXPORT void FreeScratchPixmapsForScreen(ScreenPtr /*pScreen */ );

extern _X_EXPORT PixmapPtr AllocatePixmap(ScreenPtr /*pScreen */ ,
                                          int /*pixDataSize */ );

extern _X_EXPORT void FreePixmap(PixmapPtr /*pPixmap */ );

extern _X_EXPORT PixmapPtr
PixmapShareToSecondary(PixmapPtr pixmap, ScreenPtr secondary);

extern _X_EXPORT void
PixmapUnshareSecondaryPixmap(PixmapPtr secondary_pixmap);

#define HAS_DIRTYTRACKING_ROTATION 1
#define HAS_DIRTYTRACKING_DRAWABLE_SRC 1
extern _X_EXPORT Bool
PixmapStartDirtyTracking(DrawablePtr src,
                         PixmapPtr slave_dst,
                         int x, int y, int dst_x, int dst_y,
                         Rotation rotation);

extern _X_EXPORT Bool
PixmapStopDirtyTracking(DrawablePtr src, PixmapPtr slave_dst);

/* helper function, drivers can do this themselves if they can do it more
   efficiently */
extern _X_EXPORT Bool
PixmapSyncDirtyHelper(PixmapDirtyUpdatePtr dirty);

#endif                          /* PIXMAP_H */
