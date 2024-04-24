/*
 * Copyright © 2008 Red Hat, Inc.
 * Partly based on code Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Red Hat DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL Red Hat
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
 * Author: Owen Taylor <otaylor@fishsoup.net>
 * Based on code by: Keith Packard
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "exa_priv.h"

#include "mipict.h"

#if DEBUG_GLYPH_CACHE
#define DBG_GLYPH_CACHE(a) ErrorF a
#else
#define DBG_GLYPH_CACHE(a)
#endif

/* Width of the pixmaps we use for the caches; this should be less than
 * max texture size of the driver; this may need to actually come from
 * the driver.
 */
#define CACHE_PICTURE_WIDTH 1024

/* Maximum number of glyphs we buffer on the stack before flushing
 * rendering to the mask or destination surface.
 */
#define GLYPH_BUFFER_SIZE 256

typedef struct {
    PicturePtr mask;
    ExaCompositeRectRec rects[GLYPH_BUFFER_SIZE];
    int count;
} ExaGlyphBuffer, *ExaGlyphBufferPtr;

typedef enum {
    ExaGlyphSuccess,            /* Glyph added to render buffer */
    ExaGlyphFail,               /* out of memory, etc */
    ExaGlyphNeedFlush,          /* would evict a glyph already in the buffer */
} ExaGlyphCacheResult;

void
exaGlyphsInit(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    int i = 0;

    memset(pExaScr->glyphCaches, 0, sizeof(pExaScr->glyphCaches));

    pExaScr->glyphCaches[i].format = PICT_a8;
    pExaScr->glyphCaches[i].glyphWidth = pExaScr->glyphCaches[i].glyphHeight =
        16;
    i++;
    pExaScr->glyphCaches[i].format = PICT_a8;
    pExaScr->glyphCaches[i].glyphWidth = pExaScr->glyphCaches[i].glyphHeight =
        32;
    i++;
    pExaScr->glyphCaches[i].format = PICT_a8r8g8b8;
    pExaScr->glyphCaches[i].glyphWidth = pExaScr->glyphCaches[i].glyphHeight =
        16;
    i++;
    pExaScr->glyphCaches[i].format = PICT_a8r8g8b8;
    pExaScr->glyphCaches[i].glyphWidth = pExaScr->glyphCaches[i].glyphHeight =
        32;
    i++;

    assert(i == EXA_NUM_GLYPH_CACHES);

    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        pExaScr->glyphCaches[i].columns =
            CACHE_PICTURE_WIDTH / pExaScr->glyphCaches[i].glyphWidth;
        pExaScr->glyphCaches[i].size = 256;
        pExaScr->glyphCaches[i].hashSize = 557;
    }
}

static void
exaUnrealizeGlyphCaches(ScreenPtr pScreen, unsigned int format)
{
    ExaScreenPriv(pScreen);
    int i;

    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        ExaGlyphCachePtr cache = &pExaScr->glyphCaches[i];

        if (cache->format != format)
            continue;

        if (cache->picture) {
            FreePicture((void *) cache->picture, (XID) 0);
            cache->picture = NULL;
        }

        free(cache->hashEntries);
        cache->hashEntries = NULL;

        free(cache->glyphs);
        cache->glyphs = NULL;
        cache->glyphCount = 0;
    }
}

#define NeedsComponent(f) (PICT_FORMAT_A(f) != 0 && PICT_FORMAT_RGB(f) != 0)

/* All caches for a single format share a single pixmap for glyph storage,
 * allowing mixing glyphs of different sizes without paying a penalty
 * for switching between mask pixmaps. (Note that for a size of font
 * right at the border between two sizes, we might be switching for almost
 * every glyph.)
 *
 * This function allocates the storage pixmap, and then fills in the
 * rest of the allocated structures for all caches with the given format.
 */
