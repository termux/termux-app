/*
 * quartzRandR.h
 *
 * Copyright (c) 2010 Jan Hauffa.
 *               2010-2012 Apple Inc.
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

#ifndef _QUARTZRANDR_H_
#define _QUARTZRANDR_H_

#include "randrstr.h"

typedef struct {
    size_t width, height;
    int refresh;
    RRScreenSizePtr pSize;
    void *ref; /* CGDisplayModeRef or CFDictionaryRef */
} QuartzModeInfo, *QuartzModeInfoPtr;

// Quartz specific per screen storage structure
typedef struct {
    // List of CoreGraphics displays that this X11 screen covers.
    // This is more than one CG display for video mirroring and
    // rootless PseudoramiX mode.
    // No CG display will be covered by more than one X11 screen.
    int displayCount;
    CGDirectDisplayID *displayIDs;
    QuartzModeInfo rootlessMode, fullscreenMode, currentMode;
} QuartzScreenRec, *QuartzScreenPtr;

#define QUARTZ_PRIV(pScreen) \
    ((QuartzScreenPtr)dixLookupPrivate(&pScreen->devPrivates, quartzScreenKey))

void
QuartzCopyDisplayIDs(ScreenPtr pScreen, int displayCount,
                     CGDirectDisplayID *displayIDs);

Bool
QuartzRandRUpdateFakeModes(BOOL force_update);
Bool
QuartzRandRInit(ScreenPtr pScreen);

/* These two functions provide functionality expected by the legacy
 * mode switching.  They are equivalent to a client requesting one
 * of the modes corresponding to these "fake" modes.
 * QuartzRandRSetFakeFullscreen takes an argument which is used to determine
 * the visibility of the windows after the change.
 */
void
QuartzRandRSetFakeRootless(void);
void
QuartzRandRSetFakeFullscreen(BOOL state);

/* Toggle fullscreen mode.  If "fake" fullscreen is the current mode,
 * this will just show/hide the X11 windows.  If we are in a RandR fullscreen
 * mode, this will toggles us to the default fake mode and hide windows if
 * it is fullscreen
 */
void
QuartzRandRToggleFullscreen(void);

#endif
