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

#ifndef REGIONSTRUCT_H
#define REGIONSTRUCT_H

typedef struct pixman_region16 RegionRec, *RegionPtr;

#include "miscstruct.h"

/* Return values from RectIn() */

#define rgnOUT 0
#define rgnIN  1
#define rgnPART 2

#define NullRegion ((RegionPtr)0)

/*
 *   clip region
 */

typedef struct pixman_region16_data RegDataRec, *RegDataPtr;

extern _X_EXPORT BoxRec RegionEmptyBox;
extern _X_EXPORT RegDataRec RegionEmptyData;
extern _X_EXPORT RegDataRec RegionBrokenData;
static inline Bool
RegionNil(RegionPtr reg)
{
    return ((reg)->data && !(reg)->data->numRects);
}

/* not a region */

static inline Bool
RegionNar(RegionPtr reg)
{
    return ((reg)->data == &RegionBrokenData);
}

static inline int
RegionNumRects(RegionPtr reg)
{
    return ((reg)->data ? (reg)->data->numRects : 1);
}

static inline int
RegionSize(RegionPtr reg)
{
    return ((reg)->data ? (reg)->data->size : 0);
}

static inline BoxPtr
RegionRects(RegionPtr reg)
{
    return ((reg)->data ? (BoxPtr) ((reg)->data + 1) : &(reg)->extents);
}

static inline BoxPtr
RegionBoxptr(RegionPtr reg)
{
    return ((BoxPtr) ((reg)->data + 1));
}

static inline BoxPtr
RegionBox(RegionPtr reg, int i)
{
    return (&RegionBoxptr(reg)[i]);
}

static inline BoxPtr
RegionTop(RegionPtr reg)
{
    return RegionBox(reg, (reg)->data->numRects);
}

static inline BoxPtr
RegionEnd(RegionPtr reg)
{
    return RegionBox(reg, (reg)->data->numRects - 1);
}

static inline size_t
RegionSizeof(size_t n)
{
    if (n < ((INT_MAX - sizeof(RegDataRec)) / sizeof(BoxRec)))
        return (sizeof(RegDataRec) + ((n) * sizeof(BoxRec)));
    else
        return 0;
}

static inline void
RegionInit(RegionPtr _pReg, BoxPtr _rect, int _size)
{
    if ((_rect) != NULL) {
        (_pReg)->extents = *(_rect);
        (_pReg)->data = (RegDataPtr) NULL;
    }
    else {
        size_t rgnSize;
        (_pReg)->extents = RegionEmptyBox;
        if (((_size) > 1) && ((rgnSize = RegionSizeof(_size)) > 0) &&
            (((_pReg)->data = (RegDataPtr) malloc(rgnSize)) != NULL)) {
            (_pReg)->data->size = (_size);
            (_pReg)->data->numRects = 0;
        }
        else
            (_pReg)->data = &RegionEmptyData;
    }
}

static inline Bool
RegionInitBoxes(RegionPtr pReg, BoxPtr boxes, int nBoxes)
{
    return pixman_region_init_rects(pReg, boxes, nBoxes);
}

static inline void
RegionUninit(RegionPtr _pReg)
{
    if ((_pReg)->data && (_pReg)->data->size) {
        free((_pReg)->data);
        (_pReg)->data = NULL;
    }
}

static inline void
RegionReset(RegionPtr _pReg, BoxPtr _pBox)
{
    (_pReg)->extents = *(_pBox);
    RegionUninit(_pReg);
    (_pReg)->data = (RegDataPtr) NULL;
}

static inline Bool
RegionNotEmpty(RegionPtr _pReg)
{
    return !RegionNil(_pReg);
}

static inline Bool
RegionBroken(RegionPtr _pReg)
{
    return RegionNar(_pReg);
}

static inline void
RegionEmpty(RegionPtr _pReg)
{
    RegionUninit(_pReg);
    (_pReg)->extents.x2 = (_pReg)->extents.x1;
    (_pReg)->extents.y2 = (_pReg)->extents.y1;
    (_pReg)->data = &RegionEmptyData;
}

static inline BoxPtr
RegionExtents(RegionPtr _pReg)
{
    return (&(_pReg)->extents);
}

static inline void
RegionNull(RegionPtr _pReg)
{
    (_pReg)->extents = RegionEmptyBox;
    (_pReg)->data = &RegionEmptyData;
}

extern _X_EXPORT void InitRegions(void);

extern _X_EXPORT RegionPtr RegionCreate(BoxPtr /*rect */ ,
                                        int /*size */ );

extern _X_EXPORT void RegionDestroy(RegionPtr /*pReg */ );

extern _X_EXPORT RegionPtr RegionDuplicate(RegionPtr /* pOld */);

static inline Bool
RegionCopy(RegionPtr dst, RegionPtr src)
{
    return pixman_region_copy(dst, src);
}

static inline Bool
RegionIntersect(RegionPtr newReg,       /* destination Region */
                RegionPtr reg1, RegionPtr reg2  /* source regions     */
    )
{
    return pixman_region_intersect(newReg, reg1, reg2);
}

static inline Bool
RegionUnion(RegionPtr newReg,   /* destination Region */
            RegionPtr reg1, RegionPtr reg2      /* source regions     */
    )
{
    return pixman_region_union(newReg, reg1, reg2);
}

