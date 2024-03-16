/************************************************************

Copyright 1989, 1998  The Open Group

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

********************************************************/

/* THIS IS NOT AN X CONSORTIUM STANDARD OR AN X PROJECT TEAM SPECIFICATION */

#define SHM

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef HAVE_MEMFD_CREATE
#include <sys/mman.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "extnsionst.h"
#include "servermd.h"
#include "shmint.h"
#include "xace.h"
#include <X11/extensions/shmproto.h>
#include <X11/Xfuncproto.h>
#include <sys/mman.h>
#include "protocol-versions.h"
#include "busfault.h"

/* Needed for Solaris cross-zone shared memory extension */
#ifdef HAVE_SHMCTL64
#include <sys/ipc_impl.h>
#define SHMSTAT(id, buf)	shmctl64(id, IPC_STAT64, buf)
#define SHMSTAT_TYPE 		struct shmid_ds64
#define SHMPERM_TYPE 		struct ipc_perm64
#define SHM_PERM(buf) 		buf.shmx_perm
#define SHM_SEGSZ(buf)		buf.shmx_segsz
#define SHMPERM_UID(p)		p->ipcx_uid
#define SHMPERM_CUID(p)		p->ipcx_cuid
#define SHMPERM_GID(p)		p->ipcx_gid
#define SHMPERM_CGID(p)		p->ipcx_cgid
#define SHMPERM_MODE(p)		p->ipcx_mode
#define SHMPERM_ZONEID(p)	p->ipcx_zoneid
#else
#define SHMSTAT(id, buf) 	shmctl(id, IPC_STAT, buf)
#define SHMSTAT_TYPE 		struct shmid_ds
#define SHMPERM_TYPE 		struct ipc_perm
#define SHM_PERM(buf) 		buf.shm_perm
#define SHM_SEGSZ(buf)		buf.shm_segsz
#define SHMPERM_UID(p)		p->uid
#define SHMPERM_CUID(p)		p->cuid
#define SHMPERM_GID(p)		p->gid
#define SHMPERM_CGID(p)		p->cgid
#define SHMPERM_MODE(p)		p->mode
#endif

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

#include "extinit.h"

typedef struct _ShmScrPrivateRec {
    CloseScreenProcPtr CloseScreen;
    ShmFuncsPtr shmFuncs;
    DestroyPixmapProcPtr destroyPixmap;
} ShmScrPrivateRec;

static PixmapPtr fbShmCreatePixmap(XSHM_CREATE_PIXMAP_ARGS);
static int ShmDetachSegment(void *value, XID shmseg);
static void ShmResetProc(ExtensionEntry *extEntry);
static void SShmCompletionEvent(xShmCompletionEvent *from,
                                xShmCompletionEvent *to);

static Bool ShmDestroyPixmap(PixmapPtr pPixmap);

static unsigned char ShmReqCode;
int ShmCompletionCode;
int BadShmSegCode;
RESTYPE ShmSegType;
static ShmDescPtr Shmsegs;
static Bool sharedPixmaps;
static DevPrivateKeyRec shmScrPrivateKeyRec;

#define shmScrPrivateKey (&shmScrPrivateKeyRec)
static DevPrivateKeyRec shmPixmapPrivateKeyRec;

#define shmPixmapPrivateKey (&shmPixmapPrivateKeyRec)
static ShmFuncs miFuncs = { NULL, NULL };
static ShmFuncs fbFuncs = { fbShmCreatePixmap, NULL };

#define ShmGetScreenPriv(s) ((ShmScrPrivateRec *)dixLookupPrivate(&(s)->devPrivates, shmScrPrivateKey))

#define VERIFY_SHMSEG(shmseg,shmdesc,client) \
{ \
    int tmprc; \
    tmprc = dixLookupResourceByType((void **)&(shmdesc), shmseg, ShmSegType, \
                                    client, DixReadAccess); \
    if (tmprc != Success) \
	return tmprc; \
}

#define VERIFY_SHMPTR(shmseg,offset,needwrite,shmdesc,client) \
{ \
    VERIFY_SHMSEG(shmseg, shmdesc, client); \
    if ((offset & 3) || (offset > shmdesc->size)) \
    { \
	client->errorValue = offset; \
	return BadValue; \
    } \
    if (needwrite && !shmdesc->writable) \
	return BadAccess; \
}

#define VERIFY_SHMSIZE(shmdesc,offset,len,client) \
{ \
    if ((offset + len) > shmdesc->size) \
    { \
	return BadAccess; \
    } \
}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__CYGWIN__) || defined(__DragonFly__)

static Bool badSysCall = FALSE;

static void
SigSysHandler(int signo)
{
    badSysCall = TRUE;
}

static Bool
CheckForShmSyscall(void)
{
    void (*oldHandler) (int);
    int shmid = -1;

    /* If no SHM support in the kernel, the bad syscall will generate SIGSYS */
    oldHandler = OsSignal(SIGSYS, SigSysHandler);

    badSysCall = FALSE;
    shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT);

    if (shmid != -1) {
        /* Successful allocation - clean up */
        shmctl(shmid, IPC_RMID, NULL);
    }
    else {
        /* Allocation failed */
        badSysCall = TRUE;
    }
    OsSignal(SIGSYS, oldHandler);
    return !badSysCall;
}

#define MUST_CHECK_FOR_SHM_SYSCALL

#endif

const DevPrivateKey ShmGetDevPrivateKeyRec(void) {
    return shmPixmapPrivateKey;
}

static Bool
ShmCloseScreen(ScreenPtr pScreen)
{
    ShmScrPrivateRec *screen_priv = ShmGetScreenPriv(pScreen);

    pScreen->CloseScreen = screen_priv->CloseScreen;
    dixSetPrivate(&pScreen->devPrivates, shmScrPrivateKey, NULL);
    free(screen_priv);
    return (*pScreen->CloseScreen) (pScreen);
}

