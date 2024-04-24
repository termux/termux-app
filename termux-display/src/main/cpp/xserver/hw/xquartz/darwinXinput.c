/*
 * X server support of the XINPUT extension for xquartz
 *
 * This is currently a copy of Xi/stubs.c, but eventually this
 * should include more complete XINPUT support.
 */

/************************************************************

   Copyright 1989, 1998  The Open Group

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

   Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

   All Rights Reserved

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation, and that the name of Hewlett-Packard not be
   used in advertising or publicity pertaining to distribution of the
   software without specific, written prior permission.

   HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
   HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
   ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
   SOFTWARE.

 ********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "XIstubs.h"
#include "darwin.h"

/****************************************************************************
 *
 * Caller:	ProcXSetDeviceMode
 *
 * Change the mode of an extension device.
 * This function is used to change the mode of a device from reporting
 * relative motion to reporting absolute positional information, and
 * vice versa.
 * The default implementation below is that no such devices are supported.
 *
 */

int
SetDeviceMode(ClientPtr client, DeviceIntPtr dev, int mode)
{
    DEBUG_LOG("SetDeviceMode(%p, %p, %d)\n", client, dev, mode);
    return BadMatch;
}

/****************************************************************************
 *
 * Caller:	ProcXSetDeviceValuators
 *
 * Set the value of valuators on an extension input device.
 * This function is used to set the initial value of valuators on
 * those input devices that are capable of reporting either relative
 * motion or an absolute position, and allow an initial position to be set.
 * The default implementation below is that no such devices are supported.
 *
 */

int
SetDeviceValuators(ClientPtr client, DeviceIntPtr dev,
                   int *valuators, int first_valuator, int num_valuators)
{
    DEBUG_LOG("SetDeviceValuators(%p, %p, %p, %d, %d)\n", client,
              dev, valuators, first_valuator, num_valuators);
    return BadMatch;
}

/****************************************************************************
 *
 * Caller:	ProcXChangeDeviceControl
 *
 * Change the specified device controls on an extension input device.
 *
 */

int
ChangeDeviceControl(ClientPtr client, DeviceIntPtr dev,
                    xDeviceCtl * control)
{

    DEBUG_LOG("ChangeDeviceControl(%p, %p, %p)\n", client, dev, control);
    return BadMatch;
}

/****************************************************************************
 *
 * Caller: configAddDevice (and others)
 *
 * Add a new device with the specified options.
 *
 */
int
NewInputDeviceRequest(InputOption *options, InputAttributes *attrs,
                      DeviceIntPtr *pdev)
{
    DEBUG_LOG("NewInputDeviceRequest(%p, %p)\n", options, pdev);
    return BadValue;
}

/****************************************************************************
 *
 * Caller: configRemoveDevice (and others)
 *
 * Remove the specified device previously added.
 *
 */
void
DeleteInputDeviceRequest(DeviceIntPtr dev)
{
    DEBUG_LOG("DeleteInputDeviceRequest(%p)\n", dev);
}

/****************************************************************************
 *
 * Caller: configRemoveDevice (and others)
 *
 * Remove any traces of the input device specified in config_info.
 * This is only necessary if the ddx keeps information around beyond
 * the NewInputDeviceRequest/DeleteInputDeviceRequest
 *
 */
void
RemoveInputDeviceTraces(const char *config_info)
{
    DEBUG_LOG("RemoveInputDeviceTraces(%s)\n", config_info);
}
