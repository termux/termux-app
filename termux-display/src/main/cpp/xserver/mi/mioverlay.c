
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "scrnintstr.h"
#include <X11/extensions/shapeproto.h>
#include "validate.h"
#include "windowstr.h"
#include "mi.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "privates.h"
#include "mivalidate.h"
#include "mioverlay.h"
#include "migc.h"

#include "globals.h"

typedef struct {
    RegionRec exposed;
    RegionRec borderExposed;
    RegionPtr borderVisible;
    DDXPointRec oldAbsCorner;
} miOverlayValDataRec, *miOverlayValDataPtr;

typedef struct _TreeRec {
    WindowPtr pWin;
    struct _TreeRec *parent;
    struct _TreeRec *firstChild;
    struct _TreeRec *lastChild;
    struct _TreeRec *prevSib;
    struct _TreeRec *nextSib;
    RegionRec borderClip;
    RegionRec clipList;
    unsigned visibility;
    miOverlayValDataPtr valdata;
} miOverlayTreeRec, *miOverlayTreePtr;

typedef struct {
    miOverlayTreePtr tree;
} miOverlayWindowRec, *miOverlayWindowPtr;

typedef struct {
    CloseScreenProcPtr CloseScreen;
    CreateWindowProcPtr CreateWindow;
    DestroyWindowProcPtr DestroyWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    RealizeWindowProcPtr RealizeWindow;
    miOverlayTransFunc MakeTransparent;
    miOverlayInOverlayFunc InOverlay;
    Bool underlayMarked;
    Bool copyUnderlay;
} miOverlayScreenRec, *miOverlayScreenPtr;

static DevPrivateKeyRec miOverlayWindowKeyRec;

#define miOverlayWindowKey (&miOverlayWindowKeyRec)
static DevPrivateKeyRec miOverlayScreenKeyRec;

#define miOverlayScreenKey (&miOverlayScreenKeyRec)

static void RebuildTree(WindowPtr);
static Bool HasUnderlayChildren(WindowPtr);
static void MarkUnderlayWindow(WindowPtr);
static Bool CollectUnderlayChildrenRegions(WindowPtr, RegionPtr);

static Bool miOverlayCloseScreen(ScreenPtr);
static Bool miOverlayCreateWindow(WindowPtr);
static Bool miOverlayDestroyWindow(WindowPtr);
static Bool miOverlayUnrealizeWindow(WindowPtr);
static Bool miOverlayRealizeWindow(WindowPtr);
static void miOverlayMarkWindow(WindowPtr);
static void miOverlayReparentWindow(WindowPtr, WindowPtr);
static void miOverlayRestackWindow(WindowPtr, WindowPtr);
static Bool miOverlayMarkOverlappedWindows(WindowPtr, WindowPtr, WindowPtr *);
static void miOverlayMarkUnrealizedWindow(WindowPtr, WindowPtr, Bool);
static int miOverlayValidateTree(WindowPtr, WindowPtr, VTKind);
static void miOverlayHandleExposures(WindowPtr);
static void miOverlayMoveWindow(WindowPtr, int, int, WindowPtr, VTKind);
static void miOverlayWindowExposures(WindowPtr, RegionPtr);
static void miOverlayResizeWindow(WindowPtr, int, int, unsigned int,
                                  unsigned int, WindowPtr);
static void miOverlayClearToBackground(WindowPtr, int, int, int, int, Bool);

static void miOverlaySetShape(WindowPtr, int);
static void miOverlayChangeBorderWidth(WindowPtr, unsigned int);

#define MIOVERLAY_GET_SCREEN_PRIVATE(pScreen) ((miOverlayScreenPtr) \
	dixLookupPrivate(&(pScreen)->devPrivates, miOverlayScreenKey))
#define MIOVERLAY_GET_WINDOW_PRIVATE(pWin) ((miOverlayWindowPtr) \
	dixLookupPrivate(&(pWin)->devPrivates, miOverlayWindowKey))
#define MIOVERLAY_GET_WINDOW_TREE(pWin) \
	(MIOVERLAY_GET_WINDOW_PRIVATE(pWin)->tree)

#define IN_UNDERLAY(w) MIOVERLAY_GET_WINDOW_TREE(w)
#define IN_OVERLAY(w) !MIOVERLAY_GET_WINDOW_TREE(w)

#define MARK_OVERLAY(w) miMarkWindow(w)
#define MARK_UNDERLAY(w) MarkUnderlayWindow(w)

#define HasParentRelativeBorder(w) (!(w)->borderIsPixel && \
                                    HasBorder(w) && \
                                    (w)->backgroundState == ParentRelative)

Bool
miInitOverlay(ScreenPtr pScreen,
              miOverlayInOverlayFunc inOverlayFunc,
              miOverlayTransFunc transFunc)
{
    miOverlayScreenPtr pScreenPriv;

    if (!inOverlayFunc || !transFunc)
        return FALSE;

    if (!dixRegisterPrivateKey
        (&miOverlayWindowKeyRec, PRIVATE_WINDOW, sizeof(miOverlayWindowRec)))
        return FALSE;

    if (!dixRegisterPrivateKey(&miOverlayScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!(pScreenPriv = malloc(sizeof(miOverlayScreenRec))))
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, miOverlayScreenKey, pScreenPriv);

    pScreenPriv->InOverlay = inOverlayFunc;
    pScreenPriv->MakeTransparent = transFunc;
    pScreenPriv->underlayMarked = FALSE;

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->CreateWindow = pScreen->CreateWindow;
    pScreenPriv->DestroyWindow = pScreen->DestroyWindow;
    pScreenPriv->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreenPriv->RealizeWindow = pScreen->RealizeWindow;

    pScreen->CloseScreen = miOverlayCloseScreen;
    pScreen->CreateWindow = miOverlayCreateWindow;
    pScreen->DestroyWindow = miOverlayDestroyWindow;
    pScreen->UnrealizeWindow = miOverlayUnrealizeWindow;
    pScreen->RealizeWindow = miOverlayRealizeWindow;

    pScreen->ReparentWindow = miOverlayReparentWindow;
    pScreen->RestackWindow = miOverlayRestackWindow;
    pScreen->MarkOverlappedWindows = miOverlayMarkOverlappedWindows;
    pScreen->MarkUnrealizedWindow = miOverlayMarkUnrealizedWindow;
    pScreen->ValidateTree = miOverlayValidateTree;
    pScreen->HandleExposures = miOverlayHandleExposures;
    pScreen->MoveWindow = miOverlayMoveWindow;
    pScreen->WindowExposures = miOverlayWindowExposures;
    pScreen->ResizeWindow = miOverlayResizeWindow;
    pScreen->MarkWindow = miOverlayMarkWindow;
    pScreen->ClearToBackground = miOverlayClearToBackground;
    pScreen->SetShape = miOverlaySetShape;
    pScreen->ChangeBorderWidth = miOverlayChangeBorderWidth;

    return TRUE;
}

static Bool
miOverlayCloseScreen(ScreenPtr pScreen)
{
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->CreateWindow = pScreenPriv->CreateWindow;
    pScreen->DestroyWindow = pScreenPriv->DestroyWindow;
    pScreen->UnrealizeWindow = pScreenPriv->UnrealizeWindow;
    pScreen->RealizeWindow = pScreenPriv->RealizeWindow;

    free(pScreenPriv);

    return (*pScreen->CloseScreen) (pScreen);
}

