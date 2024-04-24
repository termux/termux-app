/***********************************************************

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

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "resource.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "region.h"
#include "dixstruct.h"

#include "privates.h"
#include "dix.h"
#include "xace.h"
#include <assert.h>

extern FontPtr defaultFont;

static Bool CreateDefaultTile(GCPtr pGC);

static unsigned char DefaultDash[2] = { 4, 4 };

void
ValidateGC(DrawablePtr pDraw, GC * pGC)
{
    (*pGC->funcs->ValidateGC) (pGC, pGC->stateChanges, pDraw);
    pGC->stateChanges = 0;
    pGC->serialNumber = pDraw->serialNumber;
}

/*
 * ChangeGC/ChangeGCXIDs:
 *
 * The client performing the gc change must be passed so that access
 * checks can be performed on any tiles, stipples, or fonts that are
 * specified.  ddxen can call this too; they should normally pass
 * NullClient for the client since any access checking should have
 * already been done at a higher level.
 *
 * If you have any XIDs, you must use ChangeGCXIDs:
 *
 *     CARD32 v[2];
 *     v[0] = FillTiled;
 *     v[1] = pid;
 *     ChangeGCXIDs(client, pGC, GCFillStyle|GCTile, v);
 *
 * However, if you need to pass a pointer to a pixmap or font, you must
 * use ChangeGC:
 *
 *     ChangeGCVal v[2];
 *     v[0].val = FillTiled;
 *     v[1].ptr = pPixmap;
 *     ChangeGC(client, pGC, GCFillStyle|GCTile, v);
 *
 * If you have neither XIDs nor pointers, you can use either function,
 * but ChangeGC will do less work.
 *
 *     ChangeGCVal v[2];
 *     v[0].val = foreground;
 *     v[1].val = background;
 *     ChangeGC(client, pGC, GCForeground|GCBackground, v);
 */

#define NEXTVAL(_type, _var) { \
	_var = (_type)(pUnion->val); pUnion++; \
    }

#define NEXT_PTR(_type, _var) { \
    _var = (_type)pUnion->ptr; pUnion++; }

int
ChangeGC(ClientPtr client, GC * pGC, BITS32 mask, ChangeGCValPtr pUnion)
{
    BITS32 index2;
    int error = 0;
    PixmapPtr pPixmap;
    BITS32 maskQ;

    assert(pUnion);
    pGC->serialNumber |= GC_CHANGE_SERIAL_BIT;

    maskQ = mask;               /* save these for when we walk the GCque */
    while (mask && !error) {
        index2 = (BITS32) lowbit(mask);
        mask &= ~index2;
        pGC->stateChanges |= index2;
        switch (index2) {
        case GCFunction:
        {
            CARD8 newalu;
            NEXTVAL(CARD8, newalu);

            if (newalu <= GXset)
                pGC->alu = newalu;
            else {
                if (client)
                    client->errorValue = newalu;
                error = BadValue;
            }
            break;
        }
        case GCPlaneMask:
            NEXTVAL(unsigned long, pGC->planemask);

            break;
        case GCForeground:
            NEXTVAL(unsigned long, pGC->fgPixel);

            /*
             * this is for CreateGC
             */
            if (!pGC->tileIsPixel && !pGC->tile.pixmap) {
                pGC->tileIsPixel = TRUE;
                pGC->tile.pixel = pGC->fgPixel;
            }
            break;
        case GCBackground:
            NEXTVAL(unsigned long, pGC->bgPixel);

            break;
        case GCLineWidth:      /* ??? line width is a CARD16 */
            NEXTVAL(CARD16, pGC->lineWidth);

            break;
        case GCLineStyle:
        {
            unsigned int newlinestyle;
            NEXTVAL(unsigned int, newlinestyle);

            if (newlinestyle <= LineDoubleDash)
                pGC->lineStyle = newlinestyle;
            else {
                if (client)
                    client->errorValue = newlinestyle;
                error = BadValue;
            }
            break;
        }
        case GCCapStyle:
        {
            unsigned int newcapstyle;
            NEXTVAL(unsigned int, newcapstyle);

            if (newcapstyle <= CapProjecting)
                pGC->capStyle = newcapstyle;
            else {
                if (client)
                    client->errorValue = newcapstyle;
                error = BadValue;
            }
            break;
        }
        case GCJoinStyle:
        {
            unsigned int newjoinstyle;
            NEXTVAL(unsigned int, newjoinstyle);

            if (newjoinstyle <= JoinBevel)
                pGC->joinStyle = newjoinstyle;
            else {
                if (client)
                    client->errorValue = newjoinstyle;
                error = BadValue;
            }
            break;
        }
        case GCFillStyle:
        {
            unsigned int newfillstyle;
            NEXTVAL(unsigned int, newfillstyle);

            if (newfillstyle <= FillOpaqueStippled)
                pGC->fillStyle = newfillstyle;
            else {
                if (client)
                    client->errorValue = newfillstyle;
                error = BadValue;
            }
            break;
        }
        case GCFillRule:
        {
            unsigned int newfillrule;
            NEXTVAL(unsigned int, newfillrule);

            if (newfillrule <= WindingRule)
                pGC->fillRule = newfillrule;
            else {
                if (client)
                    client->errorValue = newfillrule;
                error = BadValue;
            }
            break;
        }
        case GCTile:
            NEXT_PTR(PixmapPtr, pPixmap);

            if ((pPixmap->drawable.depth != pGC->depth) ||
                (pPixmap->drawable.pScreen != pGC->pScreen)) {
                error = BadMatch;
            }
            else {
                pPixmap->refcnt++;
                if (!pGC->tileIsPixel)
                    (*pGC->pScreen->DestroyPixmap) (pGC->tile.pixmap);
                pGC->tileIsPixel = FALSE;
                pGC->tile.pixmap = pPixmap;
            }
            break;
        case GCStipple:
            NEXT_PTR(PixmapPtr, pPixmap);

            if (pPixmap && ((pPixmap->drawable.depth != 1) ||
                            (pPixmap->drawable.pScreen != pGC->pScreen)))
            {
                error = BadMatch;
            }
            else {
                if (pPixmap)
                    pPixmap->refcnt++;
                if (pGC->stipple)
                    (*pGC->pScreen->DestroyPixmap) (pGC->stipple);
                pGC->stipple = pPixmap;
            }
            break;
        case GCTileStipXOrigin:
            NEXTVAL(INT16, pGC->patOrg.x);

            break;
        case GCTileStipYOrigin:
            NEXTVAL(INT16, pGC->patOrg.y);

            break;
        case GCFont:
        {
            FontPtr pFont;
            NEXT_PTR(FontPtr, pFont);

            pFont->refcnt++;
            if (pGC->font)
                CloseFont(pGC->font, (Font) 0);
            pGC->font = pFont;
            break;
        }
        case GCSubwindowMode:
        {
            unsigned int newclipmode;
            NEXTVAL(unsigned int, newclipmode);

            if (newclipmode <= IncludeInferiors)
                pGC->subWindowMode = newclipmode;
            else {
                if (client)
                    client->errorValue = newclipmode;
                error = BadValue;
            }
            break;
        }
        case GCGraphicsExposures:
        {
            unsigned int newge;
            NEXTVAL(unsigned int, newge);

            if (newge <= xTrue)
                pGC->graphicsExposures = newge;
            else {
                if (client)
                    client->errorValue = newge;
                error = BadValue;
            }
            break;
        }
        case GCClipXOrigin:
            NEXTVAL(INT16, pGC->clipOrg.x);

            break;
        case GCClipYOrigin:
            NEXTVAL(INT16, pGC->clipOrg.y);

            break;
        case GCClipMask:
            NEXT_PTR(PixmapPtr, pPixmap);

            if (pPixmap) {
                if ((pPixmap->drawable.depth != 1) ||
                    (pPixmap->drawable.pScreen != pGC->pScreen)) {
                    error = BadMatch;
                    break;
                }
                pPixmap->refcnt++;
            }
            (*pGC->funcs->ChangeClip) (pGC, pPixmap ? CT_PIXMAP : CT_NONE,
                                       (void *) pPixmap, 0);
            break;
        case GCDashOffset:
            NEXTVAL(INT16, pGC->dashOffset);

            break;
        case GCDashList:
        {
            CARD8 newdash;
            NEXTVAL(CARD8, newdash);

            if (newdash == 4) {
                if (pGC->dash != DefaultDash) {
                    free(pGC->dash);
                    pGC->numInDashList = 2;
                    pGC->dash = DefaultDash;
                }
            }
            else if (newdash != 0) {
                unsigned char *dash;

                dash = malloc(2 * sizeof(unsigned char));
                if (dash) {
                    if (pGC->dash != DefaultDash)
                        free(pGC->dash);
                    pGC->numInDashList = 2;
                    pGC->dash = dash;
                    dash[0] = newdash;
                    dash[1] = newdash;
                }
                else
                    error = BadAlloc;
            }
            else {
                if (client)
                    client->errorValue = newdash;
                error = BadValue;
            }
            break;
        }
        case GCArcMode:
        {
            unsigned int newarcmode;
            NEXTVAL(unsigned int, newarcmode);

            if (newarcmode <= ArcPieSlice)
                pGC->arcMode = newarcmode;
            else {
                if (client)
                    client->errorValue = newarcmode;
                error = BadValue;
            }
            break;
        }
        default:
            if (client)
                client->errorValue = maskQ;
            error = BadValue;
            break;
        }
    }                           /* end while mask && !error */

    if (pGC->fillStyle == FillTiled && pGC->tileIsPixel) {
        if (!CreateDefaultTile(pGC)) {
            pGC->fillStyle = FillSolid;
            error = BadAlloc;
        }
    }
    (*pGC->funcs->ChangeGC) (pGC, maskQ);
    return error;
}