static Bool
exaRealizeGlyphCaches(ScreenPtr pScreen, unsigned int format)
{
    ExaScreenPriv(pScreen);

    int depth = PIXMAN_FORMAT_DEPTH(format);
    PictFormatPtr pPictFormat;
    PixmapPtr pPixmap;
    PicturePtr pPicture;
    CARD32 component_alpha;
    int height;
    int i;
    int error;

    pPictFormat = PictureMatchFormat(pScreen, depth, format);
    if (!pPictFormat)
        return FALSE;

    /* Compute the total vertical size needed for the format */

    height = 0;
    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        ExaGlyphCachePtr cache = &pExaScr->glyphCaches[i];
        int rows;

        if (cache->format != format)
            continue;

        cache->yOffset = height;

        rows = (cache->size + cache->columns - 1) / cache->columns;
        height += rows * cache->glyphHeight;
    }

    /* Now allocate the pixmap and picture */
    pPixmap = (*pScreen->CreatePixmap) (pScreen,
                                        CACHE_PICTURE_WIDTH, height, depth, 0);
    if (!pPixmap)
        return FALSE;

    component_alpha = NeedsComponent(pPictFormat->format);
    pPicture = CreatePicture(0, &pPixmap->drawable, pPictFormat,
                             CPComponentAlpha, &component_alpha, serverClient,
                             &error);

    (*pScreen->DestroyPixmap) (pPixmap);        /* picture holds a refcount */

    if (!pPicture)
        return FALSE;

    /* And store the picture in all the caches for the format */
    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        ExaGlyphCachePtr cache = &pExaScr->glyphCaches[i];
        int j;

        if (cache->format != format)
            continue;

        cache->picture = pPicture;
        cache->picture->refcnt++;
        cache->hashEntries = xallocarray(cache->hashSize, sizeof(int));
        cache->glyphs = xallocarray(cache->size, sizeof(ExaCachedGlyphRec));
        cache->glyphCount = 0;

        if (!cache->hashEntries || !cache->glyphs)
            goto bail;

        for (j = 0; j < cache->hashSize; j++)
            cache->hashEntries[j] = -1;

        cache->evictionPosition = rand() % cache->size;
    }

    /* Each cache references the picture individually */
    FreePicture((void *) pPicture, (XID) 0);
    return TRUE;

 bail:
    exaUnrealizeGlyphCaches(pScreen, format);
    return FALSE;
}

void
exaGlyphsFini(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    int i;

    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        ExaGlyphCachePtr cache = &pExaScr->glyphCaches[i];

        if (cache->picture)
            exaUnrealizeGlyphCaches(pScreen, cache->format);
    }
}

static int
exaGlyphCacheHashLookup(ExaGlyphCachePtr cache, GlyphPtr pGlyph)
{
    int slot;

    slot = (*(CARD32 *) pGlyph->sha1) % cache->hashSize;

    while (TRUE) {              /* hash table can never be full */
        int entryPos = cache->hashEntries[slot];

        if (entryPos == -1)
            return -1;

        if (memcmp
            (pGlyph->sha1, cache->glyphs[entryPos].sha1,
             sizeof(pGlyph->sha1)) == 0) {
            return entryPos;
        }

        slot--;
        if (slot < 0)
            slot = cache->hashSize - 1;
    }
}

static void
exaGlyphCacheHashInsert(ExaGlyphCachePtr cache, GlyphPtr pGlyph, int pos)
{
    int slot;

    memcpy(cache->glyphs[pos].sha1, pGlyph->sha1, sizeof(pGlyph->sha1));

    slot = (*(CARD32 *) pGlyph->sha1) % cache->hashSize;

    while (TRUE) {              /* hash table can never be full */
        if (cache->hashEntries[slot] == -1) {
            cache->hashEntries[slot] = pos;
            return;
        }

        slot--;
        if (slot < 0)
            slot = cache->hashSize - 1;
    }
}

static void
exaGlyphCacheHashRemove(ExaGlyphCachePtr cache, int pos)
{
    int slot;
    int emptiedSlot = -1;

    slot = (*(CARD32 *) cache->glyphs[pos].sha1) % cache->hashSize;

    while (TRUE) {              /* hash table can never be full */
        int entryPos = cache->hashEntries[slot];

        if (entryPos == -1)
            return;

        if (entryPos == pos) {
            cache->hashEntries[slot] = -1;
            emptiedSlot = slot;
        }
        else if (emptiedSlot != -1) {
            /* See if we can move this entry into the emptied slot, we can't
             * do that if if entry would have hashed between the current position
             * and the emptied slot. (taking wrapping into account). Bad positions
             * are:
             *
             * |   XXXXXXXXXX             |
             *     i         j
             *
             * |XXX                   XXXX|
             *     j                  i
             *
             * i - slot, j - emptiedSlot
             *
             * (Knuth 6.4R)
             */

            int entrySlot =
                (*(CARD32 *) cache->glyphs[entryPos].sha1) % cache->hashSize;

            if (!((entrySlot >= slot && entrySlot < emptiedSlot) ||
                  (emptiedSlot < slot &&
                   (entrySlot < emptiedSlot || entrySlot >= slot)))) {
                cache->hashEntries[emptiedSlot] = entryPos;
                cache->hashEntries[slot] = -1;
                emptiedSlot = slot;
            }
        }

        slot--;
        if (slot < 0)
            slot = cache->hashSize - 1;
    }
}

