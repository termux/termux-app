/*

   XFree86 Xv DDX written by Mark Vojkovich (markv@valinux.com)
   Adapted for KDrive by Pontus Lidman <pontus.lidman@nokia.com>

   Copyright (C) 2000, 2001 - Nokia Home Communications
   Copyright (C) 1998, 1999 - The XFree86 Project Inc.

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "kdrive.h"

#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "mivalidate.h"
#include "validate.h"
#include "resource.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>

#include "kxv.h"
#include "fourcc.h"

/* XvAdaptorRec fields */

static int KdXVPutVideo(DrawablePtr, XvPortPtr, GCPtr,
                        INT16, INT16, CARD16, CARD16,
                        INT16, INT16, CARD16, CARD16);
static int KdXVPutStill(DrawablePtr, XvPortPtr, GCPtr,
                        INT16, INT16, CARD16, CARD16,
                        INT16, INT16, CARD16, CARD16);
static int KdXVGetVideo(DrawablePtr, XvPortPtr, GCPtr,
                        INT16, INT16, CARD16, CARD16,
                        INT16, INT16, CARD16, CARD16);
static int KdXVGetStill(DrawablePtr, XvPortPtr, GCPtr,
                        INT16, INT16, CARD16, CARD16,
                        INT16, INT16, CARD16, CARD16);
static int KdXVStopVideo(XvPortPtr, DrawablePtr);
static int KdXVSetPortAttribute(XvPortPtr, Atom, INT32);
static int KdXVGetPortAttribute(XvPortPtr, Atom, INT32 *);
static int KdXVQueryBestSize(XvPortPtr, CARD8,
                             CARD16, CARD16, CARD16, CARD16,
                             unsigned int *, unsigned int *);
static int KdXVPutImage(DrawablePtr, XvPortPtr, GCPtr,
                        INT16, INT16, CARD16, CARD16,
                        INT16, INT16, CARD16, CARD16,
                        XvImagePtr, unsigned char *, Bool, CARD16, CARD16);
static int KdXVQueryImageAttributes(XvPortPtr, XvImagePtr,
                                    CARD16 *, CARD16 *, int *, int *);

/* ScreenRec fields */

static Bool KdXVDestroyWindow(WindowPtr pWin);
static void KdXVWindowExposures(WindowPtr pWin, RegionPtr r1);
static void KdXVClipNotify(WindowPtr pWin, int dx, int dy);
static Bool KdXVCloseScreen(ScreenPtr);

/* misc */
static Bool KdXVInitAdaptors(ScreenPtr, KdVideoAdaptorPtr, int);

static DevPrivateKeyRec KdXVWindowKeyRec;

#define KdXVWindowKey (&KdXVWindowKeyRec)
static DevPrivateKey KdXvScreenKey;
static DevPrivateKeyRec KdXVScreenPrivateKey;
static unsigned long KdXVGeneration = 0;
static unsigned long PortResource = 0;

#define GET_XV_SCREEN(pScreen) ((XvScreenPtr) \
    dixLookupPrivate(&(pScreen)->devPrivates, KdXvScreenKey))

#define GET_KDXV_SCREEN(pScreen) \
    ((KdXVScreenPtr)(dixGetPrivate(&pScreen->devPrivates, &KdXVScreenPrivateKey)))

#define GET_KDXV_WINDOW(pWin) ((KdXVWindowPtr) \
    dixLookupPrivate(&(pWin)->devPrivates, KdXVWindowKey))

Bool
KdXVScreenInit(ScreenPtr pScreen, KdVideoAdaptorPtr adaptors, int num)
{
    KdXVScreenPtr ScreenPriv;

/*   fprintf(stderr,"KdXVScreenInit initializing %d adaptors\n",num); */

    if (KdXVGeneration != serverGeneration)
        KdXVGeneration = serverGeneration;

    if (noXvExtension)
        return FALSE;

    if (!dixRegisterPrivateKey(&KdXVWindowKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&KdXVScreenPrivateKey, PRIVATE_SCREEN, 0))
        return FALSE;

    if (Success != XvScreenInit(pScreen))
        return FALSE;

    KdXvScreenKey = XvGetScreenKey();
    PortResource = XvGetRTPort();

    ScreenPriv = malloc(sizeof(KdXVScreenRec));
    dixSetPrivate(&pScreen->devPrivates, &KdXVScreenPrivateKey, ScreenPriv);

    if (!ScreenPriv)
        return FALSE;

    ScreenPriv->DestroyWindow = pScreen->DestroyWindow;
    ScreenPriv->WindowExposures = pScreen->WindowExposures;
    ScreenPriv->ClipNotify = pScreen->ClipNotify;
    ScreenPriv->CloseScreen = pScreen->CloseScreen;

/*   fprintf(stderr,"XV: Wrapping screen funcs\n"); */

    pScreen->DestroyWindow = KdXVDestroyWindow;
    pScreen->WindowExposures = KdXVWindowExposures;
    pScreen->ClipNotify = KdXVClipNotify;
    pScreen->CloseScreen = KdXVCloseScreen;

    if (!KdXVInitAdaptors(pScreen, adaptors, num))
        return FALSE;

    return TRUE;
}

static void
KdXVFreeAdaptor(XvAdaptorPtr pAdaptor)
{
    int i;

    if (pAdaptor->pPorts) {
        XvPortPtr pPort = pAdaptor->pPorts;
        XvPortRecPrivatePtr pPriv;

        for (i = 0; i < pAdaptor->nPorts; i++, pPort++) {
            pPriv = (XvPortRecPrivatePtr) pPort->devPriv.ptr;
            if (pPriv) {
                if (pPriv->clientClip)
                    RegionDestroy(pPriv->clientClip);
                if (pPriv->pCompositeClip && pPriv->FreeCompositeClip)
                    RegionDestroy(pPriv->pCompositeClip);
                free(pPriv);
            }
        }
    }

    XvFreeAdaptor(pAdaptor);
}

static Bool
KdXVInitAdaptors(ScreenPtr pScreen, KdVideoAdaptorPtr infoPtr, int number)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;

    XvScreenPtr pxvs = GET_XV_SCREEN(pScreen);
    KdVideoAdaptorPtr adaptorPtr;
    XvAdaptorPtr pAdaptor, pa;
    XvAdaptorRecPrivatePtr adaptorPriv;
    int na, numAdaptor;
    XvPortRecPrivatePtr portPriv;
    XvPortPtr pPort, pp;
    int numPort;
    KdVideoFormatPtr formatPtr;
    XvFormatPtr pFormat, pf;
    int numFormat, totFormat;
    KdVideoEncodingPtr encodingPtr;
    XvEncodingPtr pEncode, pe;
    int numVisuals;
    VisualPtr pVisual;
    int i;

    pxvs->nAdaptors = 0;
    pxvs->pAdaptors = NULL;

    if (!(pAdaptor = calloc(number, sizeof(XvAdaptorRec))))
        return FALSE;

    for (pa = pAdaptor, na = 0, numAdaptor = 0; na < number; na++, adaptorPtr++) {
        adaptorPtr = &infoPtr[na];

        if (!adaptorPtr->StopVideo || !adaptorPtr->SetPortAttribute ||
            !adaptorPtr->GetPortAttribute || !adaptorPtr->QueryBestSize)
            continue;

        /* client libs expect at least one encoding */
        if (!adaptorPtr->nEncodings || !adaptorPtr->pEncodings)
            continue;

        pa->type = adaptorPtr->type;

        if (!adaptorPtr->PutVideo && !adaptorPtr->GetVideo)
            pa->type &= ~XvVideoMask;

        if (!adaptorPtr->PutStill && !adaptorPtr->GetStill)
            pa->type &= ~XvStillMask;

        if (!adaptorPtr->PutImage || !adaptorPtr->QueryImageAttributes)
            pa->type &= ~XvImageMask;

        if (!adaptorPtr->PutVideo && !adaptorPtr->PutImage &&
            !adaptorPtr->PutStill)
            pa->type &= ~XvInputMask;

        if (!adaptorPtr->GetVideo && !adaptorPtr->GetStill)
            pa->type &= ~XvOutputMask;

        if (!(adaptorPtr->type & (XvPixmapMask | XvWindowMask)))
            continue;
        if (!(adaptorPtr->type & (XvImageMask | XvVideoMask | XvStillMask)))
            continue;

        pa->pScreen = pScreen;
        pa->ddPutVideo = KdXVPutVideo;
        pa->ddPutStill = KdXVPutStill;
        pa->ddGetVideo = KdXVGetVideo;
        pa->ddGetStill = KdXVGetStill;
        pa->ddStopVideo = KdXVStopVideo;
        pa->ddPutImage = KdXVPutImage;
        pa->ddSetPortAttribute = KdXVSetPortAttribute;
        pa->ddGetPortAttribute = KdXVGetPortAttribute;
        pa->ddQueryBestSize = KdXVQueryBestSize;
        pa->ddQueryImageAttributes = KdXVQueryImageAttributes;
        pa->name = strdup(adaptorPtr->name);

        if (adaptorPtr->nEncodings &&
            (pEncode = calloc(adaptorPtr->nEncodings, sizeof(XvEncodingRec)))) {

            for (pe = pEncode, encodingPtr = adaptorPtr->pEncodings, i = 0;
                 i < adaptorPtr->nEncodings; pe++, i++, encodingPtr++) {
                pe->id = encodingPtr->id;
                pe->pScreen = pScreen;
                pe->name = strdup(encodingPtr->name);
                pe->width = encodingPtr->width;
                pe->height = encodingPtr->height;
                pe->rate.numerator = encodingPtr->rate.numerator;
                pe->rate.denominator = encodingPtr->rate.denominator;
            }
            pa->nEncodings = adaptorPtr->nEncodings;
            pa->pEncodings = pEncode;
        }

        if (adaptorPtr->nImages &&
            (pa->pImages = calloc(adaptorPtr->nImages, sizeof(XvImageRec)))) {
            memcpy(pa->pImages, adaptorPtr->pImages,
                   adaptorPtr->nImages * sizeof(XvImageRec));
            pa->nImages = adaptorPtr->nImages;
        }

        if (adaptorPtr->nAttributes &&
            (pa->pAttributes = calloc(adaptorPtr->nAttributes,
                                      sizeof(XvAttributeRec)))) {
            memcpy(pa->pAttributes, adaptorPtr->pAttributes,
                   adaptorPtr->nAttributes * sizeof(XvAttributeRec));

            for (i = 0; i < adaptorPtr->nAttributes; i++) {
                pa->pAttributes[i].name =
                    strdup(adaptorPtr->pAttributes[i].name);
            }

            pa->nAttributes = adaptorPtr->nAttributes;
        }

        totFormat = adaptorPtr->nFormats;

        if (!(pFormat = calloc(totFormat, sizeof(XvFormatRec)))) {
            KdXVFreeAdaptor(pa);
            continue;
        }
        for (pf = pFormat, i = 0, numFormat = 0, formatPtr =
             adaptorPtr->pFormats; i < adaptorPtr->nFormats; i++, formatPtr++) {
            numVisuals = pScreen->numVisuals;
            pVisual = pScreen->visuals;

            while (numVisuals--) {
                if ((pVisual->class == formatPtr->class) &&
                    (pVisual->nplanes == formatPtr->depth)) {

                    if (numFormat >= totFormat) {
                        void *moreSpace;

                        totFormat *= 2;
                        moreSpace = reallocarray(pFormat, totFormat,
                                                 sizeof(XvFormatRec));
                        if (!moreSpace)
                            break;
                        pFormat = moreSpace;
                        pf = pFormat + numFormat;
                    }

                    pf->visual = pVisual->vid;
                    pf->depth = formatPtr->depth;

                    pf++;
                    numFormat++;
                }
                pVisual++;
            }
        }
        pa->nFormats = numFormat;
        pa->pFormats = pFormat;
        if (!numFormat) {
            KdXVFreeAdaptor(pa);
            continue;
        }

        if (!(adaptorPriv = calloc(1, sizeof(XvAdaptorRecPrivate)))) {
            KdXVFreeAdaptor(pa);
            continue;
        }

        adaptorPriv->flags = adaptorPtr->flags;
        adaptorPriv->PutVideo = adaptorPtr->PutVideo;
        adaptorPriv->PutStill = adaptorPtr->PutStill;
        adaptorPriv->GetVideo = adaptorPtr->GetVideo;
        adaptorPriv->GetStill = adaptorPtr->GetStill;
        adaptorPriv->StopVideo = adaptorPtr->StopVideo;
        adaptorPriv->SetPortAttribute = adaptorPtr->SetPortAttribute;
        adaptorPriv->GetPortAttribute = adaptorPtr->GetPortAttribute;
        adaptorPriv->QueryBestSize = adaptorPtr->QueryBestSize;
        adaptorPriv->QueryImageAttributes = adaptorPtr->QueryImageAttributes;
        adaptorPriv->PutImage = adaptorPtr->PutImage;
        adaptorPriv->ReputImage = adaptorPtr->ReputImage;

        pa->devPriv.ptr = (void *) adaptorPriv;

        if (!(pPort = calloc(adaptorPtr->nPorts, sizeof(XvPortRec)))) {
            KdXVFreeAdaptor(pa);
            continue;
        }
        for (pp = pPort, i = 0, numPort = 0; i < adaptorPtr->nPorts; i++) {

            if (!(pp->id = FakeClientID(0)))
                continue;

            if (!(portPriv = calloc(1, sizeof(XvPortRecPrivate))))
                continue;

            if (!AddResource(pp->id, PortResource, pp)) {
                free(portPriv);
                continue;
            }

            pp->pAdaptor = pa;
            pp->pNotify = (XvPortNotifyPtr) NULL;
            pp->pDraw = (DrawablePtr) NULL;
            pp->client = (ClientPtr) NULL;
            pp->grab.client = (ClientPtr) NULL;
            pp->time = currentTime;
            pp->devPriv.ptr = portPriv;

            portPriv->screen = screen;
            portPriv->AdaptorRec = adaptorPriv;
            portPriv->DevPriv.ptr = adaptorPtr->pPortPrivates[i].ptr;

            pp++;
            numPort++;
        }
        pa->nPorts = numPort;
        pa->pPorts = pPort;
        if (!numPort) {
            KdXVFreeAdaptor(pa);
            continue;
        }

        pa->base_id = pPort->id;

        pa++;
        numAdaptor++;
    }

    if (numAdaptor) {
        pxvs->nAdaptors = numAdaptor;
        pxvs->pAdaptors = pAdaptor;
    }
    else {
        free(pAdaptor);
        return FALSE;
    }

    return TRUE;
}

