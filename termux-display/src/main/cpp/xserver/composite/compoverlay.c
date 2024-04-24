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
#include "xace.h"

#ifdef PANORAMIX
#include "panoramiXsrv.h"
#endif

/*
 * Delete the given overlay client list element from its screen list.
 */
void
compFreeOverlayClient(CompOverlayClientPtr pOcToDel)
{
    ScreenPtr pScreen = pOcToDel->pScreen;
    CompScreenPtr cs = GetCompScreen(pScreen);
    CompOverlayClientPtr *pPrev, pOc;

    for (pPrev = &cs->pOverlayClients; (pOc = *pPrev); pPrev = &pOc->pNext) {
        if (pOc == pOcToDel) {
            *pPrev = pOc->pNext;
            free(pOc);
            break;
        }
    }

    /* Destroy overlay window when there are no more clients using it */
    if (cs->pOverlayClients == NULL)
        compDestroyOverlayWindow(pScreen);
}

/*
 * Return the client's first overlay client rec from the given screen
 */
CompOverlayClientPtr
compFindOverlayClient(ScreenPtr pScreen, ClientPtr pClient)
{
    CompScreenPtr cs = GetCompScreen(pScreen);
    CompOverlayClientPtr pOc;

    for (pOc = cs->pOverlayClients; pOc != NULL; pOc = pOc->pNext)
        if (pOc->pClient == pClient)
            return pOc;

    return NULL;
}

/*
 * Create an overlay client object for the given client
 */
CompOverlayClientPtr
compCreateOverlayClient(ScreenPtr pScreen, ClientPtr pClient)
{
    CompScreenPtr cs = GetCompScreen(pScreen);
    CompOverlayClientPtr pOc;

    pOc = (CompOverlayClientPtr) malloc(sizeof(CompOverlayClientRec));
    if (pOc == NULL)
        return NULL;

    pOc->pClient = pClient;
    pOc->pScreen = pScreen;
    pOc->resource = FakeClientID(pClient->index);
    pOc->pNext = cs->pOverlayClients;
    cs->pOverlayClients = pOc;

    /*
     * Create a resource for this element so it can be deleted
     * when the client goes away.
     */
    if (!AddResource(pOc->resource, CompositeClientOverlayType, (void *) pOc))
        return NULL;

    return pOc;
}

/*
 * Create the overlay window and map it
 */
Bool
compCreateOverlayWindow(ScreenPtr pScreen)
{
    CompScreenPtr cs = GetCompScreen(pScreen);
    WindowPtr pRoot = pScreen->root;
    WindowPtr pWin;
    XID attrs[] = { None, TRUE };       /* backPixmap, overrideRedirect */
    int result;
    int w = pScreen->width;
    int h = pScreen->height;
    int x = 0, y = 0;

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        x = -pScreen->x;
        y = -pScreen->y;
        w = PanoramiXPixWidth;
        h = PanoramiXPixHeight;
    }
#endif

    pWin = cs->pOverlayWin =
        CreateWindow(cs->overlayWid, pRoot, x, y, w, h, 0,
                     InputOutput, CWBackPixmap | CWOverrideRedirect, &attrs[0],
                     pRoot->drawable.depth,
                     serverClient, pScreen->rootVisual, &result);
    if (pWin == NULL)
        return FALSE;

    if (!AddResource(pWin->drawable.id, RT_WINDOW, (void *) pWin))
        return FALSE;

    MapWindow(pWin, serverClient);

    return TRUE;
}

/*
 * Destroy the overlay window
 */
void
compDestroyOverlayWindow(ScreenPtr pScreen)
{
    CompScreenPtr cs = GetCompScreen(pScreen);

    cs->pOverlayWin = NullWindow;
    FreeResource(cs->overlayWid, RT_NONE);
}