static ShmScrPrivateRec *
ShmInitScreenPriv(ScreenPtr pScreen)
{
    ShmScrPrivateRec *screen_priv = ShmGetScreenPriv(pScreen);

    if (!screen_priv) {
        screen_priv = calloc(1, sizeof(ShmScrPrivateRec));
        screen_priv->CloseScreen = pScreen->CloseScreen;
        dixSetPrivate(&pScreen->devPrivates, shmScrPrivateKey, screen_priv);
        pScreen->CloseScreen = ShmCloseScreen;
    }
    return screen_priv;
}

static Bool
ShmRegisterPrivates(void)
{
    if (!dixRegisterPrivateKey(&shmScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&shmPixmapPrivateKeyRec, PRIVATE_PIXMAP, 0))
        return FALSE;
    return TRUE;
}

 /*ARGSUSED*/ static void
ShmResetProc(ExtensionEntry * extEntry)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++)
        ShmRegisterFuncs(screenInfo.screens[i], NULL);
}

void
ShmRegisterFuncs(ScreenPtr pScreen, ShmFuncsPtr funcs)
{
    if (!ShmRegisterPrivates())
        return;
    ShmInitScreenPriv(pScreen)->shmFuncs = funcs;
}

static Bool
ShmDestroyPixmap(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ShmScrPrivateRec *screen_priv = ShmGetScreenPriv(pScreen);
    void *shmdesc = NULL;
    Bool ret;

    if (pPixmap->refcnt == 1)
        shmdesc = dixLookupPrivate(&pPixmap->devPrivates, shmPixmapPrivateKey);

    pScreen->DestroyPixmap = screen_priv->destroyPixmap;
    ret = (*pScreen->DestroyPixmap) (pPixmap);
    screen_priv->destroyPixmap = pScreen->DestroyPixmap;
    pScreen->DestroyPixmap = ShmDestroyPixmap;

    if (shmdesc)
	ShmDetachSegment(shmdesc, 0);

    return ret;
}

void
ShmRegisterFbFuncs(ScreenPtr pScreen)
{
    ShmRegisterFuncs(pScreen, &fbFuncs);
}

static int
ProcShmQueryVersion(ClientPtr client)
{
    xShmQueryVersionReply rep = {
        .type = X_Reply,
        .sharedPixmaps = sharedPixmaps,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_SHM_MAJOR_VERSION,
        .minorVersion = SERVER_SHM_MINOR_VERSION,
        .uid = geteuid(),
        .gid = getegid(),
        .pixmapFormat = sharedPixmaps ? ZPixmap : 0
    };

    REQUEST_SIZE_MATCH(xShmQueryVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
        swaps(&rep.uid);
        swaps(&rep.gid);
    }
    WriteToClient(client, sizeof(xShmQueryVersionReply), &rep);
    return Success;
}

/*
 * Simulate the access() system call for a shared memory segment,
 * using the credentials from the client if available.
 */
static int
shm_access(ClientPtr client, SHMPERM_TYPE * perm, int readonly)
{
    int uid, gid;
    mode_t mask;
    int uidset = 0, gidset = 0;
    LocalClientCredRec *lcc;

    if (GetLocalClientCreds(client, &lcc) != -1) {

        if (lcc->fieldsSet & LCC_UID_SET) {
            uid = lcc->euid;
            uidset = 1;
        }
        if (lcc->fieldsSet & LCC_GID_SET) {
            gid = lcc->egid;
            gidset = 1;
        }

#if defined(HAVE_GETZONEID) && defined(SHMPERM_ZONEID)
        if (((lcc->fieldsSet & LCC_ZID_SET) == 0) || (lcc->zoneid == -1)
            || (lcc->zoneid != SHMPERM_ZONEID(perm))) {
            uidset = 0;
            gidset = 0;
        }
#endif
        FreeLocalClientCreds(lcc);

        if (uidset) {
            /* User id 0 always gets access */
            if (uid == 0) {
                return 0;
            }
            /* Check the owner */
            if (SHMPERM_UID(perm) == uid || SHMPERM_CUID(perm) == uid) {
                mask = S_IRUSR;
                if (!readonly) {
                    mask |= S_IWUSR;
                }
                return (SHMPERM_MODE(perm) & mask) == mask ? 0 : -1;
            }
        }

        if (gidset) {
            /* Check the group */
            if (SHMPERM_GID(perm) == gid || SHMPERM_CGID(perm) == gid) {
                mask = S_IRGRP;
                if (!readonly) {
                    mask |= S_IWGRP;
                }
                return (SHMPERM_MODE(perm) & mask) == mask ? 0 : -1;
            }
        }
    }
    /* Otherwise, check everyone else */
    mask = S_IROTH;
    if (!readonly) {
        mask |= S_IWOTH;
    }
    return (SHMPERM_MODE(perm) & mask) == mask ? 0 : -1;
}

static int
ProcShmAttach(ClientPtr client)
{
    SHMSTAT_TYPE buf;
    ShmDescPtr shmdesc;

    REQUEST(xShmAttachReq);

    REQUEST_SIZE_MATCH(xShmAttachReq);
    LEGAL_NEW_RESOURCE(stuff->shmseg, client);
    if ((stuff->readOnly != xTrue) && (stuff->readOnly != xFalse)) {
        client->errorValue = stuff->readOnly;
        return BadValue;
    }
    for (shmdesc = Shmsegs; shmdesc; shmdesc = shmdesc->next) {
        if (!SHMDESC_IS_FD(shmdesc) && shmdesc->shmid == stuff->shmid)
            break;
    }
    if (shmdesc) {
        if (!stuff->readOnly && !shmdesc->writable)
            return BadAccess;
        shmdesc->refcnt++;
    }
    else {
        shmdesc = malloc(sizeof(ShmDescRec));
        if (!shmdesc)
            return BadAlloc;
#ifdef SHM_FD_PASSING
        shmdesc->is_fd = FALSE;
#endif
        shmdesc->addr = shmat(stuff->shmid, 0,
                              stuff->readOnly ? SHM_RDONLY : 0);
        if ((shmdesc->addr == ((char *) -1)) || SHMSTAT(stuff->shmid, &buf)) {
            free(shmdesc);
            return BadAccess;
        }

        /* The attach was performed with root privs. We must
         * do manual checking of access rights for the credentials
         * of the client */

        if (shm_access(client, &(SHM_PERM(buf)), stuff->readOnly) == -1) {
            shmdt(shmdesc->addr);
            free(shmdesc);
            return BadAccess;
        }

        shmdesc->shmid = stuff->shmid;
        shmdesc->refcnt = 1;
        shmdesc->writable = !stuff->readOnly;
        shmdesc->size = SHM_SEGSZ(buf);
        shmdesc->next = Shmsegs;
        Shmsegs = shmdesc;
    }
    if (!AddResource(stuff->shmseg, ShmSegType, (void *) shmdesc))
        return BadAlloc;
    return Success;
}

 /*ARGSUSED*/ static int