#undef NEXTVAL
#undef NEXT_PTR

static const struct {
    BITS32 mask;
    RESTYPE type;
    Mask access_mode;
} xidfields[] = {
    {GCTile, RT_PIXMAP, DixReadAccess},
    {GCStipple, RT_PIXMAP, DixReadAccess},
    {GCFont, RT_FONT, DixUseAccess},
    {GCClipMask, RT_PIXMAP, DixReadAccess},
};

int
ChangeGCXIDs(ClientPtr client, GC * pGC, BITS32 mask, CARD32 *pC32)
{
    ChangeGCVal vals[GCLastBit + 1];
    int i;

    if (mask & ~GCAllBits) {
        client->errorValue = mask;
        return BadValue;
    }
    for (i = Ones(mask); i--;)
        vals[i].val = pC32[i];
    for (i = 0; i < ARRAY_SIZE(xidfields); ++i) {
        int offset, rc;

        if (!(mask & xidfields[i].mask))
            continue;
        offset = Ones(mask & (xidfields[i].mask - 1));
        if (xidfields[i].mask == GCClipMask && vals[offset].val == None) {
            vals[offset].ptr = NullPixmap;
            continue;
        }
        rc = dixLookupResourceByType(&vals[offset].ptr, vals[offset].val,
                                     xidfields[i].type, client,
                                     xidfields[i].access_mode);
        if (rc != Success) {
            client->errorValue = vals[offset].val;
            return rc;
        }
    }
    return ChangeGC(client, pGC, mask, vals);
}

