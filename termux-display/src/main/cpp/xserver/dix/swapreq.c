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
#include <X11/Xprotostr.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"         /* for SendEvent */
#include "swapreq.h"

/* Thanks to Jack Palevich for testing and subsequently rewriting all this */

/* Byte swap a list of longs */
void
SwapLongs(CARD32 *list, unsigned long count)
{
    while (count >= 8) {
        swapl(list + 0);
        swapl(list + 1);
        swapl(list + 2);
        swapl(list + 3);
        swapl(list + 4);
        swapl(list + 5);
        swapl(list + 6);
        swapl(list + 7);
        list += 8;
        count -= 8;
    }
    if (count != 0) {
        do {
            swapl(list);
            list++;
        } while (--count != 0);
    }
}

/* Byte swap a list of shorts */
void
SwapShorts(short *list, unsigned long count)
{
    while (count >= 16) {
        swaps(list + 0);
        swaps(list + 1);
        swaps(list + 2);
        swaps(list + 3);
        swaps(list + 4);
        swaps(list + 5);
        swaps(list + 6);
        swaps(list + 7);
        swaps(list + 8);
        swaps(list + 9);
        swaps(list + 10);
        swaps(list + 11);
        swaps(list + 12);
        swaps(list + 13);
        swaps(list + 14);
        swaps(list + 15);
        list += 16;
        count -= 16;
    }
    if (count != 0) {
        do {
            swaps(list);
            list++;
        } while (--count != 0);
    }
}

/* The following is used for all requests that have
   no fields to be swapped (except "length") */
int _X_COLD
SProcSimpleReq(ClientPtr client)
{
    REQUEST(xReq);
    swaps(&stuff->length);
    return (*ProcVector[stuff->reqType]) (client);
}

/* The following is used for all requests that have
   only a single 32-bit field to be swapped, coming
   right after the "length" field */
int _X_COLD
SProcResourceReq(ClientPtr client)
{
    REQUEST(xResourceReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xResourceReq);        /* not EXACT */
    swapl(&stuff->id);
    return (*ProcVector[stuff->reqType]) (client);
}

int _X_COLD
SProcCreateWindow(ClientPtr client)
{
    REQUEST(xCreateWindowReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);
    swapl(&stuff->wid);
    swapl(&stuff->parent);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swaps(&stuff->borderWidth);
    swaps(&stuff->class);
    swapl(&stuff->visual);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_CreateWindow]) (client));
}

int _X_COLD
SProcChangeWindowAttributes(ClientPtr client)
{
    REQUEST(xChangeWindowAttributesReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    swapl(&stuff->window);
    swapl(&stuff->valueMask);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeWindowAttributes]) (client));
}

int _X_COLD
SProcReparentWindow(ClientPtr client)
{
    REQUEST(xReparentWindowReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xReparentWindowReq);
    swapl(&stuff->window);
    swapl(&stuff->parent);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return ((*ProcVector[X_ReparentWindow]) (client));
}

int _X_COLD
SProcConfigureWindow(ClientPtr client)
{
    REQUEST(xConfigureWindowReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);
    swapl(&stuff->window);
    swaps(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_ConfigureWindow]) (client));

}

int _X_COLD
SProcInternAtom(ClientPtr client)
{
    REQUEST(xInternAtomReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xInternAtomReq);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_InternAtom]) (client));
}

int _X_COLD
SProcChangeProperty(ClientPtr client)
{
    REQUEST(xChangePropertyReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangePropertyReq);
    swapl(&stuff->window);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->nUnits);
    switch (stuff->format) {
    case 8:
        break;
    case 16:
        SwapRestS(stuff);
        break;
    case 32:
        SwapRestL(stuff);
        break;
    }
    return ((*ProcVector[X_ChangeProperty]) (client));
}

int _X_COLD
SProcDeleteProperty(ClientPtr client)
{
    REQUEST(xDeletePropertyReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    swapl(&stuff->window);
    swapl(&stuff->property);
    return ((*ProcVector[X_DeleteProperty]) (client));

}

int _X_COLD
SProcGetProperty(ClientPtr client)
{
    REQUEST(xGetPropertyReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGetPropertyReq);
    swapl(&stuff->window);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->longOffset);
    swapl(&stuff->longLength);
    return ((*ProcVector[X_GetProperty]) (client));
}

int _X_COLD
SProcSetSelectionOwner(ClientPtr client)
{
    REQUEST(xSetSelectionOwnerReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSetSelectionOwnerReq);
    swapl(&stuff->window);
    swapl(&stuff->selection);
    swapl(&stuff->time);
    return ((*ProcVector[X_SetSelectionOwner]) (client));
}

