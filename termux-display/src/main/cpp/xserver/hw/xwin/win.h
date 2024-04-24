/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 *		Kensuke Matsuzaki
 */

#ifndef _WIN_H_
#define _WIN_H_

#ifndef NO
#define NO					0
#endif
#ifndef YES
#define YES					1
#endif

/* We can handle WM_MOUSEHWHEEL even though _WIN32_WINNT < 0x0600 */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

/* Turn debug messages on or off */
#ifndef CYGDEBUG
#define CYGDEBUG				NO
#endif

#define WIN_DEFAULT_BPP				0
#define WIN_DEFAULT_WHITEPIXEL			255
#define WIN_DEFAULT_BLACKPIXEL			0
#define WIN_DEFAULT_LINEBIAS			0
#define WIN_DEFAULT_E3B_TIME			50      /* milliseconds */
#define WIN_DEFAULT_DPI				96
#define WIN_DEFAULT_REFRESH			0
#define WIN_DEFAULT_WIN_KILL			TRUE
#define WIN_DEFAULT_UNIX_KILL			FALSE
#define WIN_DEFAULT_CLIP_UPDATES_NBOXES		0
#ifdef XWIN_EMULATEPSEUDO
#define WIN_DEFAULT_EMULATE_PSEUDO		FALSE
#endif
#define WIN_DEFAULT_USER_GAVE_HEIGHT_AND_WIDTH	FALSE

/*
 * Windows only supports 256 color palettes
 */
#define WIN_NUM_PALETTE_ENTRIES			256

/*
 * Number of times to call Restore in an attempt to restore the primary surface
 */
#define WIN_REGAIN_SURFACE_RETRIES		1

/*
 * Build a supported display depths mask by shifting one to the left
 * by the number of bits in the supported depth.
 */
#define WIN_SUPPORTED_BPPS	( (1 << (32 - 1)) | (1 << (24 - 1)) \
				| (1 << (16 - 1)) | (1 << (15 - 1)) \
				| (1 << ( 8 - 1)))
#define WIN_CHECK_DEPTH		YES

/*
 * Timer IDs for WM_TIMER
 */
#define WIN_E3B_TIMER_ID		1
#define WIN_POLLING_MOUSE_TIMER_ID	2

#define MOUSE_POLLING_INTERVAL		50

#define WIN_E3B_OFF		-1
#define WIN_E3B_DEFAULT         0

#define WIN_FD_INVALID		-1

#define WIN_SERVER_NONE		0x0L    /* 0 */
#define WIN_SERVER_SHADOW_GDI	0x1L    /* 1 */
#define WIN_SERVER_SHADOW_DDNL	0x4L    /* 4 */

#define AltMapIndex		Mod1MapIndex
#define NumLockMapIndex		Mod2MapIndex
#define AltLangMapIndex		Mod3MapIndex
#define KanaMapIndex		Mod4MapIndex
#define ScrollLockMapIndex	Mod5MapIndex

#define WIN_MOD_LALT		0x00000001
#define WIN_MOD_RALT		0x00000002
#define WIN_MOD_LCONTROL	0x00000004
#define WIN_MOD_RCONTROL	0x00000008

#define WIN_24BPP_MASK_RED	0x00FF0000
#define WIN_24BPP_MASK_GREEN	0x0000FF00
#define WIN_24BPP_MASK_BLUE	0x000000FF

#define WIN_MAX_KEYS_PER_KEY	4

#define NONAMELESSUNION

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <errno.h>
#define HANDLE void *
#include <pthread.h>
#undef HANDLE

#ifdef HAVE_MMAP
#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE 0
#endif                          /* MAP_FILE */
#endif                          /* HAVE_MMAP */

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include <X11/Xprotostr.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "pixmap.h"
#include "region.h"
#include "gcstruct.h"
#include "colormap.h"
#include "colormapst.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mi.h"
#include "micmap.h"
#include "mifillarc.h"
#include "mifpoly.h"
#include "input.h"
#include "mipointer.h"
#include "X11/keysym.h"
#include "micoord.h"
#include "miline.h"
#include "shadow.h"
#include "fb.h"