/* Video should be clipped to the intersection of the window cliplist
   and the client cliplist specified in the GC for which the video was
   initialized.  When we need to reclip a window, the GC that started
   the video may not even be around anymore.  That's why we save the
   client clip from the GC when the video is initialized.  We then
   use KdXVUpdateCompositeClip to calculate the new composite clip
   when we need it.  This is different from what DEC did.  They saved
   the GC and used its clip list when they needed to reclip the window,
   even if the client clip was different from the one the video was
   initialized with.  If the original GC was destroyed, they had to stop
   the video.  I like the new method better (MArk).

   This function only works for windows.  Will need to rewrite when
   (if) we support pixmap rendering.
*/

static void
KdXVUpdateCompositeClip(XvPortRecPrivatePtr portPriv)
{
    RegionPtr pregWin, pCompositeClip;
    WindowPtr pWin;
    Bool freeCompClip = FALSE;

    if (portPriv->pCompositeClip)
        return;

    pWin = (WindowPtr) portPriv->pDraw;

    /* get window clip list */
    if (portPriv->subWindowMode == IncludeInferiors) {
        pregWin = NotClippedByChildren(pWin);
        freeCompClip = TRUE;
    }
    else
        pregWin = &pWin->clipList;

    if (!portPriv->clientClip) {
        portPriv->pCompositeClip = pregWin;
        portPriv->FreeCompositeClip = freeCompClip;
        return;
    }

    pCompositeClip = RegionCreate(NullBox, 1);
    RegionCopy(pCompositeClip, portPriv->clientClip);
    RegionTranslate(pCompositeClip,
                    portPriv->pDraw->x + portPriv->clipOrg.x,
                    portPriv->pDraw->y + portPriv->clipOrg.y);
    RegionIntersect(pCompositeClip, pregWin, pCompositeClip);

    portPriv->pCompositeClip = pCompositeClip;
    portPriv->FreeCompositeClip = TRUE;

    if (freeCompClip) {
        RegionDestroy(pregWin);
    }
}