int _X_COLD
SProcConvertSelection(ClientPtr client)
{
    REQUEST(xConvertSelectionReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xConvertSelectionReq);
    swapl(&stuff->requestor);
    swapl(&stuff->selection);
    swapl(&stuff->target);
    swapl(&stuff->property);
    swapl(&stuff->time);
    return ((*ProcVector[X_ConvertSelection]) (client));
}

int _X_COLD
SProcSendEvent(ClientPtr client)
{
    xEvent eventT = { .u.u.type = 0 };
    EventSwapPtr proc;

    REQUEST(xSendEventReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSendEventReq);
    swapl(&stuff->destination);
    swapl(&stuff->eventMask);

    /* Generic events can have variable size, but SendEvent request holds
       exactly 32B of event data. */
    if (stuff->event.u.u.type == GenericEvent) {
        client->errorValue = stuff->event.u.u.type;
        return BadValue;
    }

    /* Swap event */
    proc = EventSwapVector[stuff->event.u.u.type & 0177];
    if (!proc || proc == NotImplemented)        /* no swapping proc; invalid event type? */
        return BadValue;
    (*proc) (&stuff->event, &eventT);
    stuff->event = eventT;

    return ((*ProcVector[X_SendEvent]) (client));
}

int _X_COLD
SProcGrabPointer(ClientPtr client)
{
    REQUEST(xGrabPointerReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGrabPointerReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->eventMask);
    swapl(&stuff->confineTo);
    swapl(&stuff->cursor);
    swapl(&stuff->time);
    return ((*ProcVector[X_GrabPointer]) (client));
}

int _X_COLD
SProcGrabButton(ClientPtr client)
{
    REQUEST(xGrabButtonReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGrabButtonReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->eventMask);
    swapl(&stuff->confineTo);
    swapl(&stuff->cursor);
    swaps(&stuff->modifiers);
    return ((*ProcVector[X_GrabButton]) (client));
}

int _X_COLD
SProcUngrabButton(ClientPtr client)
{
    REQUEST(xUngrabButtonReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xUngrabButtonReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->modifiers);
    return ((*ProcVector[X_UngrabButton]) (client));
}

int _X_COLD
SProcChangeActivePointerGrab(ClientPtr client)
{
    REQUEST(xChangeActivePointerGrabReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xChangeActivePointerGrabReq);
    swapl(&stuff->cursor);
    swapl(&stuff->time);
    swaps(&stuff->eventMask);
    return ((*ProcVector[X_ChangeActivePointerGrab]) (client));
}

int _X_COLD
SProcGrabKeyboard(ClientPtr client)
{
    REQUEST(xGrabKeyboardReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGrabKeyboardReq);
    swapl(&stuff->grabWindow);
    swapl(&stuff->time);
    return ((*ProcVector[X_GrabKeyboard]) (client));
}

int _X_COLD
SProcGrabKey(ClientPtr client)
{
    REQUEST(xGrabKeyReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGrabKeyReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->modifiers);
    return ((*ProcVector[X_GrabKey]) (client));
}

int _X_COLD
SProcUngrabKey(ClientPtr client)
{
    REQUEST(xUngrabKeyReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xUngrabKeyReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->modifiers);
    return ((*ProcVector[X_UngrabKey]) (client));
}

int _X_COLD
SProcGetMotionEvents(ClientPtr client)
{
    REQUEST(xGetMotionEventsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGetMotionEventsReq);
    swapl(&stuff->window);
    swapl(&stuff->start);
    swapl(&stuff->stop);
    return ((*ProcVector[X_GetMotionEvents]) (client));
}

int _X_COLD
SProcTranslateCoords(ClientPtr client)
{
    REQUEST(xTranslateCoordsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xTranslateCoordsReq);
    swapl(&stuff->srcWid);
    swapl(&stuff->dstWid);
    swaps(&stuff->srcX);
    swaps(&stuff->srcY);
    return ((*ProcVector[X_TranslateCoords]) (client));
}

int _X_COLD
SProcWarpPointer(ClientPtr client)
{
    REQUEST(xWarpPointerReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xWarpPointerReq);
    swapl(&stuff->srcWid);
    swapl(&stuff->dstWid);
    swaps(&stuff->srcX);
    swaps(&stuff->srcY);
    swaps(&stuff->srcWidth);
    swaps(&stuff->srcHeight);
    swaps(&stuff->dstX);
    swaps(&stuff->dstY);
    return ((*ProcVector[X_WarpPointer]) (client));
}

