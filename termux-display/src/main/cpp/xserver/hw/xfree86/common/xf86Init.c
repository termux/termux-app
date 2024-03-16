/*
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */
/*
 * Copyright (c) 1992-2003 by The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdlib.h>
#include <errno.h>

#undef HAS_UTSNAME
#if !defined(WIN32)
#define HAS_UTSNAME 1
#include <sys/utsname.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "input.h"
#include "servermd.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "dbus-core.h"
#include "systemd-logind.h"

#include "loaderProcs.h"

#define XF86_OS_PRIVS
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86_OSlib.h"
#include "xf86cmap.h"
#include "xorgVersion.h"
#include "mipointer.h"
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "xf86Extensions.h"
#include "xf86DDC.h"
#include "xf86Xinput.h"
#include "xf86InPriv.h"
#include "xf86Crtc.h"
#include "picturestr.h"
#include "randrstr.h"
#include "xf86Bus.h"
#ifdef XSERVER_LIBPCIACCESS
#include "xf86VGAarbiter.h"
#endif
#include "globals.h"
#include "xserver-properties.h"

#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif

#ifdef __linux__
#include <linux/major.h>
#include <sys/sysmacros.h>
#endif
#include <hotplug.h>

void (*xf86OSPMClose) (void) = NULL;
static Bool xorgHWOpenConsole = FALSE;

/* Common pixmap formats */

static PixmapFormatRec formats[MAXFORMATS] = {
    {1, 1, BITMAP_SCANLINE_PAD},
    {4, 8, BITMAP_SCANLINE_PAD},
    {8, 8, BITMAP_SCANLINE_PAD},
    {15, 16, BITMAP_SCANLINE_PAD},
    {16, 16, BITMAP_SCANLINE_PAD},
    {24, 32, BITMAP_SCANLINE_PAD},
    {32, 32, BITMAP_SCANLINE_PAD},
};

static int numFormats = 7;
static Bool formatsDone = FALSE;


static void
xf86PrintBanner(void)
{
    xf86ErrorFVerb(0, "\nX.Org X Server %d.%d.%d",
                   XORG_VERSION_MAJOR, XORG_VERSION_MINOR, XORG_VERSION_PATCH);
#if XORG_VERSION_SNAP > 0
    xf86ErrorFVerb(0, ".%d", XORG_VERSION_SNAP);
#endif

#if XORG_VERSION_SNAP >= 900
    /* When the minor number is 99, that signifies that the we are making
     * a release candidate for a major version.  (X.0.0)
     * When the patch number is 99, that signifies that the we are making
     * a release candidate for a minor version.  (X.Y.0)
     * When the patch number is < 99, then we are making a release
     * candidate for the next point release.  (X.Y.Z)
     */
#if XORG_VERSION_MINOR >= 99
    xf86ErrorFVerb(0, " (%d.0.0 RC %d)", XORG_VERSION_MAJOR + 1,
                   XORG_VERSION_SNAP - 900);
#elif XORG_VERSION_PATCH == 99
    xf86ErrorFVerb(0, " (%d.%d.0 RC %d)", XORG_VERSION_MAJOR,
                   XORG_VERSION_MINOR + 1, XORG_VERSION_SNAP - 900);
#else
    xf86ErrorFVerb(0, " (%d.%d.%d RC %d)", XORG_VERSION_MAJOR,
                   XORG_VERSION_MINOR, XORG_VERSION_PATCH + 1,
                   XORG_VERSION_SNAP - 900);
#endif
#endif

#ifdef XORG_CUSTOM_VERSION
    xf86ErrorFVerb(0, " (%s)", XORG_CUSTOM_VERSION);
#endif
    xf86ErrorFVerb(0, "\nX Protocol Version %d, Revision %d\n",
                   X_PROTOCOL, X_PROTOCOL_REVISION);
#ifdef HAS_UTSNAME
    {
        struct utsname name;

        /* Linux & BSD state that 0 is success, SysV (including Solaris, HP-UX,
           and Irix) and Single Unix Spec 3 just say that non-negative is success.
           All agree that failure is represented by a negative number.
         */
        if (uname(&name) >= 0) {
            xf86ErrorFVerb(0, "Current Operating System: %s %s %s %s %s\n",
                           name.sysname, name.nodename, name.release,
                           name.version, name.machine);
#ifdef __linux__
            do {
                char buf[80];
                int fd = open("/proc/cmdline", O_RDONLY);

                if (fd != -1) {
                    xf86ErrorFVerb(0, "Kernel command line: ");
                    memset(buf, 0, 80);
                    while (read(fd, buf, 80) > 0) {
                        xf86ErrorFVerb(0, "%.80s", buf);
                        memset(buf, 0, 80);
                    }
                    close(fd);
                }
            } while (0);
#endif
        }
    }
#endif
#if defined(BUILDERSTRING)
    xf86ErrorFVerb(0, "%s \n", BUILDERSTRING);
#endif
    xf86ErrorFVerb(0, "Current version of pixman: %s\n",
                   pixman_version_string());
    xf86ErrorFVerb(0, "\tBefore reporting problems, check "
                   "" __VENDORDWEBSUPPORT__ "\n"
                   "\tto make sure that you have the latest version.\n");
}

Bool
xf86PrivsElevated(void)
{
    return PrivsElevated();
}

