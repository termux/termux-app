#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <android/log.h>
#include <gcstruct.h>
#include <privates.h>
#include <scrnintstr.h>
#include <dri3.h>
#include <sys/mman.h>
#include <fb.h>
#include <android/hardware_buffer.h>
#include <sys/stat.h>
#include <errno.h>
#include "screenint.h"
#include "lorie.h"
#include "renderer.h"

#define log(prio, ...) __android_log_print(ANDROID_LOG_ ## prio, "LorieNative", __VA_ARGS__)

/*
 * Design is pretty simple.
 * We need somehow attach Android's HardwareBuffers and turnip's textures to X11 pixmaps.
 *
 * About turnip.
 * We can not simply import Mesa's texture since we do not use Mesa in project
 * and even in the case it we ever use it we can not mix mesa with Android's libEGL.
 * Xext's shm allows us to attach dmabuf fd with given width, height and offset,
 * but does not let us specify stride of buffer.
 * For this case we use DRI3's pixmap_from_fds request since it allows us specify
 * width, height, stride and even import fd from client.
 *
 * About Android Hardware Buffers.
 * NDK API does not let us simply flatten GraphicBuffer like it is done in regular AOSP code
 * or even simply extract native handle which contains data and file descriptors
 * needed to recreate GraphicBuffer in different process.
 * But we have AHardwareBuffer_sendHandleToUnixSocket and AHardwareBuffer_recvHandleFromUnixSocket
 * which let us send and receive AHardwareBuffer (aka GraphicBuffer) through Unix socket.
 * So X11 client can simply open socketpair, wait for a while until server sends one byte
 * and respond with AhardwareBuffer using AHardwareBuffer_sendHandleToUnixSocket.
 * About attaching AHardwareBuffer to X11 pixmap:
 * We can not simply do AHardwareBuffer_lock after creating pixmap and expect it to work.
 * Some platforms (especially platforms with separate GPU memory which can not be accessed with CPU)
 * explicitly copy contents of video memory to CPU memory on AHardwareBuffer_lock and content of buffer
 * copied this way is not affected by any actions in X11 client process.
 * So we must explicitly call AHardwareBuffer_lock when GC needs to access pixmap
 * and call AHardwareBuffer_unlock when it finishes its work.
 * Also we consider AHardwareBuffer we have to be read-only, write allowed only for X11 client.
 * So all functions except CopyArea applied to pixmap with attached AHardwareBuffer should be no-ops.
 */

static DevPrivateKeyRec lorieGCPrivateKey;
static DevPrivateKeyRec lorieScrPrivateKey;
static DevPrivateKeyRec lorieAHBPixPrivateKey;
static DevPrivateKeyRec lorieMmappedPixPrivateKey;

typedef struct {
    const GCOps *ops;
    const GCFuncs *funcs;
} LorieGCPrivRec, *LorieGCPrivPtr;

typedef struct {
    CreateGCProcPtr CreateGC;
    DestroyPixmapProcPtr DestroyPixmap;
} LorieScrPrivRec, *LorieScrPrivPtr;

typedef struct {
    AHardwareBuffer* buffer;
} LorieAHBPixPrivRec, *LorieAHBPixPrivPtr;

static Bool FalseNoop() { return FALSE; }

#define lorieGCPriv(pGC) LorieGCPrivPtr pGCPriv = dixLookupPrivate(&(pGC)->devPrivates, &lorieGCPrivateKey)
#define lorieScrPriv(pScr) LorieScrPrivPtr pScrPriv = ((LorieScrPrivPtr) dixLookupPrivate(&(pScr)->devPrivates, &lorieScrPrivateKey))
#define loriePixFromDrawable(pDrawable, suffix) \
    PixmapPtr pDrawable ## Pix ## suffix = (pDrawable->type == DRAWABLE_PIXMAP) ? \
        (PixmapPtr) (((char*) pDrawable) - offsetof(PixmapRec, drawable)) : 0
