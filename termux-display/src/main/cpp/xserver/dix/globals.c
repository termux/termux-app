/************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include "misc.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "input.h"
#include "dixfont.h"
#include "dixstruct.h"
#include "os.h"

ScreenInfo screenInfo;

KeybdCtrl defaultKeyboardControl = {
    DEFAULT_KEYBOARD_CLICK,
    DEFAULT_BELL,
    DEFAULT_BELL_PITCH,
    DEFAULT_BELL_DURATION,
    DEFAULT_AUTOREPEAT,
    DEFAULT_AUTOREPEATS,
    DEFAULT_LEDS,
    0
};

PtrCtrl defaultPointerControl = {
    DEFAULT_PTR_NUMERATOR,
    DEFAULT_PTR_DENOMINATOR,
    DEFAULT_PTR_THRESHOLD,
    0
};

ClientPtr clients[MAXCLIENTS];
ClientPtr serverClient;
int currentMaxClients;          /* current size of clients array */
long maxBigRequestSize = MAX_BIG_REQUEST_SIZE;

unsigned long globalSerialNumber = 0;
unsigned long serverGeneration = 0;

/* these next four are initialized in main.c */
CARD32 ScreenSaverTime;
CARD32 ScreenSaverInterval;
int ScreenSaverBlanking;
int ScreenSaverAllowExposures;

/* default time of 10 minutes */
CARD32 defaultScreenSaverTime = (10 * (60 * 1000));
CARD32 defaultScreenSaverInterval = (10 * (60 * 1000));
int defaultScreenSaverBlanking = PreferBlanking;
int defaultScreenSaverAllowExposures = AllowExposures;

#ifdef SCREENSAVER
Bool screenSaverSuspended = FALSE;
#endif

const char *defaultFontPath = COMPILEDDEFAULTFONTPATH;
FontPtr defaultFont;            /* not declared in dix.h to avoid including font.h in
                                   every compilation of dix code */
CursorPtr rootCursor;
Bool party_like_its_1989 = FALSE;
Bool whiteRoot = FALSE;

TimeStamp currentTime;

int defaultColorVisualClass = -1;
int monitorResolution = 0;

const char *display;
int displayfd = -1;
Bool explicit_display = FALSE;
char *ConnectionInfo;