ShmDetachSegment(void *value, /* must conform to DeleteType */
                 XID unused)
{
    ShmDescPtr shmdesc = (ShmDescPtr) value;
    ShmDescPtr *prev;

    if (--shmdesc->refcnt)
        return TRUE;
#if SHM_FD_PASSING
    if (shmdesc->is_fd) {
        if (shmdesc->busfault)
            busfault_unregister(shmdesc->busfault);
        munmap(shmdesc->addr, shmdesc->size);
    } else
#endif
        shmdt(shmdesc->addr);
    for (prev = &Shmsegs; *prev != shmdesc; prev = &(*prev)->next);
    *prev = shmdesc->next;
    free(shmdesc);
    return Success;
}

static int
ProcShmDetach(ClientPtr client)
{
    ShmDescPtr shmdesc;

    REQUEST(xShmDetachReq);

    REQUEST_SIZE_MATCH(xShmDetachReq);
    VERIFY_SHMSEG(stuff->shmseg, shmdesc, client);
    FreeResource(stuff->shmseg, RT_NONE);
    return Success;
}

/*
 * If the given request doesn't exactly match PutImage's constraints,
 * wrap the image in a scratch pixmap header and let CopyArea sort it out.
 */
static void
doShmPutImage(DrawablePtr dst, GCPtr pGC,
              int depth, unsigned int format,
              int w, int h, int sx, int sy, int sw, int sh, int dx, int dy,
              char *data)
{
    PixmapPtr pPixmap;

    if (format == ZPixmap || (format == XYPixmap && depth == 1)) {
        pPixmap = GetScratchPixmapHeader(dst->pScreen, w, h, depth,
                                         BitsPerPixel(depth),
                                         PixmapBytePad(w, depth), data);
        if (!pPixmap)
            return;
        pGC->ops->CopyArea((DrawablePtr) pPixmap, dst, pGC, sx, sy, sw, sh, dx,
                           dy);
        FreeScratchPixmapHeader(pPixmap);
    }
    else {
        GCPtr putGC = GetScratchGC(depth, dst->pScreen);

        if (!putGC)
            return;

        pPixmap = (*dst->pScreen->CreatePixmap) (dst->pScreen, sw, sh, depth,
                                                 CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pPixmap) {
            FreeScratchGC(putGC);
            return;
        }
        ValidateGC(&pPixmap->drawable, putGC);
        (*putGC->ops->PutImage) (&pPixmap->drawable, putGC, depth, -sx, -sy, w,
                                 h, 0,
                                 (format == XYPixmap) ? XYPixmap : ZPixmap,
                                 data);
        FreeScratchGC(putGC);
        if (format == XYBitmap)
            (void) (*pGC->ops->CopyPlane) (&pPixmap->drawable, dst, pGC, 0, 0,
                                           sw, sh, dx, dy, 1L);
        else
            (void) (*pGC->ops->CopyArea) (&pPixmap->drawable, dst, pGC, 0, 0,
                                          sw, sh, dx, dy);
        (*pPixmap->drawable.pScreen->DestroyPixmap) (pPixmap);
    }
}

static int
ProcShmPutImage(ClientPtr client)
{
    GCPtr pGC;
    DrawablePtr pDraw;
    long length;
    ShmDescPtr shmdesc;

    REQUEST(xShmPutImageReq);

    REQUEST_SIZE_MATCH(xShmPutImageReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, FALSE, shmdesc, client);
    if ((stuff->sendEvent != xTrue) && (stuff->sendEvent != xFalse))
        return BadValue;
    if (stuff->format == XYBitmap) {
        if (stuff->depth != 1)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
    }
    else if (stuff->format == XYPixmap) {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
        length *= stuff->depth;
    }
    else if (stuff->format == ZPixmap) {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, stuff->depth);
    }
    else {
        client->errorValue = stuff->format;
        return BadValue;
    }

    /*
     * There's a potential integer overflow in this check:
     * VERIFY_SHMSIZE(shmdesc, stuff->offset, length * stuff->totalHeight,
     *                client);
     * the version below ought to avoid it
     */
    if (stuff->totalHeight != 0 &&
        length > (shmdesc->size - stuff->offset) / stuff->totalHeight) {
        client->errorValue = stuff->totalWidth;
        return BadValue;
    }
    if (stuff->srcX > stuff->totalWidth) {
        client->errorValue = stuff->srcX;
        return BadValue;
    }
    if (stuff->srcY > stuff->totalHeight) {
        client->errorValue = stuff->srcY;
        return BadValue;
    }
    if ((stuff->srcX + stuff->srcWidth) > stuff->totalWidth) {
        client->errorValue = stuff->srcWidth;
        return BadValue;
    }
    if ((stuff->srcY + stuff->srcHeight) > stuff->totalHeight) {
        client->errorValue = stuff->srcHeight;
        return BadValue;
    }

    if ((((stuff->format == ZPixmap) && (stuff->srcX == 0)) ||
         ((stuff->format != ZPixmap) &&
          (stuff->srcX < screenInfo.bitmapScanlinePad) &&
          ((stuff->format == XYBitmap) ||
           ((stuff->srcY == 0) &&
            (stuff->srcHeight == stuff->totalHeight))))) &&
        ((stuff->srcX + stuff->srcWidth) == stuff->totalWidth))
        (*pGC->ops->PutImage) (pDraw, pGC, stuff->depth,
                               stuff->dstX, stuff->dstY,
                               stuff->totalWidth, stuff->srcHeight,
                               stuff->srcX, stuff->format,
                               shmdesc->addr + stuff->offset +
                               (stuff->srcY * length));
    else
        doShmPutImage(pDraw, pGC, stuff->depth, stuff->format,
                      stuff->totalWidth, stuff->totalHeight,
                      stuff->srcX, stuff->srcY,
                      stuff->srcWidth, stuff->srcHeight,
                      stuff->dstX, stuff->dstY, shmdesc->addr + stuff->offset);

    if (stuff->sendEvent) {
        xShmCompletionEvent ev = {
            .type = ShmCompletionCode,
            .drawable = stuff->drawable,
            .minorEvent = X_ShmPutImage,
            .majorEvent = ShmReqCode,
            .shmseg = stuff->shmseg,
            .offset = stuff->offset
        };
        WriteEventsToClient(client, 1, (xEvent *) &ev);
    }

    return Success;
}