Bool
xf86HasTTYs(void)
{
#ifdef __linux__
    struct stat tty0devAttributes;
    return (stat("/dev/tty0", &tty0devAttributes) == 0 && major(tty0devAttributes.st_rdev) == TTY_MAJOR);
#else
    return TRUE;
#endif
}

static void
xf86AutoConfigOutputDevices(void)
{
    int i;

    if (!xf86Info.autoBindGPU)
        return;

    for (i = 0; i < xf86NumGPUScreens; i++) {
        int scrnum = xf86GPUScreens[i]->confScreen->screennum;
        RRProviderAutoConfigGpuScreen(xf86ScrnToScreen(xf86GPUScreens[i]),
                                      xf86ScrnToScreen(xf86Screens[scrnum]));
    }
}

static void
AddSeatId(CallbackListPtr *pcbl, void *data, void *screen)
{
    ScreenPtr pScreen = screen;
    Atom SeatAtom = MakeAtom(SEAT_ATOM_NAME, sizeof(SEAT_ATOM_NAME) - 1, TRUE);
    int err;

    err = dixChangeWindowProperty(serverClient, pScreen->root, SeatAtom,
                                  XA_STRING, 8, PropModeReplace,
                                  strlen(data) + 1, data, FALSE);

    if (err != Success)
        xf86DrvMsg(pScreen->myNum, X_WARNING,
                   "Failed to register seat property\n");
}

static void
AddVTAtoms(CallbackListPtr *pcbl, void *data, void *screen)
{
#define VT_ATOM_NAME         "XFree86_VT"
    int err, HasVT = 1;
    ScreenPtr pScreen = screen;
    Atom VTAtom = MakeAtom(VT_ATOM_NAME, sizeof(VT_ATOM_NAME) - 1, TRUE);
    Atom HasVTAtom = MakeAtom(HAS_VT_ATOM_NAME, sizeof(HAS_VT_ATOM_NAME) - 1,
                              TRUE);

    err = dixChangeWindowProperty(serverClient, pScreen->root, VTAtom,
                                  XA_INTEGER, 32, PropModeReplace, 1,
                                  &xf86Info.vtno, FALSE);

    err |= dixChangeWindowProperty(serverClient, pScreen->root, HasVTAtom,
                                   XA_INTEGER, 32, PropModeReplace, 1,
                                   &HasVT, FALSE);

    if (err != Success)
        xf86DrvMsg(pScreen->myNum, X_WARNING,
                   "Failed to register VT properties\n");
}

static Bool
xf86ScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    pScrn->pScreen = pScreen;
    return pScrn->ScreenInit (pScreen, argc, argv);
}

static void
xf86EnsureRANDR(ScreenPtr pScreen)
{
#ifdef RANDR
        if (!dixPrivateKeyRegistered(rrPrivKey) ||
            !rrGetScrPriv(pScreen))
            xf86RandRInit(pScreen);
#endif
}

/*
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *      That includes vt-manager setup, querying all possible devices and
 *      collecting the pixmap formats.
 */