int _X_COLD
SProcSetInputFocus(ClientPtr client)
{
    REQUEST(xSetInputFocusReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSetInputFocusReq);
    swapl(&stuff->focus);
    swapl(&stuff->time);
    return ((*ProcVector[X_SetInputFocus]) (client));
}

int _X_COLD
SProcOpenFont(ClientPtr client)
{
    REQUEST(xOpenFontReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xOpenFontReq);
    swapl(&stuff->fid);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_OpenFont]) (client));
}

int _X_COLD
SProcListFonts(ClientPtr client)
{
    REQUEST(xListFontsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xListFontsReq);
    swaps(&stuff->maxNames);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_ListFonts]) (client));
}

int _X_COLD
SProcListFontsWithInfo(ClientPtr client)
{
    REQUEST(xListFontsWithInfoReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xListFontsWithInfoReq);
    swaps(&stuff->maxNames);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_ListFontsWithInfo]) (client));
}

int _X_COLD
SProcSetFontPath(ClientPtr client)
{
    REQUEST(xSetFontPathReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSetFontPathReq);
    swaps(&stuff->nFonts);
    return ((*ProcVector[X_SetFontPath]) (client));
}

int _X_COLD
SProcCreatePixmap(ClientPtr client)
{
    REQUEST(xCreatePixmapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    swapl(&stuff->pid);
    swapl(&stuff->drawable);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return ((*ProcVector[X_CreatePixmap]) (client));
}

int _X_COLD
SProcCreateGC(ClientPtr client)
{
    REQUEST(xCreateGCReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    swapl(&stuff->gc);
    swapl(&stuff->drawable);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_CreateGC]) (client));
}

int _X_COLD
SProcChangeGC(ClientPtr client)
{
    REQUEST(xChangeGCReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeGCReq);
    swapl(&stuff->gc);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeGC]) (client));
}

int _X_COLD
SProcCopyGC(ClientPtr client)
{
    REQUEST(xCopyGCReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCopyGCReq);
    swapl(&stuff->srcGC);
    swapl(&stuff->dstGC);
    swapl(&stuff->mask);
    return ((*ProcVector[X_CopyGC]) (client));
}

int _X_COLD
SProcSetDashes(ClientPtr client)
{
    REQUEST(xSetDashesReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSetDashesReq);
    swapl(&stuff->gc);
    swaps(&stuff->dashOffset);
    swaps(&stuff->nDashes);
    return ((*ProcVector[X_SetDashes]) (client));

}

int _X_COLD
SProcSetClipRectangles(ClientPtr client)
{
    REQUEST(xSetClipRectanglesReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);
    swapl(&stuff->gc);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    SwapRestS(stuff);
    return ((*ProcVector[X_SetClipRectangles]) (client));
}

int _X_COLD
SProcClearToBackground(ClientPtr client)
{
    REQUEST(xClearAreaReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xClearAreaReq);
    swapl(&stuff->window);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return ((*ProcVector[X_ClearArea]) (client));
}

int _X_COLD
SProcCopyArea(ClientPtr client)
{
    REQUEST(xCopyAreaReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCopyAreaReq);
    swapl(&stuff->srcDrawable);
    swapl(&stuff->dstDrawable);
    swapl(&stuff->gc);
    swaps(&stuff->srcX);
    swaps(&stuff->srcY);
    swaps(&stuff->dstX);
    swaps(&stuff->dstY);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return ((*ProcVector[X_CopyArea]) (client));
}

int _X_COLD
SProcCopyPlane(ClientPtr client)
{
    REQUEST(xCopyPlaneReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCopyPlaneReq);
    swapl(&stuff->srcDrawable);
    swapl(&stuff->dstDrawable);
    swapl(&stuff->gc);
    swaps(&stuff->srcX);
    swaps(&stuff->srcY);
    swaps(&stuff->dstX);
    swaps(&stuff->dstY);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->bitPlane);
    return ((*ProcVector[X_CopyPlane]) (client));
}

/* The following routine is used for all Poly drawing requests
   (except FillPoly, which uses a different request format) */