static Bool
miOverlayCreateWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    miOverlayWindowPtr pWinPriv = MIOVERLAY_GET_WINDOW_PRIVATE(pWin);
    miOverlayTreePtr pTree = NULL;
    Bool result = TRUE;

    pWinPriv->tree = NULL;

    if (!pWin->parent || !((*pScreenPriv->InOverlay) (pWin))) {
        if (!(pTree = (miOverlayTreePtr) calloc(1, sizeof(miOverlayTreeRec))))
            return FALSE;
    }

    if (pScreenPriv->CreateWindow) {
        pScreen->CreateWindow = pScreenPriv->CreateWindow;
        result = (*pScreen->CreateWindow) (pWin);
        pScreen->CreateWindow = miOverlayCreateWindow;
    }

    if (pTree) {
        if (result) {
            pTree->pWin = pWin;
            pTree->visibility = VisibilityNotViewable;
            pWinPriv->tree = pTree;
            if (pWin->parent) {
                RegionNull(&(pTree->borderClip));
                RegionNull(&(pTree->clipList));
                RebuildTree(pWin);
            }
            else {
                BoxRec fullBox;

                fullBox.x1 = 0;
                fullBox.y1 = 0;
                fullBox.x2 = pScreen->width;
                fullBox.y2 = pScreen->height;
                RegionInit(&(pTree->borderClip), &fullBox, 1);
                RegionInit(&(pTree->clipList), &fullBox, 1);
            }
        }
        else
            free(pTree);
    }

    return TRUE;
}

static Bool
miOverlayDestroyWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    Bool result = TRUE;

    if (pTree) {
        if (pTree->prevSib)
            pTree->prevSib->nextSib = pTree->nextSib;
        else if (pTree->parent)
            pTree->parent->firstChild = pTree->nextSib;

        if (pTree->nextSib)
            pTree->nextSib->prevSib = pTree->prevSib;
        else if (pTree->parent)
            pTree->parent->lastChild = pTree->prevSib;

        RegionUninit(&(pTree->borderClip));
        RegionUninit(&(pTree->clipList));
        free(pTree);
    }

    if (pScreenPriv->DestroyWindow) {
        pScreen->DestroyWindow = pScreenPriv->DestroyWindow;
        result = (*pScreen->DestroyWindow) (pWin);
        pScreen->DestroyWindow = miOverlayDestroyWindow;
    }

    return result;
}

static Bool
miOverlayUnrealizeWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    Bool result = TRUE;

    if (pTree)
        pTree->visibility = VisibilityNotViewable;

    if (pScreenPriv->UnrealizeWindow) {
        pScreen->UnrealizeWindow = pScreenPriv->UnrealizeWindow;
        result = (*pScreen->UnrealizeWindow) (pWin);
        pScreen->UnrealizeWindow = miOverlayUnrealizeWindow;
    }

    return result;
}

static Bool
miOverlayRealizeWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    Bool result = TRUE;

    if (pScreenPriv->RealizeWindow) {
        pScreen->RealizeWindow = pScreenPriv->RealizeWindow;
        result = (*pScreen->RealizeWindow) (pWin);
        pScreen->RealizeWindow = miOverlayRealizeWindow;
    }

    /* we only need to catch the root window realization */

    if (result && !pWin->parent && !((*pScreenPriv->InOverlay) (pWin))) {
        BoxRec box;

        box.x1 = box.y1 = 0;
        box.x2 = pWin->drawable.width;
        box.y2 = pWin->drawable.height;
        (*pScreenPriv->MakeTransparent) (pScreen, 1, &box);
    }

    return result;
}

static void
miOverlayReparentWindow(WindowPtr pWin, WindowPtr pPriorParent)
{
    if (IN_UNDERLAY(pWin) || HasUnderlayChildren(pWin)) {
        /* This could probably be more optimal */
        RebuildTree(pWin->drawable.pScreen->root->firstChild);
    }
}

static void
miOverlayRestackWindow(WindowPtr pWin, WindowPtr oldNextSib)
{
    if (IN_UNDERLAY(pWin) || HasUnderlayChildren(pWin)) {
        /* This could probably be more optimal */
        RebuildTree(pWin);
    }
}

static Bool
miOverlayMarkOverlappedWindows(WindowPtr pWin,
                               WindowPtr pFirst, WindowPtr *pLayerWin)
{
    WindowPtr pChild, pLast;
    Bool overMarked, underMarked, doUnderlay, markAll;
    miOverlayTreePtr pTree = NULL, tLast, tChild;
    BoxPtr box;

    overMarked = underMarked = markAll = FALSE;

    if (pLayerWin)
        *pLayerWin = pWin;      /* hah! */

    doUnderlay = (IN_UNDERLAY(pWin) || HasUnderlayChildren(pWin));

    box = RegionExtents(&pWin->borderSize);

    if ((pChild = pFirst)) {
        pLast = pChild->parent->lastChild;
        while (1) {
            if (pChild == pWin)
                markAll = TRUE;

            if (doUnderlay && IN_UNDERLAY(pChild))
                pTree = MIOVERLAY_GET_WINDOW_TREE(pChild);

            if (pChild->viewable) {
                if (RegionBroken(&pChild->winSize))
                    SetWinSize(pChild);
                if (RegionBroken(&pChild->borderSize))
                    SetBorderSize(pChild);

                if (markAll || RegionContainsRect(&pChild->borderSize, box)) {
                    MARK_OVERLAY(pChild);
                    overMarked = TRUE;
                    if (doUnderlay && IN_UNDERLAY(pChild)) {
                        MARK_UNDERLAY(pChild);
                        underMarked = TRUE;
                    }
                    if (pChild->firstChild) {
                        pChild = pChild->firstChild;
                        continue;
                    }
                }
            }
            while (!pChild->nextSib && (pChild != pLast)) {
                pChild = pChild->parent;
                if (doUnderlay && IN_UNDERLAY(pChild))
                    pTree = MIOVERLAY_GET_WINDOW_TREE(pChild);
            }

            if (pChild == pWin)
                markAll = FALSE;

            if (pChild == pLast)
                break;

            pChild = pChild->nextSib;
        }
        if (overMarked)
            MARK_OVERLAY(pWin->parent);
    }

    if (doUnderlay && !pTree) {
        if (!(pTree = MIOVERLAY_GET_WINDOW_TREE(pWin))) {
            pChild = pWin->lastChild;
            while (1) {
                if ((pTree = MIOVERLAY_GET_WINDOW_TREE(pChild)))
                    break;

                if (pChild->lastChild) {
                    pChild = pChild->lastChild;
                    continue;
                }

                while (!pChild->prevSib)
                    pChild = pChild->parent;

                pChild = pChild->prevSib;
            }
        }
    }

    if (pTree && pTree->nextSib) {
        tChild = pTree->parent->lastChild;
        tLast = pTree->nextSib;

        while (1) {
            if (tChild->pWin->viewable) {
                if (RegionBroken(&tChild->pWin->winSize))
                    SetWinSize(tChild->pWin);
                if (RegionBroken(&tChild->pWin->borderSize))
                    SetBorderSize(tChild->pWin);

                if (RegionContainsRect(&(tChild->pWin->borderSize), box)) {
                    MARK_UNDERLAY(tChild->pWin);
                    underMarked = TRUE;
                }
            }

            if (tChild->lastChild) {
                tChild = tChild->lastChild;
                continue;
            }

            while (!tChild->prevSib && (tChild != tLast))
                tChild = tChild->parent;

            if (tChild == tLast)
                break;

            tChild = tChild->prevSib;
        }
    }

    if (underMarked) {
        ScreenPtr pScreen = pWin->drawable.pScreen;

        MARK_UNDERLAY(pTree->parent->pWin);
        MIOVERLAY_GET_SCREEN_PRIVATE(pScreen)->underlayMarked = TRUE;
    }

    return underMarked || overMarked;
}

