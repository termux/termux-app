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

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "extnsionst.h"
#include "extinit.h"
#include "servermd.h"
#include <X11/extensions/render.h>
#include <X11/extensions/renderproto.h>
#include "picturestr.h"
#include "glyphstr.h"
#include <X11/Xfuncproto.h>
#include "cursorstr.h"
#include "xace.h"
#include "protocol-versions.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

#include <stdint.h>

static int ProcRenderQueryVersion(ClientPtr pClient);
static int ProcRenderQueryPictFormats(ClientPtr pClient);
static int ProcRenderQueryPictIndexValues(ClientPtr pClient);
static int ProcRenderQueryDithers(ClientPtr pClient);
static int ProcRenderCreatePicture(ClientPtr pClient);
static int ProcRenderChangePicture(ClientPtr pClient);
static int ProcRenderSetPictureClipRectangles(ClientPtr pClient);
static int ProcRenderFreePicture(ClientPtr pClient);
static int ProcRenderComposite(ClientPtr pClient);
static int ProcRenderScale(ClientPtr pClient);
static int ProcRenderTrapezoids(ClientPtr pClient);
static int ProcRenderTriangles(ClientPtr pClient);
static int ProcRenderTriStrip(ClientPtr pClient);
static int ProcRenderTriFan(ClientPtr pClient);
static int ProcRenderColorTrapezoids(ClientPtr pClient);
static int ProcRenderColorTriangles(ClientPtr pClient);
static int ProcRenderTransform(ClientPtr pClient);
static int ProcRenderCreateGlyphSet(ClientPtr pClient);
static int ProcRenderReferenceGlyphSet(ClientPtr pClient);
static int ProcRenderFreeGlyphSet(ClientPtr pClient);
static int ProcRenderAddGlyphs(ClientPtr pClient);
static int ProcRenderAddGlyphsFromPicture(ClientPtr pClient);
static int ProcRenderFreeGlyphs(ClientPtr pClient);
static int ProcRenderCompositeGlyphs(ClientPtr pClient);
static int ProcRenderFillRectangles(ClientPtr pClient);
static int ProcRenderCreateCursor(ClientPtr pClient);
static int ProcRenderSetPictureTransform(ClientPtr pClient);
static int ProcRenderQueryFilters(ClientPtr pClient);
static int ProcRenderSetPictureFilter(ClientPtr pClient);
static int ProcRenderCreateAnimCursor(ClientPtr pClient);
static int ProcRenderAddTraps(ClientPtr pClient);
static int ProcRenderCreateSolidFill(ClientPtr pClient);
static int ProcRenderCreateLinearGradient(ClientPtr pClient);
static int ProcRenderCreateRadialGradient(ClientPtr pClient);
static int ProcRenderCreateConicalGradient(ClientPtr pClient);

static int ProcRenderDispatch(ClientPtr pClient);

static int SProcRenderQueryVersion(ClientPtr pClient);
static int SProcRenderQueryPictFormats(ClientPtr pClient);
static int SProcRenderQueryPictIndexValues(ClientPtr pClient);
static int SProcRenderQueryDithers(ClientPtr pClient);
static int SProcRenderCreatePicture(ClientPtr pClient);
static int SProcRenderChangePicture(ClientPtr pClient);
static int SProcRenderSetPictureClipRectangles(ClientPtr pClient);
static int SProcRenderFreePicture(ClientPtr pClient);
static int SProcRenderComposite(ClientPtr pClient);
static int SProcRenderScale(ClientPtr pClient);
static int SProcRenderTrapezoids(ClientPtr pClient);
static int SProcRenderTriangles(ClientPtr pClient);
static int SProcRenderTriStrip(ClientPtr pClient);
static int SProcRenderTriFan(ClientPtr pClient);
static int SProcRenderColorTrapezoids(ClientPtr pClient);
static int SProcRenderColorTriangles(ClientPtr pClient);
static int SProcRenderTransform(ClientPtr pClient);
static int SProcRenderCreateGlyphSet(ClientPtr pClient);
static int SProcRenderReferenceGlyphSet(ClientPtr pClient);
static int SProcRenderFreeGlyphSet(ClientPtr pClient);
static int SProcRenderAddGlyphs(ClientPtr pClient);
static int SProcRenderAddGlyphsFromPicture(ClientPtr pClient);
static int SProcRenderFreeGlyphs(ClientPtr pClient);
static int SProcRenderCompositeGlyphs(ClientPtr pClient);
static int SProcRenderFillRectangles(ClientPtr pClient);
static int SProcRenderCreateCursor(ClientPtr pClient);
static int SProcRenderSetPictureTransform(ClientPtr pClient);
static int SProcRenderQueryFilters(ClientPtr pClient);
static int SProcRenderSetPictureFilter(ClientPtr pClient);
static int SProcRenderCreateAnimCursor(ClientPtr pClient);
static int SProcRenderAddTraps(ClientPtr pClient);
static int SProcRenderCreateSolidFill(ClientPtr pClient);
static int SProcRenderCreateLinearGradient(ClientPtr pClient);
static int SProcRenderCreateRadialGradient(ClientPtr pClient);
static int SProcRenderCreateConicalGradient(ClientPtr pClient);

static int SProcRenderDispatch(ClientPtr pClient);

int (*ProcRenderVector[RenderNumberRequests]) (ClientPtr) = {
ProcRenderQueryVersion,
        ProcRenderQueryPictFormats,
        ProcRenderQueryPictIndexValues,
        ProcRenderQueryDithers,
        ProcRenderCreatePicture,
        ProcRenderChangePicture,
        ProcRenderSetPictureClipRectangles,
        ProcRenderFreePicture,
        ProcRenderComposite,
        ProcRenderScale,
        ProcRenderTrapezoids,
        ProcRenderTriangles,
        ProcRenderTriStrip,
        ProcRenderTriFan,
        ProcRenderColorTrapezoids,
        ProcRenderColorTriangles,
        ProcRenderTransform,
        ProcRenderCreateGlyphSet,
        ProcRenderReferenceGlyphSet,
        ProcRenderFreeGlyphSet,
        ProcRenderAddGlyphs,
        ProcRenderAddGlyphsFromPicture,
        ProcRenderFreeGlyphs,
        ProcRenderCompositeGlyphs,
        ProcRenderCompositeGlyphs,
        ProcRenderCompositeGlyphs,
        ProcRenderFillRectangles,
        ProcRenderCreateCursor,
        ProcRenderSetPictureTransform,
        ProcRenderQueryFilters,
        ProcRenderSetPictureFilter,
        ProcRenderCreateAnimCursor,
        ProcRenderAddTraps,
        ProcRenderCreateSolidFill,
        ProcRenderCreateLinearGradient,
        ProcRenderCreateRadialGradient, ProcRenderCreateConicalGradient};

int (*SProcRenderVector[RenderNumberRequests]) (ClientPtr) = {
SProcRenderQueryVersion,
        SProcRenderQueryPictFormats,
        SProcRenderQueryPictIndexValues,
        SProcRenderQueryDithers,
        SProcRenderCreatePicture,
        SProcRenderChangePicture,
        SProcRenderSetPictureClipRectangles,
        SProcRenderFreePicture,
        SProcRenderComposite,
        SProcRenderScale,
        SProcRenderTrapezoids,
        SProcRenderTriangles,
        SProcRenderTriStrip,
        SProcRenderTriFan,
        SProcRenderColorTrapezoids,
        SProcRenderColorTriangles,
        SProcRenderTransform,
        SProcRenderCreateGlyphSet,
        SProcRenderReferenceGlyphSet,
        SProcRenderFreeGlyphSet,
        SProcRenderAddGlyphs,
        SProcRenderAddGlyphsFromPicture,
        SProcRenderFreeGlyphs,
        SProcRenderCompositeGlyphs,
        SProcRenderCompositeGlyphs,
        SProcRenderCompositeGlyphs,
        SProcRenderFillRectangles,
        SProcRenderCreateCursor,
        SProcRenderSetPictureTransform,
        SProcRenderQueryFilters,
        SProcRenderSetPictureFilter,
        SProcRenderCreateAnimCursor,
        SProcRenderAddTraps,
        SProcRenderCreateSolidFill,
        SProcRenderCreateLinearGradient,
        SProcRenderCreateRadialGradient, SProcRenderCreateConicalGradient};

int RenderErrBase;
static DevPrivateKeyRec RenderClientPrivateKeyRec;

#define RenderClientPrivateKey (&RenderClientPrivateKeyRec )

typedef struct _RenderClient {
    int major_version;
    int minor_version;
} RenderClientRec, *RenderClientPtr;

#define GetRenderClient(pClient) ((RenderClientPtr)dixLookupPrivate(&(pClient)->devPrivates, RenderClientPrivateKey))

#ifdef PANORAMIX
RESTYPE XRT_PICTURE;
#endif

void
RenderExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!PictureType)
        return;
    if (!PictureFinishInit())
        return;
    if (!dixRegisterPrivateKey
        (&RenderClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(RenderClientRec)))
        return;

    extEntry = AddExtension(RENDER_NAME, 0, RenderNumberErrors,
                            ProcRenderDispatch, SProcRenderDispatch,
                            NULL, StandardMinorOpcode);
    if (!extEntry)
        return;
    RenderErrBase = extEntry->errorBase;
#ifdef PANORAMIX
    if (XRT_PICTURE)
        SetResourceTypeErrorValue(XRT_PICTURE, RenderErrBase + BadPicture);
#endif
    SetResourceTypeErrorValue(PictureType, RenderErrBase + BadPicture);
    SetResourceTypeErrorValue(PictFormatType, RenderErrBase + BadPictFormat);
    SetResourceTypeErrorValue(GlyphSetType, RenderErrBase + BadGlyphSet);
}

static int
ProcRenderQueryVersion(ClientPtr client)
{
    RenderClientPtr pRenderClient = GetRenderClient(client);
    xRenderQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    REQUEST(xRenderQueryVersionReq);

    REQUEST_SIZE_MATCH(xRenderQueryVersionReq);

    pRenderClient->major_version = stuff->majorVersion;
    pRenderClient->minor_version = stuff->minorVersion;

    if ((stuff->majorVersion * 1000 + stuff->minorVersion) <
        (SERVER_RENDER_MAJOR_VERSION * 1000 + SERVER_RENDER_MINOR_VERSION)) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }
    else {
        rep.majorVersion = SERVER_RENDER_MAJOR_VERSION;
        rep.minorVersion = SERVER_RENDER_MINOR_VERSION;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xRenderQueryVersionReply), &rep);
    return Success;
}

static VisualPtr
findVisual(ScreenPtr pScreen, VisualID vid)
{
    VisualPtr pVisual;
    int v;

    for (v = 0; v < pScreen->numVisuals; v++) {
        pVisual = pScreen->visuals + v;
        if (pVisual->vid == vid)
            return pVisual;
    }
    return 0;
}

