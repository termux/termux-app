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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xsha1.h"

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "picturestr.h"
#include "glyphstr.h"
#include "mipict.h"

/*
 * From Knuth -- a good choice for hash/rehash values is p, p-2 where
 * p and p-2 are both prime.  These tables are sized to have an extra 10%
 * free to avoid exponential performance degradation as the hash table fills
 */
static GlyphHashSetRec glyphHashSets[] = {
    {32, 43, 41},
    {64, 73, 71},
    {128, 151, 149},
    {256, 283, 281},
    {512, 571, 569},
    {1024, 1153, 1151},
    {2048, 2269, 2267},
    {4096, 4519, 4517},
    {8192, 9013, 9011},
    {16384, 18043, 18041},
    {32768, 36109, 36107},
    {65536, 72091, 72089},
    {131072, 144409, 144407},
    {262144, 288361, 288359},
    {524288, 576883, 576881},
    {1048576, 1153459, 1153457},
    {2097152, 2307163, 2307161},
    {4194304, 4613893, 4613891},
    {8388608, 9227641, 9227639},
    {16777216, 18455029, 18455027},
    {33554432, 36911011, 36911009},
    {67108864, 73819861, 73819859},
    {134217728, 147639589, 147639587},
    {268435456, 295279081, 295279079},
    {536870912, 590559793, 590559791}
};

#define NGLYPHHASHSETS	ARRAY_SIZE(glyphHashSets)

static GlyphHashRec globalGlyphs[GlyphFormatNum];

void
GlyphUninit(ScreenPtr pScreen)
{
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    GlyphPtr glyph;
    int fdepth, i;

    for (fdepth = 0; fdepth < GlyphFormatNum; fdepth++) {
        if (!globalGlyphs[fdepth].hashSet)
            continue;

        for (i = 0; i < globalGlyphs[fdepth].hashSet->size; i++) {
            glyph = globalGlyphs[fdepth].table[i].glyph;
            if (glyph && glyph != DeletedGlyph) {
                if (GetGlyphPicture(glyph, pScreen)) {
                    FreePicture((void *) GetGlyphPicture(glyph, pScreen), 0);
                    SetGlyphPicture(glyph, pScreen, NULL);
                }
                (*ps->UnrealizeGlyph) (pScreen, glyph);
            }
        }
    }
}

static GlyphHashSetPtr
FindGlyphHashSet(CARD32 filled)
{
    int i;

    for (i = 0; i < NGLYPHHASHSETS; i++)
        if (glyphHashSets[i].entries >= filled)
            return &glyphHashSets[i];
    return 0;
}

static GlyphRefPtr
FindGlyphRef(GlyphHashPtr hash,
             CARD32 signature, Bool match, unsigned char sha1[20])
{
    CARD32 elt, step, s;
    GlyphPtr glyph;
    GlyphRefPtr table, gr, del;
    CARD32 tableSize = hash->hashSet->size;

    table = hash->table;
    elt = signature % tableSize;
    step = 0;
    del = 0;
    for (;;) {
        gr = &table[elt];
        s = gr->signature;
        glyph = gr->glyph;
        if (!glyph) {
            if (del)
                gr = del;
            break;
        }
        if (glyph == DeletedGlyph) {
            if (!del)
                del = gr;
            else if (gr == del)
                break;
        }
        else if (s == signature &&
                 (!match || memcmp(glyph->sha1, sha1, 20) == 0)) {
            break;
        }
        if (!step) {
            step = signature % hash->hashSet->rehash;
            if (!step)
                step = 1;
        }
        elt += step;
        if (elt >= tableSize)
            elt -= tableSize;
    }
    return gr;
}

int
HashGlyph(xGlyphInfo * gi,
          CARD8 *bits, unsigned long size, unsigned char sha1[20])
{
    void *ctx = x_sha1_init();
    int success;

    if (!ctx)
        return BadAlloc;

    success = x_sha1_update(ctx, gi, sizeof(xGlyphInfo));
    if (!success)
        return BadAlloc;
    success = x_sha1_update(ctx, bits, size);
    if (!success)
        return BadAlloc;
    success = x_sha1_final(ctx, sha1);
    if (!success)
        return BadAlloc;
    return Success;
}

GlyphPtr
FindGlyphByHash(unsigned char sha1[20], int format)
{
    GlyphRefPtr gr;
    CARD32 signature = *(CARD32 *) sha1;

    if (!globalGlyphs[format].hashSet)
        return NULL;

    gr = FindGlyphRef(&globalGlyphs[format], signature, TRUE, sha1);

    if (gr->glyph && gr->glyph != DeletedGlyph)
        return gr->glyph;
    else
        return NULL;
}