static void
miOverlayComputeClips(WindowPtr pParent,
                      RegionPtr universe, VTKind kind, RegionPtr exposed)
{
    ScreenPtr pScreen = pParent->drawable.pScreen;
    int oldVis, newVis, dx, dy;
    BoxRec borderSize;
    RegionPtr borderVisible;
    RegionRec childUniverse, childUnion;
    miOverlayTreePtr tParent = MIOVERLAY_GET_WINDOW_TREE(pParent);
    miOverlayTreePtr tChild;
    Bool overlap;

    borderSize.x1 = pParent->drawable.x - wBorderWidth(pParent);
    borderSize.y1 = pParent->drawable.y - wBorderWidth(pParent);
    dx = (int) pParent->drawable.x + (int) pParent->drawable.width +
        wBorderWidth(pParent);
    if (dx > 32767)
        dx = 32767;
    borderSize.x2 = dx;
    dy = (int) pParent->drawable.y + (int) pParent->drawable.height +
        wBorderWidth(pParent);
    if (dy > 32767)
        dy = 32767;
    borderSize.y2 = dy;

    oldVis = tParent->visibility;
    switch (RegionContainsRect(universe, &borderSize)) {
    case rgnIN:
        newVis = VisibilityUnobscured;
        break;
    case rgnPART:
        newVis = VisibilityPartiallyObscured;
        {
            RegionPtr pBounding;

            if ((pBounding = wBoundingShape(pParent))) {
                switch (miShapedWindowIn(universe, pBounding,
                                         &borderSize,
                                         pParent->drawable.x,
                                         pParent->drawable.y)) {
                case rgnIN:
                    newVis = VisibilityUnobscured;
                    break;
                case rgnOUT:
                    newVis = VisibilityFullyObscured;
                    break;
                }
            }
        }
        break;
    default:
        newVis = VisibilityFullyObscured;
        break;
    }
    tParent->visibility = newVis;

    dx = pParent->drawable.x - tParent->valdata->oldAbsCorner.x;
    dy = pParent->drawable.y - tParent->valdata->oldAbsCorner.y;

    switch (kind) {
    case VTMap:
    case VTStack:
    case VTUnmap:
        break;
    case VTMove:
        if ((oldVis == newVis) &&
            ((oldVis == VisibilityFullyObscured) ||
             (oldVis == VisibilityUnobscured))) {
            tChild = tParent;
            while (1) {
                if (tChild->pWin->viewable) {
                    if (tChild->visibility != VisibilityFullyObscured) {
                        RegionTranslate(&tChild->borderClip, dx, dy);
                        RegionTranslate(&tChild->clipList, dx, dy);

                        tChild->pWin->drawable.serialNumber =
                            NEXT_SERIAL_NUMBER;
                        if (pScreen->ClipNotify)
                            (*pScreen->ClipNotify) (tChild->pWin, dx, dy);
                    }
                    if (tChild->valdata) {
                        RegionNull(&tChild->valdata->borderExposed);
                        if (HasParentRelativeBorder(tChild->pWin)) {
                            RegionSubtract(&tChild->valdata->borderExposed,
                                           &tChild->borderClip,
                                           &tChild->pWin->winSize);
                        }
                        RegionNull(&tChild->valdata->exposed);
                    }
                    if (tChild->firstChild) {
                        tChild = tChild->firstChild;
                        continue;
                    }
                }
                while (!tChild->nextSib && (tChild != tParent))
                    tChild = tChild->parent;
                if (tChild == tParent)
                    break;
                tChild = tChild->nextSib;
            }
            return;
        }
        /* fall through */
    default:
        if (dx || dy) {
            RegionTranslate(&tParent->borderClip, dx, dy);
            RegionTranslate(&tParent->clipList, dx, dy);
        }
        break;
    case VTBroken:
        RegionEmpty(&tParent->borderClip);
        RegionEmpty(&tParent->clipList);
        break;
    }

    borderVisible = tParent->valdata->borderVisible;
    RegionNull(&tParent->valdata->borderExposed);
    RegionNull(&tParent->valdata->exposed);

    if (HasBorder(pParent)) {
        if (borderVisible) {
            RegionSubtract(exposed, universe, borderVisible);
            RegionDestroy(borderVisible);
        }
        else
            RegionSubtract(exposed, universe, &tParent->borderClip);

        if (HasParentRelativeBorder(pParent) && (dx || dy))
            RegionSubtract(&tParent->valdata->borderExposed,
                           universe, &pParent->winSize);
        else
            RegionSubtract(&tParent->valdata->borderExposed,
                           exposed, &pParent->winSize);

        RegionCopy(&tParent->borderClip, universe);
        RegionIntersect(universe, universe, &pParent->winSize);
    }
    else
        RegionCopy(&tParent->borderClip, universe);

    if ((tChild = tParent->firstChild) && pParent->mapped) {
        RegionNull(&childUniverse);
        RegionNull(&childUnion);

        for (; tChild; tChild = tChild->nextSib) {
            if (tChild->pWin->viewable)
                RegionAppend(&childUnion, &tChild->pWin->borderSize);
        }

        RegionValidate(&childUnion, &overlap);

        for (tChild = tParent->firstChild; tChild; tChild = tChild->nextSib) {
            if (tChild->pWin->viewable) {
                if (tChild->valdata) {
                    RegionIntersect(&childUniverse, universe,
                                    &tChild->pWin->borderSize);
                    miOverlayComputeClips(tChild->pWin, &childUniverse,
                                          kind, exposed);
                }
                if (overlap)
                    RegionSubtract(universe, universe,
                                   &tChild->pWin->borderSize);
            }
        }
        if (!overlap)
            RegionSubtract(universe, universe, &childUnion);
        RegionUninit(&childUnion);
        RegionUninit(&childUniverse);
    }

    if (oldVis == VisibilityFullyObscured || oldVis == VisibilityNotViewable) {
        RegionCopy(&tParent->valdata->exposed, universe);
    }
    else if (newVis != VisibilityFullyObscured &&
             newVis != VisibilityNotViewable) {
        RegionSubtract(&tParent->valdata->exposed,
                       universe, &tParent->clipList);
    }

    /* HACK ALERT - copying contents of regions, instead of regions */
    {
        RegionRec tmp;

        tmp = tParent->clipList;
        tParent->clipList = *universe;
        *universe = tmp;
    }

    pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    if (pScreen->ClipNotify)
        (*pScreen->ClipNotify) (pParent, dx, dy);
}