void
InitOutput(ScreenInfo * pScreenInfo, int argc, char **argv)
{
    int i, j, k, scr_index;
    const char **modulelist;
    void **optionlist;
    Bool autoconfig = FALSE;
    Bool sigio_blocked = FALSE;
    Bool want_hw_access = FALSE;
    GDevPtr configured_device;

    xf86Initialising = TRUE;

    config_pre_init();

    if (serverGeneration == 1) {
        xf86PrintBanner();
        LogPrintMarkers();
        if (xf86LogFile) {
            time_t t;
            const char *ct;

            t = time(NULL);
            ct = ctime(&t);
            xf86MsgVerb(xf86LogFileFrom, 0, "Log file: \"%s\", Time: %s",
                        xf86LogFile, ct);
        }

        /* Read and parse the config file */
        if (!xf86DoConfigure && !xf86DoShowOptions) {
            switch (xf86HandleConfigFile(FALSE)) {
            case CONFIG_OK:
                break;
            case CONFIG_PARSE_ERROR:
                xf86Msg(X_ERROR, "Error parsing the config file\n");
                return;
            case CONFIG_NOFILE:
                autoconfig = TRUE;
                break;
            }
        }

        /* Initialise the loader */
        LoaderInit();

        /* Tell the loader the default module search path */
        LoaderSetPath(xf86ModulePath);

        if (xf86Info.ignoreABI) {
            LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
        }

        if (xf86DoShowOptions)
            DoShowOptions();

        dbus_core_init();
        systemd_logind_init();

        /* Do a general bus probe.  This will be a PCI probe for x86 platforms */
        xf86BusProbe();

        if (xf86DoConfigure)
            DoConfigure();

        if (autoconfig) {
            if (!xf86AutoConfig()) {
                xf86Msg(X_ERROR, "Auto configuration failed\n");
                return;
            }
        }

        xf86OSPMClose = xf86OSPMOpen();

        xf86ExtensionInit();

        /* Load all modules specified explicitly in the config file */
        if ((modulelist = xf86ModulelistFromConfig(&optionlist))) {
            xf86LoadModules(modulelist, optionlist);
            free(modulelist);
            free(optionlist);
        }

        /* Load all driver modules specified in the config file */
        /* If there aren't any specified in the config file, autoconfig them */
        /* FIXME: Does not handle multiple active screen sections, but I'm not
         * sure if we really want to handle that case*/
        configured_device = xf86ConfigLayout.screens->screen->device;
        if ((!configured_device) || (!configured_device->driver)) {
            if (!autoConfigDevice(configured_device)) {
                xf86Msg(X_ERROR, "Automatic driver configuration failed\n");
                return;
            }
        }
        if ((modulelist = xf86DriverlistFromConfig())) {
            xf86LoadModules(modulelist, NULL);
            free(modulelist);
        }

        /* Load all input driver modules specified in the config file. */
        if ((modulelist = xf86InputDriverlistFromConfig())) {
            xf86LoadModules(modulelist, NULL);
            free(modulelist);
        }

        /*
         * It is expected that xf86AddDriver()/xf86AddInputDriver will be
         * called for each driver as it is loaded.  Those functions save the
         * module pointers for drivers.
         * XXX Nothing keeps track of them for other modules.
         */
        /* XXX What do we do if not all of these could be loaded? */

        /*
         * At this point, xf86DriverList[] is all filled in with entries for
         * each of the drivers to try and xf86NumDrivers has the number of
         * drivers.  If there are none, return now.
         */

        if (xf86NumDrivers == 0) {
            xf86Msg(X_ERROR, "No drivers available.\n");
            return;
        }

        /*
         * Call each of the Identify functions and call the driverFunc to check
         * if HW access is required.  The Identify functions print out some
         * identifying information, and anything else that might be
         * needed at this early stage.
         */

        for (i = 0; i < xf86NumDrivers; i++) {
            xorgHWFlags flags = HW_IO;

            if (xf86DriverList[i]->Identify != NULL)
                xf86DriverList[i]->Identify(0);

            if (xf86DriverList[i]->driverFunc)
                xf86DriverList[i]->driverFunc(NULL,
                                              GET_REQUIRED_HW_INTERFACES,
                                              &flags);

            if (NEED_IO_ENABLED(flags))
                want_hw_access = TRUE;

            /* Non-seat0 X servers should not open console */
            if (!(flags & HW_SKIP_CONSOLE) && !ServerIsNotSeat0() && xf86HasTTYs())
                xorgHWOpenConsole = TRUE;
        }

        if (xorgHWOpenConsole)
            xf86OpenConsole();
        else
            xf86Info.dontVTSwitch = TRUE;

	/* Enable full I/O access */
	if (want_hw_access)
	    xorgHWAccess = xf86EnableIO();

        if (xf86BusConfig() == FALSE)
            return;

        xf86PostProbe();

        /*
         * Sort the drivers to match the requested ording.  Using a slow
         * bubble sort.
         */
        for (j = 0; j < xf86NumScreens - 1; j++) {
            for (i = 0; i < xf86NumScreens - j - 1; i++) {
                if (xf86Screens[i + 1]->confScreen->screennum <
                    xf86Screens[i]->confScreen->screennum) {
                    ScrnInfoPtr tmpScrn = xf86Screens[i + 1];

                    xf86Screens[i + 1] = xf86Screens[i];
                    xf86Screens[i] = tmpScrn;
                }
            }
        }
        /* Fix up the indexes */
        for (i = 0; i < xf86NumScreens; i++) {
            xf86Screens[i]->scrnIndex = i;
        }

        /*
         * Call the driver's PreInit()'s to complete initialisation for the first
         * generation.
         */

        for (i = 0; i < xf86NumScreens; i++) {
            xf86VGAarbiterScrnInit(xf86Screens[i]);
            xf86VGAarbiterLock(xf86Screens[i]);
            if (xf86Screens[i]->PreInit &&
                xf86Screens[i]->PreInit(xf86Screens[i], 0))
                xf86Screens[i]->configured = TRUE;
            xf86VGAarbiterUnlock(xf86Screens[i]);
        }
        for (i = 0; i < xf86NumScreens; i++)
            if (!xf86Screens[i]->configured)
                xf86DeleteScreen(xf86Screens[i--]);

        for (i = 0; i < xf86NumGPUScreens; i++) {
            xf86VGAarbiterScrnInit(xf86GPUScreens[i]);
            xf86VGAarbiterLock(xf86GPUScreens[i]);
            if (xf86GPUScreens[i]->PreInit &&
                xf86GPUScreens[i]->PreInit(xf86GPUScreens[i], 0))
                xf86GPUScreens[i]->configured = TRUE;
            xf86VGAarbiterUnlock(xf86GPUScreens[i]);
        }
        for (i = 0; i < xf86NumGPUScreens; i++)
            if (!xf86GPUScreens[i]->configured)
                xf86DeleteScreen(xf86GPUScreens[i--]);

        /*
         * If no screens left, return now.
         */

        if (xf86NumScreens == 0) {
            xf86Msg(X_ERROR,
                    "Screen(s) found, but none have a usable configuration.\n");
            return;
        }

        /* Remove (unload) drivers that are not required */
        for (i = 0; i < xf86NumDrivers; i++)
            if (xf86DriverList[i] && xf86DriverList[i]->refCount <= 0)
                xf86DeleteDriver(i);

        /*
         * At this stage we know how many screens there are.
         */

        for (i = 0; i < xf86NumScreens; i++)
            xf86InitViewport(xf86Screens[i]);

        /*
         * Collect all pixmap formats and check for conflicts at the display
         * level.  Should we die here?  Or just delete the offending screens?
         */
        for (i = 0; i < xf86NumScreens; i++) {
            if (xf86Screens[i]->imageByteOrder !=
                xf86Screens[0]->imageByteOrder)
                FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");
            if (xf86Screens[i]->bitmapScanlinePad !=
                xf86Screens[0]->bitmapScanlinePad)
                FatalError
                    ("Inconsistent display bitmapScanlinePad.  Exiting\n");
            if (xf86Screens[i]->bitmapScanlineUnit !=
                xf86Screens[0]->bitmapScanlineUnit)
                FatalError
                    ("Inconsistent display bitmapScanlineUnit.  Exiting\n");
            if (xf86Screens[i]->bitmapBitOrder !=
                xf86Screens[0]->bitmapBitOrder)
                FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");
        }

        /* Collect additional formats */
        for (i = 0; i < xf86NumScreens; i++) {
            for (j = 0; j < xf86Screens[i]->numFormats; j++) {
                for (k = 0;; k++) {
                    if (k >= numFormats) {
                        if (k >= MAXFORMATS)
                            FatalError("Too many pixmap formats!  Exiting\n");
                        formats[k] = xf86Screens[i]->formats[j];
                        numFormats++;
                        break;
                    }
                    if (formats[k].depth == xf86Screens[i]->formats[j].depth) {
                        if ((formats[k].bitsPerPixel ==
                             xf86Screens[i]->formats[j].bitsPerPixel) &&
                            (formats[k].scanlinePad ==
                             xf86Screens[i]->formats[j].scanlinePad))
                            break;
                        FatalError("Inconsistent pixmap format for depth %d."
                                   "  Exiting\n", formats[k].depth);
                    }
                }
            }
        }
        formatsDone = TRUE;
    }
    else {
        /*
         * serverGeneration != 1; some OSs have to do things here, too.
         */
        if (xorgHWOpenConsole)
            xf86OpenConsole();

        /*
           should we reopen it here? We need to deal with an already opened
           device. We could leave this to the OS layer. For now we simply
           close it here
         */
        if (xf86OSPMClose)
            xf86OSPMClose();
        if ((xf86OSPMClose = xf86OSPMOpen()) != NULL)
            xf86MsgVerb(X_INFO, 3, "APM registered successfully\n");

        /* Make sure full I/O access is enabled */
        if (xorgHWAccess)
            xf86EnableIO();
    }

    if (xf86Info.vtno >= 0)
        AddCallback(&RootWindowFinalizeCallback, AddVTAtoms, NULL);

    if (SeatId)
        AddCallback(&RootWindowFinalizeCallback, AddSeatId, SeatId);

    /*
     * Use the previously collected parts to setup pScreenInfo
     */

    pScreenInfo->imageByteOrder = xf86Screens[0]->imageByteOrder;
    pScreenInfo->bitmapScanlinePad = xf86Screens[0]->bitmapScanlinePad;
    pScreenInfo->bitmapScanlineUnit = xf86Screens[0]->bitmapScanlineUnit;
    pScreenInfo->bitmapBitOrder = xf86Screens[0]->bitmapBitOrder;
    pScreenInfo->numPixmapFormats = numFormats;
    for (i = 0; i < numFormats; i++)
        pScreenInfo->formats[i] = formats[i];

    /* Make sure the server's VT is active */

    if (serverGeneration != 1) {
        xf86Resetting = TRUE;
        /* All screens are in the same state, so just check the first */
        if (!xf86VTOwner()) {
#ifdef HAS_USL_VTS
            ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ);
#endif
            input_lock();
            sigio_blocked = TRUE;
        }
    }

    for (i = 0; i < xf86NumScreens; i++)
        if (!xf86ColormapAllocatePrivates(xf86Screens[i]))
            FatalError("Cannot register DDX private keys");

    if (!dixRegisterPrivateKey(&xf86ScreenKeyRec, PRIVATE_SCREEN, 0))
        FatalError("Cannot register DDX private keys");

    for (i = 0; i < xf86NumScreens; i++) {
        xf86VGAarbiterLock(xf86Screens[i]);
        /*
         * Almost everything uses these defaults, and many of those that
         * don't, will wrap them.
         */
        xf86Screens[i]->EnableDisableFBAccess = xf86EnableDisableFBAccess;
#ifdef XFreeXDGA
        xf86Screens[i]->SetDGAMode = xf86SetDGAMode;
#endif
        scr_index = AddScreen(xf86ScreenInit, argc, argv);
        xf86VGAarbiterUnlock(xf86Screens[i]);
        if (scr_index == i) {
            /*
             * Hook in our ScrnInfoRec, and initialise some other pScreen
             * fields.
             */
            dixSetPrivate(&screenInfo.screens[scr_index]->devPrivates,
                          xf86ScreenKey, xf86Screens[i]);
            xf86Screens[i]->pScreen = screenInfo.screens[scr_index];
            /* The driver should set this, but make sure it is set anyway */
            xf86Screens[i]->vtSema = TRUE;
        }
        else {
            /* This shouldn't normally happen */
            FatalError("AddScreen/ScreenInit failed for driver %d\n", i);
        }

        if (PictureGetSubpixelOrder(xf86Screens[i]->pScreen) == SubPixelUnknown) {
            xf86MonPtr DDC = (xf86MonPtr) (xf86Screens[i]->monitor->DDC);

            PictureSetSubpixelOrder(xf86Screens[i]->pScreen,
                                    DDC ?
                                    (DDC->features.input_type ?
                                     SubPixelHorizontalRGB : SubPixelNone) :
                                    SubPixelUnknown);
        }

        /*
         * If the driver hasn't set up its own RANDR support, install the
         * fallback support.
         */
        xf86EnsureRANDR(xf86Screens[i]->pScreen);
    }

    for (i = 0; i < xf86NumGPUScreens; i++) {
        ScrnInfoPtr pScrn = xf86GPUScreens[i];
        xf86VGAarbiterLock(pScrn);

        /*
         * Almost everything uses these defaults, and many of those that
         * don't, will wrap them.
         */
        pScrn->EnableDisableFBAccess = xf86EnableDisableFBAccess;
#ifdef XFreeXDGA
        pScrn->SetDGAMode = xf86SetDGAMode;
#endif
        scr_index = AddGPUScreen(xf86ScreenInit, argc, argv);
        xf86VGAarbiterUnlock(pScrn);
        if (scr_index == i) {
            dixSetPrivate(&screenInfo.gpuscreens[scr_index]->devPrivates,
                          xf86ScreenKey, xf86GPUScreens[i]);
            pScrn->pScreen = screenInfo.gpuscreens[scr_index];
            /* The driver should set this, but make sure it is set anyway */
            pScrn->vtSema = TRUE;
        } else {
            FatalError("AddScreen/ScreenInit failed for gpu driver %d %d\n", i, scr_index);
        }
    }

    for (i = 0; i < xf86NumGPUScreens; i++) {
        int scrnum = xf86GPUScreens[i]->confScreen->screennum;
        AttachUnboundGPU(xf86Screens[scrnum]->pScreen, xf86GPUScreens[i]->pScreen);
    }

    xf86AutoConfigOutputDevices();

    xf86VGAarbiterWrapFunctions();
    if (sigio_blocked)
        input_unlock();

    xf86InitOrigins();

    xf86Resetting = FALSE;
    xf86Initialising = FALSE;

    RegisterBlockAndWakeupHandlers((ServerBlockHandlerProcPtr) NoopDDA, xf86Wakeup,
                                   NULL);
}