#define CACHE_X(pos) (((pos) % cache->columns) * cache->glyphWidth)
#define CACHE_Y(pos) (cache->yOffset + ((pos) / cache->columns) * cache->glyphHeight)

/* The most efficient thing to way to upload the glyph to the screen
 * is to use the UploadToScreen() driver hook; this allows us to
 * pipeline glyph uploads and to avoid creating gpu backed pixmaps for
 * glyphs that we'll never use again.
 *
 * If we can't do it with UploadToScreen (because the glyph has a gpu copy,
 * etc), we fall back to CompositePicture.
 *
 * We need to damage the cache pixmap manually in either case because the damage
 * layer unwrapped the picture screen before calling exaGlyphs.
 */
static void
exaGlyphCacheUploadGlyph(ScreenPtr pScreen,
                         ExaGlyphCachePtr cache, int x, int y, GlyphPtr pGlyph)
{
    ExaScreenPriv(pScreen);
    PicturePtr pGlyphPicture = GetGlyphPicture(pGlyph, pScreen);
    PixmapPtr pGlyphPixmap = (PixmapPtr) pGlyphPicture->pDrawable;

    ExaPixmapPriv(pGlyphPixmap);
    PixmapPtr pCachePixmap = (PixmapPtr) cache->picture->pDrawable;

    if (!pExaScr->info->UploadToScreen || pExaScr->swappedOut ||
        pExaPixmap->accel_blocked)
        goto composite;

    /* If the glyph pixmap is already uploaded, no point in doing
     * things this way */
    if (exaPixmapHasGpuCopy(pGlyphPixmap))
        goto composite;

    /* UploadToScreen only works if bpp match */
    if (pGlyphPixmap->drawable.bitsPerPixel !=
        pCachePixmap->drawable.bitsPerPixel)
        goto composite;

    if (pExaScr->do_migration) {
        ExaMigrationRec pixmaps[1];

        /* cache pixmap must have a gpu copy. */
        pixmaps[0].as_dst = TRUE;
        pixmaps[0].as_src = FALSE;
        pixmaps[0].pPix = pCachePixmap;
        pixmaps[0].pReg = NULL;
        exaDoMigration(pixmaps, 1, TRUE);
    }

    if (!exaPixmapHasGpuCopy(pCachePixmap))
        goto composite;

    /* x,y are in pixmap coordinates, no need for cache{X,Y}off */
    if (pExaScr->info->UploadToScreen(pCachePixmap,
                                      x,
                                      y,
                                      pGlyph->info.width,
                                      pGlyph->info.height,
                                      (char *) pExaPixmap->sys_ptr,
                                      pExaPixmap->sys_pitch))
        goto damage;

 composite:
    CompositePicture(PictOpSrc,
                     pGlyphPicture,
                     None,
                     cache->picture,
                     0, 0, 0, 0, x, y, pGlyph->info.width, pGlyph->info.height);

 damage:
    /* The cache pixmap isn't a window, so no need to offset coordinates. */
    exaPixmapDirty(pCachePixmap,
                   x, y, x + cache->glyphWidth, y + cache->glyphHeight);
}

