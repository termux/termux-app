/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright 2000 VA Linux Systems, Inc.
   Copyright (c) 2002-2012 Apple Computer, Inc.
   All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sub license, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial portions
   of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
   ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <sys/time.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#define _APPLEDRI_SERVER_
#include "appledristr.h"
#include "swaprep.h"
#include "dri.h"
#include "dristruct.h"
#include "mi.h"
#include "mipointer.h"
#include "rootless.h"
#include "rootlessCommon.h"
#include "x-hash.h"
#include "x-hook.h"
#include "driWrap.h"

static DevPrivateKeyRec DRIScreenPrivKeyRec;
#define DRIScreenPrivKey       (&DRIScreenPrivKeyRec)
static DevPrivateKeyRec DRIWindowPrivKeyRec;
#define DRIWindowPrivKey       (&DRIWindowPrivKeyRec)
static DevPrivateKeyRec DRIPixmapPrivKeyRec;
#define DRIPixmapPrivKey       (&DRIPixmapPrivKeyRec)
static DevPrivateKeyRec DRIPixmapBufferPrivKeyRec;
#define DRIPixmapBufferPrivKey (&DRIPixmapBufferPrivKeyRec)

static RESTYPE DRIDrawablePrivResType;

static x_hash_table *surface_hash;      /* maps surface ids -> drawablePrivs */

static Bool
DRIFreePixmapImp(DrawablePtr pDrawable);

typedef struct {
    DrawablePtr pDrawable;
    int refCount;
    int bytesPerPixel;
    int width;
    int height;
    char shmPath[PATH_MAX];
    int fd; /* From shm_open (for now) */
    size_t length; /* length of buffer */
    void *buffer;
} DRIPixmapBuffer, *DRIPixmapBufferPtr;

Bool
DRIScreenInit(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv;
    int i;

    if (!dixRegisterPrivateKey(&DRIScreenPrivKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&DRIWindowPrivKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&DRIPixmapPrivKeyRec, PRIVATE_PIXMAP, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&DRIPixmapBufferPrivKeyRec, PRIVATE_PIXMAP, 0))
        return FALSE;

    pDRIPriv = (DRIScreenPrivPtr)calloc(1, sizeof(DRIScreenPrivRec));
    if (!pDRIPriv) {
        dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
        return FALSE;
    }

    dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, pDRIPriv);
    pDRIPriv->directRenderingSupport = TRUE;
    pDRIPriv->nrWindows = 0;

    /* Initialize drawable tables */
    for (i = 0; i < DRI_MAX_DRAWABLES; i++) {
        pDRIPriv->DRIDrawables[i] = NULL;
    }

    return TRUE;
}

Bool
DRIFinishScreenInit(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    /* Wrap DRI support */
    pDRIPriv->wrap.CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = DRICopyWindow;

    pDRIPriv->wrap.ClipNotify = pScreen->ClipNotify;
    pScreen->ClipNotify = DRIClipNotify;

    //    ErrorF("[DRI] screen %d installation complete\n", pScreen->myNum);

    return DRIWrapInit(pScreen);
}

void
DRICloseScreen(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv && pDRIPriv->directRenderingSupport) {
        free(pDRIPriv);
        dixSetPrivate(&pScreen->devPrivates, DRIScreenPrivKey, NULL);
    }
}

Bool
DRIExtensionInit(void)
{
    DRIDrawablePrivResType = CreateNewResourceType(DRIDrawablePrivDelete,
                                                   "DRIDrawable");

    return DRIDrawablePrivResType != 0;
}

void
DRIReset(void)
{
    /*
     * This stub routine is called when the X Server recycles, resources
     * allocated by DRIExtensionInit need to be managed here.
     *
     * Currently this routine is a stub because all the interesting resources
     * are managed via the screen init process.
     */
}

Bool
DRIQueryDirectRenderingCapable(ScreenPtr pScreen, Bool* isCapable)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv)
        *isCapable = pDRIPriv->directRenderingSupport;
    else
        *isCapable = FALSE;

    return TRUE;
}