int _X_COLD
SProcPoly(ClientPtr client)
{
    REQUEST(xPolyPointReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xPolyPointReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    SwapRestS(stuff);
    return ((*ProcVector[stuff->reqType]) (client));
}

/* cannot use SProcPoly for this one, because xFillPolyReq
   is longer than xPolyPointReq, and we don't want to swap
   the difference as shorts! */
int _X_COLD
SProcFillPoly(ClientPtr client)
{
    REQUEST(xFillPolyReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xFillPolyReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    SwapRestS(stuff);
    return ((*ProcVector[X_FillPoly]) (client));
}

int _X_COLD
SProcPutImage(ClientPtr client)
{
    REQUEST(xPutImageReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xPutImageReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swaps(&stuff->dstX);
    swaps(&stuff->dstY);
    /* Image should already be swapped */
    return ((*ProcVector[X_PutImage]) (client));

}

int _X_COLD
SProcGetImage(ClientPtr client)
{
    REQUEST(xGetImageReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGetImageReq);
    swapl(&stuff->drawable);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->planeMask);
    return ((*ProcVector[X_GetImage]) (client));
}

/* ProcPolyText used for both PolyText8 and PolyText16 */

int _X_COLD
SProcPolyText(ClientPtr client)
{
    REQUEST(xPolyTextReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xPolyTextReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return ((*ProcVector[stuff->reqType]) (client));
}

/* ProcImageText used for both ImageText8 and ImageText16 */

int _X_COLD
SProcImageText(ClientPtr client)
{
    REQUEST(xImageTextReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xImageTextReq);
    swapl(&stuff->drawable);
    swapl(&stuff->gc);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return ((*ProcVector[stuff->reqType]) (client));
}

int _X_COLD
SProcCreateColormap(ClientPtr client)
{
    REQUEST(xCreateColormapReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCreateColormapReq);
    swapl(&stuff->mid);
    swapl(&stuff->window);
    swapl(&stuff->visual);
    return ((*ProcVector[X_CreateColormap]) (client));
}

int _X_COLD
SProcCopyColormapAndFree(ClientPtr client)
{
    REQUEST(xCopyColormapAndFreeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);
    swapl(&stuff->mid);
    swapl(&stuff->srcCmap);
    return ((*ProcVector[X_CopyColormapAndFree]) (client));

}

int _X_COLD
SProcAllocColor(ClientPtr client)
{
    REQUEST(xAllocColorReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xAllocColorReq);
    swapl(&stuff->cmap);
    swaps(&stuff->red);
    swaps(&stuff->green);
    swaps(&stuff->blue);
    return ((*ProcVector[X_AllocColor]) (client));
}

int _X_COLD
SProcAllocNamedColor(ClientPtr client)
{
    REQUEST(xAllocNamedColorReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xAllocNamedColorReq);
    swapl(&stuff->cmap);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_AllocNamedColor]) (client));
}

int _X_COLD
SProcAllocColorCells(ClientPtr client)
{
    REQUEST(xAllocColorCellsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    swapl(&stuff->cmap);
    swaps(&stuff->colors);
    swaps(&stuff->planes);
    return ((*ProcVector[X_AllocColorCells]) (client));
}

int _X_COLD
SProcAllocColorPlanes(ClientPtr client)
{
    REQUEST(xAllocColorPlanesReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);
    swapl(&stuff->cmap);
    swaps(&stuff->colors);
    swaps(&stuff->red);
    swaps(&stuff->green);
    swaps(&stuff->blue);
    return ((*ProcVector[X_AllocColorPlanes]) (client));
}

int _X_COLD
SProcFreeColors(ClientPtr client)
{
    REQUEST(xFreeColorsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    swapl(&stuff->cmap);
    swapl(&stuff->planeMask);
    SwapRestL(stuff);
    return ((*ProcVector[X_FreeColors]) (client));

}

void _X_COLD
SwapColorItem(xColorItem * pItem)
{
    swapl(&pItem->pixel);
    swaps(&pItem->red);
    swaps(&pItem->green);
    swaps(&pItem->blue);
}

int _X_COLD
SProcStoreColors(ClientPtr client)
{
    long count;
    xColorItem *pItem;

    REQUEST(xStoreColorsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    swapl(&stuff->cmap);
    pItem = (xColorItem *) &stuff[1];
    for (count = LengthRestB(stuff) / sizeof(xColorItem); --count >= 0;)
        SwapColorItem(pItem++);
    return ((*ProcVector[X_StoreColors]) (client));
}

int _X_COLD
SProcStoreNamedColor(ClientPtr client)
{
    REQUEST(xStoreNamedColorReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xStoreNamedColorReq);
    swapl(&stuff->cmap);
    swapl(&stuff->pixel);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_StoreNamedColor]) (client));
}

int _X_COLD
SProcQueryColors(ClientPtr client)
{
    REQUEST(xQueryColorsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xQueryColorsReq);
    swapl(&stuff->cmap);
    SwapRestL(stuff);
    return ((*ProcVector[X_QueryColors]) (client));
}

