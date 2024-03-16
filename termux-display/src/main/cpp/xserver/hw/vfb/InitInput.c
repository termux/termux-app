/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "mi.h"
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "inputstr.h"
#include <X11/Xos.h>
#include "mipointer.h"
#include "xkbsrv.h"
#include <X11/keysym.h>
#include "xserver-properties.h"
#include "exevents.h"
#include "extinit.h"

void
ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

void
DDXRingBell(int volume, int pitch, int duration)
{
}

#define VFB_MIN_KEY 8
#define VFB_MAX_KEY 255

static int
vfbKeybdProc(DeviceIntPtr pDevice, int onoff)
{
    DevicePtr pDev = (DevicePtr) pDevice;

    switch (onoff) {
    case DEVICE_INIT:
        InitKeyboardDeviceStruct(pDevice, NULL, NULL, NULL);
        break;
    case DEVICE_ON:
        pDev->on = TRUE;
        break;
    case DEVICE_OFF:
        pDev->on = FALSE;
        break;
    case DEVICE_CLOSE:
        break;
    }
    return Success;
}

static int
vfbMouseProc(DeviceIntPtr pDevice, int onoff)
{
#define NBUTTONS 3
#define NAXES 2

    BYTE map[NBUTTONS + 1];
    DevicePtr pDev = (DevicePtr) pDevice;
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };

    switch (onoff) {
    case DEVICE_INIT:
        map[1] = 1;
        map[2] = 2;
        map[3] = 3;

        btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

        InitPointerDeviceStruct(pDev, map, NBUTTONS, btn_labels,
                                (PtrCtrlProcPtr) NoopDDA,
                                GetMotionHistorySize(), NAXES, axes_labels);
        break;

    case DEVICE_ON:
        pDev->on = TRUE;
        break;

    case DEVICE_OFF:
        pDev->on = FALSE;
        break;

    case DEVICE_CLOSE:
        break;
    }
    return Success;

#undef NBUTTONS
#undef NAXES
}

void
InitInput(int argc, char *argv[])
{
    DeviceIntPtr p, k;
    Atom xiclass;

    p = AddInputDevice(serverClient, vfbMouseProc, TRUE);
    k = AddInputDevice(serverClient, vfbKeybdProc, TRUE);
    xiclass = MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE);
    AssignTypeAndName(p, xiclass, "Xvfb mouse");
    xiclass = MakeAtom(XI_KEYBOARD, sizeof(XI_KEYBOARD) - 1, TRUE);
    AssignTypeAndName(k, xiclass, "Xvfb keyboard");
    (void) mieqInit();
}

void
CloseInput(void)
{
    mieqFini();
}