#include "mipict.h"
#include "picturestr.h"

#ifdef RANDR
#include "randrstr.h"
#endif

/*
 * Windows headers
 */
#include "winms.h"
#include "winresource.h"

/*
 * Define Windows constants
 */

#define WM_TRAYICON		(WM_USER + 1000)
#define WM_INIT_SYS_MENU	(WM_USER + 1001)
#define WM_GIVEUP		(WM_USER + 1002)

/* Local includes */
#include "winwindow.h"
#include "winmsg.h"

/*
 * Debugging macros
 */

#if CYGDEBUG
#define DEBUG_MSG(str,...) \
if (fDebugProcMsg) \
{ \
  char *pszTemp; \
  int iLength; \
  if (asprintf (&pszTemp, str, ##__VA_ARGS__) != -1) { \
    MessageBox (NULL, pszTemp, szFunctionName, MB_OK); \
    free (pszTemp); \
  } \
}
#else
#define DEBUG_MSG(str,...)
#endif

#if CYGDEBUG
#define DEBUG_FN_NAME(str) PTSTR szFunctionName = str
#else
#define DEBUG_FN_NAME(str)
#endif

#if CYGDEBUG || YES
#define DEBUGVARS BOOL fDebugProcMsg = FALSE
#else
#define DEBUGVARS
#endif

#if CYGDEBUG || YES
#define DEBUGPROC_MSG fDebugProcMsg = TRUE
#else
#define DEBUGPROC_MSG
#endif

#define PROFILEPOINT(point,thresh)\
{\
static unsigned int PROFPT##point = 0;\
if (++PROFPT##point % thresh == 0)\
ErrorF (#point ": PROFILEPOINT hit %u times\n", PROFPT##point);\
}

#define DEFINE_ATOM_HELPER(func,atom_name)			\
static Atom func (void) {					\
    static int generation;					\
    static Atom atom;						\
    if (generation != serverGeneration) {			\
	generation = serverGeneration;				\
	atom = MakeAtom (atom_name, strlen (atom_name), TRUE);	\
    }								\
    return atom;						\
}

/*
 * Typedefs for engine dependent function pointers
 */

typedef Bool (*winAllocateFBProcPtr) (ScreenPtr);

typedef void (*winFreeFBProcPtr) (ScreenPtr);

typedef void (*winShadowUpdateProcPtr) (ScreenPtr, shadowBufPtr);

typedef Bool (*winInitScreenProcPtr) (ScreenPtr);

typedef Bool (*winCloseScreenProcPtr) (ScreenPtr);

typedef Bool (*winInitVisualsProcPtr) (ScreenPtr);

typedef Bool (*winAdjustVideoModeProcPtr) (ScreenPtr);

typedef Bool (*winCreateBoundingWindowProcPtr) (ScreenPtr);

typedef Bool (*winFinishScreenInitProcPtr) (int, ScreenPtr, int, char **);

typedef Bool (*winBltExposedRegionsProcPtr) (ScreenPtr);

typedef Bool (*winBltExposedWindowRegionProcPtr) (ScreenPtr, WindowPtr);

typedef Bool (*winActivateAppProcPtr) (ScreenPtr);

typedef Bool (*winRedrawScreenProcPtr) (ScreenPtr pScreen);

typedef Bool (*winRealizeInstalledPaletteProcPtr) (ScreenPtr pScreen);

typedef Bool (*winInstallColormapProcPtr) (ColormapPtr pColormap);

typedef Bool (*winStoreColorsProcPtr) (ColormapPtr pmap,
                                       int ndef, xColorItem * pdefs);

typedef Bool (*winCreateColormapProcPtr) (ColormapPtr pColormap);

typedef Bool (*winDestroyColormapProcPtr) (ColormapPtr pColormap);

typedef Bool (*winCreatePrimarySurfaceProcPtr) (ScreenPtr);

typedef Bool (*winReleasePrimarySurfaceProcPtr) (ScreenPtr);

typedef Bool (*winCreateScreenResourcesProc) (ScreenPtr);

/*
 * Pixmap privates
 */

typedef struct {
    HBITMAP hBitmap;
    void *pbBits;
    BITMAPINFOHEADER *pbmih;
    BOOL owned;
} winPrivPixmapRec, *winPrivPixmapPtr;

/*
 * Colormap privates
 */

typedef struct {
    HPALETTE hPalette;
    LPDIRECTDRAWPALETTE lpDDPalette;
    RGBQUAD rgbColors[WIN_NUM_PALETTE_ENTRIES];
    PALETTEENTRY peColors[WIN_NUM_PALETTE_ENTRIES];
} winPrivCmapRec, *winPrivCmapPtr;


/*
 * Windows Cursor handling.
 */

typedef struct {
    /* from GetSystemMetrics */
    int sm_cx;
    int sm_cy;

    BOOL visible;
    HCURSOR handle;
    QueryBestSizeProcPtr QueryBestSize;
    miPointerSpriteFuncPtr spriteFuncs;
} winCursorRec;

/*
 * Resize modes
 */
typedef enum {
    resizeDefault = -1,
    resizeNotAllowed,
    resizeWithScrollbars,
    resizeWithRandr
} winResizeMode;

/*
 * Screen information structure that we need before privates are available
 * in the server startup sequence.
 */

typedef struct {
    ScreenPtr pScreen;

    /* Did the user specify a height and width? */
    Bool fUserGaveHeightAndWidth;

    DWORD dwScreen;

    int iMonitor;
    HMONITOR hMonitor;
    DWORD dwUserWidth;
    DWORD dwUserHeight;
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD dwPaddedWidth;

    /* Did the user specify a screen position? */
    Bool fUserGavePosition;
    DWORD dwInitialX;
    DWORD dwInitialY;

    /*
     * dwStride is the number of whole pixels that occupy a scanline,
     * including those pixels that are not displayed.  This is basically
     * a rounding up of the width.
     */
    DWORD dwStride;

    /* Offset of the screen in the window when using scrollbars */
    DWORD dwXOffset;
    DWORD dwYOffset;

    DWORD dwBPP;
    DWORD dwDepth;
    DWORD dwRefreshRate;
    char *pfb;
    DWORD dwEngine;
    DWORD dwEnginePreferred;
    DWORD dwClipUpdatesNBoxes;
#ifdef XWIN_EMULATEPSEUDO
    Bool fEmulatePseudo;
#endif
    Bool fFullScreen;
    Bool fDecoration;
    Bool fRootless;
    Bool fMultiWindow;
    Bool fCompositeWM;
    Bool fMultiMonitorOverride;
    Bool fMultipleMonitors;
    Bool fLessPointer;
    winResizeMode iResizeMode;
    Bool fNoTrayIcon;
    int iE3BTimeout;
    /* Windows (Alt+F4) and Unix (Ctrl+Alt+Backspace) Killkey */
    Bool fUseWinKillKey;
    Bool fUseUnixKillKey;
    Bool fIgnoreInput;

    /* Did the user explicitly set this screen? */
    Bool fExplicitScreen;

    /* Icons for screen window */
    HICON hIcon;
    HICON hIconSm;
} winScreenInfo, *winScreenInfoPtr;

/*
 * Screen privates
 */

typedef struct _winPrivScreenRec {
    winScreenInfoPtr pScreenInfo;

    Bool fEnabled;
    Bool fClosed;
    Bool fActive;
    Bool fBadDepth;

    int iDeltaZ;
    int iDeltaV;

    int iConnectedClients;

    CloseScreenProcPtr CloseScreen;

    DWORD dwRedMask;
    DWORD dwGreenMask;
    DWORD dwBlueMask;
    DWORD dwBitsPerRGB;

    DWORD dwModeKeyStates;

    /* Handle to icons that must be freed */
    HICON hiconNotifyIcon;

    /* Palette management */
    ColormapPtr pcmapInstalled;

    /* Pointer to the root visual so we only have to look it up once */
    VisualPtr pRootVisual;

    /* 3 button emulation variables */
    int iE3BCachedPress;
    Bool fE3BFakeButton2Sent;

    /* Privates used by shadow fb GDI engine */
    HBITMAP hbmpShadow;
    HDC hdcScreen;
    HDC hdcShadow;
    HWND hwndScreen;
    BITMAPINFOHEADER *pbmih;

    /* Privates used by shadow fb DirectDraw Nonlocking engine */
    LPDIRECTDRAW pdd;
    LPDIRECTDRAW4 pdd4;
    LPDIRECTDRAWSURFACE4 pddsShadow4;
    LPDIRECTDRAWSURFACE4 pddsPrimary4;
    LPDIRECTDRAWCLIPPER pddcPrimary;
    BOOL fRetryCreateSurface;

    /* Privates used by multi-window */
    pthread_t ptWMProc;
    pthread_t ptXMsgProc;
    void *pWMInfo;
    Bool fRootWindowShown;

    /* Privates used for any module running in a separate thread */
    pthread_mutex_t pmServerStarted;
    Bool fServerStarted;

    /* Engine specific functions */
    winAllocateFBProcPtr pwinAllocateFB;
    winFreeFBProcPtr pwinFreeFB;
    winShadowUpdateProcPtr pwinShadowUpdate;
    winInitScreenProcPtr pwinInitScreen;
    winCloseScreenProcPtr pwinCloseScreen;
    winInitVisualsProcPtr pwinInitVisuals;
    winAdjustVideoModeProcPtr pwinAdjustVideoMode;
    winCreateBoundingWindowProcPtr pwinCreateBoundingWindow;
    winFinishScreenInitProcPtr pwinFinishScreenInit;
    winBltExposedRegionsProcPtr pwinBltExposedRegions;
    winBltExposedWindowRegionProcPtr pwinBltExposedWindowRegion;
    winActivateAppProcPtr pwinActivateApp;
    winRedrawScreenProcPtr pwinRedrawScreen;
    winRealizeInstalledPaletteProcPtr pwinRealizeInstalledPalette;
    winInstallColormapProcPtr pwinInstallColormap;
    winStoreColorsProcPtr pwinStoreColors;
    winCreateColormapProcPtr pwinCreateColormap;
    winDestroyColormapProcPtr pwinDestroyColormap;
    winCreatePrimarySurfaceProcPtr pwinCreatePrimarySurface;
    winReleasePrimarySurfaceProcPtr pwinReleasePrimarySurface;
    winCreateScreenResourcesProc pwinCreateScreenResources;

    /* Window Procedures for Rootless mode */
    CreateWindowProcPtr CreateWindow;
    DestroyWindowProcPtr DestroyWindow;
    PositionWindowProcPtr PositionWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    ValidateTreeProcPtr ValidateTree;
    PostValidateTreeProcPtr PostValidateTree;
    CopyWindowProcPtr CopyWindow;
    ClearToBackgroundProcPtr ClearToBackground;
    ClipNotifyProcPtr ClipNotify;
    RestackWindowProcPtr RestackWindow;
    ReparentWindowProcPtr ReparentWindow;
    ResizeWindowProcPtr ResizeWindow;
    MoveWindowProcPtr MoveWindow;
    SetShapeProcPtr SetShape;
    ModifyPixmapHeaderProcPtr ModifyPixmapHeader;

    winCursorRec cursor;

    Bool fNativeGlActive;
} winPrivScreenRec;

typedef struct {
    void *value;
    XID id;
} WindowIDPairRec, *WindowIDPairPtr;

/*
 * Extern declares for general global variables
 */

#include "winglobals.h"

extern winScreenInfo *g_ScreenInfo;
extern miPointerScreenFuncRec g_winPointerCursorFuncs;
extern DWORD g_dwEvents;

#ifdef HAS_DEVWINDOWS
extern int g_fdMessageQueue;
#endif
extern DevPrivateKeyRec g_iScreenPrivateKeyRec;

#define g_iScreenPrivateKey  	(&g_iScreenPrivateKeyRec)
extern DevPrivateKeyRec g_iCmapPrivateKeyRec;

#define g_iCmapPrivateKey 	(&g_iCmapPrivateKeyRec)
extern DevPrivateKeyRec g_iGCPrivateKeyRec;

#define g_iGCPrivateKey 	(&g_iGCPrivateKeyRec)
extern DevPrivateKeyRec g_iPixmapPrivateKeyRec;

#define g_iPixmapPrivateKey 	(&g_iPixmapPrivateKeyRec)
extern DevPrivateKeyRec g_iWindowPrivateKeyRec;

#define g_iWindowPrivateKey 	(&g_iWindowPrivateKeyRec)

extern unsigned long g_ulServerGeneration;
extern DWORD g_dwEnginesSupported;
extern HINSTANCE g_hInstance;
extern int g_copyROP[];
extern int g_patternROP[];
extern const char *g_pszQueryHost;
extern DeviceIntPtr g_pwinPointer;
extern DeviceIntPtr g_pwinKeyboard;

/*
 * Extern declares for dynamically loaded library function pointers
 */

extern FARPROC g_fpDirectDrawCreate;
extern FARPROC g_fpDirectDrawCreateClipper;

/*
 * Screen privates macros
 */

#define winGetScreenPriv(pScreen) ((winPrivScreenPtr) \
    dixLookupPrivate(&(pScreen)->devPrivates, g_iScreenPrivateKey))

#define winSetScreenPriv(pScreen,v) \
    dixSetPrivate(&(pScreen)->devPrivates, g_iScreenPrivateKey, v)

#define winScreenPriv(pScreen) \
	winPrivScreenPtr pScreenPriv = winGetScreenPriv(pScreen)

/*
 * Colormap privates macros
 */

#define winGetCmapPriv(pCmap) ((winPrivCmapPtr) \
    dixLookupPrivate(&(pCmap)->devPrivates, g_iCmapPrivateKey))

#define winSetCmapPriv(pCmap,v) \
    dixSetPrivate(&(pCmap)->devPrivates, g_iCmapPrivateKey, v)

#define winCmapPriv(pCmap) \
	winPrivCmapPtr pCmapPriv = winGetCmapPriv(pCmap)

/*
 * GC privates macros
 */

#define winGetGCPriv(pGC) ((winPrivGCPtr) \
    dixLookupPrivate(&(pGC)->devPrivates, g_iGCPrivateKey))

