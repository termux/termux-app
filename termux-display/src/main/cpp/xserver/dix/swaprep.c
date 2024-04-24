/************************************************************

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "dixstruct.h"
#include <X11/fonts/fontstruct.h>
#include "scrnintstr.h"
#include "swaprep.h"
#include "globals.h"

static void SwapFontInfo(xQueryFontReply * pr);

static void SwapCharInfo(xCharInfo * pInfo);

static void SwapFont(xQueryFontReply * pr, Bool hasGlyphs);

/**
 * Thanks to Jack Palevich for testing and subsequently rewriting all this
 *
 *  \param size size in bytes
 */
void _X_COLD
Swap32Write(ClientPtr pClient, int size, CARD32 *pbuf)
{
    int i;

    size >>= 2;
    for (i = 0; i < size; i++)
        /* brackets are mandatory here, because "swapl" macro expands
           to several statements */
    {
        swapl(&pbuf[i]);
    }
    WriteToClient(pClient, size << 2, pbuf);
}

/**
 *
 * \param size size in bytes
 */
void _X_COLD
CopySwap32Write(ClientPtr pClient, int size, CARD32 *pbuf)
{
    int bufsize = size;
    CARD32 *pbufT;
    CARD32 *from, *to, *fromLast, *toLast;
    CARD32 tmpbuf[1];

    /* Allocate as big a buffer as we can... */
    while (!(pbufT = malloc(bufsize))) {
        bufsize >>= 1;
        if (bufsize == 4) {
            pbufT = tmpbuf;
            break;
        }
    }

    /* convert lengths from # of bytes to # of longs */
    size >>= 2;
    bufsize >>= 2;

    from = pbuf;
    fromLast = from + size;
    while (from < fromLast) {
        int nbytes;

        to = pbufT;
        toLast = to + min(bufsize, fromLast - from);
        nbytes = (toLast - to) << 2;
        while (to < toLast) {
            /* can't write "cpswapl(*from++, *to++)" because cpswapl is a macro
               that evaluates its args more than once */
            cpswapl(*from, *to);
            from++;
            to++;
        }
        WriteToClient(pClient, nbytes, pbufT);
    }

    if (pbufT != tmpbuf)
        free(pbufT);
}

/**
 *
 * \param size size in bytes
 */
void _X_COLD
CopySwap16Write(ClientPtr pClient, int size, short *pbuf)
{
    int bufsize = size;
    short *pbufT;
    short *from, *to, *fromLast, *toLast;
    short tmpbuf[2];

    /* Allocate as big a buffer as we can... */
    while (!(pbufT = malloc(bufsize))) {
        bufsize >>= 1;
        if (bufsize == 4) {
            pbufT = tmpbuf;
            break;
        }
    }

    /* convert lengths from # of bytes to # of shorts */
    size >>= 1;
    bufsize >>= 1;

    from = pbuf;
    fromLast = from + size;
    while (from < fromLast) {
        int nbytes;

        to = pbufT;
        toLast = to + min(bufsize, fromLast - from);
        nbytes = (toLast - to) << 1;
        while (to < toLast) {
            /* can't write "cpswaps(*from++, *to++)" because cpswaps is a macro
               that evaluates its args more than once */
            cpswaps(*from, *to);
            from++;
            to++;
        }
        WriteToClient(pClient, nbytes, pbufT);
    }

    if (pbufT != tmpbuf)
        free(pbufT);
}

/* Extra-small reply */
void _X_COLD
SGenericReply(ClientPtr pClient, int size, xGenericReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    WriteToClient(pClient, size, pRep);
}

