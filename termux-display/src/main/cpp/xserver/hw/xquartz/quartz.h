/*
 * quartz.h
 *
 * External interface of the Quartz display modes seen by the generic, mode
 * independent parts of the Darwin X server.
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 * Copyright (c) 2001-2003 Greg Parker and Torrey T. Lyons.
 *                 All Rights Reserved.
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

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include <X11/Xdefs.h>
#include "privates.h"

#include "screenint.h"
#include "window.h"
#include "pixmap.h"

/*------------------------------------------
   Quartz display mode function types
   ------------------------------------------*/

/*
 * Display mode initialization
 */
typedef void (*DisplayInitProc)(void);
typedef Bool (*AddScreenProc)(int index, ScreenPtr pScreen);
typedef Bool (*SetupScreenProc)(int index, ScreenPtr pScreen);
typedef void (*InitInputProc)(int argc, char **argv);

/*
 * Cursor functions
 */
typedef Bool (*InitCursorProc)(ScreenPtr pScreen);

/*
 * Suspend and resume X11 activity
 */
typedef void (*SuspendScreenProc)(ScreenPtr pScreen);
typedef void (*ResumeScreenProc)(ScreenPtr pScreen);

/*
 * Screen state change support
 */
typedef void (*AddPseudoramiXScreensProc)
    (int *x, int *y, int *width, int *height, ScreenPtr pScreen);
typedef void (*UpdateScreenProc)(ScreenPtr pScreen);

/*
 * Rootless helper functions
 */
typedef Bool (*IsX11WindowProc)(int windowNumber);
typedef void (*HideWindowsProc)(Bool hide);

/*
 * Rootless functions for optional export to GLX layer
 */
typedef void * (*FrameForWindowProc)(WindowPtr pWin, Bool create);
typedef WindowPtr (*TopLevelParentProc)(WindowPtr pWindow);
typedef Bool (*CreateSurfaceProc)
    (ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable,
    unsigned int client_id, unsigned int *surface_id,
    unsigned int key[2], void (*notify)(void *arg, void *data),
    void *notify_data);
typedef Bool (*DestroySurfaceProc)
    (ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable,
    void (*notify)(void *arg, void *data), void *notify_data);

/*
 * Quartz display mode function list
 */
typedef struct _QuartzModeProcs {
    DisplayInitProc DisplayInit;
    AddScreenProc AddScreen;
    SetupScreenProc SetupScreen;
    InitInputProc InitInput;

    InitCursorProc InitCursor;

    SuspendScreenProc SuspendScreen;
    ResumeScreenProc ResumeScreen;

    AddPseudoramiXScreensProc AddPseudoramiXScreens;
    UpdateScreenProc UpdateScreen;

    IsX11WindowProc IsX11Window;
    HideWindowsProc HideWindows;

    FrameForWindowProc FrameForWindow;
    TopLevelParentProc TopLevelParent;
    CreateSurfaceProc CreateSurface;
    DestroySurfaceProc DestroySurface;
} QuartzModeProcsRec, *QuartzModeProcsPtr;

extern QuartzModeProcsPtr quartzProcs;

extern Bool XQuartzFullscreenVisible; /* Are the windows visible (predicated on !rootless) */
extern Bool XQuartzServerVisible;     /* Is the server visible ... TODO: Refactor to "active" */
extern Bool XQuartzEnableKeyEquivalents;
extern Bool XQuartzRootlessDefault;  /* Is our default mode rootless? */
extern Bool XQuartzIsRootless;       /* Is our current mode rootless (or FS)? */
extern Bool XQuartzFullscreenMenu;   /* Show the menu bar (autohide) while in FS */
extern Bool XQuartzFullscreenDisableHotkeys;
extern Bool XQuartzOptionSendsAlt;   /* Alt or Mode_switch? */

extern int32_t XQuartzShieldingWindowLevel; /* CGShieldingWindowLevel() or 0 */

// Other shared data
extern DevPrivateKeyRec quartzScreenKeyRec;
#define quartzScreenKey (&quartzScreenKeyRec)
extern int aquaMenuBarHeight;

// Name of GLX bundle for native OpenGL
extern const char      *quartzOpenGLBundle;

Bool
QuartzAddScreen(int index, ScreenPtr pScreen);
Bool
QuartzSetupScreen(int index, ScreenPtr pScreen);
void
QuartzInitOutput(int argc, char **argv);
void
QuartzInitInput(int argc, char **argv);
void
QuartzInitServer(int argc, char **argv, char **envp);
void
QuartzGiveUp(void);
void
QuartzProcessEvent(xEvent *xe);
void
QuartzUpdateScreens(void);

void
QuartzShow(void);
void
QuartzHide(void);
void
QuartzSetRootClip(int mode);
void
QuartzSpaceChanged(uint32_t space_id);

void
QuartzSetRootless(Bool state);
void
QuartzShowFullscreen(Bool state);

int
server_main(int argc, char **argv, char **envp);
#endif
