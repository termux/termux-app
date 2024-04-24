/*

Copyright 1986, 1998  The Open Group

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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

int
XChangeKeyboardControl(
    register Display *dpy,
    unsigned long mask,
    XKeyboardControl *value_list)
{
    unsigned long values[8];
    register unsigned long *value = values;
    long nvalues;
    register xChangeKeyboardControlReq *req;

    LockDisplay(dpy);
    GetReq(ChangeKeyboardControl, req);
    req->mask = mask;

    if (mask & KBKeyClickPercent)
	*value++ = value_list->key_click_percent;

    if (mask & KBBellPercent)
    	*value++ = value_list->bell_percent;

    if (mask & KBBellPitch)
    	*value++ = value_list->bell_pitch;

    if (mask & KBBellDuration)
    	*value++ = value_list->bell_duration;

    if (mask & KBLed)
    	*value++ = value_list->led;

    if (mask & KBLedMode)
	*value++ = value_list->led_mode;

    if (mask & KBKey)
        *value++ = value_list->key;

    if (mask & KBAutoRepeatMode)
        *value++ = value_list->auto_repeat_mode;


    req->length += (nvalues = value - values);

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    nvalues <<= 2;
    Data32 (dpy, (long *) values, nvalues);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }
