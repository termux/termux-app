/*
 * Copyright © 1999 Keith Packard
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
#include "kdrive.h"
#include <mivalidate.h>
#include <dixstruct.h>
#include "privates.h"
#ifdef RANDR
#include <randrstr.h>
#endif
#include "glx_extinit.h"

#ifdef XV
#include "kxv.h"
#endif

#ifdef DPMSExtension
#include "dpmsproc.h"
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#if defined(CONFIG_UDEV) || defined(CONFIG_HAL)
#include <hotplug.h>
#endif

/* This stub can be safely removed once we can
 * split input and GPU parts in hotplug.h et al. */
#include <systemd-logind.h>

typedef struct _kdDepths {
    CARD8 depth;
    CARD8 bpp;
} KdDepths;

KdDepths kdDepths[] = {
    {1, 1},
    {4, 4},
    {8, 8},
    {15, 16},
    {16, 16},
    {24, 32},
    {32, 32}
};

#define KD_DEFAULT_BUTTONS 5

DevPrivateKeyRec kdScreenPrivateKeyRec;
static unsigned long kdGeneration;

Bool kdEmulateMiddleButton;
Bool kdRawPointerCoordinates;
Bool kdDisableZaphod;
static Bool kdEnabled;
static int kdSubpixelOrder;
static char *kdSwitchCmd;
static DDXPointRec kdOrigin;
Bool kdHasPointer = FALSE;
Bool kdHasKbd = FALSE;
const char *kdGlobalXkbRules = NULL;
const char *kdGlobalXkbModel = NULL;
const char *kdGlobalXkbLayout = NULL;
const char *kdGlobalXkbVariant = NULL;
const char *kdGlobalXkbOptions = NULL;

void
KdDisableScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);

    if (!pScreenPriv->enabled)
        return;
    if (!pScreenPriv->closed)
        SetRootClip(pScreen, ROOT_CLIP_NONE);
    KdDisableColormap(pScreen);
    if (!pScreenPriv->screen->dumb && pScreenPriv->card->cfuncs->disableAccel)
        (*pScreenPriv->card->cfuncs->disableAccel) (pScreen);
    pScreenPriv->enabled = FALSE;
}

static void
KdDoSwitchCmd(const char *reason)
{
    if (kdSwitchCmd) {
        char *command;
        int ret;

        if (asprintf(&command, "%s %s", kdSwitchCmd, reason) == -1)
            return;

        /* Ignore the return value from system; I'm not sure
         * there's anything more useful to be done when
         * it fails
         */
        ret = system(command);
        (void) ret;
        free(command);
    }
}

static void
KdSuspend(void)
{
    KdCardInfo *card;
    KdScreenInfo *screen;

    if (kdEnabled) {
        for (card = kdCardInfo; card; card = card->next) {
            for (screen = card->screenList; screen; screen = screen->next)
                if (screen->mynum == card->selected && screen->pScreen)
                    KdDisableScreen(screen->pScreen);
        }
        KdDisableInput();
        KdDoSwitchCmd("suspend");
    }
}

static void
KdDisableScreens(void)
{
    KdSuspend();
    kdEnabled = FALSE;
}

Bool
KdEnableScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);

    if (pScreenPriv->enabled)
        return TRUE;
    pScreenPriv->enabled = TRUE;
    pScreenPriv->dpmsState = KD_DPMS_NORMAL;
    pScreenPriv->card->selected = pScreenPriv->screen->mynum;
    if (!pScreenPriv->screen->dumb && pScreenPriv->card->cfuncs->enableAccel)
        (*pScreenPriv->card->cfuncs->enableAccel) (pScreen);
    KdEnableColormap(pScreen);
    SetRootClip(pScreen, ROOT_CLIP_FULL);
    return TRUE;
}

void
ddxGiveUp(enum ExitCode error)
{
    KdDisableScreens();
}

static Bool kdDumbDriver;
static Bool kdSoftCursor;

const char *
KdParseFindNext(const char *cur, const char *delim, char *save, char *last)
{
    while (*cur && !strchr(delim, *cur)) {
        *save++ = *cur++;
    }
    *save = 0;
    *last = *cur;
    if (*cur)
        cur++;
    return cur;
}

