/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "ephyr.h"
#include "exa_priv.h"
#include "fbpict.h"

#define EPHYR_TRACE_DRAW 0

#if EPHYR_TRACE_DRAW
#define TRACE_DRAW() ErrorF("%s\n", __FUNCTION__);
#else
#define TRACE_DRAW() do { } while (0)
#endif

/* Use some oddball alignments, to expose issues in alignment handling in EXA. */
#define EPHYR_OFFSET_ALIGN	24
#define EPHYR_PITCH_ALIGN	24

#define EPHYR_OFFSCREEN_SIZE	(16 * 1024 * 1024)
#define EPHYR_OFFSCREEN_BASE	(1 * 1024 * 1024)

/**
 * Forces a real devPrivate.ptr for hidden pixmaps, so that we can call down to
 * fb functions.
 */
static void
ephyrPreparePipelinedAccess(PixmapPtr pPix, int index)
{
    KdScreenPriv(pPix->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    assert(fakexa->saved_ptrs[index] == NULL);
    fakexa->saved_ptrs[index] = pPix->devPrivate.ptr;

    if (pPix->devPrivate.ptr != NULL)
        return;

    pPix->devPrivate.ptr = fakexa->exa->memoryBase + exaGetPixmapOffset(pPix);
}

/**
 * Restores the original devPrivate.ptr of the pixmap from before we messed with
 * it.
 */
static void
ephyrFinishPipelinedAccess(PixmapPtr pPix, int index)
{
    KdScreenPriv(pPix->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    pPix->devPrivate.ptr = fakexa->saved_ptrs[index];
    fakexa->saved_ptrs[index] = NULL;
}

/**
 * Sets up a scratch GC for fbFill, and saves other parameters for the
 * ephyrSolid implementation.
 */
static Bool
ephyrPrepareSolid(PixmapPtr pPix, int alu, Pixel pm, Pixel fg)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    ChangeGCVal tmpval[3];

    ephyrPreparePipelinedAccess(pPix, EXA_PREPARE_DEST);

    fakexa->pDst = pPix;
    fakexa->pGC = GetScratchGC(pPix->drawable.depth, pScreen);

    tmpval[0].val = alu;
    tmpval[1].val = pm;
    tmpval[2].val = fg;
    ChangeGC(NullClient, fakexa->pGC, GCFunction | GCPlaneMask | GCForeground,
             tmpval);

    ValidateGC(&pPix->drawable, fakexa->pGC);

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbFill of the rectangle to be drawn.
 */
static void
ephyrSolid(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbFill(&fakexa->pDst->drawable, fakexa->pGC, x1, y1, x2 - x1, y2 - y1);
}

/**
 * Cleans up the scratch GC created in ephyrPrepareSolid.
 */
static void
ephyrDoneSolid(PixmapPtr pPix)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    FreeScratchGC(fakexa->pGC);

    ephyrFinishPipelinedAccess(pPix, EXA_PREPARE_DEST);
}

/**
 * Sets up a scratch GC for fbCopyArea, and saves other parameters for the
 * ephyrCopy implementation.
 */
static Bool
ephyrPrepareCopy(PixmapPtr pSrc, PixmapPtr pDst, int dx, int dy, int alu,
                 Pixel pm)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    ChangeGCVal tmpval[2];

    ephyrPreparePipelinedAccess(pDst, EXA_PREPARE_DEST);
    ephyrPreparePipelinedAccess(pSrc, EXA_PREPARE_SRC);

    fakexa->pSrc = pSrc;
    fakexa->pDst = pDst;
    fakexa->pGC = GetScratchGC(pDst->drawable.depth, pScreen);

    tmpval[0].val = alu;
    tmpval[1].val = pm;
    ChangeGC(NullClient, fakexa->pGC, GCFunction | GCPlaneMask, tmpval);

    ValidateGC(&pDst->drawable, fakexa->pGC);

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbCopyArea to take care of the requested copy.
 */
static void
ephyrCopy(PixmapPtr pDst, int srcX, int srcY, int dstX, int dstY, int w, int h)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbCopyArea(&fakexa->pSrc->drawable, &fakexa->pDst->drawable, fakexa->pGC,
               srcX, srcY, w, h, dstX, dstY);
}

/**
 * Cleans up the scratch GC created in ephyrPrepareCopy.
 */