#define winSetGCPriv(pGC,v) \
    dixSetPrivate(&(pGC)->devPrivates, g_iGCPrivateKey, v)

#define winGCPriv(pGC) \
	winPrivGCPtr pGCPriv = winGetGCPriv(pGC)

/*
 * Pixmap privates macros
 */

#define winGetPixmapPriv(pPixmap) ((winPrivPixmapPtr) \
    dixLookupPrivate(&(pPixmap)->devPrivates, g_iPixmapPrivateKey))

#define winSetPixmapPriv(pPixmap,v) \
    dixLookupPrivate(&(pPixmap)->devPrivates, g_iPixmapPrivateKey, v)

#define winPixmapPriv(pPixmap) \
	winPrivPixmapPtr pPixmapPriv = winGetPixmapPriv(pPixmap)

/*
 * Window privates macros
 */

#define winGetWindowPriv(pWin) ((winPrivWinPtr) \
    dixLookupPrivate(&(pWin)->devPrivates, g_iWindowPrivateKey))

#define winSetWindowPriv(pWin,v) \
    dixLookupPrivate(&(pWin)->devPrivates, g_iWindowPrivateKey, v)

#define winWindowPriv(pWin) \
	winPrivWinPtr pWinPriv = winGetWindowPriv(pWin)

