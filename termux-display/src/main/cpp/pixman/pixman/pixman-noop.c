/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2011 Red Hat, Inc.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "pixman-private.h"
#include "pixman-combine32.h"
#include "pixman-inlines.h"

static void
noop_composite (pixman_implementation_t *imp,
		pixman_composite_info_t *info)
{
    return;
}

static uint32_t *
noop_get_scanline (pixman_iter_t *iter, const uint32_t *mask)
{
    uint32_t *result = iter->buffer;

    iter->buffer += iter->image->bits.rowstride;

    return result;
}

static void
noop_init_solid_narrow (pixman_iter_t *iter,
			const pixman_iter_info_t *info)
{ 
    pixman_image_t *image = iter->image;
    uint32_t *buffer = iter->buffer;
    uint32_t *end = buffer + iter->width;
    uint32_t color;

    if (iter->image->type == SOLID)
	color = image->solid.color_32;
    else
	color = image->bits.fetch_pixel_32 (&image->bits, 0, 0);

    while (buffer < end)
	*(buffer++) = color;
}

static void
noop_init_solid_wide (pixman_iter_t *iter,
		      const pixman_iter_info_t *info)
{
    pixman_image_t *image = iter->image;
    argb_t *buffer = (argb_t *)iter->buffer;
    argb_t *end = buffer + iter->width;
    argb_t color;

    if (iter->image->type == SOLID)
	color = image->solid.color_float;
    else
	color = image->bits.fetch_pixel_float (&image->bits, 0, 0);

    while (buffer < end)
	*(buffer++) = color;
}

static void
noop_init_direct_buffer (pixman_iter_t *iter, const pixman_iter_info_t *info)
{
    pixman_image_t *image = iter->image;

    iter->buffer =
	image->bits.bits + iter->y * image->bits.rowstride + iter->x;
}

static void
dest_write_back_direct (pixman_iter_t *iter)
{
    iter->buffer += iter->image->bits.rowstride;
}

static const pixman_iter_info_t noop_iters[] =
{
    /* Source iters */
    { PIXMAN_any,
      0, ITER_IGNORE_ALPHA | ITER_IGNORE_RGB | ITER_SRC,
      NULL,
      _pixman_iter_get_scanline_noop,
      NULL
    },
    { PIXMAN_solid,
      FAST_PATH_NO_ALPHA_MAP, ITER_NARROW | ITER_SRC,
      noop_init_solid_narrow,
      _pixman_iter_get_scanline_noop,
      NULL,
    },
    { PIXMAN_solid,
      FAST_PATH_NO_ALPHA_MAP, ITER_WIDE | ITER_SRC,
      noop_init_solid_wide,
      _pixman_iter_get_scanline_noop,
      NULL
    },
    { PIXMAN_a8r8g8b8,
      FAST_PATH_STANDARD_FLAGS | FAST_PATH_ID_TRANSFORM |
          FAST_PATH_BITS_IMAGE | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST,
      ITER_NARROW | ITER_SRC,
      noop_init_direct_buffer,
      noop_get_scanline,
      NULL
    },
    /* Dest iters */
    { PIXMAN_a8r8g8b8,
      FAST_PATH_STD_DEST_FLAGS, ITER_NARROW | ITER_DEST,
      noop_init_direct_buffer,
      _pixman_iter_get_scanline_noop,
      dest_write_back_direct
    },
    { PIXMAN_x8r8g8b8,
      FAST_PATH_STD_DEST_FLAGS, ITER_NARROW | ITER_DEST | ITER_LOCALIZED_ALPHA,
      noop_init_direct_buffer,
      _pixman_iter_get_scanline_noop,
      dest_write_back_direct
    },
    { PIXMAN_null },
};

static const pixman_fast_path_t noop_fast_paths[] =
{
    { PIXMAN_OP_DST, PIXMAN_any, 0, PIXMAN_any, 0, PIXMAN_any, 0, noop_composite },
    { PIXMAN_OP_NONE },
};

pixman_implementation_t *
_pixman_implementation_create_noop (pixman_implementation_t *fallback)
{
    pixman_implementation_t *imp =
	_pixman_implementation_create (fallback, noop_fast_paths);
 
    imp->iter_info = noop_iters;

    return imp;
}