static GCPtr
NewGCObject(ScreenPtr pScreen, int depth)
{
    GCPtr pGC;

    pGC = dixAllocateScreenObjectWithPrivates(pScreen, GC, PRIVATE_GC);
    if (!pGC) {
        return (GCPtr) NULL;
    }

    pGC->pScreen = pScreen;
    pGC->depth = depth;
    pGC->alu = GXcopy;          /* dst <- src */
    pGC->planemask = ~0;
    pGC->serialNumber = 0;
    pGC->funcs = 0;
    pGC->fgPixel = 0;
    pGC->bgPixel = 1;
    pGC->lineWidth = 0;
    pGC->lineStyle = LineSolid;
    pGC->capStyle = CapButt;
    pGC->joinStyle = JoinMiter;
    pGC->fillStyle = FillSolid;
    pGC->fillRule = EvenOddRule;
    pGC->arcMode = ArcPieSlice;
    pGC->tile.pixel = 0;
    pGC->tile.pixmap = NullPixmap;

    pGC->tileIsPixel = TRUE;
    pGC->patOrg.x = 0;
    pGC->patOrg.y = 0;
    pGC->subWindowMode = ClipByChildren;
    pGC->graphicsExposures = TRUE;
    pGC->clipOrg.x = 0;
    pGC->clipOrg.y = 0;
    pGC->clientClip = (void *) NULL;
    pGC->numInDashList = 2;
    pGC->dash = DefaultDash;
    pGC->dashOffset = 0;

    /* use the default font and stipple */
    pGC->font = defaultFont;
    if (pGC->font)              /* necessary, because open of default font could fail */
        pGC->font->refcnt++;
    pGC->stipple = pGC->pScreen->defaultStipple;
    if (pGC->stipple)
        pGC->stipple->refcnt++;

    /* this is not a scratch GC */
    pGC->scratch_inuse = FALSE;
    return pGC;
}

/* CreateGC(pDrawable, mask, pval, pStatus)
   creates a default GC for the given drawable, using mask to fill
   in any non-default values.
   Returns a pointer to the new GC on success, NULL otherwise.
   returns status of non-default fields in pStatus
BUG:
   should check for failure to create default tile

*/
GCPtr
CreateGC(DrawablePtr pDrawable, BITS32 mask, XID *pval, int *pStatus,
         XID gcid, ClientPtr client)
{
    GCPtr pGC;

    pGC = NewGCObject(pDrawable->pScreen, pDrawable->depth);
    if (!pGC) {
        *pStatus = BadAlloc;
        return (GCPtr) NULL;
    }

    pGC->serialNumber = GC_CHANGE_SERIAL_BIT;
    if (mask & GCForeground) {
        /*
         * magic special case -- ChangeGC checks for this condition
         * and snags the Foreground value to create a pseudo default-tile
         */
        pGC->tileIsPixel = FALSE;
    }
    else {
        pGC->tileIsPixel = TRUE;
    }

    /* security creation/labeling check */
    *pStatus = XaceHook(XACE_RESOURCE_ACCESS, client, gcid, RT_GC, pGC,
                        RT_NONE, NULL, DixCreateAccess | DixSetAttrAccess);
    if (*pStatus != Success)
        goto out;

    pGC->stateChanges = GCAllBits;
    if (!(*pGC->pScreen->CreateGC) (pGC))
        *pStatus = BadAlloc;
    else if (mask)
        *pStatus = ChangeGCXIDs(client, pGC, mask, pval);
    else
        *pStatus = Success;

 out:
    if (*pStatus != Success) {
        if (!pGC->tileIsPixel && !pGC->tile.pixmap)
            pGC->tileIsPixel = TRUE;    /* undo special case */
        FreeGC(pGC, (XID) 0);
        pGC = (GCPtr) NULL;
    }

    return pGC;
}

static Bool
CreateDefaultTile(GCPtr pGC)
{
    ChangeGCVal tmpval[3];
    PixmapPtr pTile;
    GCPtr pgcScratch;
    xRectangle rect;
    CARD16 w, h;

    w = 1;
    h = 1;
    (*pGC->pScreen->QueryBestSize) (TileShape, &w, &h, pGC->pScreen);
    pTile = (PixmapPtr)
        (*pGC->pScreen->CreatePixmap) (pGC->pScreen, w, h, pGC->depth, 0);
    pgcScratch = GetScratchGC(pGC->depth, pGC->pScreen);
    if (!pTile || !pgcScratch) {
        if (pTile)
            (*pTile->drawable.pScreen->DestroyPixmap) (pTile);
        if (pgcScratch)
            FreeScratchGC(pgcScratch);
        return FALSE;
    }
    tmpval[0].val = GXcopy;
    tmpval[1].val = pGC->tile.pixel;
    tmpval[2].val = FillSolid;
    (void) ChangeGC(NullClient, pgcScratch,
                    GCFunction | GCForeground | GCFillStyle, tmpval);
    ValidateGC((DrawablePtr) pTile, pgcScratch);
    rect.x = 0;
    rect.y = 0;
    rect.width = w;
    rect.height = h;
    (*pgcScratch->ops->PolyFillRect) ((DrawablePtr) pTile, pgcScratch, 1,
                                      &rect);
    /* Always remember to free the scratch graphics context after use. */
    FreeScratchGC(pgcScratch);

    pGC->tileIsPixel = FALSE;
    pGC->tile.pixmap = pTile;
    return TRUE;
}