/* Extra-large reply */
void _X_COLD
SGetWindowAttributesReply(ClientPtr pClient, int size,
                          xGetWindowAttributesReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->visualID);
    swaps(&pRep->class);
    swapl(&pRep->backingBitPlanes);
    swapl(&pRep->backingPixel);
    swapl(&pRep->colormap);
    swapl(&pRep->allEventMasks);
    swapl(&pRep->yourEventMask);
    swaps(&pRep->doNotPropagateMask);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetGeometryReply(ClientPtr pClient, int size, xGetGeometryReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->root);
    swaps(&pRep->x);
    swaps(&pRep->y);
    swaps(&pRep->width);
    swaps(&pRep->height);
    swaps(&pRep->borderWidth);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SQueryTreeReply(ClientPtr pClient, int size, xQueryTreeReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->root);
    swapl(&pRep->parent);
    swaps(&pRep->nChildren);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SInternAtomReply(ClientPtr pClient, int size, xInternAtomReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->atom);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetAtomNameReply(ClientPtr pClient, int size, xGetAtomNameReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nameLength);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetPropertyReply(ClientPtr pClient, int size, xGetPropertyReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->propertyType);
    swapl(&pRep->bytesAfter);
    swapl(&pRep->nItems);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SListPropertiesReply(ClientPtr pClient, int size, xListPropertiesReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nProperties);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetSelectionOwnerReply(ClientPtr pClient, int size,
                        xGetSelectionOwnerReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->owner);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SQueryPointerReply(ClientPtr pClient, int size, xQueryPointerReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->root);
    swapl(&pRep->child);
    swaps(&pRep->rootX);
    swaps(&pRep->rootY);
    swaps(&pRep->winX);
    swaps(&pRep->winY);
    swaps(&pRep->mask);
    WriteToClient(pClient, size, pRep);
}

static void _X_COLD
SwapTimecoord(xTimecoord * pCoord)
{
    swapl(&pCoord->time);
    swaps(&pCoord->x);
    swaps(&pCoord->y);
}

void _X_COLD
SwapTimeCoordWrite(ClientPtr pClient, int size, xTimecoord * pRep)
{
    int i, n;
    xTimecoord *pRepT;

    n = size / sizeof(xTimecoord);
    pRepT = pRep;
    for (i = 0; i < n; i++) {
        SwapTimecoord(pRepT);
        pRepT++;
    }
    WriteToClient(pClient, size, pRep);

}

void _X_COLD
SGetMotionEventsReply(ClientPtr pClient, int size, xGetMotionEventsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->nEvents);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
STranslateCoordsReply(ClientPtr pClient, int size, xTranslateCoordsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->child);
    swaps(&pRep->dstX);
    swaps(&pRep->dstY);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetInputFocusReply(ClientPtr pClient, int size, xGetInputFocusReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->focus);
    WriteToClient(pClient, size, pRep);
}

/* extra long reply */
void _X_COLD
SQueryKeymapReply(ClientPtr pClient, int size, xQueryKeymapReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    WriteToClient(pClient, size, pRep);
}

static void _X_COLD
SwapCharInfo(xCharInfo * pInfo)
{
    swaps(&pInfo->leftSideBearing);
    swaps(&pInfo->rightSideBearing);
    swaps(&pInfo->characterWidth);
    swaps(&pInfo->ascent);
    swaps(&pInfo->descent);
    swaps(&pInfo->attributes);
}

static void _X_COLD
SwapFontInfo(xQueryFontReply * pr)
{
    swaps(&pr->minCharOrByte2);
    swaps(&pr->maxCharOrByte2);
    swaps(&pr->defaultChar);
    swaps(&pr->nFontProps);
    swaps(&pr->fontAscent);
    swaps(&pr->fontDescent);
    SwapCharInfo(&pr->minBounds);
    SwapCharInfo(&pr->maxBounds);
    swapl(&pr->nCharInfos);
}

static void _X_COLD
SwapFont(xQueryFontReply * pr, Bool hasGlyphs)
{
    unsigned i;
    xCharInfo *pxci;
    unsigned nchars, nprops;
    char *pby;

    swaps(&pr->sequenceNumber);
    swapl(&pr->length);
    nchars = pr->nCharInfos;
    nprops = pr->nFontProps;
    SwapFontInfo(pr);
    pby = (char *) &pr[1];
    /* Font properties are an atom and either an int32 or a CARD32, so
     * they are always 2 4 byte values */
    for (i = 0; i < nprops; i++) {
        swapl((int *) pby);
        pby += 4;
        swapl((int *) pby);
        pby += 4;
    }
    if (hasGlyphs) {
        pxci = (xCharInfo *) pby;
        for (i = 0; i < nchars; i++, pxci++)
            SwapCharInfo(pxci);
    }
}

