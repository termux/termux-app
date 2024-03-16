/*
 * Copyright © 2010 Nokia Corporation
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
 *
 * Author:  Siarhei Siamashka (siarhei.siamashka@nokia.com)
 */

#ifndef PIXMAN_ARM_COMMON_H
#define PIXMAN_ARM_COMMON_H

#include "pixman-inlines.h"

/* Define some macros which can expand into proxy functions between
 * ARM assembly optimized functions and the rest of pixman fast path API.
 *
 * All the low level ARM assembly functions have to use ARM EABI
 * calling convention and take up to 8 arguments:
 *    width, height, dst, dst_stride, src, src_stride, mask, mask_stride
 *
 * The arguments are ordered with the most important coming first (the
 * first 4 arguments are passed to function in registers, the rest are
 * on stack). The last arguments are optional, for example if the
 * function is not using mask, then 'mask' and 'mask_stride' can be
 * omitted when doing a function call.
 *
 * Arguments 'src' and 'mask' contain either a pointer to the top left
 * pixel of the composited rectangle or a pixel color value depending
 * on the function type. In the case of just a color value (solid source
 * or mask), the corresponding stride argument is unused.
 */

#define SKIP_ZERO_SRC  1
#define SKIP_ZERO_MASK 2

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(cputype, name,                \
                                          src_type, src_cnt,            \
                                          dst_type, dst_cnt)            \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t   w,                   \
                                         int32_t   h,                   \
                                         dst_type *dst,                 \
                                         int32_t   dst_stride,          \
                                         src_type *src,                 \
                                         int32_t   src_stride);         \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type *dst_line;							\
    src_type *src_line;                                                 \
    int32_t dst_stride, src_stride;                                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride);     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_DST(flags, cputype, name,           \
                                        dst_type, dst_cnt)              \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src);               \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
			    pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);					\
    dst_type  *dst_line;                                                \
    int32_t    dst_stride;                                              \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (					\
	imp, src_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                            \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src);                      \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(flags, cputype, name,      \
                                             mask_type, mask_cnt,       \
                                             dst_type, dst_cnt)         \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src,                \
                                         int32_t    unused,             \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    mask_type *mask_line;                                               \
    int32_t    dst_stride, mask_stride;                                 \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (					\
	imp, src_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                            \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src, 0,                    \
                                             mask_line, mask_stride);   \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(flags, cputype, name,       \
                                            src_type, src_cnt,          \
                                            dst_type, dst_cnt)          \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         uint32_t   mask);              \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    src_type  *src_line;                                                \
    int32_t    dst_stride, src_stride;                                  \
    uint32_t   mask;                                                    \
                                                                        \
    mask = _pixman_image_get_solid (					\
	imp, mask_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_MASK) && mask == 0)                          \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask);                     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(cputype, name,           \
                                               src_type, src_cnt,       \
                                               mask_type, mask_cnt,     \
                                               dst_type, dst_cnt)       \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    src_type  *src_line;                                                \
    mask_type *mask_line;                                               \
    int32_t    dst_stride, src_stride, mask_stride;                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask_line, mask_stride);   \
}

#define PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(cputype, name, op,             \
                                               src_type, dst_type)            \
void                                                                          \
pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (                \
                                                   int32_t          w,        \
                                                   dst_type *       dst,      \
                                                   const src_type * src,      \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx);  \
                                                                              \
static force_inline void                                                      \
scaled_nearest_scanline_##cputype##_##name##_##op (dst_type *       pd,       \
                                                   const src_type * ps,       \
                                                   int32_t          w,        \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   pixman_bool_t    zero_src) \
{                                                                             \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (w, pd, ps,  \
                                                                  vx, unit_x, \
                                                                  max_vx);    \
}                                                                             \
                                                                              \
