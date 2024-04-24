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

#ifndef PIXMAN_MIPS_DSPR2_H
#define PIXMAN_MIPS_DSPR2_H

#include "pixman-private.h"
#include "pixman-inlines.h"

#define SKIP_ZERO_SRC  1
#define SKIP_ZERO_MASK 2
#define DO_FAST_MEMCPY 3

void
pixman_mips_fast_memcpy (void *dst, void *src, uint32_t n_bytes);
void
pixman_fill_buff16_mips (void *dst, uint32_t n_bytes, uint16_t value);
void
pixman_fill_buff32_mips (void *dst, uint32_t n_bytes, uint32_t value);

/****************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_SRC_DST(flags, name,          \
                                           src_type, src_cnt,    \
                                           dst_type, dst_cnt)    \
void                                                             \
pixman_composite_##name##_asm_mips (dst_type *dst,               \
                                    src_type *src,               \
                                    int32_t   w);                \
                                                                 \
static void                                                      \
mips_composite_##name (pixman_implementation_t *imp,             \
                       pixman_composite_info_t *info)            \
{                                                                \
    PIXMAN_COMPOSITE_ARGS (info);                                \
    dst_type *dst_line, *dst;                                    \
    src_type *src_line, *src;                                    \
    int32_t dst_stride, src_stride;                              \
    int bpp = PIXMAN_FORMAT_BPP (dest_image->bits.format) / 8;   \
                                                                 \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,    \
                           src_stride, src_line, src_cnt);       \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type, \
                           dst_stride, dst_line, dst_cnt);       \
                                                                 \
    while (height--)                                             \
    {                                                            \
      dst = dst_line;                                            \
      dst_line += dst_stride;                                    \
      src = src_line;                                            \
      src_line += src_stride;                                    \
                                                                 \
      if (flags == DO_FAST_MEMCPY)                               \
        pixman_mips_fast_memcpy (dst, src, width * bpp);         \
      else                                                       \
        pixman_composite_##name##_asm_mips (dst, src, width);    \
    }                                                            \
}

/****************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_N_DST(flags, name,            \
                                         dst_type, dst_cnt)      \
void                                                             \
pixman_composite_##name##_asm_mips (dst_type *dst,               \
                                    uint32_t  src,               \
                                    int32_t   w);                \
                                                                 \
static void                                                      \
mips_composite_##name (pixman_implementation_t *imp,             \
                       pixman_composite_info_t *info)            \
{                                                                \
    PIXMAN_COMPOSITE_ARGS (info);                                \
    dst_type  *dst_line, *dst;                                   \
    int32_t    dst_stride;                                       \
    uint32_t   src;                                              \
                                                                 \
    src = _pixman_image_get_solid (                              \
    imp, src_image, dest_image->bits.format);                    \
                                                                 \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                     \
        return;                                                  \
                                                                 \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type, \
                           dst_stride, dst_line, dst_cnt);       \
                                                                 \
    while (height--)                                             \
    {                                                            \
        dst = dst_line;                                          \
        dst_line += dst_stride;                                  \
                                                                 \
        pixman_composite_##name##_asm_mips (dst, src, width);    \
    }                                                            \
}

/*******************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_N_MASK_DST(flags, name,          \
                                              mask_type, mask_cnt,  \
                                              dst_type, dst_cnt)    \
void                                                                \
pixman_composite_##name##_asm_mips (dst_type  *dst,                 \
                                    uint32_t  src,                  \
                                    mask_type *mask,                \
                                    int32_t   w);                   \
                                                                    \
static void                                                         \
mips_composite_##name (pixman_implementation_t *imp,                \
                       pixman_composite_info_t *info)               \
{                                                                   \
    PIXMAN_COMPOSITE_ARGS (info);                                   \
    dst_type  *dst_line, *dst;                                      \
    mask_type *mask_line, *mask;                                    \
    int32_t    dst_stride, mask_stride;                             \
    uint32_t   src;                                                 \
                                                                    \
    src = _pixman_image_get_solid (                                 \
        imp, src_image, dest_image->bits.format);                   \
                                                                    \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                        \
        return;                                                     \
                                                                    \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,    \
                           dst_stride, dst_line, dst_cnt);          \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,   \
                           mask_stride, mask_line, mask_cnt);       \
                                                                    \
    while (height--)                                                \
    {                                                               \
        dst = dst_line;                                             \
        dst_line += dst_stride;                                     \
        mask = mask_line;                                           \
        mask_line += mask_stride;                                   \
        pixman_composite_##name##_asm_mips (dst, src, mask, width); \
    }                                                               \
}

/*******************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_SRC_N_DST(flags, name,           \
                                            src_type, src_cnt,      \
                                            dst_type, dst_cnt)      \
void                                                                \
pixman_composite_##name##_asm_mips (dst_type  *dst,                 \
                                    src_type  *src,                 \
                                    uint32_t   mask,                \
                                    int32_t    w);                  \
                                                                    \
static void                                                         \
mips_composite_##name (pixman_implementation_t *imp,                \
                       pixman_composite_info_t *info)               \
{                                                                   \
    PIXMAN_COMPOSITE_ARGS (info);                                   \
    dst_type  *dst_line, *dst;                                      \
    src_type  *src_line, *src;                                      \
    int32_t    dst_stride, src_stride;                              \
    uint32_t   mask;                                                \
                                                                    \
    mask = _pixman_image_get_solid (                                \
        imp, mask_image, dest_image->bits.format);                  \
                                                                    \
    if ((flags & SKIP_ZERO_MASK) && mask == 0)                      \
        return;                                                     \
                                                                    \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,    \
                           dst_stride, dst_line, dst_cnt);          \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,       \
                           src_stride, src_line, src_cnt);          \
                                                                    \
    while (height--)                                                \
    {                                                               \
        dst = dst_line;                                             \
        dst_line += dst_stride;                                     \
        src = src_line;                                             \
        src_line += src_stride;                                     \
                                                                    \
        pixman_composite_##name##_asm_mips (dst, src, mask, width); \
    }                                                               \
}

/************************************************************************/