/**
 * Initialize all supported input devices present and referenced in the
 * xorg.conf.
 */
void
InitInput(int argc, char **argv)
{
    InputInfoPtr *pInfo;
    DeviceIntPtr dev;

    xf86Info.vtRequestsPending = FALSE;

    /* Enable threaded input */
    InputThreadPreInit();

    mieqInit();

    /* Initialize all configured input devices */
    for (pInfo = xf86ConfigLayout.inputs; pInfo && *pInfo; pInfo++) {
        (*pInfo)->options =
            xf86AddNewOption((*pInfo)->options, "driver", (*pInfo)->driver);
        (*pInfo)->options =
            xf86AddNewOption((*pInfo)->options, "identifier", (*pInfo)->name);
        /* If one fails, the others will too */
        if (NewInputDeviceRequest((*pInfo)->options, NULL, &dev) == BadAlloc)
            break;
    }

    config_init();
}

void
CloseInput(void)
{
    config_fini();
    mieqFini();
}

/*
 * OsVendorInit --
 *      OS/Vendor-specific initialisations.  Called from OsInit(), which
 *      is called by dix before establishing the well known sockets.
 */

void
OsVendorInit(void)
{
    static Bool beenHere = FALSE;

    OsSignal(SIGCHLD, SIG_DFL);   /* Need to wait for child processes */

    if (!beenHere) {
        umask(022);
        xf86LogInit();
    }

    /* Set stderr to non-blocking. */
#ifndef O_NONBLOCK
#if defined(FNDELAY)
#define O_NONBLOCK FNDELAY
#elif defined(O_NDELAY)
#define O_NONBLOCK O_NDELAY
#endif

#ifdef O_NONBLOCK
    if (!beenHere) {
        if (PrivsElevated()) {
            int status;

            status = fcntl(fileno(stderr), F_GETFL, 0);
            if (status != -1) {
                fcntl(fileno(stderr), F_SETFL, status | O_NONBLOCK);
            }
        }
    }
#endif
#endif

    beenHere = TRUE;
}

/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by by dix before normal server death.
 *      For SYSV386 we must switch the terminal back to normal mode. No error-
 *      checking here, since there should be restored as much as possible.
 */

void
ddxGiveUp(enum ExitCode error)
{
    int i;

    if (error == EXIT_ERR_ABORT) {
        input_lock();

        /* try to restore the original video state */
#ifdef DPMSExtension            /* Turn screens back on */
        if (DPMSPowerLevel != DPMSModeOn)
            DPMSSet(serverClient, DPMSModeOn);
#endif
        if (xf86Screens) {
            for (i = 0; i < xf86NumScreens; i++)
                if (xf86Screens[i]->vtSema) {
                    /*
                     * if we are aborting before ScreenInit() has finished we
                     * might not have been wrapped yet. Therefore enable screen
                     * explicitly.
                     */
                    xf86VGAarbiterLock(xf86Screens[i]);
                    (xf86Screens[i]->LeaveVT) (xf86Screens[i]);
                    xf86VGAarbiterUnlock(xf86Screens[i]);
                }
        }
    }

    xf86VGAarbiterFini();

    if (xf86OSPMClose)
        xf86OSPMClose();
    xf86OSPMClose = NULL;

    for (i = 0; i < xf86NumScreens; i++) {
        /*
         * zero all access functions to
         * trap calls when switched away.
         */
        xf86Screens[i]->vtSema = FALSE;
    }

    if (xorgHWOpenConsole)
        xf86CloseConsole();

    systemd_logind_fini();
    dbus_core_fini();

    xf86CloseLog(error);
}

