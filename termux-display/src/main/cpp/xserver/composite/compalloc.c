/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "compint.h"

static Bool
compScreenUpdate(ClientPtr pClient, void *closure)
{
    ScreenPtr pScreen = closure;
    CompScreenPtr cs = GetCompScreen(pScreen);

    compCheckTree(pScreen);
    compPaintChildrenToWindow(pScreen->root);

    /* Next damage will restore the worker */
    cs->pendingScreenUpdate = FALSE;
    return TRUE;
}

void
compMarkAncestors(WindowPtr pWin)
{
    pWin = pWin->parent;
    while (pWin) {
        if (pWin->damagedDescendants)
            return;
        pWin->damagedDescendants = TRUE;
        pWin = pWin->parent;
    }
}

static void
compReportDamage(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    WindowPtr pWin = (WindowPtr) closure;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    CompScreenPtr cs = GetCompScreen(pScreen);
    CompWindowPtr cw = GetCompWindow(pWin);

    if (!cs->pendingScreenUpdate) {
        QueueWorkProc(compScreenUpdate, serverClient, pScreen);
        cs->pendingScreenUpdate = TRUE;
    }
    cw->damaged = TRUE;

    compMarkAncestors(pWin);
}

static void
compDestroyDamage(DamagePtr pDamage, void *closure)
{
    WindowPtr pWin = (WindowPtr) closure;
    CompWindowPtr cw = GetCompWindow(pWin);

    cw->damage = 0;
    cw->damaged = 0;
    cw->damageRegistered = 0;
}

static Bool
compMarkWindows(WindowPtr pWin, WindowPtr *ppLayerWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pLayerWin = pWin;

    if (!pWin->viewable)
        return FALSE;

    (*pScreen->MarkOverlappedWindows) (pWin, pWin, &pLayerWin);
    (*pScreen->MarkWindow) (pLayerWin->parent);

    *ppLayerWin = pLayerWin;

    return TRUE;
}

static void
compHandleMarkedWindows(WindowPtr pWin, WindowPtr pLayerWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    (*pScreen->ValidateTree) (pLayerWin->parent, pLayerWin, VTOther);
    (*pScreen->HandleExposures) (pLayerWin->parent);
    if (pScreen->PostValidateTree)
        (*pScreen->PostValidateTree) (pLayerWin->parent, pLayerWin, VTOther);
}

/*
 * Redirect one window for one client
 */
int
compRedirectWindow(ClientPtr pClient, WindowPtr pWin, int update)
{
    CompWindowPtr cw = GetCompWindow(pWin);
    CompClientWindowPtr ccw;
    CompScreenPtr cs = GetCompScreen(pWin->drawable.pScreen);
    WindowPtr pLayerWin;
    Bool anyMarked = FALSE;

    if (pWin == cs->pOverlayWin) {
        return Success;
    }

    if (!pWin->parent)
        return BadMatch;

    /*
     * Only one Manual update is allowed
     */
    if (cw && update == CompositeRedirectManual)
        for (ccw = cw->clients; ccw; ccw = ccw->next)
            if (ccw->update == CompositeRedirectManual)
                return BadAccess;

    /*
     * Allocate per-client per-window structure
     * The client *could* allocate multiple, but while supported,
     * it is not expected to be common
     */
    ccw = malloc(sizeof(CompClientWindowRec));
    if (!ccw)
        return BadAlloc;
    ccw->id = FakeClientID(pClient->index);
    ccw->update = update;
    /*
     * Now make sure there's a per-window structure to hang this from
     */
    if (!cw) {
        cw = malloc(sizeof(CompWindowRec));
        if (!cw) {
            free(ccw);
            return BadAlloc;
        }
        cw->damage = DamageCreate(compReportDamage,
                                  compDestroyDamage,
                                  DamageReportNonEmpty,
                                  FALSE, pWin->drawable.pScreen, pWin);
        if (!cw->damage) {
            free(ccw);
            free(cw);
            return BadAlloc;
        }

        anyMarked = compMarkWindows(pWin, &pLayerWin);

        RegionNull(&cw->borderClip);
        cw->update = CompositeRedirectAutomatic;
        cw->clients = 0;
        cw->oldx = COMP_ORIGIN_INVALID;
        cw->oldy = COMP_ORIGIN_INVALID;
        cw->damageRegistered = FALSE;
        cw->damaged = FALSE;
        cw->pOldPixmap = NullPixmap;
        dixSetPrivate(&pWin->devPrivates, CompWindowPrivateKey, cw);
    }
    ccw->next = cw->clients;
    cw->clients = ccw;
    if (!AddResource(ccw->id, CompositeClientWindowType, pWin))
        return BadAlloc;
    if (ccw->update == CompositeRedirectManual) {
        if (!anyMarked)
            anyMarked = compMarkWindows(pWin, &pLayerWin);

        if (cw->damageRegistered) {
            DamageUnregister(cw->damage);
            cw->damageRegistered = FALSE;
        }
        cw->update = CompositeRedirectManual;
    }
    else if (cw->update == CompositeRedirectAutomatic && !cw->damageRegistered) {
        if (!anyMarked)
            anyMarked = compMarkWindows(pWin, &pLayerWin);
    }

    if (!compCheckRedirect(pWin)) {
        FreeResource(ccw->id, RT_NONE);
        return BadAlloc;
    }

    if (anyMarked)
        compHandleMarkedWindows(pWin, pLayerWin);

    return Success;
}