static ExaGlyphCacheResult
exaGlyphCacheBufferGlyph(ScreenPtr pScreen,
                         ExaGlyphCachePtr cache,
                         ExaGlyphBufferPtr buffer,
                         GlyphPtr pGlyph,
                         PicturePtr pSrc,
                         PicturePtr pDst,
                         INT16 xSrc,
                         INT16 ySrc,
                         INT16 xMask, INT16 yMask, INT16 xDst, INT16 yDst)
{
    ExaCompositeRectPtr rect;
    int pos;
    int x, y;

    if (buffer->mask && buffer->mask != cache->picture)
        return ExaGlyphNeedFlush;

    if (!cache->picture) {
        if (!exaRealizeGlyphCaches(pScreen, cache->format))
            return ExaGlyphFail;
    }

    DBG_GLYPH_CACHE(("(%d,%d,%s): buffering glyph %lx\n",
                     cache->glyphWidth, cache->glyphHeight,
                     cache->format == PICT_a8 ? "A" : "ARGB",
                     (long) *(CARD32 *) pGlyph->sha1));

    pos = exaGlyphCacheHashLookup(cache, pGlyph);
    if (pos != -1) {
        DBG_GLYPH_CACHE(("  found existing glyph at %d\n", pos));
        x = CACHE_X(pos);
        y = CACHE_Y(pos);
    }
    else {
        if (cache->glyphCount < cache->size) {
            /* Space remaining; we fill from the start */
            pos = cache->glyphCount;
            x = CACHE_X(pos);
            y = CACHE_Y(pos);
            cache->glyphCount++;
            DBG_GLYPH_CACHE(("  storing glyph in free space at %d\n", pos));

            exaGlyphCacheHashInsert(cache, pGlyph, pos);

        }
        else {
            /* Need to evict an entry. We have to see if any glyphs
             * already in the output buffer were at this position in
             * the cache
             */
            pos = cache->evictionPosition;
            x = CACHE_X(pos);
            y = CACHE_Y(pos);
            DBG_GLYPH_CACHE(("  evicting glyph at %d\n", pos));
            if (buffer->count) {
                int i;

                for (i = 0; i < buffer->count; i++) {
                    if (pSrc ?
                        (buffer->rects[i].xMask == x &&
                         buffer->rects[i].yMask ==
                         y) : (buffer->rects[i].xSrc == x &&
                               buffer->rects[i].ySrc == y)) {
                        DBG_GLYPH_CACHE(("  must flush buffer\n"));
                        return ExaGlyphNeedFlush;
                    }
                }
            }

            /* OK, we're all set, swap in the new glyph */
            exaGlyphCacheHashRemove(cache, pos);
            exaGlyphCacheHashInsert(cache, pGlyph, pos);

            /* And pick a new eviction position */
            cache->evictionPosition = rand() % cache->size;
        }

        exaGlyphCacheUploadGlyph(pScreen, cache, x, y, pGlyph);
    }

    buffer->mask = cache->picture;

    rect = &buffer->rects[buffer->count];

    if (pSrc) {
        rect->xSrc = xSrc;
        rect->ySrc = ySrc;
        rect->xMask = x;
        rect->yMask = y;
    }
    else {
        rect->xSrc = x;
        rect->ySrc = y;
        rect->xMask = 0;
        rect->yMask = 0;
    }

    rect->pDst = pDst;
    rect->xDst = xDst;
    rect->yDst = yDst;
    rect->width = pGlyph->info.width;
    rect->height = pGlyph->info.height;

    buffer->count++;

    return ExaGlyphSuccess;
}

#undef CACHE_X
#undef CACHE_Y

static ExaGlyphCacheResult
exaBufferGlyph(ScreenPtr pScreen,
               ExaGlyphBufferPtr buffer,
               GlyphPtr pGlyph,
               PicturePtr pSrc,
               PicturePtr pDst,
               INT16 xSrc,
               INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst, INT16 yDst)
{
    ExaScreenPriv(pScreen);
    unsigned int format = (GetGlyphPicture(pGlyph, pScreen))->format;
    int width = pGlyph->info.width;
    int height = pGlyph->info.height;
    ExaCompositeRectPtr rect;
    PicturePtr mask;
    int i;

    if (buffer->count == GLYPH_BUFFER_SIZE)
        return ExaGlyphNeedFlush;

    if (PICT_FORMAT_BPP(format) == 1)
        format = PICT_a8;

    for (i = 0; i < EXA_NUM_GLYPH_CACHES; i++) {
        ExaGlyphCachePtr cache = &pExaScr->glyphCaches[i];

        if (format == cache->format &&
            width <= cache->glyphWidth && height <= cache->glyphHeight) {
            ExaGlyphCacheResult result = exaGlyphCacheBufferGlyph(pScreen,
                                                                  &pExaScr->
                                                                  glyphCaches
                                                                  [i],
                                                                  buffer,
                                                                  pGlyph,
                                                                  pSrc,
                                                                  pDst,
                                                                  xSrc, ySrc,
                                                                  xMask, yMask,
                                                                  xDst, yDst);

            switch (result) {
            case ExaGlyphFail:
                break;
            case ExaGlyphSuccess:
            case ExaGlyphNeedFlush:
                return result;
            }
        }
    }

    /* Couldn't find the glyph in the cache, use the glyph picture directly */

    mask = GetGlyphPicture(pGlyph, pScreen);
    if (buffer->mask && buffer->mask != mask)
        return ExaGlyphNeedFlush;

    buffer->mask = mask;

    rect = &buffer->rects[buffer->count];
    rect->xSrc = xSrc;
    rect->ySrc = ySrc;
    rect->xMask = xMask;
    rect->yMask = yMask;
    rect->xDst = xDst;
    rect->yDst = yDst;
    rect->width = width;
    rect->height = height;

    buffer->count++;

    return ExaGlyphSuccess;
}