static void
miOverlayMarkWindow(WindowPtr pWin)
{
    miOverlayTreePtr pTree = NULL;
    WindowPtr pChild, pGrandChild;

    miMarkWindow(pWin);

    /* look for UnmapValdata among immediate children */

    if (!(pChild = pWin->firstChild))
        return;

    for (; pChild; pChild = pChild->nextSib) {
        if (pChild->valdata == UnmapValData) {
            if (IN_UNDERLAY(pChild)) {
                pTree = MIOVERLAY_GET_WINDOW_TREE(pChild);
                pTree->valdata = (miOverlayValDataPtr) UnmapValData;
                continue;
            }
            else {
                if (!(pGrandChild = pChild->firstChild))
                    continue;

                while (1) {
                    if (IN_UNDERLAY(pGrandChild)) {
                        pTree = MIOVERLAY_GET_WINDOW_TREE(pGrandChild);
                        pTree->valdata = (miOverlayValDataPtr) UnmapValData;
                    }
                    else if (pGrandChild->firstChild) {
                        pGrandChild = pGrandChild->firstChild;
                        continue;
                    }

                    while (!pGrandChild->nextSib && (pGrandChild != pChild))
                        pGrandChild = pGrandChild->parent;

                    if (pChild == pGrandChild)
                        break;

                    pGrandChild = pGrandChild->nextSib;
                }
            }
        }
    }

    if (pTree) {
        MARK_UNDERLAY(pTree->parent->pWin);
        MIOVERLAY_GET_SCREEN_PRIVATE(pWin->drawable.pScreen)->underlayMarked =
            TRUE;
    }
}

static void
miOverlayMarkUnrealizedWindow(WindowPtr pChild,
                              WindowPtr pWin, Bool fromConfigure)
{
    if ((pChild != pWin) || fromConfigure) {
        miOverlayTreePtr pTree;

        RegionEmpty(&pChild->clipList);
        if (pChild->drawable.pScreen->ClipNotify)
            (*pChild->drawable.pScreen->ClipNotify) (pChild, 0, 0);
        RegionEmpty(&pChild->borderClip);
        if ((pTree = MIOVERLAY_GET_WINDOW_TREE(pChild))) {
            if (pTree->valdata != (miOverlayValDataPtr) UnmapValData) {
                RegionEmpty(&pTree->clipList);
                RegionEmpty(&pTree->borderClip);
            }
        }
    }
}

static int
miOverlayValidateTree(WindowPtr pParent, WindowPtr pChild,      /* first child effected */
                      VTKind kind)
{
    ScreenPtr pScreen = pParent->drawable.pScreen;
    miOverlayScreenPtr pPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    RegionRec totalClip, childClip, exposed;
    miOverlayTreePtr tParent, tChild, tWin;
    Bool overlap;
    WindowPtr newParent;

    if (!pPriv->underlayMarked)
        goto SKIP_UNDERLAY;

    if (!pChild)
        pChild = pParent->firstChild;

    RegionNull(&totalClip);
    RegionNull(&childClip);
    RegionNull(&exposed);

    newParent = pParent;

    while (IN_OVERLAY(newParent))
        newParent = newParent->parent;

    tParent = MIOVERLAY_GET_WINDOW_TREE(newParent);

    if (IN_UNDERLAY(pChild))
        tChild = MIOVERLAY_GET_WINDOW_TREE(pChild);
    else
        tChild = tParent->firstChild;

    if (RegionBroken(&tParent->clipList) && !RegionBroken(&tParent->borderClip)) {
        kind = VTBroken;
        RegionCopy(&totalClip, &tParent->borderClip);
        RegionIntersect(&totalClip, &totalClip, &tParent->pWin->winSize);

        for (tWin = tParent->firstChild; tWin != tChild; tWin = tWin->nextSib) {
            if (tWin->pWin->viewable)
                RegionSubtract(&totalClip, &totalClip, &tWin->pWin->borderSize);
        }
        RegionEmpty(&tParent->clipList);
    }
    else {
        for (tWin = tChild; tWin; tWin = tWin->nextSib) {
            if (tWin->valdata)
                RegionAppend(&totalClip, &tWin->borderClip);
        }
        RegionValidate(&totalClip, &overlap);
    }

    if (kind != VTStack)
        RegionUnion(&totalClip, &totalClip, &tParent->clipList);

    for (tWin = tChild; tWin; tWin = tWin->nextSib) {
        if (tWin->valdata) {
            if (tWin->pWin->viewable) {
                RegionIntersect(&childClip, &totalClip,
                                &tWin->pWin->borderSize);
                miOverlayComputeClips(tWin->pWin, &childClip, kind, &exposed);
                RegionSubtract(&totalClip, &totalClip, &tWin->pWin->borderSize);
            }
            else {              /* Means we are unmapping */
                RegionEmpty(&tWin->clipList);
                RegionEmpty(&tWin->borderClip);
                tWin->valdata = NULL;
            }
        }
    }

    RegionUninit(&childClip);

    if (!((*pPriv->InOverlay) (newParent))) {
        RegionNull(&tParent->valdata->exposed);
        RegionNull(&tParent->valdata->borderExposed);
    }

    switch (kind) {
    case VTStack:
        break;
    default:
        if (!((*pPriv->InOverlay) (newParent)))
            RegionSubtract(&tParent->valdata->exposed, &totalClip,
                           &tParent->clipList);
        /* fall through */
    case VTMap:
        RegionCopy(&tParent->clipList, &totalClip);
        if (!((*pPriv->InOverlay) (newParent)))
            newParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        break;
    }

    RegionUninit(&totalClip);
    RegionUninit(&exposed);

 SKIP_UNDERLAY:

    miValidateTree(pParent, pChild, kind);

    return 1;
}