#ifdef CHECK_DUPLICATES
void
DuplicateRef(GlyphPtr glyph, char *where)
{
    ErrorF("Duplicate Glyph 0x%x from %s\n", glyph, where);
}

void
CheckDuplicates(GlyphHashPtr hash, char *where)
{
    GlyphPtr g;
    int i, j;

    for (i = 0; i < hash->hashSet->size; i++) {
        g = hash->table[i].glyph;
        if (!g || g == DeletedGlyph)
            continue;
        for (j = i + 1; j < hash->hashSet->size; j++)
            if (hash->table[j].glyph == g)
                DuplicateRef(g, where);
    }
}
#else
#define CheckDuplicates(a,b)
#define DuplicateRef(a,b)
#endif

static void
FreeGlyphPicture(GlyphPtr glyph)
{
    PictureScreenPtr ps;
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];

        if (GetGlyphPicture(glyph, pScreen))
            FreePicture((void *) GetGlyphPicture(glyph, pScreen), 0);

        ps = GetPictureScreenIfSet(pScreen);
        if (ps)
            (*ps->UnrealizeGlyph) (pScreen, glyph);
    }
}

static void
FreeGlyph(GlyphPtr glyph, int format)
{
    CheckDuplicates(&globalGlyphs[format], "FreeGlyph");
    if (--glyph->refcnt == 0) {
        GlyphRefPtr gr;
        int i;
        int first;
        CARD32 signature;

        first = -1;
        for (i = 0; i < globalGlyphs[format].hashSet->size; i++)
            if (globalGlyphs[format].table[i].glyph == glyph) {
                if (first != -1)
                    DuplicateRef(glyph, "FreeGlyph check");
                first = i;
            }

        signature = *(CARD32 *) glyph->sha1;
        gr = FindGlyphRef(&globalGlyphs[format], signature, TRUE, glyph->sha1);
        if (gr - globalGlyphs[format].table != first)
            DuplicateRef(glyph, "Found wrong one");
        if (gr->glyph && gr->glyph != DeletedGlyph) {
            gr->glyph = DeletedGlyph;
            gr->signature = 0;
            globalGlyphs[format].tableEntries--;
        }

        FreeGlyphPicture(glyph);
        dixFreeObjectWithPrivates(glyph, PRIVATE_GLYPH);
    }
}

void
AddGlyph(GlyphSetPtr glyphSet, GlyphPtr glyph, Glyph id)
{
    GlyphRefPtr gr;
    CARD32 signature;

    CheckDuplicates(&globalGlyphs[glyphSet->fdepth], "AddGlyph top global");
    /* Locate existing matching glyph */
    signature = *(CARD32 *) glyph->sha1;
    gr = FindGlyphRef(&globalGlyphs[glyphSet->fdepth], signature,
                      TRUE, glyph->sha1);
    if (gr->glyph && gr->glyph != DeletedGlyph && gr->glyph != glyph) {
        FreeGlyphPicture(glyph);
        dixFreeObjectWithPrivates(glyph, PRIVATE_GLYPH);
        glyph = gr->glyph;
    }
    else if (gr->glyph != glyph) {
        gr->glyph = glyph;
        gr->signature = signature;
        globalGlyphs[glyphSet->fdepth].tableEntries++;
    }

    /* Insert/replace glyphset value */
    gr = FindGlyphRef(&glyphSet->hash, id, FALSE, 0);
    ++glyph->refcnt;
    if (gr->glyph && gr->glyph != DeletedGlyph)
        FreeGlyph(gr->glyph, glyphSet->fdepth);
    else
        glyphSet->hash.tableEntries++;
    gr->glyph = glyph;
    gr->signature = id;
    CheckDuplicates(&globalGlyphs[glyphSet->fdepth], "AddGlyph bottom");
}

Bool
DeleteGlyph(GlyphSetPtr glyphSet, Glyph id)
{
    GlyphRefPtr gr;
    GlyphPtr glyph;

    gr = FindGlyphRef(&glyphSet->hash, id, FALSE, 0);
    glyph = gr->glyph;
    if (glyph && glyph != DeletedGlyph) {
        gr->glyph = DeletedGlyph;
        glyphSet->hash.tableEntries--;
        FreeGlyph(glyph, glyphSet->fdepth);
        return TRUE;
    }
    return FALSE;
}

GlyphPtr
FindGlyph(GlyphSetPtr glyphSet, Glyph id)
{
    GlyphPtr glyph;

    glyph = FindGlyphRef(&glyphSet->hash, id, FALSE, 0)->glyph;
    if (glyph == DeletedGlyph)
        glyph = 0;
    return glyph;
}

GlyphPtr
AllocateGlyph(xGlyphInfo * gi, int fdepth)
{
    PictureScreenPtr ps;
    int size;
    GlyphPtr glyph;
    int i;
    int head_size;

    head_size = sizeof(GlyphRec) + screenInfo.numScreens * sizeof(PicturePtr);
    size = (head_size + dixPrivatesSize(PRIVATE_GLYPH));
    glyph = (GlyphPtr) malloc(size);
    if (!glyph)
        return 0;
    glyph->refcnt = 0;
    glyph->size = size + sizeof(xGlyphInfo);
    glyph->info = *gi;
    dixInitPrivates(glyph, (char *) glyph + head_size, PRIVATE_GLYPH);

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        SetGlyphPicture(glyph, pScreen, NULL);
        ps = GetPictureScreenIfSet(pScreen);

        if (ps) {
            if (!(*ps->RealizeGlyph) (pScreen, glyph))
                goto bail;
        }
    }

    return glyph;

 bail:
    while (i--) {
        ps = GetPictureScreenIfSet(screenInfo.screens[i]);
        if (ps)
            (*ps->UnrealizeGlyph) (screenInfo.screens[i], glyph);
    }

    dixFreeObjectWithPrivates(glyph, PRIVATE_GLYPH);
    return 0;
}

static Bool
AllocateGlyphHash(GlyphHashPtr hash, GlyphHashSetPtr hashSet)
{
    hash->table = calloc(hashSet->size, sizeof(GlyphRefRec));
    if (!hash->table)
        return FALSE;
    hash->hashSet = hashSet;
    hash->tableEntries = 0;
    return TRUE;
}

static Bool
ResizeGlyphHash(GlyphHashPtr hash, CARD32 change, Bool global)
{
    CARD32 tableEntries;
    GlyphHashSetPtr hashSet;
    GlyphHashRec newHash;
    GlyphRefPtr gr;
    GlyphPtr glyph;
    int i;
    int oldSize;
    CARD32 s;

    tableEntries = hash->tableEntries + change;
    hashSet = FindGlyphHashSet(tableEntries);
    if (hashSet == hash->hashSet)
        return TRUE;
    if (global)
        CheckDuplicates(hash, "ResizeGlyphHash top");
    if (!AllocateGlyphHash(&newHash, hashSet))
        return FALSE;
    if (hash->table) {
        oldSize = hash->hashSet->size;
        for (i = 0; i < oldSize; i++) {
            glyph = hash->table[i].glyph;
            if (glyph && glyph != DeletedGlyph) {
                s = hash->table[i].signature;
                gr = FindGlyphRef(&newHash, s, global, glyph->sha1);

                gr->signature = s;
                gr->glyph = glyph;
                ++newHash.tableEntries;
            }
        }
        free(hash->table);
    }
    *hash = newHash;
    if (global)
        CheckDuplicates(hash, "ResizeGlyphHash bottom");
    return TRUE;
}

Bool
ResizeGlyphSet(GlyphSetPtr glyphSet, CARD32 change)
{
    return (ResizeGlyphHash(&glyphSet->hash, change, FALSE) &&
            ResizeGlyphHash(&globalGlyphs[glyphSet->fdepth], change, TRUE));
}

GlyphSetPtr
AllocateGlyphSet(int fdepth, PictFormatPtr format)
{
    GlyphSetPtr glyphSet;

    if (!globalGlyphs[fdepth].hashSet) {
        if (!AllocateGlyphHash(&globalGlyphs[fdepth], &glyphHashSets[0]))
            return FALSE;
    }

    glyphSet = dixAllocateObjectWithPrivates(GlyphSetRec, PRIVATE_GLYPHSET);
    if (!glyphSet)
        return FALSE;

    if (!AllocateGlyphHash(&glyphSet->hash, &glyphHashSets[0])) {
        free(glyphSet);
        return FALSE;
    }
    glyphSet->refcnt = 1;
    glyphSet->fdepth = fdepth;
    glyphSet->format = format;
    return glyphSet;
}

int
FreeGlyphSet(void *value, XID gid)
{
    GlyphSetPtr glyphSet = (GlyphSetPtr) value;

    if (--glyphSet->refcnt == 0) {
        CARD32 i, tableSize = glyphSet->hash.hashSet->size;
        GlyphRefPtr table = glyphSet->hash.table;
        GlyphPtr glyph;

        for (i = 0; i < tableSize; i++) {
            glyph = table[i].glyph;
            if (glyph && glyph != DeletedGlyph)
                FreeGlyph(glyph, glyphSet->fdepth);
        }
        if (!globalGlyphs[glyphSet->fdepth].tableEntries) {
            free(globalGlyphs[glyphSet->fdepth].table);
            globalGlyphs[glyphSet->fdepth].table = 0;
            globalGlyphs[glyphSet->fdepth].hashSet = 0;
        }
        else
            ResizeGlyphHash(&globalGlyphs[glyphSet->fdepth], 0, TRUE);
        free(table);
        dixFreeObjectWithPrivates(glyphSet, PRIVATE_GLYPHSET);
    }
    return Success;
}

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