int _X_COLD
SProcLookupColor(ClientPtr client)
{
    REQUEST(xLookupColorReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xLookupColorReq);
    swapl(&stuff->cmap);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_LookupColor]) (client));
}

int _X_COLD
SProcCreateCursor(ClientPtr client)
{
    REQUEST(xCreateCursorReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCreateCursorReq);
    swapl(&stuff->cid);
    swapl(&stuff->source);
    swapl(&stuff->mask);
    swaps(&stuff->foreRed);
    swaps(&stuff->foreGreen);
    swaps(&stuff->foreBlue);
    swaps(&stuff->backRed);
    swaps(&stuff->backGreen);
    swaps(&stuff->backBlue);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return ((*ProcVector[X_CreateCursor]) (client));
}

int _X_COLD
SProcCreateGlyphCursor(ClientPtr client)
{
    REQUEST(xCreateGlyphCursorReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCreateGlyphCursorReq);
    swapl(&stuff->cid);
    swapl(&stuff->source);
    swapl(&stuff->mask);
    swaps(&stuff->sourceChar);
    swaps(&stuff->maskChar);
    swaps(&stuff->foreRed);
    swaps(&stuff->foreGreen);
    swaps(&stuff->foreBlue);
    swaps(&stuff->backRed);
    swaps(&stuff->backGreen);
    swaps(&stuff->backBlue);
    return ((*ProcVector[X_CreateGlyphCursor]) (client));
}

int _X_COLD
SProcRecolorCursor(ClientPtr client)
{
    REQUEST(xRecolorCursorReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecolorCursorReq);
    swapl(&stuff->cursor);
    swaps(&stuff->foreRed);
    swaps(&stuff->foreGreen);
    swaps(&stuff->foreBlue);
    swaps(&stuff->backRed);
    swaps(&stuff->backGreen);
    swaps(&stuff->backBlue);
    return ((*ProcVector[X_RecolorCursor]) (client));
}

int _X_COLD
SProcQueryBestSize(ClientPtr client)
{
    REQUEST(xQueryBestSizeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xQueryBestSizeReq);
    swapl(&stuff->drawable);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return ((*ProcVector[X_QueryBestSize]) (client));

}

int _X_COLD
SProcQueryExtension(ClientPtr client)
{
    REQUEST(xQueryExtensionReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xQueryExtensionReq);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_QueryExtension]) (client));
}

int _X_COLD
SProcChangeKeyboardMapping(ClientPtr client)
{
    REQUEST(xChangeKeyboardMappingReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeKeyboardMappingReq);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeKeyboardMapping]) (client));
}

int _X_COLD
SProcChangeKeyboardControl(ClientPtr client)
{
    REQUEST(xChangeKeyboardControlReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeKeyboardControlReq);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeKeyboardControl]) (client));
}

int _X_COLD
SProcChangePointerControl(ClientPtr client)
{
    REQUEST(xChangePointerControlReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xChangePointerControlReq);
    swaps(&stuff->accelNum);
    swaps(&stuff->accelDenum);
    swaps(&stuff->threshold);
    return ((*ProcVector[X_ChangePointerControl]) (client));
}

int _X_COLD
SProcSetScreenSaver(ClientPtr client)
{
    REQUEST(xSetScreenSaverReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSetScreenSaverReq);
    swaps(&stuff->timeout);
    swaps(&stuff->interval);
    return ((*ProcVector[X_SetScreenSaver]) (client));
}

int _X_COLD
SProcChangeHosts(ClientPtr client)
{
    REQUEST(xChangeHostsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeHostsReq);
    swaps(&stuff->hostLength);
    return ((*ProcVector[X_ChangeHosts]) (client));

}

int _X_COLD
SProcRotateProperties(ClientPtr client)
{
    REQUEST(xRotatePropertiesReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xRotatePropertiesReq);
    swapl(&stuff->window);
    swaps(&stuff->nAtoms);
    swaps(&stuff->nPositions);
    SwapRestL(stuff);
    return ((*ProcVector[X_RotateProperties]) (client));
}

int _X_COLD
SProcNoOperation(ClientPtr client)
{
    REQUEST(xReq);
    swaps(&stuff->length);
    return ((*ProcVector[X_NoOperation]) (client));
}

void _X_COLD
SwapConnClientPrefix(xConnClientPrefix * pCCP)
{
    swaps(&pCCP->majorVersion);
    swaps(&pCCP->minorVersion);
    swaps(&pCCP->nbytesAuthProto);
    swaps(&pCCP->nbytesAuthString);
}