/* Save the current clientClip and update the CompositeClip whenever
   we have a fresh GC */

static void
KdXVCopyClip(XvPortRecPrivatePtr portPriv, GCPtr pGC)
{
    /* copy the new clip if it exists */
    if (pGC->clientClip) {
        if (!portPriv->clientClip)
            portPriv->clientClip = RegionCreate(NullBox, 1);
        /* Note: this is in window coordinates */
        RegionCopy(portPriv->clientClip, pGC->clientClip);
    }
    else if (portPriv->clientClip) {    /* free the old clientClip */
        RegionDestroy(portPriv->clientClip);
        portPriv->clientClip = NULL;
    }

    /* get rid of the old clip list */
    if (portPriv->pCompositeClip && portPriv->FreeCompositeClip) {
        RegionDestroy(portPriv->pCompositeClip);
    }

    portPriv->clipOrg = pGC->clipOrg;
    portPriv->pCompositeClip = pGC->pCompositeClip;
    portPriv->FreeCompositeClip = FALSE;
    portPriv->subWindowMode = pGC->subWindowMode;
}

static int
KdXVRegetVideo(XvPortRecPrivatePtr portPriv)
{
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    int ret = Success;
    Bool clippedAway = FALSE;

    KdXVUpdateCompositeClip(portPriv);

    /* translate the video region to the screen */
    WinBox.x1 = portPriv->pDraw->x + portPriv->drw_x;
    WinBox.y1 = portPriv->pDraw->y + portPriv->drw_y;
    WinBox.x2 = WinBox.x1 + portPriv->drw_w;
    WinBox.y2 = WinBox.y1 + portPriv->drw_h;

    /* clip to the window composite clip */
    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, portPriv->pCompositeClip);

    /* that's all if it's totally obscured */
    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto CLIP_VIDEO_BAILOUT;
    }

    ret = (*portPriv->AdaptorRec->GetVideo) (portPriv->screen, portPriv->pDraw,
                                             portPriv->vid_x, portPriv->vid_y,
                                             WinBox.x1, WinBox.y1,
                                             portPriv->vid_w, portPriv->vid_h,
                                             portPriv->drw_w, portPriv->drw_h,
                                             &ClipRegion,
                                             portPriv->DevPriv.ptr);

    if (ret == Success)
        portPriv->isOn = XV_ON;

 CLIP_VIDEO_BAILOUT:

    if ((clippedAway || (ret != Success)) && portPriv->isOn == XV_ON) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    /* This clip was copied and only good for one shot */
    if (!portPriv->FreeCompositeClip)
        portPriv->pCompositeClip = NULL;

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVReputVideo(XvPortRecPrivatePtr portPriv)
{
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    ScreenPtr pScreen = portPriv->pDraw->pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    int ret = Success;
    Bool clippedAway = FALSE;

    KdXVUpdateCompositeClip(portPriv);

    /* translate the video region to the screen */
    WinBox.x1 = portPriv->pDraw->x + portPriv->drw_x;
    WinBox.y1 = portPriv->pDraw->y + portPriv->drw_y;
    WinBox.x2 = WinBox.x1 + portPriv->drw_w;
    WinBox.y2 = WinBox.y1 + portPriv->drw_h;

    /* clip to the window composite clip */
    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, portPriv->pCompositeClip);

    /* clip and translate to the viewport */
    if (portPriv->AdaptorRec->flags & VIDEO_CLIP_TO_VIEWPORT) {
        RegionRec VPReg;
        BoxRec VPBox;

        VPBox.x1 = 0;
        VPBox.y1 = 0;
        VPBox.x2 = screen->width;
        VPBox.y2 = screen->height;

        RegionInit(&VPReg, &VPBox, 1);
        RegionIntersect(&ClipRegion, &ClipRegion, &VPReg);
        RegionUninit(&VPReg);
    }

    /* that's all if it's totally obscured */
    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto CLIP_VIDEO_BAILOUT;
    }

    ret = (*portPriv->AdaptorRec->PutVideo) (portPriv->screen, portPriv->pDraw,
                                             portPriv->vid_x, portPriv->vid_y,
                                             WinBox.x1, WinBox.y1,
                                             portPriv->vid_w, portPriv->vid_h,
                                             portPriv->drw_w, portPriv->drw_h,
                                             &ClipRegion,
                                             portPriv->DevPriv.ptr);

    if (ret == Success)
        portPriv->isOn = XV_ON;

 CLIP_VIDEO_BAILOUT:

    if ((clippedAway || (ret != Success)) && (portPriv->isOn == XV_ON)) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    /* This clip was copied and only good for one shot */
    if (!portPriv->FreeCompositeClip)
        portPriv->pCompositeClip = NULL;

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVReputImage(XvPortRecPrivatePtr portPriv)
{
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    ScreenPtr pScreen = portPriv->pDraw->pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    int ret = Success;
    Bool clippedAway = FALSE;

    KdXVUpdateCompositeClip(portPriv);

    /* translate the video region to the screen */
    WinBox.x1 = portPriv->pDraw->x + portPriv->drw_x;
    WinBox.y1 = portPriv->pDraw->y + portPriv->drw_y;
    WinBox.x2 = WinBox.x1 + portPriv->drw_w;
    WinBox.y2 = WinBox.y1 + portPriv->drw_h;

    /* clip to the window composite clip */
    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, portPriv->pCompositeClip);

    /* clip and translate to the viewport */
    if (portPriv->AdaptorRec->flags & VIDEO_CLIP_TO_VIEWPORT) {
        RegionRec VPReg;
        BoxRec VPBox;

        VPBox.x1 = 0;
        VPBox.y1 = 0;
        VPBox.x2 = screen->width;
        VPBox.y2 = screen->height;

        RegionInit(&VPReg, &VPBox, 1);
        RegionIntersect(&ClipRegion, &ClipRegion, &VPReg);
        RegionUninit(&VPReg);
    }

    /* that's all if it's totally obscured */
    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto CLIP_VIDEO_BAILOUT;
    }

    ret =
        (*portPriv->AdaptorRec->ReputImage) (portPriv->screen, portPriv->pDraw,
                                             WinBox.x1, WinBox.y1, &ClipRegion,
                                             portPriv->DevPriv.ptr);

    portPriv->isOn = (ret == Success) ? XV_ON : XV_OFF;

 CLIP_VIDEO_BAILOUT:

    if ((clippedAway || (ret != Success)) && (portPriv->isOn == XV_ON)) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    /* This clip was copied and only good for one shot */
    if (!portPriv->FreeCompositeClip)
        portPriv->pCompositeClip = NULL;

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVEnlistPortInWindow(WindowPtr pWin, XvPortRecPrivatePtr portPriv)
{
    KdXVWindowPtr winPriv, PrivRoot;

    winPriv = PrivRoot = GET_KDXV_WINDOW(pWin);

    /* Enlist our port in the window private */
    while (winPriv) {
        if (winPriv->PortRec == portPriv)       /* we're already listed */
            break;
        winPriv = winPriv->next;
    }

    if (!winPriv) {
        winPriv = malloc(sizeof(KdXVWindowRec));
        if (!winPriv)
            return BadAlloc;
        winPriv->PortRec = portPriv;
        winPriv->next = PrivRoot;
        dixSetPrivate(&pWin->devPrivates, KdXVWindowKey, winPriv);
    }
    return Success;
}

