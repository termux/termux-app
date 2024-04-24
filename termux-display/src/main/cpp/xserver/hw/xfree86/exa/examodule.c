/*
 * Copyright © 2006 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "exa_priv.h"

#include "xf86str.h"
#include "xf86.h"

typedef struct _ExaXorgScreenPrivRec {
    CloseScreenProcPtr SavedCloseScreen;
    xf86EnableDisableFBAccessProc *SavedEnableDisableFBAccess;
    OptionInfoPtr options;
} ExaXorgScreenPrivRec, *ExaXorgScreenPrivPtr;

static DevPrivateKeyRec exaXorgScreenPrivateKeyRec;

#define exaXorgScreenPrivateKey (&exaXorgScreenPrivateKeyRec)

typedef enum {
    EXAOPT_MIGRATION_HEURISTIC,
    EXAOPT_NO_COMPOSITE,
    EXAOPT_NO_UTS,
    EXAOPT_NO_DFS,
    EXAOPT_OPTIMIZE_MIGRATION
} EXAOpts;

static const OptionInfoRec EXAOptions[] = {
    {EXAOPT_MIGRATION_HEURISTIC, "MigrationHeuristic",
     OPTV_ANYSTR, {0}, FALSE},
    {EXAOPT_NO_COMPOSITE, "EXANoComposite",
     OPTV_BOOLEAN, {0}, FALSE},
    {EXAOPT_NO_UTS, "EXANoUploadToScreen",
     OPTV_BOOLEAN, {0}, FALSE},
    {EXAOPT_NO_DFS, "EXANoDownloadFromScreen",
     OPTV_BOOLEAN, {0}, FALSE},
    {EXAOPT_OPTIMIZE_MIGRATION, "EXAOptimizeMigration",
     OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL,
     OPTV_NONE, {0}, FALSE}
};

static Bool
exaXorgCloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ExaXorgScreenPrivPtr pScreenPriv = (ExaXorgScreenPrivPtr)
        dixLookupPrivate(&pScreen->devPrivates, exaXorgScreenPrivateKey);

    pScreen->CloseScreen = pScreenPriv->SavedCloseScreen;

    pScrn->EnableDisableFBAccess = pScreenPriv->SavedEnableDisableFBAccess;

    free(pScreenPriv->options);
    free(pScreenPriv);

    return pScreen->CloseScreen(pScreen);
}

static void
exaXorgEnableDisableFBAccess(ScrnInfoPtr pScrn, Bool enable)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    ExaXorgScreenPrivPtr pScreenPriv = (ExaXorgScreenPrivPtr)
        dixLookupPrivate(&pScreen->devPrivates, exaXorgScreenPrivateKey);

    if (!enable)
        exaEnableDisableFBAccess(pScreen, enable);

    if (pScreenPriv->SavedEnableDisableFBAccess)
        pScreenPriv->SavedEnableDisableFBAccess(pScrn, enable);

    if (enable)
        exaEnableDisableFBAccess(pScreen, enable);
}

/**
 * This will be called during exaDriverInit, giving us the chance to set options
 * and hook in our EnableDisableFBAccess.
 */
void
exaDDXDriverInit(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ExaXorgScreenPrivPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&exaXorgScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return;

    pScreenPriv = calloc(1, sizeof(ExaXorgScreenPrivRec));
    if (pScreenPriv == NULL)
        return;

    pScreenPriv->options = xnfalloc(sizeof(EXAOptions));
    memcpy(pScreenPriv->options, EXAOptions, sizeof(EXAOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pScreenPriv->options);

    if (pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS) {
        if (!(pExaScr->info->flags & EXA_HANDLES_PIXMAPS) &&
            pExaScr->info->offScreenBase < pExaScr->info->memorySize) {
            const char *heuristicName;

            heuristicName = xf86GetOptValString(pScreenPriv->options,
                                                EXAOPT_MIGRATION_HEURISTIC);
            if (heuristicName != NULL) {
                if (strcmp(heuristicName, "greedy") == 0)
                    pExaScr->migration = ExaMigrationGreedy;
                else if (strcmp(heuristicName, "always") == 0)
                    pExaScr->migration = ExaMigrationAlways;
                else if (strcmp(heuristicName, "smart") == 0)
                    pExaScr->migration = ExaMigrationSmart;
                else {
                    xf86DrvMsg(pScreen->myNum, X_WARNING,
                               "EXA: unknown migration heuristic %s\n",
                               heuristicName);
                }
            }
        }

        pExaScr->optimize_migration =
            xf86ReturnOptValBool(pScreenPriv->options,
                                 EXAOPT_OPTIMIZE_MIGRATION, TRUE);
    }

    if (xf86ReturnOptValBool(pScreenPriv->options, EXAOPT_NO_COMPOSITE, FALSE)) {
        xf86DrvMsg(pScreen->myNum, X_CONFIG,
                   "EXA: Disabling Composite operation "
                   "(RENDER acceleration)\n");
        pExaScr->info->CheckComposite = NULL;
        pExaScr->info->PrepareComposite = NULL;
    }

    if (xf86ReturnOptValBool(pScreenPriv->options, EXAOPT_NO_UTS, FALSE)) {
        xf86DrvMsg(pScreen->myNum, X_CONFIG, "EXA: Disabling UploadToScreen\n");
        pExaScr->info->UploadToScreen = NULL;
    }

    if (xf86ReturnOptValBool(pScreenPriv->options, EXAOPT_NO_DFS, FALSE)) {
        xf86DrvMsg(pScreen->myNum, X_CONFIG,
                   "EXA: Disabling DownloadFromScreen\n");
        pExaScr->info->DownloadFromScreen = NULL;
    }

    dixSetPrivate(&pScreen->devPrivates, exaXorgScreenPrivateKey, pScreenPriv);

    pScreenPriv->SavedEnableDisableFBAccess = pScrn->EnableDisableFBAccess;
    pScrn->EnableDisableFBAccess = exaXorgEnableDisableFBAccess;

    pScreenPriv->SavedCloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = exaXorgCloseScreen;

}

static XF86ModuleVersionInfo exaVersRec = {
    "exa",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    EXA_VERSION_MAJOR, EXA_VERSION_MINOR, EXA_VERSION_RELEASE,
    ABI_CLASS_VIDEODRV,         /* requires the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData exaModuleData = { &exaVersRec, NULL, NULL };