Rotation
KdAddRotation(Rotation a, Rotation b)
{
    Rotation rotate = (a & RR_Rotate_All) * (b & RR_Rotate_All);
    Rotation reflect = (a & RR_Reflect_All) ^ (b & RR_Reflect_All);

    if (rotate > RR_Rotate_270)
        rotate /= (RR_Rotate_270 * RR_Rotate_90);
    return reflect | rotate;
}

Rotation
KdSubRotation(Rotation a, Rotation b)
{
    Rotation rotate = (a & RR_Rotate_All) * 16 / (b & RR_Rotate_All);
    Rotation reflect = (a & RR_Reflect_All) ^ (b & RR_Reflect_All);

    if (rotate > RR_Rotate_270)
        rotate /= (RR_Rotate_270 * RR_Rotate_90);
    return reflect | rotate;
}

void
KdParseScreen(KdScreenInfo * screen, const char *arg)
{
    char delim;
    char save[1024];
    int i;
    int pixels, mm;

    screen->dumb = kdDumbDriver;
    screen->softCursor = kdSoftCursor;
    screen->origin = kdOrigin;
    screen->randr = RR_Rotate_0;
    screen->x = 0;
    screen->y = 0;
    screen->width = 0;
    screen->height = 0;
    screen->width_mm = 0;
    screen->height_mm = 0;
    screen->subpixel_order = kdSubpixelOrder;
    screen->rate = 0;
    screen->fb.depth = 0;
    if (!arg)
        return;
    if (strlen(arg) >= sizeof(save))
        return;

    for (i = 0; i < 2; i++) {
        arg = KdParseFindNext(arg, "x/+@XY", save, &delim);
        if (!save[0])
            return;

        pixels = atoi(save);
        mm = 0;

        if (delim == '/') {
            arg = KdParseFindNext(arg, "x+@XY", save, &delim);
            if (!save[0])
                return;
            mm = atoi(save);
        }

        if (i == 0) {
            screen->width = pixels;
            screen->width_mm = mm;
        }
        else {
            screen->height = pixels;
            screen->height_mm = mm;
        }
        if (delim != 'x' && delim != '+' && delim != '@' &&
            delim != 'X' && delim != 'Y' &&
            (delim != '\0' || i == 0))
            return;
    }

    kdOrigin.x += screen->width;
    kdOrigin.y = 0;
    kdDumbDriver = FALSE;
    kdSoftCursor = FALSE;
    kdSubpixelOrder = SubPixelUnknown;

    if (delim == '+') {
        arg = KdParseFindNext(arg, "+@xXY", save, &delim);
        if (save[0])
            screen->x = atoi(save);
    }

    if (delim == '+') {
        arg = KdParseFindNext(arg, "@xXY", save, &delim);
        if (save[0])
            screen->y = atoi(save);
    }

    if (delim == '@') {
        arg = KdParseFindNext(arg, "xXY", save, &delim);
        if (save[0]) {
            int rotate = atoi(save);

            if (rotate < 45)
                screen->randr = RR_Rotate_0;
            else if (rotate < 135)
                screen->randr = RR_Rotate_90;
            else if (rotate < 225)
                screen->randr = RR_Rotate_180;
            else if (rotate < 315)
                screen->randr = RR_Rotate_270;
            else
                screen->randr = RR_Rotate_0;
        }
    }
    if (delim == 'X') {
        arg = KdParseFindNext(arg, "xY", save, &delim);
        screen->randr |= RR_Reflect_X;
    }

    if (delim == 'Y') {
        arg = KdParseFindNext(arg, "xY", save, &delim);
        screen->randr |= RR_Reflect_Y;
    }

    arg = KdParseFindNext(arg, "x/,", save, &delim);
    if (save[0]) {
        screen->fb.depth = atoi(save);
        if (delim == '/') {
            arg = KdParseFindNext(arg, "x,", save, &delim);
            if (save[0])
                screen->fb.bitsPerPixel = atoi(save);
        }
        else
            screen->fb.bitsPerPixel = 0;
    }

    if (delim == 'x') {
        arg = KdParseFindNext(arg, "x", save, &delim);
        if (save[0])
            screen->rate = atoi(save);
    }
}