static void
ephyrDoneCopy(PixmapPtr pDst)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    FreeScratchGC(fakexa->pGC);

    ephyrFinishPipelinedAccess(fakexa->pSrc, EXA_PREPARE_SRC);
    ephyrFinishPipelinedAccess(fakexa->pDst, EXA_PREPARE_DEST);
}

/**
 * Reports that we can always accelerate the given operation.  This may not be
 * desirable from an EXA testing standpoint -- testing the fallback paths would
 * be useful, too.
 */
static Bool
ephyrCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
                    PicturePtr pDstPicture)
{
    /* Exercise the component alpha helper, so fail on this case like a normal
     * driver
     */
    if (pMaskPicture && pMaskPicture->componentAlpha && op == PictOpOver)
        return FALSE;

    return TRUE;
}

/**
 * Saves off the parameters for ephyrComposite.
 */
static Bool
ephyrPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
                      PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask,
                      PixmapPtr pDst)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    ephyrPreparePipelinedAccess(pDst, EXA_PREPARE_DEST);
    if (pSrc != NULL)
        ephyrPreparePipelinedAccess(pSrc, EXA_PREPARE_SRC);
    if (pMask != NULL)
        ephyrPreparePipelinedAccess(pMask, EXA_PREPARE_MASK);

    fakexa->op = op;
    fakexa->pSrcPicture = pSrcPicture;
    fakexa->pMaskPicture = pMaskPicture;
    fakexa->pDstPicture = pDstPicture;
    fakexa->pSrc = pSrc;
    fakexa->pMask = pMask;
    fakexa->pDst = pDst;

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbComposite to complete the requested drawing operation.
 */
static void
ephyrComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
               int dstX, int dstY, int w, int h)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbComposite(fakexa->op, fakexa->pSrcPicture, fakexa->pMaskPicture,
                fakexa->pDstPicture, srcX, srcY, maskX, maskY, dstX, dstY,
                w, h);
}

static void
ephyrDoneComposite(PixmapPtr pDst)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    if (fakexa->pMask != NULL)
        ephyrFinishPipelinedAccess(fakexa->pMask, EXA_PREPARE_MASK);
    if (fakexa->pSrc != NULL)
        ephyrFinishPipelinedAccess(fakexa->pSrc, EXA_PREPARE_SRC);
    ephyrFinishPipelinedAccess(fakexa->pDst, EXA_PREPARE_DEST);
}

/**
 * Does fake acceleration of DownloadFromScren using memcpy.
 */
static Bool
ephyrDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h, char *dst,
                        int dst_pitch)
{
    KdScreenPriv(pSrc->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    unsigned char *src;
    int src_pitch, cpp;

    if (pSrc->drawable.bitsPerPixel < 8)
        return FALSE;

    ephyrPreparePipelinedAccess(pSrc, EXA_PREPARE_SRC);

    cpp = pSrc->drawable.bitsPerPixel / 8;
    src_pitch = exaGetPixmapPitch(pSrc);
    src = fakexa->exa->memoryBase + exaGetPixmapOffset(pSrc);
    src += y * src_pitch + x * cpp;

    for (; h > 0; h--) {
        memcpy(dst, src, w * cpp);
        dst += dst_pitch;
        src += src_pitch;
    }

    exaMarkSync(pSrc->drawable.pScreen);

    ephyrFinishPipelinedAccess(pSrc, EXA_PREPARE_SRC);

    return TRUE;
}

/**
 * Does fake acceleration of UploadToScreen using memcpy.
 */
static Bool
ephyrUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src,
                    int src_pitch)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    unsigned char *dst;
    int dst_pitch, cpp;

    if (pDst->drawable.bitsPerPixel < 8)
        return FALSE;

    ephyrPreparePipelinedAccess(pDst, EXA_PREPARE_DEST);

    cpp = pDst->drawable.bitsPerPixel / 8;
    dst_pitch = exaGetPixmapPitch(pDst);
    dst = fakexa->exa->memoryBase + exaGetPixmapOffset(pDst);
    dst += y * dst_pitch + x * cpp;

    for (; h > 0; h--) {
        memcpy(dst, src, w * cpp);
        dst += dst_pitch;
        src += src_pitch;
    }

    exaMarkSync(pDst->drawable.pScreen);

    ephyrFinishPipelinedAccess(pDst, EXA_PREPARE_DEST);

    return TRUE;
}

