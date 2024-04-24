/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Keyboard.h"
#include "Handlers.h"
#include "Events.h"
#include "Init.h"
#include "Args.h"
#include "Drawable.h"
#include "XNGC.h"
#include "XNFont.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif

Bool xnestDoFullGeneration = True;

#ifdef GLXEXT
void
GlxExtensionInit(void)
{
}
#endif

void
InitOutput(ScreenInfo * screen_info, int argc, char *argv[])
{
    int i, j;

    xnestOpenDisplay(argc, argv);

    screen_info->imageByteOrder = ImageByteOrder(xnestDisplay);
    screen_info->bitmapScanlineUnit = BitmapUnit(xnestDisplay);
    screen_info->bitmapScanlinePad = BitmapPad(xnestDisplay);
    screen_info->bitmapBitOrder = BitmapBitOrder(xnestDisplay);

    screen_info->numPixmapFormats = 0;
    for (i = 0; i < xnestNumPixmapFormats; i++)
        for (j = 0; j < xnestNumDepths; j++)
            if ((xnestPixmapFormats[i].depth == 1) ||
                (xnestPixmapFormats[i].depth == xnestDepths[j])) {
                screen_info->formats[screen_info->numPixmapFormats].depth =
                    xnestPixmapFormats[i].depth;
                screen_info->formats[screen_info->numPixmapFormats].bitsPerPixel =
                    xnestPixmapFormats[i].bits_per_pixel;
                screen_info->formats[screen_info->numPixmapFormats].scanlinePad =
                    xnestPixmapFormats[i].scanline_pad;
                screen_info->numPixmapFormats++;
                break;
            }

    xnestFontPrivateIndex = xfont2_allocate_font_private_index();

    if (!xnestNumScreens)
        xnestNumScreens = 1;

    for (i = 0; i < xnestNumScreens; i++)
        AddScreen(xnestOpenScreen, argc, argv);

    xnestNumScreens = screen_info->numScreens;

    xnestDoFullGeneration = xnestFullGeneration;
}

static void
xnestNotifyConnection(int fd, int ready, void *data)
{
    xnestCollectEvents();
}

void
InitInput(int argc, char *argv[])
{
    int rc;

    rc = AllocDevicePair(serverClient, "Xnest",
                         &xnestPointerDevice,
                         &xnestKeyboardDevice,
                         xnestPointerProc, xnestKeyboardProc, FALSE);

    if (rc != Success)
        FatalError("Failed to init Xnest default devices.\n");

    mieqInit();

    SetNotifyFd(XConnectionNumber(xnestDisplay), xnestNotifyConnection, X_NOTIFY_READ, NULL);

    RegisterBlockAndWakeupHandlers(xnestBlockHandler, xnestWakeupHandler, NULL);
}

void
CloseInput(void)
{
    mieqFini();
}

void
ddxGiveUp(enum ExitCode error)
{
    xnestDoFullGeneration = True;
    xnestCloseDisplay();
}

#ifdef __APPLE__
void
DarwinHandleGUI(int argc, char *argv[])
{
}
#endif

void
OsVendorInit(void)
{
    return;
}

void
OsVendorFatalError(const char *f, va_list args)
{
    return;
}

#if defined(DDXBEFORERESET)
void
ddxBeforeReset(void)
{
    return;
}
#endif

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif
