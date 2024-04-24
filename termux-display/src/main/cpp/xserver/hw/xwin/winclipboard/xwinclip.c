/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the copyright holder(s)
 *and author(s) shall not be used in advertising or otherwise to promote
 *the sale, use or other dealings in this Software without prior written
 *authorization from the copyright holder(s) and author(s).
 *
 * Authors:	Harold L Hunt II
 *              Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winclipboard.h"

/*
 * Main function
 */

int
main (int argc, char *argv[])
{
  int			i;
  char			*pszDisplay = NULL;

  /* Parse command-line parameters */
  for (i = 1; i < argc; ++i)
    {
      /* Look for -display "display_name" or --display "display_name" */
      if (i < argc - 1
	  && (!strcmp (argv[i], "-display")
	      || !strcmp (argv[i], "--display")))
	{
	  /* Grab a pointer to the display parameter */
	  pszDisplay = argv[i + 1];

	  /* Skip the display argument */
	  i++;
	  continue;
	}

      /* Look for -noprimary */
      if (!strcmp (argv[i], "-noprimary"))
	{
	  fPrimarySelection = 0;
	  continue;
	}

      /* Yack when we find a parameter that we don't know about */
      printf ("Unknown parameter: %s\nExiting.\n", argv[i]);
      exit (1);
    }

  winClipboardProc(pszDisplay, NULL /* Use XAUTHORITY for auth data */);

  return 0;
}