Bool
DRIAuthConnection(ScreenPtr pScreen, unsigned int magic)
{
#if 0
    /* FIXME: something? */

    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (drmAuthMagic(pDRIPriv->drmFD, magic)) return FALSE;
#endif
    return TRUE;
}

static void
DRIUpdateSurface(DRIDrawablePrivPtr pDRIDrawablePriv, DrawablePtr pDraw)
{
    xp_window_changes wc;
    unsigned int flags = 0;

    if (pDRIDrawablePriv->sid == 0)
        return;

    wc.depth = (pDraw->bitsPerPixel == 32 ? XP_DEPTH_ARGB8888
                : pDraw->bitsPerPixel == 16 ? XP_DEPTH_RGB555 : XP_DEPTH_NIL);
    if (wc.depth != XP_DEPTH_NIL)
        flags |= XP_DEPTH;

    if (pDraw->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr)pDraw;
        WindowPtr pTopWin = TopLevelParent(pWin);

        wc.x = pWin->drawable.x - (pTopWin->drawable.x - pTopWin->borderWidth);
        wc.y = pWin->drawable.y - (pTopWin->drawable.y - pTopWin->borderWidth);
        wc.width = pWin->drawable.width + 2 * pWin->borderWidth;
        wc.height = pWin->drawable.height + 2 * pWin->borderWidth;
        wc.bit_gravity = XP_GRAVITY_NONE;

        wc.shape_nrects = RegionNumRects(&pWin->clipList);
        wc.shape_rects = RegionRects(&pWin->clipList);
        wc.shape_tx = -(pTopWin->drawable.x - pTopWin->borderWidth);
        wc.shape_ty = -(pTopWin->drawable.y - pTopWin->borderWidth);

        flags |= XP_BOUNDS | XP_SHAPE;

    }
    else if (pDraw->type == DRAWABLE_PIXMAP) {
        wc.x = 0;
        wc.y = 0;
        wc.width = pDraw->width;
        wc.height = pDraw->height;
        wc.bit_gravity = XP_GRAVITY_NONE;
        flags |= XP_BOUNDS;
    }

    xp_configure_surface(pDRIDrawablePriv->sid, flags, &wc);
}

/* Return NULL if an error occurs. */
static DRIDrawablePrivPtr
CreateSurfaceForWindow(ScreenPtr pScreen, WindowPtr pWin,
                       xp_window_id *widPtr)
{
    DRIDrawablePrivPtr pDRIDrawablePriv;
    xp_window_id wid = 0;

    *widPtr = 0;

    pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if (pDRIDrawablePriv == NULL) {
        xp_error err;
        xp_window_changes wc;

        /* allocate a DRI Window Private record */
        if (!(pDRIDrawablePriv = malloc(sizeof(*pDRIDrawablePriv)))) {
            return NULL;
        }

        pDRIDrawablePriv->pDraw = (DrawablePtr)pWin;
        pDRIDrawablePriv->pScreen = pScreen;
        pDRIDrawablePriv->refCount = 0;
        pDRIDrawablePriv->drawableIndex = -1;
        pDRIDrawablePriv->notifiers = NULL;

        /* find the physical window */
        wid = x_cvt_vptr_to_uint(RootlessFrameForWindow(pWin, TRUE));

        if (wid == 0) {
            free(pDRIDrawablePriv);
            return NULL;
        }

        /* allocate the physical surface */
        err = xp_create_surface(wid, &pDRIDrawablePriv->sid);

        if (err != Success) {
            free(pDRIDrawablePriv);
            return NULL;
        }

        /* Make it visible */
        wc.stack_mode = XP_MAPPED_ABOVE;
        wc.sibling = 0;
        err = xp_configure_surface(pDRIDrawablePriv->sid, XP_STACKING, &wc);

        if (err != Success) {
            xp_destroy_surface(pDRIDrawablePriv->sid);
            free(pDRIDrawablePriv);
            return NULL;
        }

        /* save private off of preallocated index */
        dixSetPrivate(&pWin->devPrivates, DRIWindowPrivKey,
                      pDRIDrawablePriv);
    }

    *widPtr = wid;

    return pDRIDrawablePriv;
}