static void
miOverlayHandleExposures(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    WindowPtr pChild;
    ValidatePtr val;
    WindowExposuresProcPtr WindowExposures;

    WindowExposures = pWin->drawable.pScreen->WindowExposures;
    if (pPriv->underlayMarked) {
        miOverlayTreePtr pTree;
        miOverlayValDataPtr mival;

        pChild = pWin;
        while (IN_OVERLAY(pChild))
            pChild = pChild->parent;

        pTree = MIOVERLAY_GET_WINDOW_TREE(pChild);

        while (1) {
            if ((mival = pTree->valdata)) {
                if (!((*pPriv->InOverlay) (pTree->pWin))) {
                    if (RegionNotEmpty(&mival->borderExposed)) {
                        pScreen->PaintWindow(pTree->pWin, &mival->borderExposed,
                                             PW_BORDER);
                    }
                    RegionUninit(&mival->borderExposed);

                    (*WindowExposures) (pTree->pWin, &mival->exposed);
                    RegionUninit(&mival->exposed);
                }
                free(mival);
                pTree->valdata = NULL;
                if (pTree->firstChild) {
                    pTree = pTree->firstChild;
                    continue;
                }
            }
            while (!pTree->nextSib && (pTree->pWin != pChild))
                pTree = pTree->parent;
            if (pTree->pWin == pChild)
                break;
            pTree = pTree->nextSib;
        }
        pPriv->underlayMarked = FALSE;
    }

    pChild = pWin;
    while (1) {
        if ((val = pChild->valdata)) {
            if (!((*pPriv->InOverlay) (pChild))) {
                RegionUnion(&val->after.exposed, &val->after.exposed,
                            &val->after.borderExposed);

                if (RegionNotEmpty(&val->after.exposed)) {
                    (*(MIOVERLAY_GET_SCREEN_PRIVATE(pScreen)->MakeTransparent))
                        (pScreen, RegionNumRects(&val->after.exposed),
                         RegionRects(&val->after.exposed));
                }
            }
            else {
                if (RegionNotEmpty(&val->after.borderExposed)) {
                    pScreen->PaintWindow(pChild, &val->after.borderExposed,
                                         PW_BORDER);
                }
                (*WindowExposures) (pChild, &val->after.exposed);
            }
            RegionUninit(&val->after.borderExposed);
            RegionUninit(&val->after.exposed);
            free(val);
            pChild->valdata = NULL;
            if (pChild->firstChild) {
                pChild = pChild->firstChild;
                continue;
            }
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            break;
        pChild = pChild->nextSib;
    }
}

static void
miOverlayMoveWindow(WindowPtr pWin,
                    int x, int y, WindowPtr pNextSib, VTKind kind)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    WindowPtr pParent, windowToValidate;
    Bool WasViewable = (Bool) (pWin->viewable);
    short bw;
    RegionRec overReg, underReg;
    DDXPointRec oldpt;

    if (!(pParent = pWin->parent))
        return;
    bw = wBorderWidth(pWin);

    oldpt.x = pWin->drawable.x;
    oldpt.y = pWin->drawable.y;
    if (WasViewable) {
        RegionNull(&overReg);
        RegionNull(&underReg);
        if (pTree) {
            RegionCopy(&overReg, &pWin->borderClip);
            RegionCopy(&underReg, &pTree->borderClip);
        }
        else {
            RegionCopy(&overReg, &pWin->borderClip);
            CollectUnderlayChildrenRegions(pWin, &underReg);
        }
        (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);
    }
    pWin->origin.x = x + (int) bw;
    pWin->origin.y = y + (int) bw;
    x = pWin->drawable.x = pParent->drawable.x + x + (int) bw;
    y = pWin->drawable.y = pParent->drawable.y + y + (int) bw;

    SetWinSize(pWin);
    SetBorderSize(pWin);

    (*pScreen->PositionWindow) (pWin, x, y);

    windowToValidate = MoveWindowInStack(pWin, pNextSib);

    ResizeChildrenWinSize(pWin, x - oldpt.x, y - oldpt.y, 0, 0);

    if (WasViewable) {
        miOverlayScreenPtr pPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);

        (*pScreen->MarkOverlappedWindows) (pWin, windowToValidate, NULL);

        (*pScreen->ValidateTree) (pWin->parent, NullWindow, kind);
        if (RegionNotEmpty(&underReg)) {
            pPriv->copyUnderlay = TRUE;
            (*pWin->drawable.pScreen->CopyWindow) (pWin, oldpt, &underReg);
        }
        RegionUninit(&underReg);
        if (RegionNotEmpty(&overReg)) {
            pPriv->copyUnderlay = FALSE;
            (*pWin->drawable.pScreen->CopyWindow) (pWin, oldpt, &overReg);
        }
        RegionUninit(&overReg);
        (*pScreen->HandleExposures) (pWin->parent);

        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (pWin->parent, NullWindow, kind);
    }
    if (pWin->realized)
        WindowsRestructured();
}

#ifndef RECTLIMIT
#define RECTLIMIT 25
#endif

static void
miOverlayWindowExposures(WindowPtr pWin, RegionPtr prgn)
{
    RegionPtr exposures = prgn;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (prgn && !RegionNil(prgn)) {
        RegionRec expRec;
        int clientInterested =
            (pWin->eventMask | wOtherEventMasks(pWin)) & ExposureMask;
        if (clientInterested && (RegionNumRects(prgn) > RECTLIMIT)) {
            miOverlayScreenPtr pPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
            BoxRec box;

            box = *RegionExtents(prgn);
            exposures = &expRec;
            RegionInit(exposures, &box, 1);
            RegionReset(prgn, &box);
            /* This is the only reason why we are replacing mi's version
               of this file */

            if (!((*pPriv->InOverlay) (pWin))) {
                miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

                RegionIntersect(prgn, prgn, &pTree->clipList);
            }
            else
                RegionIntersect(prgn, prgn, &pWin->clipList);
        }
        pScreen->PaintWindow(pWin, prgn, PW_BACKGROUND);
        if (clientInterested)
            miSendExposures(pWin, exposures,
                            pWin->drawable.x, pWin->drawable.y);
        if (exposures == &expRec)
            RegionUninit(exposures);
        RegionEmpty(prgn);
    }
}

typedef struct {
    RegionPtr over;
    RegionPtr under;
} miOverlayTwoRegions;

static int
miOverlayRecomputeExposures(WindowPtr pWin, void *value)
{
    miOverlayTwoRegions *pValid = (miOverlayTwoRegions *) value;
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

    if (pWin->valdata) {
        /*
         * compute exposed regions of this window
         */
        RegionSubtract(&pWin->valdata->after.exposed,
                       &pWin->clipList, pValid->over);
        /*
         * compute exposed regions of the border
         */
        RegionSubtract(&pWin->valdata->after.borderExposed,
                       &pWin->borderClip, &pWin->winSize);
        RegionSubtract(&pWin->valdata->after.borderExposed,
                       &pWin->valdata->after.borderExposed, pValid->over);
    }

    if (pTree && pTree->valdata) {
        RegionSubtract(&pTree->valdata->exposed,
                       &pTree->clipList, pValid->under);
        RegionSubtract(&pTree->valdata->borderExposed,
                       &pTree->borderClip, &pWin->winSize);
        RegionSubtract(&pTree->valdata->borderExposed,
                       &pTree->valdata->borderExposed, pValid->under);
    }
    else if (!pWin->valdata)
        return WT_NOMATCH;

    return WT_WALKCHILDREN;
}