static int
ProcRenderQueryPictFormats(ClientPtr client)
{
    RenderClientPtr pRenderClient = GetRenderClient(client);
    xRenderQueryPictFormatsReply *reply;
    xPictScreen *pictScreen;
    xPictDepth *pictDepth;
    xPictVisual *pictVisual;
    xPictFormInfo *pictForm;
    CARD32 *pictSubpixel;
    ScreenPtr pScreen;
    VisualPtr pVisual;
    DepthPtr pDepth;
    int v, d;
    PictureScreenPtr ps;
    PictFormatPtr pFormat;
    int nformat;
    int ndepth;
    int nvisual;
    int rlength;
    int s;
    int numScreens;
    int numSubpixel;

/*    REQUEST(xRenderQueryPictFormatsReq); */

    REQUEST_SIZE_MATCH(xRenderQueryPictFormatsReq);

#ifdef PANORAMIX
    if (noPanoramiXExtension)
        numScreens = screenInfo.numScreens;
    else
        numScreens = ((xConnSetup *) ConnectionInfo)->numRoots;
#else
    numScreens = screenInfo.numScreens;
#endif
    ndepth = nformat = nvisual = 0;
    for (s = 0; s < numScreens; s++) {
        pScreen = screenInfo.screens[s];
        for (d = 0; d < pScreen->numDepths; d++) {
            pDepth = pScreen->allowedDepths + d;
            ++ndepth;

            for (v = 0; v < pDepth->numVids; v++) {
                pVisual = findVisual(pScreen, pDepth->vids[v]);
                if (pVisual &&
                    PictureMatchVisual(pScreen, pDepth->depth, pVisual))
                    ++nvisual;
            }
        }
        ps = GetPictureScreenIfSet(pScreen);
        if (ps)
            nformat += ps->nformats;
    }
    if (pRenderClient->major_version == 0 && pRenderClient->minor_version < 6)
        numSubpixel = 0;
    else
        numSubpixel = numScreens;

    rlength = (sizeof(xRenderQueryPictFormatsReply) +
               nformat * sizeof(xPictFormInfo) +
               numScreens * sizeof(xPictScreen) +
               ndepth * sizeof(xPictDepth) +
               nvisual * sizeof(xPictVisual) + numSubpixel * sizeof(CARD32));
    reply = (xRenderQueryPictFormatsReply *) calloc(1, rlength);
    if (!reply)
        return BadAlloc;
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = bytes_to_int32(rlength - sizeof(xGenericReply));
    reply->numFormats = nformat;
    reply->numScreens = numScreens;
    reply->numDepths = ndepth;
    reply->numVisuals = nvisual;
    reply->numSubpixel = numSubpixel;

    pictForm = (xPictFormInfo *) (reply + 1);

    for (s = 0; s < numScreens; s++) {
        pScreen = screenInfo.screens[s];
        ps = GetPictureScreenIfSet(pScreen);
        if (ps) {
            for (nformat = 0, pFormat = ps->formats;
                 nformat < ps->nformats; nformat++, pFormat++) {
                pictForm->id = pFormat->id;
                pictForm->type = pFormat->type;
                pictForm->depth = pFormat->depth;
                pictForm->direct.red = pFormat->direct.red;
                pictForm->direct.redMask = pFormat->direct.redMask;
                pictForm->direct.green = pFormat->direct.green;
                pictForm->direct.greenMask = pFormat->direct.greenMask;
                pictForm->direct.blue = pFormat->direct.blue;
                pictForm->direct.blueMask = pFormat->direct.blueMask;
                pictForm->direct.alpha = pFormat->direct.alpha;
                pictForm->direct.alphaMask = pFormat->direct.alphaMask;
                if (pFormat->type == PictTypeIndexed &&
                    pFormat->index.pColormap)
                    pictForm->colormap = pFormat->index.pColormap->mid;
                else
                    pictForm->colormap = None;
                if (client->swapped) {
                    swapl(&pictForm->id);
                    swaps(&pictForm->direct.red);
                    swaps(&pictForm->direct.redMask);
                    swaps(&pictForm->direct.green);
                    swaps(&pictForm->direct.greenMask);
                    swaps(&pictForm->direct.blue);
                    swaps(&pictForm->direct.blueMask);
                    swaps(&pictForm->direct.alpha);
                    swaps(&pictForm->direct.alphaMask);
                    swapl(&pictForm->colormap);
                }
                pictForm++;
            }
        }
    }

    pictScreen = (xPictScreen *) pictForm;
    for (s = 0; s < numScreens; s++) {
        pScreen = screenInfo.screens[s];
        pictDepth = (xPictDepth *) (pictScreen + 1);
        ndepth = 0;
        for (d = 0; d < pScreen->numDepths; d++) {
            pictVisual = (xPictVisual *) (pictDepth + 1);
            pDepth = pScreen->allowedDepths + d;

            nvisual = 0;
            for (v = 0; v < pDepth->numVids; v++) {
                pVisual = findVisual(pScreen, pDepth->vids[v]);
                if (pVisual && (pFormat = PictureMatchVisual(pScreen,
                                                             pDepth->depth,
                                                             pVisual))) {
                    pictVisual->visual = pVisual->vid;
                    pictVisual->format = pFormat->id;
                    if (client->swapped) {
                        swapl(&pictVisual->visual);
                        swapl(&pictVisual->format);
                    }
                    pictVisual++;
                    nvisual++;
                }
            }
            pictDepth->depth = pDepth->depth;
            pictDepth->nPictVisuals = nvisual;
            if (client->swapped) {
                swaps(&pictDepth->nPictVisuals);
            }
            ndepth++;
            pictDepth = (xPictDepth *) pictVisual;
        }
        pictScreen->nDepth = ndepth;
        ps = GetPictureScreenIfSet(pScreen);
        if (ps)
            pictScreen->fallback = ps->fallback->id;
        else
            pictScreen->fallback = 0;
        if (client->swapped) {
            swapl(&pictScreen->nDepth);
            swapl(&pictScreen->fallback);
        }
        pictScreen = (xPictScreen *) pictDepth;
    }
    pictSubpixel = (CARD32 *) pictScreen;

    for (s = 0; s < numSubpixel; s++) {
        pScreen = screenInfo.screens[s];
        ps = GetPictureScreenIfSet(pScreen);
        if (ps)
            *pictSubpixel = ps->subpixel;
        else
            *pictSubpixel = SubPixelUnknown;
        if (client->swapped) {
            swapl(pictSubpixel);
        }
        ++pictSubpixel;
    }

    if (client->swapped) {
        swaps(&reply->sequenceNumber);
        swapl(&reply->length);
        swapl(&reply->numFormats);
        swapl(&reply->numScreens);
        swapl(&reply->numDepths);
        swapl(&reply->numVisuals);
        swapl(&reply->numSubpixel);
    }
    WriteToClient(client, rlength, reply);
    free(reply);
    return Success;
}