/* Return NULL if an error occurs. */
static DRIDrawablePrivPtr
CreateSurfaceForPixmap(ScreenPtr pScreen, PixmapPtr pPix)
{
    DRIDrawablePrivPtr pDRIDrawablePriv;

    pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_PIXMAP(pPix);

    if (pDRIDrawablePriv == NULL) {
        xp_error err;

        /* allocate a DRI Window Private record */
        if (!(pDRIDrawablePriv = calloc(1, sizeof(*pDRIDrawablePriv)))) {
            return NULL;
        }

        pDRIDrawablePriv->pDraw = (DrawablePtr)pPix;
        pDRIDrawablePriv->pScreen = pScreen;
        pDRIDrawablePriv->refCount = 0;
        pDRIDrawablePriv->drawableIndex = -1;
        pDRIDrawablePriv->notifiers = NULL;

        /* Passing a null window id to Xplugin in 10.3+ asks for
           an accelerated offscreen surface. */

        err = xp_create_surface(0, &pDRIDrawablePriv->sid);
        if (err != Success) {
            free(pDRIDrawablePriv);
            return NULL;
        }

        /*
         * The DRIUpdateSurface will be called to resize the surface
         * after this function, if the export is successful.
         */

        /* save private off of preallocated index */
        dixSetPrivate(&pPix->devPrivates, DRIPixmapPrivKey,
                      pDRIDrawablePriv);
    }

    return pDRIDrawablePriv;
}

Bool
DRICreateSurface(ScreenPtr pScreen, Drawable id,
                 DrawablePtr pDrawable, xp_client_id client_id,
                 xp_surface_id *surface_id, unsigned int ret_key[2],
                 void (*notify)(void *arg, void *data), void *notify_data)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    xp_window_id wid = 0;
    DRIDrawablePrivPtr pDRIDrawablePriv;

    if (pDrawable->type == DRAWABLE_WINDOW) {
        /* <rdar://problem/12338921>
         * http://bugs.winehq.org/show_bug.cgi?id=31751
         */
        RootlessStopDrawing((WindowPtr)pDrawable, FALSE);

        pDRIDrawablePriv = CreateSurfaceForWindow(pScreen,
                                                  (WindowPtr)pDrawable, &wid);

        if (NULL == pDRIDrawablePriv)
            return FALSE;  /*error*/
    } else if (pDrawable->type == DRAWABLE_PIXMAP) {
        pDRIDrawablePriv = CreateSurfaceForPixmap(pScreen,
                                                  (PixmapPtr)pDrawable);

        if (NULL == pDRIDrawablePriv)
            return FALSE;  /*error*/
    } else {
        /* We handle GLXPbuffers in a different way (via CGL). */
        return FALSE;
    }

    /* Finish initialization of new surfaces */
    if (pDRIDrawablePriv->refCount == 0) {
        unsigned int key[2] = { 0 };
        xp_error err;

        /* try to give the client access to the surface */
        if (client_id != 0) {
            /*
             * Xplugin accepts a 0 wid if the surface id is offscreen, such
             * as for a pixmap.
             */
            err = xp_export_surface(wid, pDRIDrawablePriv->sid,
                                    client_id, key);
            if (err != Success) {
                xp_destroy_surface(pDRIDrawablePriv->sid);
                free(pDRIDrawablePriv);

                /*
                 * Now set the dix privates to NULL that were previously set.
                 * This prevents reusing an invalid pointer.
                 */
                if (pDrawable->type == DRAWABLE_WINDOW) {
                    WindowPtr pWin = (WindowPtr)pDrawable;

                    dixSetPrivate(&pWin->devPrivates, DRIWindowPrivKey, NULL);
                }
                else if (pDrawable->type == DRAWABLE_PIXMAP) {
                    PixmapPtr pPix = (PixmapPtr)pDrawable;

                    dixSetPrivate(&pPix->devPrivates, DRIPixmapPrivKey, NULL);
                }

                return FALSE;
            }
        }

        pDRIDrawablePriv->key[0] = key[0];
        pDRIDrawablePriv->key[1] = key[1];

        ++pDRIPriv->nrWindows;

        /* and stash it by surface id */
        if (surface_hash == NULL)
            surface_hash = x_hash_table_new(NULL, NULL, NULL, NULL);
        x_hash_table_insert(surface_hash,
                            x_cvt_uint_to_vptr(
                                pDRIDrawablePriv->sid), pDRIDrawablePriv);

        /* track this in case this window is destroyed */
        AddResource(id, DRIDrawablePrivResType, (void *)pDrawable);

        /* Initialize shape */
        DRIUpdateSurface(pDRIDrawablePriv, pDrawable);
    }

    pDRIDrawablePriv->refCount++;

    *surface_id = pDRIDrawablePriv->sid;

    if (ret_key != NULL) {
        ret_key[0] = pDRIDrawablePriv->key[0];
        ret_key[1] = pDRIDrawablePriv->key[1];
    }

    if (notify != NULL) {
        pDRIDrawablePriv->notifiers = x_hook_add(pDRIDrawablePriv->notifiers,
                                                 notify, notify_data);
    }

    return TRUE;
}