void _X_COLD
SQueryFontReply(ClientPtr pClient, int size, xQueryFontReply * pRep)
{
    SwapFont(pRep, TRUE);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SQueryTextExtentsReply(ClientPtr pClient, int size,
                       xQueryTextExtentsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->fontAscent);
    swaps(&pRep->fontDescent);
    swaps(&pRep->overallAscent);
    swaps(&pRep->overallDescent);
    swapl(&pRep->overallWidth);
    swapl(&pRep->overallLeft);
    swapl(&pRep->overallRight);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SListFontsReply(ClientPtr pClient, int size, xListFontsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nFonts);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SListFontsWithInfoReply(ClientPtr pClient, int size,
                        xListFontsWithInfoReply * pRep)
{
    SwapFont((xQueryFontReply *) pRep, FALSE);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetFontPathReply(ClientPtr pClient, int size, xGetFontPathReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nPaths);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetImageReply(ClientPtr pClient, int size, xGetImageReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->visual);
    WriteToClient(pClient, size, pRep);
    /* Fortunately, image doesn't need swapping */
}

void _X_COLD
SListInstalledColormapsReply(ClientPtr pClient, int size,
                             xListInstalledColormapsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nColormaps);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SAllocColorReply(ClientPtr pClient, int size, xAllocColorReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->red);
    swaps(&pRep->green);
    swaps(&pRep->blue);
    swapl(&pRep->pixel);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SAllocNamedColorReply(ClientPtr pClient, int size, xAllocNamedColorReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->pixel);
    swaps(&pRep->exactRed);
    swaps(&pRep->exactGreen);
    swaps(&pRep->exactBlue);
    swaps(&pRep->screenRed);
    swaps(&pRep->screenGreen);
    swaps(&pRep->screenBlue);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SAllocColorCellsReply(ClientPtr pClient, int size, xAllocColorCellsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nPixels);
    swaps(&pRep->nMasks);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SAllocColorPlanesReply(ClientPtr pClient, int size,
                       xAllocColorPlanesReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nPixels);
    swapl(&pRep->redMask);
    swapl(&pRep->greenMask);
    swapl(&pRep->blueMask);
    WriteToClient(pClient, size, pRep);
}

static void _X_COLD
SwapRGB(xrgb * prgb)
{
    swaps(&prgb->red);
    swaps(&prgb->green);
    swaps(&prgb->blue);
}

void _X_COLD
SQColorsExtend(ClientPtr pClient, int size, xrgb * prgb)
{
    int i, n;
    xrgb *prgbT;

    n = size / sizeof(xrgb);
    prgbT = prgb;
    for (i = 0; i < n; i++) {
        SwapRGB(prgbT);
        prgbT++;
    }
    WriteToClient(pClient, size, prgb);
}

