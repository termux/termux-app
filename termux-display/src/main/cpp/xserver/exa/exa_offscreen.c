/*
 * Copyright © 2003 Anders Carlsson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Anders Carlsson not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Anders Carlsson makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ANDERS CARLSSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANDERS CARLSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/** @file
 * This allocator allocates blocks of memory by maintaining a list of areas.
 * When allocating, the contiguous block of areas with the minimum eviction
 * cost is found and evicted in order to make room for the new allocation.
 */

#include "exa_priv.h"

#include <limits.h>
#include <assert.h>
#include <stdlib.h>

#if DEBUG_OFFSCREEN
#define DBG_OFFSCREEN(a) ErrorF a
#else
#define DBG_OFFSCREEN(a)
#endif

#if DEBUG_OFFSCREEN
static void
ExaOffscreenValidate(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea *prev = 0, *area;

    assert(pExaScr->info->offScreenAreas->base_offset ==
           pExaScr->info->offScreenBase);
    for (area = pExaScr->info->offScreenAreas; area; area = area->next) {
        assert(area->offset >= area->base_offset);
        assert(area->offset < (area->base_offset + area->size));
        if (prev)
            assert(prev->base_offset + prev->size == area->base_offset);
        prev = area;
    }
    assert(prev->base_offset + prev->size == pExaScr->info->memorySize);
}
#else
#define ExaOffscreenValidate(s)
#endif

static ExaOffscreenArea *
ExaOffscreenKickOut(ScreenPtr pScreen, ExaOffscreenArea * area)
{
    if (area->save)
        (*area->save) (pScreen, area);
    return exaOffscreenFree(pScreen, area);
}

static void
exaUpdateEvictionCost(ExaOffscreenArea * area, unsigned offScreenCounter)
{
    unsigned age;

    if (area->state == ExaOffscreenAvail)
        return;

    age = offScreenCounter - area->last_use;

    /* This is unlikely to happen, but could result in a division by zero... */
    if (age > (UINT_MAX / 2)) {
        age = UINT_MAX / 2;
        area->last_use = offScreenCounter - age;
    }

    area->eviction_cost = area->size / age;
}

static ExaOffscreenArea *
exaFindAreaToEvict(ExaScreenPrivPtr pExaScr, int size, int align)
{
    ExaOffscreenArea *begin, *end, *best;
    unsigned cost, best_cost;
    int avail, real_size;

    best_cost = UINT_MAX;
    begin = end = pExaScr->info->offScreenAreas;
    avail = 0;
    cost = 0;
    best = 0;

    while (end != NULL) {
 restart:
        while (begin != NULL && begin->state == ExaOffscreenLocked)
            begin = end = begin->next;

        if (begin == NULL)
            break;

        /* adjust size needed to account for alignment loss for this area */
        real_size = size + (begin->base_offset + begin->size - size) % align;

        while (avail < real_size && end != NULL) {
            if (end->state == ExaOffscreenLocked) {
                /* Can't more room here, restart after this locked area */
                avail = 0;
                cost = 0;
                begin = end;
                goto restart;
            }
            avail += end->size;
            exaUpdateEvictionCost(end, pExaScr->offScreenCounter);
            cost += end->eviction_cost;
            end = end->next;
        }

        /* Check the cost, update best */
        if (avail >= real_size && cost < best_cost) {
            best = begin;
            best_cost = cost;
        }

        avail -= begin->size;
        cost -= begin->eviction_cost;
        begin = begin->next;
    }

    return best;
}

/**
 * exaOffscreenAlloc allocates offscreen memory
 *
 * @param pScreen current screen
 * @param size size in bytes of the allocation
 * @param align byte alignment requirement for the offset of the allocated area
 * @param locked whether the allocated area is locked and can't be kicked out
 * @param save callback for when the area is evicted from memory
 * @param privdata private data for the save callback.
 *
 * Allocates offscreen memory from the device associated with pScreen.  size
 * and align determine where and how large the allocated area is, and locked
 * will mark whether it should be held in card memory.  privdata may be any
 * pointer for the save callback when the area is removed.
 *
 * Note that locked areas do get evicted on VT switch unless the driver
 * requested version 2.1 or newer behavior.  In that case, the save callback is
 * still called.
 */
