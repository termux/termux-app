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

#ifndef _KDRIVE_H_
#define _KDRIVE_H_

#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#include "shadow.h"
#include "randrstr.h"
#include "globals.h"

#include "xkbstr.h"

#define KD_DPMS_NORMAL	    0
#define KD_DPMS_STANDBY	    1
#define KD_DPMS_SUSPEND	    2
#define KD_DPMS_POWERDOWN   3
#define KD_DPMS_MAX	    KD_DPMS_POWERDOWN

#define Status int

typedef struct _KdCardInfo {
    struct _KdCardFuncs *cfuncs;
    void *closure;
    void *driver;
    struct _KdScreenInfo *screenList;
    int selected;
    struct _KdCardInfo *next;
} KdCardInfo;

extern KdCardInfo *kdCardInfo;

/*
 * Configuration information per X screen
 */
typedef struct _KdFrameBuffer {
    CARD8 *frameBuffer;
    int depth;
    int bitsPerPixel;
    int pixelStride;
    int byteStride;
    Bool shadow;
    unsigned long visuals;
    Pixel redMask, greenMask, blueMask;
    void *closure;
} KdFrameBuffer;

#define RR_Rotate_All	(RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270)
#define RR_Reflect_All	(RR_Reflect_X|RR_Reflect_Y)

typedef struct _KdScreenInfo {
    struct _KdScreenInfo *next;
    KdCardInfo *card;
    ScreenPtr pScreen;
    void *driver;
    Rotation randr;             /* rotation and reflection */
    int x;
    int y;
    int width;
    int height;
    int rate;
    int width_mm;
    int height_mm;
    int subpixel_order;
    Bool dumb;
    Bool softCursor;
    int mynum;
    DDXPointRec origin;
    KdFrameBuffer fb;
} KdScreenInfo;

typedef struct _KdCardFuncs {
    Bool (*cardinit) (KdCardInfo *);    /* detect and map device */
    Bool (*scrinit) (KdScreenInfo *);   /* initialize screen information */
    Bool (*initScreen) (ScreenPtr);     /* initialize ScreenRec */
    Bool (*finishInitScreen) (ScreenPtr pScreen);
    Bool (*createRes) (ScreenPtr);      /* create screen resources */
    void (*scrfini) (KdScreenInfo *);   /* close down screen */
    void (*cardfini) (KdCardInfo *);    /* close down */

    Bool (*initCursor) (ScreenPtr);     /* detect and map cursor */

    Bool (*initAccel) (ScreenPtr);
    void (*enableAccel) (ScreenPtr);
    void (*disableAccel) (ScreenPtr);
    void (*finiAccel) (ScreenPtr);

    void (*getColors) (ScreenPtr, int, xColorItem *);
    void (*putColors) (ScreenPtr, int, xColorItem *);

    void (*closeScreen) (ScreenPtr);    /* close ScreenRec */
} KdCardFuncs;

#define KD_MAX_PSEUDO_DEPTH 8
#define KD_MAX_PSEUDO_SIZE	    (1 << KD_MAX_PSEUDO_DEPTH)

typedef struct {
    KdScreenInfo *screen;
    KdCardInfo *card;

    Bool enabled;
    Bool closed;
    int bytesPerPixel;

    int dpmsState;

    ColormapPtr pInstalledmap;  /* current colormap */
    xColorItem systemPalette[KD_MAX_PSEUDO_SIZE];       /* saved windows colors */

    CreateScreenResourcesProcPtr CreateScreenResources;
    CloseScreenProcPtr CloseScreen;
} KdPrivScreenRec, *KdPrivScreenPtr;

typedef enum _kdPointerState {
    start,
    button_1_pend,
    button_1_down,
    button_2_down,
    button_3_pend,
    button_3_down,
    synth_2_down_13,
    synth_2_down_3,
    synth_2_down_1,
    num_input_states
} KdPointerState;

#define KD_MAX_BUTTON  32

#define KD_KEYBOARD 1
#define KD_MOUSE 2
#define KD_TOUCHSCREEN 3