/*
 * wrapper macros
 */
#define _WIN_WRAP(priv, real, mem, func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define _WIN_UNWRAP(priv, real, mem) {\
    real->mem = priv->mem; \
}

#define WIN_WRAP(mem, func) _WIN_WRAP(pScreenPriv, pScreen, mem, func)

#define WIN_UNWRAP(mem) _WIN_UNWRAP(pScreenPriv, pScreen, mem)

/*
 * BEGIN DDX and DIX Function Prototypes
 */

/*
 * winallpriv.c
 */

Bool
 winAllocatePrivates(ScreenPtr pScreen);

Bool
 winInitCmapPrivates(ColormapPtr pCmap, int i);

Bool
 winAllocateCmapPrivates(ColormapPtr pCmap);

/*
 * winblock.c
 */

void

winBlockHandler(ScreenPtr pScreen, void *pTimeout);

/*
 * winclipboardinit.c
 */

Bool
 winInitClipboard(void);

void
 winClipboardShutdown(void);

/*
 * wincmap.c
 */

void
 winSetColormapFunctions(ScreenPtr pScreen);

Bool
 winCreateDefColormap(ScreenPtr pScreen);

/*
 * wincreatewnd.c
 */

Bool
 winCreateBoundingWindowFullScreen(ScreenPtr pScreen);

