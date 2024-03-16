/*
 * Xplugin rootless implementation screen functions
 *
 * Copyright (c) 2002-2012 Apple Computer, Inc. All Rights Reserved.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include "quartz.h"
#include "quartzRandR.h"
#include "xpr.h"
#include "xprEvent.h"
#include "pseudoramiX.h"
#include "darwinEvents.h"
#include "rootless.h"
#include "dri.h"
#include "globals.h"
#include <Xplugin.h>
#include "applewmExt.h"
#include "micmap.h"

#include "rootlessCommon.h"

#ifdef DAMAGE
#include "damage.h"
#endif

#include "nonsdk_extinit.h"

/* 10.4's deferred update makes X slower.. have to live with the tearing
 * for now.. */
#define XP_NO_DEFERRED_UPDATES 8

// Name of GLX bundle for native OpenGL
static const char *xprOpenGLBundle = "glxCGL.bundle";

/*
 * eventHandler
 *  Callback handler for Xplugin events.
 */
static void
eventHandler(unsigned int type, const void *arg,
             unsigned int arg_size, void *data)
{

    switch (type) {
    case XP_EVENT_DISPLAY_CHANGED:
        DEBUG_LOG("XP_EVENT_DISPLAY_CHANGED\n");
        DarwinSendDDXEvent(kXquartzDisplayChanged, 0);
        break;

    case XP_EVENT_WINDOW_STATE_CHANGED:
        if (arg_size >= sizeof(xp_window_state_event)) {
            const xp_window_state_event *ws_arg = arg;

            DEBUG_LOG("XP_EVENT_WINDOW_STATE_CHANGED: id=%d, state=%d\n",
                      ws_arg->id,
                      ws_arg->state);
            DarwinSendDDXEvent(kXquartzWindowState, 2,
                               ws_arg->id, ws_arg->state);
        }
        else {
            DEBUG_LOG("XP_EVENT_WINDOW_STATE_CHANGED: ignored\n");
        }
        break;

    case XP_EVENT_WINDOW_MOVED:
        DEBUG_LOG("XP_EVENT_WINDOW_MOVED\n");
        if (arg_size == sizeof(xp_window_id)) {
            xp_window_id id = *(xp_window_id *)arg;
            DarwinSendDDXEvent(kXquartzWindowMoved, 1, id);
        }
        break;

    case XP_EVENT_SURFACE_DESTROYED:
        DEBUG_LOG("XP_EVENT_SURFACE_DESTROYED\n");

    case XP_EVENT_SURFACE_CHANGED:
        DEBUG_LOG("XP_EVENT_SURFACE_CHANGED\n");
        if (arg_size == sizeof(xp_surface_id)) {
            int kind;

            if (type == XP_EVENT_SURFACE_DESTROYED)
                kind = AppleDRISurfaceNotifyDestroyed;
            else
                kind = AppleDRISurfaceNotifyChanged;

            DRISurfaceNotify(*(xp_surface_id *)arg, kind);
        }
        break;

#ifdef XP_EVENT_SPACE_CHANGED
    case  XP_EVENT_SPACE_CHANGED:
        DEBUG_LOG("XP_EVENT_SPACE_CHANGED\n");
        if (arg_size == sizeof(uint32_t)) {
            uint32_t space_id = *(uint32_t *)arg;
            DarwinSendDDXEvent(kXquartzSpaceChanged, 1, space_id);
        }
        break;

#endif
    default:
        ErrorF("Unknown XP_EVENT type (%d) in xprScreen:eventHandler\n", type);
    }
}

/*
 * displayAtIndex
 *  Return the display ID for a particular display index.
 */
static CGDirectDisplayID
displayAtIndex(int index)
{
    CGError err;
    CGDisplayCount cnt;
    CGDirectDisplayID dpy[index + 1];

    err = CGGetActiveDisplayList(index + 1, dpy, &cnt);
    if (err == kCGErrorSuccess && cnt == index + 1)
        return dpy[index];
    else
        return kCGNullDirectDisplay;
}