ExaOffscreenArea *
exaOffscreenAlloc(ScreenPtr pScreen, int size, int align,
                  Bool locked, ExaOffscreenSaveProc save, void *privData)
{
    ExaOffscreenArea *area;

    ExaScreenPriv(pScreen);
    int real_size = 0, largest_avail = 0;

#if DEBUG_OFFSCREEN
    static int number = 0;

    ErrorF("================= ============ allocating a new pixmap %d\n",
           ++number);
#endif

    ExaOffscreenValidate(pScreen);
    if (!align)
        align = 1;

    if (!size) {
        DBG_OFFSCREEN(("Alloc 0x%x -> EMPTY\n", size));
        return NULL;
    }

    /* throw out requests that cannot fit */
    if (size > (pExaScr->info->memorySize - pExaScr->info->offScreenBase)) {
        DBG_OFFSCREEN(("Alloc 0x%x vs (0x%lx) -> TOBIG\n", size,
                       pExaScr->info->memorySize -
                       pExaScr->info->offScreenBase));
        return NULL;
    }

    /* Try to find a free space that'll fit. */
    for (area = pExaScr->info->offScreenAreas; area; area = area->next) {
        /* skip allocated areas */
        if (area->state != ExaOffscreenAvail)
            continue;

        /* adjust size to match alignment requirement */
        real_size = size + (area->base_offset + area->size - size) % align;

        /* does it fit? */
        if (real_size <= area->size)
            break;

        if (area->size > largest_avail)
            largest_avail = area->size;
    }

    if (!area) {
        area = exaFindAreaToEvict(pExaScr, size, align);

        if (!area) {
            DBG_OFFSCREEN(("Alloc 0x%x -> NOSPACE\n", size));
            /* Could not allocate memory */
            ExaOffscreenValidate(pScreen);
            return NULL;
        }

        /* adjust size needed to account for alignment loss for this area */
        real_size = size + (area->base_offset + area->size - size) % align;

        /*
         * Kick out first area if in use
         */
        if (area->state != ExaOffscreenAvail)
            area = ExaOffscreenKickOut(pScreen, area);
        /*
         * Now get the system to merge the other needed areas together
         */
        while (area->size < real_size) {
            assert(area->next);
            assert(area->next->state == ExaOffscreenRemovable);
            (void) ExaOffscreenKickOut(pScreen, area->next);
        }
    }

    /* save extra space in new area */
    if (real_size < area->size) {
        ExaOffscreenArea *new_area = malloc(sizeof(ExaOffscreenArea));

        if (!new_area)
            return NULL;
        new_area->base_offset = area->base_offset;

        new_area->offset = new_area->base_offset;
        new_area->align = 0;
        new_area->size = area->size - real_size;
        new_area->state = ExaOffscreenAvail;
        new_area->save = NULL;
        new_area->last_use = 0;
        new_area->eviction_cost = 0;
        new_area->next = area;
        new_area->prev = area->prev;
        if (area->prev->next)
            area->prev->next = new_area;
        else
            pExaScr->info->offScreenAreas = new_area;
        area->prev = new_area;
        area->base_offset = new_area->base_offset + new_area->size;
        area->size = real_size;
    }
    else
        pExaScr->numOffscreenAvailable--;

    /*
     * Mark this area as in use
     */
    if (locked)
        area->state = ExaOffscreenLocked;
    else
        area->state = ExaOffscreenRemovable;
    area->privData = privData;
    area->save = save;
    area->last_use = pExaScr->offScreenCounter++;
    area->offset = (area->base_offset + align - 1);
    area->offset -= area->offset % align;
    area->align = align;

    ExaOffscreenValidate(pScreen);

    DBG_OFFSCREEN(("Alloc 0x%x -> 0x%x (0x%x)\n", size,
                   area->base_offset, area->offset));
    return area;
}

/**
 * Ejects all offscreen areas, and uninitializes the offscreen memory manager.
 */