#define loriePixPriv(pDrawable, suffix) \
    LorieAHBPixPrivPtr pPixPriv ## suffix = (pDrawable ## Pix ## suffix) ? ((LorieAHBPixPrivPtr) \
        dixLookupPrivate(&(pDrawable ## Pix ## suffix)->devPrivates, &lorieAHBPixPrivateKey)) : 0

#define wrap(priv, real, mem, func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define LORIE_GC_OP_PROLOGUE(pGC) \
    lorieGCPriv(pGC);  \
    const GCFuncs *oldFuncs = pGC->funcs; \
    const GCOps *oldOps = pGC->ops; \
    unwrap(pGCPriv, pGC, funcs);  \
    unwrap(pGCPriv, pGC, ops); \

#define LORIE_GC_OP_EPILOGUE(pGC) \
    wrap(pGCPriv, pGC, funcs, oldFuncs); \
    wrap(pGCPriv, pGC, ops, oldOps)

#define LORIE_GC_FUNC_PROLOGUE(pGC) \
    lorieGCPriv(pGC); \
    unwrap(pGCPriv, pGC, funcs); \
    if (pGCPriv->ops) unwrap(pGCPriv, pGC, ops)

#define LORIE_GC_FUNC_EPILOGUE(pGC) \
    wrap(pGCPriv, pGC, funcs, &lorieGCFuncs);  \
    if (pGCPriv->ops) wrap(pGCPriv, pGC, ops, &lorieGCOps)

#define unwrap(priv, real, mem) {\
    real->mem = priv->mem; \
}

static const GCOps lorieGCOps;
static const GCFuncs lorieGCFuncs;

static void lorieValidateGC(GCPtr pGC, unsigned long stateChanges, DrawablePtr pDrawable) {
    LORIE_GC_FUNC_PROLOGUE(pGC)
    (*pGC->funcs->ValidateGC) (pGC, stateChanges, pDrawable);
    LORIE_GC_FUNC_EPILOGUE(pGC)
}

static void lorieChangeGC(GCPtr pGC, unsigned long mask) {
    LORIE_GC_FUNC_PROLOGUE(pGC)
    (*pGC->funcs->ChangeGC) (pGC, mask);
    LORIE_GC_FUNC_EPILOGUE(pGC)
}

static void lorieCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst) {
    LORIE_GC_FUNC_PROLOGUE(pGCSrc)
    (*pGCSrc->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    LORIE_GC_FUNC_EPILOGUE(pGCSrc)
}

static void lorieDestroyGC(GCPtr pGC) {
    LORIE_GC_FUNC_PROLOGUE(pGC)
    (*pGC->funcs->DestroyGC) (pGC);
    LORIE_GC_FUNC_EPILOGUE(pGC)
}

static void lorieChangeClip(GCPtr pGC, int type, void *pvalue, int nrects) {
    LORIE_GC_FUNC_PROLOGUE(pGC)
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    LORIE_GC_FUNC_EPILOGUE(pGC)
}

static void lorieDestroyClip(GCPtr pGC) {
    LORIE_GC_FUNC_PROLOGUE(pGC)
    (*pGC->funcs->DestroyClip) (pGC);
    LORIE_GC_FUNC_EPILOGUE(pGC)
}

static void lorieCopyClip(GCPtr pgcDst, GCPtr pgcSrc) {
    LORIE_GC_FUNC_PROLOGUE(pgcDst)
    (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
    LORIE_GC_FUNC_EPILOGUE(pgcDst)
}

static const GCFuncs lorieGCFuncs = {
    lorieValidateGC, lorieChangeGC, lorieCopyGC, lorieDestroyGC,
    lorieChangeClip, lorieDestroyClip, lorieCopyClip
};

static void lorieFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nInit, DDXPointPtr pptInit, int * pwidthInit, int fSorted) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->FillSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void lorieSetSpans(DrawablePtr pDrawable, GCPtr pGC, char * psrc, DDXPointPtr ppt, int * pwidth, int nspans, int fSorted) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->SetSpans) (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePutImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y, int w, int h, int leftPad, int format, char * pBits) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PutImage) (pDrawable, pGC, depth, x, y, w, h, leftPad, format, pBits);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static RegionPtr lorieCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy, int w, int h, int dstx, int dsty) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pSrc, 0);
    loriePixPriv(pSrc, 0);
    loriePixFromDrawable(pDst, 1);
    loriePixPriv(pDst, 1);
    RegionPtr r = NULL;
    Bool wasLocked = TRUE;
    if (pPixPriv0 && !pPixPriv1) {
        wasLocked = pSrcPix0->devPrivate.ptr != NULL;
        if (!wasLocked) {
            void *addr = NULL;
            int error;
            if ((error = AHardwareBuffer_lock(pPixPriv0->buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &addr)) != 0)
                log(ERROR, "DRI3: AHardwareBuffer_lock failed: %d", error);
            if (addr)
                pSrc->pScreen->ModifyPixmapHeader(pSrcPix0, 0, 0, 0, 0, 0, addr);
        }
    }

    if (!pPixPriv1)
        r = (*pGC->ops->CopyArea) (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);

    if (!wasLocked) {
        AHardwareBuffer_unlock(pPixPriv0->buffer, NULL);
        pSrcPix0->devPrivate.ptr = NULL;
    }
    LORIE_GC_OP_EPILOGUE(pGC)
    return r;
}