int
CopyGC(GC * pgcSrc, GC * pgcDst, BITS32 mask)
{
    BITS32 index2;
    BITS32 maskQ;
    int error = 0;

    if (pgcSrc == pgcDst)
        return Success;
    pgcDst->serialNumber |= GC_CHANGE_SERIAL_BIT;
    pgcDst->stateChanges |= mask;
    maskQ = mask;
    while (mask) {
        index2 = (BITS32) lowbit(mask);
        mask &= ~index2;
        switch (index2) {
        case GCFunction:
            pgcDst->alu = pgcSrc->alu;
            break;
        case GCPlaneMask:
            pgcDst->planemask = pgcSrc->planemask;
            break;
        case GCForeground:
            pgcDst->fgPixel = pgcSrc->fgPixel;
            break;
        case GCBackground:
            pgcDst->bgPixel = pgcSrc->bgPixel;
            break;
        case GCLineWidth:
            pgcDst->lineWidth = pgcSrc->lineWidth;
            break;
        case GCLineStyle:
            pgcDst->lineStyle = pgcSrc->lineStyle;
            break;
        case GCCapStyle:
            pgcDst->capStyle = pgcSrc->capStyle;
            break;
        case GCJoinStyle:
            pgcDst->joinStyle = pgcSrc->joinStyle;
            break;
        case GCFillStyle:
            pgcDst->fillStyle = pgcSrc->fillStyle;
            break;
        case GCFillRule:
            pgcDst->fillRule = pgcSrc->fillRule;
            break;
        case GCTile:
        {
            if (EqualPixUnion(pgcDst->tileIsPixel,
                              pgcDst->tile,
                              pgcSrc->tileIsPixel, pgcSrc->tile)) {
                break;
            }
            if (!pgcDst->tileIsPixel)
                (*pgcDst->pScreen->DestroyPixmap) (pgcDst->tile.pixmap);
            pgcDst->tileIsPixel = pgcSrc->tileIsPixel;
            pgcDst->tile = pgcSrc->tile;
            if (!pgcDst->tileIsPixel)
                pgcDst->tile.pixmap->refcnt++;
            break;
        }
        case GCStipple:
        {
            if (pgcDst->stipple == pgcSrc->stipple)
                break;
            if (pgcDst->stipple)
                (*pgcDst->pScreen->DestroyPixmap) (pgcDst->stipple);
            pgcDst->stipple = pgcSrc->stipple;
            if (pgcDst->stipple)
                pgcDst->stipple->refcnt++;
            break;
        }
        case GCTileStipXOrigin:
            pgcDst->patOrg.x = pgcSrc->patOrg.x;
            break;
        case GCTileStipYOrigin:
            pgcDst->patOrg.y = pgcSrc->patOrg.y;
            break;
        case GCFont:
            if (pgcDst->font == pgcSrc->font)
                break;
            if (pgcDst->font)
                CloseFont(pgcDst->font, (Font) 0);
            if ((pgcDst->font = pgcSrc->font) != NullFont)
                (pgcDst->font)->refcnt++;
            break;
        case GCSubwindowMode:
            pgcDst->subWindowMode = pgcSrc->subWindowMode;
            break;
        case GCGraphicsExposures:
            pgcDst->graphicsExposures = pgcSrc->graphicsExposures;
            break;
        case GCClipXOrigin:
            pgcDst->clipOrg.x = pgcSrc->clipOrg.x;
            break;
        case GCClipYOrigin:
            pgcDst->clipOrg.y = pgcSrc->clipOrg.y;
            break;
        case GCClipMask:
            (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
            break;
        case GCDashOffset:
            pgcDst->dashOffset = pgcSrc->dashOffset;
            break;
        case GCDashList:
            if (pgcSrc->dash == DefaultDash) {
                if (pgcDst->dash != DefaultDash) {
                    free(pgcDst->dash);
                    pgcDst->numInDashList = pgcSrc->numInDashList;
                    pgcDst->dash = pgcSrc->dash;
                }
            }
            else {
                unsigned char *dash;
                unsigned int i;

                dash = malloc(pgcSrc->numInDashList * sizeof(unsigned char));
                if (dash) {
                    if (pgcDst->dash != DefaultDash)
                        free(pgcDst->dash);
                    pgcDst->numInDashList = pgcSrc->numInDashList;
                    pgcDst->dash = dash;
                    for (i = 0; i < pgcSrc->numInDashList; i++)
                        dash[i] = pgcSrc->dash[i];
                }
                else
                    error = BadAlloc;
            }
            break;
        case GCArcMode:
            pgcDst->arcMode = pgcSrc->arcMode;
            break;
        default:
            FatalError("CopyGC: Unhandled mask!\n");
        }
    }
    if (pgcDst->fillStyle == FillTiled && pgcDst->tileIsPixel) {
        if (!CreateDefaultTile(pgcDst)) {
            pgcDst->fillStyle = FillSolid;
            error = BadAlloc;
        }
    }
    (*pgcDst->funcs->CopyGC) (pgcSrc, maskQ, pgcDst);
    return error;
}

/**
 * does the diX part of freeing the characteristics in the GC.
 *
 *  \param value  must conform to DeleteType
 */
int
FreeGC(void *value, XID gid)
{
    GCPtr pGC = (GCPtr) value;

    CloseFont(pGC->font, (Font) 0);
    (*pGC->funcs->DestroyClip) (pGC);

    if (!pGC->tileIsPixel)
        (*pGC->pScreen->DestroyPixmap) (pGC->tile.pixmap);
    if (pGC->stipple)
        (*pGC->pScreen->DestroyPixmap) (pGC->stipple);

    (*pGC->funcs->DestroyGC) (pGC);
    if (pGC->dash != DefaultDash)
        free(pGC->dash);
    dixFreeObjectWithPrivates(pGC, PRIVATE_GC);
    return Success;
}

/* CreateScratchGC(pScreen, depth)
    like CreateGC, but doesn't do the default tile or stipple,
since we can't create them without already having a GC.  any code
using the tile or stipple has to set them explicitly anyway,
since the state of the scratch gc is unknown.  This is OK
because ChangeGC() has to be able to deal with NULL tiles and
stipples anyway (in case the CreateGC() call has provided a
value for them -- we can't set the default tile until the
client-supplied attributes are installed, since the fgPixel
is what fills the default tile.  (maybe this comment should
go with CreateGC() or ChangeGC().)
*/

static GCPtr
CreateScratchGC(ScreenPtr pScreen, unsigned depth)
{
    GCPtr pGC;

    pGC = NewGCObject(pScreen, depth);
    if (!pGC)
        return (GCPtr) NULL;

    pGC->stateChanges = GCAllBits;
    if (!(*pScreen->CreateGC) (pGC)) {
        FreeGC(pGC, (XID) 0);
        pGC = (GCPtr) NULL;
    }
    pGC->graphicsExposures = FALSE;
    return pGC;
}

void
FreeGCperDepth(int screenNum)
{
    int i;
    ScreenPtr pScreen;
    GCPtr *ppGC;

    pScreen = screenInfo.screens[screenNum];
    ppGC = pScreen->GCperDepth;

    for (i = 0; i <= pScreen->numDepths; i++) {
        (void) FreeGC(ppGC[i], (XID) 0);
        ppGC[i] = NULL;
    }
}

Bool
CreateGCperDepth(int screenNum)
{
    int i;
    ScreenPtr pScreen;
    DepthPtr pDepth;
    GCPtr *ppGC;

    pScreen = screenInfo.screens[screenNum];
    ppGC = pScreen->GCperDepth;
    /* do depth 1 separately because it's not included in list */
    if (!(ppGC[0] = CreateScratchGC(pScreen, 1)))
        return FALSE;
    /* Make sure we don't overflow GCperDepth[] */
    if (pScreen->numDepths > MAXFORMATS)
        return FALSE;

    pDepth = pScreen->allowedDepths;
    for (i = 0; i < pScreen->numDepths; i++, pDepth++) {
        if (!(ppGC[i + 1] = CreateScratchGC(pScreen, pDepth->depth))) {
            for (; i >= 0; i--)
                (void) FreeGC(ppGC[i], (XID) 0);
            return FALSE;
        }
    }
    return TRUE;
}

Bool
CreateDefaultStipple(int screenNum)
{
    ScreenPtr pScreen;
    ChangeGCVal tmpval[3];
    xRectangle rect;
    CARD16 w, h;
    GCPtr pgcScratch;

    pScreen = screenInfo.screens[screenNum];

    w = 16;
    h = 16;
    (*pScreen->QueryBestSize) (StippleShape, &w, &h, pScreen);
    if (!(pScreen->defaultStipple = pScreen->CreatePixmap(pScreen, w, h, 1, 0)))
        return FALSE;
    /* fill stipple with 1 */
    tmpval[0].val = GXcopy;
    tmpval[1].val = 1;
    tmpval[2].val = FillSolid;
    pgcScratch = GetScratchGC(1, pScreen);
    if (!pgcScratch) {
        (*pScreen->DestroyPixmap) (pScreen->defaultStipple);
        return FALSE;
    }
    (void) ChangeGC(NullClient, pgcScratch,
                    GCFunction | GCForeground | GCFillStyle, tmpval);
    ValidateGC((DrawablePtr) pScreen->defaultStipple, pgcScratch);
    rect.x = 0;
    rect.y = 0;
    rect.width = w;
    rect.height = h;
    (*pgcScratch->ops->PolyFillRect) ((DrawablePtr) pScreen->defaultStipple,
                                      pgcScratch, 1, &rect);
    FreeScratchGC(pgcScratch);
    return TRUE;
}

void
FreeDefaultStipple(int screenNum)
{
    ScreenPtr pScreen = screenInfo.screens[screenNum];

    (*pScreen->DestroyPixmap) (pScreen->defaultStipple);
}

int
SetDashes(GCPtr pGC, unsigned offset, unsigned ndash, unsigned char *pdash)
{
    long i;
    unsigned char *p, *indash;
    BITS32 maskQ = 0;

    i = ndash;
    p = pdash;
    while (i--) {
        if (!*p++) {
            /* dash segment must be > 0 */
            return BadValue;
        }
    }

    if (ndash & 1)
        p = malloc(2 * ndash * sizeof(unsigned char));
    else
        p = malloc(ndash * sizeof(unsigned char));
    if (!p)
        return BadAlloc;

    pGC->serialNumber |= GC_CHANGE_SERIAL_BIT;
    if (offset != pGC->dashOffset) {
        pGC->dashOffset = offset;
        pGC->stateChanges |= GCDashOffset;
        maskQ |= GCDashOffset;
    }

    if (pGC->dash != DefaultDash)
        free(pGC->dash);
    pGC->numInDashList = ndash;
    pGC->dash = p;
    if (ndash & 1) {
        pGC->numInDashList += ndash;
        indash = pdash;
        i = ndash;
        while (i--)
            *p++ = *indash++;
    }
    while (ndash--)
        *p++ = *pdash++;
    pGC->stateChanges |= GCDashList;
    maskQ |= GCDashList;

    if (pGC->funcs->ChangeGC)
        (*pGC->funcs->ChangeGC) (pGC, maskQ);
    return Success;
}

int
VerifyRectOrder(int nrects, xRectangle *prects, int ordering)
{
    xRectangle *prectP, *prectN;
    int i;

    switch (ordering) {
    case Unsorted:
        return CT_UNSORTED;
    case YSorted:
        if (nrects > 1) {
            for (i = 1, prectP = prects, prectN = prects + 1;
                 i < nrects; i++, prectP++, prectN++)
                if (prectN->y < prectP->y)
                    return -1;
        }
        return CT_YSORTED;
    case YXSorted:
        if (nrects > 1) {
            for (i = 1, prectP = prects, prectN = prects + 1;
                 i < nrects; i++, prectP++, prectN++)
                if ((prectN->y < prectP->y) ||
                    ((prectN->y == prectP->y) && (prectN->x < prectP->x)))
                    return -1;
        }
        return CT_YXSORTED;
    case YXBanded:
        if (nrects > 1) {
            for (i = 1, prectP = prects, prectN = prects + 1;
                 i < nrects; i++, prectP++, prectN++)
                if ((prectN->y != prectP->y &&
                     prectN->y < prectP->y + (int) prectP->height) ||
                    ((prectN->y == prectP->y) &&
                     (prectN->height != prectP->height ||
                      prectN->x < prectP->x + (int) prectP->width)))
                    return -1;
        }
        return CT_YXBANDED;
    }
    return -1;
}

int
SetClipRects(GCPtr pGC, int xOrigin, int yOrigin, int nrects,
             xRectangle *prects, int ordering)
{
    int newct, size;
    xRectangle *prectsNew;

    newct = VerifyRectOrder(nrects, prects, ordering);
    if (newct < 0)
        return BadMatch;
    size = nrects * sizeof(xRectangle);
    prectsNew = malloc(size);
    if (!prectsNew && size)
        return BadAlloc;

    pGC->serialNumber |= GC_CHANGE_SERIAL_BIT;
    pGC->clipOrg.x = xOrigin;
    pGC->stateChanges |= GCClipXOrigin;

    pGC->clipOrg.y = yOrigin;
    pGC->stateChanges |= GCClipYOrigin;

    if (size)
        memmove((char *) prectsNew, (char *) prects, size);
    (*pGC->funcs->ChangeClip) (pGC, newct, (void *) prectsNew, nrects);
    if (pGC->funcs->ChangeGC)
        (*pGC->funcs->ChangeGC) (pGC,
                                 GCClipXOrigin | GCClipYOrigin | GCClipMask);
    return Success;
}

/*
   sets reasonable defaults
   if we can get a pre-allocated one, use it and mark it as used.
   if we can't, create one out of whole cloth (The Velveteen GC -- if
   you use it often enough it will become real.)
*/
GCPtr
GetScratchGC(unsigned depth, ScreenPtr pScreen)
{
    int i;
    GCPtr pGC;

    for (i = 0; i <= pScreen->numDepths; i++) {
        pGC = pScreen->GCperDepth[i];
        if (pGC && pGC->depth == depth && !pGC->scratch_inuse) {
            pGC->scratch_inuse = TRUE;

            pGC->alu = GXcopy;
            pGC->planemask = ~0;
            pGC->serialNumber = 0;
            pGC->fgPixel = 0;
            pGC->bgPixel = 1;
            pGC->lineWidth = 0;
            pGC->lineStyle = LineSolid;
            pGC->capStyle = CapButt;
            pGC->joinStyle = JoinMiter;
            pGC->fillStyle = FillSolid;
            pGC->fillRule = EvenOddRule;
            pGC->arcMode = ArcChord;
            pGC->patOrg.x = 0;
            pGC->patOrg.y = 0;
            pGC->subWindowMode = ClipByChildren;
            pGC->graphicsExposures = FALSE;
            pGC->clipOrg.x = 0;
            pGC->clipOrg.y = 0;
            if (pGC->clientClip)
                (*pGC->funcs->ChangeClip) (pGC, CT_NONE, NULL, 0);
            pGC->stateChanges = GCAllBits;
            return pGC;
        }
    }
    /* if we make it this far, need to roll our own */
    return CreateScratchGC(pScreen, depth);
}

/*
   if the gc to free is in the table of pre-existing ones,
mark it as available.
   if not, free it for real
*/
void
FreeScratchGC(GCPtr pGC)
{
    if (pGC->scratch_inuse)
        pGC->scratch_inuse = FALSE;
    else
        FreeGC(pGC, (GContext) 0);
}