void
ExaOffscreenSwapOut(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    ExaOffscreenValidate(pScreen);
    /* loop until a single free area spans the space */
    for (;;) {
        ExaOffscreenArea *area = pExaScr->info->offScreenAreas;

        if (!area)
            break;
        if (area->state == ExaOffscreenAvail) {
            area = area->next;
            if (!area)
                break;
        }
        assert(area->state != ExaOffscreenAvail);
        (void) ExaOffscreenKickOut(pScreen, area);
        ExaOffscreenValidate(pScreen);
    }
    ExaOffscreenValidate(pScreen);
    ExaOffscreenFini(pScreen);
}

/** Ejects all pixmaps managed by EXA. */
static void
ExaOffscreenEjectPixmaps(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    ExaOffscreenValidate(pScreen);
    /* loop until a single free area spans the space */
    for (;;) {
        ExaOffscreenArea *area;

        for (area = pExaScr->info->offScreenAreas; area != NULL;
             area = area->next) {
            if (area->state == ExaOffscreenRemovable &&
                area->save == exaPixmapSave) {
                (void) ExaOffscreenKickOut(pScreen, area);
                ExaOffscreenValidate(pScreen);
                break;
            }
        }
        if (area == NULL)
            break;
    }
    ExaOffscreenValidate(pScreen);
}

void
ExaOffscreenSwapIn(ScreenPtr pScreen)
{
    exaOffscreenInit(pScreen);
}

/**
 * Prepares EXA for disabling of FB access, or restoring it.
 *
 * In version 2.1, the disabling results in pixmaps being ejected, while other
 * allocations remain.  With this plus the prevention of migration while
 * swappedOut is set, EXA by itself should not cause any access of the
 * framebuffer to occur while swapped out.  Any remaining issues are the
 * responsibility of the driver.
 *
 * Prior to version 2.1, all allocations, including locked ones, are ejected
 * when access is disabled, and the allocator is torn down while swappedOut
 * is set.  This is more drastic, and caused implementation difficulties for
 * many drivers that could otherwise handle the lack of FB access while
 * swapped out.
 */
void
exaEnableDisableFBAccess(ScreenPtr pScreen, Bool enable)
{
    ExaScreenPriv(pScreen);

    if (pExaScr->info->flags & EXA_HANDLES_PIXMAPS)
        return;

    if (!enable && pExaScr->disableFbCount++ == 0) {
        if (pExaScr->info->exa_minor < 1)
            ExaOffscreenSwapOut(pScreen);
        else
            ExaOffscreenEjectPixmaps(pScreen);
        pExaScr->swappedOut = TRUE;
    }

    if (enable && --pExaScr->disableFbCount == 0) {
        if (pExaScr->info->exa_minor < 1)
            ExaOffscreenSwapIn(pScreen);
        pExaScr->swappedOut = FALSE;
    }
}

/* merge the next free area into this one */
static void
ExaOffscreenMerge(ExaScreenPrivPtr pExaScr, ExaOffscreenArea * area)
{
    ExaOffscreenArea *next = area->next;

    /* account for space */
    area->size += next->size;
    /* frob pointer */
    area->next = next->next;
    if (area->next)
        area->next->prev = area;
    else
        pExaScr->info->offScreenAreas->prev = area;
    free(next);

    pExaScr->numOffscreenAvailable--;
}

/**
 * exaOffscreenFree frees an allocation.
 *
 * @param pScreen current screen
 * @param area offscreen area to free
 *
 * exaOffscreenFree frees an allocation created by exaOffscreenAlloc.  Note that
 * the save callback of the area is not called, and it is up to the driver to
 * do any cleanup necessary as a result.
 *
 * @return pointer to the newly freed area. This behavior should not be relied
 * on.
 */