void
OsVendorFatalError(const char *f, va_list args)
{
#ifdef VENDORSUPPORT
    ErrorFSigSafe("\nPlease refer to your Operating System Vendor support "
                 "pages\nat %s for support on this crash.\n", VENDORSUPPORT);
#else
    ErrorFSigSafe("\nPlease consult the " XVENDORNAME " support \n\t at "
                 __VENDORDWEBSUPPORT__ "\n for help. \n");
#endif
    if (xf86LogFile && xf86LogFileWasOpened)
        ErrorFSigSafe("Please also check the log file at \"%s\" for additional "
                     "information.\n", xf86LogFile);
    ErrorFSigSafe("\n");
}

int
xf86SetVerbosity(int verb)
{
    int save = xf86Verbose;

    xf86Verbose = verb;
    LogSetParameter(XLOG_VERBOSITY, verb);
    return save;
}

int
xf86SetLogVerbosity(int verb)
{
    int save = xf86LogVerbose;

    xf86LogVerbose = verb;
    LogSetParameter(XLOG_FILE_VERBOSITY, verb);
    return save;
}

static void
xf86PrintDefaultModulePath(void)
{
    ErrorF("%s\n", DEFAULT_MODULE_PATH);
}

static void
xf86PrintDefaultLibraryPath(void)
{
    ErrorF("%s\n", DEFAULT_LIBRARY_PATH);
}

static void
xf86CheckPrivs(const char *option, const char *arg)
{
    if (PrivsElevated() && !xf86PathIsSafe(arg)) {
        FatalError("\nInvalid argument for %s - \"%s\"\n"
                    "\tWith elevated privileges %s must specify a relative path\n"
                    "\twithout any \"..\" elements.\n\n", option, arg, option);
    }
}

/*
 * ddxProcessArgument --
 *	Process device-dependent command line args. Returns 0 if argument is
 *      not device dependent, otherwise Count of number of elements of argv
 *      that are part of a device dependent commandline option.
 *
 */

