/*
  File: winglobals.h
  Purpose: declarations for global variables

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice (including the next
  paragraph) shall be included in all copies or substantial portions of the
  Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

*/

#ifndef WINGLOBALS_H
#define WINGLOBALS_H

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <pthread.h>

/*
 * References to external symbols
 */

extern int g_iNumScreens;
extern int g_iLastScreen;
extern char *g_pszCommandLine;
extern Bool g_fSilentFatalError;
extern const char *g_pszLogFile;

#ifdef RELOCATE_PROJECTROOT
extern Bool g_fLogFileChanged;
#endif
extern int g_iLogVerbose;
extern Bool g_fLogInited;

extern Bool g_fAuthEnabled;
extern Bool g_fXdmcpEnabled;
extern Bool g_fCompositeAlpha;

extern Bool g_fNoHelpMessageBox;
extern Bool g_fNativeGl;
extern Bool g_fHostInTitle;

extern HWND g_hDlgDepthChange;
extern HWND g_hDlgExit;
extern HWND g_hDlgAbout;

extern Bool g_fSoftwareCursor;
extern Bool g_fCursor;

/* Typedef for DIX wrapper functions */
typedef int (*winDispatchProcPtr) (ClientPtr);

/*
 * Wrapped DIX functions
 */
extern winDispatchProcPtr winProcEstablishConnectionOrig;
extern Bool g_fClipboard;
extern Bool g_fClipboardStarted;

/* The global X default icons */
extern HICON g_hIconX;
extern HICON g_hSmallIconX;

extern DWORD g_dwCurrentThreadID;

extern Bool g_fKeyboardHookLL;
extern Bool g_fButton[3];

extern pthread_mutex_t g_pmTerminating;

#endif                          /* WINGLOBALS_H */