void _X_COLD
SQueryColorsReply(ClientPtr pClient, int size, xQueryColorsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nColors);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SLookupColorReply(ClientPtr pClient, int size, xLookupColorReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->exactRed);
    swaps(&pRep->exactGreen);
    swaps(&pRep->exactBlue);
    swaps(&pRep->screenRed);
    swaps(&pRep->screenGreen);
    swaps(&pRep->screenBlue);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SQueryBestSizeReply(ClientPtr pClient, int size, xQueryBestSizeReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->width);
    swaps(&pRep->height);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SListExtensionsReply(ClientPtr pClient, int size, xListExtensionsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetKeyboardMappingReply(ClientPtr pClient, int size,
                         xGetKeyboardMappingReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetPointerMappingReply(ClientPtr pClient, int size,
                        xGetPointerMappingReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetModifierMappingReply(ClientPtr pClient, int size,
                         xGetModifierMappingReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetKeyboardControlReply(ClientPtr pClient, int size,
                         xGetKeyboardControlReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swapl(&pRep->ledMask);
    swaps(&pRep->bellPitch);
    swaps(&pRep->bellDuration);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetPointerControlReply(ClientPtr pClient, int size,
                        xGetPointerControlReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->accelNumerator);
    swaps(&pRep->accelDenominator);
    swaps(&pRep->threshold);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SGetScreenSaverReply(ClientPtr pClient, int size, xGetScreenSaverReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swaps(&pRep->timeout);
    swaps(&pRep->interval);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SLHostsExtend(ClientPtr pClient, int size, char *buf)
{
    char *bufT = buf;
    char *endbuf = buf + size;

    while (bufT < endbuf) {
        xHostEntry *host = (xHostEntry *) bufT;
        int len = host->length;

        swaps(&host->length);
        bufT += sizeof(xHostEntry) + pad_to_int32(len);
    }
    WriteToClient(pClient, size, buf);
}

void _X_COLD
SListHostsReply(ClientPtr pClient, int size, xListHostsReply * pRep)
{
    swaps(&pRep->sequenceNumber);
    swapl(&pRep->length);
    swaps(&pRep->nHosts);
    WriteToClient(pClient, size, pRep);
}

void _X_COLD
SErrorEvent(xError * from, xError * to)
{
    to->type = X_Error;
    to->errorCode = from->errorCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->resourceID, to->resourceID);
    cpswaps(from->minorCode, to->minorCode);
    to->majorCode = from->majorCode;
}

void _X_COLD
SKeyButtonPtrEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.keyButtonPointer.time, to->u.keyButtonPointer.time);
    cpswapl(from->u.keyButtonPointer.root, to->u.keyButtonPointer.root);
    cpswapl(from->u.keyButtonPointer.event, to->u.keyButtonPointer.event);
    cpswapl(from->u.keyButtonPointer.child, to->u.keyButtonPointer.child);
    cpswaps(from->u.keyButtonPointer.rootX, to->u.keyButtonPointer.rootX);
    cpswaps(from->u.keyButtonPointer.rootY, to->u.keyButtonPointer.rootY);
    cpswaps(from->u.keyButtonPointer.eventX, to->u.keyButtonPointer.eventX);
    cpswaps(from->u.keyButtonPointer.eventY, to->u.keyButtonPointer.eventY);
    cpswaps(from->u.keyButtonPointer.state, to->u.keyButtonPointer.state);
    to->u.keyButtonPointer.sameScreen = from->u.keyButtonPointer.sameScreen;
}

void _X_COLD
SEnterLeaveEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.enterLeave.time, to->u.enterLeave.time);
    cpswapl(from->u.enterLeave.root, to->u.enterLeave.root);
    cpswapl(from->u.enterLeave.event, to->u.enterLeave.event);
    cpswapl(from->u.enterLeave.child, to->u.enterLeave.child);
    cpswaps(from->u.enterLeave.rootX, to->u.enterLeave.rootX);
    cpswaps(from->u.enterLeave.rootY, to->u.enterLeave.rootY);
    cpswaps(from->u.enterLeave.eventX, to->u.enterLeave.eventX);
    cpswaps(from->u.enterLeave.eventY, to->u.enterLeave.eventY);
    cpswaps(from->u.enterLeave.state, to->u.enterLeave.state);
    to->u.enterLeave.mode = from->u.enterLeave.mode;
    to->u.enterLeave.flags = from->u.enterLeave.flags;
}

void _X_COLD
SFocusEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.focus.window, to->u.focus.window);
    to->u.focus.mode = from->u.focus.mode;
}

void _X_COLD
SExposeEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.expose.window, to->u.expose.window);
    cpswaps(from->u.expose.x, to->u.expose.x);
    cpswaps(from->u.expose.y, to->u.expose.y);
    cpswaps(from->u.expose.width, to->u.expose.width);
    cpswaps(from->u.expose.height, to->u.expose.height);
    cpswaps(from->u.expose.count, to->u.expose.count);
}

