/*
 * Copyright (c) 2012
 *      MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author:  Nemanja Lukic (nemanja.lukic@rt-rk.com)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"
#include "pixman-mips-dspr2.h"

PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_x888_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_8888_0565,
                                    uint32_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_0565_8888,
                                    uint16_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (DO_FAST_MEMCPY, src_0565_0565,
                                    uint16_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (DO_FAST_MEMCPY, src_8888_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (DO_FAST_MEMCPY, src_0888_0888,
                                    uint8_t, 3, uint8_t, 3)
#if defined(__MIPSEL__) || defined(__MIPSEL) || defined(_MIPSEL) || defined(MIPSEL)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_0888_8888_rev,
                                    uint8_t, 3, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_0888_0565_rev,
                                    uint8_t, 3, uint16_t, 1)
#endif
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_pixbuf_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, src_rpixbuf_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, over_8888_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, over_8888_0565,
                                    uint32_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, add_8_8,
                                    uint8_t, 1, uint8_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, add_8888_8888,
                                    uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, out_reverse_8_0565,
                                    uint8_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST (0, out_reverse_8_8888,
                                    uint8_t,  1, uint32_t, 1)

PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (0, src_n_8_8888,
                                       uint8_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (0, src_n_8_8,
                                       uint8_t, 1, uint8_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, over_n_8888_8888_ca,
                                       uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, over_n_8888_0565_ca,
                                       uint32_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, over_n_8_8,
                                       uint8_t, 1, uint8_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, over_n_8_8888,
                                       uint8_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, over_n_8_0565,
                                       uint8_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, add_n_8_8,
                                       uint8_t, 1, uint8_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, add_n_8_8888,
                                       uint8_t, 1, uint32_t, 1)

PIXMAN_MIPS_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, over_8888_n_8888,
                                      uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, over_8888_n_0565,
                                      uint32_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, over_0565_n_0565,
                                      uint16_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, add_8888_n_8888,
                                      uint32_t, 1, uint32_t, 1)

PIXMAN_MIPS_BIND_FAST_PATH_N_DST (SKIP_ZERO_SRC, over_n_0565,
                                  uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_DST (SKIP_ZERO_SRC, over_n_8888,
                                  uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_DST (SKIP_ZERO_SRC, over_reverse_n_8888,
                                  uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_N_DST (0, in_n_8,
                                  uint8_t, 1)

PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (add_8_8_8, uint8_t,  1,
                                         uint8_t,  1, uint8_t,  1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (add_8888_8_8888, uint32_t, 1,
                                         uint8_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (add_8888_8888_8888, uint32_t, 1,
                                         uint32_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (add_0565_8_0565, uint16_t, 1,
                                         uint8_t,  1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (over_8888_8_8888, uint32_t, 1,
                                         uint8_t, 1, uint32_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (over_8888_8_0565, uint32_t, 1,
                                         uint8_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (over_0565_8_0565, uint16_t, 1,
                                         uint8_t, 1, uint16_t, 1)
PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST (over_8888_8888_8888, uint32_t, 1,
                                         uint32_t, 1, uint32_t, 1)

PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_DST (8888_8888, OVER,
                                         uint32_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_DST (8888_0565, OVER,
                                         uint32_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_DST (0565_8888, SRC,
                                         uint16_t, uint32_t)

PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (0, 8888_8888, SRC,
                                          uint32_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (0, 8888_0565, SRC,
                                          uint32_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (0, 0565_8888, SRC,
                                          uint16_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (0, 0565_0565, SRC,
                                          uint16_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (SKIP_ZERO_SRC, 8888_8888, OVER,
                                          uint32_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST (SKIP_ZERO_SRC, 8888_8888, ADD,
                                          uint32_t, uint32_t)

PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_A8_DST (SKIP_ZERO_SRC, 8888_8_0565,
                                            OVER, uint32_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_A8_DST (SKIP_ZERO_SRC, 0565_8_0565,
                                            OVER, uint16_t, uint16_t)

PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (0, 8888_8_8888, SRC,
                                             uint32_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (0, 8888_8_0565, SRC,
                                             uint32_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (0, 0565_8_x888, SRC,
                                             uint16_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (0, 0565_8_0565, SRC,
                                             uint16_t, uint16_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (SKIP_ZERO_SRC, 8888_8_8888, OVER,
                                             uint32_t, uint32_t)
PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST (SKIP_ZERO_SRC, 8888_8_8888, ADD,
                                             uint32_t, uint32_t)

static pixman_bool_t
mips_dspr2_fill (pixman_implementation_t *imp,
                 uint32_t *               bits,
                 int                      stride,
                 int                      bpp,
                 int                      x,
                 int                      y,
                 int                      width,
                 int                      height,
                 uint32_t                 _xor)
{
    uint8_t *byte_line;
    uint32_t byte_width;
    switch (bpp)
    {
    case 16:
        stride = stride * (int) sizeof (uint32_t) / 2;
        byte_line = (uint8_t *)(((uint16_t *)bits) + stride * y + x);
        byte_width = width * 2;
        stride *= 2;

        while (height--)
        {
            uint8_t *dst = byte_line;
            byte_line += stride;
            pixman_fill_buff16_mips (dst, byte_width, _xor & 0xffff);
        }
        return TRUE;
    case 32:
        stride = stride * (int) sizeof (uint32_t) / 4;
        byte_line = (uint8_t *)(((uint32_t *)bits) + stride * y + x);
        byte_width = width * 4;
        stride *= 4;

        while (height--)
        {
            uint8_t *dst = byte_line;
            byte_line += stride;
            pixman_fill_buff32_mips (dst, byte_width, _xor);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

static pixman_bool_t
mips_dspr2_blt (pixman_implementation_t *imp,
                uint32_t *               src_bits,
                uint32_t *               dst_bits,
                int                      src_stride,
                int                      dst_stride,
                int                      src_bpp,
                int                      dst_bpp,
                int                      src_x,
                int                      src_y,
                int                      dest_x,
                int                      dest_y,
                int                      width,
                int                      height)
{
    if (src_bpp != dst_bpp)
        return FALSE;

    uint8_t *src_bytes;
    uint8_t *dst_bytes;
    uint32_t byte_width;

    switch (src_bpp)
    {
    case 16:
        src_stride = src_stride * (int) sizeof (uint32_t) / 2;
        dst_stride = dst_stride * (int) sizeof (uint32_t) / 2;
        src_bytes =(uint8_t *)(((uint16_t *)src_bits)
                                          + src_stride * (src_y) + (src_x));
        dst_bytes = (uint8_t *)(((uint16_t *)dst_bits)
                                           + dst_stride * (dest_y) + (dest_x));
        byte_width = width * 2;
        src_stride *= 2;
        dst_stride *= 2;

        while (height--)
        {
            uint8_t *src = src_bytes;
            uint8_t *dst = dst_bytes;
            src_bytes += src_stride;
            dst_bytes += dst_stride;
            pixman_mips_fast_memcpy (dst, src, byte_width);
        }
        return TRUE;
    case 32:
        src_stride = src_stride * (int) sizeof (uint32_t) / 4;
        dst_stride = dst_stride * (int) sizeof (uint32_t) / 4;
        src_bytes = (uint8_t *)(((uint32_t *)src_bits)
                                           + src_stride * (src_y) + (src_x));
        dst_bytes = (uint8_t *)(((uint32_t *)dst_bits)
                                           + dst_stride * (dest_y) + (dest_x));
        byte_width = width * 4;
        src_stride *= 4;
        dst_stride *= 4;

        while (height--)
        {
            uint8_t *src = src_bytes;
            uint8_t *dst = dst_bytes;
            src_bytes += src_stride;
            dst_bytes += dst_stride;
            pixman_mips_fast_memcpy (dst, src, byte_width);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

static const pixman_fast_path_t mips_dspr2_fast_paths[] =
{
    PIXMAN_STD_FAST_PATH (SRC, r5g6b5,   null, r5g6b5,   mips_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5,   null, b5g6r5,   mips_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, r5g6b5,   mips_composite_src_8888_0565),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, r5g6b5,   mips_composite_src_8888_0565),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, b5g6r5,   mips_composite_src_8888_0565),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, b5g6r5,   mips_composite_src_8888_0565),
    PIXMAN_STD_FAST_PATH (SRC, r5g6b5,   null, a8r8g8b8, mips_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, r5g6b5,   null, x8r8g8b8, mips_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5,   null, a8b8g8r8, mips_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5,   null, x8b8g8r8, mips_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, x8r8g8b8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, x8r8g8b8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, x8b8g8r8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, x8b8g8r8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, a8r8g8b8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, a8b8g8r8, mips_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, a8r8g8b8, mips_composite_src_x888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, a8b8g8r8, mips_composite_src_x888_8888),
    PIXMAN_STD_FAST_PATH (SRC, r8g8b8,   null, r8g8b8,   mips_composite_src_0888_0888),
#if defined(__MIPSEL__) || defined(__MIPSEL) || defined(_MIPSEL) || defined(MIPSEL)
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8,   null, x8r8g8b8, mips_composite_src_0888_8888_rev),
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8,   null, r5g6b5,   mips_composite_src_0888_0565_rev),
#endif
    PIXMAN_STD_FAST_PATH (SRC, pixbuf,   pixbuf,  a8r8g8b8, mips_composite_src_pixbuf_8888),
    PIXMAN_STD_FAST_PATH (SRC, pixbuf,   pixbuf,  a8b8g8r8, mips_composite_src_rpixbuf_8888),
    PIXMAN_STD_FAST_PATH (SRC, rpixbuf,  rpixbuf, a8r8g8b8, mips_composite_src_rpixbuf_8888),
    PIXMAN_STD_FAST_PATH (SRC, rpixbuf,  rpixbuf, a8b8g8r8, mips_composite_src_pixbuf_8888),
    PIXMAN_STD_FAST_PATH (SRC, solid,    a8,   a8r8g8b8, mips_composite_src_n_8_8888),
    PIXMAN_STD_FAST_PATH (SRC, solid,    a8,   x8r8g8b8, mips_composite_src_n_8_8888),
    PIXMAN_STD_FAST_PATH (SRC, solid,    a8,   a8b8g8r8, mips_composite_src_n_8_8888),
    PIXMAN_STD_FAST_PATH (SRC, solid,    a8,   x8b8g8r8, mips_composite_src_n_8_8888),
    PIXMAN_STD_FAST_PATH (SRC, solid,    a8,   a8,       mips_composite_src_n_8_8),

    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, a8r8g8b8, mips_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, x8r8g8b8, mips_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, a8b8g8r8, mips_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, x8b8g8r8, mips_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, r5g6b5,   mips_composite_over_n_8888_0565_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, b5g6r5,   mips_composite_over_n_8888_0565_ca),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       a8,       mips_composite_over_n_8_8),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       a8r8g8b8, mips_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       x8r8g8b8, mips_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       a8b8g8r8, mips_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       x8b8g8r8, mips_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       r5g6b5,   mips_composite_over_n_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid,    a8,       b5g6r5,   mips_composite_over_n_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid,    null,     r5g6b5,   mips_composite_over_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid,    null,     a8r8g8b8, mips_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid,    null,     x8r8g8b8, mips_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid,    a8r8g8b8, mips_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid,    x8r8g8b8, mips_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid,    r5g6b5,   mips_composite_over_8888_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid,    b5g6r5,   mips_composite_over_8888_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, r5g6b5,   solid,    r5g6b5,   mips_composite_over_0565_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, b5g6r5,   solid,    b5g6r5,   mips_composite_over_0565_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, a8,       a8r8g8b8, mips_composite_over_8888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, a8,       x8r8g8b8, mips_composite_over_8888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, a8,       a8b8g8r8, mips_composite_over_8888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, a8,       x8b8g8r8, mips_composite_over_8888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, a8,       r5g6b5,   mips_composite_over_8888_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, a8,       b5g6r5,   mips_composite_over_8888_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, r5g6b5,   a8,       r5g6b5,   mips_composite_over_0565_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, b5g6r5,   a8,       b5g6r5,   mips_composite_over_0565_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, a8r8g8b8, mips_composite_over_8888_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null,     a8r8g8b8, mips_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null,     x8r8g8b8, mips_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null,     a8b8g8r8, mips_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null,     x8b8g8r8, mips_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null,     r5g6b5,   mips_composite_over_8888_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null,     b5g6r5,   mips_composite_over_8888_0565),
    PIXMAN_STD_FAST_PATH (ADD,  solid,    a8,       a8,       mips_composite_add_n_8_8),
    PIXMAN_STD_FAST_PATH (ADD,  solid,    a8,       a8r8g8b8, mips_composite_add_n_8_8888),
    PIXMAN_STD_FAST_PATH (ADD,  solid,    a8,       a8b8g8r8, mips_composite_add_n_8_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8,       a8,       a8,       mips_composite_add_8_8_8),
    PIXMAN_STD_FAST_PATH (ADD,  r5g6b5,   a8,       r5g6b5,   mips_composite_add_0565_8_0565),
    PIXMAN_STD_FAST_PATH (ADD,  b5g6r5,   a8,       b5g6r5,   mips_composite_add_0565_8_0565),
    PIXMAN_STD_FAST_PATH (ADD,  a8r8g8b8, a8,       a8r8g8b8, mips_composite_add_8888_8_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8b8g8r8, a8,       a8b8g8r8, mips_composite_add_8888_8_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8r8g8b8, a8r8g8b8, a8r8g8b8, mips_composite_add_8888_8888_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8r8g8b8, solid,    a8r8g8b8, mips_composite_add_8888_n_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8b8g8r8, solid,    a8b8g8r8, mips_composite_add_8888_n_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8,       null,     a8,       mips_composite_add_8_8),
    PIXMAN_STD_FAST_PATH (ADD,  a8r8g8b8, null,     a8r8g8b8, mips_composite_add_8888_8888),
    PIXMAN_STD_FAST_PATH (ADD,  a8b8g8r8, null,     a8b8g8r8, mips_composite_add_8888_8888),
    PIXMAN_STD_FAST_PATH (OUT_REVERSE, a8,    null, r5g6b5,   mips_composite_out_reverse_8_0565),
    PIXMAN_STD_FAST_PATH (OUT_REVERSE, a8,    null, b5g6r5,   mips_composite_out_reverse_8_0565),
    PIXMAN_STD_FAST_PATH (OUT_REVERSE, a8,    null, a8r8g8b8, mips_composite_out_reverse_8_8888),
    PIXMAN_STD_FAST_PATH (OUT_REVERSE, a8,    null, a8b8g8r8, mips_composite_out_reverse_8_8888),
    PIXMAN_STD_FAST_PATH (OVER_REVERSE, solid, null, a8r8g8b8, mips_composite_over_reverse_n_8888),
    PIXMAN_STD_FAST_PATH (OVER_REVERSE, solid, null, a8b8g8r8, mips_composite_over_reverse_n_8888),
    PIXMAN_STD_FAST_PATH (IN,           solid, null, a8,       mips_composite_in_n_8),

    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, mips_8888_8888),
    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8b8g8r8, a8b8g8r8, mips_8888_8888),
    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, x8r8g8b8, mips_8888_8888),
    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8b8g8r8, x8b8g8r8, mips_8888_8888),

    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, r5g6b5, mips_8888_0565),
    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (OVER, a8b8g8r8, b5g6r5, mips_8888_0565),

    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (SRC, b5g6r5, x8b8g8r8, mips_0565_8888),
    PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH (SRC, r5g6b5, x8r8g8b8, mips_0565_8888),
    /* Note: NONE repeat is not supported yet */
    SIMPLE_NEAREST_FAST_PATH_COVER (SRC, r5g6b5, a8r8g8b8, mips_0565_8888),
    SIMPLE_NEAREST_FAST_PATH_COVER (SRC, b5g6r5, a8b8g8r8, mips_0565_8888),
    SIMPLE_NEAREST_FAST_PATH_PAD (SRC, r5g6b5, a8r8g8b8, mips_0565_8888),
    SIMPLE_NEAREST_FAST_PATH_PAD (SRC, b5g6r5, a8b8g8r8, mips_0565_8888),

    SIMPLE_NEAREST_A8_MASK_FAST_PATH (OVER, a8r8g8b8, r5g6b5, mips_8888_8_0565),
    SIMPLE_NEAREST_A8_MASK_FAST_PATH (OVER, a8b8g8r8, b5g6r5, mips_8888_8_0565),

    SIMPLE_NEAREST_A8_MASK_FAST_PATH (OVER, r5g6b5, r5g6b5, mips_0565_8_0565),
    SIMPLE_NEAREST_A8_MASK_FAST_PATH (OVER, b5g6r5, b5g6r5, mips_0565_8_0565),

    SIMPLE_BILINEAR_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8, mips_8888_8888),
    SIMPLE_BILINEAR_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8, mips_8888_8888),
    SIMPLE_BILINEAR_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8, mips_8888_8888),

    SIMPLE_BILINEAR_FAST_PATH (SRC, a8r8g8b8, r5g6b5, mips_8888_0565),
    SIMPLE_BILINEAR_FAST_PATH (SRC, x8r8g8b8, r5g6b5, mips_8888_0565),

    SIMPLE_BILINEAR_FAST_PATH (SRC, r5g6b5, x8r8g8b8, mips_0565_8888),
    SIMPLE_BILINEAR_FAST_PATH (SRC, r5g6b5, r5g6b5, mips_0565_0565),

    SIMPLE_BILINEAR_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, mips_8888_8888),
    SIMPLE_BILINEAR_FAST_PATH (OVER, a8r8g8b8, x8r8g8b8, mips_8888_8888),

    SIMPLE_BILINEAR_FAST_PATH (ADD, a8r8g8b8, a8r8g8b8, mips_8888_8888),
    SIMPLE_BILINEAR_FAST_PATH (ADD, a8r8g8b8, x8r8g8b8, mips_8888_8888),

    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8, mips_8888_8_8888),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8, mips_8888_8_8888),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8, mips_8888_8_8888),

    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, a8r8g8b8, r5g6b5, mips_8888_8_0565),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, x8r8g8b8, r5g6b5, mips_8888_8_0565),

    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, r5g6b5, x8r8g8b8, mips_0565_8_x888),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (SRC, r5g6b5, r5g6b5, mips_0565_8_0565),

    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, mips_8888_8_8888),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (OVER, a8r8g8b8, x8r8g8b8, mips_8888_8_8888),

    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (ADD, a8r8g8b8, a8r8g8b8, mips_8888_8_8888),
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH (ADD, a8r8g8b8, x8r8g8b8, mips_8888_8_8888),
    { PIXMAN_OP_NONE },
};

static void
mips_dspr2_combine_over_u (pixman_implementation_t *imp,
                           pixman_op_t              op,
                           uint32_t *               dest,
                           const uint32_t *         src,
                           const uint32_t *         mask,
                           int                      width)
{
    if (mask)
        pixman_composite_over_8888_8888_8888_asm_mips (
            dest, (uint32_t *)src, (uint32_t *)mask, width);
    else
        pixman_composite_over_8888_8888_asm_mips (
		    dest, (uint32_t *)src, width);
}

pixman_implementation_t *
_pixman_implementation_create_mips_dspr2 (pixman_implementation_t *fallback)
{
    pixman_implementation_t *imp =
        _pixman_implementation_create (fallback, mips_dspr2_fast_paths);

    imp->combine_32[PIXMAN_OP_OVER] = mips_dspr2_combine_over_u;

    imp->blt = mips_dspr2_blt;
    imp->fill = mips_dspr2_fill;

    return imp;
}