static Bool
ephyrPrepareAccess(PixmapPtr pPix, int index)
{
    /* Make sure we don't somehow end up with a pointer that is in framebuffer
     * and hasn't been readied for us.
     */
    assert(pPix->devPrivate.ptr != NULL);

    return TRUE;
}

/**
 * In fakexa, we currently only track whether we have synced to the latest
 * "accelerated" drawing that has happened or not.  It's not used for anything
 * yet.
 */
static int
ephyrMarkSync(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fakexa->is_synced = FALSE;

    return 0;
}

/**
 * Assumes that we're waiting on the latest marker.  When EXA gets smarter and
 * starts using markers in a fine-grained way (for example, waiting on drawing
 * to required pixmaps to complete, rather than waiting for all drawing to
 * complete), we'll want to make the ephyrMarkSync/ephyrWaitMarker
 * implementation fine-grained as well.
 */
static void
ephyrWaitMarker(ScreenPtr pScreen, int marker)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fakexa->is_synced = TRUE;
}

/**
 * This function initializes EXA to use the fake acceleration implementation
 * which just falls through to software.  The purpose is to have a reliable,
 * correct driver with which to test changes to the EXA core.
 */
Bool
ephyrDrawInit(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrPriv *priv = screen->card->driver;
    EphyrFakexaPriv *fakexa;
    Bool success;

    fakexa = calloc(1, sizeof(*fakexa));
    if (fakexa == NULL)
        return FALSE;

    fakexa->exa = exaDriverAlloc();
    if (fakexa->exa == NULL) {
        free(fakexa);
        return FALSE;
    }

    fakexa->exa->memoryBase = (CARD8 *) (priv->base);
    fakexa->exa->memorySize = priv->bytes_per_line * ephyrBufferHeight(screen);
    fakexa->exa->offScreenBase = priv->bytes_per_line * screen->height;

    /* Since we statically link against EXA, we shouldn't have to be smart about
     * versioning.
     */
    fakexa->exa->exa_major = 2;
    fakexa->exa->exa_minor = 0;

    fakexa->exa->PrepareSolid = ephyrPrepareSolid;
    fakexa->exa->Solid = ephyrSolid;
    fakexa->exa->DoneSolid = ephyrDoneSolid;

    fakexa->exa->PrepareCopy = ephyrPrepareCopy;
    fakexa->exa->Copy = ephyrCopy;
    fakexa->exa->DoneCopy = ephyrDoneCopy;

    fakexa->exa->CheckComposite = ephyrCheckComposite;
    fakexa->exa->PrepareComposite = ephyrPrepareComposite;
    fakexa->exa->Composite = ephyrComposite;
    fakexa->exa->DoneComposite = ephyrDoneComposite;

    fakexa->exa->DownloadFromScreen = ephyrDownloadFromScreen;
    fakexa->exa->UploadToScreen = ephyrUploadToScreen;

    fakexa->exa->MarkSync = ephyrMarkSync;
    fakexa->exa->WaitMarker = ephyrWaitMarker;

    fakexa->exa->PrepareAccess = ephyrPrepareAccess;

    fakexa->exa->pixmapOffsetAlign = EPHYR_OFFSET_ALIGN;
    fakexa->exa->pixmapPitchAlign = EPHYR_PITCH_ALIGN;

    fakexa->exa->maxX = 1023;
    fakexa->exa->maxY = 1023;

    fakexa->exa->flags = EXA_OFFSCREEN_PIXMAPS;

    success = exaDriverInit(pScreen, fakexa->exa);
    if (success) {
        ErrorF("Initialized fake EXA acceleration\n");
        scrpriv->fakexa = fakexa;
    }
    else {
        ErrorF("Failed to initialize EXA\n");
        free(fakexa->exa);
        free(fakexa);
    }

    return success;
}

void
ephyrDrawEnable(ScreenPtr pScreen)
{
}

void
ephyrDrawDisable(ScreenPtr pScreen)
{
}

void
ephyrDrawFini(ScreenPtr pScreen)
{
}

/**
 * exaDDXDriverInit is required by the top-level EXA module, and is used by
 * the xorg DDX to hook in its EnableDisableFB wrapper.  We don't need it, since
 * we won't be enabling/disabling the FB.
 */
void
exaDDXDriverInit(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    pExaScr->migration = ExaMigrationSmart;
    pExaScr->checkDirtyCorrectness = TRUE;
}