Bool
 winCreateBoundingWindowWindowed(ScreenPtr pScreen);

/*
 * windialogs.c
 */

void
 winDisplayExitDialog(winPrivScreenPtr pScreenPriv);

void
 winDisplayDepthChangeDialog(winPrivScreenPtr pScreenPriv);

void
 winDisplayAboutDialog(winPrivScreenPtr pScreenPriv);

/*
 * winengine.c
 */

void
 winDetectSupportedEngines(void);

Bool
 winSetEngine(ScreenPtr pScreen);

Bool
 winGetDDProcAddresses(void);

void
 winReleaseDDProcAddresses(void);

/*
 * winerror.c
 */

#ifdef DDXOSVERRORF
void
OsVendorVErrorF(const char *pszFormat, va_list va_args)
_X_ATTRIBUTE_PRINTF(1, 0);
#endif

void
winMessageBoxF(const char *pszError, UINT uType, ...)
_X_ATTRIBUTE_PRINTF(1, 3);

/*
 * winglobals.c
 */

void
 winInitializeGlobals(void);

/*
 * winkeybd.c
 */

int
 winTranslateKey(WPARAM wParam, LPARAM lParam);

int
 winKeybdProc(DeviceIntPtr pDeviceInt, int iState);

void
 winInitializeModeKeyStates(void);