ExaOffscreenArea *
exaOffscreenFree(ScreenPtr pScreen, ExaOffscreenArea * area)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea *next = area->next;
    ExaOffscreenArea *prev;

    DBG_OFFSCREEN(("Free 0x%x -> 0x%x (0x%x)\n", area->size,
                   area->base_offset, area->offset));
    ExaOffscreenValidate(pScreen);

    area->state = ExaOffscreenAvail;
    area->save = NULL;
    area->last_use = 0;
    area->eviction_cost = 0;
    /*
     * Find previous area
     */
    if (area == pExaScr->info->offScreenAreas)
        prev = NULL;
    else
        prev = area->prev;

    pExaScr->numOffscreenAvailable++;

    /* link with next area if free */
    if (next && next->state == ExaOffscreenAvail)
        ExaOffscreenMerge(pExaScr, area);

    /* link with prev area if free */
    if (prev && prev->state == ExaOffscreenAvail) {
        area = prev;
        ExaOffscreenMerge(pExaScr, area);
    }

    ExaOffscreenValidate(pScreen);
    DBG_OFFSCREEN(("\tdone freeing\n"));
    return area;
}

void
ExaOffscreenMarkUsed(PixmapPtr pPixmap)
{
    ExaPixmapPriv(pPixmap);
    ExaScreenPriv(pPixmap->drawable.pScreen);

    if (!pExaPixmap || !pExaPixmap->area)
        return;

    pExaPixmap->area->last_use = pExaScr->offScreenCounter++;
}

/**
 * Defragment offscreen memory by compacting allocated areas at the end of it,
 * leaving the total amount of memory available as a single area at the
 * beginning (when there are no pinned allocations).
 */
_X_HIDDEN ExaOffscreenArea *
ExaOffscreenDefragment(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea *area, *largest_available = NULL;
    int largest_size = 0;
    PixmapPtr pDstPix;
    ExaPixmapPrivPtr pExaDstPix;

    pDstPix = (*pScreen->CreatePixmap) (pScreen, 0, 0, 0, 0);

    if (!pDstPix)
        return NULL;

    pExaDstPix = ExaGetPixmapPriv(pDstPix);
    pExaDstPix->use_gpu_copy = TRUE;

    for (area = pExaScr->info->offScreenAreas->prev;
         area != pExaScr->info->offScreenAreas;) {
        ExaOffscreenArea *prev = area->prev;
        PixmapPtr pSrcPix;
        ExaPixmapPrivPtr pExaSrcPix;
        Bool save_use_gpu_copy;
        int save_pitch;

        if (area->state != ExaOffscreenAvail ||
            prev->state == ExaOffscreenLocked ||
            (prev->state == ExaOffscreenRemovable &&
             prev->save != exaPixmapSave)) {
            area = prev;
            continue;
        }

        if (prev->state == ExaOffscreenAvail) {
            if (area == largest_available) {
                largest_available = prev;
                largest_size += prev->size;
            }
            area = prev;
            ExaOffscreenMerge(pExaScr, area);
            continue;
        }

        if (area->size > largest_size) {
            largest_available = area;
            largest_size = area->size;
        }

        pSrcPix = prev->privData;
        pExaSrcPix = ExaGetPixmapPriv(pSrcPix);

        pExaDstPix->fb_ptr = pExaScr->info->memoryBase +
            area->base_offset + area->size - prev->size + prev->base_offset -
            prev->offset;
        pExaDstPix->fb_ptr -= (unsigned long) pExaDstPix->fb_ptr % prev->align;

        if (pExaDstPix->fb_ptr <= pExaSrcPix->fb_ptr) {
            area = prev;
            continue;
        }

        if (!(pExaScr->info->flags & EXA_SUPPORTS_OFFSCREEN_OVERLAPS) &&
            (pExaSrcPix->fb_ptr + prev->size) > pExaDstPix->fb_ptr) {
            area = prev;
            continue;
        }

        save_use_gpu_copy = pExaSrcPix->use_gpu_copy;
        save_pitch = pSrcPix->devKind;

        pExaSrcPix->use_gpu_copy = TRUE;
        pSrcPix->devKind = pExaSrcPix->fb_pitch;

        pDstPix->drawable.width = pSrcPix->drawable.width;
        pDstPix->devKind = pSrcPix->devKind;
        pDstPix->drawable.height = pSrcPix->drawable.height;
        pDstPix->drawable.depth = pSrcPix->drawable.depth;
        pDstPix->drawable.bitsPerPixel = pSrcPix->drawable.bitsPerPixel;

        if (!pExaScr->info->PrepareCopy(pSrcPix, pDstPix, -1, -1, GXcopy, ~0)) {
            pExaSrcPix->use_gpu_copy = save_use_gpu_copy;
            pSrcPix->devKind = save_pitch;
            area = prev;
            continue;
        }

        pExaScr->info->Copy(pDstPix, 0, 0, 0, 0, pDstPix->drawable.width,
                            pDstPix->drawable.height);
        pExaScr->info->DoneCopy(pDstPix);
        exaMarkSync(pScreen);

        DBG_OFFSCREEN(("Before swap: prev=0x%08x-0x%08x-0x%08x area=0x%08x-0x%08x-0x%08x\n", prev->base_offset, prev->offset, prev->base_offset + prev->size, area->base_offset, area->offset, area->base_offset + area->size));

        /* Calculate swapped area offsets and sizes */
        area->base_offset = prev->base_offset;
        area->offset = area->base_offset;
        prev->offset += pExaDstPix->fb_ptr - pExaSrcPix->fb_ptr;
        assert(prev->offset >= pExaScr->info->offScreenBase);
        assert(prev->offset < pExaScr->info->memorySize);
        prev->base_offset = prev->offset;
        if (area->next)
            prev->size = area->next->base_offset - prev->base_offset;
        else
            prev->size = pExaScr->info->memorySize - prev->base_offset;
        area->size = prev->base_offset - area->base_offset;

        DBG_OFFSCREEN(("After swap: area=0x%08x-0x%08x-0x%08x prev=0x%08x-0x%08x-0x%08x\n", area->base_offset, area->offset, area->base_offset + area->size, prev->base_offset, prev->offset, prev->base_offset + prev->size));

        /* Swap areas in list */
        if (area->next)
            area->next->prev = prev;
        else
            pExaScr->info->offScreenAreas->prev = prev;
        if (prev->prev->next)
            prev->prev->next = area;
        else
            pExaScr->info->offScreenAreas = area;
        prev->next = area->next;
        area->next = prev;
        area->prev = prev->prev;
        prev->prev = area;
        if (!area->prev->next)
            pExaScr->info->offScreenAreas = area;

#if DEBUG_OFFSCREEN
        if (prev->prev == prev || prev->next == prev)
            ErrorF("Whoops, prev points to itself!\n");

        if (area->prev == area || area->next == area)
            ErrorF("Whoops, area points to itself!\n");
#endif

        pExaSrcPix->fb_ptr = pExaDstPix->fb_ptr;
        pExaSrcPix->use_gpu_copy = save_use_gpu_copy;
        pSrcPix->devKind = save_pitch;
    }

    pDstPix->drawable.width = 0;
    pDstPix->drawable.height = 0;
    pDstPix->drawable.depth = 0;
    pDstPix->drawable.bitsPerPixel = 0;

    (*pScreen->DestroyPixmap) (pDstPix);

    if (area->state == ExaOffscreenAvail && area->size > largest_size)
        return area;

    return largest_available;
}