extern _X_EXPORT Bool RegionAppend(RegionPtr /*dstrgn */ ,
                                   RegionPtr /*rgn */ );

extern _X_EXPORT Bool RegionValidate(RegionPtr /*badreg */ ,
                                     Bool * /*pOverlap */ );

extern _X_EXPORT RegionPtr RegionFromRects(int /*nrects */ ,
                                           xRectanglePtr /*prect */ ,
                                           int /*ctype */ );

/*-
 *-----------------------------------------------------------------------
 * Subtract --
 *	Subtract regS from regM and leave the result in regD.
 *	S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *	TRUE if successful.
 *
 * Side Effects:
 *	regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static inline Bool
RegionSubtract(RegionPtr regD, RegionPtr regM, RegionPtr regS)
{
    return pixman_region_subtract(regD, regM, regS);
}

/*-
 *-----------------------------------------------------------------------
 * Inverse --
 *	Take a region and a box and return a region that is everything
 *	in the box but not in the region. The careful reader will note
 *	that this is the same as subtracting the region from the box...
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	newReg is overwritten.
 *
 *-----------------------------------------------------------------------
 */

static inline Bool
RegionInverse(RegionPtr newReg, /* Destination region */
              RegionPtr reg1,   /* Region to invert */
              BoxPtr invRect    /* Bounding box for inversion */
    )
{
    return pixman_region_inverse(newReg, reg1, invRect);
}

static inline int
RegionContainsRect(RegionPtr region, BoxPtr prect)
{
    return pixman_region_contains_rectangle(region, prect);
}

/* TranslateRegion(pReg, x, y)
   translates in place
*/

static inline void
RegionTranslate(RegionPtr pReg, int x, int y)
{
    pixman_region_translate(pReg, x, y);
}

extern _X_EXPORT Bool RegionBreak(RegionPtr /*pReg */ );

static inline Bool
RegionContainsPoint(RegionPtr pReg, int x, int y, BoxPtr box    /* "return" value */
    )
{
    return pixman_region_contains_point(pReg, x, y, box);
}

static inline Bool
RegionEqual(RegionPtr reg1, RegionPtr reg2)
{
    return pixman_region_equal(reg1, reg2);
}

extern _X_EXPORT Bool RegionRectAlloc(RegionPtr /*pRgn */ ,
                                      int       /*n */
    );

#ifdef DEBUG
extern _X_EXPORT Bool RegionIsValid(RegionPtr   /*prgn */
    );
#endif

extern _X_EXPORT void RegionPrint(RegionPtr /*pReg */ );

#define INCLUDE_LEGACY_REGION_DEFINES
#ifdef INCLUDE_LEGACY_REGION_DEFINES

#define REGION_NIL				RegionNil
#define REGION_NAR				RegionNar
#define REGION_NUM_RECTS			RegionNumRects
#define REGION_SIZE				RegionSize
#define REGION_RECTS				RegionRects
#define REGION_BOXPTR				RegionBoxptr
#define REGION_BOX				RegionBox
#define REGION_TOP				RegionTop
#define REGION_END				RegionEnd
#define REGION_SZOF				RegionSizeof
#define BITMAP_TO_REGION			BitmapToRegion
#define REGION_CREATE(pScreen, r, s)		RegionCreate(r,s)
#define REGION_COPY(pScreen, d, r)		RegionCopy(d, r)
#define REGION_DESTROY(pScreen, r)		RegionDestroy(r)
#define REGION_INTERSECT(pScreen, res, r1, r2)	RegionIntersect(res, r1, r2)
#define REGION_UNION(pScreen, res, r1, r2)	RegionUnion(res, r1, r2)
#define REGION_SUBTRACT(pScreen, res, r1, r2)	RegionSubtract(res, r1, r2)
#define REGION_INVERSE(pScreen, n, r, b)	RegionInverse(n, r, b)
#define REGION_TRANSLATE(pScreen, r, x, y)	RegionTranslate(r, x, y)
#define RECT_IN_REGION(pScreen, r, b) 		RegionContainsRect(r, b)
#define POINT_IN_REGION(pScreen, r, x, y, b) 	RegionContainsPoint(r, x, y, b)
#define REGION_EQUAL(pScreen, r1, r2)		RegionEqual(r1, r2)
#define REGION_APPEND(pScreen, d, r)		RegionAppend(d, r)
#define REGION_VALIDATE(pScreen, r, o)		RegionValidate(r, o)
#define RECTS_TO_REGION(pScreen, n, r, c)	RegionFromRects(n, r, c)
#define REGION_BREAK(pScreen, r)		RegionBreak(r)
#define REGION_INIT(pScreen, r, b, s)		RegionInit(r, b, s)
#define REGION_UNINIT(pScreen, r)		RegionUninit(r)
#define REGION_RESET(pScreen, r, b)		RegionReset(r, b)
#define REGION_NOTEMPTY(pScreen, r)		RegionNotEmpty(r)
#define REGION_BROKEN(pScreen, r)		RegionBroken(r)
#define REGION_EMPTY(pScreen, r)		RegionEmpty(r)
#define REGION_EXTENTS(pScreen, r)		RegionExtents(r)
#define REGION_NULL(pScreen, r)			RegionNull(r)

#endif                          /* INCLUDE_LEGACY_REGION_DEFINES */
#endif                          /* REGIONSTRUCT_H */