static void
KdXVRemovePortFromWindow(WindowPtr pWin, XvPortRecPrivatePtr portPriv)
{
    KdXVWindowPtr winPriv, prevPriv = NULL;

    winPriv = GET_KDXV_WINDOW(pWin);

    while (winPriv) {
        if (winPriv->PortRec == portPriv) {
            if (prevPriv)
                prevPriv->next = winPriv->next;
            else
                dixSetPrivate(&pWin->devPrivates, KdXVWindowKey, winPriv->next);
            free(winPriv);
            break;
        }
        prevPriv = winPriv;
        winPriv = winPriv->next;
    }
    portPriv->pDraw = NULL;
}

/****  ScreenRec fields ****/

static Bool
KdXVDestroyWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    KdXVScreenPtr ScreenPriv = GET_KDXV_SCREEN(pScreen);
    KdXVWindowPtr tmp, WinPriv = GET_KDXV_WINDOW(pWin);
    int ret;

    while (WinPriv) {
        XvPortRecPrivatePtr pPriv = WinPriv->PortRec;

        if (pPriv->isOn > XV_OFF) {
            (*pPriv->AdaptorRec->StopVideo) (pPriv->screen, pPriv->DevPriv.ptr,
                                             TRUE);
            pPriv->isOn = XV_OFF;
        }

        pPriv->pDraw = NULL;
        tmp = WinPriv;
        WinPriv = WinPriv->next;
        free(tmp);
    }

    dixSetPrivate(&pWin->devPrivates, KdXVWindowKey, NULL);

    pScreen->DestroyWindow = ScreenPriv->DestroyWindow;
    ret = (*pScreen->DestroyWindow) (pWin);
    pScreen->DestroyWindow = KdXVDestroyWindow;

    return ret;
}

