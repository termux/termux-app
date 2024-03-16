/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
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

#include <unistd.h>
#include <pthread.h>

#include "win.h"
#include "winclipboard/winclipboard.h"
#include "windisplay.h"
#include "winauth.h"

#define WIN_CLIPBOARD_RETRIES			40
#define WIN_CLIPBOARD_DELAY			1

/*
 * Local variables
 */

static pthread_t g_ptClipboardProc;

/*
 *
 */
static void *
winClipboardThreadProc(void *arg)
{
  char szDisplay[512];
  xcb_auth_info_t *auth_info;
  int clipboardRestarts = 0;

  while (1)
    {
      Bool fShutdown;

      ++clipboardRestarts;

      /* Setup the display connection string */
      /*
       * NOTE: Always connect to screen 0 since we require that screen
       * numbers start at 0 and increase without gaps.  We only need
       * to connect to one screen on the display to get events
       * for all screens on the display.  That is why there is only
       * one clipboard client thread.
      */
      winGetDisplayName(szDisplay, 0);

      /* Print the display connection string */
      ErrorF("winClipboardThreadProc - DISPLAY=%s\n", szDisplay);

      /* Flag that clipboard client has been launched */
      g_fClipboardStarted = TRUE;

      /* Use our generated cookie for authentication */
      auth_info = winGetXcbAuthInfo();

      fShutdown = winClipboardProc(szDisplay, auth_info);

      /* Flag that clipboard client has stopped */
      g_fClipboardStarted = FALSE;

      if (fShutdown)
        break;

      /* checking if we need to restart */
      if (clipboardRestarts >= WIN_CLIPBOARD_RETRIES) {
        /* terminates clipboard thread but the main server still lives */
        ErrorF("winClipboardProc - the clipboard thread has restarted %d times and seems to be unstable, disabling clipboard integration\n", clipboardRestarts);
        g_fClipboard = FALSE;
        break;
      }

      sleep(WIN_CLIPBOARD_DELAY);
      ErrorF("winClipboardProc - trying to restart clipboard thread \n");
    }

  return NULL;
}

/*
 * Initialize the Clipboard module
 */

Bool
winInitClipboard(void)
{
    winDebug("winInitClipboard ()\n");

    /* Spawn a thread for the Clipboard module */
    if (pthread_create(&g_ptClipboardProc, NULL, winClipboardThreadProc, NULL)) {
        /* Bail if thread creation failed */
        ErrorF("winInitClipboard - pthread_create failed.\n");
        return FALSE;
    }

    return TRUE;
}

void
winClipboardShutdown(void)
{
  /* Close down clipboard resources */
  if (g_fClipboard && g_fClipboardStarted) {
    /* Synchronously destroy the clipboard window */
    winClipboardWindowDestroy();

    /* Wait for the clipboard thread to exit */
    pthread_join(g_ptClipboardProc, NULL);

    g_fClipboardStarted = FALSE;

    winDebug("winClipboardShutdown - Clipboard thread has exited.\n");
  }
}