void
 winRestoreModeKeyStates(void);

Bool
 winIsFakeCtrl_L(UINT message, WPARAM wParam, LPARAM lParam);

void
 winKeybdReleaseKeys(void);

void
 winSendKeyEvent(DWORD dwKey, Bool fDown);

BOOL winCheckKeyPressed(WPARAM wParam, LPARAM lParam);

void
 winFixShiftKeys(int iScanCode);

/*
 * winkeyhook.c
 */

Bool
 winInstallKeyboardHookLL(void);

void
 winRemoveKeyboardHookLL(void);

/*
 * winmisc.c
 */

CARD8
 winCountBits(DWORD dw);

Bool
 winUpdateFBPointer(ScreenPtr pScreen, void *pbits);

/*
 * winmouse.c
 */

int
 winMouseProc(DeviceIntPtr pDeviceInt, int iState);

int
 winMouseWheel(int *iTotalDeltaZ, int iDeltaZ, int iButtonUp, int iButtonDown);

void
 winMouseButtonsSendEvent(int iEventType, int iButton);

int

winMouseButtonsHandle(ScreenPtr pScreen,
                      int iEventType, int iButton, WPARAM wParam);

void
 winEnqueueMotion(int x, int y);

/*
 * winscrinit.c
 */

Bool
 winScreenInit(ScreenPtr pScreen, int argc, char **argv);

Bool
 winFinishScreenInitFB(int i, ScreenPtr pScreen, int argc, char **argv);

/*
 * winshadddnl.c
 */

Bool
 winSetEngineFunctionsShadowDDNL(ScreenPtr pScreen);

/*
 * winshadgdi.c
 */

Bool
 winSetEngineFunctionsShadowGDI(ScreenPtr pScreen);

/*
 * winwakeup.c
 */

void
winWakeupHandler(ScreenPtr pScreen, int iResult);

/*
 * winwindow.c
 */

Bool
 winCreateWindowRootless(WindowPtr pWindow);