static void
KdXVWindowExposures(WindowPtr pWin, RegionPtr reg1)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    KdXVScreenPtr ScreenPriv = GET_KDXV_SCREEN(pScreen);
    KdXVWindowPtr WinPriv = GET_KDXV_WINDOW(pWin);
    KdXVWindowPtr pPrev;
    XvPortRecPrivatePtr pPriv;
    Bool AreasExposed;

    AreasExposed = (WinPriv && reg1 && RegionNotEmpty(reg1));

    pScreen->WindowExposures = ScreenPriv->WindowExposures;
    (*pScreen->WindowExposures) (pWin, reg1);
    pScreen->WindowExposures = KdXVWindowExposures;

    /* filter out XClearWindow/Area */
    if (!pWin->valdata)
        return;

    pPrev = NULL;

    while (WinPriv) {
        pPriv = WinPriv->PortRec;

        /* Reput anyone with a reput function */

        switch (pPriv->type) {
        case XvInputMask:
            KdXVReputVideo(pPriv);
            break;
        case XvOutputMask:
            KdXVRegetVideo(pPriv);
            break;
        default:               /* overlaid still/image */
            if (pPriv->AdaptorRec->ReputImage)
                KdXVReputImage(pPriv);
            else if (AreasExposed) {
                KdXVWindowPtr tmp;

                if (pPriv->isOn == XV_ON) {
                    (*pPriv->AdaptorRec->StopVideo) (pPriv->screen,
                                                     pPriv->DevPriv.ptr, FALSE);
                    pPriv->isOn = XV_PENDING;
                }
                pPriv->pDraw = NULL;

                if (!pPrev)
                    dixSetPrivate(&pWin->devPrivates, KdXVWindowKey,
                                  WinPriv->next);
                else
                    pPrev->next = WinPriv->next;
                tmp = WinPriv;
                WinPriv = WinPriv->next;
                free(tmp);
                continue;
            }
            break;
        }
        pPrev = WinPriv;
        WinPriv = WinPriv->next;
    }
}

static void
KdXVClipNotify(WindowPtr pWin, int dx, int dy)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    KdXVScreenPtr ScreenPriv = GET_KDXV_SCREEN(pScreen);
    KdXVWindowPtr WinPriv = GET_KDXV_WINDOW(pWin);
    KdXVWindowPtr tmp, pPrev = NULL;
    XvPortRecPrivatePtr pPriv;
    Bool visible = (pWin->visibility == VisibilityUnobscured) ||
        (pWin->visibility == VisibilityPartiallyObscured);

    while (WinPriv) {
        pPriv = WinPriv->PortRec;

        if (pPriv->pCompositeClip && pPriv->FreeCompositeClip)
            RegionDestroy(pPriv->pCompositeClip);

        pPriv->pCompositeClip = NULL;

        /* Stop everything except images, but stop them too if the
           window isn't visible.  But we only remove the images. */

        if (pPriv->type || !visible) {
            if (pPriv->isOn == XV_ON) {
                (*pPriv->AdaptorRec->StopVideo) (pPriv->screen,
                                                 pPriv->DevPriv.ptr, FALSE);
                pPriv->isOn = XV_PENDING;
            }

            if (!pPriv->type) { /* overlaid still/image */
                pPriv->pDraw = NULL;

                if (!pPrev)
                    dixSetPrivate(&pWin->devPrivates, KdXVWindowKey,
                                  WinPriv->next);
                else
                    pPrev->next = WinPriv->next;
                tmp = WinPriv;
                WinPriv = WinPriv->next;
                free(tmp);
                continue;
            }
        }

        pPrev = WinPriv;
        WinPriv = WinPriv->next;
    }

    if (ScreenPriv->ClipNotify) {
        pScreen->ClipNotify = ScreenPriv->ClipNotify;
        (*pScreen->ClipNotify) (pWin, dx, dy);
        pScreen->ClipNotify = KdXVClipNotify;
    }
}

/**** Required XvScreenRec fields ****/

static Bool
KdXVCloseScreen(ScreenPtr pScreen)
{
    XvScreenPtr pxvs = GET_XV_SCREEN(pScreen);
    KdXVScreenPtr ScreenPriv = GET_KDXV_SCREEN(pScreen);
    XvAdaptorPtr pa;
    int c;

    if (!ScreenPriv)
        return TRUE;

    pScreen->DestroyWindow = ScreenPriv->DestroyWindow;
    pScreen->WindowExposures = ScreenPriv->WindowExposures;
    pScreen->ClipNotify = ScreenPriv->ClipNotify;
    pScreen->CloseScreen = ScreenPriv->CloseScreen;

/*   fprintf(stderr,"XV: Unwrapping screen funcs\n"); */

    for (c = 0, pa = pxvs->pAdaptors; c < pxvs->nAdaptors; c++, pa++) {
        KdXVFreeAdaptor(pa);
    }

    free(pxvs->pAdaptors);
    free(ScreenPriv);

    return pScreen->CloseScreen(pScreen);
}

/**** XvAdaptorRec fields ****/

static int
KdXVPutVideo(DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    KdScreenPriv(portPriv->screen->pScreen);
    int result;

    /* No dumping video to pixmaps... For now anyhow */
    if (pDraw->type != DRAWABLE_WINDOW) {
        pPort->pDraw = (DrawablePtr) NULL;
        return BadAlloc;
    }

    /* If we are changing windows, unregister our port in the old window */
    if (portPriv->pDraw && (portPriv->pDraw != pDraw))
        KdXVRemovePortFromWindow((WindowPtr) (portPriv->pDraw), portPriv);

    /* Register our port with the new window */
    result = KdXVEnlistPortInWindow((WindowPtr) pDraw, portPriv);
    if (result != Success)
        return result;

    portPriv->pDraw = pDraw;
    portPriv->type = XvInputMask;

    /* save a copy of these parameters */
    portPriv->vid_x = vid_x;
    portPriv->vid_y = vid_y;
    portPriv->vid_w = vid_w;
    portPriv->vid_h = vid_h;
    portPriv->drw_x = drw_x;
    portPriv->drw_y = drw_y;
    portPriv->drw_w = drw_w;
    portPriv->drw_h = drw_h;

    /* make sure we have the most recent copy of the clientClip */
    KdXVCopyClip(portPriv, pGC);

    /* To indicate to the DI layer that we were successful */
    pPort->pDraw = pDraw;

    if (!pScreenPriv->enabled)
        return Success;

    return (KdXVReputVideo(portPriv));
}

static int
KdXVPutStill(DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);
    ScreenPtr pScreen = pDraw->pScreen;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    int ret = Success;
    Bool clippedAway = FALSE;

    if (pDraw->type != DRAWABLE_WINDOW)
        return BadAlloc;

    if (!pScreenPriv->enabled)
        return Success;

    WinBox.x1 = pDraw->x + drw_x;
    WinBox.y1 = pDraw->y + drw_y;
    WinBox.x2 = WinBox.x1 + drw_w;
    WinBox.y2 = WinBox.y1 + drw_h;

    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, pGC->pCompositeClip);

    if (portPriv->AdaptorRec->flags & VIDEO_CLIP_TO_VIEWPORT) {
        RegionRec VPReg;
        BoxRec VPBox;

        VPBox.x1 = 0;
        VPBox.y1 = 0;
        VPBox.x2 = screen->width;
        VPBox.y2 = screen->height;

        RegionInit(&VPReg, &VPBox, 1);
        RegionIntersect(&ClipRegion, &ClipRegion, &VPReg);
        RegionUninit(&VPReg);
    }

    if (portPriv->pDraw) {
        KdXVRemovePortFromWindow((WindowPtr) (portPriv->pDraw), portPriv);
    }

    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto PUT_STILL_BAILOUT;
    }

    ret = (*portPriv->AdaptorRec->PutStill) (portPriv->screen, pDraw,
                                             vid_x, vid_y, WinBox.x1, WinBox.y1,
                                             vid_w, vid_h, drw_w, drw_h,
                                             &ClipRegion,
                                             portPriv->DevPriv.ptr);

    if ((ret == Success) &&
        (portPriv->AdaptorRec->flags & VIDEO_OVERLAID_STILLS)) {

        KdXVEnlistPortInWindow((WindowPtr) pDraw, portPriv);
        portPriv->isOn = XV_ON;
        portPriv->pDraw = pDraw;
        portPriv->drw_x = drw_x;
        portPriv->drw_y = drw_y;
        portPriv->drw_w = drw_w;
        portPriv->drw_h = drw_h;
        portPriv->type = 0;     /* no mask means it's transient and should
                                   not be reput once it's removed */
        pPort->pDraw = pDraw;   /* make sure we can get stop requests */
    }

 PUT_STILL_BAILOUT:

    if ((clippedAway || (ret != Success)) && (portPriv->isOn == XV_ON)) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVGetVideo(DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);
    int result;

    KdScreenPriv(portPriv->screen->pScreen);

    /* No pixmaps... For now anyhow */
    if (pDraw->type != DRAWABLE_WINDOW) {
        pPort->pDraw = (DrawablePtr) NULL;
        return BadAlloc;
    }

    /* If we are changing windows, unregister our port in the old window */
    if (portPriv->pDraw && (portPriv->pDraw != pDraw))
        KdXVRemovePortFromWindow((WindowPtr) (portPriv->pDraw), portPriv);

    /* Register our port with the new window */
    result = KdXVEnlistPortInWindow((WindowPtr) pDraw, portPriv);
    if (result != Success)
        return result;

    portPriv->pDraw = pDraw;
    portPriv->type = XvOutputMask;

    /* save a copy of these parameters */
    portPriv->vid_x = vid_x;
    portPriv->vid_y = vid_y;
    portPriv->vid_w = vid_w;
    portPriv->vid_h = vid_h;
    portPriv->drw_x = drw_x;
    portPriv->drw_y = drw_y;
    portPriv->drw_w = drw_w;
    portPriv->drw_h = drw_h;

    /* make sure we have the most recent copy of the clientClip */
    KdXVCopyClip(portPriv, pGC);

    /* To indicate to the DI layer that we were successful */
    pPort->pDraw = pDraw;

    if (!pScreenPriv->enabled)
        return Success;

    return (KdXVRegetVideo(portPriv));
}

static int
KdXVGetStill(DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);
    ScreenPtr pScreen = pDraw->pScreen;

    KdScreenPriv(pScreen);
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    int ret = Success;
    Bool clippedAway = FALSE;

    if (pDraw->type != DRAWABLE_WINDOW)
        return BadAlloc;

    if (!pScreenPriv->enabled)
        return Success;

    WinBox.x1 = pDraw->x + drw_x;
    WinBox.y1 = pDraw->y + drw_y;
    WinBox.x2 = WinBox.x1 + drw_w;
    WinBox.y2 = WinBox.y1 + drw_h;

    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, pGC->pCompositeClip);

    if (portPriv->pDraw) {
        KdXVRemovePortFromWindow((WindowPtr) (portPriv->pDraw), portPriv);
    }

    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto GET_STILL_BAILOUT;
    }

    ret = (*portPriv->AdaptorRec->GetStill) (portPriv->screen, pDraw,
                                             vid_x, vid_y, WinBox.x1, WinBox.y1,
                                             vid_w, vid_h, drw_w, drw_h,
                                             &ClipRegion,
                                             portPriv->DevPriv.ptr);

 GET_STILL_BAILOUT:

    if ((clippedAway || (ret != Success)) && (portPriv->isOn == XV_ON)) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVStopVideo(XvPortPtr pPort, DrawablePtr pDraw)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    KdScreenPriv(portPriv->screen->pScreen);

    if (pDraw->type != DRAWABLE_WINDOW)
        return BadAlloc;

    KdXVRemovePortFromWindow((WindowPtr) pDraw, portPriv);

    if (!pScreenPriv->enabled)
        return Success;

    /* Must free resources. */

    if (portPriv->isOn > XV_OFF) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, TRUE);
        portPriv->isOn = XV_OFF;
    }

    return Success;
}

