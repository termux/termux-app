/*
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _GLYPHSTR_H_
#define _GLYPHSTR_H_

#include <X11/extensions/renderproto.h>
#include "picture.h"
#include "screenint.h"
#include "regionstr.h"
#include "miscstruct.h"
#include "privates.h"

#define GlyphFormat1	0
#define GlyphFormat4	1
#define GlyphFormat8	2
#define GlyphFormat16	3
#define GlyphFormat32	4
#define GlyphFormatNum	5

typedef struct _Glyph {
    CARD32 refcnt;
    PrivateRec *devPrivates;
    unsigned char sha1[20];
    CARD32 size;                /* info + bitmap */
    xGlyphInfo info;
    /* per-screen pixmaps follow */
} GlyphRec, *GlyphPtr;

#define GlyphPicture(glyph) ((PicturePtr *) ((glyph) + 1))

typedef struct _GlyphRef {
    CARD32 signature;
    GlyphPtr glyph;
} GlyphRefRec, *GlyphRefPtr;

#define DeletedGlyph	((GlyphPtr) 1)

typedef struct _GlyphHashSet {
    CARD32 entries;
    CARD32 size;
    CARD32 rehash;
} GlyphHashSetRec, *GlyphHashSetPtr;

typedef struct _GlyphHash {
    GlyphRefPtr table;
    GlyphHashSetPtr hashSet;
    CARD32 tableEntries;
} GlyphHashRec, *GlyphHashPtr;

typedef struct _GlyphSet {
    CARD32 refcnt;
    int fdepth;
    PictFormatPtr format;
    GlyphHashRec hash;
    PrivateRec *devPrivates;
} GlyphSetRec, *GlyphSetPtr;

#define GlyphSetGetPrivate(pGlyphSet,k)					\
    dixLookupPrivate(&(pGlyphSet)->devPrivates, k)

#define GlyphSetSetPrivate(pGlyphSet,k,ptr)				\
    dixSetPrivate(&(pGlyphSet)->devPrivates, k, ptr)

typedef struct _GlyphList {
    INT16 xOff;
    INT16 yOff;
    CARD8 len;
    PictFormatPtr format;
} GlyphListRec, *GlyphListPtr;

extern void
 GlyphUninit(ScreenPtr pScreen);

extern GlyphPtr FindGlyphByHash(unsigned char sha1[20], int format);

extern int
HashGlyph(xGlyphInfo * gi,
          CARD8 *bits, unsigned long size, unsigned char sha1[20]);

extern void
 AddGlyph(GlyphSetPtr glyphSet, GlyphPtr glyph, Glyph id);

extern Bool
 DeleteGlyph(GlyphSetPtr glyphSet, Glyph id);

extern GlyphPtr FindGlyph(GlyphSetPtr glyphSet, Glyph id);

extern GlyphPtr AllocateGlyph(xGlyphInfo * gi, int format);

extern Bool
 ResizeGlyphSet(GlyphSetPtr glyphSet, CARD32 change);

extern GlyphSetPtr AllocateGlyphSet(int fdepth, PictFormatPtr format);

extern int
 FreeGlyphSet(void *value, XID gid);

#define GLYPH_HAS_GLYPH_PICTURE_ACCESSOR 1 /* used for api compat */
extern _X_EXPORT PicturePtr
 GetGlyphPicture(GlyphPtr glyph, ScreenPtr pScreen);
extern _X_EXPORT void
 SetGlyphPicture(GlyphPtr glyph, ScreenPtr pScreen, PicturePtr picture);

#endif                          /* _GLYPHSTR_H_ */