static RegionPtr lorieCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC, int srcx, int srcy, int width, int height, int dstx, int dsty, unsigned long bitPlane) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pSrcDrawable, 0);
    loriePixFromDrawable(pDstDrawable, 1);
    loriePixPriv(pSrcDrawable, 0);
    loriePixPriv(pDstDrawable, 1);
    RegionPtr r = NULL;
    if (!pPixPriv0 && !pPixPriv1)
        r = (*pGC->ops->CopyPlane) (pSrcDrawable, pDstDrawable, pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
    LORIE_GC_OP_EPILOGUE(pGC)
    return r;
}

static void loriePolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pptInit);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, pptInit);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolySegment(DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment * pSegs) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolySegment) (pDrawable, pGC, nseg, pSegs);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nrects, xRectangle * pRects) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyRectangle) (pDrawable, pGC, nrects, pRects);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolyArc(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc * parcs) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyArc) (pDrawable, pGC, narcs, parcs);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void lorieFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode, int count, DDXPointPtr pPts) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, count, pPts);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nrectFill, xRectangle * prectInit) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrectFill, prectInit);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc * parcs) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, parcs);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static int loriePolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char * chars) {
    LORIE_GC_OP_PROLOGUE(pGC)
    int r = x;
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        r = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);
    LORIE_GC_OP_EPILOGUE(pGC)
    return r;
}

static int loriePolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short * chars) {
    LORIE_GC_OP_PROLOGUE(pGC)
    int r = x;
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        r = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);
    LORIE_GC_OP_EPILOGUE(pGC)
    return r;
}