static void
KdParseRgba(char *rgba)
{
    if (!strcmp(rgba, "rgb"))
        kdSubpixelOrder = SubPixelHorizontalRGB;
    else if (!strcmp(rgba, "bgr"))
        kdSubpixelOrder = SubPixelHorizontalBGR;
    else if (!strcmp(rgba, "vrgb"))
        kdSubpixelOrder = SubPixelVerticalRGB;
    else if (!strcmp(rgba, "vbgr"))
        kdSubpixelOrder = SubPixelVerticalBGR;
    else if (!strcmp(rgba, "none"))
        kdSubpixelOrder = SubPixelNone;
    else
        kdSubpixelOrder = SubPixelUnknown;
}

void
KdUseMsg(void)
{
    ErrorF("\nTinyX Device Dependent Usage:\n");
    ErrorF
        ("-screen WIDTH[/WIDTHMM]xHEIGHT[/HEIGHTMM][+[-]XOFFSET][+[-]YOFFSET][@ROTATION][X][Y][xDEPTH/BPP[xFREQ]]  Specify screen characteristics\n");
    ErrorF
        ("-rgba rgb/bgr/vrgb/vbgr/none   Specify subpixel ordering for LCD panels\n");
    ErrorF
        ("-mouse driver [,n,,options]    Specify the pointer driver and its options (n is the number of buttons)\n");
    ErrorF
        ("-keybd driver [,,options]      Specify the keyboard driver and its options\n");
    ErrorF("-xkb-rules       Set default XkbRules value (can be overridden by -keybd options)\n");
    ErrorF("-xkb-model       Set default XkbModel value (can be overridden by -keybd options)\n");
    ErrorF("-xkb-layout      Set default XkbLayout value (can be overridden by -keybd options)\n");
    ErrorF("-xkb-variant     Set default XkbVariant value (can be overridden by -keybd options)\n");
    ErrorF("-xkb-options     Set default XkbOptions value (can be overridden by -keybd options)\n");
    ErrorF("-zaphod          Disable cursor screen switching\n");
    ErrorF("-2button         Emulate 3 button mouse\n");
    ErrorF("-3button         Disable 3 button mouse emulation\n");
    ErrorF
        ("-rawcoord        Don't transform pointer coordinates on rotation\n");
    ErrorF("-dumb            Disable hardware acceleration\n");
    ErrorF("-softCursor      Force software cursor\n");
    ErrorF("-videoTest       Start the server, pause momentarily and exit\n");
    ErrorF
        ("-origin X,Y      Locates the next screen in the the virtual screen (Xinerama)\n");
    ErrorF("-switchCmd       Command to execute on vt switch\n");
    ErrorF
        ("vtxx             Use virtual terminal xx instead of the next available\n");
}

