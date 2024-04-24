/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86DDC.h"
#include "xf86_OSproc.h"
#include "dgaproc.h"
#include "xf86Crtc.h"
#include "xf86Modes.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"

static Bool
xf86_dga_get_modes(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    DGAModePtr modes, mode;
    DisplayModePtr display_mode;
    int bpp = scrn->bitsPerPixel >> 3;
    int num;

    num = 0;
    display_mode = scrn->modes;
    while (display_mode) {
        num++;
        display_mode = display_mode->next;
        if (display_mode == scrn->modes)
            break;
    }

    if (!num)
        return FALSE;

    modes = xallocarray(num, sizeof(DGAModeRec));
    if (!modes)
        return FALSE;

    num = 0;
    display_mode = scrn->modes;
    while (display_mode) {
        mode = modes + num++;

        mode->mode = display_mode;
        mode->flags = DGA_CONCURRENT_ACCESS;
        if (display_mode->Flags & V_DBLSCAN)
            mode->flags |= DGA_DOUBLESCAN;
        if (display_mode->Flags & V_INTERLACE)
            mode->flags |= DGA_INTERLACED;
        mode->byteOrder = scrn->imageByteOrder;
        mode->depth = scrn->depth;
        mode->bitsPerPixel = scrn->bitsPerPixel;
        mode->red_mask = scrn->mask.red;
        mode->green_mask = scrn->mask.green;
        mode->blue_mask = scrn->mask.blue;
        mode->visualClass = (bpp == 1) ? PseudoColor : TrueColor;
        mode->viewportWidth = display_mode->HDisplay;
        mode->viewportHeight = display_mode->VDisplay;
        mode->xViewportStep = (bpp == 3) ? 2 : 1;
        mode->yViewportStep = 1;
        mode->viewportFlags = DGA_FLIP_RETRACE;
        mode->offset = 0;
        mode->address = 0;
        mode->imageWidth = mode->viewportWidth;
        mode->imageHeight = mode->viewportHeight;
        mode->bytesPerScanline = (mode->imageWidth * scrn->bitsPerPixel) >> 3;
        mode->pixmapWidth = mode->imageWidth;
        mode->pixmapHeight = mode->imageHeight;
        mode->maxViewportX = 0;
        mode->maxViewportY = 0;

        display_mode = display_mode->next;
        if (display_mode == scrn->modes)
            break;
    }
    free(xf86_config->dga_modes);
    xf86_config->dga_nmode = num;
    xf86_config->dga_modes = modes;
    return TRUE;
}

static Bool
xf86_dga_set_mode(ScrnInfoPtr scrn, DGAModePtr display_mode)
{
    ScreenPtr pScreen = scrn->pScreen;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    if (!display_mode) {
        if (xf86_config->dga_save_mode) {
            xf86SwitchMode(pScreen, xf86_config->dga_save_mode);
            xf86_config->dga_save_mode = NULL;
        }
    }
    else {
        if (!xf86_config->dga_save_mode) {
            xf86_config->dga_save_mode = scrn->currentMode;
            xf86SwitchMode(pScreen, display_mode->mode);
        }
    }
    return TRUE;
}

static int
xf86_dga_get_viewport(ScrnInfoPtr scrn)
{
    return 0;
}

static void
xf86_dga_set_viewport(ScrnInfoPtr scrn, int x, int y, int flags)
{
    scrn->AdjustFrame(scrn, x, y);
}

static Bool
xf86_dga_open_framebuffer(ScrnInfoPtr scrn,
                          char **name,
                          unsigned char **mem, int *size, int *offset,
                          int *flags)
{
    return FALSE;
}

static void
xf86_dga_close_framebuffer(ScrnInfoPtr scrn)
{
}

static DGAFunctionRec xf86_dga_funcs = {
    xf86_dga_open_framebuffer,
    xf86_dga_close_framebuffer,
    xf86_dga_set_mode,
    xf86_dga_set_viewport,
    xf86_dga_get_viewport,
    NULL,
    NULL,
    NULL,
    NULL
};

Bool
xf86DiDGAReInit(ScreenPtr pScreen)
{
    return TRUE;
}

Bool
_xf86_di_dga_reinit_internal(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    if (!DGAScreenAvailable(pScreen))
        return TRUE;

    if (!xf86_dga_get_modes(pScreen))
        return FALSE;

    return DGAReInitModes(pScreen, xf86_config->dga_modes,
                          xf86_config->dga_nmode);
}

Bool
xf86DiDGAInit(ScreenPtr pScreen, unsigned long dga_address)
{
    return TRUE;
}

Bool
_xf86_di_dga_init_internal(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    xf86_config->dga_flags = 0;
    xf86_config->dga_address = 0;
    xf86_config->dga_width = 0;
    xf86_config->dga_height = 0;
    xf86_config->dga_stride = 0;

    if (!xf86_dga_get_modes(pScreen))
        return FALSE;

    return DGAInit(pScreen, &xf86_dga_funcs, xf86_config->dga_modes,
                   xf86_config->dga_nmode);
}
