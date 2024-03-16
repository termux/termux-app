/*
 * Copyright © 2001 Keith Packard
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#include <stdlib.h>

#include "glamor_priv.h"
/**
 * Sets the offsets to add to coordinates to make them address the same bits in
 * the backing drawable. These coordinates are nonzero only for redirected
 * windows.
 */
void
glamor_get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap,
                           int *x, int *y)
{
#ifdef COMPOSITE
    if (drawable->type == DRAWABLE_WINDOW) {
        *x = -pixmap->screen_x;
        *y = -pixmap->screen_y;
        return;
    }
#endif

    *x = 0;
    *y = 0;
}

void
glamor_pixmap_init(ScreenPtr screen)
{

}

void
glamor_pixmap_fini(ScreenPtr screen)
{
}

void
glamor_set_destination_pixmap_fbo(glamor_screen_private *glamor_priv,
                                  glamor_pixmap_fbo *fbo, int x0, int y0,
                                  int width, int height)
{
    glamor_make_current(glamor_priv);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
    glViewport(x0, y0, width, height);
}

void
glamor_set_destination_pixmap_priv_nc(glamor_screen_private *glamor_priv,
                                      PixmapPtr pixmap,
                                      glamor_pixmap_private *pixmap_priv)
{
    int w, h;

    PIXMAP_PRIV_GET_ACTUAL_SIZE(pixmap, pixmap_priv, w, h);
    glamor_set_destination_pixmap_fbo(glamor_priv, pixmap_priv->fbo, 0, 0, w, h);
}

int
glamor_set_destination_pixmap_priv(glamor_screen_private *glamor_priv,
                                   PixmapPtr pixmap,
                                   glamor_pixmap_private *pixmap_priv)
{
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
        return -1;

    glamor_set_destination_pixmap_priv_nc(glamor_priv, pixmap, pixmap_priv);
    return 0;
}

int
glamor_set_destination_pixmap(PixmapPtr pixmap)
{
    int err;
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    err = glamor_set_destination_pixmap_priv(glamor_priv, pixmap, pixmap_priv);
    return err;
}

Bool
glamor_set_planemask(int depth, unsigned long planemask)
{
    if (glamor_pm_is_solid(depth, planemask)) {
        return GL_TRUE;
    }

    glamor_fallback("unsupported planemask %lx\n", planemask);
    return GL_FALSE;
}

Bool
glamor_set_alu(ScreenPtr screen, unsigned char alu)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    if (glamor_priv->is_gles) {
        if (alu != GXcopy)
            return FALSE;
        else
            return TRUE;
    }

    if (alu == GXcopy) {
        glDisable(GL_COLOR_LOGIC_OP);
        return TRUE;
    }
    glEnable(GL_COLOR_LOGIC_OP);
    switch (alu) {
    case GXclear:
        glLogicOp(GL_CLEAR);
        break;
    case GXand:
        glLogicOp(GL_AND);
        break;
    case GXandReverse:
        glLogicOp(GL_AND_REVERSE);
        break;
    case GXandInverted:
        glLogicOp(GL_AND_INVERTED);
        break;
    case GXnoop:
        glLogicOp(GL_NOOP);
        break;
    case GXxor:
        glLogicOp(GL_XOR);
        break;
    case GXor:
        glLogicOp(GL_OR);
        break;
    case GXnor:
        glLogicOp(GL_NOR);
        break;
    case GXequiv:
        glLogicOp(GL_EQUIV);
        break;
    case GXinvert:
        glLogicOp(GL_INVERT);
        break;
    case GXorReverse:
        glLogicOp(GL_OR_REVERSE);
        break;
    case GXcopyInverted:
        glLogicOp(GL_COPY_INVERTED);
        break;
    case GXorInverted:
        glLogicOp(GL_OR_INVERTED);
        break;
    case GXnand:
        glLogicOp(GL_NAND);
        break;
    case GXset:
        glLogicOp(GL_SET);
        break;
    default:
        glamor_fallback("unsupported alu %x\n", alu);
        return FALSE;
    }

    return TRUE;
}