int
KdProcessArgument(int argc, char **argv, int i)
{
    KdCardInfo *card;
    KdScreenInfo *screen;

    if (!strcmp(argv[i], "-screen")) {
        if ((i + 1) < argc) {
            card = KdCardInfoLast();
            if (!card) {
                InitCard(0);
                card = KdCardInfoLast();
            }
            if (card) {
                screen = KdScreenInfoAdd(card);
                KdParseScreen(screen, argv[i + 1]);
            }
            else
                ErrorF("No matching card found!\n");
        }
        else
            UseMsg();
        return 2;
    }
    if (!strcmp(argv[i], "-zaphod")) {
        kdDisableZaphod = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-3button")) {
        kdEmulateMiddleButton = FALSE;
        return 1;
    }
    if (!strcmp(argv[i], "-2button")) {
        kdEmulateMiddleButton = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-rawcoord")) {
        kdRawPointerCoordinates = 1;
        return 1;
    }
    if (!strcmp(argv[i], "-dumb")) {
        kdDumbDriver = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-softCursor")) {
        kdSoftCursor = TRUE;
        return 1;
    }
    if (!strcmp(argv[i], "-origin")) {
        if ((i + 1) < argc) {
            char *x = argv[i + 1];
            char *y = strchr(x, ',');

            if (x)
                kdOrigin.x = atoi(x);
            else
                kdOrigin.x = 0;
            if (y)
                kdOrigin.y = atoi(y + 1);
            else
                kdOrigin.y = 0;
        }
        else
            UseMsg();
        return 2;
    }
    if (!strcmp(argv[i], "-rgba")) {
        if ((i + 1) < argc)
            KdParseRgba(argv[i + 1]);
        else
            UseMsg();
        return 2;
    }
    if (!strcmp(argv[i], "-switchCmd")) {
        if ((i + 1) < argc)
            kdSwitchCmd = argv[i + 1];
        else
            UseMsg();
        return 2;
    }
    if (!strcmp(argv[i], "-xkb-rules")) {
        if (i + 1 >= argc) {
            UseMsg();
            FatalError("Missing argument for option -xkb-rules.\n");
        }
        kdGlobalXkbRules = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-xkb-model")) {
        if (i + 1 >= argc) {
            UseMsg();
            FatalError("Missing argument for option -xkb-model.\n");
        }
        kdGlobalXkbModel = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-xkb-layout")) {
        if (i + 1 >= argc) {
            UseMsg();
            FatalError("Missing argument for option -xkb-layout.\n");
        }
        kdGlobalXkbLayout = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-xkb-variant")) {
        if (i + 1 >= argc) {
            UseMsg();
            FatalError("Missing argument for option -xkb-variant.\n");
        }
        kdGlobalXkbVariant = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-xkb-options")) {
        if (i + 1 >= argc) {
            UseMsg();
            FatalError("Missing argument for option -xkb-options.\n");
        }
        kdGlobalXkbOptions = argv[i + 1];
        return 2;
    }
    if (!strcmp(argv[i], "-mouse") || !strcmp(argv[i], "-pointer")) {
        if (i + 1 >= argc)
            UseMsg();
        KdAddConfigPointer(argv[i + 1]);
        kdHasPointer = TRUE;
        return 2;
    }
    if (!strcmp(argv[i], "-keybd")) {
        if (i + 1 >= argc)
            UseMsg();
        KdAddConfigKeyboard(argv[i + 1]);
        kdHasKbd = TRUE;
        return 2;
    }

    return 0;
}