Bool
DRIDestroySurface(ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable,
                  void (*notify)(void *, void *), void *notify_data)
{
    DRIDrawablePrivPtr pDRIDrawablePriv;

    if (pDrawable->type == DRAWABLE_WINDOW) {
        pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW((WindowPtr)pDrawable);
    }
    else if (pDrawable->type == DRAWABLE_PIXMAP) {
        pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_PIXMAP((PixmapPtr)pDrawable);
    }
    else {
        return FALSE;
    }

    if (pDRIDrawablePriv != NULL) {
        /*
         * This doesn't seem to be used, because notify is NULL in all callers.
         */

        if (notify != NULL) {
            pDRIDrawablePriv->notifiers = x_hook_remove(
                pDRIDrawablePriv->notifiers,
                notify, notify_data);
        }

        --pDRIDrawablePriv->refCount;

        /*
         * Check if the drawable privates still have a reference to the
         * surface.
         */

        if (pDRIDrawablePriv->refCount <= 0) {
            /*
             * This calls back to DRIDrawablePrivDelete which
             * frees the private area and dispatches events, if needed.
             */
            FreeResourceByType(id, DRIDrawablePrivResType, FALSE);
        }
    }

    return TRUE;
}

/*
 * The assumption is that this is called when the refCount of a surface
 * drops to <= 0, or the window/pixmap is destroyed.
 */
Bool
DRIDrawablePrivDelete(void *pResource, XID id)
{
    DrawablePtr pDrawable = (DrawablePtr)pResource;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pDrawable->pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv = NULL;
    WindowPtr pWin = NULL;
    PixmapPtr pPix = NULL;

    if (pDrawable->type == DRAWABLE_WINDOW) {
        pWin = (WindowPtr)pDrawable;
        pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
    }
    else if (pDrawable->type == DRAWABLE_PIXMAP) {
        pPix = (PixmapPtr)pDrawable;
        pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_PIXMAP(pPix);
    }

    if (pDRIDrawablePriv == NULL) {
        /*
         * We reuse __func__ and the resource type for the GLXPixmap code.
         * Attempt to free a pixmap buffer associated with the resource
         * if possible.
         */
        return DRIFreePixmapImp(pDrawable);
    }

    if (pDRIDrawablePriv->drawableIndex != -1) {
        /* release drawable table entry */
        pDRIPriv->DRIDrawables[pDRIDrawablePriv->drawableIndex] = NULL;
    }

    if (pDRIDrawablePriv->sid != 0) {
        DRISurfaceNotify(pDRIDrawablePriv->sid,
                         AppleDRISurfaceNotifyDestroyed);
    }

    if (pDRIDrawablePriv->notifiers != NULL)
        x_hook_free(pDRIDrawablePriv->notifiers);

    free(pDRIDrawablePriv);

    if (pDrawable->type == DRAWABLE_WINDOW) {
        dixSetPrivate(&pWin->devPrivates, DRIWindowPrivKey, NULL);
    }
    else if (pDrawable->type == DRAWABLE_PIXMAP) {
        dixSetPrivate(&pPix->devPrivates, DRIPixmapPrivKey, NULL);
    }

    --pDRIPriv->nrWindows;

    return TRUE;
}