static int
ProcShmGetImage(ClientPtr client)
{
    DrawablePtr pDraw;
    long lenPer = 0, length;
    Mask plane = 0;
    xShmGetImageReply xgi;
    ShmDescPtr shmdesc;
    VisualID visual = None;
    RegionPtr pVisibleRegion = NULL;
    int rc;

    REQUEST(xShmGetImageReq);

    REQUEST_SIZE_MATCH(xShmGetImageReq);
    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
        client->errorValue = stuff->format;
        return BadValue;
    }
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixReadAccess);
    if (rc != Success)
        return rc;
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);
    if (pDraw->type == DRAWABLE_WINDOW) {
        if (   /* check for being viewable */
               !((WindowPtr) pDraw)->realized ||
               /* check for being on screen */
               pDraw->x + stuff->x < 0 ||
               pDraw->x + stuff->x + (int) stuff->width > pDraw->pScreen->width
               || pDraw->y + stuff->y < 0 ||
               pDraw->y + stuff->y + (int) stuff->height >
               pDraw->pScreen->height ||
               /* check for being inside of border */
               stuff->x < -wBorderWidth((WindowPtr) pDraw) ||
               stuff->x + (int) stuff->width >
               wBorderWidth((WindowPtr) pDraw) + (int) pDraw->width ||
               stuff->y < -wBorderWidth((WindowPtr) pDraw) ||
               stuff->y + (int) stuff->height >
               wBorderWidth((WindowPtr) pDraw) + (int) pDraw->height)
            return BadMatch;
        visual = wVisual(((WindowPtr) pDraw));
        if (pDraw->type == DRAWABLE_WINDOW)
            pVisibleRegion = &((WindowPtr) pDraw)->borderClip;
        pDraw->pScreen->SourceValidate(pDraw, stuff->x, stuff->y,
                                       stuff->width, stuff->height,
                                       IncludeInferiors);
    }
    else {
        if (stuff->x < 0 ||
            stuff->x + (int) stuff->width > pDraw->width ||
            stuff->y < 0 || stuff->y + (int) stuff->height > pDraw->height)
            return BadMatch;
        visual = None;
    }
    xgi = (xShmGetImageReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .visual = visual,
        .depth = pDraw->depth
    };
    if (stuff->format == ZPixmap) {
        length = PixmapBytePad(stuff->width, pDraw->depth) * stuff->height;
    }
    else {
        lenPer = PixmapBytePad(stuff->width, 1) * stuff->height;
        plane = ((Mask) 1) << (pDraw->depth - 1);
        /* only planes asked for */
        length = lenPer * Ones(stuff->planeMask & (plane | (plane - 1)));
    }

    VERIFY_SHMSIZE(shmdesc, stuff->offset, length, client);
    xgi.size = length;

    if (length == 0) {
        /* nothing to do */
    }
    else if (stuff->format == ZPixmap) {
        (*pDraw->pScreen->GetImage) (pDraw, stuff->x, stuff->y,
                                     stuff->width, stuff->height,
                                     stuff->format, stuff->planeMask,
                                     shmdesc->addr + stuff->offset);
        if (pVisibleRegion)
            XaceCensorImage(client, pVisibleRegion,
                    PixmapBytePad(stuff->width, pDraw->depth), pDraw,
                    stuff->x, stuff->y, stuff->width, stuff->height,
                    stuff->format, shmdesc->addr + stuff->offset);
    }
    else {

        length = stuff->offset;
        for (; plane; plane >>= 1) {
            if (stuff->planeMask & plane) {
                (*pDraw->pScreen->GetImage) (pDraw,
                                             stuff->x, stuff->y,
                                             stuff->width, stuff->height,
                                             stuff->format, plane,
                                             shmdesc->addr + length);
                if (pVisibleRegion)
                    XaceCensorImage(client, pVisibleRegion,
                            BitmapBytePad(stuff->width), pDraw,
                            stuff->x, stuff->y, stuff->width, stuff->height,
                            stuff->format, shmdesc->addr + length);
                length += lenPer;
            }
        }
    }

    if (client->swapped) {
        swaps(&xgi.sequenceNumber);
        swapl(&xgi.length);
        swapl(&xgi.visual);
        swapl(&xgi.size);
    }
    WriteToClient(client, sizeof(xShmGetImageReply), &xgi);

    return Success;
}