/* ARGSUSED */
int
ddxProcessArgument(int argc, char **argv, int i)
{
    /* First the options that are not allowed with elevated privileges */
    if (!strcmp(argv[i], "-modulepath")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (xf86PrivsElevated())
              FatalError("\nInvalid argument -modulepath "
                "with elevated privileges\n");
        xf86ModulePath = argv[i + 1];
        xf86ModPathFrom = X_CMDLINE;
        return 2;
    }
    if (!strcmp(argv[i], "-logfile")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (xf86PrivsElevated())
              FatalError("\nInvalid argument -logfile "
                "with elevated privileges\n");
        xf86LogFile = argv[i + 1];
        xf86LogFileFrom = X_CMDLINE;
        return 2;
    }
    if (!strcmp(argv[i], "-config") || !strcmp(argv[i], "-xf86config")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86CheckPrivs(argv[i], argv[i + 1]);
        xf86ConfigFile = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-configdir")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86CheckPrivs(argv[i], argv[i + 1]);
        xf86ConfigDir = argv[i + 1];
        return 2;
    }
#ifdef XF86VIDMODE
    if (!strcmp(argv[i], "-disableVidMode")) {
        xf86VidModeDisabled = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-allowNonLocalXvidtune")) {
        xf86VidModeAllowNonLocal = TRUE;
        return 1;
    }
#endif
    if (!strcmp(argv[i], "-allowMouseOpenFail")) {
        xf86AllowMouseOpenFail = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-ignoreABI")) {
        LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
        return 1;
    }
    if (!strcmp(argv[i], "-verbose")) {
        if (++i < argc && argv[i]) {
            char *end;
            long val;

            val = strtol(argv[i], &end, 0);
            if (*end == '\0') {
                xf86SetVerbosity(val);
                return 2;
            }
        }
        xf86SetVerbosity(++xf86Verbose);
        return 1;
    }
    if (!strcmp(argv[i], "-logverbose")) {
        if (++i < argc && argv[i]) {
            char *end;
            long val;

            val = strtol(argv[i], &end, 0);
            if (*end == '\0') {
                xf86SetLogVerbosity(val);
                return 2;
            }
        }
        xf86SetLogVerbosity(++xf86LogVerbose);
        return 1;
    }
    if (!strcmp(argv[i], "-quiet")) {
        xf86SetVerbosity(-1);
        return 1;
    }
    if (!strcmp(argv[i], "-showconfig") || !strcmp(argv[i], "-version")) {
        xf86PrintBanner();
        exit(0);
    }
    if (!strcmp(argv[i], "-showDefaultModulePath")) {
        xf86PrintDefaultModulePath();
        exit(0);
    }
    if (!strcmp(argv[i], "-showDefaultLibPath")) {
        xf86PrintDefaultLibraryPath();
        exit(0);
    }
    /* Notice the -fp flag, but allow it to pass to the dix layer */
    if (!strcmp(argv[i], "-fp")) {
        xf86fpFlag = TRUE;
        return 0;
    }
    /* Notice the -bs flag, but allow it to pass to the dix layer */
    if (!strcmp(argv[i], "-bs")) {
        xf86bsDisableFlag = TRUE;
        return 0;
    }
    /* Notice the +bs flag, but allow it to pass to the dix layer */
    if (!strcmp(argv[i], "+bs")) {
        xf86bsEnableFlag = TRUE;
        return 0;
    }
    /* Notice the -s flag, but allow it to pass to the dix layer */
    if (!strcmp(argv[i], "-s")) {
        xf86sFlag = TRUE;
        return 0;
    }
    if (!strcmp(argv[i], "-pixmap32") || !strcmp(argv[i], "-pixmap24")) {
        /* silently accept */
        return 1;
    }
    if (!strcmp(argv[i], "-fbbpp")) {
        int bpp;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (sscanf(argv[++i], "%d", &bpp) == 1) {
            xf86FbBpp = bpp;
            return 2;
        }
        else {
            ErrorF("Invalid fbbpp\n");
            return 0;
        }
    }
    if (!strcmp(argv[i], "-depth")) {
        int depth;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (sscanf(argv[++i], "%d", &depth) == 1) {
            xf86Depth = depth;
            return 2;
        }
        else {
            ErrorF("Invalid depth\n");
            return 0;
        }
    }
    if (!strcmp(argv[i], "-weight")) {
        int red, green, blue;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (sscanf(argv[++i], "%1d%1d%1d", &red, &green, &blue) == 3) {
            xf86Weight.red = red;
            xf86Weight.green = green;
            xf86Weight.blue = blue;
            return 2;
        }
        else {
            ErrorF("Invalid weighting\n");
            return 0;
        }
    }
    if (!strcmp(argv[i], "-gamma") || !strcmp(argv[i], "-rgamma") ||
        !strcmp(argv[i], "-ggamma") || !strcmp(argv[i], "-bgamma")) {
        double gamma;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (sscanf(argv[++i], "%lf", &gamma) == 1) {
            if (gamma < GAMMA_MIN || gamma > GAMMA_MAX) {
                ErrorF("gamma out of range, only  %.2f <= gamma_value <= %.1f"
                       " is valid\n", GAMMA_MIN, GAMMA_MAX);
                return 0;
            }
            if (!strcmp(argv[i - 1], "-gamma"))
                xf86Gamma.red = xf86Gamma.green = xf86Gamma.blue = gamma;
            else if (!strcmp(argv[i - 1], "-rgamma"))
                xf86Gamma.red = gamma;
            else if (!strcmp(argv[i - 1], "-ggamma"))
                xf86Gamma.green = gamma;
            else if (!strcmp(argv[i - 1], "-bgamma"))
                xf86Gamma.blue = gamma;
            return 2;
        }
    }
    if (!strcmp(argv[i], "-layout")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86LayoutName = argv[++i];
        return 2;
    }
    if (!strcmp(argv[i], "-screen")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86ScreenName = argv[++i];
        return 2;
    }
    if (!strcmp(argv[i], "-pointer")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86PointerName = argv[++i];
        return 2;
    }
    if (!strcmp(argv[i], "-keyboard")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xf86KeyboardName = argv[++i];
        return 2;
    }
    if (!strcmp(argv[i], "-nosilk")) {
        xf86silkenMouseDisableFlag = TRUE;
        return 1;
    }
#ifdef HAVE_ACPI
    if (!strcmp(argv[i], "-noacpi")) {
        xf86acpiDisableFlag = TRUE;
        return 1;
    }
#endif
    if (!strcmp(argv[i], "-configure")) {
        if (getuid() != 0 && geteuid() == 0) {
            ErrorF("The '-configure' option can only be used by root.\n");
            exit(1);
        }
        xf86DoConfigure = TRUE;
        xf86AllowMouseOpenFail = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-showopts")) {
        if (getuid() != 0 && geteuid() == 0) {
            ErrorF("The '-showopts' option can only be used by root.\n");
            exit(1);
        }
        xf86DoShowOptions = TRUE;
        return 1;
    }
#ifdef XSERVER_LIBPCIACCESS
    if (!strcmp(argv[i], "-isolateDevice")) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        if (strncmp(argv[++i], "PCI:", 4)) {
            FatalError("Bus types other than PCI not yet isolable\n");
        }
        xf86PciIsolateDevice(argv[i]);
        return 2;
    }
#endif
    /* Notice cmdline xkbdir, but pass to dix as well */
    if (!strcmp(argv[i], "-xkbdir")) {
        xf86xkbdirFlag = TRUE;
        return 0;
    }
    if (!strcmp(argv[i], "-novtswitch")) {
        xf86Info.autoVTSwitch = FALSE;
        return 1;
    }
    if (!strcmp(argv[i], "-sharevts")) {
        xf86Info.ShareVTs = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-iglx") || !strcmp(argv[i], "+iglx")) {
        xf86Info.iglxFrom = X_CMDLINE;
        return 0;
    }
    if (!strcmp(argv[i], "-noautoBindGPU")) {
        xf86AutoBindGPUDisabled = TRUE;
        return 1;
    }

    /* OS-specific processing */
    return xf86ProcessArgument(argc, argv, i);
}