void _X_COLD
SGraphicsExposureEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.graphicsExposure.drawable, to->u.graphicsExposure.drawable);
    cpswaps(from->u.graphicsExposure.x, to->u.graphicsExposure.x);
    cpswaps(from->u.graphicsExposure.y, to->u.graphicsExposure.y);
    cpswaps(from->u.graphicsExposure.width, to->u.graphicsExposure.width);
    cpswaps(from->u.graphicsExposure.height, to->u.graphicsExposure.height);
    cpswaps(from->u.graphicsExposure.minorEvent,
            to->u.graphicsExposure.minorEvent);
    cpswaps(from->u.graphicsExposure.count, to->u.graphicsExposure.count);
    to->u.graphicsExposure.majorEvent = from->u.graphicsExposure.majorEvent;
}

void _X_COLD
SNoExposureEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.noExposure.drawable, to->u.noExposure.drawable);
    cpswaps(from->u.noExposure.minorEvent, to->u.noExposure.minorEvent);
    to->u.noExposure.majorEvent = from->u.noExposure.majorEvent;
}

void _X_COLD
SVisibilityEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.visibility.window, to->u.visibility.window);
    to->u.visibility.state = from->u.visibility.state;
}

void _X_COLD
SCreateNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.createNotify.window, to->u.createNotify.window);
    cpswapl(from->u.createNotify.parent, to->u.createNotify.parent);
    cpswaps(from->u.createNotify.x, to->u.createNotify.x);
    cpswaps(from->u.createNotify.y, to->u.createNotify.y);
    cpswaps(from->u.createNotify.width, to->u.createNotify.width);
    cpswaps(from->u.createNotify.height, to->u.createNotify.height);
    cpswaps(from->u.createNotify.borderWidth, to->u.createNotify.borderWidth);
    to->u.createNotify.override = from->u.createNotify.override;
}

void _X_COLD
SDestroyNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.destroyNotify.event, to->u.destroyNotify.event);
    cpswapl(from->u.destroyNotify.window, to->u.destroyNotify.window);
}

void _X_COLD
SUnmapNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.unmapNotify.event, to->u.unmapNotify.event);
    cpswapl(from->u.unmapNotify.window, to->u.unmapNotify.window);
    to->u.unmapNotify.fromConfigure = from->u.unmapNotify.fromConfigure;
}

void _X_COLD
SMapNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.mapNotify.event, to->u.mapNotify.event);
    cpswapl(from->u.mapNotify.window, to->u.mapNotify.window);
    to->u.mapNotify.override = from->u.mapNotify.override;
}

void _X_COLD
SMapRequestEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.mapRequest.parent, to->u.mapRequest.parent);
    cpswapl(from->u.mapRequest.window, to->u.mapRequest.window);
}

void _X_COLD
SReparentEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.reparent.event, to->u.reparent.event);
    cpswapl(from->u.reparent.window, to->u.reparent.window);
    cpswapl(from->u.reparent.parent, to->u.reparent.parent);
    cpswaps(from->u.reparent.x, to->u.reparent.x);
    cpswaps(from->u.reparent.y, to->u.reparent.y);
    to->u.reparent.override = from->u.reparent.override;
}

void _X_COLD
SConfigureNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.configureNotify.event, to->u.configureNotify.event);
    cpswapl(from->u.configureNotify.window, to->u.configureNotify.window);
    cpswapl(from->u.configureNotify.aboveSibling,
            to->u.configureNotify.aboveSibling);
    cpswaps(from->u.configureNotify.x, to->u.configureNotify.x);
    cpswaps(from->u.configureNotify.y, to->u.configureNotify.y);
    cpswaps(from->u.configureNotify.width, to->u.configureNotify.width);
    cpswaps(from->u.configureNotify.height, to->u.configureNotify.height);
    cpswaps(from->u.configureNotify.borderWidth,
            to->u.configureNotify.borderWidth);
    to->u.configureNotify.override = from->u.configureNotify.override;
}

void _X_COLD
SConfigureRequestEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;  /* actually stack-mode */
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.configureRequest.parent, to->u.configureRequest.parent);
    cpswapl(from->u.configureRequest.window, to->u.configureRequest.window);
    cpswapl(from->u.configureRequest.sibling, to->u.configureRequest.sibling);
    cpswaps(from->u.configureRequest.x, to->u.configureRequest.x);
    cpswaps(from->u.configureRequest.y, to->u.configureRequest.y);
    cpswaps(from->u.configureRequest.width, to->u.configureRequest.width);
    cpswaps(from->u.configureRequest.height, to->u.configureRequest.height);
    cpswaps(from->u.configureRequest.borderWidth,
            to->u.configureRequest.borderWidth);
    cpswaps(from->u.configureRequest.valueMask,
            to->u.configureRequest.valueMask);
}