void
compRestoreWindow(WindowPtr pWin, PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pParent = pWin->parent;

    if (pParent->drawable.depth == pWin->drawable.depth) {
        GCPtr pGC = GetScratchGC(pWin->drawable.depth, pScreen);
        int bw = (int) pWin->borderWidth;
        int x = bw;
        int y = bw;
        int w = pWin->drawable.width;
        int h = pWin->drawable.height;

        if (pGC) {
            ChangeGCVal val;

            val.val = IncludeInferiors;
            ChangeGC(NullClient, pGC, GCSubwindowMode, &val);
            ValidateGC(&pWin->drawable, pGC);
            (*pGC->ops->CopyArea) (&pPixmap->drawable,
                                   &pWin->drawable, pGC, x, y, w, h, 0, 0);
            FreeScratchGC(pGC);
        }
    }
}

/*
 * Free one of the per-client per-window resources, clearing
 * redirect and the per-window pointer as appropriate
 */
void
compFreeClientWindow(WindowPtr pWin, XID id)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    CompWindowPtr cw = GetCompWindow(pWin);
    CompClientWindowPtr ccw, *prev;
    Bool anyMarked = FALSE;
    WindowPtr pLayerWin;
    PixmapPtr pPixmap = NULL;

    if (!cw)
        return;
    for (prev = &cw->clients; (ccw = *prev); prev = &ccw->next) {
        if (ccw->id == id) {
            *prev = ccw->next;
            if (ccw->update == CompositeRedirectManual)
                cw->update = CompositeRedirectAutomatic;
            free(ccw);
            break;
        }
    }
    if (!cw->clients) {
        anyMarked = compMarkWindows(pWin, &pLayerWin);

        if (pWin->redirectDraw != RedirectDrawNone) {
            pPixmap = (*pScreen->GetWindowPixmap) (pWin);
            compSetParentPixmap(pWin);
        }

        if (cw->damage)
            DamageDestroy(cw->damage);

        RegionUninit(&cw->borderClip);

        dixSetPrivate(&pWin->devPrivates, CompWindowPrivateKey, NULL);
        free(cw);
    }
    else if (cw->update == CompositeRedirectAutomatic &&
             !cw->damageRegistered && pWin->redirectDraw != RedirectDrawNone) {
        anyMarked = compMarkWindows(pWin, &pLayerWin);

        DamageRegister(&pWin->drawable, cw->damage);
        cw->damageRegistered = TRUE;
        pWin->redirectDraw = RedirectDrawAutomatic;
        DamageDamageRegion(&pWin->drawable, &pWin->borderSize);
    }

    if (anyMarked)
        compHandleMarkedWindows(pWin, pLayerWin);

    if (pPixmap) {
        compRestoreWindow(pWin, pPixmap);
        (*pScreen->DestroyPixmap) (pPixmap);
    }
}