#ifdef PANORAMIX
static int
ProcPanoramiXShmPutImage(ClientPtr client)
{
    int j, result, orig_x, orig_y;
    PanoramiXRes *draw, *gc;
    Bool sendEvent, isRoot;

    REQUEST(xShmPutImageReq);
    REQUEST_SIZE_MATCH(xShmPutImageReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &gc, stuff->gc,
                                     XRT_GC, client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    orig_x = stuff->dstX;
    orig_y = stuff->dstY;
    sendEvent = stuff->sendEvent;
    stuff->sendEvent = 0;
    FOR_NSCREENS(j) {
        if (!j)
            stuff->sendEvent = sendEvent;
        stuff->drawable = draw->info[j].id;
        stuff->gc = gc->info[j].id;
        if (isRoot) {
            stuff->dstX = orig_x - screenInfo.screens[j]->x;
            stuff->dstY = orig_y - screenInfo.screens[j]->y;
        }
        result = ProcShmPutImage(client);
        if (result != Success)
            break;
    }
    return result;
}

static int
ProcPanoramiXShmGetImage(ClientPtr client)
{
    PanoramiXRes *draw;
    DrawablePtr *drawables;
    DrawablePtr pDraw;
    xShmGetImageReply xgi;
    ShmDescPtr shmdesc;
    int i, x, y, w, h, format, rc;
    Mask plane = 0, planemask;
    long lenPer = 0, length, widthBytesLine;
    Bool isRoot;

    REQUEST(xShmGetImageReq);

    REQUEST_SIZE_MATCH(xShmGetImageReq);

    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
        client->errorValue = stuff->format;
        return BadValue;
    }

    rc = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                  XRC_DRAWABLE, client, DixWriteAccess);
    if (rc != Success)
        return (rc == BadValue) ? BadDrawable : rc;

    if (draw->type == XRT_PIXMAP)
        return ProcShmGetImage(client);

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixReadAccess);
    if (rc != Success)
        return rc;

    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);

    x = stuff->x;
    y = stuff->y;
    w = stuff->width;
    h = stuff->height;
    format = stuff->format;
    planemask = stuff->planeMask;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    if (isRoot) {
        if (                    /* check for being onscreen */
               x < 0 || x + w > PanoramiXPixWidth ||
               y < 0 || y + h > PanoramiXPixHeight)
            return BadMatch;
    }
    else {
        if (                    /* check for being onscreen */
               screenInfo.screens[0]->x + pDraw->x + x < 0 ||
               screenInfo.screens[0]->x + pDraw->x + x + w > PanoramiXPixWidth
               || screenInfo.screens[0]->y + pDraw->y + y < 0 ||
               screenInfo.screens[0]->y + pDraw->y + y + h > PanoramiXPixHeight
               ||
               /* check for being inside of border */
               x < -wBorderWidth((WindowPtr) pDraw) ||
               x + w > wBorderWidth((WindowPtr) pDraw) + (int) pDraw->width ||
               y < -wBorderWidth((WindowPtr) pDraw) ||
               y + h > wBorderWidth((WindowPtr) pDraw) + (int) pDraw->height)
            return BadMatch;
    }

    if (format == ZPixmap) {
        widthBytesLine = PixmapBytePad(w, pDraw->depth);
        length = widthBytesLine * h;
    }
    else {
        widthBytesLine = PixmapBytePad(w, 1);
        lenPer = widthBytesLine * h;
        plane = ((Mask) 1) << (pDraw->depth - 1);
        length = lenPer * Ones(planemask & (plane | (plane - 1)));
    }

    VERIFY_SHMSIZE(shmdesc, stuff->offset, length, client);

    drawables = calloc(PanoramiXNumScreens, sizeof(DrawablePtr));
    if (!drawables)
        return BadAlloc;

    drawables[0] = pDraw;
    FOR_NSCREENS_FORWARD_SKIP(i) {
        rc = dixLookupDrawable(drawables + i, draw->info[i].id, client, 0,
                               DixReadAccess);
        if (rc != Success) {
            free(drawables);
            return rc;
        }
    }
    FOR_NSCREENS_FORWARD(i) {
        drawables[i]->pScreen->SourceValidate(drawables[i], 0, 0,
                                              drawables[i]->width,
                                              drawables[i]->height,
                                              IncludeInferiors);
    }

    xgi = (xShmGetImageReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .visual = wVisual(((WindowPtr) pDraw)),
        .depth = pDraw->depth
    };

    xgi.size = length;

    if (length == 0) {          /* nothing to do */
    }
    else if (format == ZPixmap) {
        XineramaGetImageData(drawables, x, y, w, h, format, planemask,
                             shmdesc->addr + stuff->offset,
                             widthBytesLine, isRoot);
    }
    else {

        length = stuff->offset;
        for (; plane; plane >>= 1) {
            if (planemask & plane) {
                XineramaGetImageData(drawables, x, y, w, h,
                                     format, plane, shmdesc->addr + length,
                                     widthBytesLine, isRoot);
                length += lenPer;
            }
        }
    }
    free(drawables);

    if (client->swapped) {
        swaps(&xgi.sequenceNumber);
        swapl(&xgi.length);
        swapl(&xgi.visual);
        swapl(&xgi.size);
    }
    WriteToClient(client, sizeof(xShmGetImageReply), &xgi);

    return Success;
}