FAST_NEAREST_MAINLOOP (cputype##_##name##_cover_##op,                         \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, COVER)                             \
FAST_NEAREST_MAINLOOP (cputype##_##name##_none_##op,                          \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, NONE)                              \
FAST_NEAREST_MAINLOOP (cputype##_##name##_pad_##op,                           \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, PAD)                               \
FAST_NEAREST_MAINLOOP (cputype##_##name##_normal_##op,                        \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, NORMAL)

#define PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_A8_DST(flags, cputype, name, op,   \
                                                  src_type, dst_type)         \
void                                                                          \
pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (                \
                                                   int32_t          w,        \
                                                   dst_type *       dst,      \
                                                   const src_type * src,      \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   const uint8_t *  mask);    \
                                                                              \
static force_inline void                                                      \
scaled_nearest_scanline_##cputype##_##name##_##op (const uint8_t *  mask,     \
                                                   dst_type *       pd,       \
                                                   const src_type * ps,       \
                                                   int32_t          w,        \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   pixman_bool_t    zero_src) \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                               \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (w, pd, ps,  \
                                                                  vx, unit_x, \
                                                                  max_vx,     \
                                                                  mask);      \
}                                                                             \
                                                                              \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                  \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, COVER, TRUE, FALSE)\
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_none_##op,                   \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, NONE, TRUE, FALSE) \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                    \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, PAD, TRUE, FALSE)  \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                 \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, NORMAL, TRUE, FALSE)

/* Provide entries for the fast path table */
#define PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(op,s,d,func)              \
    SIMPLE_NEAREST_A8_MASK_FAST_PATH (op,s,d,func),                           \
    SIMPLE_NEAREST_A8_MASK_FAST_PATH_NORMAL (op,s,d,func)

/*****************************************************************************/

#define PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(flags, cputype, name, op,     \
                                                src_type, dst_type)           \
void                                                                          \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (               \
                                                dst_type *       dst,         \
                                                const src_type * top,         \
                                                const src_type * bottom,      \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   x,           \
                                                pixman_fixed_t   ux,          \
                                                int              width);      \
                                                                              \
static force_inline void                                                      \
scaled_bilinear_scanline_##cputype##_##name##_##op (                          \
                                                dst_type *       dst,         \
                                                const uint32_t * mask,        \
                                                const src_type * src_top,     \
                                                const src_type * src_bottom,  \
                                                int32_t          w,           \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   vx,          \
                                                pixman_fixed_t   unit_x,      \
                                                pixman_fixed_t   max_vx,      \
                                                pixman_bool_t    zero_src)    \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                               \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (           \
                            dst, src_top, src_bottom, wt, wb, vx, unit_x, w); \
}                                                                             \
                                                                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                 \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, COVER, FLAG_NONE)        \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_none_##op,                  \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, NONE, FLAG_NONE)         \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                   \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, PAD, FLAG_NONE)          \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, NORMAL,                  \
                       FLAG_NONE)


#define PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(flags, cputype, name, op,  \
                                                src_type, dst_type)           \
void                                                                          \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (               \
                                                dst_type *       dst,         \
                                                const uint8_t *  mask,        \
                                                const src_type * top,         \
                                                const src_type * bottom,      \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   x,           \
                                                pixman_fixed_t   ux,          \
                                                int              width);      \
                                                                              \
static force_inline void                                                      \
scaled_bilinear_scanline_##cputype##_##name##_##op (                          \
                                                dst_type *       dst,         \
                                                const uint8_t *  mask,        \
                                                const src_type * src_top,     \
                                                const src_type * src_bottom,  \
                                                int32_t          w,           \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   vx,          \
                                                pixman_fixed_t   unit_x,      \
                                                pixman_fixed_t   max_vx,      \
                                                pixman_bool_t    zero_src)    \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                                   \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (           \
                      dst, mask, src_top, src_bottom, wt, wb, vx, unit_x, w); \
}                                                                             \
                                                                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                 \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, COVER,                    \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_none_##op,                  \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, NONE,                     \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                   \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, PAD,                      \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, NORMAL,                   \
                       FLAG_HAVE_NON_SOLID_MASK)


#endif