/*
 * This is easy, just free the appropriate resource.
 */

int
compUnredirectWindow(ClientPtr pClient, WindowPtr pWin, int update)
{
    CompWindowPtr cw = GetCompWindow(pWin);
    CompClientWindowPtr ccw;

    if (!cw)
        return BadValue;

    for (ccw = cw->clients; ccw; ccw = ccw->next)
        if (ccw->update == update && CLIENT_ID(ccw->id) == pClient->index) {
            FreeResource(ccw->id, RT_NONE);
            return Success;
        }
    return BadValue;
}

/*
 * Redirect all subwindows for one client
 */

int
compRedirectSubwindows(ClientPtr pClient, WindowPtr pWin, int update)
{
    CompSubwindowsPtr csw = GetCompSubwindows(pWin);
    CompClientWindowPtr ccw;
    WindowPtr pChild;

    /*
     * Only one Manual update is allowed
     */
    if (csw && update == CompositeRedirectManual)
        for (ccw = csw->clients; ccw; ccw = ccw->next)
            if (ccw->update == CompositeRedirectManual)
                return BadAccess;
    /*
     * Allocate per-client per-window structure
     * The client *could* allocate multiple, but while supported,
     * it is not expected to be common
     */
    ccw = malloc(sizeof(CompClientWindowRec));
    if (!ccw)
        return BadAlloc;
    ccw->id = FakeClientID(pClient->index);
    ccw->update = update;
    /*
     * Now make sure there's a per-window structure to hang this from
     */
    if (!csw) {
        csw = malloc(sizeof(CompSubwindowsRec));
        if (!csw) {
            free(ccw);
            return BadAlloc;
        }
        csw->update = CompositeRedirectAutomatic;
        csw->clients = 0;
        dixSetPrivate(&pWin->devPrivates, CompSubwindowsPrivateKey, csw);
    }
    /*
     * Redirect all existing windows
     */
    for (pChild = pWin->lastChild; pChild; pChild = pChild->prevSib) {
        int ret = compRedirectWindow(pClient, pChild, update);

        if (ret != Success) {
            for (pChild = pChild->nextSib; pChild; pChild = pChild->nextSib)
                (void) compUnredirectWindow(pClient, pChild, update);
            if (!csw->clients) {
                free(csw);
                dixSetPrivate(&pWin->devPrivates, CompSubwindowsPrivateKey, 0);
            }
            free(ccw);
            return ret;
        }
    }
    /*
     * Hook into subwindows list
     */
    ccw->next = csw->clients;
    csw->clients = ccw;
    if (!AddResource(ccw->id, CompositeClientSubwindowsType, pWin))
        return BadAlloc;
    if (ccw->update == CompositeRedirectManual) {
        csw->update = CompositeRedirectManual;
        /*
         * tell damage extension that damage events for this client are
         * critical output
         */
        DamageExtSetCritical(pClient, TRUE);
        pWin->inhibitBGPaint = TRUE;
    }
    return Success;
}

/*
 * Free one of the per-client per-subwindows resources,
 * which frees one redirect per subwindow
 */
void
compFreeClientSubwindows(WindowPtr pWin, XID id)
{
    CompSubwindowsPtr csw = GetCompSubwindows(pWin);
    CompClientWindowPtr ccw, *prev;
    WindowPtr pChild;

    if (!csw)
        return;
    for (prev = &csw->clients; (ccw = *prev); prev = &ccw->next) {
        if (ccw->id == id) {
            ClientPtr pClient = clients[CLIENT_ID(id)];

            *prev = ccw->next;
            if (ccw->update == CompositeRedirectManual) {
                /*
                 * tell damage extension that damage events for this client are
                 * critical output
                 */
                DamageExtSetCritical(pClient, FALSE);
                csw->update = CompositeRedirectAutomatic;
                pWin->inhibitBGPaint = FALSE;
                if (pWin->mapped)
                    (*pWin->drawable.pScreen->ClearToBackground) (pWin, 0, 0, 0,
                                                                  0, TRUE);
            }

            /*
             * Unredirect all existing subwindows
             */
            for (pChild = pWin->lastChild; pChild; pChild = pChild->prevSib)
                (void) compUnredirectWindow(pClient, pChild, ccw->update);

            free(ccw);
            break;
        }
    }

    /*
     * Check if all of the per-client records are gone
     */
    if (!csw->clients) {
        dixSetPrivate(&pWin->devPrivates, CompSubwindowsPrivateKey, NULL);
        free(csw);
    }
}