static Bool
KdAllocatePrivates(ScreenPtr pScreen)
{
    KdPrivScreenPtr pScreenPriv;

    if (kdGeneration != serverGeneration)
        kdGeneration = serverGeneration;

    if (!dixRegisterPrivateKey(&kdScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    pScreenPriv = calloc(1, sizeof(*pScreenPriv));
    if (!pScreenPriv)
        return FALSE;
    KdSetScreenPriv(pScreen, pScreenPriv);
    return TRUE;
}

static Bool
KdCreateScreenResources(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdCardInfo *card = pScreenPriv->card;
    Bool ret;

    pScreen->CreateScreenResources = pScreenPriv->CreateScreenResources;
    if (pScreen->CreateScreenResources)
        ret = (*pScreen->CreateScreenResources) (pScreen);
    else
        ret = -1;
    pScreenPriv->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = KdCreateScreenResources;
    if (ret && card->cfuncs->createRes)
        ret = (*card->cfuncs->createRes) (pScreen);
    return ret;
}

static Bool
KdCloseScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    KdCardInfo *card = pScreenPriv->card;
    Bool ret;

    if (card->cfuncs->closeScreen)
        (*card->cfuncs->closeScreen)(pScreen);

    pScreenPriv->closed = TRUE;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    if (pScreen->CloseScreen)
        ret = (*pScreen->CloseScreen) (pScreen);
    else
        ret = TRUE;

    if (screen->mynum == card->selected)
        KdDisableScreen(pScreen);

    if (!pScreenPriv->screen->dumb && card->cfuncs->finiAccel)
        (*card->cfuncs->finiAccel) (pScreen);

    if (card->cfuncs->scrfini)
        (*card->cfuncs->scrfini) (screen);

    /*
     * Clean up card when last screen is closed, DIX closes them in
     * reverse order, thus we check for when the first in the list is closed
     */
    if (screen == card->screenList) {
        if (card->cfuncs->cardfini)
            (*card->cfuncs->cardfini) (card);
        /*
         * Clean up OS when last card is closed
         */
        if (card == kdCardInfo) {
            kdEnabled = FALSE;
        }
    }

    pScreenPriv->screen->pScreen = 0;

    free((void *) pScreenPriv);
    return ret;
}

static Bool
KdSaveScreen(ScreenPtr pScreen, int on)
{
    return FALSE;
}

static Bool
KdCreateWindow(WindowPtr pWin)
{
#ifndef PHOENIX
    if (!pWin->parent) {
        KdScreenPriv(pWin->drawable.pScreen);

        if (!pScreenPriv->enabled) {
            RegionEmpty(&pWin->borderClip);
            RegionBreak(&pWin->clipList);
        }
    }
#endif
    return fbCreateWindow(pWin);
}

void
KdSetSubpixelOrder(ScreenPtr pScreen, Rotation randr)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    int subpixel_order = screen->subpixel_order;
    Rotation subpixel_dir;
    int i;

    static struct {
        int subpixel_order;
        Rotation direction;
    } orders[] = {
        {SubPixelHorizontalRGB, RR_Rotate_0},
        {SubPixelHorizontalBGR, RR_Rotate_180},
        {SubPixelVerticalRGB, RR_Rotate_270},
        {SubPixelVerticalBGR, RR_Rotate_90},
    };

    static struct {
        int bit;
        int normal;
        int reflect;
    } reflects[] = {
        {RR_Reflect_X, SubPixelHorizontalRGB, SubPixelHorizontalBGR},
        {RR_Reflect_X, SubPixelHorizontalBGR, SubPixelHorizontalRGB},
        {RR_Reflect_Y, SubPixelVerticalRGB, SubPixelVerticalBGR},
        {RR_Reflect_Y, SubPixelVerticalRGB, SubPixelVerticalRGB},
    };

    /* map subpixel to direction */
    for (i = 0; i < 4; i++)
        if (orders[i].subpixel_order == subpixel_order)
            break;
    if (i < 4) {
        subpixel_dir =
            KdAddRotation(randr & RR_Rotate_All, orders[i].direction);

        /* map back to subpixel order */
        for (i = 0; i < 4; i++)
            if (orders[i].direction & subpixel_dir) {
                subpixel_order = orders[i].subpixel_order;
                break;
            }
        /* reflect */
        for (i = 0; i < 4; i++)
            if ((randr & reflects[i].bit) &&
                reflects[i].normal == subpixel_order) {
                subpixel_order = reflects[i].reflect;
                break;
            }
    }
    PictureSetSubpixelOrder(pScreen, subpixel_order);
}

/* Pass through AddScreen, which doesn't take any closure */
static KdScreenInfo *kdCurrentScreen;