static int
ProcRenderQueryPictIndexValues(ClientPtr client)
{
    PictFormatPtr pFormat;
    int rc, num;
    int rlength;
    int i;

    REQUEST(xRenderQueryPictIndexValuesReq);
    xRenderQueryPictIndexValuesReply *reply;
    xIndexValue *values;

    REQUEST_AT_LEAST_SIZE(xRenderQueryPictIndexValuesReq);

    rc = dixLookupResourceByType((void **) &pFormat, stuff->format,
                                 PictFormatType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    if (pFormat->type != PictTypeIndexed) {
        client->errorValue = stuff->format;
        return BadMatch;
    }
    num = pFormat->index.nvalues;
    rlength = (sizeof(xRenderQueryPictIndexValuesReply) +
               num * sizeof(xIndexValue));
    reply = (xRenderQueryPictIndexValuesReply *) calloc(1, rlength);
    if (!reply)
        return BadAlloc;

    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = bytes_to_int32(rlength - sizeof(xGenericReply));
    reply->numIndexValues = num;

    values = (xIndexValue *) (reply + 1);

    memcpy(reply + 1, pFormat->index.pValues, num * sizeof(xIndexValue));

    if (client->swapped) {
        for (i = 0; i < num; i++) {
            swapl(&values[i].pixel);
            swaps(&values[i].red);
            swaps(&values[i].green);
            swaps(&values[i].blue);
            swaps(&values[i].alpha);
        }
        swaps(&reply->sequenceNumber);
        swapl(&reply->length);
        swapl(&reply->numIndexValues);
    }

    WriteToClient(client, rlength, reply);
    free(reply);
    return Success;
}

static int
ProcRenderQueryDithers(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderCreatePicture(ClientPtr client)
{
    PicturePtr pPicture;
    DrawablePtr pDrawable;
    PictFormatPtr pFormat;
    int len, error, rc;

    REQUEST(xRenderCreatePictureReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreatePictureReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);
    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess | DixAddAccess);
    if (rc != Success)
        return rc;

    rc = dixLookupResourceByType((void **) &pFormat, stuff->format,
                                 PictFormatType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    if (pFormat->depth != pDrawable->depth)
        return BadMatch;
    len = client->req_len - bytes_to_int32(sizeof(xRenderCreatePictureReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    pPicture = CreatePicture(stuff->pid,
                             pDrawable,
                             pFormat,
                             stuff->mask, (XID *) (stuff + 1), client, &error);
    if (!pPicture)
        return error;
    if (!AddResource(stuff->pid, PictureType, (void *) pPicture))
        return BadAlloc;
    return Success;
}

static int
ProcRenderChangePicture(ClientPtr client)
{
    PicturePtr pPicture;

    REQUEST(xRenderChangePictureReq);
    int len;

    REQUEST_AT_LEAST_SIZE(xRenderChangePictureReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixSetAttrAccess);

    len = client->req_len - bytes_to_int32(sizeof(xRenderChangePictureReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    return ChangePicture(pPicture, stuff->mask, (XID *) (stuff + 1),
                         (DevUnion *) 0, client);
}

static int
ProcRenderSetPictureClipRectangles(ClientPtr client)
{
    REQUEST(xRenderSetPictureClipRectanglesReq);
    PicturePtr pPicture;
    int nr;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureClipRectanglesReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixSetAttrAccess);
    if (!pPicture->pDrawable)
        return RenderErrBase + BadPicture;

    nr = (client->req_len << 2) - sizeof(xRenderSetPictureClipRectanglesReq);
    if (nr & 4)
        return BadLength;
    nr >>= 3;
    return SetPictureClipRects(pPicture,
                               stuff->xOrigin, stuff->yOrigin,
                               nr, (xRectangle *) &stuff[1]);
}

static int
ProcRenderFreePicture(ClientPtr client)
{
    PicturePtr pPicture;

    REQUEST(xRenderFreePictureReq);

    REQUEST_SIZE_MATCH(xRenderFreePictureReq);

    VERIFY_PICTURE(pPicture, stuff->picture, client, DixDestroyAccess);
    FreeResource(stuff->picture, RT_NONE);
    return Success;
}

static Bool
PictOpValid(CARD8 op)
{
    if ( /*PictOpMinimum <= op && */ op <= PictOpMaximum)
        return TRUE;
    if (PictOpDisjointMinimum <= op && op <= PictOpDisjointMaximum)
        return TRUE;
    if (PictOpConjointMinimum <= op && op <= PictOpConjointMaximum)
        return TRUE;
    if (PictOpBlendMinimum <= op && op <= PictOpBlendMaximum)
        return TRUE;
    return FALSE;
}

static int
ProcRenderComposite(ClientPtr client)
{
    PicturePtr pSrc, pMask, pDst;

    REQUEST(xRenderCompositeReq);

    REQUEST_SIZE_MATCH(xRenderCompositeReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_ALPHA(pMask, stuff->mask, client, DixReadAccess);
    if ((pSrc->pDrawable &&
         pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen) || (pMask &&
                                                                   pMask->
                                                                   pDrawable &&
                                                                   pDst->
                                                                   pDrawable->
                                                                   pScreen !=
                                                                   pMask->
                                                                   pDrawable->
                                                                   pScreen))
        return BadMatch;
    CompositePicture(stuff->op,
                     pSrc,
                     pMask,
                     pDst,
                     stuff->xSrc,
                     stuff->ySrc,
                     stuff->xMask,
                     stuff->yMask,
                     stuff->xDst, stuff->yDst, stuff->width, stuff->height);
    return Success;
}

static int
ProcRenderScale(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTrapezoids(ClientPtr client)
{
    int rc, ntraps;
    PicturePtr pSrc, pDst;
    PictFormatPtr pFormat;

    REQUEST(xRenderTrapezoidsReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrapezoidsReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
        return BadMatch;
    if (stuff->maskFormat) {
        rc = dixLookupResourceByType((void **) &pFormat, stuff->maskFormat,
                                     PictFormatType, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    else
        pFormat = 0;
    ntraps = (client->req_len << 2) - sizeof(xRenderTrapezoidsReq);
    if (ntraps % sizeof(xTrapezoid))
        return BadLength;
    ntraps /= sizeof(xTrapezoid);
    if (ntraps)
        CompositeTrapezoids(stuff->op, pSrc, pDst, pFormat,
                            stuff->xSrc, stuff->ySrc,
                            ntraps, (xTrapezoid *) &stuff[1]);
    return Success;
}

static int
ProcRenderTriangles(ClientPtr client)
{
    int rc, ntris;
    PicturePtr pSrc, pDst;
    PictFormatPtr pFormat;

    REQUEST(xRenderTrianglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrianglesReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
        return BadMatch;
    if (stuff->maskFormat) {
        rc = dixLookupResourceByType((void **) &pFormat, stuff->maskFormat,
                                     PictFormatType, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    else
        pFormat = 0;
    ntris = (client->req_len << 2) - sizeof(xRenderTrianglesReq);
    if (ntris % sizeof(xTriangle))
        return BadLength;
    ntris /= sizeof(xTriangle);
    if (ntris)
        CompositeTriangles(stuff->op, pSrc, pDst, pFormat,
                           stuff->xSrc, stuff->ySrc,
                           ntris, (xTriangle *) &stuff[1]);
    return Success;
}

static int
ProcRenderTriStrip(ClientPtr client)
{
    int rc, npoints;
    PicturePtr pSrc, pDst;
    PictFormatPtr pFormat;

    REQUEST(xRenderTrianglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrianglesReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
        return BadMatch;
    if (stuff->maskFormat) {
        rc = dixLookupResourceByType((void **) &pFormat, stuff->maskFormat,
                                     PictFormatType, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    else
        pFormat = 0;
    npoints = ((client->req_len << 2) - sizeof(xRenderTriStripReq));
    if (npoints & 4)
        return BadLength;
    npoints >>= 3;
    if (npoints >= 3)
        CompositeTriStrip(stuff->op, pSrc, pDst, pFormat,
                          stuff->xSrc, stuff->ySrc,
                          npoints, (xPointFixed *) &stuff[1]);
    return Success;
}

static int
ProcRenderTriFan(ClientPtr client)
{
    int rc, npoints;
    PicturePtr pSrc, pDst;
    PictFormatPtr pFormat;

    REQUEST(xRenderTrianglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrianglesReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
        return BadMatch;
    if (stuff->maskFormat) {
        rc = dixLookupResourceByType((void **) &pFormat, stuff->maskFormat,
                                     PictFormatType, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    else
        pFormat = 0;
    npoints = ((client->req_len << 2) - sizeof(xRenderTriStripReq));
    if (npoints & 4)
        return BadLength;
    npoints >>= 3;
    if (npoints >= 3)
        CompositeTriFan(stuff->op, pSrc, pDst, pFormat,
                        stuff->xSrc, stuff->ySrc,
                        npoints, (xPointFixed *) &stuff[1]);
    return Success;
}

static int
ProcRenderColorTrapezoids(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderColorTriangles(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTransform(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderCreateGlyphSet(ClientPtr client)
{
    GlyphSetPtr glyphSet;
    PictFormatPtr format;
    int rc, f;

    REQUEST(xRenderCreateGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderCreateGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);
    rc = dixLookupResourceByType((void **) &format, stuff->format,
                                 PictFormatType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    switch (format->depth) {
    case 1:
        f = GlyphFormat1;
        break;
    case 4:
        f = GlyphFormat4;
        break;
    case 8:
        f = GlyphFormat8;
        break;
    case 16:
        f = GlyphFormat16;
        break;
    case 32:
        f = GlyphFormat32;
        break;
    default:
        return BadMatch;
    }
    if (format->type != PictTypeDirect)
        return BadMatch;
    glyphSet = AllocateGlyphSet(f, format);
    if (!glyphSet)
        return BadAlloc;
    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->gsid, GlyphSetType,
                  glyphSet, RT_NONE, NULL, DixCreateAccess);
    if (rc != Success)
        return rc;
    if (!AddResource(stuff->gsid, GlyphSetType, (void *) glyphSet))
        return BadAlloc;
    return Success;
}

static int
ProcRenderReferenceGlyphSet(ClientPtr client)
{
    GlyphSetPtr glyphSet;
    int rc;

    REQUEST(xRenderReferenceGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderReferenceGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);

    rc = dixLookupResourceByType((void **) &glyphSet, stuff->existing,
                                 GlyphSetType, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->existing;
        return rc;
    }
    glyphSet->refcnt++;
    if (!AddResource(stuff->gsid, GlyphSetType, (void *) glyphSet))
        return BadAlloc;
    return Success;
}

#define NLOCALDELTA	64
#define NLOCALGLYPH	256

static int
ProcRenderFreeGlyphSet(ClientPtr client)
{
    GlyphSetPtr glyphSet;
    int rc;

    REQUEST(xRenderFreeGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderFreeGlyphSetReq);
    rc = dixLookupResourceByType((void **) &glyphSet, stuff->glyphset,
                                 GlyphSetType, client, DixDestroyAccess);
    if (rc != Success) {
        client->errorValue = stuff->glyphset;
        return rc;
    }
    FreeResource(stuff->glyphset, RT_NONE);
    return Success;
}

typedef struct _GlyphNew {
    Glyph id;
    GlyphPtr glyph;
    Bool found;
    unsigned char sha1[20];
} GlyphNewRec, *GlyphNewPtr;

#define NeedsComponent(f) (PICT_FORMAT_A(f) != 0 && PICT_FORMAT_RGB(f) != 0)

static int
ProcRenderAddGlyphs(ClientPtr client)
{
    GlyphSetPtr glyphSet;

    REQUEST(xRenderAddGlyphsReq);
    GlyphNewRec glyphsLocal[NLOCALGLYPH];
    GlyphNewPtr glyphsBase, glyphs, glyph_new;
    int remain, nglyphs;
    CARD32 *gids;
    xGlyphInfo *gi;
    CARD8 *bits;
    unsigned int size;
    int err;
    int i, screen;
    PicturePtr pSrc = NULL, pDst = NULL;
    PixmapPtr pSrcPix = NULL, pDstPix = NULL;
    CARD32 component_alpha;

    REQUEST_AT_LEAST_SIZE(xRenderAddGlyphsReq);
    err =
        dixLookupResourceByType((void **) &glyphSet, stuff->glyphset,
                                GlyphSetType, client, DixAddAccess);
    if (err != Success) {
        client->errorValue = stuff->glyphset;
        return err;
    }

    err = BadAlloc;
    nglyphs = stuff->nglyphs;
    if (nglyphs > UINT32_MAX / sizeof(GlyphNewRec))
        return BadAlloc;

    component_alpha = NeedsComponent(glyphSet->format->format);

    if (nglyphs <= NLOCALGLYPH) {
        memset(glyphsLocal, 0, sizeof(glyphsLocal));
        glyphsBase = glyphsLocal;
    }
    else {
        glyphsBase = (GlyphNewPtr) calloc(nglyphs, sizeof(GlyphNewRec));
        if (!glyphsBase)
            return BadAlloc;
    }

    remain = (client->req_len << 2) - sizeof(xRenderAddGlyphsReq);

    glyphs = glyphsBase;

    gids = (CARD32 *) (stuff + 1);
    gi = (xGlyphInfo *) (gids + nglyphs);
    bits = (CARD8 *) (gi + nglyphs);
    remain -= (sizeof(CARD32) + sizeof(xGlyphInfo)) * nglyphs;

    /* protect against bad nglyphs */
    if (gi < ((xGlyphInfo *) stuff) ||
        gi > ((xGlyphInfo *) ((CARD32 *) stuff + client->req_len)) ||
        bits < ((CARD8 *) stuff) ||
        bits > ((CARD8 *) ((CARD32 *) stuff + client->req_len))) {
        err = BadLength;
        goto bail;
    }

    for (i = 0; i < nglyphs; i++) {
        size_t padded_width;

        glyph_new = &glyphs[i];

        padded_width = PixmapBytePad(gi[i].width, glyphSet->format->depth);

        if (gi[i].height &&
            padded_width > (UINT32_MAX - sizeof(GlyphRec)) / gi[i].height)
            break;

        size = gi[i].height * padded_width;
        if (remain < size)
            break;

        err = HashGlyph(&gi[i], bits, size, glyph_new->sha1);
        if (err)
            goto bail;

        glyph_new->glyph = FindGlyphByHash(glyph_new->sha1, glyphSet->fdepth);

        if (glyph_new->glyph && glyph_new->glyph != DeletedGlyph) {
            glyph_new->found = TRUE;
        }
        else {
            GlyphPtr glyph;

            glyph_new->found = FALSE;
            glyph_new->glyph = glyph = AllocateGlyph(&gi[i], glyphSet->fdepth);
            if (!glyph) {
                err = BadAlloc;
                goto bail;
            }

            for (screen = 0; screen < screenInfo.numScreens; screen++) {
                int width = gi[i].width;
                int height = gi[i].height;
                int depth = glyphSet->format->depth;
                ScreenPtr pScreen;
                int error;

                /* Skip work if it's invisibly small anyway */
                if (!width || !height)
                    break;

                pScreen = screenInfo.screens[screen];
                pSrcPix = GetScratchPixmapHeader(pScreen,
                                                 width, height,
                                                 depth, depth, -1, bits);
                if (!pSrcPix) {
                    err = BadAlloc;
                    goto bail;
                }

                pSrc = CreatePicture(0, &pSrcPix->drawable,
                                     glyphSet->format, 0, NULL,
                                     serverClient, &error);
                if (!pSrc) {
                    err = BadAlloc;
                    goto bail;
                }

                pDstPix = (pScreen->CreatePixmap) (pScreen,
                                                   width, height, depth,
                                                   CREATE_PIXMAP_USAGE_GLYPH_PICTURE);

                if (!pDstPix) {
                    err = BadAlloc;
                    goto bail;
                }

                pDst = CreatePicture(0, &pDstPix->drawable,
                                  glyphSet->format,
                                  CPComponentAlpha, &component_alpha,
                                  serverClient, &error);
                SetGlyphPicture(glyph, pScreen, pDst);

                /* The picture takes a reference to the pixmap, so we
                   drop ours. */
                (pScreen->DestroyPixmap) (pDstPix);
                pDstPix = NULL;

                if (!pDst) {
                    err = BadAlloc;
                    goto bail;
                }

                CompositePicture(PictOpSrc,
                                 pSrc,
                                 None, pDst, 0, 0, 0, 0, 0, 0, width, height);

                FreePicture((void *) pSrc, 0);
                pSrc = NULL;
                FreeScratchPixmapHeader(pSrcPix);
                pSrcPix = NULL;
            }

            memcpy(glyph_new->glyph->sha1, glyph_new->sha1, 20);
        }

        glyph_new->id = gids[i];

        if (size & 3)
            size += 4 - (size & 3);
        bits += size;
        remain -= size;
    }
    if (remain || i < nglyphs) {
        err = BadLength;
        goto bail;
    }
    if (!ResizeGlyphSet(glyphSet, nglyphs)) {
        err = BadAlloc;
        goto bail;
    }
    for (i = 0; i < nglyphs; i++)
        AddGlyph(glyphSet, glyphs[i].glyph, glyphs[i].id);

    if (glyphsBase != glyphsLocal)
        free(glyphsBase);
    return Success;
 bail:
    if (pSrc)
        FreePicture((void *) pSrc, 0);
    if (pSrcPix)
        FreeScratchPixmapHeader(pSrcPix);
    for (i = 0; i < nglyphs; i++)
        if (glyphs[i].glyph && !glyphs[i].found)
            free(glyphs[i].glyph);
    if (glyphsBase != glyphsLocal)
        free(glyphsBase);
    return err;
}

static int
ProcRenderAddGlyphsFromPicture(ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderFreeGlyphs(ClientPtr client)
{
    REQUEST(xRenderFreeGlyphsReq);
    GlyphSetPtr glyphSet;
    int rc, nglyph;
    CARD32 *gids;
    CARD32 glyph;

    REQUEST_AT_LEAST_SIZE(xRenderFreeGlyphsReq);
    rc = dixLookupResourceByType((void **) &glyphSet, stuff->glyphset,
                                 GlyphSetType, client, DixRemoveAccess);
    if (rc != Success) {
        client->errorValue = stuff->glyphset;
        return rc;
    }
    nglyph =
        bytes_to_int32((client->req_len << 2) - sizeof(xRenderFreeGlyphsReq));
    gids = (CARD32 *) (stuff + 1);
    while (nglyph-- > 0) {
        glyph = *gids++;
        if (!DeleteGlyph(glyphSet, glyph)) {
            client->errorValue = glyph;
            return RenderErrBase + BadGlyph;
        }
    }
    return Success;
}

static int
ProcRenderCompositeGlyphs(ClientPtr client)
{
    GlyphSetPtr glyphSet;
    GlyphSet gs;
    PicturePtr pSrc, pDst;
    PictFormatPtr pFormat;
    GlyphListRec listsLocal[NLOCALDELTA];
    GlyphListPtr lists, listsBase;
    GlyphPtr glyphsLocal[NLOCALGLYPH];
    Glyph glyph;
    GlyphPtr *glyphs, *glyphsBase;
    xGlyphElt *elt;
    CARD8 *buffer, *end;
    int nglyph;
    int nlist;
    int space;
    int size;
    int rc, n;

    REQUEST(xRenderCompositeGlyphsReq);

    REQUEST_AT_LEAST_SIZE(xRenderCompositeGlyphsReq);

    switch (stuff->renderReqType) {
    default:
        size = 1;
        break;
    case X_RenderCompositeGlyphs16:
        size = 2;
        break;
    case X_RenderCompositeGlyphs32:
        size = 4;
        break;
    }

    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
        return BadMatch;
    if (stuff->maskFormat) {
        rc = dixLookupResourceByType((void **) &pFormat, stuff->maskFormat,
                                     PictFormatType, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    else
        pFormat = 0;

    rc = dixLookupResourceByType((void **) &glyphSet, stuff->glyphset,
                                 GlyphSetType, client, DixUseAccess);
    if (rc != Success)
        return rc;

    buffer = (CARD8 *) (stuff + 1);
    end = (CARD8 *) stuff + (client->req_len << 2);
    nglyph = 0;
    nlist = 0;
    while (buffer + sizeof(xGlyphElt) < end) {
        elt = (xGlyphElt *) buffer;
        buffer += sizeof(xGlyphElt);

        if (elt->len == 0xff) {
            buffer += 4;
        }
        else {
            nlist++;
            nglyph += elt->len;
            space = size * elt->len;
            if (space & 3)
                space += 4 - (space & 3);
            buffer += space;
        }
    }
    if (nglyph <= NLOCALGLYPH)
        glyphsBase = glyphsLocal;
    else {
        glyphsBase = xallocarray(nglyph, sizeof(GlyphPtr));
        if (!glyphsBase)
            return BadAlloc;
    }
    if (nlist <= NLOCALDELTA)
        listsBase = listsLocal;
    else {
        listsBase = xallocarray(nlist, sizeof(GlyphListRec));
        if (!listsBase) {
            rc = BadAlloc;
            goto bail;
        }
    }
    buffer = (CARD8 *) (stuff + 1);
    glyphs = glyphsBase;
    lists = listsBase;
    while (buffer + sizeof(xGlyphElt) < end) {
        elt = (xGlyphElt *) buffer;
        buffer += sizeof(xGlyphElt);

        if (elt->len == 0xff) {
            if (buffer + sizeof(GlyphSet) < end) {
                memcpy(&gs, buffer, sizeof(GlyphSet));
                rc = dixLookupResourceByType((void **) &glyphSet, gs,
                                             GlyphSetType, client,
                                             DixUseAccess);
                if (rc != Success)
                    goto bail;
            }
            buffer += 4;
        }
        else {
            lists->xOff = elt->deltax;
            lists->yOff = elt->deltay;
            lists->format = glyphSet->format;
            lists->len = 0;
            n = elt->len;
            while (n--) {
                if (buffer + size <= end) {
                    switch (size) {
                    case 1:
                        glyph = *((CARD8 *) buffer);
                        break;
                    case 2:
                        glyph = *((CARD16 *) buffer);
                        break;
                    case 4:
                    default:
                        glyph = *((CARD32 *) buffer);
                        break;
                    }
                    if ((*glyphs = FindGlyph(glyphSet, glyph))) {
                        lists->len++;
                        glyphs++;
                    }
                }
                buffer += size;
            }
            space = size * elt->len;
            if (space & 3)
                buffer += 4 - (space & 3);
            lists++;
        }
    }
    if (buffer > end) {
        rc = BadLength;
        goto bail;
    }

    CompositeGlyphs(stuff->op,
                    pSrc,
                    pDst,
                    pFormat,
                    stuff->xSrc, stuff->ySrc, nlist, listsBase, glyphsBase);
    rc = Success;

 bail:
    if (glyphsBase != glyphsLocal)
        free(glyphsBase);
    if (listsBase != listsLocal)
        free(listsBase);
    return rc;
}

static int
ProcRenderFillRectangles(ClientPtr client)
{
    PicturePtr pDst;
    int things;

    REQUEST(xRenderFillRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderFillRectanglesReq);
    if (!PictOpValid(stuff->op)) {
        client->errorValue = stuff->op;
        return BadValue;
    }
    VERIFY_PICTURE(pDst, stuff->dst, client, DixWriteAccess);
    if (!pDst->pDrawable)
        return BadDrawable;

    things = (client->req_len << 2) - sizeof(xRenderFillRectanglesReq);
    if (things & 4)
        return BadLength;
    things >>= 3;

    CompositeRects(stuff->op,
                   pDst, &stuff->color, things, (xRectangle *) &stuff[1]);

    return Success;
}

static void
RenderSetBit(unsigned char *line, int x, int bit)
{
    unsigned char mask;

    if (screenInfo.bitmapBitOrder == LSBFirst)
        mask = (1 << (x & 7));
    else
        mask = (0x80 >> (x & 7));
    /* XXX assumes byte order is host byte order */
    line += (x >> 3);
    if (bit)
        *line |= mask;
    else
        *line &= ~mask;
}

#define DITHER_DIM 2

static CARD32 orderedDither[DITHER_DIM][DITHER_DIM] = {
    {1, 3,},
    {4, 2,},
};

#define DITHER_SIZE  ((sizeof orderedDither / sizeof orderedDither[0][0]) + 1)

static int
ProcRenderCreateCursor(ClientPtr client)
{
    REQUEST(xRenderCreateCursorReq);
    PicturePtr pSrc;
    ScreenPtr pScreen;
    unsigned short width, height;
    CARD32 *argbbits, *argb;
    unsigned char *srcbits, *srcline;
    unsigned char *mskbits, *mskline;
    int stride;
    int x, y;
    int nbytes_mono;
    CursorMetricRec cm;
    CursorPtr pCursor;
    CARD32 twocolor[3];
    int rc, ncolor;

    REQUEST_SIZE_MATCH(xRenderCreateCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);

    VERIFY_PICTURE(pSrc, stuff->src, client, DixReadAccess);
    if (!pSrc->pDrawable)
        return BadDrawable;
    pScreen = pSrc->pDrawable->pScreen;
    width = pSrc->pDrawable->width;
    height = pSrc->pDrawable->height;
    if (height && width > UINT32_MAX / (height * sizeof(CARD32)))
        return BadAlloc;
    if (stuff->x > width || stuff->y > height)
        return BadMatch;
    argbbits = malloc(width * height * sizeof(CARD32));
    if (!argbbits)
        return BadAlloc;

    stride = BitmapBytePad(width);
    nbytes_mono = stride * height;
    srcbits = calloc(1, nbytes_mono);
    if (!srcbits) {
        free(argbbits);
        return BadAlloc;
    }
    mskbits = calloc(1, nbytes_mono);
    if (!mskbits) {
        free(argbbits);
        free(srcbits);
        return BadAlloc;
    }

    /* what kind of maniac creates a cursor from a window picture though */
    if (pSrc->pDrawable->type == DRAWABLE_WINDOW)
        pScreen->SourceValidate(pSrc->pDrawable, 0, 0, width, height,
                                IncludeInferiors);

    if (pSrc->format == PICT_a8r8g8b8) {
        (*pScreen->GetImage) (pSrc->pDrawable,
                              0, 0, width, height, ZPixmap,
                              0xffffffff, (void *) argbbits);
    }
    else {
        PixmapPtr pPixmap;
        PicturePtr pPicture;
        PictFormatPtr pFormat;
        int error;

        pFormat = PictureMatchFormat(pScreen, 32, PICT_a8r8g8b8);
        if (!pFormat) {
            free(argbbits);
            free(srcbits);
            free(mskbits);
            return BadImplementation;
        }
        pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, 32,
                                            CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pPixmap) {
            free(argbbits);
            free(srcbits);
            free(mskbits);
            return BadAlloc;
        }
        pPicture = CreatePicture(0, &pPixmap->drawable, pFormat, 0, 0,
                                 client, &error);
        if (!pPicture) {
            free(argbbits);
            free(srcbits);
            free(mskbits);
            return error;
        }
        (*pScreen->DestroyPixmap) (pPixmap);
        CompositePicture(PictOpSrc,
                         pSrc, 0, pPicture, 0, 0, 0, 0, 0, 0, width, height);
        (*pScreen->GetImage) (pPicture->pDrawable,
                              0, 0, width, height, ZPixmap,
                              0xffffffff, (void *) argbbits);
        FreePicture(pPicture, 0);
    }
    /*
     * Check whether the cursor can be directly supported by
     * the core cursor code
     */
    ncolor = 0;
    argb = argbbits;
    for (y = 0; ncolor <= 2 && y < height; y++) {
        for (x = 0; ncolor <= 2 && x < width; x++) {
            CARD32 p = *argb++;
            CARD32 a = (p >> 24);

            if (a == 0)         /* transparent */
                continue;
            if (a == 0xff) {    /* opaque */
                int n;

                for (n = 0; n < ncolor; n++)
                    if (p == twocolor[n])
                        break;
                if (n == ncolor)
                    twocolor[ncolor++] = p;
            }
            else
                ncolor = 3;
        }
    }

    /*
     * Convert argb image to two plane cursor
     */
    srcline = srcbits;
    mskline = mskbits;
    argb = argbbits;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            CARD32 p = *argb++;

            if (ncolor <= 2) {
                CARD32 a = ((p >> 24));

                RenderSetBit(mskline, x, a != 0);
                RenderSetBit(srcline, x, a != 0 && p == twocolor[0]);
            }
            else {
                CARD32 a = ((p >> 24) * DITHER_SIZE + 127) / 255;
                CARD32 i = ((CvtR8G8B8toY15(p) >> 7) * DITHER_SIZE + 127) / 255;
                CARD32 d =
                    orderedDither[y & (DITHER_DIM - 1)][x & (DITHER_DIM - 1)];
                /* Set mask from dithered alpha value */
                RenderSetBit(mskline, x, a > d);
                /* Set src from dithered intensity value */
                RenderSetBit(srcline, x, a > d && i <= d);
            }
        }
        srcline += stride;
        mskline += stride;
    }
    /*
     * Dither to white and black if the cursor has more than two colors
     */
    if (ncolor > 2) {
        twocolor[0] = 0xff000000;
        twocolor[1] = 0xffffffff;
    }
    else {
        free(argbbits);
        argbbits = 0;
    }

#define GetByte(p,s)	(((p) >> (s)) & 0xff)
#define GetColor(p,s)	(GetByte(p,s) | (GetByte(p,s) << 8))

    cm.width = width;
    cm.height = height;
    cm.xhot = stuff->x;
    cm.yhot = stuff->y;
    rc = AllocARGBCursor(srcbits, mskbits, argbbits, &cm,
                         GetColor(twocolor[0], 16),
                         GetColor(twocolor[0], 8),
                         GetColor(twocolor[0], 0),
                         GetColor(twocolor[1], 16),
                         GetColor(twocolor[1], 8),
                         GetColor(twocolor[1], 0),
                         &pCursor, client, stuff->cid);
    if (rc != Success)
        goto bail;
    if (!AddResource(stuff->cid, RT_CURSOR, (void *) pCursor)) {
        rc = BadAlloc;
        goto bail;
    }

    return Success;
 bail:
    free(srcbits);
    free(mskbits);
    return rc;
}

static int
ProcRenderSetPictureTransform(ClientPtr client)
{
    REQUEST(xRenderSetPictureTransformReq);
    PicturePtr pPicture;

    REQUEST_SIZE_MATCH(xRenderSetPictureTransformReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixSetAttrAccess);
    return SetPictureTransform(pPicture, (PictTransform *) &stuff->transform);
}

static int
ProcRenderQueryFilters(ClientPtr client)
{
    REQUEST(xRenderQueryFiltersReq);
    DrawablePtr pDrawable;
    xRenderQueryFiltersReply *reply;
    int nbytesName;
    int nnames;
    ScreenPtr pScreen;
    PictureScreenPtr ps;
    int i, j, len, total_bytes, rc;
    INT16 *aliases;
    char *names;

    REQUEST_SIZE_MATCH(xRenderQueryFiltersReq);
    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pDrawable->pScreen;
    nbytesName = 0;
    nnames = 0;
    ps = GetPictureScreenIfSet(pScreen);
    if (ps) {
        for (i = 0; i < ps->nfilters; i++)
            nbytesName += 1 + strlen(ps->filters[i].name);
        for (i = 0; i < ps->nfilterAliases; i++)
            nbytesName += 1 + strlen(ps->filterAliases[i].alias);
        nnames = ps->nfilters + ps->nfilterAliases;
    }
    len = ((nnames + 1) >> 1) + bytes_to_int32(nbytesName);
    total_bytes = sizeof(xRenderQueryFiltersReply) + (len << 2);
    reply = (xRenderQueryFiltersReply *) calloc(1, total_bytes);
    if (!reply)
        return BadAlloc;
    aliases = (INT16 *) (reply + 1);
    names = (char *) (aliases + ((nnames + 1) & ~1));

    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = len;
    reply->numAliases = nnames;
    reply->numFilters = nnames;
    if (ps) {

        /* fill in alias values */
        for (i = 0; i < ps->nfilters; i++)
            aliases[i] = FilterAliasNone;
        for (i = 0; i < ps->nfilterAliases; i++) {
            for (j = 0; j < ps->nfilters; j++)
                if (ps->filterAliases[i].filter_id == ps->filters[j].id)
                    break;
            if (j == ps->nfilters) {
                for (j = 0; j < ps->nfilterAliases; j++)
                    if (ps->filterAliases[i].filter_id ==
                        ps->filterAliases[j].alias_id) {
                        break;
                    }
                if (j == ps->nfilterAliases)
                    j = FilterAliasNone;
                else
                    j = j + ps->nfilters;
            }
            aliases[i + ps->nfilters] = j;
        }

        /* fill in filter names */
        for (i = 0; i < ps->nfilters; i++) {
            j = strlen(ps->filters[i].name);
            *names++ = j;
            memcpy(names, ps->filters[i].name, j);
            names += j;
        }

        /* fill in filter alias names */
        for (i = 0; i < ps->nfilterAliases; i++) {
            j = strlen(ps->filterAliases[i].alias);
            *names++ = j;
            memcpy(names, ps->filterAliases[i].alias, j);
            names += j;
        }
    }

    if (client->swapped) {
        for (i = 0; i < reply->numAliases; i++) {
            swaps(&aliases[i]);
        }
        swaps(&reply->sequenceNumber);
        swapl(&reply->length);
        swapl(&reply->numAliases);
        swapl(&reply->numFilters);
    }
    WriteToClient(client, total_bytes, reply);
    free(reply);

    return Success;
}

static int
ProcRenderSetPictureFilter(ClientPtr client)
{
    REQUEST(xRenderSetPictureFilterReq);
    PicturePtr pPicture;
    int result;
    xFixed *params;
    int nparams;
    char *name;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureFilterReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixSetAttrAccess);
    name = (char *) (stuff + 1);
    params = (xFixed *) (name + pad_to_int32(stuff->nbytes));
    nparams = ((xFixed *) stuff + client->req_len) - params;
    if (nparams < 0)
	return BadLength;

    result = SetPictureFilter(pPicture, name, stuff->nbytes, params, nparams);
    return result;
}

static int
ProcRenderCreateAnimCursor(ClientPtr client)
{
    REQUEST(xRenderCreateAnimCursorReq);
    CursorPtr *cursors;
    CARD32 *deltas;
    CursorPtr pCursor;
    int ncursor;
    xAnimCursorElt *elt;
    int i;
    int ret;

    REQUEST_AT_LEAST_SIZE(xRenderCreateAnimCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);
    if (client->req_len & 1)
        return BadLength;
    ncursor =
        (client->req_len -
         (bytes_to_int32(sizeof(xRenderCreateAnimCursorReq)))) >> 1;
    cursors = xallocarray(ncursor, sizeof(CursorPtr) + sizeof(CARD32));
    if (!cursors)
        return BadAlloc;
    deltas = (CARD32 *) (cursors + ncursor);
    elt = (xAnimCursorElt *) (stuff + 1);
    for (i = 0; i < ncursor; i++) {
        ret = dixLookupResourceByType((void **) (cursors + i), elt->cursor,
                                      RT_CURSOR, client, DixReadAccess);
        if (ret != Success) {
            free(cursors);
            return ret;
        }
        deltas[i] = elt->delay;
        elt++;
    }
    ret = AnimCursorCreate(cursors, deltas, ncursor, &pCursor, client,
                           stuff->cid);
    free(cursors);
    if (ret != Success)
        return ret;

    if (AddResource(stuff->cid, RT_CURSOR, (void *) pCursor))
        return Success;
    return BadAlloc;
}

static int
ProcRenderAddTraps(ClientPtr client)
{
    int ntraps;
    PicturePtr pPicture;

    REQUEST(xRenderAddTrapsReq);

    REQUEST_AT_LEAST_SIZE(xRenderAddTrapsReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixWriteAccess);
    if (!pPicture->pDrawable)
        return BadDrawable;
    ntraps = (client->req_len << 2) - sizeof(xRenderAddTrapsReq);
    if (ntraps % sizeof(xTrap))
        return BadLength;
    ntraps /= sizeof(xTrap);
    if (ntraps)
        AddTraps(pPicture,
                 stuff->xOff, stuff->yOff, ntraps, (xTrap *) &stuff[1]);
    return Success;
}

static int
ProcRenderCreateSolidFill(ClientPtr client)
{
    PicturePtr pPicture;
    int error = 0;

    REQUEST(xRenderCreateSolidFillReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateSolidFillReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    pPicture = CreateSolidPicture(stuff->pid, &stuff->color, &error);
    if (!pPicture)
        return error;
    /* security creation/labeling check */
    error = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, PictureType,
                     pPicture, RT_NONE, NULL, DixCreateAccess);
    if (error != Success)
        return error;
    if (!AddResource(stuff->pid, PictureType, (void *) pPicture))
        return BadAlloc;
    return Success;
}

static int
ProcRenderCreateLinearGradient(ClientPtr client)
{
    PicturePtr pPicture;
    int len;
    int error = 0;
    xFixed *stops;
    xRenderColor *colors;

    REQUEST(xRenderCreateLinearGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateLinearGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateLinearGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *) (stuff + 1);
    colors = (xRenderColor *) (stops + stuff->nStops);

    pPicture = CreateLinearGradientPicture(stuff->pid, &stuff->p1, &stuff->p2,
                                           stuff->nStops, stops, colors,
                                           &error);
    if (!pPicture)
        return error;
    /* security creation/labeling check */
    error = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, PictureType,
                     pPicture, RT_NONE, NULL, DixCreateAccess);
    if (error != Success)
        return error;
    if (!AddResource(stuff->pid, PictureType, (void *) pPicture))
        return BadAlloc;
    return Success;
}

static int
ProcRenderCreateRadialGradient(ClientPtr client)
{
    PicturePtr pPicture;
    int len;
    int error = 0;
    xFixed *stops;
    xRenderColor *colors;

    REQUEST(xRenderCreateRadialGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateRadialGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateRadialGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *) (stuff + 1);
    colors = (xRenderColor *) (stops + stuff->nStops);

    pPicture =
        CreateRadialGradientPicture(stuff->pid, &stuff->inner, &stuff->outer,
                                    stuff->inner_radius, stuff->outer_radius,
                                    stuff->nStops, stops, colors, &error);
    if (!pPicture)
        return error;
    /* security creation/labeling check */
    error = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, PictureType,
                     pPicture, RT_NONE, NULL, DixCreateAccess);
    if (error != Success)
        return error;
    if (!AddResource(stuff->pid, PictureType, (void *) pPicture))
        return BadAlloc;
    return Success;
}

static int
ProcRenderCreateConicalGradient(ClientPtr client)
{
    PicturePtr pPicture;
    int len;
    int error = 0;
    xFixed *stops;
    xRenderColor *colors;

    REQUEST(xRenderCreateConicalGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateConicalGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateConicalGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *) (stuff + 1);
    colors = (xRenderColor *) (stops + stuff->nStops);

    pPicture =
        CreateConicalGradientPicture(stuff->pid, &stuff->center, stuff->angle,
                                     stuff->nStops, stops, colors, &error);
    if (!pPicture)
        return error;
    /* security creation/labeling check */
    error = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, PictureType,
                     pPicture, RT_NONE, NULL, DixCreateAccess);
    if (error != Success)
        return error;
    if (!AddResource(stuff->pid, PictureType, (void *) pPicture))
        return BadAlloc;
    return Success;
}

static int
ProcRenderDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < RenderNumberRequests)
        return (*ProcRenderVector[stuff->data]) (client);
    else
        return BadRequest;
}

static int _X_COLD
SProcRenderQueryVersion(ClientPtr client)
{
    REQUEST(xRenderQueryVersionReq);
    REQUEST_SIZE_MATCH(xRenderQueryVersionReq);
    swaps(&stuff->length);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderQueryPictFormats(ClientPtr client)
{
    REQUEST(xRenderQueryPictFormatsReq);
    REQUEST_SIZE_MATCH(xRenderQueryPictFormatsReq);
    swaps(&stuff->length);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderQueryPictIndexValues(ClientPtr client)
{
    REQUEST(xRenderQueryPictIndexValuesReq);
    REQUEST_AT_LEAST_SIZE(xRenderQueryPictIndexValuesReq);
    swaps(&stuff->length);
    swapl(&stuff->format);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderQueryDithers(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderCreatePicture(ClientPtr client)
{
    REQUEST(xRenderCreatePictureReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreatePictureReq);
    swaps(&stuff->length);
    swapl(&stuff->pid);
    swapl(&stuff->drawable);
    swapl(&stuff->format);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderChangePicture(ClientPtr client)
{
    REQUEST(xRenderChangePictureReq);
    REQUEST_AT_LEAST_SIZE(xRenderChangePictureReq);
    swaps(&stuff->length);
    swapl(&stuff->picture);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderSetPictureClipRectangles(ClientPtr client)
{
    REQUEST(xRenderSetPictureClipRectanglesReq);
    REQUEST_AT_LEAST_SIZE(xRenderSetPictureClipRectanglesReq);
    swaps(&stuff->length);
    swapl(&stuff->picture);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    SwapRestS(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderFreePicture(ClientPtr client)
{
    REQUEST(xRenderFreePictureReq);
    REQUEST_SIZE_MATCH(xRenderFreePictureReq);
    swaps(&stuff->length);
    swapl(&stuff->picture);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderComposite(ClientPtr client)
{
    REQUEST(xRenderCompositeReq);
    REQUEST_SIZE_MATCH(xRenderCompositeReq);
    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->mask);
    swapl(&stuff->dst);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    swaps(&stuff->xMask);
    swaps(&stuff->yMask);
    swaps(&stuff->xDst);
    swaps(&stuff->yDst);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderScale(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderTrapezoids(ClientPtr client)
{
    REQUEST(xRenderTrapezoidsReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrapezoidsReq);
    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->dst);
    swapl(&stuff->maskFormat);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderTriangles(ClientPtr client)
{
    REQUEST(xRenderTrianglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrianglesReq);
    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->dst);
    swapl(&stuff->maskFormat);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderTriStrip(ClientPtr client)
{
    REQUEST(xRenderTriStripReq);

    REQUEST_AT_LEAST_SIZE(xRenderTriStripReq);
    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->dst);
    swapl(&stuff->maskFormat);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderTriFan(ClientPtr client)
{
    REQUEST(xRenderTriFanReq);

    REQUEST_AT_LEAST_SIZE(xRenderTriFanReq);
    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->dst);
    swapl(&stuff->maskFormat);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderColorTrapezoids(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderColorTriangles(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderTransform(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderCreateGlyphSet(ClientPtr client)
{
    REQUEST(xRenderCreateGlyphSetReq);
    REQUEST_SIZE_MATCH(xRenderCreateGlyphSetReq);
    swaps(&stuff->length);
    swapl(&stuff->gsid);
    swapl(&stuff->format);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderReferenceGlyphSet(ClientPtr client)
{
    REQUEST(xRenderReferenceGlyphSetReq);
    REQUEST_SIZE_MATCH(xRenderReferenceGlyphSetReq);
    swaps(&stuff->length);
    swapl(&stuff->gsid);
    swapl(&stuff->existing);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderFreeGlyphSet(ClientPtr client)
{
    REQUEST(xRenderFreeGlyphSetReq);
    REQUEST_SIZE_MATCH(xRenderFreeGlyphSetReq);
    swaps(&stuff->length);
    swapl(&stuff->glyphset);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderAddGlyphs(ClientPtr client)
{
    register int i;
    CARD32 *gids;
    void *end;
    xGlyphInfo *gi;

    REQUEST(xRenderAddGlyphsReq);
    REQUEST_AT_LEAST_SIZE(xRenderAddGlyphsReq);
    swaps(&stuff->length);
    swapl(&stuff->glyphset);
    swapl(&stuff->nglyphs);
    if (stuff->nglyphs & 0xe0000000)
        return BadLength;
    end = (CARD8 *) stuff + (client->req_len << 2);
    gids = (CARD32 *) (stuff + 1);
    gi = (xGlyphInfo *) (gids + stuff->nglyphs);
    if ((char *) end - (char *) (gids + stuff->nglyphs) < 0)
        return BadLength;
    if ((char *) end - (char *) (gi + stuff->nglyphs) < 0)
        return BadLength;
    for (i = 0; i < stuff->nglyphs; i++) {
        swapl(&gids[i]);
        swaps(&gi[i].width);
        swaps(&gi[i].height);
        swaps(&gi[i].x);
        swaps(&gi[i].y);
        swaps(&gi[i].xOff);
        swaps(&gi[i].yOff);
    }
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderAddGlyphsFromPicture(ClientPtr client)
{
    return BadImplementation;
}

static int _X_COLD
SProcRenderFreeGlyphs(ClientPtr client)
{
    REQUEST(xRenderFreeGlyphsReq);
    REQUEST_AT_LEAST_SIZE(xRenderFreeGlyphsReq);
    swaps(&stuff->length);
    swapl(&stuff->glyphset);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCompositeGlyphs(ClientPtr client)
{
    xGlyphElt *elt;
    CARD8 *buffer;
    CARD8 *end;
    int space;
    int i;
    int size;

    REQUEST(xRenderCompositeGlyphsReq);
    REQUEST_AT_LEAST_SIZE(xRenderCompositeGlyphsReq);

    switch (stuff->renderReqType) {
    default:
        size = 1;
        break;
    case X_RenderCompositeGlyphs16:
        size = 2;
        break;
    case X_RenderCompositeGlyphs32:
        size = 4;
        break;
    }

    swaps(&stuff->length);
    swapl(&stuff->src);
    swapl(&stuff->dst);
    swapl(&stuff->maskFormat);
    swapl(&stuff->glyphset);
    swaps(&stuff->xSrc);
    swaps(&stuff->ySrc);
    buffer = (CARD8 *) (stuff + 1);
    end = (CARD8 *) stuff + (client->req_len << 2);
    while (buffer + sizeof(xGlyphElt) < end) {
        elt = (xGlyphElt *) buffer;
        buffer += sizeof(xGlyphElt);

        swaps(&elt->deltax);
        swaps(&elt->deltay);

        i = elt->len;
        if (i == 0xff) {
            if (buffer + 4 > end) {
                return BadLength;
            }
            swapl((int *) buffer);
            buffer += 4;
        }
        else {
            space = size * i;
            switch (size) {
            case 1:
                buffer += i;
                break;
            case 2:
                if (buffer + i * 2 > end) {
                    return BadLength;
                }
                while (i--) {
                    swaps((short *) buffer);
                    buffer += 2;
                }
                break;
            case 4:
                if (buffer + i * 4 > end) {
                    return BadLength;
                }
                while (i--) {
                    swapl((int *) buffer);
                    buffer += 4;
                }
                break;
            }
            if (space & 3)
                buffer += 4 - (space & 3);
        }
    }
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderFillRectangles(ClientPtr client)
{
    REQUEST(xRenderFillRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xRenderFillRectanglesReq);
    swaps(&stuff->length);
    swapl(&stuff->dst);
    swaps(&stuff->color.red);
    swaps(&stuff->color.green);
    swaps(&stuff->color.blue);
    swaps(&stuff->color.alpha);
    SwapRestS(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCreateCursor(ClientPtr client)
{
    REQUEST(xRenderCreateCursorReq);
    REQUEST_SIZE_MATCH(xRenderCreateCursorReq);

    swaps(&stuff->length);
    swapl(&stuff->cid);
    swapl(&stuff->src);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderSetPictureTransform(ClientPtr client)
{
    REQUEST(xRenderSetPictureTransformReq);
    REQUEST_SIZE_MATCH(xRenderSetPictureTransformReq);

    swaps(&stuff->length);
    swapl(&stuff->picture);
    swapl(&stuff->transform.matrix11);
    swapl(&stuff->transform.matrix12);
    swapl(&stuff->transform.matrix13);
    swapl(&stuff->transform.matrix21);
    swapl(&stuff->transform.matrix22);
    swapl(&stuff->transform.matrix23);
    swapl(&stuff->transform.matrix31);
    swapl(&stuff->transform.matrix32);
    swapl(&stuff->transform.matrix33);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderQueryFilters(ClientPtr client)
{
    REQUEST(xRenderQueryFiltersReq);
    REQUEST_SIZE_MATCH(xRenderQueryFiltersReq);

    swaps(&stuff->length);
    swapl(&stuff->drawable);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderSetPictureFilter(ClientPtr client)
{
    REQUEST(xRenderSetPictureFilterReq);
    REQUEST_AT_LEAST_SIZE(xRenderSetPictureFilterReq);

    swaps(&stuff->length);
    swapl(&stuff->picture);
    swaps(&stuff->nbytes);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCreateAnimCursor(ClientPtr client)
{
    REQUEST(xRenderCreateAnimCursorReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreateAnimCursorReq);

    swaps(&stuff->length);
    swapl(&stuff->cid);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderAddTraps(ClientPtr client)
{
    REQUEST(xRenderAddTrapsReq);
    REQUEST_AT_LEAST_SIZE(xRenderAddTrapsReq);

    swaps(&stuff->length);
    swapl(&stuff->picture);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    SwapRestL(stuff);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCreateSolidFill(ClientPtr client)
{
    REQUEST(xRenderCreateSolidFillReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreateSolidFillReq);

    swaps(&stuff->length);
    swapl(&stuff->pid);
    swaps(&stuff->color.alpha);
    swaps(&stuff->color.red);
    swaps(&stuff->color.green);
    swaps(&stuff->color.blue);
    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static void _X_COLD
swapStops(void *stuff, int num)
{
    int i;
    CARD32 *stops;
    CARD16 *colors;

    stops = (CARD32 *) (stuff);
    for (i = 0; i < num; ++i) {
        swapl(stops);
        ++stops;
    }
    colors = (CARD16 *) (stops);
    for (i = 0; i < 4 * num; ++i) {
        swaps(colors);
        ++colors;
    }
}

static int _X_COLD
SProcRenderCreateLinearGradient(ClientPtr client)
{
    int len;

    REQUEST(xRenderCreateLinearGradientReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreateLinearGradientReq);

    swaps(&stuff->length);
    swapl(&stuff->pid);
    swapl(&stuff->p1.x);
    swapl(&stuff->p1.y);
    swapl(&stuff->p2.x);
    swapl(&stuff->p2.y);
    swapl(&stuff->nStops);

    len = (client->req_len << 2) - sizeof(xRenderCreateLinearGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    swapStops(stuff + 1, stuff->nStops);

    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCreateRadialGradient(ClientPtr client)
{
    int len;

    REQUEST(xRenderCreateRadialGradientReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreateRadialGradientReq);

    swaps(&stuff->length);
    swapl(&stuff->pid);
    swapl(&stuff->inner.x);
    swapl(&stuff->inner.y);
    swapl(&stuff->outer.x);
    swapl(&stuff->outer.y);
    swapl(&stuff->inner_radius);
    swapl(&stuff->outer_radius);
    swapl(&stuff->nStops);

    len = (client->req_len << 2) - sizeof(xRenderCreateRadialGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    swapStops(stuff + 1, stuff->nStops);

    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderCreateConicalGradient(ClientPtr client)
{
    int len;

    REQUEST(xRenderCreateConicalGradientReq);
    REQUEST_AT_LEAST_SIZE(xRenderCreateConicalGradientReq);

    swaps(&stuff->length);
    swapl(&stuff->pid);
    swapl(&stuff->center.x);
    swapl(&stuff->center.y);
    swapl(&stuff->angle);
    swapl(&stuff->nStops);

    len = (client->req_len << 2) - sizeof(xRenderCreateConicalGradientReq);
    if (stuff->nStops > UINT32_MAX / (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;
    if (len != stuff->nStops * (sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    swapStops(stuff + 1, stuff->nStops);

    return (*ProcRenderVector[stuff->renderReqType]) (client);
}

static int _X_COLD
SProcRenderDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < RenderNumberRequests)
        return (*SProcRenderVector[stuff->data]) (client);
    else
        return BadRequest;
}

#ifdef PANORAMIX
#define VERIFY_XIN_PICTURE(pPicture, pid, client, mode) {\
    int rc = dixLookupResourceByType((void **)&(pPicture), pid,\
                                     XRT_PICTURE, client, mode);\
    if (rc != Success)\
	return rc;\
}

#define VERIFY_XIN_ALPHA(pPicture, pid, client, mode) {\
    if (pid == None) \
	pPicture = 0; \
    else { \
	VERIFY_XIN_PICTURE(pPicture, pid, client, mode); \
    } \
} \

int (*PanoramiXSaveRenderVector[RenderNumberRequests]) (ClientPtr);

static int
PanoramiXRenderCreatePicture(ClientPtr client)
{
    REQUEST(xRenderCreatePictureReq);
    PanoramiXRes *refDraw, *newPict;
    int result, j;

    REQUEST_AT_LEAST_SIZE(xRenderCreatePictureReq);
    result = dixLookupResourceByClass((void **) &refDraw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;
    if (!(newPict = (PanoramiXRes *) malloc(sizeof(PanoramiXRes))))
        return BadAlloc;
    newPict->type = XRT_PICTURE;
    panoramix_setup_ids(newPict, client, stuff->pid);

    if (refDraw->type == XRT_WINDOW &&
        stuff->drawable == screenInfo.screens[0]->root->drawable.id) {
        newPict->u.pict.root = TRUE;
    }
    else
        newPict->u.pict.root = FALSE;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->pid = newPict->info[j].id;
        stuff->drawable = refDraw->info[j].id;
        result = (*PanoramiXSaveRenderVector[X_RenderCreatePicture]) (client);
        if (result != Success)
            break;
    }

    if (result == Success)
        AddResource(newPict->info[0].id, XRT_PICTURE, newPict);
    else
        free(newPict);

    return result;
}

static int
PanoramiXRenderChangePicture(ClientPtr client)
{
    PanoramiXRes *pict;
    int result = Success, j;

    REQUEST(xRenderChangePictureReq);

    REQUEST_AT_LEAST_SIZE(xRenderChangePictureReq);

    VERIFY_XIN_PICTURE(pict, stuff->picture, client, DixWriteAccess);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->picture = pict->info[j].id;
        result = (*PanoramiXSaveRenderVector[X_RenderChangePicture]) (client);
        if (result != Success)
            break;
    }

    return result;
}

static int
PanoramiXRenderSetPictureClipRectangles(ClientPtr client)
{
    REQUEST(xRenderSetPictureClipRectanglesReq);
    int result = Success, j;
    PanoramiXRes *pict;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureClipRectanglesReq);

    VERIFY_XIN_PICTURE(pict, stuff->picture, client, DixWriteAccess);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->picture = pict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderSetPictureClipRectangles])
            (client);
        if (result != Success)
            break;
    }

    return result;
}

static int
PanoramiXRenderSetPictureTransform(ClientPtr client)
{
    REQUEST(xRenderSetPictureTransformReq);
    int result = Success, j;
    PanoramiXRes *pict;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureTransformReq);

    VERIFY_XIN_PICTURE(pict, stuff->picture, client, DixWriteAccess);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->picture = pict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderSetPictureTransform]) (client);
        if (result != Success)
            break;
    }

    return result;
}

static int
PanoramiXRenderSetPictureFilter(ClientPtr client)
{
    REQUEST(xRenderSetPictureFilterReq);
    int result = Success, j;
    PanoramiXRes *pict;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureFilterReq);

    VERIFY_XIN_PICTURE(pict, stuff->picture, client, DixWriteAccess);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->picture = pict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderSetPictureFilter]) (client);
        if (result != Success)
            break;
    }

    return result;
}

static int
PanoramiXRenderFreePicture(ClientPtr client)
{
    PanoramiXRes *pict;
    int result = Success, j;

    REQUEST(xRenderFreePictureReq);

    REQUEST_SIZE_MATCH(xRenderFreePictureReq);

    client->errorValue = stuff->picture;

    VERIFY_XIN_PICTURE(pict, stuff->picture, client, DixDestroyAccess);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->picture = pict->info[j].id;
        result = (*PanoramiXSaveRenderVector[X_RenderFreePicture]) (client);
        if (result != Success)
            break;
    }

    /* Since ProcRenderFreePicture is using FreeResource, it will free
       our resource for us on the last pass through the loop above */

    return result;
}

static int
PanoramiXRenderComposite(ClientPtr client)
{
    PanoramiXRes *src, *msk, *dst;
    int result = Success, j;
    xRenderCompositeReq orig;

    REQUEST(xRenderCompositeReq);

    REQUEST_SIZE_MATCH(xRenderCompositeReq);

    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_ALPHA(msk, stuff->mask, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    orig = *stuff;

    FOR_NSCREENS_FORWARD(j) {
        stuff->src = src->info[j].id;
        if (src->u.pict.root) {
            stuff->xSrc = orig.xSrc - screenInfo.screens[j]->x;
            stuff->ySrc = orig.ySrc - screenInfo.screens[j]->y;
        }
        stuff->dst = dst->info[j].id;
        if (dst->u.pict.root) {
            stuff->xDst = orig.xDst - screenInfo.screens[j]->x;
            stuff->yDst = orig.yDst - screenInfo.screens[j]->y;
        }
        if (msk) {
            stuff->mask = msk->info[j].id;
            if (msk->u.pict.root) {
                stuff->xMask = orig.xMask - screenInfo.screens[j]->x;
                stuff->yMask = orig.yMask - screenInfo.screens[j]->y;
            }
        }
        result = (*PanoramiXSaveRenderVector[X_RenderComposite]) (client);
        if (result != Success)
            break;
    }

    return result;
}

static int
PanoramiXRenderCompositeGlyphs(ClientPtr client)
{
    PanoramiXRes *src, *dst;
    int result = Success, j;

    REQUEST(xRenderCompositeGlyphsReq);
    xGlyphElt origElt, *elt;
    INT16 xSrc, ySrc;

    REQUEST_AT_LEAST_SIZE(xRenderCompositeGlyphsReq);
    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    if (client->req_len << 2 >= (sizeof(xRenderCompositeGlyphsReq) +
                                 sizeof(xGlyphElt))) {
        elt = (xGlyphElt *) (stuff + 1);
        origElt = *elt;
        xSrc = stuff->xSrc;
        ySrc = stuff->ySrc;
        FOR_NSCREENS_FORWARD(j) {
            stuff->src = src->info[j].id;
            if (src->u.pict.root) {
                stuff->xSrc = xSrc - screenInfo.screens[j]->x;
                stuff->ySrc = ySrc - screenInfo.screens[j]->y;
            }
            stuff->dst = dst->info[j].id;
            if (dst->u.pict.root) {
                elt->deltax = origElt.deltax - screenInfo.screens[j]->x;
                elt->deltay = origElt.deltay - screenInfo.screens[j]->y;
            }
            result =
                (*PanoramiXSaveRenderVector[stuff->renderReqType]) (client);
            if (result != Success)
                break;
        }
    }

    return result;
}

static int
PanoramiXRenderFillRectangles(ClientPtr client)
{
    PanoramiXRes *dst;
    int result = Success, j;

    REQUEST(xRenderFillRectanglesReq);
    char *extra;
    int extra_len;

    REQUEST_AT_LEAST_SIZE(xRenderFillRectanglesReq);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);
    extra_len = (client->req_len << 2) - sizeof(xRenderFillRectanglesReq);
    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);
        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            if (dst->u.pict.root) {
                int x_off = screenInfo.screens[j]->x;
                int y_off = screenInfo.screens[j]->y;

                if (x_off || y_off) {
                    xRectangle *rects = (xRectangle *) (stuff + 1);
                    int i = extra_len / sizeof(xRectangle);

                    while (i--) {
                        rects->x -= x_off;
                        rects->y -= y_off;
                        rects++;
                    }
                }
            }
            stuff->dst = dst->info[j].id;
            result =
                (*PanoramiXSaveRenderVector[X_RenderFillRectangles]) (client);
            if (result != Success)
                break;
        }
        free(extra);
    }

    return result;
}

static int
PanoramiXRenderTrapezoids(ClientPtr client)
{
    PanoramiXRes *src, *dst;
    int result = Success, j;

    REQUEST(xRenderTrapezoidsReq);
    char *extra;
    int extra_len;

    REQUEST_AT_LEAST_SIZE(xRenderTrapezoidsReq);

    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    extra_len = (client->req_len << 2) - sizeof(xRenderTrapezoidsReq);

    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);

        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            if (dst->u.pict.root) {
                int x_off = screenInfo.screens[j]->x;
                int y_off = screenInfo.screens[j]->y;

                if (x_off || y_off) {
                    xTrapezoid *trap = (xTrapezoid *) (stuff + 1);
                    int i = extra_len / sizeof(xTrapezoid);

                    while (i--) {
                        trap->top -= y_off;
                        trap->bottom -= y_off;
                        trap->left.p1.x -= x_off;
                        trap->left.p1.y -= y_off;
                        trap->left.p2.x -= x_off;
                        trap->left.p2.y -= y_off;
                        trap->right.p1.x -= x_off;
                        trap->right.p1.y -= y_off;
                        trap->right.p2.x -= x_off;
                        trap->right.p2.y -= y_off;
                        trap++;
                    }
                }
            }

            stuff->src = src->info[j].id;
            stuff->dst = dst->info[j].id;
            result = (*PanoramiXSaveRenderVector[X_RenderTrapezoids]) (client);

            if (result != Success)
                break;
        }

        free(extra);
    }

    return result;
}

static int
PanoramiXRenderTriangles(ClientPtr client)
{
    PanoramiXRes *src, *dst;
    int result = Success, j;

    REQUEST(xRenderTrianglesReq);
    char *extra;
    int extra_len;

    REQUEST_AT_LEAST_SIZE(xRenderTrianglesReq);

    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    extra_len = (client->req_len << 2) - sizeof(xRenderTrianglesReq);

    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);

        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            if (dst->u.pict.root) {
                int x_off = screenInfo.screens[j]->x;
                int y_off = screenInfo.screens[j]->y;

                if (x_off || y_off) {
                    xTriangle *tri = (xTriangle *) (stuff + 1);
                    int i = extra_len / sizeof(xTriangle);

                    while (i--) {
                        tri->p1.x -= x_off;
                        tri->p1.y -= y_off;
                        tri->p2.x -= x_off;
                        tri->p2.y -= y_off;
                        tri->p3.x -= x_off;
                        tri->p3.y -= y_off;
                        tri++;
                    }
                }
            }

            stuff->src = src->info[j].id;
            stuff->dst = dst->info[j].id;
            result = (*PanoramiXSaveRenderVector[X_RenderTriangles]) (client);

            if (result != Success)
                break;
        }

        free(extra);
    }

    return result;
}

static int
PanoramiXRenderTriStrip(ClientPtr client)
{
    PanoramiXRes *src, *dst;
    int result = Success, j;

    REQUEST(xRenderTriStripReq);
    char *extra;
    int extra_len;

    REQUEST_AT_LEAST_SIZE(xRenderTriStripReq);

    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    extra_len = (client->req_len << 2) - sizeof(xRenderTriStripReq);

    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);

        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            if (dst->u.pict.root) {
                int x_off = screenInfo.screens[j]->x;
                int y_off = screenInfo.screens[j]->y;

                if (x_off || y_off) {
                    xPointFixed *fixed = (xPointFixed *) (stuff + 1);
                    int i = extra_len / sizeof(xPointFixed);

                    while (i--) {
                        fixed->x -= x_off;
                        fixed->y -= y_off;
                        fixed++;
                    }
                }
            }

            stuff->src = src->info[j].id;
            stuff->dst = dst->info[j].id;
            result = (*PanoramiXSaveRenderVector[X_RenderTriStrip]) (client);

            if (result != Success)
                break;
        }

        free(extra);
    }

    return result;
}

static int
PanoramiXRenderTriFan(ClientPtr client)
{
    PanoramiXRes *src, *dst;
    int result = Success, j;

    REQUEST(xRenderTriFanReq);
    char *extra;
    int extra_len;

    REQUEST_AT_LEAST_SIZE(xRenderTriFanReq);

    VERIFY_XIN_PICTURE(src, stuff->src, client, DixReadAccess);
    VERIFY_XIN_PICTURE(dst, stuff->dst, client, DixWriteAccess);

    extra_len = (client->req_len << 2) - sizeof(xRenderTriFanReq);

    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);

        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            if (dst->u.pict.root) {
                int x_off = screenInfo.screens[j]->x;
                int y_off = screenInfo.screens[j]->y;

                if (x_off || y_off) {
                    xPointFixed *fixed = (xPointFixed *) (stuff + 1);
                    int i = extra_len / sizeof(xPointFixed);

                    while (i--) {
                        fixed->x -= x_off;
                        fixed->y -= y_off;
                        fixed++;
                    }
                }
            }

            stuff->src = src->info[j].id;
            stuff->dst = dst->info[j].id;
            result = (*PanoramiXSaveRenderVector[X_RenderTriFan]) (client);

            if (result != Success)
                break;
        }

        free(extra);
    }

    return result;
}

static int
PanoramiXRenderAddTraps(ClientPtr client)
{
    PanoramiXRes *picture;
    int result = Success, j;

    REQUEST(xRenderAddTrapsReq);
    char *extra;
    int extra_len;
    INT16 x_off, y_off;

    REQUEST_AT_LEAST_SIZE(xRenderAddTrapsReq);
    VERIFY_XIN_PICTURE(picture, stuff->picture, client, DixWriteAccess);
    extra_len = (client->req_len << 2) - sizeof(xRenderAddTrapsReq);
    if (extra_len && (extra = (char *) malloc(extra_len))) {
        memcpy(extra, stuff + 1, extra_len);
        x_off = stuff->xOff;
        y_off = stuff->yOff;
        FOR_NSCREENS_FORWARD(j) {
            if (j)
                memcpy(stuff + 1, extra, extra_len);
            stuff->picture = picture->info[j].id;

            if (picture->u.pict.root) {
                stuff->xOff = x_off + screenInfo.screens[j]->x;
                stuff->yOff = y_off + screenInfo.screens[j]->y;
            }
            result = (*PanoramiXSaveRenderVector[X_RenderAddTraps]) (client);
            if (result != Success)
                break;
        }
        free(extra);
    }

    return result;
}

static int
PanoramiXRenderCreateSolidFill(ClientPtr client)
{
    REQUEST(xRenderCreateSolidFillReq);
    PanoramiXRes *newPict;
    int result = Success, j;

    REQUEST_AT_LEAST_SIZE(xRenderCreateSolidFillReq);

    if (!(newPict = (PanoramiXRes *) malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPict->type = XRT_PICTURE;
    panoramix_setup_ids(newPict, client, stuff->pid);
    newPict->u.pict.root = FALSE;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->pid = newPict->info[j].id;
        result = (*PanoramiXSaveRenderVector[X_RenderCreateSolidFill]) (client);
        if (result != Success)
            break;
    }

    if (result == Success)
        AddResource(newPict->info[0].id, XRT_PICTURE, newPict);
    else
        free(newPict);

    return result;
}

static int
PanoramiXRenderCreateLinearGradient(ClientPtr client)
{
    REQUEST(xRenderCreateLinearGradientReq);
    PanoramiXRes *newPict;
    int result = Success, j;

    REQUEST_AT_LEAST_SIZE(xRenderCreateLinearGradientReq);

    if (!(newPict = (PanoramiXRes *) malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPict->type = XRT_PICTURE;
    panoramix_setup_ids(newPict, client, stuff->pid);
    newPict->u.pict.root = FALSE;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->pid = newPict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderCreateLinearGradient]) (client);
        if (result != Success)
            break;
    }

    if (result == Success)
        AddResource(newPict->info[0].id, XRT_PICTURE, newPict);
    else
        free(newPict);

    return result;
}

static int
PanoramiXRenderCreateRadialGradient(ClientPtr client)
{
    REQUEST(xRenderCreateRadialGradientReq);
    PanoramiXRes *newPict;
    int result = Success, j;

    REQUEST_AT_LEAST_SIZE(xRenderCreateRadialGradientReq);

    if (!(newPict = (PanoramiXRes *) malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPict->type = XRT_PICTURE;
    panoramix_setup_ids(newPict, client, stuff->pid);
    newPict->u.pict.root = FALSE;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->pid = newPict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderCreateRadialGradient]) (client);
        if (result != Success)
            break;
    }

    if (result == Success)
        AddResource(newPict->info[0].id, XRT_PICTURE, newPict);
    else
        free(newPict);

    return result;
}

static int
PanoramiXRenderCreateConicalGradient(ClientPtr client)
{
    REQUEST(xRenderCreateConicalGradientReq);
    PanoramiXRes *newPict;
    int result = Success, j;

    REQUEST_AT_LEAST_SIZE(xRenderCreateConicalGradientReq);

    if (!(newPict = (PanoramiXRes *) malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPict->type = XRT_PICTURE;
    panoramix_setup_ids(newPict, client, stuff->pid);
    newPict->u.pict.root = FALSE;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->pid = newPict->info[j].id;
        result =
            (*PanoramiXSaveRenderVector[X_RenderCreateConicalGradient])
            (client);
        if (result != Success)
            break;
    }

    if (result == Success)
        AddResource(newPict->info[0].id, XRT_PICTURE, newPict);
    else
        free(newPict);

    return result;
}

void
PanoramiXRenderInit(void)
{
    int i;

    XRT_PICTURE = CreateNewResourceType(XineramaDeleteResource,
                                        "XineramaPicture");
    if (RenderErrBase)
        SetResourceTypeErrorValue(XRT_PICTURE, RenderErrBase + BadPicture);
    for (i = 0; i < RenderNumberRequests; i++)
        PanoramiXSaveRenderVector[i] = ProcRenderVector[i];
    /*
     * Stuff in Xinerama aware request processing hooks
     */
    ProcRenderVector[X_RenderCreatePicture] = PanoramiXRenderCreatePicture;
    ProcRenderVector[X_RenderChangePicture] = PanoramiXRenderChangePicture;
    ProcRenderVector[X_RenderSetPictureTransform] =
        PanoramiXRenderSetPictureTransform;
    ProcRenderVector[X_RenderSetPictureFilter] =
        PanoramiXRenderSetPictureFilter;
    ProcRenderVector[X_RenderSetPictureClipRectangles] =
        PanoramiXRenderSetPictureClipRectangles;
    ProcRenderVector[X_RenderFreePicture] = PanoramiXRenderFreePicture;
    ProcRenderVector[X_RenderComposite] = PanoramiXRenderComposite;
    ProcRenderVector[X_RenderCompositeGlyphs8] = PanoramiXRenderCompositeGlyphs;
    ProcRenderVector[X_RenderCompositeGlyphs16] =
        PanoramiXRenderCompositeGlyphs;
    ProcRenderVector[X_RenderCompositeGlyphs32] =
        PanoramiXRenderCompositeGlyphs;
    ProcRenderVector[X_RenderFillRectangles] = PanoramiXRenderFillRectangles;

    ProcRenderVector[X_RenderTrapezoids] = PanoramiXRenderTrapezoids;
    ProcRenderVector[X_RenderTriangles] = PanoramiXRenderTriangles;
    ProcRenderVector[X_RenderTriStrip] = PanoramiXRenderTriStrip;
    ProcRenderVector[X_RenderTriFan] = PanoramiXRenderTriFan;
    ProcRenderVector[X_RenderAddTraps] = PanoramiXRenderAddTraps;

    ProcRenderVector[X_RenderCreateSolidFill] = PanoramiXRenderCreateSolidFill;
    ProcRenderVector[X_RenderCreateLinearGradient] =
        PanoramiXRenderCreateLinearGradient;
    ProcRenderVector[X_RenderCreateRadialGradient] =
        PanoramiXRenderCreateRadialGradient;
    ProcRenderVector[X_RenderCreateConicalGradient] =
        PanoramiXRenderCreateConicalGradient;
}

void
PanoramiXRenderReset(void)
{
    int i;

    for (i = 0; i < RenderNumberRequests; i++)
        ProcRenderVector[i] = PanoramiXSaveRenderVector[i];
    RenderErrBase = 0;
}

#endif                          /* PANORAMIX */