void _X_COLD
SGravityEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.gravity.event, to->u.gravity.event);
    cpswapl(from->u.gravity.window, to->u.gravity.window);
    cpswaps(from->u.gravity.x, to->u.gravity.x);
    cpswaps(from->u.gravity.y, to->u.gravity.y);
}

void _X_COLD
SResizeRequestEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.resizeRequest.window, to->u.resizeRequest.window);
    cpswaps(from->u.resizeRequest.width, to->u.resizeRequest.width);
    cpswaps(from->u.resizeRequest.height, to->u.resizeRequest.height);
}

void _X_COLD
SCirculateEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.circulate.event, to->u.circulate.event);
    cpswapl(from->u.circulate.window, to->u.circulate.window);
    cpswapl(from->u.circulate.parent, to->u.circulate.parent);
    to->u.circulate.place = from->u.circulate.place;
}

void _X_COLD
SPropertyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.property.window, to->u.property.window);
    cpswapl(from->u.property.atom, to->u.property.atom);
    cpswapl(from->u.property.time, to->u.property.time);
    to->u.property.state = from->u.property.state;
}

void _X_COLD
SSelectionClearEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.selectionClear.time, to->u.selectionClear.time);
    cpswapl(from->u.selectionClear.window, to->u.selectionClear.window);
    cpswapl(from->u.selectionClear.atom, to->u.selectionClear.atom);
}

void _X_COLD
SSelectionRequestEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.selectionRequest.time, to->u.selectionRequest.time);
    cpswapl(from->u.selectionRequest.owner, to->u.selectionRequest.owner);
    cpswapl(from->u.selectionRequest.requestor,
            to->u.selectionRequest.requestor);
    cpswapl(from->u.selectionRequest.selection,
            to->u.selectionRequest.selection);
    cpswapl(from->u.selectionRequest.target, to->u.selectionRequest.target);
    cpswapl(from->u.selectionRequest.property, to->u.selectionRequest.property);
}

void _X_COLD
SSelectionNotifyEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.selectionNotify.time, to->u.selectionNotify.time);
    cpswapl(from->u.selectionNotify.requestor, to->u.selectionNotify.requestor);
    cpswapl(from->u.selectionNotify.selection, to->u.selectionNotify.selection);
    cpswapl(from->u.selectionNotify.target, to->u.selectionNotify.target);
    cpswapl(from->u.selectionNotify.property, to->u.selectionNotify.property);
}

void _X_COLD
SColormapEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.colormap.window, to->u.colormap.window);
    cpswapl(from->u.colormap.colormap, to->u.colormap.colormap);
    to->u.colormap.new = from->u.colormap.new;
    to->u.colormap.state = from->u.colormap.state;
}

void _X_COLD
SMappingEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    to->u.mappingNotify.request = from->u.mappingNotify.request;
    to->u.mappingNotify.firstKeyCode = from->u.mappingNotify.firstKeyCode;
    to->u.mappingNotify.count = from->u.mappingNotify.count;
}