void
DRICopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv;

    if (pDRIPriv->nrWindows > 0) {
        pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
        if (pDRIDrawablePriv != NULL) {
            DRIUpdateSurface(pDRIDrawablePriv, &pWin->drawable);
        }
    }

    /* unwrap */
    pScreen->CopyWindow = pDRIPriv->wrap.CopyWindow;

    /* call lower layers */
    (*pScreen->CopyWindow)(pWin, ptOldOrg, prgnSrc);

    /* rewrap */
    pScreen->CopyWindow = DRICopyWindow;
}

void
DRIClipNotify(WindowPtr pWin, int dx, int dy)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv;

    if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {
        DRIUpdateSurface(pDRIDrawablePriv, &pWin->drawable);
    }

    if (pDRIPriv->wrap.ClipNotify) {
        pScreen->ClipNotify = pDRIPriv->wrap.ClipNotify;

        (*pScreen->ClipNotify)(pWin, dx, dy);

        pScreen->ClipNotify = DRIClipNotify;
    }
}

/* This lets us get at the unwrapped functions so that they can correctly
 * call the lower level functions, and choose whether they will be
 * called at every level of recursion (eg in validatetree).
 */
DRIWrappedFuncsRec *
DRIGetWrappedFuncs(ScreenPtr pScreen)
{
    return &(DRI_SCREEN_PRIV(pScreen)->wrap);
}

void
DRIQueryVersion(int *majorVersion,
                int *minorVersion,
                int *patchVersion)
{
    *majorVersion = APPLE_DRI_MAJOR_VERSION;
    *minorVersion = APPLE_DRI_MINOR_VERSION;
    *patchVersion = APPLE_DRI_PATCH_VERSION;
}

/*
 * Note: this also cleans up the hash table in addition to notifying clients.
 * The sid/surface-id should not be used after this, because it will be
 * invalid.
 */
void
DRISurfaceNotify(xp_surface_id id, int kind)
{
    DRIDrawablePrivPtr pDRIDrawablePriv = NULL;
    DRISurfaceNotifyArg arg;

    arg.id = id;
    arg.kind = kind;

    if (surface_hash != NULL) {
        pDRIDrawablePriv = x_hash_table_lookup(surface_hash,
                                               x_cvt_uint_to_vptr(id), NULL);
    }

    if (pDRIDrawablePriv == NULL)
        return;

    if (kind == AppleDRISurfaceNotifyDestroyed) {
        x_hash_table_remove(surface_hash, x_cvt_uint_to_vptr(id));
    }

    x_hook_run(pDRIDrawablePriv->notifiers, &arg);

    if (kind == AppleDRISurfaceNotifyDestroyed) {
        xp_error error;

        error = xp_destroy_surface(pDRIDrawablePriv->sid);

        if (error)
            ErrorF("%s: xp_destroy_surface failed: %d\n", __func__, error);

        /* Guard against reuse, even though we are freeing after this. */
        pDRIDrawablePriv->sid = 0;

        FreeResourceByType(pDRIDrawablePriv->pDraw->id,
                           DRIDrawablePrivResType, FALSE);
    }
}

/*
 * This creates a shared memory buffer for use with GLXPixmaps
 * and AppleSGLX.
 */