/*
 * This is easy, just free the appropriate resource.
 */

int
compUnredirectSubwindows(ClientPtr pClient, WindowPtr pWin, int update)
{
    CompSubwindowsPtr csw = GetCompSubwindows(pWin);
    CompClientWindowPtr ccw;

    if (!csw)
        return BadValue;
    for (ccw = csw->clients; ccw; ccw = ccw->next)
        if (ccw->update == update && CLIENT_ID(ccw->id) == pClient->index) {
            FreeResource(ccw->id, RT_NONE);
            return Success;
        }
    return BadValue;
}

/*
 * Add redirection information for one subwindow (during reparent)
 */

int
compRedirectOneSubwindow(WindowPtr pParent, WindowPtr pWin)
{
    CompSubwindowsPtr csw = GetCompSubwindows(pParent);
    CompClientWindowPtr ccw;

    if (!csw)
        return Success;
    for (ccw = csw->clients; ccw; ccw = ccw->next) {
        int ret = compRedirectWindow(clients[CLIENT_ID(ccw->id)],
                                     pWin, ccw->update);

        if (ret != Success)
            return ret;
    }
    return Success;
}

/*
 * Remove redirection information for one subwindow (during reparent)
 */

int
compUnredirectOneSubwindow(WindowPtr pParent, WindowPtr pWin)
{
    CompSubwindowsPtr csw = GetCompSubwindows(pParent);
    CompClientWindowPtr ccw;

    if (!csw)
        return Success;
    for (ccw = csw->clients; ccw; ccw = ccw->next) {
        int ret = compUnredirectWindow(clients[CLIENT_ID(ccw->id)],
                                       pWin, ccw->update);

        if (ret != Success)
            return ret;
    }
    return Success;
}

static PixmapPtr
compNewPixmap(WindowPtr pWin, int x, int y, int w, int h)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pParent = pWin->parent;
    PixmapPtr pPixmap;

    pPixmap = (*pScreen->CreatePixmap) (pScreen, w, h, pWin->drawable.depth,
                                        CREATE_PIXMAP_USAGE_BACKING_PIXMAP);

    if (!pPixmap)
        return 0;

    pPixmap->screen_x = x;
    pPixmap->screen_y = y;

    if (pParent->drawable.depth == pWin->drawable.depth) {
        GCPtr pGC = GetScratchGC(pWin->drawable.depth, pScreen);

        if (pGC) {
            ChangeGCVal val;

            val.val = IncludeInferiors;
            ChangeGC(NullClient, pGC, GCSubwindowMode, &val);
            ValidateGC(&pPixmap->drawable, pGC);
            (*pGC->ops->CopyArea) (&pParent->drawable,
                                   &pPixmap->drawable,
                                   pGC,
                                   x - pParent->drawable.x,
                                   y - pParent->drawable.y, w, h, 0, 0);
            FreeScratchGC(pGC);
        }
    }
    else {
        PictFormatPtr pSrcFormat = PictureWindowFormat(pParent);
        PictFormatPtr pDstFormat = PictureWindowFormat(pWin);
        XID inferiors = IncludeInferiors;
        int error;

        PicturePtr pSrcPicture = CreatePicture(None,
                                               &pParent->drawable,
                                               pSrcFormat,
                                               CPSubwindowMode,
                                               &inferiors,
                                               serverClient, &error);

        PicturePtr pDstPicture = CreatePicture(None,
                                               &pPixmap->drawable,
                                               pDstFormat,
                                               0, 0,
                                               serverClient, &error);

        if (pSrcPicture && pDstPicture) {
            CompositePicture(PictOpSrc,
                             pSrcPicture,
                             NULL,
                             pDstPicture,
                             x - pParent->drawable.x,
                             y - pParent->drawable.y, 0, 0, 0, 0, w, h);
        }
        if (pSrcPicture)
            FreePicture(pSrcPicture, 0);
        if (pDstPicture)
            FreePicture(pDstPicture, 0);
    }
    return pPixmap;
}

