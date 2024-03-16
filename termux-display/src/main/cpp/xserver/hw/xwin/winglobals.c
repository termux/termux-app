/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2008
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
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 *              Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * General global variables
 */

int g_iNumScreens = 0;
winScreenInfo *g_ScreenInfo = 0;

#ifdef HAS_DEVWINDOWS
int g_fdMessageQueue = WIN_FD_INVALID;
#endif
DevPrivateKeyRec g_iScreenPrivateKeyRec;
DevPrivateKeyRec g_iCmapPrivateKeyRec;
DevPrivateKeyRec g_iGCPrivateKeyRec;
DevPrivateKeyRec g_iPixmapPrivateKeyRec;
DevPrivateKeyRec g_iWindowPrivateKeyRec;
unsigned long g_ulServerGeneration = 0;
DWORD g_dwEnginesSupported = 0;
HINSTANCE g_hInstance = 0;
HWND g_hDlgDepthChange = NULL;
HWND g_hDlgExit = NULL;
HWND g_hDlgAbout = NULL;
const char *g_pszQueryHost = NULL;
Bool g_fXdmcpEnabled = FALSE;
Bool g_fAuthEnabled = FALSE;
Bool g_fCompositeAlpha = FALSE;
HICON g_hIconX = NULL;
HICON g_hSmallIconX = NULL;

#ifndef RELOCATE_PROJECTROOT
const char *g_pszLogFile = DEFAULT_LOGDIR "/XWin.%s.log";
#else
const char *g_pszLogFile = "XWin.log";
Bool g_fLogFileChanged = FALSE;
#endif
int g_iLogVerbose = 2;
Bool g_fLogInited = FALSE;
char *g_pszCommandLine = NULL;
Bool g_fSilentFatalError = FALSE;
DWORD g_dwCurrentThreadID = 0;
Bool g_fKeyboardHookLL = FALSE;
Bool g_fNoHelpMessageBox = FALSE;
Bool g_fSoftwareCursor = FALSE;
Bool g_fNativeGl = TRUE;
Bool g_fHostInTitle = TRUE;
pthread_mutex_t g_pmTerminating = PTHREAD_MUTEX_INITIALIZER;

/*
 * Wrapped DIX functions
 */
winDispatchProcPtr winProcEstablishConnectionOrig = NULL;

/*
 * Clipboard variables
 */

Bool g_fClipboard = TRUE;
Bool g_fClipboardStarted = FALSE;

/*
 * Re-initialize global variables that are invalidated
 * by a server reset.
 */

void
winInitializeGlobals(void)
{
    g_dwCurrentThreadID = GetCurrentThreadId();
}