static void
exaGlyphsToMask(PicturePtr pMask, ExaGlyphBufferPtr buffer)
{
    exaCompositeRects(PictOpAdd, buffer->mask, NULL, pMask,
                      buffer->count, buffer->rects);

    buffer->count = 0;
    buffer->mask = NULL;
}

static void
exaGlyphsToDst(CARD8 op, PicturePtr pSrc, PicturePtr pDst, ExaGlyphBufferPtr buffer)
{
    exaCompositeRects(op, pSrc, buffer->mask, pDst, buffer->count,
                      buffer->rects);

    buffer->count = 0;
    buffer->mask = NULL;
}

/* Cut and paste from render/glyph.c - probably should export it instead */
static void
GlyphExtents(int nlist, GlyphListPtr list, GlyphPtr * glyphs, BoxPtr extents)
{
    int x1, x2, y1, y2;
    int n;
    GlyphPtr glyph;
    int x, y;

    x = 0;
    y = 0;
    extents->x1 = MAXSHORT;
    extents->x2 = MINSHORT;
    extents->y1 = MAXSHORT;
    extents->y2 = MINSHORT;
    while (nlist--) {
        x += list->xOff;
        y += list->yOff;
        n = list->len;
        list++;
        while (n--) {
            glyph = *glyphs++;
            x1 = x - glyph->info.x;
            if (x1 < MINSHORT)
                x1 = MINSHORT;
            y1 = y - glyph->info.y;
            if (y1 < MINSHORT)
                y1 = MINSHORT;
            x2 = x1 + glyph->info.width;
            if (x2 > MAXSHORT)
                x2 = MAXSHORT;
            y2 = y1 + glyph->info.height;
            if (y2 > MAXSHORT)
                y2 = MAXSHORT;
            if (x1 < extents->x1)
                extents->x1 = x1;
            if (x2 > extents->x2)
                extents->x2 = x2;
            if (y1 < extents->y1)
                extents->y1 = y1;
            if (y2 > extents->y2)
                extents->y2 = y2;
            x += glyph->info.xOff;
            y += glyph->info.yOff;
        }
    }
}