Bool
 winDestroyWindowRootless(WindowPtr pWindow);

Bool
 winPositionWindowRootless(WindowPtr pWindow, int x, int y);

Bool
 winChangeWindowAttributesRootless(WindowPtr pWindow, unsigned long mask);

Bool
 winUnmapWindowRootless(WindowPtr pWindow);

Bool
 winMapWindowRootless(WindowPtr pWindow);

void
 winSetShapeRootless(WindowPtr pWindow, int kind);

/*
 * winmultiwindowshape.c
 */

void
 winReshapeMultiWindow(WindowPtr pWin);

void
 winSetShapeMultiWindow(WindowPtr pWindow, int kind);

void
 winUpdateRgnMultiWindow(WindowPtr pWindow);

/*
 * winmultiwindowwindow.c
 */

Bool
 winCreateWindowMultiWindow(WindowPtr pWindow);

Bool
 winDestroyWindowMultiWindow(WindowPtr pWindow);

Bool
 winPositionWindowMultiWindow(WindowPtr pWindow, int x, int y);

Bool
 winChangeWindowAttributesMultiWindow(WindowPtr pWindow, unsigned long mask);

Bool
 winUnmapWindowMultiWindow(WindowPtr pWindow);

Bool
 winMapWindowMultiWindow(WindowPtr pWindow);

void
 winReparentWindowMultiWindow(WindowPtr pWin, WindowPtr pPriorParent);

void
 winRestackWindowMultiWindow(WindowPtr pWin, WindowPtr pOldNextSib);

void
 winReorderWindowsMultiWindow(void);

void

winResizeWindowMultiWindow(WindowPtr pWin, int x, int y, unsigned int w,
                           unsigned int h, WindowPtr pSib);
void

winMoveWindowMultiWindow(WindowPtr pWin, int x, int y,
                         WindowPtr pSib, VTKind kind);

void

winCopyWindowMultiWindow(WindowPtr pWin, DDXPointRec oldpt,
                         RegionPtr oldRegion);

PixmapPtr
winCreatePixmapMultiwindow(ScreenPtr pScreen, int width, int height, int depth,
                           unsigned usage_hint);
Bool
winDestroyPixmapMultiwindow(PixmapPtr pPixmap);

Bool
winModifyPixmapHeaderMultiwindow(PixmapPtr pPixmap,
                                 int width,
                                 int height,
                                 int depth,
                                 int bitsPerPixel, int devKind, void *pPixData);

XID
 winGetWindowID(WindowPtr pWin);

int
 winAdjustXWindow(WindowPtr pWin, HWND hwnd);

/*
 * winmultiwindowwndproc.c
 */

LRESULT CALLBACK
winTopLevelWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * wintrayicon.c
 */

void
 winInitNotifyIcon(winPrivScreenPtr pScreenPriv);

void
 winDeleteNotifyIcon(winPrivScreenPtr pScreenPriv);

LRESULT
winHandleIconMessage(HWND hwnd, UINT message,
                     WPARAM wParam, LPARAM lParam,
                     winPrivScreenPtr pScreenPriv);

/*
 * winwndproc.c
 */

LRESULT CALLBACK
winWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * winwindowswm.c
 */

void

winWindowsWMSendEvent(int type, unsigned int mask, int which, int arg,
                      Window window, int x, int y, int w, int h);

void
 winWindowsWMExtensionInit(void);

/*
 * wincursor.c
 */

Bool
 winInitCursor(ScreenPtr pScreen);

/*
 * winprocarg.c
 */
void
 winInitializeScreens(int maxscreens);

/*
 * winrandr.c
 */
Bool
 winRandRInit(ScreenPtr pScreen);
void

winDoRandRScreenSetSize(ScreenPtr pScreen,
                        CARD16 width,
                        CARD16 height, CARD32 mmWidth, CARD32 mmHeight);

/*
 * winmsgwindow.c
 */
Bool
winCreateMsgWindowThread(void);

/*
 * winos.c
 */
void
winOS(void);

/*
 * END DDX and DIX Function Prototypes
 */

#endif                          /* _WIN_H_ */