/*
 * ddxUseMsg --
 *	Print out correct use of device dependent commandline options.
 *      Maybe the user now knows what really to do ...
 */

void
ddxUseMsg(void)
{
    ErrorF("\n");
    ErrorF("\n");
    ErrorF("Device Dependent Usage\n");
    if (!PrivsElevated()) {
        ErrorF("-modulepath paths      specify the module search path\n");
        ErrorF("-logfile file          specify a log file name\n");
        ErrorF("-configure             probe for devices and write an "
               XCONFIGFILE "\n");
        ErrorF
            ("-showopts              print available options for all installed drivers\n");
    }
    ErrorF
        ("-config file           specify a configuration file, relative to the\n");
    ErrorF("                       " XCONFIGFILE
           " search path, only root can use absolute\n");
    ErrorF
        ("-configdir dir         specify a configuration directory, relative to the\n");
    ErrorF("                       " XCONFIGDIR
           " search path, only root can use absolute\n");
    ErrorF("-verbose [n]           verbose startup messages\n");
    ErrorF("-logverbose [n]        verbose log messages\n");
    ErrorF("-quiet                 minimal startup messages\n");
    ErrorF("-fbbpp n               set bpp for the framebuffer. Default: 8\n");
    ErrorF("-depth n               set colour depth. Default: 8\n");
    ErrorF
        ("-gamma f               set gamma value (0.1 < f < 10.0) Default: 1.0\n");
    ErrorF("-rgamma f              set gamma value for red phase\n");
    ErrorF("-ggamma f              set gamma value for green phase\n");
    ErrorF("-bgamma f              set gamma value for blue phase\n");
    ErrorF
        ("-weight nnn            set RGB weighting at 16 bpp.  Default: 565\n");
    ErrorF("-layout name           specify the ServerLayout section name\n");
    ErrorF("-screen name           specify the Screen section name\n");
    ErrorF
        ("-keyboard name         specify the core keyboard InputDevice name\n");
    ErrorF
        ("-pointer name          specify the core pointer InputDevice name\n");
    ErrorF("-nosilk                disable Silken Mouse\n");
#ifdef XF86VIDMODE
    ErrorF("-disableVidMode        disable mode adjustments with xvidtune\n");
    ErrorF
        ("-allowNonLocalXvidtune allow xvidtune to be run as a non-local client\n");
#endif
    ErrorF
        ("-allowMouseOpenFail    start server even if the mouse can't be initialized\n");
    ErrorF("-ignoreABI             make module ABI mismatches non-fatal\n");
#ifdef XSERVER_LIBPCIACCESS
    ErrorF
        ("-isolateDevice bus_id  restrict device resets to bus_id (PCI only)\n");
#endif
    ErrorF("-version               show the server version\n");
    ErrorF("-showDefaultModulePath show the server default module path\n");
    ErrorF("-showDefaultLibPath    show the server default library path\n");
    ErrorF
        ("-novtswitch            don't automatically switch VT at reset & exit\n");
    ErrorF("-sharevts              share VTs with another X server\n");
    /* OS-specific usage */
    xf86UseMsg();
    ErrorF("\n");
}

/*
 * xf86LoadModules iterates over a list that is being passed in.
 */
Bool
xf86LoadModules(const char **list, void **optlist)
{
    int errmaj;
    void *opt;
    int i;
    char *name;
    Bool failed = FALSE;

    if (!list)
        return TRUE;

    for (i = 0; list[i] != NULL; i++) {

        /* Normalise the module name */
        name = xf86NormalizeName(list[i]);

        /* Skip empty names */
        if (name == NULL || *name == '\0') {
            free(name);
            continue;
        }

        /* Replace obsolete keyboard driver with kbd */
        if (!xf86NameCmp(name, "keyboard")) {
            strcpy(name, "kbd");
        }

        if (optlist)
            opt = optlist[i];
        else
            opt = NULL;

        if (!LoadModule(name, opt, NULL, &errmaj)) {
            LoaderErrorMsg(NULL, name, errmaj, 0);
            failed = TRUE;
        }
        free(name);
    }
    return !failed;
}

/* Pixmap format stuff */

PixmapFormatPtr
xf86GetPixFormat(ScrnInfoPtr pScrn, int depth)
{
    int i;

    for (i = 0; i < numFormats; i++)
        if (formats[i].depth == depth)
            break;
    if (i != numFormats)
        return &formats[i];
    else if (!formatsDone) {
        /* Check for screen-specified formats */
        for (i = 0; i < pScrn->numFormats; i++)
            if (pScrn->formats[i].depth == depth)
                break;
        if (i != pScrn->numFormats)
            return &pScrn->formats[i];
    }
    return NULL;
}

int
xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth)
{
    PixmapFormatPtr format;

    format = xf86GetPixFormat(pScrn, depth);
    if (format)
        return format->bitsPerPixel;
    else
        return 0;
}

#ifdef DDXBEFORERESET
void
ddxBeforeReset(void)
{
}
#endif

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
    xf86OSInputThreadInit();
}
#endif