static int
ProcPanoramiXShmCreatePixmap(ClientPtr client)
{
    ScreenPtr pScreen = NULL;
    PixmapPtr pMap = NULL;
    DrawablePtr pDraw;
    DepthPtr pDepth;
    int i, j, result, rc;
    ShmDescPtr shmdesc;

    REQUEST(xShmCreatePixmapReq);
    unsigned int width, height, depth;
    unsigned long size;
    PanoramiXRes *newPix;

    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    client->errorValue = stuff->pid;
    if (!sharedPixmaps)
        return BadImplementation;
    LEGAL_NEW_RESOURCE(stuff->pid, client);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);

    width = stuff->width;
    height = stuff->height;
    depth = stuff->depth;
    if (!width || !height || !depth) {
        client->errorValue = 0;
        return BadValue;
    }
    if (width > 32767 || height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i = 0; i < pDraw->pScreen->numDepths; i++, pDepth++)
            if (pDepth->depth == stuff->depth)
                goto CreatePmap;
        client->errorValue = stuff->depth;
        return BadValue;
    }

 CreatePmap:
    size = PixmapBytePad(width, depth) * height;
    if (sizeof(size) == 4 && BitsPerPixel(depth) > 8) {
        if (size < width * height)
            return BadAlloc;
    }
    /* thankfully, offset is unsigned */
    if (stuff->offset + size < size)
        return BadAlloc;

    VERIFY_SHMSIZE(shmdesc, stuff->offset, size, client);

    if (!(newPix = malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = TRUE;
    panoramix_setup_ids(newPix, client, stuff->pid);

    result = Success;

    FOR_NSCREENS(j) {
        ShmScrPrivateRec *screen_priv;

        pScreen = screenInfo.screens[j];

        screen_priv = ShmGetScreenPriv(pScreen);
        pMap = (*screen_priv->shmFuncs->CreatePixmap) (pScreen,
                                                       stuff->width,
                                                       stuff->height,
                                                       stuff->depth,
                                                       shmdesc->addr +
                                                       stuff->offset);

        if (pMap) {
            result = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid,
                              RT_PIXMAP, pMap, RT_NONE, NULL, DixCreateAccess);
            if (result != Success) {
                pDraw->pScreen->DestroyPixmap(pMap);
                break;
            }
            dixSetPrivate(&pMap->devPrivates, shmPixmapPrivateKey, shmdesc);
            shmdesc->refcnt++;
            pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
            pMap->drawable.id = newPix->info[j].id;
            if (!AddResource(newPix->info[j].id, RT_PIXMAP, (void *) pMap)) {
                result = BadAlloc;
                break;
            }
        }
        else {
            result = BadAlloc;
            break;
        }
    }

    if (result != Success) {
        while (j--)
            FreeResource(newPix->info[j].id, RT_NONE);
        free(newPix);
    }
    else
        AddResource(stuff->pid, XRT_PIXMAP, newPix);

    return result;
}
#endif

static PixmapPtr
fbShmCreatePixmap(ScreenPtr pScreen,
                  int width, int height, int depth, char *addr)
{
    PixmapPtr pPixmap;

    pPixmap = (*pScreen->CreatePixmap) (pScreen, 0, 0, pScreen->rootDepth, 0);
    if (!pPixmap)
        return NullPixmap;

    if (!(*pScreen->ModifyPixmapHeader) (pPixmap, width, height, depth,
                                         BitsPerPixel(depth),
                                         PixmapBytePad(width, depth),
                                         (void *) addr)) {
        (*pScreen->DestroyPixmap) (pPixmap);
        return NullPixmap;
    }
    return pPixmap;
}

static int
ProcShmCreatePixmap(ClientPtr client)
{
    PixmapPtr pMap;
    DrawablePtr pDraw;
    DepthPtr pDepth;
    int i, rc;
    ShmDescPtr shmdesc;
    ShmScrPrivateRec *screen_priv;

    REQUEST(xShmCreatePixmapReq);
    unsigned int width, height, depth;
    unsigned long size;

    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    client->errorValue = stuff->pid;
    if (!sharedPixmaps)
        return BadImplementation;
    LEGAL_NEW_RESOURCE(stuff->pid, client);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);

    width = stuff->width;
    height = stuff->height;
    depth = stuff->depth;
    if (!width || !height || !depth) {
        client->errorValue = 0;
        return BadValue;
    }
    if (width > 32767 || height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i = 0; i < pDraw->pScreen->numDepths; i++, pDepth++)
            if (pDepth->depth == stuff->depth)
                goto CreatePmap;
        client->errorValue = stuff->depth;
        return BadValue;
    }

 CreatePmap:
    size = PixmapBytePad(width, depth) * height;
    if (sizeof(size) == 4 && BitsPerPixel(depth) > 8) {
        if (size < width * height)
            return BadAlloc;
    }
    /* thankfully, offset is unsigned */
    if (stuff->offset + size < size)
        return BadAlloc;

    VERIFY_SHMSIZE(shmdesc, stuff->offset, size, client);
    screen_priv = ShmGetScreenPriv(pDraw->pScreen);
    pMap = (*screen_priv->shmFuncs->CreatePixmap) (pDraw->pScreen, stuff->width,
                                                   stuff->height, stuff->depth,
                                                   shmdesc->addr +
                                                   stuff->offset);
    if (pMap) {
        rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, RT_PIXMAP,
                      pMap, RT_NONE, NULL, DixCreateAccess);
        if (rc != Success) {
            pDraw->pScreen->DestroyPixmap(pMap);
            return rc;
        }
        dixSetPrivate(&pMap->devPrivates, shmPixmapPrivateKey, shmdesc);
        shmdesc->refcnt++;
        pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        pMap->drawable.id = stuff->pid;
        if (AddResource(stuff->pid, RT_PIXMAP, (void *) pMap)) {
            return Success;
        }
    }
    return BadAlloc;
}

#ifdef SHM_FD_PASSING

static void
ShmBusfaultNotify(void *context)
{
    ShmDescPtr shmdesc = context;

    ErrorF("shared memory 0x%x truncated by client\n",
           (unsigned int) shmdesc->resource);
    busfault_unregister(shmdesc->busfault);
    shmdesc->busfault = NULL;
    FreeResource (shmdesc->resource, RT_NONE);
}