#define NeedsComponent(f) (PICT_FORMAT_A(f) != 0 && PICT_FORMAT_RGB(f) != 0)

void
CompositeGlyphs(CARD8 op,
                PicturePtr pSrc,
                PicturePtr pDst,
                PictFormatPtr maskFormat,
                INT16 xSrc,
                INT16 ySrc, int nlist, GlyphListPtr lists, GlyphPtr * glyphs)
{
    PictureScreenPtr ps = GetPictureScreen(pDst->pDrawable->pScreen);

    ValidatePicture(pSrc);
    ValidatePicture(pDst);
    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, lists,
                   glyphs);
}

Bool
miRealizeGlyph(ScreenPtr pScreen, GlyphPtr glyph)
{
    return TRUE;
}

void
miUnrealizeGlyph(ScreenPtr pScreen, GlyphPtr glyph)
{
}

void
miGlyphs(CARD8 op,
         PicturePtr pSrc,
         PicturePtr pDst,
         PictFormatPtr maskFormat,
         INT16 xSrc,
         INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
    PicturePtr pPicture;
    PixmapPtr pMaskPixmap = 0;
    PicturePtr pMask;
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    int width = 0, height = 0;
    int x, y;
    int xDst = list->xOff, yDst = list->yOff;
    int n;
    GlyphPtr glyph;
    int error;
    BoxRec extents = { 0, 0, 0, 0 };
    CARD32 component_alpha;

    if (maskFormat) {
        GCPtr pGC;
        xRectangle rect;

        GlyphExtents(nlist, list, glyphs, &extents);

        if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
            return;
        width = extents.x2 - extents.x1;
        height = extents.y2 - extents.y1;
        pMaskPixmap = (*pScreen->CreatePixmap) (pScreen, width, height,
                                                maskFormat->depth,
                                                CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pMaskPixmap)
            return;
        component_alpha = NeedsComponent(maskFormat->format);
        pMask = CreatePicture(0, &pMaskPixmap->drawable,
                              maskFormat, CPComponentAlpha, &component_alpha,
                              serverClient, &error);
        if (!pMask) {
            (*pScreen->DestroyPixmap) (pMaskPixmap);
            return;
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
        pMask = pDst;
        x = 0;
        y = 0;
    }
    while (nlist--) {
        x += list->xOff;
        y += list->yOff;
        n = list->len;
        while (n--) {
            glyph = *glyphs++;
            pPicture = GetGlyphPicture(glyph, pScreen);

            if (pPicture) {
                if (maskFormat) {
                    CompositePicture(PictOpAdd,
                                     pPicture,
                                     None,
                                     pMask,
                                     0, 0,
                                     0, 0,
                                     x - glyph->info.x,
                                     y - glyph->info.y,
                                     glyph->info.width, glyph->info.height);
                }
                else {
                    CompositePicture(op,
                                     pSrc,
                                     pPicture,
                                     pDst,
                                     xSrc + (x - glyph->info.x) - xDst,
                                     ySrc + (y - glyph->info.y) - yDst,
                                     0, 0,
                                     x - glyph->info.x,
                                     y - glyph->info.y,
                                     glyph->info.width, glyph->info.height);
                }
            }

            x += glyph->info.xOff;
            y += glyph->info.yOff;
        }
        list++;
    }
    if (maskFormat) {
        x = extents.x1;
        y = extents.y1;
        CompositePicture(op,
                         pSrc,
                         pMask,
                         pDst,
                         xSrc + x - xDst,
                         ySrc + y - yDst, 0, 0, x, y, width, height);
        FreePicture((void *) pMask, (XID) 0);
        (*pScreen->DestroyPixmap) (pMaskPixmap);
    }
}

PicturePtr GetGlyphPicture(GlyphPtr glyph, ScreenPtr pScreen)
{
    if (pScreen->isGPU)
        return NULL;
    return GlyphPicture(glyph)[pScreen->myNum];
}

void SetGlyphPicture(GlyphPtr glyph, ScreenPtr pScreen, PicturePtr picture)
{
    GlyphPicture(glyph)[pScreen->myNum] = picture;
}