Bool
DRICreatePixmap(ScreenPtr pScreen, Drawable id,
                DrawablePtr pDrawable, char *path,
                size_t pathmax)
{
    DRIPixmapBufferPtr shared;
    PixmapPtr pPix;

    if (pDrawable->type != DRAWABLE_PIXMAP)
        return FALSE;

    pPix = (PixmapPtr)pDrawable;

    shared = malloc(sizeof(*shared));
    if (NULL == shared) {
        FatalError("failed to allocate DRIPixmapBuffer in %s\n", __func__);
    }

    shared->pDrawable = pDrawable;
    shared->refCount = 1;

    if (pDrawable->bitsPerPixel >= 24) {
        shared->bytesPerPixel = 4;
    }
    else if (pDrawable->bitsPerPixel <= 16) {
        shared->bytesPerPixel = 2;
    }

    shared->width = pDrawable->width;
    shared->height = pDrawable->height;

    if (-1 == snprintf(shared->shmPath, sizeof(shared->shmPath),
                       "%d_0x%lx", getpid(),
                       (unsigned long)id)) {
        FatalError("buffer overflow in %s\n", __func__);
    }

    shared->fd = shm_open(shared->shmPath,
                          O_RDWR | O_EXCL | O_CREAT,
                          S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);

    if (-1 == shared->fd) {
        free(shared);
        return FALSE;
    }

    shared->length = shared->width * shared->height * shared->bytesPerPixel;

    if (-1 == ftruncate(shared->fd, shared->length)) {
        ErrorF("failed to ftruncate (extend) file.");
        shm_unlink(shared->shmPath);
        close(shared->fd);
        free(shared);
        return FALSE;
    }

    shared->buffer = mmap(NULL, shared->length,
                          PROT_READ | PROT_WRITE,
                          MAP_FILE | MAP_SHARED, shared->fd, 0);

    if (MAP_FAILED == shared->buffer) {
        ErrorF("failed to mmap shared memory.");
        shm_unlink(shared->shmPath);
        close(shared->fd);
        free(shared);
        return FALSE;
    }

    strlcpy(path, shared->shmPath, pathmax);

    dixSetPrivate(&pPix->devPrivates, DRIPixmapBufferPrivKey, shared);

    AddResource(id, DRIDrawablePrivResType, (void *)pDrawable);

    return TRUE;
}

Bool
DRIGetPixmapData(DrawablePtr pDrawable, int *width, int *height,
                 int *pitch, int *bpp, void **ptr)
{
    PixmapPtr pPix;
    DRIPixmapBufferPtr shared;

    if (pDrawable->type != DRAWABLE_PIXMAP)
        return FALSE;

    pPix = (PixmapPtr)pDrawable;

    shared = dixLookupPrivate(&pPix->devPrivates, DRIPixmapBufferPrivKey);

    if (NULL == shared)
        return FALSE;

    assert(pDrawable->width == shared->width);
    assert(pDrawable->height == shared->height);

    *width = shared->width;
    *height = shared->height;
    *bpp = shared->bytesPerPixel;
    *pitch = shared->width * shared->bytesPerPixel;
    *ptr = shared->buffer;

    return TRUE;
}

static Bool
DRIFreePixmapImp(DrawablePtr pDrawable)
{
    DRIPixmapBufferPtr shared;
    PixmapPtr pPix;

    if (pDrawable->type != DRAWABLE_PIXMAP)
        return FALSE;

    pPix = (PixmapPtr)pDrawable;

    shared = dixLookupPrivate(&pPix->devPrivates, DRIPixmapBufferPrivKey);

    if (NULL == shared)
        return FALSE;

    close(shared->fd);
    munmap(shared->buffer, shared->length);
    shm_unlink(shared->shmPath);
    free(shared);

    dixSetPrivate(&pPix->devPrivates, DRIPixmapBufferPrivKey, (void *)NULL);

    return TRUE;
}

void
DRIDestroyPixmap(DrawablePtr pDrawable)
{
    if (DRIFreePixmapImp(pDrawable))
        FreeResourceByType(pDrawable->id, DRIDrawablePrivResType, FALSE);

}