static int
ProcShmAttachFd(ClientPtr client)
{
    int fd;
    ShmDescPtr shmdesc;
    REQUEST(xShmAttachFdReq);
    struct stat statb;

    SetReqFds(client, 1);
    REQUEST_SIZE_MATCH(xShmAttachFdReq);
    LEGAL_NEW_RESOURCE(stuff->shmseg, client);
    if ((stuff->readOnly != xTrue) && (stuff->readOnly != xFalse)) {
        client->errorValue = stuff->readOnly;
        return BadValue;
    }
    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadMatch;

    if (fstat(fd, &statb) < 0 || statb.st_size == 0) {
        close(fd);
        return BadMatch;
    }

    shmdesc = malloc(sizeof(ShmDescRec));
    if (!shmdesc) {
        close(fd);
        return BadAlloc;
    }
    shmdesc->is_fd = TRUE;
    shmdesc->addr = mmap(NULL, statb.st_size,
                         stuff->readOnly ? PROT_READ : PROT_READ|PROT_WRITE,
                         MAP_SHARED,
                         fd, 0);

    close(fd);
    if (shmdesc->addr == ((char *) -1)) {
        free(shmdesc);
        return BadAccess;
    }

    shmdesc->refcnt = 1;
    shmdesc->writable = !stuff->readOnly;
    shmdesc->size = statb.st_size;
    shmdesc->resource = stuff->shmseg;

    shmdesc->busfault = busfault_register_mmap(shmdesc->addr, shmdesc->size, ShmBusfaultNotify, shmdesc);
    if (!shmdesc->busfault) {
        munmap(shmdesc->addr, shmdesc->size);
        free(shmdesc);
        return BadAlloc;
    }

    shmdesc->next = Shmsegs;
    Shmsegs = shmdesc;

    if (!AddResource(stuff->shmseg, ShmSegType, (void *) shmdesc))
        return BadAlloc;
    return Success;
}

static int
shm_tmpfile(void)
{
    const char *shmdirs[] = {
        "/run/shm",
        "/var/tmp",
        "/tmp",
    };
    int	fd;

#ifdef HAVE_MEMFD_CREATE
    fd = memfd_create("xorg", MFD_CLOEXEC|MFD_ALLOW_SEALING);
    if (fd != -1) {
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK);
        DebugF ("Using memfd_create\n");
        return fd;
    }
#endif

#ifdef O_TMPFILE
    for (int i = 0; i < ARRAY_SIZE(shmdirs); i++) {
        fd = open(shmdirs[i], O_TMPFILE|O_RDWR|O_CLOEXEC|O_EXCL, 0666);
        if (fd >= 0) {
            DebugF ("Using O_TMPFILE\n");
            return fd;
        }
    }
    ErrorF ("Not using O_TMPFILE\n");
#endif

    for (int i = 0; i < ARRAY_SIZE(shmdirs); i++) {
        char template[PATH_MAX];
        snprintf(template, ARRAY_SIZE(template), "%s/shmfd-XXXXXX", shmdirs[i]);
#ifdef HAVE_MKOSTEMP
        fd = mkostemp(template, O_CLOEXEC);
#else
        fd = mkstemp(template);
#endif
        if (fd < 0)
            continue;
        unlink(template);
#ifndef HAVE_MKOSTEMP
        int flags = fcntl(fd, F_GETFD);
        if (flags != -1) {
            flags |= FD_CLOEXEC;
            (void) fcntl(fd, F_SETFD, flags);
        }
#endif
        return fd;
    }

    return -1;
}

static int
ProcShmCreateSegment(ClientPtr client)
{
    int fd;
    ShmDescPtr shmdesc;
    REQUEST(xShmCreateSegmentReq);
    xShmCreateSegmentReply rep = {
        .type = X_Reply,
        .nfd = 1,
        .sequenceNumber = client->sequence,
        .length = 0,
    };

    REQUEST_SIZE_MATCH(xShmCreateSegmentReq);
    LEGAL_NEW_RESOURCE(stuff->shmseg, client);
    if ((stuff->readOnly != xTrue) && (stuff->readOnly != xFalse)) {
        client->errorValue = stuff->readOnly;
        return BadValue;
    }
    fd = shm_tmpfile();
    if (fd < 0)
        return BadAlloc;
    if (ftruncate(fd, stuff->size) < 0) {
        close(fd);
        return BadAlloc;
    }
    shmdesc = malloc(sizeof(ShmDescRec));
    if (!shmdesc) {
        close(fd);
        return BadAlloc;
    }
    shmdesc->is_fd = TRUE;
    shmdesc->addr = mmap(NULL, stuff->size,
                         stuff->readOnly ? PROT_READ : PROT_READ|PROT_WRITE,
                         MAP_SHARED,
                         fd, 0);

    if (shmdesc->addr == ((char *) -1)) {
        close(fd);
        free(shmdesc);
        return BadAccess;
    }

    shmdesc->refcnt = 1;
    shmdesc->writable = !stuff->readOnly;
    shmdesc->size = stuff->size;

    shmdesc->busfault = busfault_register_mmap(shmdesc->addr, shmdesc->size, ShmBusfaultNotify, shmdesc);
    if (!shmdesc->busfault) {
        close(fd);
        munmap(shmdesc->addr, shmdesc->size);
        free(shmdesc);
        return BadAlloc;
    }

    shmdesc->next = Shmsegs;
    Shmsegs = shmdesc;

    if (!AddResource(stuff->shmseg, ShmSegType, (void *) shmdesc)) {
        close(fd);
        return BadAlloc;
    }

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        FreeResource(stuff->shmseg, RT_NONE);
        close(fd);
        return BadAlloc;
    }
    WriteToClient(client, sizeof (xShmCreateSegmentReply), &rep);
    return Success;
}
#endif /* SHM_FD_PASSING */

static int
ProcShmDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data == X_ShmQueryVersion)
        return ProcShmQueryVersion(client);

    if (!client->local)
        return BadRequest;

    switch (stuff->data) {
    case X_ShmAttach:
        return ProcShmAttach(client);
    case X_ShmDetach:
        return ProcShmDetach(client);
    case X_ShmPutImage:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShmPutImage(client);
#endif
        return ProcShmPutImage(client);
    case X_ShmGetImage:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShmGetImage(client);
#endif
        return ProcShmGetImage(client);
    case X_ShmCreatePixmap:
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            return ProcPanoramiXShmCreatePixmap(client);
#endif
        return ProcShmCreatePixmap(client);
#ifdef SHM_FD_PASSING
    case X_ShmAttachFd:
        return ProcShmAttachFd(client);
    case X_ShmCreateSegment:
        return ProcShmCreateSegment(client);
#endif
    default:
        return BadRequest;
    }
}