typedef struct _KdPointerInfo KdPointerInfo;

typedef struct _KdPointerDriver {
    const char *name;
     Status(*Init) (KdPointerInfo *);
     Status(*Enable) (KdPointerInfo *);
    void (*Disable) (KdPointerInfo *);
    void (*Fini) (KdPointerInfo *);
    struct _KdPointerDriver *next;
} KdPointerDriver;

struct _KdPointerInfo {
    DeviceIntPtr dixdev;
    char *name;
    char *path;
    char *protocol;
    InputOption *options;
    int inputClass;

    CARD8 map[KD_MAX_BUTTON + 1];
    int nButtons;
    int nAxes;

    Bool emulateMiddleButton;
    unsigned long emulationTimeout;
    int emulationDx, emulationDy;

    Bool timeoutPending;
    KdPointerState mouseState;
    Bool eventHeld;
    struct {
        int type;
        int x;
        int y;
        int z;
        int flags;
        int absrel;
    } heldEvent;
    unsigned char buttonState;
    Bool transformCoordinates;
    int pressureThreshold;

    KdPointerDriver *driver;
    void *driverPrivate;

    struct _KdPointerInfo *next;
};

void KdAddPointerDriver(KdPointerDriver * driver);
void KdRemovePointerDriver(KdPointerDriver * driver);
KdPointerInfo *KdNewPointer(void);
void KdFreePointer(KdPointerInfo *);
int KdAddPointer(KdPointerInfo * ki);
int KdAddConfigPointer(char *pointer);
void KdRemovePointer(KdPointerInfo * ki);

#define KD_KEY_COUNT 248
#define KD_MIN_KEYCODE  8
#define KD_MAX_KEYCODE  255
#define KD_MAX_WIDTH    4
#define KD_MAX_LENGTH   (KD_MAX_KEYCODE - KD_MIN_KEYCODE + 1)

typedef struct {
    KeySym modsym;
    int modbit;
} KdKeySymModsRec;

typedef struct _KdKeyboardInfo KdKeyboardInfo;

typedef struct _KdKeyboardDriver {
    const char *name;
    Bool (*Init) (KdKeyboardInfo *);
    Bool (*Enable) (KdKeyboardInfo *);
    void (*Leds) (KdKeyboardInfo *, int);
    void (*Bell) (KdKeyboardInfo *, int, int, int);
    void (*Disable) (KdKeyboardInfo *);
    void (*Fini) (KdKeyboardInfo *);
    struct _KdKeyboardDriver *next;
} KdKeyboardDriver;

struct _KdKeyboardInfo {
    struct _KdKeyboardInfo *next;
    DeviceIntPtr dixdev;
    void *closure;
    char *name;
    char *path;
    int inputClass;
    char *xkbRules;
    char *xkbModel;
    char *xkbLayout;
    char *xkbVariant;
    char *xkbOptions;
    int LockLed;

    int minScanCode;
    int maxScanCode;

    int leds;
    int bellPitch;
    int bellDuration;
    InputOption *options;

    KdKeyboardDriver *driver;
    void *driverPrivate;
};

void KdAddKeyboardDriver(KdKeyboardDriver * driver);
void KdRemoveKeyboardDriver(KdKeyboardDriver * driver);
KdKeyboardInfo *KdNewKeyboard(void);
void KdFreeKeyboard(KdKeyboardInfo * ki);
int KdAddConfigKeyboard(char *pointer);
int KdAddKeyboard(KdKeyboardInfo * ki);
void KdRemoveKeyboard(KdKeyboardInfo * ki);

typedef struct _KdPointerMatrix {
    int matrix[2][3];
} KdPointerMatrix;

extern DevPrivateKeyRec kdScreenPrivateKeyRec;

#define kdScreenPrivateKey (&kdScreenPrivateKeyRec)

extern Bool kdEmulateMiddleButton;
extern Bool kdDisableZaphod;

