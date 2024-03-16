/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2009
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
 * Authors:	Kensuke Matsuzaki
 *              Colin Harrison
 */
#if !defined(_WINWINDOW_H_)
#define _WINWINDOW_H_

#ifndef NO
#define NO			0
#endif
#ifndef YES
#define YES			1
#endif

/* Constant strings */
#ifndef PROJECT_NAME
#define PROJECT_NAME		"Cygwin/X"
#endif
#define EXECUTABLE_NAME         "XWin"
#define WINDOW_CLASS		"cygwin/x"
#define WINDOW_TITLE		PROJECT_NAME ":%s.%d"
#define WINDOW_TITLE_XDMCP	"%s:%s.%d"
#define WIN_SCR_PROP		"cyg_screen_prop rl"
#define WINDOW_CLASS_X		"cygwin/x X rl"
#define WINDOW_CLASS_X_MSG      "cygwin/x X msg"
#define WINDOW_TITLE_X		PROJECT_NAME " X"
#define WIN_WINDOW_PROP		"cyg_window_prop_rl"
#ifdef HAS_DEVWINDOWS
#define WIN_MSG_QUEUE_FNAME	"/dev/windows"
#endif
#define WIN_WID_PROP		"cyg_wid_prop_rl"
#define WIN_NEEDMANAGE_PROP	"cyg_override_redirect_prop_rl"
#ifndef CYGMULTIWINDOW_DEBUG
#define CYGMULTIWINDOW_DEBUG    NO
#endif
#ifndef CYGWINDOWING_DEBUG
#define CYGWINDOWING_DEBUG	NO
#endif

#define XMING_SIGNATURE		0x12345678L

typedef struct _winPrivScreenRec *winPrivScreenPtr;

/*
 * Window privates
 */

typedef struct {
    DWORD dwDummy;
    HRGN hRgn;
    HWND hWnd;
    winPrivScreenPtr pScreenPriv;
    Bool fXKilled;
    HDWP hDwp;
#ifdef XWIN_GLX_WINDOWS
    Bool fWglUsed;
#endif
} winPrivWinRec, *winPrivWinPtr;

typedef struct _winWMMessageRec {
    DWORD dwID;
    DWORD msg;
    int iWindow;
    HWND hwndWindow;
    int iX, iY;
    int iWidth, iHeight;
} winWMMessageRec, *winWMMessagePtr;

/*
 * winmultiwindowwm.c
 */

#define		WM_WM_MOVE		(WM_USER + 1)
#define		WM_WM_SIZE		(WM_USER + 2)
#define		WM_WM_RAISE		(WM_USER + 3)
#define		WM_WM_LOWER		(WM_USER + 4)
#define		WM_WM_UNMAP		(WM_USER + 6)
#define		WM_WM_KILL		(WM_USER + 7)
#define		WM_WM_ACTIVATE		(WM_USER + 8)
#define		WM_WM_NAME_EVENT	(WM_USER + 9)
#define		WM_WM_ICON_EVENT	(WM_USER + 10)
#define		WM_WM_CHANGE_STATE	(WM_USER + 11)
#define		WM_WM_MAP_UNMANAGED	(WM_USER + 12)
#define		WM_WM_MAP_MANAGED	(WM_USER + 13)
#define		WM_WM_HINTS_EVENT	(WM_USER + 14)

#define		MwmHintsDecorations	(1L << 1)

#define		MwmDecorAll		(1L << 0)
#define		MwmDecorBorder		(1L << 1)
#define		MwmDecorHandle		(1L << 2)
#define		MwmDecorTitle		(1L << 3)
#define		MwmDecorMenu		(1L << 4)
#define		MwmDecorMinimize	(1L << 5)
#define		MwmDecorMaximize	(1L << 6)

/*
  This structure only contains 3 elements.  The Motif 2.0 structure contains 5,
  but we only need the first 3, so that is all we will define

  This structure represents xcb_get_property()'s view of the property as a
  sequence of ints, rather than XGetWindowProperty()'s view of the property as a
  sequence of arch-dependent longs.
*/
typedef struct MwmHints {
    unsigned int flags, functions, decorations;
} MwmHints;

#define		PropMwmHintsElements	3

void
 winSendMessageToWM(void *pWMInfo, winWMMessagePtr msg);

Bool

winInitWM(void **ppWMInfo,
          pthread_t * ptWMProc,
          pthread_t * ptXMsgProc,
          pthread_mutex_t * ppmServerStarted,
          int dwScreen, HWND hwndScreen, Bool compositeWM);

void
 winDeinitMultiWindowWM(void);

void
 winPropertyStoreInit(void);

void
 winPropertyStoreDestroy(void);

void
 winSetAppUserModelID(HWND hWnd, const char *AppID);

void
 winShowWindowOnTaskbar(HWND hWnd, Bool show);

#endif