static void
miOverlayResizeWindow(WindowPtr pWin,
                      int x, int y,
                      unsigned int w, unsigned int h, WindowPtr pSib)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pParent;
    miOverlayTreePtr tChild, pTree;
    Bool WasViewable = (Bool) (pWin->viewable);
    unsigned short width = pWin->drawable.width;
    unsigned short height = pWin->drawable.height;
    short oldx = pWin->drawable.x;
    short oldy = pWin->drawable.y;
    int bw = wBorderWidth(pWin);
    short dw, dh;
    DDXPointRec oldpt;
    RegionPtr oldRegion = NULL, oldRegion2 = NULL;
    WindowPtr pFirstChange;
    WindowPtr pChild;
    RegionPtr gravitate[StaticGravity + 1];
    RegionPtr gravitate2[StaticGravity + 1];
    unsigned g;
    int nx, ny;                 /* destination x,y */
    int newx, newy;             /* new inner window position */
    RegionPtr pRegion = NULL;
    RegionPtr destClip, destClip2;
    RegionPtr oldWinClip = NULL, oldWinClip2 = NULL;
    RegionPtr borderVisible = NullRegion;
    RegionPtr borderVisible2 = NullRegion;
    Bool shrunk = FALSE;        /* shrunk in an inner dimension */
    Bool moved = FALSE;         /* window position changed */
    Bool doUnderlay;

    /* if this is a root window, can't be resized */
    if (!(pParent = pWin->parent))
        return;

    pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    doUnderlay = ((pTree) || HasUnderlayChildren(pWin));
    newx = pParent->drawable.x + x + bw;
    newy = pParent->drawable.y + y + bw;
    if (WasViewable) {
        /*
         * save the visible region of the window
         */
        oldRegion = RegionCreate(NullBox, 1);
        RegionCopy(oldRegion, &pWin->winSize);
        if (doUnderlay) {
            oldRegion2 = RegionCreate(NullBox, 1);
            RegionCopy(oldRegion2, &pWin->winSize);
        }

        /*
         * categorize child windows into regions to be moved
         */
        for (g = 0; g <= StaticGravity; g++)
            gravitate[g] = gravitate2[g] = NULL;
        for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
            g = pChild->winGravity;
            if (g != UnmapGravity) {
                if (!gravitate[g])
                    gravitate[g] = RegionCreate(NullBox, 1);
                RegionUnion(gravitate[g], gravitate[g], &pChild->borderClip);

                if (doUnderlay) {
                    if (!gravitate2[g])
                        gravitate2[g] = RegionCreate(NullBox, 0);

                    if ((tChild = MIOVERLAY_GET_WINDOW_TREE(pChild))) {
                        RegionUnion(gravitate2[g],
                                    gravitate2[g], &tChild->borderClip);
                    }
                    else
                        CollectUnderlayChildrenRegions(pChild, gravitate2[g]);
                }
            }
            else {
                UnmapWindow(pChild, TRUE);
            }
        }
        (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);

        oldWinClip = oldWinClip2 = NULL;
        if (pWin->bitGravity != ForgetGravity) {
            oldWinClip = RegionCreate(NullBox, 1);
            RegionCopy(oldWinClip, &pWin->clipList);
            if (pTree) {
                oldWinClip2 = RegionCreate(NullBox, 1);
                RegionCopy(oldWinClip2, &pTree->clipList);
            }
        }
        /*
         * if the window is changing size, borderExposed
         * can't be computed correctly without some help.
         */
        if (pWin->drawable.height > h || pWin->drawable.width > w)
            shrunk = TRUE;

        if (newx != oldx || newy != oldy)
            moved = TRUE;

        if ((pWin->drawable.height != h || pWin->drawable.width != w) &&
            HasBorder(pWin)) {
            borderVisible = RegionCreate(NullBox, 1);
            if (pTree)
                borderVisible2 = RegionCreate(NullBox, 1);
            /* for tiled borders, we punt and draw the whole thing */
            if (pWin->borderIsPixel || !moved) {
                if (shrunk || moved)
                    RegionSubtract(borderVisible,
                                   &pWin->borderClip, &pWin->winSize);
                else
                    RegionCopy(borderVisible, &pWin->borderClip);
                if (pTree) {
                    if (shrunk || moved)
                        RegionSubtract(borderVisible,
                                       &pTree->borderClip, &pWin->winSize);
                    else
                        RegionCopy(borderVisible, &pTree->borderClip);
                }
            }
        }
    }
    pWin->origin.x = x + bw;
    pWin->origin.y = y + bw;
    pWin->drawable.height = h;
    pWin->drawable.width = w;

    x = pWin->drawable.x = newx;
    y = pWin->drawable.y = newy;

    SetWinSize(pWin);
    SetBorderSize(pWin);

    dw = (int) w - (int) width;
    dh = (int) h - (int) height;
    ResizeChildrenWinSize(pWin, x - oldx, y - oldy, dw, dh);

    /* let the hardware adjust background and border pixmaps, if any */
    (*pScreen->PositionWindow) (pWin, x, y);

    pFirstChange = MoveWindowInStack(pWin, pSib);

    if (WasViewable) {
        pRegion = RegionCreate(NullBox, 1);

        (*pScreen->MarkOverlappedWindows) (pWin, pFirstChange, NULL);

        pWin->valdata->before.resized = TRUE;
        pWin->valdata->before.borderVisible = borderVisible;
        if (pTree)
            pTree->valdata->borderVisible = borderVisible2;

        (*pScreen->ValidateTree) (pWin->parent, pFirstChange, VTOther);
        /*
         * the entire window is trashed unless bitGravity
         * recovers portions of it
         */
        RegionCopy(&pWin->valdata->after.exposed, &pWin->clipList);
        if (pTree)
            RegionCopy(&pTree->valdata->exposed, &pTree->clipList);
    }

    GravityTranslate(x, y, oldx, oldy, dw, dh, pWin->bitGravity, &nx, &ny);

    if (WasViewable) {
        miOverlayScreenPtr pPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
        miOverlayTwoRegions TwoRegions;

        /* avoid the border */
        if (HasBorder(pWin)) {
            int offx, offy, dx, dy;

            /* kruft to avoid double translates for each gravity */
            offx = 0;
            offy = 0;
            for (g = 0; g <= StaticGravity; g++) {
                if (!gravitate[g] && !gravitate2[g])
                    continue;

                /* align winSize to gravitate[g].
                 * winSize is in new coordinates,
                 * gravitate[g] is still in old coordinates */
                GravityTranslate(x, y, oldx, oldy, dw, dh, g, &nx, &ny);

                dx = (oldx - nx) - offx;
                dy = (oldy - ny) - offy;
                if (dx || dy) {
                    RegionTranslate(&pWin->winSize, dx, dy);
                    offx += dx;
                    offy += dy;
                }
                if (gravitate[g])
                    RegionIntersect(gravitate[g], gravitate[g], &pWin->winSize);
                if (gravitate2[g])
                    RegionIntersect(gravitate2[g], gravitate2[g],
                                    &pWin->winSize);
            }
            /* get winSize back where it belongs */
            if (offx || offy)
                RegionTranslate(&pWin->winSize, -offx, -offy);
        }
        /*
         * add screen bits to the appropriate bucket
         */

        if (oldWinClip2) {
            RegionCopy(pRegion, oldWinClip2);
            RegionTranslate(pRegion, nx - oldx, ny - oldy);
            RegionIntersect(oldWinClip2, pRegion, &pTree->clipList);

            for (g = pWin->bitGravity + 1; g <= StaticGravity; g++) {
                if (gravitate2[g])
                    RegionSubtract(oldWinClip2, oldWinClip2, gravitate2[g]);
            }
            RegionTranslate(oldWinClip2, oldx - nx, oldy - ny);
            g = pWin->bitGravity;
            if (!gravitate2[g])
                gravitate2[g] = oldWinClip2;
            else {
                RegionUnion(gravitate2[g], gravitate2[g], oldWinClip2);
                RegionDestroy(oldWinClip2);
            }
        }

        if (oldWinClip) {
            /*
             * clip to new clipList
             */
            RegionCopy(pRegion, oldWinClip);
            RegionTranslate(pRegion, nx - oldx, ny - oldy);
            RegionIntersect(oldWinClip, pRegion, &pWin->clipList);
            /*
             * don't step on any gravity bits which will be copied after this
             * region.  Note -- this assumes that the regions will be copied
             * in gravity order.
             */
            for (g = pWin->bitGravity + 1; g <= StaticGravity; g++) {
                if (gravitate[g])
                    RegionSubtract(oldWinClip, oldWinClip, gravitate[g]);
            }
            RegionTranslate(oldWinClip, oldx - nx, oldy - ny);
            g = pWin->bitGravity;
            if (!gravitate[g])
                gravitate[g] = oldWinClip;
            else {
                RegionUnion(gravitate[g], gravitate[g], oldWinClip);
                RegionDestroy(oldWinClip);
            }
        }

        /*
         * move the bits on the screen
         */

        destClip = destClip2 = NULL;

        for (g = 0; g <= StaticGravity; g++) {
            if (!gravitate[g] && !gravitate2[g])
                continue;

            GravityTranslate(x, y, oldx, oldy, dw, dh, g, &nx, &ny);

            oldpt.x = oldx + (x - nx);
            oldpt.y = oldy + (y - ny);

            /* Note that gravitate[g] is *translated* by CopyWindow */

            /* only copy the remaining useful bits */

            if (gravitate[g])
                RegionIntersect(gravitate[g], gravitate[g], oldRegion);
            if (gravitate2[g])
                RegionIntersect(gravitate2[g], gravitate2[g], oldRegion2);

            /* clip to not overwrite already copied areas */

            if (destClip && gravitate[g]) {
                RegionTranslate(destClip, oldpt.x - x, oldpt.y - y);
                RegionSubtract(gravitate[g], gravitate[g], destClip);
                RegionTranslate(destClip, x - oldpt.x, y - oldpt.y);
            }
            if (destClip2 && gravitate2[g]) {
                RegionTranslate(destClip2, oldpt.x - x, oldpt.y - y);
                RegionSubtract(gravitate2[g], gravitate2[g], destClip2);
                RegionTranslate(destClip2, x - oldpt.x, y - oldpt.y);
            }

            /* and move those bits */

            if (oldpt.x != x || oldpt.y != y) {
                if (gravitate2[g]) {
                    pPriv->copyUnderlay = TRUE;
                    (*pScreen->CopyWindow) (pWin, oldpt, gravitate2[g]);
                }
                if (gravitate[g]) {
                    pPriv->copyUnderlay = FALSE;
                    (*pScreen->CopyWindow) (pWin, oldpt, gravitate[g]);
                }
            }

            /* remove any overwritten bits from the remaining useful bits */

            if (gravitate[g])
                RegionSubtract(oldRegion, oldRegion, gravitate[g]);
            if (gravitate2[g])
                RegionSubtract(oldRegion2, oldRegion2, gravitate2[g]);

            /*
             * recompute exposed regions of child windows
             */

            for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
                if (pChild->winGravity != g)
                    continue;

                TwoRegions.over = gravitate[g];
                TwoRegions.under = gravitate2[g];

                TraverseTree(pChild, miOverlayRecomputeExposures,
                             (void *) (&TwoRegions));
            }

            /*
             * remove the successfully copied regions of the
             * window from its exposed region
             */

            if (g == pWin->bitGravity) {
                if (gravitate[g])
                    RegionSubtract(&pWin->valdata->after.exposed,
                                   &pWin->valdata->after.exposed, gravitate[g]);
                if (gravitate2[g] && pTree)
                    RegionSubtract(&pTree->valdata->exposed,
                                   &pTree->valdata->exposed, gravitate2[g]);
            }
            if (gravitate[g]) {
                if (!destClip)
                    destClip = gravitate[g];
                else {
                    RegionUnion(destClip, destClip, gravitate[g]);
                    RegionDestroy(gravitate[g]);
                }
            }
            if (gravitate2[g]) {
                if (!destClip2)
                    destClip2 = gravitate2[g];
                else {
                    RegionUnion(destClip2, destClip2, gravitate2[g]);
                    RegionDestroy(gravitate2[g]);
                }
            }
        }

        RegionDestroy(pRegion);
        RegionDestroy(oldRegion);
        if (doUnderlay)
            RegionDestroy(oldRegion2);
        if (destClip)
            RegionDestroy(destClip);
        if (destClip2)
            RegionDestroy(destClip2);
        (*pScreen->HandleExposures) (pWin->parent);
        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (pWin->parent, pFirstChange, VTOther);
    }
    if (pWin->realized)
        WindowsRestructured();
}