/*
 * displayScreenBounds
 *  Return the bounds of a particular display.
 */
static CGRect
displayScreenBounds(CGDirectDisplayID id)
{
    CGRect frame;

    frame = CGDisplayBounds(id);

    DEBUG_LOG("    %dx%d @ (%d,%d).\n",
              (int)frame.size.width, (int)frame.size.height,
              (int)frame.origin.x, (int)frame.origin.y);

    Boolean spacePerDisplay = false;
    Boolean ok;
    (void)CFPreferencesAppSynchronize(CFSTR("com.apple.spaces"));
    spacePerDisplay = ! CFPreferencesGetAppBooleanValue(CFSTR("spans-displays"),
                                                        CFSTR("com.apple.spaces"),
                                                        &ok);
    if (!ok)
        spacePerDisplay = true;

    /* Remove menubar to help standard X11 window managers.
     * On Mavericks and later, the menu bar is on all displays when spans-displays is false or unset.
     */
    if (XQuartzIsRootless &&
        (spacePerDisplay || (frame.origin.x == 0 && frame.origin.y == 0))) {
        frame.origin.y += aquaMenuBarHeight;
        frame.size.height -= aquaMenuBarHeight;
    }

    DEBUG_LOG("    %dx%d @ (%d,%d).\n",
              (int)frame.size.width, (int)frame.size.height,
              (int)frame.origin.x, (int)frame.origin.y);

    return frame;
}

/*
 * xprAddPseudoramiXScreens
 *  Add a single virtual screen encompassing all the physical screens
 *  with PseudoramiX.
 */
static void
xprAddPseudoramiXScreens(int *x, int *y, int *width, int *height,
                         ScreenPtr pScreen)
{
    CGDisplayCount i, displayCount;
    CGDirectDisplayID *displayList = NULL;
    CGRect unionRect = CGRectNull, frame;

    // Find all the CoreGraphics displays
    CGGetActiveDisplayList(0, NULL, &displayCount);
    DEBUG_LOG("displayCount: %d\n", (int)displayCount);

    if (!displayCount) {
        ErrorF(
            "CoreGraphics has reported no connected displays.  Creating a stub 800x600 display.\n");
        *x = *y = 0;
        *width = 800;
        *height = 600;
        PseudoramiXAddScreen(*x, *y, *width, *height);
        QuartzCopyDisplayIDs(pScreen, 0, NULL);
        return;
    }

    /* If the displays are captured, we are in a RandR game mode
     * on the primary display, so we only want to include the first
     * display.  The others are covered by the shield window.
     */
    if (CGDisplayIsCaptured(kCGDirectMainDisplay))
        displayCount = 1;

    displayList = malloc(displayCount * sizeof(CGDirectDisplayID));
    if (!displayList)
        FatalError("Unable to allocate memory for list of displays.\n");
    CGGetActiveDisplayList(displayCount, displayList, &displayCount);
    QuartzCopyDisplayIDs(pScreen, displayCount, displayList);

    /* Get the union of all screens */
    for (i = 0; i < displayCount; i++) {
        CGDirectDisplayID dpy = displayList[i];
        frame = displayScreenBounds(dpy);
        unionRect = CGRectUnion(unionRect, frame);
    }

    /* Use unionRect as the screen size for the X server. */
    *x = unionRect.origin.x;
    *y = unionRect.origin.y;
    *width = unionRect.size.width;
    *height = unionRect.size.height;

    DEBUG_LOG("  screen union origin: (%d,%d) size: (%d,%d).\n",
              *x, *y, *width, *height);

    /* Tell PseudoramiX about the real screens. */
    for (i = 0; i < displayCount; i++) {
        CGDirectDisplayID dpy = displayList[i];

        frame = displayScreenBounds(dpy);
        frame.origin.x -= unionRect.origin.x;
        frame.origin.y -= unionRect.origin.y;

        DEBUG_LOG("    placed at X11 coordinate (%d,%d).\n",
                  (int)frame.origin.x, (int)frame.origin.y);

        PseudoramiXAddScreen(frame.origin.x, frame.origin.y,
                             frame.size.width, frame.size.height);
    }

    free(displayList);
}

