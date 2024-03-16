/*
 *Copyright (C) 2001-2004 Harold L Hunt II All Rights Reserved.
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
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <../xfree86/common/xorgVersion.h>
#include "win.h"

#ifdef DDXOSVERRORF
void
OsVendorVErrorF(const char *pszFormat, va_list va_args)
{
    /* make sure the clipboard and multiwindow threads do not interfere the
     * main thread */
    static pthread_mutex_t s_pmPrinting = PTHREAD_MUTEX_INITIALIZER;

    /* Lock the printing mutex */
    pthread_mutex_lock(&s_pmPrinting);

    /* Print the error message to a log file, could be stderr */
    LogVWrite(0, pszFormat, va_args);

    /* Unlock the printing mutex */
    pthread_mutex_unlock(&s_pmPrinting);
}
#endif

/*
 * os/log.c:FatalError () calls our vendor ErrorF, so the message
 * from a FatalError will be logged.
 *
 * Attempt to do last-ditch, safe, important cleanup here.
 */
void
OsVendorFatalError(const char *f, va_list args)
{
    char errormsg[1024] = "";

    /* Don't give duplicate warning if UseMsg was called */
    if (g_fSilentFatalError)
        return;

    if (!g_fLogInited) {
        g_fLogInited = TRUE;
        g_pszLogFile = LogInit(g_pszLogFile, ".old");
    }
    LogClose(EXIT_ERR_ABORT);

    /* Format the error message */
    vsnprintf(errormsg, sizeof(errormsg), f, args);

    /*
       Sometimes the error message needs a bit of cosmetic cleaning
       up for use in a dialog box...
     */
    {
        char *s;

        while ((s = strstr(errormsg, "\n\t")) != NULL) {
            s[0] = ' ';
            s[1] = '\n';
        }
    }

    winMessageBoxF("A fatal error has occurred and " PROJECT_NAME " will now exit.\n\n"
                   "%s\n\n"
                   "Please open %s for more information.\n",
                   MB_ICONERROR,
                   errormsg,
                   (g_pszLogFile ? g_pszLogFile : "the logfile"));
}

/*
 * winMessageBoxF - Print a formatted error message in a useful
 * message box.
 */

void
winMessageBoxF(const char *pszError, UINT uType, ...)
{
    char *pszErrorF = NULL;
    char *pszMsgBox = NULL;
    va_list args;
    int size;

    va_start(args, uType);
    size = vasprintf(&pszErrorF, pszError, args);
    va_end(args);
    if (size == -1) {
        pszErrorF = NULL;
        goto winMessageBoxF_Cleanup;
    }

#define MESSAGEBOXF \
	"%s\n" \
	"Vendor: %s\n" \
	"Release: %d.%d.%d.%d\n" \
	"Contact: %s\n" \
	"%s\n\n" \
	"XWin was started with the following command-line:\n\n" \
	"%s\n"

    size = asprintf(&pszMsgBox, MESSAGEBOXF,
                    pszErrorF, XVENDORNAME,
                    XORG_VERSION_MAJOR, XORG_VERSION_MINOR, XORG_VERSION_PATCH,
                    XORG_VERSION_SNAP,
                    BUILDERADDR, BUILDERSTRING, g_pszCommandLine);

    if (size == -1) {
        pszMsgBox = NULL;
        goto winMessageBoxF_Cleanup;
    }

    /* Display the message box string */
    MessageBox(NULL, pszMsgBox, PROJECT_NAME, MB_OK | uType);

 winMessageBoxF_Cleanup:
    free(pszErrorF);
    free(pszMsgBox);
#undef MESSAGEBOXF
}