void _X_COLD
SClientMessageEvent(xEvent *from, xEvent *to)
{
    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;  /* actually format */
    cpswaps(from->u.u.sequenceNumber, to->u.u.sequenceNumber);
    cpswapl(from->u.clientMessage.window, to->u.clientMessage.window);
    cpswapl(from->u.clientMessage.u.l.type, to->u.clientMessage.u.l.type);
    switch (from->u.u.detail) {
    case 8:
        memmove(to->u.clientMessage.u.b.bytes,
                from->u.clientMessage.u.b.bytes, 20);
        break;
    case 16:
        cpswaps(from->u.clientMessage.u.s.shorts0,
                to->u.clientMessage.u.s.shorts0);
        cpswaps(from->u.clientMessage.u.s.shorts1,
                to->u.clientMessage.u.s.shorts1);
        cpswaps(from->u.clientMessage.u.s.shorts2,
                to->u.clientMessage.u.s.shorts2);
        cpswaps(from->u.clientMessage.u.s.shorts3,
                to->u.clientMessage.u.s.shorts3);
        cpswaps(from->u.clientMessage.u.s.shorts4,
                to->u.clientMessage.u.s.shorts4);
        cpswaps(from->u.clientMessage.u.s.shorts5,
                to->u.clientMessage.u.s.shorts5);
        cpswaps(from->u.clientMessage.u.s.shorts6,
                to->u.clientMessage.u.s.shorts6);
        cpswaps(from->u.clientMessage.u.s.shorts7,
                to->u.clientMessage.u.s.shorts7);
        cpswaps(from->u.clientMessage.u.s.shorts8,
                to->u.clientMessage.u.s.shorts8);
        cpswaps(from->u.clientMessage.u.s.shorts9,
                to->u.clientMessage.u.s.shorts9);
        break;
    case 32:
        cpswapl(from->u.clientMessage.u.l.longs0,
                to->u.clientMessage.u.l.longs0);
        cpswapl(from->u.clientMessage.u.l.longs1,
                to->u.clientMessage.u.l.longs1);
        cpswapl(from->u.clientMessage.u.l.longs2,
                to->u.clientMessage.u.l.longs2);
        cpswapl(from->u.clientMessage.u.l.longs3,
                to->u.clientMessage.u.l.longs3);
        cpswapl(from->u.clientMessage.u.l.longs4,
                to->u.clientMessage.u.l.longs4);
        break;
    }
}

void _X_COLD
SKeymapNotifyEvent(xEvent *from, xEvent *to)
{
    /* Keymap notify events are special; they have no
       sequence number field, and contain entirely 8-bit data */
    *to = *from;
}

static void _X_COLD
SwapConnSetup(xConnSetup * pConnSetup, xConnSetup * pConnSetupT)
{
    cpswapl(pConnSetup->release, pConnSetupT->release);
    cpswapl(pConnSetup->ridBase, pConnSetupT->ridBase);
    cpswapl(pConnSetup->ridMask, pConnSetupT->ridMask);
    cpswapl(pConnSetup->motionBufferSize, pConnSetupT->motionBufferSize);
    cpswaps(pConnSetup->nbytesVendor, pConnSetupT->nbytesVendor);
    cpswaps(pConnSetup->maxRequestSize, pConnSetupT->maxRequestSize);
    pConnSetupT->minKeyCode = pConnSetup->minKeyCode;
    pConnSetupT->maxKeyCode = pConnSetup->maxKeyCode;
    pConnSetupT->numRoots = pConnSetup->numRoots;
    pConnSetupT->numFormats = pConnSetup->numFormats;
    pConnSetupT->imageByteOrder = pConnSetup->imageByteOrder;
    pConnSetupT->bitmapBitOrder = pConnSetup->bitmapBitOrder;
    pConnSetupT->bitmapScanlineUnit = pConnSetup->bitmapScanlineUnit;
    pConnSetupT->bitmapScanlinePad = pConnSetup->bitmapScanlinePad;
}

static void _X_COLD
SwapWinRoot(xWindowRoot * pRoot, xWindowRoot * pRootT)
{
    cpswapl(pRoot->windowId, pRootT->windowId);
    cpswapl(pRoot->defaultColormap, pRootT->defaultColormap);
    cpswapl(pRoot->whitePixel, pRootT->whitePixel);
    cpswapl(pRoot->blackPixel, pRootT->blackPixel);
    cpswapl(pRoot->currentInputMask, pRootT->currentInputMask);
    cpswaps(pRoot->pixWidth, pRootT->pixWidth);
    cpswaps(pRoot->pixHeight, pRootT->pixHeight);
    cpswaps(pRoot->mmWidth, pRootT->mmWidth);
    cpswaps(pRoot->mmHeight, pRootT->mmHeight);
    cpswaps(pRoot->minInstalledMaps, pRootT->minInstalledMaps);
    cpswaps(pRoot->maxInstalledMaps, pRootT->maxInstalledMaps);
    cpswapl(pRoot->rootVisualID, pRootT->rootVisualID);
    pRootT->backingStore = pRoot->backingStore;
    pRootT->saveUnders = pRoot->saveUnders;
    pRootT->rootDepth = pRoot->rootDepth;
    pRootT->nDepths = pRoot->nDepths;
}