#define PIXMAN_MIPS_BIND_FAST_PATH_SRC_MASK_DST(name, src_type, src_cnt, \
                                                mask_type, mask_cnt,     \
                                                dst_type, dst_cnt)       \
void                                                                     \
pixman_composite_##name##_asm_mips (dst_type  *dst,                      \
                                    src_type  *src,                      \
                                    mask_type *mask,                     \
                                    int32_t   w);                        \
                                                                         \
static void                                                              \
mips_composite_##name (pixman_implementation_t *imp,                     \
                       pixman_composite_info_t *info)                    \
{                                                                        \
    PIXMAN_COMPOSITE_ARGS (info);                                        \
    dst_type  *dst_line, *dst;                                           \
    src_type  *src_line, *src;                                           \
    mask_type *mask_line, *mask;                                         \
    int32_t    dst_stride, src_stride, mask_stride;                      \
                                                                         \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);               \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,            \
                           src_stride, src_line, src_cnt);               \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,        \
                           mask_stride, mask_line, mask_cnt);            \
                                                                         \
    while (height--)                                                     \
    {                                                                    \
        dst = dst_line;                                                  \
        dst_line += dst_stride;                                          \
        mask = mask_line;                                                \
        mask_line += mask_stride;                                        \
        src = src_line;                                                  \
        src_line += src_stride;                                          \
        pixman_composite_##name##_asm_mips (dst, src, mask, width);      \
    }                                                                    \
}

/****************************************************************************/

#define PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_DST(name, op,                    \
                                                src_type, dst_type)          \
void                                                                         \
pixman_scaled_nearest_scanline_##name##_##op##_asm_mips (                    \
                                                   dst_type *       dst,     \
                                                   const src_type * src,     \
                                                   int32_t          w,       \
                                                   pixman_fixed_t   vx,      \
                                                   pixman_fixed_t   unit_x); \
                                                                             \
static force_inline void                                                     \
scaled_nearest_scanline_mips_##name##_##op (dst_type *       pd,             \
                                            const src_type * ps,             \
                                            int32_t          w,              \
                                            pixman_fixed_t   vx,             \
                                            pixman_fixed_t   unit_x,         \
                                            pixman_fixed_t   max_vx,         \
                                            pixman_bool_t    zero_src)       \
{                                                                            \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_mips (pd, ps, w,      \
                                                             vx, unit_x);    \
}                                                                            \
                                                                             \
FAST_NEAREST_MAINLOOP (mips_##name##_cover_##op,                             \
                       scaled_nearest_scanline_mips_##name##_##op,           \
                       src_type, dst_type, COVER)                            \
FAST_NEAREST_MAINLOOP (mips_##name##_none_##op,                              \
                       scaled_nearest_scanline_mips_##name##_##op,           \
                       src_type, dst_type, NONE)                             \
FAST_NEAREST_MAINLOOP (mips_##name##_pad_##op,                               \
                       scaled_nearest_scanline_mips_##name##_##op,           \
                       src_type, dst_type, PAD)

/* Provide entries for the fast path table */
#define PIXMAN_MIPS_SIMPLE_NEAREST_FAST_PATH(op,s,d,func)                    \
    SIMPLE_NEAREST_FAST_PATH_COVER (op,s,d,func),                            \
    SIMPLE_NEAREST_FAST_PATH_NONE (op,s,d,func),                             \
    SIMPLE_NEAREST_FAST_PATH_PAD (op,s,d,func)


/*****************************************************************************/

#define PIXMAN_MIPS_BIND_SCALED_NEAREST_SRC_A8_DST(flags, name, op,           \
                                                  src_type, dst_type)         \
void                                                                          \
pixman_scaled_nearest_scanline_##name##_##op##_asm_mips (                     \
                                                   dst_type *       dst,      \
                                                   const src_type * src,      \
                                                   const uint8_t *  mask,     \
                                                   int32_t          w,        \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x);  \
                                                                              \
