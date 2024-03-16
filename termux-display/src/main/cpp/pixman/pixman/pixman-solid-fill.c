/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007, 2009 Red Hat, Inc.
 * Copyright © 2009 Soren Sandmann
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "pixman-private.h"

static uint32_t
color_to_uint32 (const pixman_color_t *color)
{
    return
        ((unsigned int) color->alpha >> 8 << 24) |
        ((unsigned int) color->red >> 8 << 16) |
        ((unsigned int) color->green & 0xff00) |
        ((unsigned int) color->blue >> 8);
}

static argb_t
color_to_float (const pixman_color_t *color)
{
    argb_t result;

    result.a = pixman_unorm_to_float (color->alpha, 16);
    result.r = pixman_unorm_to_float (color->red, 16);
    result.g = pixman_unorm_to_float (color->green, 16);
    result.b = pixman_unorm_to_float (color->blue, 16);

    return result;
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_solid_fill (const pixman_color_t *color)
{
    pixman_image_t *img = _pixman_image_allocate ();

    if (!img)
	return NULL;

    img->type = SOLID;
    img->solid.color = *color;
    img->solid.color_32 = color_to_uint32 (color);
    img->solid.color_float = color_to_float (color);

    return img;
}