static Bool
KdScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    KdScreenInfo *screen = kdCurrentScreen;
    KdCardInfo *card = screen->card;
    KdPrivScreenPtr pScreenPriv;

    /*
     * note that screen->fb is set up for the nominal orientation
     * of the screen; that means if randr is rotated, the values
     * there should reflect a rotated frame buffer (or shadow).
     */
    Bool rotated = (screen->randr & (RR_Rotate_90 | RR_Rotate_270)) != 0;
    int width, height, *width_mmp, *height_mmp;

    KdAllocatePrivates(pScreen);

    pScreenPriv = KdGetScreenPriv(pScreen);

    if (!rotated) {
        width = screen->width;
        height = screen->height;
        width_mmp = &screen->width_mm;
        height_mmp = &screen->height_mm;
    }
    else {
        width = screen->height;
        height = screen->width;
        width_mmp = &screen->height_mm;
        height_mmp = &screen->width_mm;
    }
    screen->pScreen = pScreen;
    pScreenPriv->screen = screen;
    pScreenPriv->card = card;
    pScreenPriv->bytesPerPixel = screen->fb.bitsPerPixel >> 3;
    pScreenPriv->dpmsState = KD_DPMS_NORMAL;
    pScreen->x = screen->origin.x;
    pScreen->y = screen->origin.y;

    if (!monitorResolution)
        monitorResolution = 75;
    /*
     * This is done in this order so that backing store wraps
     * our GC functions; fbFinishScreenInit initializes MI
     * backing store
     */
    if (!fbSetupScreen(pScreen,
                       screen->fb.frameBuffer,
                       width, height,
                       monitorResolution, monitorResolution,
                       screen->fb.pixelStride, screen->fb.bitsPerPixel)) {
        return FALSE;
    }

    /*
     * Set colormap functions
     */
    pScreen->InstallColormap = KdInstallColormap;
    pScreen->UninstallColormap = KdUninstallColormap;
    pScreen->ListInstalledColormaps = KdListInstalledColormaps;
    pScreen->StoreColors = KdStoreColors;

    pScreen->SaveScreen = KdSaveScreen;
    pScreen->CreateWindow = KdCreateWindow;

    if (!fbFinishScreenInit(pScreen,
                            screen->fb.frameBuffer,
                            width, height,
                            monitorResolution, monitorResolution,
                            screen->fb.pixelStride, screen->fb.bitsPerPixel)) {
        return FALSE;
    }

    /*
     * Fix screen sizes; for some reason mi takes dpi instead of mm.
     * Rounding errors are annoying
     */
    if (*width_mmp)
        pScreen->mmWidth = *width_mmp;
    else
        *width_mmp = pScreen->mmWidth;
    if (*height_mmp)
        pScreen->mmHeight = *height_mmp;
    else
        *height_mmp = pScreen->mmHeight;

    /*
     * Plug in our own block/wakeup handlers.
     * miScreenInit installs NoopDDA in both places
     */
    pScreen->BlockHandler = KdBlockHandler;
    pScreen->WakeupHandler = KdWakeupHandler;

    if (!fbPictureInit(pScreen, 0, 0))
        return FALSE;
    if (card->cfuncs->initScreen)
        if (!(*card->cfuncs->initScreen) (pScreen))
            return FALSE;

    if (!screen->dumb && card->cfuncs->initAccel)
        if (!(*card->cfuncs->initAccel) (pScreen))
            screen->dumb = TRUE;

    if (card->cfuncs->finishInitScreen)
        if (!(*card->cfuncs->finishInitScreen) (pScreen))
            return FALSE;

    /*
     * Wrap CloseScreen, the order now is:
     *  KdCloseScreen
     *  fbCloseScreen
     */
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = KdCloseScreen;

    pScreenPriv->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = KdCreateScreenResources;

    if (screen->softCursor ||
        !card->cfuncs->initCursor || !(*card->cfuncs->initCursor) (pScreen)) {
        /* Use MI for cursor display and event queueing. */
        screen->softCursor = TRUE;
        miDCInitialize(pScreen, &kdPointerScreenFuncs);
    }

    if (!fbCreateDefColormap(pScreen)) {
        return FALSE;
    }

    KdSetSubpixelOrder(pScreen, screen->randr);

    /*
     * Enable the hardware
     */
    kdEnabled = TRUE;

    if (screen->mynum == card->selected) {
        pScreenPriv->enabled = TRUE;
        KdEnableColormap(pScreen);
        if (!screen->dumb && card->cfuncs->enableAccel)
            (*card->cfuncs->enableAccel) (pScreen);
    }

    return TRUE;
}

static void
KdInitScreen(ScreenInfo * pScreenInfo,
             KdScreenInfo * screen, int argc, char **argv)
{
    KdCardInfo *card = screen->card;

    if (!(*card->cfuncs->scrinit) (screen))
        FatalError("Screen initialization failed!\n");

    if (!card->cfuncs->initAccel)
        screen->dumb = TRUE;
    if (!card->cfuncs->initCursor)
        screen->softCursor = TRUE;
}

