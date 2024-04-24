/*
 * Copyright © 2010 NVIDIA Corporation
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
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "scrnintstr.h"
#include "misync.h"
#include "misyncstr.h"

DevPrivateKeyRec miSyncScreenPrivateKey;

/* Default implementations of the sync screen functions */
void
miSyncScreenCreateFence(ScreenPtr pScreen, SyncFence * pFence,
                        Bool initially_triggered)
{
    (void) pScreen;

    pFence->triggered = initially_triggered;
}

void
miSyncScreenDestroyFence(ScreenPtr pScreen, SyncFence * pFence)
{
    (void) pScreen;
    (void) pFence;
}

/* Default implementations of the per-object functions */
void
miSyncFenceSetTriggered(SyncFence * pFence)
{
    pFence->triggered = TRUE;
}

void
miSyncFenceReset(SyncFence * pFence)
{
    pFence->triggered = FALSE;
}

Bool
miSyncFenceCheckTriggered(SyncFence * pFence)
{
    return pFence->triggered;
}

void
miSyncFenceAddTrigger(SyncTrigger * pTrigger)
{
    (void) pTrigger;

    return;
}

void
miSyncFenceDeleteTrigger(SyncTrigger * pTrigger)
{
    (void) pTrigger;

    return;
}

/* Machine independent portion of the fence sync object implementation */
void
miSyncInitFence(ScreenPtr pScreen, SyncFence * pFence, Bool initially_triggered)
{
    SyncScreenPrivPtr pScreenPriv = SYNC_SCREEN_PRIV(pScreen);

    static const SyncFenceFuncsRec miSyncFenceFuncs = {
        &miSyncFenceSetTriggered,
        &miSyncFenceReset,
        &miSyncFenceCheckTriggered,
        &miSyncFenceAddTrigger,
        &miSyncFenceDeleteTrigger
    };

    pFence->pScreen = pScreen;
    pFence->funcs = miSyncFenceFuncs;

    pScreenPriv->funcs.CreateFence(pScreen, pFence, initially_triggered);

    pFence->sync.initialized = TRUE;
}

void
miSyncDestroyFence(SyncFence * pFence)
{
    pFence->sync.beingDestroyed = TRUE;

    if (pFence->sync.initialized) {
        ScreenPtr pScreen = pFence->pScreen;
        SyncScreenPrivPtr pScreenPriv = SYNC_SCREEN_PRIV(pScreen);
        SyncTriggerList *ptl, *pNext;

        /* tell all the fence's triggers that the counter has been destroyed */
        for (ptl = pFence->sync.pTriglist; ptl; ptl = pNext) {
            (*ptl->pTrigger->CounterDestroyed) (ptl->pTrigger);
            pNext = ptl->next;
            free(ptl); /* destroy the trigger list as we go */
        }

        pScreenPriv->funcs.DestroyFence(pScreen, pFence);
    }

    dixFreeObjectWithPrivates(pFence, PRIVATE_SYNC_FENCE);
}

void
miSyncTriggerFence(SyncFence * pFence)
{
    SyncTriggerList *ptl, *pNext;

    pFence->funcs.SetTriggered(pFence);

    /* run through triggers to see if any fired */
    for (ptl = pFence->sync.pTriglist; ptl; ptl = pNext) {
        pNext = ptl->next;
        if ((*ptl->pTrigger->CheckTrigger) (ptl->pTrigger, 0))
            (*ptl->pTrigger->TriggerFired) (ptl->pTrigger);
    }
}

SyncScreenFuncsPtr
miSyncGetScreenFuncs(ScreenPtr pScreen)
{
    SyncScreenPrivPtr pScreenPriv = SYNC_SCREEN_PRIV(pScreen);

    return &pScreenPriv->funcs;
}

static Bool
SyncCloseScreen(ScreenPtr pScreen)
{
    SyncScreenPrivPtr pScreenPriv = SYNC_SCREEN_PRIV(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    return (*pScreen->CloseScreen) (pScreen);
}

Bool
miSyncSetup(ScreenPtr pScreen)
{
    SyncScreenPrivPtr pScreenPriv;

    static const SyncScreenFuncsRec miSyncScreenFuncs = {
        &miSyncScreenCreateFence,
        &miSyncScreenDestroyFence
    };

    if (!dixPrivateKeyRegistered(&miSyncScreenPrivateKey)) {
        if (!dixRegisterPrivateKey(&miSyncScreenPrivateKey, PRIVATE_SCREEN,
                                   sizeof(SyncScreenPrivRec)))
            return FALSE;
    }

    pScreenPriv = SYNC_SCREEN_PRIV(pScreen);

    if (!pScreenPriv->funcs.CreateFence) {
        pScreenPriv->funcs = miSyncScreenFuncs;

        /* Wrap CloseScreen to clean up */
        pScreenPriv->CloseScreen = pScreen->CloseScreen;
        pScreen->CloseScreen = SyncCloseScreen;
    }

    return TRUE;
}