static void
miOverlaySetShape(WindowPtr pWin, int kind)
{
    Bool WasViewable = (Bool) (pWin->viewable);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (kind != ShapeInput) {
        if (WasViewable) {
            (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);

            if (HasBorder(pWin)) {
                RegionPtr borderVisible;

                borderVisible = RegionCreate(NullBox, 1);
                RegionSubtract(borderVisible,
                               &pWin->borderClip, &pWin->winSize);
                pWin->valdata->before.borderVisible = borderVisible;
                pWin->valdata->before.resized = TRUE;
                if (IN_UNDERLAY(pWin)) {
                    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
                    RegionPtr borderVisible2;

                    borderVisible2 = RegionCreate(NULL, 1);
                    RegionSubtract(borderVisible2,
                                   &pTree->borderClip, &pWin->winSize);
                    pTree->valdata->borderVisible = borderVisible2;
                }
            }
        }

        SetWinSize(pWin);
        SetBorderSize(pWin);

        ResizeChildrenWinSize(pWin, 0, 0, 0, 0);

        if (WasViewable) {
            (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);
            (*pScreen->ValidateTree) (pWin->parent, NullWindow, VTOther);
            (*pScreen->HandleExposures) (pWin->parent);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pWin->parent, NullWindow,
                                              VTOther);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
    CheckCursorConfinement(pWin);
}

static void
miOverlayChangeBorderWidth(WindowPtr pWin, unsigned int width)
{
    int oldwidth;
    ScreenPtr pScreen;
    Bool WasViewable = (Bool) (pWin->viewable);
    Bool HadBorder;

    oldwidth = wBorderWidth(pWin);
    if (oldwidth == width)
        return;
    HadBorder = HasBorder(pWin);
    pScreen = pWin->drawable.pScreen;
    if (WasViewable && (width < oldwidth))
        (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);

    pWin->borderWidth = width;
    SetBorderSize(pWin);

    if (WasViewable) {
        if (width > oldwidth) {
            (*pScreen->MarkOverlappedWindows) (pWin, pWin, NULL);

            if (HadBorder) {
                RegionPtr borderVisible;

                borderVisible = RegionCreate(NULL, 1);
                RegionSubtract(borderVisible,
                               &pWin->borderClip, &pWin->winSize);
                pWin->valdata->before.borderVisible = borderVisible;
                if (IN_UNDERLAY(pWin)) {
                    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
                    RegionPtr borderVisible2;

                    borderVisible2 = RegionCreate(NULL, 1);
                    RegionSubtract(borderVisible2,
                                   &pTree->borderClip, &pWin->winSize);
                    pTree->valdata->borderVisible = borderVisible2;
                }
            }
        }
        (*pScreen->ValidateTree) (pWin->parent, pWin, VTOther);
        (*pScreen->HandleExposures) (pWin->parent);

        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (pWin->parent, pWin, VTOther);
    }
    if (pWin->realized)
        WindowsRestructured();
}

/*  We need this as an addition since the xf86 common code doesn't
    know about the second tree which is static to this file.  */

void
miOverlaySetRootClip(ScreenPtr pScreen, Bool enable)
{
    WindowPtr pRoot = pScreen->root;
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pRoot);

    MARK_UNDERLAY(pRoot);

    if (enable) {
        BoxRec box;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pScreen->width;
        box.y2 = pScreen->height;

        RegionReset(&pTree->borderClip, &box);
    }
    else
        RegionEmpty(&pTree->borderClip);

    RegionBreak(&pTree->clipList);
}