Bool
compAllocPixmap(WindowPtr pWin)
{
    int bw = (int) pWin->borderWidth;
    int x = pWin->drawable.x - bw;
    int y = pWin->drawable.y - bw;
    int w = pWin->drawable.width + (bw << 1);
    int h = pWin->drawable.height + (bw << 1);
    PixmapPtr pPixmap = compNewPixmap(pWin, x, y, w, h);
    CompWindowPtr cw = GetCompWindow(pWin);

    if (!pPixmap)
        return FALSE;
    if (cw->update == CompositeRedirectAutomatic)
        pWin->redirectDraw = RedirectDrawAutomatic;
    else
        pWin->redirectDraw = RedirectDrawManual;

    compSetPixmap(pWin, pPixmap, bw);
    cw->oldx = COMP_ORIGIN_INVALID;
    cw->oldy = COMP_ORIGIN_INVALID;
    cw->damageRegistered = FALSE;
    if (cw->update == CompositeRedirectAutomatic) {
        DamageRegister(&pWin->drawable, cw->damage);
        cw->damageRegistered = TRUE;
    }

    /* Make sure our borderClip is up to date */
    RegionUninit(&cw->borderClip);
    RegionCopy(&cw->borderClip, &pWin->borderClip);
    cw->borderClipX = pWin->drawable.x;
    cw->borderClipY = pWin->drawable.y;

    return TRUE;
}

void
compSetParentPixmap(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    PixmapPtr pParentPixmap;
    CompWindowPtr cw = GetCompWindow(pWin);

    if (cw->damageRegistered) {
        DamageUnregister(cw->damage);
        cw->damageRegistered = FALSE;
        DamageEmpty(cw->damage);
    }
    /*
     * Move the parent-constrained border clip region back into
     * the window so that ValidateTree will handle the unmap
     * case correctly.  Unmap adds the window borderClip to the
     * parent exposed area; regions beyond the parent cause crashes
     */
    RegionCopy(&pWin->borderClip, &cw->borderClip);
    pParentPixmap = (*pScreen->GetWindowPixmap) (pWin->parent);
    pWin->redirectDraw = RedirectDrawNone;
    compSetPixmap(pWin, pParentPixmap, pWin->borderWidth);
}

/*
 * Make sure the pixmap is the right size and offset.  Allocate a new
 * pixmap to change size, adjust origin to change offset, leaving the
 * old pixmap in cw->pOldPixmap so bits can be recovered
 */
Bool
compReallocPixmap(WindowPtr pWin, int draw_x, int draw_y,
                  unsigned int w, unsigned int h, int bw)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    PixmapPtr pOld = (*pScreen->GetWindowPixmap) (pWin);
    PixmapPtr pNew;
    CompWindowPtr cw = GetCompWindow(pWin);
    int pix_x, pix_y;
    int pix_w, pix_h;

    assert(cw);
    assert(pWin->redirectDraw != RedirectDrawNone);
    cw->oldx = pOld->screen_x;
    cw->oldy = pOld->screen_y;
    pix_x = draw_x - bw;
    pix_y = draw_y - bw;
    pix_w = w + (bw << 1);
    pix_h = h + (bw << 1);
    if (pix_w != pOld->drawable.width || pix_h != pOld->drawable.height) {
        pNew = compNewPixmap(pWin, pix_x, pix_y, pix_w, pix_h);
        if (!pNew)
            return FALSE;
        cw->pOldPixmap = pOld;
        compSetPixmap(pWin, pNew, bw);
    }
    else {
        pNew = pOld;
        cw->pOldPixmap = 0;
    }
    pNew->screen_x = pix_x;
    pNew->screen_y = pix_y;
    return TRUE;
}
