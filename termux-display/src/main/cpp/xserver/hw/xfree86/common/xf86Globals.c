
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains all the XFree86 global variables.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "windowstr.h"
#include "propertyst.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Parser.h"
#include "xf86Xinput.h"
#include "xf86InPriv.h"
#include "xf86Config.h"

/* Globals that video drivers may access */

DevPrivateKeyRec xf86CreateRootWindowKeyRec;
DevPrivateKeyRec xf86ScreenKeyRec;

ScrnInfoPtr *xf86Screens = NULL;        /* List of ScrnInfos */
ScrnInfoPtr *xf86GPUScreens = NULL;        /* List of ScrnInfos */

int xf86DRMMasterFd = -1;  /* Command line argument for DRM master file descriptor */

const unsigned char byte_reversed[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

/* Globals that input drivers may access */
InputInfoPtr xf86InputDevs = NULL;

/* Globals that video drivers may not access */

xf86InfoRec xf86Info = {
    .consoleFd = -1,
    .vtno = -1,
    .lastEventTime = -1,
    .vtRequestsPending = FALSE,
#ifdef __sun
    .vtPendingNum = -1,
#endif
    .dontVTSwitch = FALSE,
    .autoVTSwitch = TRUE,
    .ShareVTs = FALSE,
    .dontZap = FALSE,
    .dontZoom = FALSE,
    .currentScreen = NULL,
#ifdef CSRG_BASED
    .consType = -1,
#endif
    .allowMouseOpenFail = FALSE,
    .vidModeEnabled = TRUE,
    .vidModeAllowNonLocal = FALSE,
    .miscModInDevEnabled = TRUE,
    .miscModInDevAllowNonLocal = FALSE,
    .pmFlag = TRUE,
#if defined(CONFIG_HAL) || defined(CONFIG_UDEV) || defined(CONFIG_WSCONS)
    .forceInputDevices = FALSE,
    .autoAddDevices = TRUE,
    .autoEnableDevices = TRUE,
#else
    .forceInputDevices = TRUE,
    .autoAddDevices = FALSE,
    .autoEnableDevices = FALSE,
#endif
#if defined(CONFIG_UDEV_KMS)
    .autoAddGPU = TRUE,
#else
    .autoAddGPU = FALSE,
#endif
    .autoBindGPU = TRUE,
};

const char *xf86ConfigFile = NULL;
const char *xf86ConfigDir = NULL;
const char *xf86ModulePath = DEFAULT_MODULE_PATH;
MessageType xf86ModPathFrom = X_DEFAULT;
const char *xf86LogFile = DEFAULT_LOGDIR "/" DEFAULT_LOGPREFIX;
MessageType xf86LogFileFrom = X_DEFAULT;
Bool xf86LogFileWasOpened = FALSE;
serverLayoutRec xf86ConfigLayout = { NULL, };
confDRIRec xf86ConfigDRI = { 0, };

XF86ConfigPtr xf86configptr = NULL;
Bool xf86Resetting = FALSE;
Bool xf86Initialising = FALSE;
Bool xf86DoConfigure = FALSE;
Bool xf86ProbeIgnorePrimary = FALSE;
Bool xf86DoShowOptions = FALSE;
DriverPtr *xf86DriverList = NULL;
int xf86NumDrivers = 0;
InputDriverPtr *xf86InputDriverList = NULL;
int xf86NumInputDrivers = 0;
int xf86NumScreens = 0;
int xf86NumGPUScreens = 0;

const char *xf86VisualNames[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

/* Parameters set only from the command line */
Bool xf86fpFlag = FALSE;
Bool xf86sFlag = FALSE;
Bool xf86bsEnableFlag = FALSE;
Bool xf86bsDisableFlag = FALSE;
Bool xf86silkenMouseDisableFlag = FALSE;
Bool xf86xkbdirFlag = FALSE;

#ifdef HAVE_ACPI
Bool xf86acpiDisableFlag = FALSE;
#endif
char *xf86LayoutName = NULL;
char *xf86ScreenName = NULL;
char *xf86PointerName = NULL;
char *xf86KeyboardName = NULL;
int xf86Verbose = DEFAULT_VERBOSE;
int xf86LogVerbose = DEFAULT_LOG_VERBOSE;
int xf86FbBpp = -1;
int xf86Depth = -1;
rgb xf86Weight = { 0, 0, 0 };

Gamma xf86Gamma = { 0.0, 0.0, 0.0 };

Bool xf86AllowMouseOpenFail = FALSE;
Bool xf86AutoBindGPUDisabled = FALSE;

#ifdef XF86VIDMODE
Bool xf86VidModeDisabled = FALSE;
Bool xf86VidModeAllowNonLocal = FALSE;
#endif
Bool xorgHWAccess = FALSE;