static int
KdXVSetPortAttribute(XvPortPtr pPort, Atom attribute, INT32 value)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    return ((*portPriv->AdaptorRec->SetPortAttribute) (portPriv->screen,
                                                       attribute, value,
                                                       portPriv->DevPriv.ptr));
}

static int
KdXVGetPortAttribute(XvPortPtr pPort, Atom attribute, INT32 *p_value)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    return ((*portPriv->AdaptorRec->GetPortAttribute) (portPriv->screen,
                                                       attribute,
                                                       (int *) p_value,
                                                       portPriv->DevPriv.ptr));
}

static int
KdXVQueryBestSize(XvPortPtr pPort,
                  CARD8 motion,
                  CARD16 vid_w, CARD16 vid_h,
                  CARD16 drw_w, CARD16 drw_h,
                  unsigned int *p_w, unsigned int *p_h)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    (*portPriv->AdaptorRec->QueryBestSize) (portPriv->screen,
                                            (Bool) motion, vid_w, vid_h, drw_w,
                                            drw_h, p_w, p_h,
                                            portPriv->DevPriv.ptr);

    return Success;
}

static int
KdXVPutImage(DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 src_x, INT16 src_y,
             CARD16 src_w, CARD16 src_h,
             INT16 drw_x, INT16 drw_y,
             CARD16 drw_w, CARD16 drw_h,
             XvImagePtr format,
             unsigned char *data, Bool sync, CARD16 width, CARD16 height)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);
    ScreenPtr pScreen = pDraw->pScreen;

    KdScreenPriv(pScreen);
    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    int ret = Success;
    Bool clippedAway = FALSE;

    if (pDraw->type != DRAWABLE_WINDOW)
        return BadAlloc;

    if (!pScreenPriv->enabled)
        return Success;

    WinBox.x1 = pDraw->x + drw_x;
    WinBox.y1 = pDraw->y + drw_y;
    WinBox.x2 = WinBox.x1 + drw_w;
    WinBox.y2 = WinBox.y1 + drw_h;

    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, pGC->pCompositeClip);

    if (portPriv->AdaptorRec->flags & VIDEO_CLIP_TO_VIEWPORT) {
        RegionRec VPReg;
        BoxRec VPBox;

        VPBox.x1 = 0;
        VPBox.y1 = 0;
        VPBox.x2 = pScreen->width;
        VPBox.y2 = pScreen->height;

        RegionInit(&VPReg, &VPBox, 1);
        RegionIntersect(&ClipRegion, &ClipRegion, &VPReg);
        RegionUninit(&VPReg);
    }

    if (portPriv->pDraw) {
        KdXVRemovePortFromWindow((WindowPtr) (portPriv->pDraw), portPriv);
    }

    if (!RegionNotEmpty(&ClipRegion)) {
        clippedAway = TRUE;
        goto PUT_IMAGE_BAILOUT;
    }

    ret = (*portPriv->AdaptorRec->PutImage) (portPriv->screen, pDraw,
                                             src_x, src_y, WinBox.x1, WinBox.y1,
                                             src_w, src_h, drw_w, drw_h,
                                             format->id, data, width, height,
                                             sync, &ClipRegion,
                                             portPriv->DevPriv.ptr);

    if ((ret == Success) &&
        (portPriv->AdaptorRec->flags & VIDEO_OVERLAID_IMAGES)) {

        KdXVEnlistPortInWindow((WindowPtr) pDraw, portPriv);
        portPriv->isOn = XV_ON;
        portPriv->pDraw = pDraw;
        portPriv->drw_x = drw_x;
        portPriv->drw_y = drw_y;
        portPriv->drw_w = drw_w;
        portPriv->drw_h = drw_h;
        portPriv->type = 0;     /* no mask means it's transient and should
                                   not be reput once it's removed */
        pPort->pDraw = pDraw;   /* make sure we can get stop requests */
    }

 PUT_IMAGE_BAILOUT:

    if ((clippedAway || (ret != Success)) && (portPriv->isOn == XV_ON)) {
        (*portPriv->AdaptorRec->StopVideo) (portPriv->screen,
                                            portPriv->DevPriv.ptr, FALSE);
        portPriv->isOn = XV_PENDING;
    }

    RegionUninit(&WinRegion);
    RegionUninit(&ClipRegion);

    return ret;
}

static int
KdXVQueryImageAttributes(XvPortPtr pPort,
                         XvImagePtr format,
                         CARD16 *width,
                         CARD16 *height, int *pitches, int *offsets)
{
    XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr) (pPort->devPriv.ptr);

    return (*portPriv->AdaptorRec->QueryImageAttributes) (portPriv->screen,
                                                          format->id, width,
                                                          height, pitches,
                                                          offsets);
}