static void _X_COLD
SShmCompletionEvent(xShmCompletionEvent * from, xShmCompletionEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->drawable, to->drawable);
    cpswaps(from->minorEvent, to->minorEvent);
    to->majorEvent = from->majorEvent;
    cpswapl(from->shmseg, to->shmseg);
    cpswapl(from->offset, to->offset);
}

static int _X_COLD
SProcShmQueryVersion(ClientPtr client)
{
    REQUEST(xShmQueryVersionReq);

    swaps(&stuff->length);
    return ProcShmQueryVersion(client);
}

static int _X_COLD
SProcShmAttach(ClientPtr client)
{
    REQUEST(xShmAttachReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmAttachReq);
    swapl(&stuff->shmseg);
    swapl(&stuff->shmid);
    return ProcShmAttach(client);
}

static int _X_COLD
SProcShmDetach(ClientPtr client)
{
    REQUEST(xShmDetachReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmDetachReq);
    swapl(&stuff->shmseg);
    return ProcShmDetach(client);
}

static int _X_COLD
SProcShmPutImage(ClientPtr client)
{
    REQUEST(xShmPutImageReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmPutImageReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    swaps(&stuff->totalWidth);
    swaps(&stuff->totalHeight);
    swaps(&stuff->srcX);
    swaps(&stuff->srcY);
    swaps(&stuff->srcWidth);
    swaps(&stuff->srcHeight);
    swaps(&stuff->dstX);
    swaps(&stuff->dstY);
    swapl(&stuff->shmseg);
    swapl(&stuff->offset);
    return ProcShmPutImage(client);
}

static int _X_COLD
SProcShmGetImage(ClientPtr client)
{
    REQUEST(xShmGetImageReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmGetImageReq);
    swapl(&stuff->drawable);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->planeMask);
    swapl(&stuff->shmseg);
    swapl(&stuff->offset);
    return ProcShmGetImage(client);
}

static int _X_COLD
SProcShmCreatePixmap(ClientPtr client)
{
    REQUEST(xShmCreatePixmapReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    swapl(&stuff->pid);
    swapl(&stuff->drawable);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->shmseg);
    swapl(&stuff->offset);
    return ProcShmCreatePixmap(client);
}

#ifdef SHM_FD_PASSING
static int _X_COLD
SProcShmAttachFd(ClientPtr client)
{
    REQUEST(xShmAttachFdReq);
    SetReqFds(client, 1);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmAttachFdReq);
    swapl(&stuff->shmseg);
    return ProcShmAttachFd(client);
}

static int _X_COLD
SProcShmCreateSegment(ClientPtr client)
{
    REQUEST(xShmCreateSegmentReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xShmCreateSegmentReq);
    swapl(&stuff->shmseg);
    swapl(&stuff->size);
    return ProcShmCreateSegment(client);
}
#endif  /* SHM_FD_PASSING */

static int _X_COLD
SProcShmDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data == X_ShmQueryVersion)
        return SProcShmQueryVersion(client);

    if (!client->local)
        return BadRequest;

    switch (stuff->data) {
    case X_ShmAttach:
        return SProcShmAttach(client);
    case X_ShmDetach:
        return SProcShmDetach(client);
    case X_ShmPutImage:
        return SProcShmPutImage(client);
    case X_ShmGetImage:
        return SProcShmGetImage(client);
    case X_ShmCreatePixmap:
        return SProcShmCreatePixmap(client);
#ifdef SHM_FD_PASSING
    case X_ShmAttachFd:
        return SProcShmAttachFd(client);
    case X_ShmCreateSegment:
        return SProcShmCreateSegment(client);
#endif
    default:
        return BadRequest;
    }
}

void
ShmExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int i;

#ifdef MUST_CHECK_FOR_SHM_SYSCALL
    if (!CheckForShmSyscall()) {
        ErrorF("MIT-SHM extension disabled due to lack of kernel support\n");
        return;
    }
#endif

    if (!ShmRegisterPrivates())
        return;

    sharedPixmaps = xFalse;
    {
        sharedPixmaps = xTrue;
        for (i = 0; i < screenInfo.numScreens; i++) {
            ShmScrPrivateRec *screen_priv =
                ShmInitScreenPriv(screenInfo.screens[i]);
            if (!screen_priv->shmFuncs)
                screen_priv->shmFuncs = &miFuncs;
            if (!screen_priv->shmFuncs->CreatePixmap)
                sharedPixmaps = xFalse;
        }
        if (sharedPixmaps)
            for (i = 0; i < screenInfo.numScreens; i++) {
                ShmScrPrivateRec *screen_priv =
                    ShmGetScreenPriv(screenInfo.screens[i]);
                screen_priv->destroyPixmap =
                    screenInfo.screens[i]->DestroyPixmap;
                screenInfo.screens[i]->DestroyPixmap = ShmDestroyPixmap;
            }
    }
    ShmSegType = CreateNewResourceType(ShmDetachSegment, "ShmSeg");
    if (ShmSegType &&
        (extEntry = AddExtension(SHMNAME, ShmNumberEvents, ShmNumberErrors,
                                 ProcShmDispatch, SProcShmDispatch,
                                 ShmResetProc, StandardMinorOpcode))) {
        ShmReqCode = (unsigned char) extEntry->base;
        ShmCompletionCode = extEntry->eventBase;
        BadShmSegCode = extEntry->errorBase;
        SetResourceTypeErrorValue(ShmSegType, BadShmSegCode);
        EventSwapVector[ShmCompletionCode] = (EventSwapPtr) SShmCompletionEvent;
    }
}
