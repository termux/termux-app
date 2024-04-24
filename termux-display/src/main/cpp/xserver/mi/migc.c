/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "migc.h"

/* ARGSUSED */
void
miChangeGC(GCPtr pGC, unsigned long mask)
{
    return;
}

void
miDestroyGC(GCPtr pGC)
{
    if (pGC->freeCompClip)
        RegionDestroy(pGC->pCompositeClip);
}

void
miDestroyClip(GCPtr pGC)
{
    if (pGC->clientClip)
        RegionDestroy(pGC->clientClip);
    pGC->clientClip = NULL;
}

void
miChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    (*pGC->funcs->DestroyClip) (pGC);
    if (type == CT_PIXMAP) {
        /* convert the pixmap to a region */
        pGC->clientClip = BitmapToRegion(pGC->pScreen, (PixmapPtr) pvalue);
        (*pGC->pScreen->DestroyPixmap) (pvalue);
    }
    else if (type == CT_REGION) {
        /* stuff the region in the GC */
        pGC->clientClip = pvalue;
    }
    else if (type != CT_NONE) {
        pGC->clientClip = RegionFromRects(nrects, (xRectangle *) pvalue, type);
        free(pvalue);
    }
    pGC->stateChanges |= GCClipMask;
}

void
miCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    if (pgcSrc->clientClip) {
        RegionPtr prgnNew = RegionCreate(NULL, 1);
        RegionCopy(prgnNew, (RegionPtr) (pgcSrc->clientClip));
        (*pgcDst->funcs->ChangeClip) (pgcDst, CT_REGION, prgnNew, 0);
    } else {
        (*pgcDst->funcs->ChangeClip) (pgcDst, CT_NONE, NULL, 0);
    }
}

/* ARGSUSED */
void
miCopyGC(GCPtr pGCSrc, unsigned long changes, GCPtr pGCDst)
{
    return;
}

void
miComputeCompositeClip(GCPtr pGC, DrawablePtr pDrawable)
{
    if (pDrawable->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr) pDrawable;
        RegionPtr pregWin;
        Bool freeTmpClip, freeCompClip;

        if (pGC->subWindowMode == IncludeInferiors) {
            pregWin = NotClippedByChildren(pWin);
            freeTmpClip = TRUE;
        }
        else {
            pregWin = &pWin->clipList;
            freeTmpClip = FALSE;
        }
        freeCompClip = pGC->freeCompClip;

        /*
         * if there is no client clip, we can get by with just keeping the
         * pointer we got, and remembering whether or not should destroy (or
         * maybe re-use) it later.  this way, we avoid unnecessary copying of
         * regions.  (this wins especially if many clients clip by children
         * and have no client clip.)
         */
        if (!pGC->clientClip) {
            if (freeCompClip)
                RegionDestroy(pGC->pCompositeClip);
            pGC->pCompositeClip = pregWin;
            pGC->freeCompClip = freeTmpClip;
        }
        else {
            /*
             * we need one 'real' region to put into the composite clip. if
             * pregWin the current composite clip are real, we can get rid of
             * one. if pregWin is real and the current composite clip isn't,
             * use pregWin for the composite clip. if the current composite
             * clip is real and pregWin isn't, use the current composite
             * clip. if neither is real, create a new region.
             */

            RegionTranslate(pGC->clientClip,
                            pDrawable->x + pGC->clipOrg.x,
                            pDrawable->y + pGC->clipOrg.y);

            if (freeCompClip) {
                RegionIntersect(pGC->pCompositeClip, pregWin, pGC->clientClip);
                if (freeTmpClip)
                    RegionDestroy(pregWin);
            }
            else if (freeTmpClip) {
                RegionIntersect(pregWin, pregWin, pGC->clientClip);
                pGC->pCompositeClip = pregWin;
            }
            else {
                pGC->pCompositeClip = RegionCreate(NullBox, 0);
                RegionIntersect(pGC->pCompositeClip, pregWin, pGC->clientClip);
            }
            pGC->freeCompClip = TRUE;
            RegionTranslate(pGC->clientClip,
                            -(pDrawable->x + pGC->clipOrg.x),
                            -(pDrawable->y + pGC->clipOrg.y));
        }
    }                           /* end of composite clip for a window */
    else {
        BoxRec pixbounds;

        /* XXX should we translate by drawable.x/y here ? */
        /* If you want pixmaps in offscreen memory, yes */
        pixbounds.x1 = pDrawable->x;
        pixbounds.y1 = pDrawable->y;
        pixbounds.x2 = pDrawable->x + pDrawable->width;
        pixbounds.y2 = pDrawable->y + pDrawable->height;

        if (pGC->freeCompClip) {
            RegionReset(pGC->pCompositeClip, &pixbounds);
        }
        else {
            pGC->freeCompClip = TRUE;
            pGC->pCompositeClip = RegionCreate(&pixbounds, 1);
        }

        if (pGC->clientClip) {
            if (pDrawable->x || pDrawable->y) {
                RegionTranslate(pGC->clientClip,
                                pDrawable->x + pGC->clipOrg.x,
                                pDrawable->y + pGC->clipOrg.y);
                RegionIntersect(pGC->pCompositeClip,
                                pGC->pCompositeClip, pGC->clientClip);
                RegionTranslate(pGC->clientClip,
                                -(pDrawable->x + pGC->clipOrg.x),
                                -(pDrawable->y + pGC->clipOrg.y));
            }
            else {
                RegionTranslate(pGC->pCompositeClip,
                                -pGC->clipOrg.x, -pGC->clipOrg.y);
                RegionIntersect(pGC->pCompositeClip,
                                pGC->pCompositeClip, pGC->clientClip);
                RegionTranslate(pGC->pCompositeClip,
                                pGC->clipOrg.x, pGC->clipOrg.y);
            }
        }
    }                           /* end of composite clip for pixmap */
}                               /* end miComputeCompositeClip */