void
exaGlyphs(CARD8 op,
          PicturePtr pSrc,
          PicturePtr pDst,
          PictFormatPtr maskFormat,
          INT16 xSrc,
          INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
    PixmapPtr pMaskPixmap = 0;
    PicturePtr pMask = NULL;
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    int width = 0, height = 0;
    int x, y;
    int first_xOff = list->xOff, first_yOff = list->yOff;
    int n;
    GlyphPtr glyph;
    int error;
    BoxRec extents = { 0, 0, 0, 0 };
    CARD32 component_alpha;
    ExaGlyphBuffer buffer;

    if (maskFormat) {
        ExaScreenPriv(pScreen);
        GCPtr pGC;
        xRectangle rect;

        GlyphExtents(nlist, list, glyphs, &extents);

        if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
            return;
        width = extents.x2 - extents.x1;
        height = extents.y2 - extents.y1;

        if (maskFormat->depth == 1) {
            PictFormatPtr a8Format = PictureMatchFormat(pScreen, 8, PICT_a8);

            if (a8Format)
                maskFormat = a8Format;
        }

        pMaskPixmap = (*pScreen->CreatePixmap) (pScreen, width, height,
                                                maskFormat->depth,
                                                CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pMaskPixmap)
            return;
        component_alpha = NeedsComponent(maskFormat->format);
        pMask = CreatePicture(0, &pMaskPixmap->drawable,
                              maskFormat, CPComponentAlpha, &component_alpha,
                              serverClient, &error);
        if (!pMask ||
            (!component_alpha && pExaScr->info->CheckComposite &&
             !(*pExaScr->info->CheckComposite) (PictOpAdd, pSrc, NULL, pMask)))
        {
            PictFormatPtr argbFormat;

            (*pScreen->DestroyPixmap) (pMaskPixmap);

            if (!pMask)
                return;

            /* The driver can't seem to composite to a8, let's try argb (but
             * without component-alpha) */
            FreePicture((void *) pMask, (XID) 0);

            argbFormat = PictureMatchFormat(pScreen, 32, PICT_a8r8g8b8);

            if (argbFormat)
                maskFormat = argbFormat;

            pMaskPixmap = (*pScreen->CreatePixmap) (pScreen, width, height,
                                                    maskFormat->depth,
                                                    CREATE_PIXMAP_USAGE_SCRATCH);
            if (!pMaskPixmap)
                return;

            pMask = CreatePicture(0, &pMaskPixmap->drawable, maskFormat, 0, 0,
                                  serverClient, &error);
            if (!pMask) {
                (*pScreen->DestroyPixmap) (pMaskPixmap);
                return;
            }
        }
        pGC = GetScratchGC(pMaskPixmap->drawable.depth, pScreen);
        ValidateGC(&pMaskPixmap->drawable, pGC);
        rect.x = 0;
        rect.y = 0;
        rect.width = width;
        rect.height = height;
        (*pGC->ops->PolyFillRect) (&pMaskPixmap->drawable, pGC, 1, &rect);
        FreeScratchGC(pGC);
        x = -extents.x1;
        y = -extents.y1;
    }
    else {
        x = 0;
        y = 0;
    }
    buffer.count = 0;
    buffer.mask = NULL;
    while (nlist--) {
        x += list->xOff;
        y += list->yOff;
        n = list->len;
        while (n--) {
            glyph = *glyphs++;

            if (glyph->info.width > 0 && glyph->info.height > 0) {
                /* pGlyph->info.{x,y} compensate for empty space in the glyph. */
                if (maskFormat) {
                    if (exaBufferGlyph(pScreen, &buffer, glyph, NULL, pMask,
                                       0, 0, 0, 0, x - glyph->info.x,
                                       y - glyph->info.y) ==
                        ExaGlyphNeedFlush) {
                        exaGlyphsToMask(pMask, &buffer);
                        exaBufferGlyph(pScreen, &buffer, glyph, NULL, pMask,
                                       0, 0, 0, 0, x - glyph->info.x,
                                       y - glyph->info.y);
                    }
                }
                else {
                    if (exaBufferGlyph(pScreen, &buffer, glyph, pSrc, pDst,
                                       xSrc + (x - glyph->info.x) - first_xOff,
                                       ySrc + (y - glyph->info.y) - first_yOff,
                                       0, 0, x - glyph->info.x,
                                       y - glyph->info.y)
                        == ExaGlyphNeedFlush) {
                        exaGlyphsToDst(op, pSrc, pDst, &buffer);
                        exaBufferGlyph(pScreen, &buffer, glyph, pSrc, pDst,
                                       xSrc + (x - glyph->info.x) - first_xOff,
                                       ySrc + (y - glyph->info.y) - first_yOff,
                                       0, 0, x - glyph->info.x,
                                       y - glyph->info.y);
                    }
                }
            }

            x += glyph->info.xOff;
            y += glyph->info.yOff;
        }
        list++;
    }

    if (buffer.count) {
        if (maskFormat)
            exaGlyphsToMask(pMask, &buffer);
        else
            exaGlyphsToDst(op, pSrc, pDst, &buffer);
    }

    if (maskFormat) {
        x = extents.x1;
        y = extents.y1;
        CompositePicture(op,
                         pSrc,
                         pMask,
                         pDst,
                         xSrc + x - first_xOff,
                         ySrc + y - first_yOff, 0, 0, x, y, width, height);
        FreePicture((void *) pMask, (XID) 0);
        (*pScreen->DestroyPixmap) (pMaskPixmap);
    }
}