static void _X_COLD
SwapVisual(xVisualType * pVis, xVisualType * pVisT)
{
    cpswapl(pVis->visualID, pVisT->visualID);
    pVisT->class = pVis->class;
    pVisT->bitsPerRGB = pVis->bitsPerRGB;
    cpswaps(pVis->colormapEntries, pVisT->colormapEntries);
    cpswapl(pVis->redMask, pVisT->redMask);
    cpswapl(pVis->greenMask, pVisT->greenMask);
    cpswapl(pVis->blueMask, pVisT->blueMask);
}

void _X_COLD
SwapConnSetupInfo(char *pInfo, char *pInfoT)
{
    int i, j, k;
    xConnSetup *pConnSetup = (xConnSetup *) pInfo;
    xDepth *depth;
    xWindowRoot *root;

    SwapConnSetup(pConnSetup, (xConnSetup *) pInfoT);
    pInfo += sizeof(xConnSetup);
    pInfoT += sizeof(xConnSetup);

    /* Copy the vendor string */
    i = pad_to_int32(pConnSetup->nbytesVendor);
    memcpy(pInfoT, pInfo, i);
    pInfo += i;
    pInfoT += i;

    /* The Pixmap formats don't need to be swapped, just copied. */
    i = sizeof(xPixmapFormat) * pConnSetup->numFormats;
    memcpy(pInfoT, pInfo, i);
    pInfo += i;
    pInfoT += i;

    for (i = 0; i < pConnSetup->numRoots; i++) {
        root = (xWindowRoot *) pInfo;
        SwapWinRoot(root, (xWindowRoot *) pInfoT);
        pInfo += sizeof(xWindowRoot);
        pInfoT += sizeof(xWindowRoot);

        for (j = 0; j < root->nDepths; j++) {
            depth = (xDepth *) pInfo;
            ((xDepth *) pInfoT)->depth = depth->depth;
            cpswaps(depth->nVisuals, ((xDepth *) pInfoT)->nVisuals);
            pInfo += sizeof(xDepth);
            pInfoT += sizeof(xDepth);
            for (k = 0; k < depth->nVisuals; k++) {
                SwapVisual((xVisualType *) pInfo, (xVisualType *) pInfoT);
                pInfo += sizeof(xVisualType);
                pInfoT += sizeof(xVisualType);
            }
        }
    }
}

void _X_COLD
WriteSConnectionInfo(ClientPtr pClient, unsigned long size, char *pInfo)
{
    char *pInfoTBase;

    pInfoTBase = malloc(size);
    if (!pInfoTBase) {
        pClient->noClientException = -1;
        return;
    }
    SwapConnSetupInfo(pInfo, pInfoTBase);
    WriteToClient(pClient, (int) size, pInfoTBase);
    free(pInfoTBase);
}

void _X_COLD
SwapConnSetupPrefix(xConnSetupPrefix * pcspFrom, xConnSetupPrefix * pcspTo)
{
    pcspTo->success = pcspFrom->success;
    pcspTo->lengthReason = pcspFrom->lengthReason;
    cpswaps(pcspFrom->majorVersion, pcspTo->majorVersion);
    cpswaps(pcspFrom->minorVersion, pcspTo->minorVersion);
    cpswaps(pcspFrom->length, pcspTo->length);
}

void _X_COLD
WriteSConnSetupPrefix(ClientPtr pClient, xConnSetupPrefix * pcsp)
{
    xConnSetupPrefix cspT;

    SwapConnSetupPrefix(pcsp, &cspT);
    WriteToClient(pClient, sizeof(cspT), &cspT);
}

/*
 * Dummy entry for ReplySwapVector[]
 */

void _X_COLD
ReplyNotSwappd(ClientPtr pClient, int size, void *pbuf)
{
    FatalError("Not implemented");
}