static force_inline void                                                      \
scaled_nearest_scanline_mips_##name##_##op (const uint8_t *  mask,            \
                                            dst_type *       pd,              \
                                            const src_type * ps,              \
                                            int32_t          w,               \
                                            pixman_fixed_t   vx,              \
                                            pixman_fixed_t   unit_x,          \
                                            pixman_fixed_t   max_vx,          \
                                            pixman_bool_t    zero_src)        \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
        return;                                                               \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_mips (pd, ps,          \
                                                             mask, w,         \
                                                             vx, unit_x);     \
}                                                                             \
                                                                              \
FAST_NEAREST_MAINLOOP_COMMON (mips_##name##_cover_##op,                       \
                              scaled_nearest_scanline_mips_##name##_##op,     \
                              src_type, uint8_t, dst_type, COVER, TRUE, FALSE)\
FAST_NEAREST_MAINLOOP_COMMON (mips_##name##_none_##op,                        \
                              scaled_nearest_scanline_mips_##name##_##op,     \
                              src_type, uint8_t, dst_type, NONE, TRUE, FALSE) \
FAST_NEAREST_MAINLOOP_COMMON (mips_##name##_pad_##op,                         \
                              scaled_nearest_scanline_mips_##name##_##op,     \
                              src_type, uint8_t, dst_type, PAD, TRUE, FALSE)

/****************************************************************************/

#define PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_DST(flags, name, op,            \
                                                 src_type, dst_type)         \
void                                                                         \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_mips(                    \
                                             dst_type *       dst,           \
                                             const src_type * src_top,       \
                                             const src_type * src_bottom,    \
                                             int32_t          w,             \
                                             int              wt,            \
                                             int              wb,            \
                                             pixman_fixed_t   vx,            \
                                             pixman_fixed_t   unit_x);       \
static force_inline void                                                     \
scaled_bilinear_scanline_mips_##name##_##op (dst_type *       dst,           \
                                             const uint32_t * mask,          \
                                             const src_type * src_top,       \
                                             const src_type * src_bottom,    \
                                             int32_t          w,             \
                                             int              wt,            \
                                             int              wb,            \
                                             pixman_fixed_t   vx,            \
                                             pixman_fixed_t   unit_x,        \
                                             pixman_fixed_t   max_vx,        \
                                             pixman_bool_t    zero_src)      \
{                                                                            \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                 \
        return;                                                              \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_mips (dst, src_top,  \
                                                              src_bottom, w, \
                                                              wt, wb,        \
                                                              vx, unit_x);   \
}                                                                            \
                                                                             \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_cover_##op,                     \
                       scaled_bilinear_scanline_mips_##name##_##op,          \
                       src_type, uint32_t, dst_type, COVER, FLAG_NONE)       \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_none_##op,                      \
                       scaled_bilinear_scanline_mips_##name##_##op,          \
                       src_type, uint32_t, dst_type, NONE, FLAG_NONE)        \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_pad_##op,                       \
                       scaled_bilinear_scanline_mips_##name##_##op,          \
                       src_type, uint32_t, dst_type, PAD, FLAG_NONE)         \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_normal_##op,                    \
                       scaled_bilinear_scanline_mips_##name##_##op,          \
                       src_type, uint32_t, dst_type, NORMAL,                 \
                       FLAG_NONE)

/*****************************************************************************/

#define PIXMAN_MIPS_BIND_SCALED_BILINEAR_SRC_A8_DST(flags, name, op,          \
                                                src_type, dst_type)           \
void                                                                          \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_mips (                    \
                                             dst_type *       dst,            \
                                             const uint8_t *  mask,           \
                                             const src_type * top,            \
                                             const src_type * bottom,         \
                                             int              wt,             \
                                             int              wb,             \
                                             pixman_fixed_t   x,              \
                                             pixman_fixed_t   ux,             \
                                             int              width);         \
                                                                              \
static force_inline void                                                      \
scaled_bilinear_scanline_mips_##name##_##op (dst_type *       dst,            \
                                             const uint8_t *  mask,           \
                                             const src_type * src_top,        \
                                             const src_type * src_bottom,     \
                                             int32_t          w,              \
                                             int              wt,             \
                                             int              wb,             \
                                             pixman_fixed_t   vx,             \
                                             pixman_fixed_t   unit_x,         \
                                             pixman_fixed_t   max_vx,         \
                                             pixman_bool_t    zero_src)       \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
        return;                                                               \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_mips (                \
                      dst, mask, src_top, src_bottom, wt, wb, vx, unit_x, w); \
}                                                                             \
                                                                              \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_cover_##op,                      \
                       scaled_bilinear_scanline_mips_##name##_##op,           \
                       src_type, uint8_t, dst_type, COVER,                    \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_none_##op,                       \
                       scaled_bilinear_scanline_mips_##name##_##op,           \
                       src_type, uint8_t, dst_type, NONE,                     \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_pad_##op,                        \
                       scaled_bilinear_scanline_mips_##name##_##op,           \
                       src_type, uint8_t, dst_type, PAD,                      \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (mips_##name##_normal_##op,                     \
                       scaled_bilinear_scanline_mips_##name##_##op,           \
                       src_type, uint8_t, dst_type, NORMAL,                   \
                       FLAG_HAVE_NON_SOLID_MASK)

#endif //PIXMAN_MIPS_DSPR2_H