static void
miOverlayClearToBackground(WindowPtr pWin,
                           int x, int y, int w, int h, Bool generateExposures)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    BoxRec box;
    RegionRec reg;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    miOverlayScreenPtr pScreenPriv = MIOVERLAY_GET_SCREEN_PRIVATE(pScreen);
    RegionPtr clipList;
    BoxPtr extents;
    int x1, y1, x2, y2;

    x1 = pWin->drawable.x + x;
    y1 = pWin->drawable.y + y;
    if (w)
        x2 = x1 + (int) w;
    else
        x2 = x1 + (int) pWin->drawable.width - (int) x;
    if (h)
        y2 = y1 + h;
    else
        y2 = y1 + (int) pWin->drawable.height - (int) y;

    clipList = ((*pScreenPriv->InOverlay) (pWin)) ? &pWin->clipList :
        &pTree->clipList;

    extents = RegionExtents(clipList);

    if (x1 < extents->x1)
        x1 = extents->x1;
    if (x2 > extents->x2)
        x2 = extents->x2;
    if (y1 < extents->y1)
        y1 = extents->y1;
    if (y2 > extents->y2)
        y2 = extents->y2;

    if (x2 <= x1 || y2 <= y1)
        x2 = x1 = y2 = y1 = 0;

    box.x1 = x1;
    box.x2 = x2;
    box.y1 = y1;
    box.y2 = y2;

    RegionInit(&reg, &box, 1);

    RegionIntersect(&reg, &reg, clipList);
    if (generateExposures)
        (*pScreen->WindowExposures) (pWin, &reg);
    else if (pWin->backgroundState != None)
        pScreen->PaintWindow(pWin, &reg, PW_BACKGROUND);
    RegionUninit(&reg);
}

/****************************************************************/

/* not used */
Bool
miOverlayGetPrivateClips(WindowPtr pWin,
                         RegionPtr *borderClip, RegionPtr *clipList)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

    if (pTree) {
        *borderClip = &(pTree->borderClip);
        *clipList = &(pTree->clipList);
        return TRUE;
    }

    *borderClip = *clipList = NULL;

    return FALSE;
}

void
miOverlaySetTransFunction(ScreenPtr pScreen, miOverlayTransFunc transFunc)
{
    MIOVERLAY_GET_SCREEN_PRIVATE(pScreen)->MakeTransparent = transFunc;
}

Bool
miOverlayCopyUnderlay(ScreenPtr pScreen)
{
    return MIOVERLAY_GET_SCREEN_PRIVATE(pScreen)->copyUnderlay;
}

void
miOverlayComputeCompositeClip(GCPtr pGC, WindowPtr pWin)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);
    RegionPtr pregWin;
    Bool freeTmpClip, freeCompClip;

    if (!pTree) {
        miComputeCompositeClip(pGC, &pWin->drawable);
        return;
    }

    if (pGC->subWindowMode == IncludeInferiors) {
        pregWin = RegionCreate(NullBox, 1);
        freeTmpClip = TRUE;
        if (pWin->parent || (screenIsSaved != SCREEN_SAVER_ON) ||
            !HasSaverWindow(pGC->pScreen)) {
            RegionIntersect(pregWin, &pTree->borderClip, &pWin->winSize);
        }
    }
    else {
        pregWin = &pTree->clipList;
        freeTmpClip = FALSE;
    }
    freeCompClip = pGC->freeCompClip;
    if (!pGC->clientClip) {
        if (freeCompClip)
            RegionDestroy(pGC->pCompositeClip);
        pGC->pCompositeClip = pregWin;
        pGC->freeCompClip = freeTmpClip;
    }
    else {
        RegionTranslate(pGC->clientClip,
                        pWin->drawable.x + pGC->clipOrg.x,
                        pWin->drawable.y + pGC->clipOrg.y);

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
                        -(pWin->drawable.x + pGC->clipOrg.x),
                        -(pWin->drawable.y + pGC->clipOrg.y));
    }
}

Bool
miOverlayCollectUnderlayRegions(WindowPtr pWin, RegionPtr *region)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

    if (pTree) {
        *region = &pTree->borderClip;
        return FALSE;
    }

    *region = RegionCreate(NullBox, 0);

    CollectUnderlayChildrenRegions(pWin, *region);

    return TRUE;
}

static miOverlayTreePtr
DoLeaf(WindowPtr pWin, miOverlayTreePtr parent, miOverlayTreePtr prevSib)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

    pTree->parent = parent;
    pTree->firstChild = NULL;
    pTree->lastChild = NULL;
    pTree->prevSib = prevSib;
    pTree->nextSib = NULL;

    if (prevSib)
        prevSib->nextSib = pTree;

    if (!parent->firstChild)
        parent->firstChild = parent->lastChild = pTree;
    else if (parent->lastChild == prevSib)
        parent->lastChild = pTree;

    return pTree;
}

static void
RebuildTree(WindowPtr pWin)
{
    miOverlayTreePtr parent, prevSib, tChild;
    WindowPtr pChild;

    prevSib = tChild = NULL;

    pWin = pWin->parent;

    while (IN_OVERLAY(pWin))
        pWin = pWin->parent;

    parent = MIOVERLAY_GET_WINDOW_TREE(pWin);

    pChild = pWin->firstChild;
    parent->firstChild = parent->lastChild = NULL;

    while (1) {
        if (IN_UNDERLAY(pChild))
            prevSib = tChild = DoLeaf(pChild, parent, prevSib);

        if (pChild->firstChild) {
            if (IN_UNDERLAY(pChild)) {
                parent = tChild;
                prevSib = NULL;
            }
            pChild = pChild->firstChild;
            continue;
        }

        while (!pChild->nextSib) {
            pChild = pChild->parent;
            if (pChild == pWin)
                return;
            if (IN_UNDERLAY(pChild)) {
                prevSib = tChild = MIOVERLAY_GET_WINDOW_TREE(pChild);
                parent = tChild->parent;
            }
        }

        pChild = pChild->nextSib;
    }
}

static Bool
HasUnderlayChildren(WindowPtr pWin)
{
    WindowPtr pChild;

    if (!(pChild = pWin->firstChild))
        return FALSE;

    while (1) {
        if (IN_UNDERLAY(pChild))
            return TRUE;

        if (pChild->firstChild) {
            pChild = pChild->firstChild;
            continue;
        }

        while (!pChild->nextSib && (pWin != pChild))
            pChild = pChild->parent;

        if (pChild == pWin)
            break;

        pChild = pChild->nextSib;
    }

    return FALSE;
}

static Bool
CollectUnderlayChildrenRegions(WindowPtr pWin, RegionPtr pReg)
{
    WindowPtr pChild;
    miOverlayTreePtr pTree;
    Bool hasUnderlay;

    if (!(pChild = pWin->firstChild))
        return FALSE;

    hasUnderlay = FALSE;

    while (1) {
        if ((pTree = MIOVERLAY_GET_WINDOW_TREE(pChild))) {
            RegionAppend(pReg, &pTree->borderClip);
            hasUnderlay = TRUE;
        }
        else if (pChild->firstChild) {
            pChild = pChild->firstChild;
            continue;
        }

        while (!pChild->nextSib && (pWin != pChild))
            pChild = pChild->parent;

        if (pChild == pWin)
            break;

        pChild = pChild->nextSib;
    }

    if (hasUnderlay) {
        Bool overlap;

        RegionValidate(pReg, &overlap);
    }

    return hasUnderlay;
}

static void
MarkUnderlayWindow(WindowPtr pWin)
{
    miOverlayTreePtr pTree = MIOVERLAY_GET_WINDOW_TREE(pWin);

    if (pTree->valdata)
        return;
    pTree->valdata =
        (miOverlayValDataPtr) xnfalloc(sizeof(miOverlayValDataRec));
    pTree->valdata->oldAbsCorner.x = pWin->drawable.x;
    pTree->valdata->oldAbsCorner.y = pWin->drawable.y;
    pTree->valdata->borderVisible = NullRegion;
}