static void lorieImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char * chars) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void lorieImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short * chars) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void lorieImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr *ppci, void *pglyphBase) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr *ppci, void *pglyphBase) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDrawable, 0);
    loriePixPriv(pDrawable, 0);
    if (!pPixPriv0)
        (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static void loriePushPixels(GCPtr pGC, PixmapPtr pBitMapPix0, DrawablePtr pDst, int w, int h, int x, int y) {
    LORIE_GC_OP_PROLOGUE(pGC)
    loriePixFromDrawable(pDst, 1);
    loriePixPriv(pBitMap, 0);
    loriePixPriv(pDst, 1);
    if (!pPixPriv0 && !pPixPriv1)
        (*pGC->ops->PushPixels) (pGC, pBitMapPix0, pDst, w, h, x, y);
    LORIE_GC_OP_EPILOGUE(pGC)
}

static const GCOps lorieGCOps = {
    lorieFillSpans, lorieSetSpans,
    loriePutImage, lorieCopyArea,
    lorieCopyPlane, loriePolyPoint,
    loriePolylines, loriePolySegment,
    loriePolyRectangle, loriePolyArc,
    lorieFillPolygon, loriePolyFillRect,
    loriePolyFillArc, loriePolyText8,
    loriePolyText16, lorieImageText8,
    lorieImageText16, lorieImageGlyphBlt,
    loriePolyGlyphBlt, loriePushPixels,
};

static Bool
lorieCreateGC(GCPtr pGC) {
    ScreenPtr pScreen = pGC->pScreen;

    lorieScrPriv(pScreen);
    lorieGCPriv(pGC);
    Bool ret;

    unwrap(pScrPriv, pScreen, CreateGC)
    if ((ret = (*pScreen->CreateGC) (pGC))) {
        pGCPriv->ops = pGC->ops;
        pGCPriv->funcs = pGC->funcs;
        pGC->funcs = &lorieGCFuncs;
        pGC->ops = &lorieGCOps;
    }
    wrap(pScrPriv, pScreen, CreateGC, lorieCreateGC)

    return ret;
}

static Bool
lorieDestroyPixmap(PixmapPtr pPixmap) {
    Bool ret;
    void *ptr = NULL;
    LorieAHBPixPrivPtr pPixPriv = NULL;
    size_t size = 0;
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    lorieScrPriv(pScreen);

    if (pPixmap->refcnt == 1 && pPixmap->drawable.width && pPixmap->drawable.height) {
        ptr = dixLookupPrivate(&pPixmap->devPrivates, &lorieMmappedPixPrivateKey);
        pPixPriv = dixLookupPrivate(&pPixmap->devPrivates, &lorieAHBPixPrivateKey);
        size = pPixmap->devKind * pPixmap->drawable.height;
    }

    unwrap(pScrPriv, pScreen, DestroyPixmap)
    ret = (*pScreen->DestroyPixmap) (pPixmap);
    wrap(pScrPriv, pScreen, DestroyPixmap, lorieDestroyPixmap)

    if (ptr)
        munmap(ptr, size);

    if (pPixPriv) {
        if (pPixPriv->buffer)
            AHardwareBuffer_release(pPixPriv->buffer);
        free(pPixPriv);
    }

    return ret;
}

static PixmapPtr loriePixmapFromFds(ScreenPtr screen, CARD8 num_fds, const int *fds, CARD16 width, CARD16 height,
                                    const CARD32 *strides, const CARD32 *offsets, CARD8 depth, __unused CARD8 bpp, CARD64 modifier) {
    const CARD64 AHARDWAREBUFFER_SOCKET_FD = 1255;
    const CARD64 RAW_MMAPPABLE_FD = 1274;
    PixmapPtr pixmap = NullPixmap;
    LorieAHBPixPrivPtr pPixPriv = NULL;
    if (num_fds > 1) {
        log(ERROR, "DRI3: More than 1 fd");
        return NULL;
    }

    if (modifier != RAW_MMAPPABLE_FD && modifier != AHARDWAREBUFFER_SOCKET_FD) {
        log(ERROR, "DRI3: Modifier is not RAW_MMAPPABLE_FD or AHARDWAREBUFFER_SOCKET_FD");
        return NULL;
    }

    if (modifier == RAW_MMAPPABLE_FD) {
        void *addr = mmap(NULL, strides[0] * height, PROT_READ, MAP_SHARED, fds[0], offsets[0]);
        if (!addr || addr == MAP_FAILED) {
            log(ERROR, "DRI3: RAW_MMAPPABLE_FD: mmap failed");
            return NULL;
        }

        pixmap = fbCreatePixmap(screen, 0, 0, depth, 0);
        if (!pixmap) {
            log(ERROR, "DRI3: RAW_MMAPPABLE_FD: failed to create pixmap");
            munmap(addr, strides[0] * height);
            return NULL;
        }

        dixSetPrivate(&pixmap->devPrivates, &lorieMmappedPixPrivateKey, addr);
        screen->ModifyPixmapHeader(pixmap, width, height, 0, 0, strides[0], addr);

        return pixmap;
    } else if (modifier == AHARDWAREBUFFER_SOCKET_FD) {
        AHardwareBuffer_Desc desc;
        struct stat info;
        int r;
        if (fstat(fds[0], &info) != 0) {
            log(ERROR, "DRI3: fstat failed: %s", strerror(errno));
            return NULL;
        }

        if (!S_ISSOCK(info.st_mode)) {
            log(ERROR, "DRI3: modifier is AHARDWAREBUFFER_SOCKET_FD but fd is not a socket");
            return NULL;
        }

        pPixPriv = calloc(1, sizeof(LorieAHBPixPrivRec));
        if (!pPixPriv) {
            log(ERROR, "DRI3: AHARDWAREBUFFER_SOCKET_FD: failed to allocate LorieAHBPixPrivRec");
            return NULL;
        }

        pixmap = fbCreatePixmap(screen, 0, 0, depth, 0);
        if (!pixmap) {
            log(ERROR, "DRI3: failed to create pixmap");
            goto fail;
        }

        dixSetPrivate(&pixmap->devPrivates, &lorieAHBPixPrivateKey, pPixPriv);

        // Sending signal to other end of socket to send buffer.
        uint8_t buf = 1;
        if (write(fds[0], &buf, 1) != 1) {
            log(ERROR, "DRI3: AHARDWAREBUFFER_SOCKET_FD: failed to write to socket: %s", strerror(errno));
            goto fail;
        }

        if ((r = AHardwareBuffer_recvHandleFromUnixSocket(fds[0], &pPixPriv->buffer)) != 0) {
            log(ERROR, "DRI3: AHARDWAREBUFFER_SOCKET_FD: failed to obtain AHardwareBuffer from socket: %d", r);
            goto fail;
        }

        if (!pPixPriv->buffer) {
            log(ERROR, "DRI3: AHARDWAREBUFFER_SOCKET_FD: did not receive AHardwareSocket from buffer");
            goto fail;
        }

        AHardwareBuffer_describe(pPixPriv->buffer, &desc);
        if (desc.format != AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM
             && desc.format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
             && desc.format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM) {
            log(ERROR, "DRI3: AHARDWAREBUFFER_SOCKET_FD: wrong format of AHardwareBuffer. Must be one of: AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM (stands for 5).");
            goto fail;
        }

        pixmap->devPrivate.ptr = NULL;
        screen->ModifyPixmapHeader(pixmap, desc.width, desc.height, 0, 0, desc.stride * 4, NULL);
        return pixmap;
    }

    fail:
    if (pPixPriv) {
        if (pPixPriv->buffer)
            AHardwareBuffer_release(pPixPriv->buffer);
        free(pPixPriv);
    }

    if (pixmap)
        fbDestroyPixmap(pixmap);

    return NULL;
}

static int lorieGetFormats(__unused ScreenPtr screen, CARD32 *num_formats, CARD32 **formats) {
    *num_formats = 0;
    *formats = NULL;
    return TRUE;
}

static int lorieGetModifiers(__unused ScreenPtr screen, __unused uint32_t format, uint32_t *num_modifiers, uint64_t **modifiers) {
    *num_modifiers = 0;
    *modifiers = NULL;
    return TRUE;
}

static dri3_screen_info_rec dri3Info = {
    .version = 2,
    .fds_from_pixmap = FalseNoop,
    .pixmap_from_fds = loriePixmapFromFds,
    .get_formats = lorieGetFormats,
    .get_modifiers = lorieGetModifiers,
    .get_drawable_modifiers = FalseNoop
};

Bool lorieInitDri3(ScreenPtr pScreen) {
    LorieScrPrivPtr pScrPriv;

    if (!dixRegisterPrivateKey(&lorieScrPrivateKey, PRIVATE_SCREEN, 0))
        return FALSE;

    if (dixLookupPrivate(&pScreen->devPrivates, &lorieScrPrivateKey))
        return TRUE;

    if (!dixRegisterPrivateKey(&lorieGCPrivateKey, PRIVATE_GC, sizeof(LorieGCPrivRec))
     || !dixRegisterPrivateKey(&lorieAHBPixPrivateKey, PRIVATE_PIXMAP, 0)
     || !dixRegisterPrivateKey(&lorieMmappedPixPrivateKey, PRIVATE_PIXMAP, 0)
     || !dri3_screen_init(pScreen, &dri3Info))
        return FALSE;

    pScrPriv = malloc(sizeof(LorieScrPrivRec));
    if (!pScrPriv)
        return FALSE;

    wrap(pScrPriv, pScreen, CreateGC, lorieCreateGC)
    wrap(pScrPriv, pScreen, DestroyPixmap, lorieDestroyPixmap)

    dixSetPrivate(&pScreen->devPrivates, &lorieScrPrivateKey, pScrPriv);
    return TRUE;
}