#define KdGetScreenPriv(pScreen) ((KdPrivScreenPtr) \
    dixLookupPrivate(&(pScreen)->devPrivates, kdScreenPrivateKey))
#define KdSetScreenPriv(pScreen,v) \
    dixSetPrivate(&(pScreen)->devPrivates, kdScreenPrivateKey, v)
#define KdScreenPriv(pScreen) KdPrivScreenPtr pScreenPriv = KdGetScreenPriv(pScreen)

/* kcmap.c */
void
 KdEnableColormap(ScreenPtr pScreen);

void
 KdDisableColormap(ScreenPtr pScreen);

void
 KdInstallColormap(ColormapPtr pCmap);

void
 KdUninstallColormap(ColormapPtr pCmap);

int
 KdListInstalledColormaps(ScreenPtr pScreen, Colormap * pCmaps);

void
 KdStoreColors(ColormapPtr pCmap, int ndef, xColorItem * pdefs);

/* kdrive.c */
extern miPointerScreenFuncRec kdPointerScreenFuncs;

void
 KdDisableScreen(ScreenPtr pScreen);

Bool
 KdEnableScreen(ScreenPtr pScreen);

void
 KdEnableScreens(void);

void
 KdProcessSwitch(void);

Rotation KdAddRotation(Rotation a, Rotation b);

Rotation KdSubRotation(Rotation a, Rotation b);

void
 KdParseScreen(KdScreenInfo * screen, const char *arg);

const char *
KdParseFindNext(const char *cur, const char *delim, char *save, char *last);

void
 KdUseMsg(void);

int
 KdProcessArgument(int argc, char **argv, int i);

void
 KdOsAddInputDrivers(void);

void
 KdInitCard(ScreenInfo * pScreenInfo, KdCardInfo * card, int argc, char **argv);

void
 KdInitOutput(ScreenInfo * pScreenInfo, int argc, char **argv);

void
 KdSetSubpixelOrder(ScreenPtr pScreen, Rotation randr);

void
 KdBacktrace(int signum);

/* kinfo.c */
KdCardInfo *KdCardInfoAdd(KdCardFuncs * funcs, void *closure);

KdCardInfo *KdCardInfoLast(void);

void
 KdCardInfoDispose(KdCardInfo * ci);

KdScreenInfo *KdScreenInfoAdd(KdCardInfo * ci);

void
 KdScreenInfoDispose(KdScreenInfo * si);

/* kinput.c */
void
 KdInitInput(void);
void
 KdCloseInput(void);

void
KdEnqueueKeyboardEvent(KdKeyboardInfo * ki, unsigned char scan_code,
                       unsigned char is_up);

#define KD_BUTTON_1	0x01
#define KD_BUTTON_2	0x02
#define KD_BUTTON_3	0x04
#define KD_BUTTON_4	0x08
#define KD_BUTTON_5	0x10
#define KD_BUTTON_8	0x80
#define KD_POINTER_DESKTOP 0x40000000
#define KD_MOUSE_DELTA	0x80000000

void
KdEnqueuePointerEvent(KdPointerInfo * pi, unsigned long flags, int rx, int ry,
                      int rz);

void
 KdSetPointerMatrix(KdPointerMatrix *pointer);

void
KdComputePointerMatrix(KdPointerMatrix *pointer, Rotation randr, int width,
                       int height);

void
KdBlockHandler(ScreenPtr pScreen, void *timeout);

void
KdWakeupHandler(ScreenPtr pScreen, int result);

void
 KdDisableInput(void);

void
 KdEnableInput(void);

/* kshadow.c */
Bool
 KdShadowFbAlloc(KdScreenInfo * screen, Bool rotate);

void
 KdShadowFbFree(KdScreenInfo * screen);

Bool

KdShadowSet(ScreenPtr pScreen, int randr, ShadowUpdateProc update,
            ShadowWindowProc window);

void
 KdShadowUnset(ScreenPtr pScreen);

/* function prototypes to be implemented by the drivers */
void
 InitCard(char *name);

#endif                          /* _KDRIVE_H_ */