static Bool
KdSetPixmapFormats(ScreenInfo * pScreenInfo)
{
    CARD8 depthToBpp[33];       /* depth -> bpp map */
    KdCardInfo *card;
    KdScreenInfo *screen;
    int i;
    int bpp;
    PixmapFormatRec *format;

    for (i = 1; i <= 32; i++)
        depthToBpp[i] = 0;

    /*
     * Generate mappings between bitsPerPixel and depth,
     * also ensure that all screens comply with protocol
     * restrictions on equivalent formats for the same
     * depth on different screens
     */
    for (card = kdCardInfo; card; card = card->next) {
        for (screen = card->screenList; screen; screen = screen->next) {
            bpp = screen->fb.bitsPerPixel;
            if (bpp == 24)
                bpp = 32;
            if (!depthToBpp[screen->fb.depth])
                depthToBpp[screen->fb.depth] = bpp;
            else if (depthToBpp[screen->fb.depth] != bpp)
                return FALSE;
        }
    }

    /*
     * Fill in additional formats
     */
    for (i = 0; i < ARRAY_SIZE(kdDepths); i++)
        if (!depthToBpp[kdDepths[i].depth])
            depthToBpp[kdDepths[i].depth] = kdDepths[i].bpp;

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    pScreenInfo->numPixmapFormats = 0;

    for (i = 1; i <= 32; i++) {
        if (depthToBpp[i]) {
            format = &pScreenInfo->formats[pScreenInfo->numPixmapFormats++];
            format->depth = i;
            format->bitsPerPixel = depthToBpp[i];
            format->scanlinePad = BITMAP_SCANLINE_PAD;
        }
    }

    return TRUE;
}

static void
KdAddScreen(ScreenInfo * pScreenInfo,
            KdScreenInfo * screen, int argc, char **argv)
{
    int i;

    /*
     * Fill in fb visual type masks for this screen
     */
    for (i = 0; i < pScreenInfo->numPixmapFormats; i++) {
        unsigned long visuals;
        Pixel rm, gm, bm;

        visuals = 0;
        rm = gm = bm = 0;
        if (pScreenInfo->formats[i].depth == screen->fb.depth) {
            visuals = screen->fb.visuals;
            rm = screen->fb.redMask;
            gm = screen->fb.greenMask;
            bm = screen->fb.blueMask;
        }
        fbSetVisualTypesAndMasks(pScreenInfo->formats[i].depth,
                                 visuals, 8, rm, gm, bm);
    }

    kdCurrentScreen = screen;

    AddScreen(KdScreenInit, argc, argv);
}

void
KdInitOutput(ScreenInfo * pScreenInfo, int argc, char **argv)
{
    KdCardInfo *card;
    KdScreenInfo *screen;

    if (!kdCardInfo) {
        InitCard(0);
        if (!(card = KdCardInfoLast()))
            FatalError("No matching cards found!\n");
        screen = KdScreenInfoAdd(card);
        KdParseScreen(screen, 0);
    }
    /*
     * Initialize all of the screens for all of the cards
     */
    for (card = kdCardInfo; card; card = card->next) {
        int ret = 1;

        if (card->cfuncs->cardinit)
            ret = (*card->cfuncs->cardinit) (card);
        if (ret) {
            for (screen = card->screenList; screen; screen = screen->next)
                KdInitScreen(pScreenInfo, screen, argc, argv);
        }
    }

    /*
     * Merge the various pixmap formats together, this can fail
     * when two screens share depth but not bitsPerPixel
     */
    if (!KdSetPixmapFormats(pScreenInfo))
        return;

    /*
     * Add all of the screens
     */
    for (card = kdCardInfo; card; card = card->next)
        for (screen = card->screenList; screen; screen = screen->next)
            KdAddScreen(pScreenInfo, screen, argc, argv);

    xorgGlxCreateVendor();

#if defined(CONFIG_UDEV) || defined(CONFIG_HAL)
    if (SeatId) /* Enable input hot-plugging */
        config_pre_init();
#endif
}

void
OsVendorFatalError(const char *f, va_list args)
{
}

/* These stubs can be safely removed once we can
 * split input and GPU parts in hotplug.h et al. */
#ifdef CONFIG_UDEV_KMS
void
NewGPUDeviceRequest(struct OdevAttributes *attribs)
{
}

void
DeleteGPUDeviceRequest(struct OdevAttributes *attribs)
{
}
#endif

#if defined(CONFIG_UDEV) || defined(CONFIG_HAL)
struct xf86_platform_device *
xf86_find_platform_device_by_devnum(int major, int minor)
{
    return NULL;
}
#endif

#ifdef SYSTEMD_LOGIND
void
systemd_logind_vtenter(void)
{
}

void
systemd_logind_release_fd(int major, int minor, int fd)
{
    close(fd);
}
#endif