/*
 * xprDisplayInit
 *  Find number of CoreGraphics displays and initialize Xplugin.
 */
static void
xprDisplayInit(void)
{
    CGDisplayCount displayCount;

    TRACE();

    CGGetActiveDisplayList(0, NULL, &displayCount);

    /* With PseudoramiX, the X server only sees one screen; only PseudoramiX
       itself knows about all of the screens. */

    if (noPseudoramiXExtension) {
        darwinScreensFound = displayCount;
    } else {
        PseudoramiXExtensionInit();
        darwinScreensFound = 1;
    }

    if (xp_init(XP_BACKGROUND_EVENTS | XP_NO_DEFERRED_UPDATES) != Success)
        FatalError("Could not initialize the Xplugin library.");

    xp_select_events(XP_EVENT_DISPLAY_CHANGED
                     | XP_EVENT_WINDOW_STATE_CHANGED
                     | XP_EVENT_WINDOW_MOVED
#ifdef XP_EVENT_SPACE_CHANGED
                     | XP_EVENT_SPACE_CHANGED
#endif
                     | XP_EVENT_SURFACE_CHANGED
                     | XP_EVENT_SURFACE_DESTROYED,
                     eventHandler, NULL);

    AppleDRIExtensionInit();
    xprAppleWMInit();

    XQuartzIsRootless = XQuartzRootlessDefault;
    if (!XQuartzIsRootless)
        RootlessHideAllWindows();
}

/*
 * xprAddScreen
 *  Init the framebuffer and record pixmap parameters for the screen.
 */
static Bool
xprAddScreen(int index, ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    int depth = darwinDesiredDepth;

    DEBUG_LOG("index=%d depth=%d\n", index, depth);

    if (depth == -1) {
        CGDisplayModeRef modeRef;
        CFStringRef encStrRef;

        modeRef = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
        if (!modeRef)
            goto have_depth;

        encStrRef = CGDisplayModeCopyPixelEncoding(modeRef);
        CFRelease(modeRef);
        if (!encStrRef)
            goto have_depth;

        if (CFStringCompare(encStrRef, CFSTR(IO32BitDirectPixels),
                            kCFCompareCaseInsensitive) ==
            kCFCompareEqualTo) {
            depth = 24;
        }
        else if (CFStringCompare(encStrRef, CFSTR(IO16BitDirectPixels),
                                 kCFCompareCaseInsensitive) ==
                 kCFCompareEqualTo) {
            depth = 15;
        }
        else if (CFStringCompare(encStrRef, CFSTR(IO8BitIndexedPixels),
                                 kCFCompareCaseInsensitive) ==
                 kCFCompareEqualTo) {
            depth = 8;
        }

        CFRelease(encStrRef);
    }

have_depth:
    switch (depth) {
    case 8:     // pseudo-working
        dfb->visuals = PseudoColorMask;
        dfb->preferredCVC = PseudoColor;
        dfb->depth = 8;
        dfb->bitsPerRGB = 8;
        dfb->bitsPerPixel = 8;
        dfb->redMask = 0;
        dfb->greenMask = 0;
        dfb->blueMask = 0;
        break;

#if 0
    // Removed because Mountain Lion removed support for
    // 15bit backing stores.  We can possibly re-add
    // this once libXplugin is updated to work around it.
    case 15:
        dfb->visuals = TrueColorMask;     //LARGE_VISUALS;
        dfb->preferredCVC = TrueColor;
        dfb->depth = 15;
        dfb->bitsPerRGB = 5;
        dfb->bitsPerPixel = 16;
        dfb->redMask = RM_ARGB(0, 5, 5, 5);
        dfb->greenMask = GM_ARGB(0, 5, 5, 5);
        dfb->blueMask = BM_ARGB(0, 5, 5, 5);
        break;
#endif

    //        case 24:
    default:
        if (depth != 24)
            ErrorF(
                "Unsupported color depth requested.  Defaulting to 24bit. (depth=%d darwinDesiredDepth=%d)\n",
                depth, darwinDesiredDepth);
        dfb->visuals = TrueColorMask;     //LARGE_VISUALS;
        dfb->preferredCVC = TrueColor;
        dfb->depth = 24;
        dfb->bitsPerRGB = 8;
        dfb->bitsPerPixel = 32;
        dfb->redMask = RM_ARGB(0, 8, 8, 8);
        dfb->greenMask = GM_ARGB(0, 8, 8, 8);
        dfb->blueMask = BM_ARGB(0, 8, 8, 8);
        break;
    }

    if (noPseudoramiXExtension) {
        CGDirectDisplayID dpy;
        CGRect frame;

        ErrorF("Warning: noPseudoramiXExtension!\n");

        dpy = displayAtIndex(index);
        QuartzCopyDisplayIDs(pScreen, 1, &dpy);

        frame = displayScreenBounds(dpy);

        dfb->x = frame.origin.x;
        dfb->y = frame.origin.y;
        dfb->width = frame.size.width;
        dfb->height = frame.size.height;
    }
    else {
        xprAddPseudoramiXScreens(&dfb->x, &dfb->y, &dfb->width, &dfb->height,
                                 pScreen);
    }

    /* Passing zero width (pitch) makes miCreateScreenResources set the
       screen pixmap to the framebuffer pointer, i.e. NULL. The generic
       rootless code takes care of making this work. */
    dfb->pitch = 0;
    dfb->framebuffer = NULL;

    DRIScreenInit(pScreen);

    return TRUE;
}

/*
 * xprSetupScreen
 *  Setup the screen for rootless access.
 */
static Bool
xprSetupScreen(int index, ScreenPtr pScreen)
{
#ifdef DAMAGE
    // The Damage extension needs to wrap underneath the
    // generic rootless layer, so do it now.
    if (!DamageSetup(pScreen))
        return FALSE;
#endif

    // Initialize generic rootless code
    if (!xprInit(pScreen))
        return FALSE;

    return DRIFinishScreenInit(pScreen);
}

/*
 * xprUpdateScreen
 *  Update screen after configuration change.
 */
static void
xprUpdateScreen(ScreenPtr pScreen)
{
    rootlessGlobalOffsetX = darwinMainScreenX;
    rootlessGlobalOffsetY = darwinMainScreenY;

    AppleWMSetScreenOrigin(pScreen->root);

    RootlessRepositionWindows(pScreen);
    RootlessUpdateScreenPixmap(pScreen);
}

/*
 * xprInitInput
 *  Finalize xpr specific setup.
 */
static void
xprInitInput(int argc, char **argv)
{
    int i;

    rootlessGlobalOffsetX = darwinMainScreenX;
    rootlessGlobalOffsetY = darwinMainScreenY;

    for (i = 0; i < screenInfo.numScreens; i++)
        AppleWMSetScreenOrigin(screenInfo.screens[i]->root);
}

/*
 * Quartz display mode function list.
 */
static QuartzModeProcsRec xprModeProcs = {
    xprDisplayInit,
    xprAddScreen,
    xprSetupScreen,
    xprInitInput,
    QuartzInitCursor,
    QuartzSuspendXCursor,
    QuartzResumeXCursor,
    xprAddPseudoramiXScreens,
    xprUpdateScreen,
    xprIsX11Window,
    xprHideWindows,
    RootlessFrameForWindow,
    TopLevelParent,
    DRICreateSurface,
    DRIDestroySurface
};

/*
 * QuartzModeBundleInit
 *  Initialize the display mode bundle after loading.
 */
Bool
QuartzModeBundleInit(void)
{
    quartzProcs = &xprModeProcs;
    quartzOpenGLBundle = xprOpenGLBundle;
    return TRUE;
}