/**
 * exaOffscreenInit initializes the offscreen memory manager.
 *
 * @param pScreen current screen
 *
 * exaOffscreenInit is called by exaDriverInit to set up the memory manager for
 * the screen, if any offscreen memory is available.
 */
Bool
exaOffscreenInit(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea *area;

    /* Allocate a big free area */
    area = malloc(sizeof(ExaOffscreenArea));

    if (!area)
        return FALSE;

    area->state = ExaOffscreenAvail;
    area->base_offset = pExaScr->info->offScreenBase;
    area->offset = area->base_offset;
    area->align = 0;
    area->size = pExaScr->info->memorySize - area->base_offset;
    area->save = NULL;
    area->next = NULL;
    area->prev = area;
    area->last_use = 0;
    area->eviction_cost = 0;

    /* Add it to the free areas */
    pExaScr->info->offScreenAreas = area;
    pExaScr->offScreenCounter = 1;
    pExaScr->numOffscreenAvailable = 1;

    ExaOffscreenValidate(pScreen);

    return TRUE;
}

void
ExaOffscreenFini(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaOffscreenArea *area;

    /* just free all of the area records */
    while ((area = pExaScr->info->offScreenAreas)) {
        pExaScr->info->offScreenAreas = area->next;
        free(area);
    }
}
