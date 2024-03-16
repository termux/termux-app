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

#ifndef PIXMAPSTRUCT_H
#define PIXMAPSTRUCT_H
#include "pixmap.h"
#include "screenint.h"
#include "regionstr.h"
#include "privates.h"
#include "damage.h"
#include <X11/extensions/randr.h>
#include "picturestr.h"

typedef struct _Drawable {
    unsigned char type;         /* DRAWABLE_<type> */
    unsigned char class;        /* specific to type */
    unsigned char depth;
    unsigned char bitsPerPixel;
    XID id;                     /* resource id */
    short x;                    /* window: screen absolute, pixmap: 0 */
    short y;                    /* window: screen absolute, pixmap: 0 */
    unsigned short width;
    unsigned short height;
    ScreenPtr pScreen;
    unsigned long serialNumber;
} DrawableRec;

/*
 * PIXMAP -- device dependent
 */

typedef struct _Pixmap {
    DrawableRec drawable;
    PrivateRec *devPrivates;
    int refcnt;
    int devKind;                /* This is the pitch of the pixmap, typically width*bpp/8. */
    DevUnion devPrivate;        /* When !NULL, devPrivate.ptr points to the raw pixel data. */
#ifdef COMPOSITE
    short screen_x;
    short screen_y;
#endif
    unsigned usage_hint;        /* see CREATE_PIXMAP_USAGE_* */

    PixmapPtr primary_pixmap;    /* pointer to primary copy of pixmap for pixmap sharing */
} PixmapRec;

typedef struct _PixmapDirtyUpdate {
    DrawablePtr src;            /* Root window / shared pixmap */
    PixmapPtr secondary_dst;    /* Shared / scanout pixmap */
    int x, y;
    DamagePtr damage;
    struct xorg_list ent;
    int dst_x, dst_y;
    Rotation rotation;
    PictTransform transform;
    struct pixman_f_transform f_transform, f_inverse;
} PixmapDirtyUpdateRec;

static inline void
PixmapBox(BoxPtr box, PixmapPtr pixmap)
{
    box->x1 = 0;
    box->x2 = pixmap->drawable.width;

    box->y1 = 0;
    box->y2 = pixmap->drawable.height;
}


static inline void
PixmapRegionInit(RegionPtr region, PixmapPtr pixmap)
{
    BoxRec box;

    PixmapBox(&box, pixmap);
    RegionInit(region, &box, 1);
}

#endif                          /* PIXMAPSTRUCT_H */
