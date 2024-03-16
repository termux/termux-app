/*
 * Copyright © 2013 Soeren Sandmann
 * Copyright © 2013 Red Hat, Inc.
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
#include <stdio.h>
#include <stdlib.h> /* abort() */
#include <math.h>
#include <time.h>
#include "utils.h"

typedef struct pixel_combination_t pixel_combination_t;
struct pixel_combination_t
{
    pixman_op_t			op;
    pixman_format_code_t	src_format;
    uint32_t			src_pixel;
    pixman_format_code_t	mask_format;
    uint32_t			mask_pixel;
    pixman_format_code_t	dest_format;
    uint32_t			dest_pixel;
};

static const pixel_combination_t regressions[] =
{
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffc3ff,
      PIXMAN_a8,	0x7b,
      PIXMAN_a8r8g8b8,	0xff00c300,
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb5,
      PIXMAN_a4r4g4b4,	0xe3ff,
      PIXMAN_a2r2g2b2,	0x2e
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa6,
      PIXMAN_a8r8g8b8,	0x2b00ff00,
      PIXMAN_a4r4g4b4,	0x7e
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0x27000013,
      PIXMAN_a2r2g2b2,	0x80,
      PIXMAN_a4r4g4b4,	0x9d
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xe6f7,
      PIXMAN_a2r2g2b2,	0xad,
      PIXMAN_a4r4g4b4,	0x71
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0xff4f70ff,
      PIXMAN_r5g6b5,	0xb828,
      PIXMAN_a8r8g8b8,	0xcac400
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa9,
      PIXMAN_a4r4g4b4,	0x41c2,
      PIXMAN_a8r8g8b8,	0xffff2b
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x89,
      PIXMAN_a8r8g8b8,	0x977cff61,
      PIXMAN_a4r4g4b4,	0x36
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x81,
      PIXMAN_r5g6b5,	0x6f9e,
      PIXMAN_a4r4g4b4,	0x1eb
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb5,
      PIXMAN_a4r4g4b4,	0xe247,
      PIXMAN_a8r8g8b8,	0xffbaff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x97,
      PIXMAN_a2r2g2b2,	0x9d,
      PIXMAN_a2r2g2b2,	0x21
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb4,
      PIXMAN_a2r2g2b2,	0x90,
      PIXMAN_a8r8g8b8,	0xc0fd5c
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0xdf00ff70,
      PIXMAN_a8r8g8b8,	0x2597ff27,
      PIXMAN_a4r4g4b4,	0xf3
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb7,
      PIXMAN_r3g3b2,	0xb1,
      PIXMAN_a8r8g8b8,	0x9f4bcc
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xf39e,
      PIXMAN_r5g6b5,	0x34,
      PIXMAN_a8r8g8b8,	0xf6ae00
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0x3aff1dff,
      PIXMAN_a2r2g2b2,	0x64,
      PIXMAN_a8r8g8b8,	0x94ffb4
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa4,
      PIXMAN_a2r2g2b2,	0x8a,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa5,
      PIXMAN_a4r4g4b4,	0x1a,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb4,
      PIXMAN_a2r2g2b2,	0xca,
      PIXMAN_a4r4g4b4,	0x7b
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xbd,
      PIXMAN_a4r4g4b4,	0xff37,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x96,
      PIXMAN_a2r2g2b2,	0xbb,
      PIXMAN_a8r8g8b8,	0x96ffff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x89,
      PIXMAN_r3g3b2,	0x92,
      PIXMAN_a4r4g4b4,	0xa8c
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xa95b,
      PIXMAN_a2r2g2b2,	0x68,
      PIXMAN_a8r8g8b8,	0x38ff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x90,
      PIXMAN_a8r8g8b8,	0x53bd00ef,
      PIXMAN_a8r8g8b8,	0xff0003
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1f5ffff,
      PIXMAN_r3g3b2,	0x22,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x10000b6,
      PIXMAN_a8r8g8b8,	0x9645,
      PIXMAN_r5g6b5,	0x6
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x172ff00,
      PIXMAN_a4r4g4b4,	0xff61,
      PIXMAN_r3g3b2,	0xc
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x281ffc8,
      PIXMAN_r5g6b5,	0x39b8,
      PIXMAN_r5g6b5,	0x13
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100a2ff,
      PIXMAN_a4r4g4b4,	0x6500,
      PIXMAN_a2r2g2b2,	0x5
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff51,
      PIXMAN_r5g6b5,	0x52ff,
      PIXMAN_a2r2g2b2,	0x14
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x150d500,
      PIXMAN_a8r8g8b8,	0x6200b7ff,
      PIXMAN_a8r8g8b8,	0x1f5200
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2a9a700,
      PIXMAN_a8r8g8b8,	0xf7003400,
      PIXMAN_a8r8g8b8,	0x2200
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x200ffff,
      PIXMAN_r5g6b5,	0x81ff,
      PIXMAN_r5g6b5,	0x1f
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ff00ff,
      PIXMAN_r5g6b5,	0x3f00,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3ff1aa4,
      PIXMAN_a4r4g4b4,	0x2200,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x280ff2c,
      PIXMAN_r3g3b2,	0xc6,
      PIXMAN_a8r8g8b8,	0xfdfd44fe
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x13aff1d,
      PIXMAN_a2r2g2b2,	0x4b,
      PIXMAN_r5g6b5,	0x12a1
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2ffff88,
      PIXMAN_a8r8g8b8,	0xff3a49,
      PIXMAN_r5g6b5,	0xf7df
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1009700,
      PIXMAN_a2r2g2b2,	0x56,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1aacbff,
      PIXMAN_a4r4g4b4,	0x84,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x100b1ff,
      PIXMAN_a2r2g2b2,	0xf5,
      PIXMAN_a8r8g8b8,	0xfea89cff
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ff0000,
      PIXMAN_r5g6b5,	0x6800,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x10064ff,
      PIXMAN_r3g3b2,	0x61,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1bb00ff,
      PIXMAN_r5g6b5,	0x76b5,
      PIXMAN_a4r4g4b4,	0x500
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ffff41,
      PIXMAN_r5g6b5,	0x7100,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ff1231,
      PIXMAN_a8r8g8b8,	0x381089,
      PIXMAN_r5g6b5,	0x38a5
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x16e5c49,
      PIXMAN_a8r8g8b8,	0x4dfa3694,
      PIXMAN_a8r8g8b8,	0x211c16
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x134ff62,
      PIXMAN_a2r2g2b2,	0x14,
      PIXMAN_r3g3b2,	0x8
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x300ffeb,
      PIXMAN_r3g3b2,	0xc7,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x3ff8bff,
      PIXMAN_r3g3b2,	0x3e,
      PIXMAN_a8r8g8b8,	0x3008baa
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff00ff,
      PIXMAN_a4r4g4b4,	0x3466,
      PIXMAN_a4r4g4b4,	0x406
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ddc027,
      PIXMAN_a4r4g4b4,	0x7d00,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2ffff00,
      PIXMAN_a8r8g8b8,	0xc92cfb52,
      PIXMAN_a4r4g4b4,	0x200
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ff116a,
      PIXMAN_a4r4g4b4,	0x6000,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r5g6b5,	0x2f95,
      PIXMAN_r5g6b5,	0x795
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffff00,
      PIXMAN_a4r4g4b4,	0x354a,
      PIXMAN_r5g6b5,	0x3180
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1d7ff00,
      PIXMAN_a4r4g4b4,	0xd6ff,
      PIXMAN_a8r8g8b8,	0xffff0700
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1bc5db7,
      PIXMAN_r5g6b5,	0x944f,
      PIXMAN_a4r4g4b4,	0xff05
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x185ffd9,
      PIXMAN_a2r2g2b2,	0x9c,
      PIXMAN_r5g6b5,	0x3c07
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1fa7f61,
      PIXMAN_a8r8g8b8,	0xff31ff00,
      PIXMAN_r3g3b2,	0xd2
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1c4ff00,
      PIXMAN_r3g3b2,	0xb,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ff00ff,
      PIXMAN_a8r8g8b8,	0x3f3caeda,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ff00,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_r5g6b5,	0xe0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff68ff,
      PIXMAN_a4r4g4b4,	0x8046,
      PIXMAN_r5g6b5,	0xec
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x100ff28,
      PIXMAN_a8r8g8b8,	0x4c00,
      PIXMAN_r5g6b5,	0x260
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_a4r4g4b4,	0xd92a,
      PIXMAN_a8r8g8b8,	0x2200
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100289a,
      PIXMAN_a8r8g8b8,	0x74ffb8ff,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1baff00,
      PIXMAN_r5g6b5,	0x4e9d,
      PIXMAN_r5g6b5,	0x3000
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1fcffad,
      PIXMAN_r5g6b5,	0x42d7,
      PIXMAN_a8r8g8b8,	0x1c6ffe5
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x183ff00,
      PIXMAN_r3g3b2,	0x7e,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x2ff0076,
      PIXMAN_a8r8g8b8,	0x2a0000,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3d8bbff,
      PIXMAN_r5g6b5,	0x6900,
      PIXMAN_a8r8g8b8,	0x35b0000
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x14f00ff,
      PIXMAN_r5g6b5,	0xd48,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x28c72df,
      PIXMAN_a8r8g8b8,	0xff5cff31,
      PIXMAN_a4r4g4b4,	0x2
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffffff,
      PIXMAN_a8r8g8b8,	0xffad8020,
      PIXMAN_r5g6b5,	0x4
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x100ff00,
      PIXMAN_a2r2g2b2,	0x76,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1005d00,
      PIXMAN_r5g6b5,	0x7b04,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x3cdfc3e,
      PIXMAN_a8r8g8b8,	0x69ec21d3,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x200ffff,
      PIXMAN_r5g6b5,	0x30ff,
      PIXMAN_r5g6b5,	0x60ff
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x532fff4,
      PIXMAN_r5g6b5,	0xcb,
      PIXMAN_r5g6b5,	0xd9a1
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r3g3b2,	0x5f,
      PIXMAN_a2r2g2b2,	0x10
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_a8r8g8b8,	0xffd60052,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ff6491,
      PIXMAN_a8r8g8b8,	0x1e53ff00,
      PIXMAN_r5g6b5,	0x1862
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_r3g3b2,	0xc7,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x29d0fff,
      PIXMAN_a4r4g4b4,	0x25ff,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x141760a,
      PIXMAN_a4r4g4b4,	0x7ec2,
      PIXMAN_a4r4g4b4,	0x130
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1abedff,
      PIXMAN_a8r8g8b8,	0x75520068,
      PIXMAN_r3g3b2,	0x87
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x10000ff,
      PIXMAN_a8r8g8b8,	0xff00e652,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x16006075,
      PIXMAN_r5g6b5,	0xc00,
      PIXMAN_a8r8g8b8,	0x27f0900
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x200ff00,
      PIXMAN_a8r8g8b8,	0xd1b83f57,
      PIXMAN_a4r4g4b4,	0xff75
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x14000c4,
      PIXMAN_a4r4g4b4,	0x96,
      PIXMAN_a2r2g2b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ff00d1,
      PIXMAN_r3g3b2,	0x79,
      PIXMAN_a2r2g2b2,	0x0
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff00dc,
      PIXMAN_a4r4g4b4,	0xc5ff,
      PIXMAN_a2r2g2b2,	0x10
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffb2,
      PIXMAN_a8r8g8b8,	0x4cff5700,
      PIXMAN_r3g3b2,	0x48
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1327482,
      PIXMAN_a8r8g8b8,	0x247ff,
      PIXMAN_a8r8g8b8,	0x82
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1d0ff00,
      PIXMAN_r3g3b2,	0xc9,
      PIXMAN_r5g6b5,	0x240
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x13d35ff,
      PIXMAN_a2r2g2b2,	0x6d,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffc6b2,
      PIXMAN_a8r8g8b8,	0x5abe8e3c,
      PIXMAN_r5g6b5,	0x5a27
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x15700ff,
      PIXMAN_r3g3b2,	0xdd,
      PIXMAN_a8r8g8b8,	0x55
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff11ff,
      PIXMAN_r3g3b2,	0x30,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff00ff,
      PIXMAN_a2r2g2b2,	0x6d,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1421d5f,
      PIXMAN_a4r4g4b4,	0xff85,
      PIXMAN_a8r8g8b8,	0x1420f00
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1d2ffff,
      PIXMAN_r5g6b5,	0xfc,
      PIXMAN_r5g6b5,	0x1c
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffff42,
      PIXMAN_a4r4g4b4,	0x7100,
      PIXMAN_a4r4g4b4,	0x771
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x25ae3d4,
      PIXMAN_a8r8g8b8,	0x39ffc99a,
      PIXMAN_a8r8g8b8,	0x14332f
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff0643,
      PIXMAN_a8r8g8b8,	0x4c000000,
      PIXMAN_r5g6b5,	0x4802
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1966a00,
      PIXMAN_r3g3b2,	0x46,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x387ff59,
      PIXMAN_r5g6b5,	0x512c,
      PIXMAN_r5g6b5,	0x120
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1f7ffb0,
      PIXMAN_r5g6b5,	0x63b8,
      PIXMAN_a8r8g8b8,	0x1000089
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x185841c,
      PIXMAN_a2r2g2b2,	0x5c,
      PIXMAN_a8r8g8b8,	0x8400
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffc3ff,
      PIXMAN_a8r8g8b8,	0xff7b,
      PIXMAN_a8r8g8b8,	0xff00c300
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff7500,
      PIXMAN_a2r2g2b2,	0x47,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1002361,
      PIXMAN_a2r2g2b2,	0x7e,
      PIXMAN_r5g6b5,	0x64
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x10000b6,
      PIXMAN_a8r8g8b8,	0x59004463,
      PIXMAN_a4r4g4b4,	0xffa7
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff5a49,
      PIXMAN_a8r8g8b8,	0xff3fff2b,
      PIXMAN_a8r8g8b8,	0x13f000c
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x3ffecfc,
      PIXMAN_r3g3b2,	0x3c,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1630044,
      PIXMAN_a2r2g2b2,	0x63,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1d2ff58,
      PIXMAN_a8r8g8b8,	0x8f77ff,
      PIXMAN_a4r4g4b4,	0x705
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x14dffff,
      PIXMAN_a2r2g2b2,	0x9a,
      PIXMAN_a8r8g8b8,	0x1a0000
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ff92,
      PIXMAN_a4r4g4b4,	0x540c,
      PIXMAN_r5g6b5,	0x2a6
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_a4r4g4b4,	0xddd5,
      PIXMAN_a4r4g4b4,	0xdd0
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r5g6b5,	0xff8c,
      PIXMAN_a4r4g4b4,	0xff0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r3g3b2,	0x66,
      PIXMAN_r5g6b5,	0x7d1f
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_a4r4g4b4,	0xff5b,
      PIXMAN_a8r8g8b8,	0x5500
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2ed2dff,
      PIXMAN_r5g6b5,	0x7ae7,
      PIXMAN_r3g3b2,	0xce
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1b13205,
      PIXMAN_a8r8g8b8,	0x35ffff00,
      PIXMAN_r5g6b5,	0x2040
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1e60dff,
      PIXMAN_a4r4g4b4,	0x760f,
      PIXMAN_a2r2g2b2,	0x11
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x10000ff,
      PIXMAN_a4r4g4b4,	0x3,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_a8r8g8b8,	0x6600,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x30000fa,
      PIXMAN_a4r4g4b4,	0x23b7,
      PIXMAN_a8r8g8b8,	0x21
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r3g3b2,	0x60,
      PIXMAN_r3g3b2,	0x60
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3b31b30,
      PIXMAN_r3g3b2,	0x2e,
      PIXMAN_a8r8g8b8,	0x3000c20
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x160ffff,
      PIXMAN_a4r4g4b4,	0xff42,
      PIXMAN_r3g3b2,	0xed
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x172ffff,
      PIXMAN_a4r4g4b4,	0x5100,
      PIXMAN_r3g3b2,	0x29
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x16300ff,
      PIXMAN_a4r4g4b4,	0x5007,
      PIXMAN_a8r8g8b8,	0x77
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2ffff3a,
      PIXMAN_a8r8g8b8,	0x26640083,
      PIXMAN_a4r4g4b4,	0x220
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x106ff60,
      PIXMAN_r5g6b5,	0xdce,
      PIXMAN_a8r8g8b8,	0x100ba00
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100e7ff,
      PIXMAN_r5g6b5,	0xa00,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2b500f1,
      PIXMAN_a4r4g4b4,	0x7339,
      PIXMAN_a8r8g8b8,	0x1000091
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff00ff,
      PIXMAN_a4r4g4b4,	0xc863,
      PIXMAN_r5g6b5,	0x6
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1ffffca,
      PIXMAN_a8r8g8b8,	0x8b4cf000,
      PIXMAN_r3g3b2,	0xd2
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1fffe00,
      PIXMAN_r3g3b2,	0x88,
      PIXMAN_r3g3b2,	0x8
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x16f0000,
      PIXMAN_a2r2g2b2,	0x59,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x377ff43,
      PIXMAN_a4r4g4b4,	0x2a,
      PIXMAN_a8r8g8b8,	0x2d
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x11dffff,
      PIXMAN_r3g3b2,	0xcb,
      PIXMAN_r3g3b2,	0x8
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_r5g6b5,	0xbdab,
      PIXMAN_a4r4g4b4,	0xbb0
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff3343,
      PIXMAN_a8r8g8b8,	0x7a00ffff,
      PIXMAN_a2r2g2b2,	0xd
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ebff4b,
      PIXMAN_r3g3b2,	0x26,
      PIXMAN_r3g3b2,	0x24
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x2c1b3ff,
      PIXMAN_a8r8g8b8,	0x3000152a,
      PIXMAN_r3g3b2,	0x24
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1a7ffff,
      PIXMAN_r3g3b2,	0x9,
      PIXMAN_r5g6b5,	0x24a
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x4ff00ec,
      PIXMAN_a8r8g8b8,	0x1da4961e,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff25ff,
      PIXMAN_a8r8g8b8,	0x64b0ff00,
      PIXMAN_r5g6b5,	0x606c
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1fd62ff,
      PIXMAN_a4r4g4b4,	0x76b1,
      PIXMAN_r5g6b5,	0x716e
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x194ffde,
      PIXMAN_r5g6b5,	0x47ff,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x108ffff,
      PIXMAN_a8r8g8b8,	0xffffff66,
      PIXMAN_r5g6b5,	0xff0c
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x5ffffff,
      PIXMAN_r5g6b5,	0xdf,
      PIXMAN_r5g6b5,	0xc0
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ad31,
      PIXMAN_a2r2g2b2,	0xc5,
      PIXMAN_a4r4g4b4,	0x31
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff34,
      PIXMAN_a8r8g8b8,	0x6a57c491,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1fffff1,
      PIXMAN_r3g3b2,	0xaf,
      PIXMAN_r5g6b5,	0xb01e
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff67ff,
      PIXMAN_a4r4g4b4,	0x50ff,
      PIXMAN_a8r8g8b8,	0x552255
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x11bffff,
      PIXMAN_r5g6b5,	0xef0c,
      PIXMAN_r5g6b5,	0xc
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x16cf37d,
      PIXMAN_a4r4g4b4,	0xc561,
      PIXMAN_r5g6b5,	0x2301
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffff9c,
      PIXMAN_a4r4g4b4,	0x2700,
      PIXMAN_a8r8g8b8,	0xffff
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x200f322,
      PIXMAN_a8r8g8b8,	0xff3c7e,
      PIXMAN_r5g6b5,	0x2
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1f14a33,
      PIXMAN_a8r8g8b8,	0x26cff79,
      PIXMAN_r3g3b2,	0xf9
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x11d922c,
      PIXMAN_r3g3b2,	0xab,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_a2r2g2b2,	0xf5,
      PIXMAN_r3g3b2,	0x9
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x18697ff,
      PIXMAN_a4r4g4b4,	0x5700,
      PIXMAN_r5g6b5,	0xfa6d
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x12000fc,
      PIXMAN_a2r2g2b2,	0x41,
      PIXMAN_a8r8g8b8,	0xb0054
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x100ccff,
      PIXMAN_a4r4g4b4,	0x657e,
      PIXMAN_r5g6b5,	0x3b1
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffff1f,
      PIXMAN_a2r2g2b2,	0xa6,
      PIXMAN_r5g6b5,	0x2a0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x11fff82,
      PIXMAN_a4r4g4b4,	0xff94,
      PIXMAN_a8r8g8b8,	0x1010123
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x154bd19,
      PIXMAN_a4r4g4b4,	0xb600,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x10000ff,
      PIXMAN_r5g6b5,	0x8e,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x21aff00,
      PIXMAN_r5g6b5,	0x71ff,
      PIXMAN_r3g3b2,	0xf2
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ad00a7,
      PIXMAN_a4r4g4b4,	0x23,
      PIXMAN_a8r8g8b8,	0x21
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x100ff00,
      PIXMAN_r5g6b5,	0xb343,
      PIXMAN_r3g3b2,	0xc
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x3ffa500,
      PIXMAN_a8r8g8b8,	0x1af5b4,
      PIXMAN_a8r8g8b8,	0xff1abc00
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ffff11,
      PIXMAN_a8r8g8b8,	0x9f334f,
      PIXMAN_a8r8g8b8,	0x9f0005
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2c75971,
      PIXMAN_a4r4g4b4,	0x3900,
      PIXMAN_a4r4g4b4,	0x211
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ff49,
      PIXMAN_a8r8g8b8,	0x813dc25e,
      PIXMAN_r5g6b5,	0x667d
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x10000ff,
      PIXMAN_a4r4g4b4,	0x4bff,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x20ebcff,
      PIXMAN_r5g6b5,	0xc9ff,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_r5g6b5,	0x51ff,
      PIXMAN_r3g3b2,	0x44
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffd158,
      PIXMAN_a8r8g8b8,	0x7d88ffce,
      PIXMAN_r3g3b2,	0x6c
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1425e21,
      PIXMAN_a2r2g2b2,	0xa5,
      PIXMAN_r5g6b5,	0xe1
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x14b00ff,
      PIXMAN_a8r8g8b8,	0xbe95004b,
      PIXMAN_r5g6b5,	0x9
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x14fc0cd,
      PIXMAN_a8r8g8b8,	0x2d12b78b,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff8230,
      PIXMAN_a2r2g2b2,	0x4c,
      PIXMAN_r3g3b2,	0x44
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff31ff,
      PIXMAN_a2r2g2b2,	0x14,
      PIXMAN_a8r8g8b8,	0x551000
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x17800ff,
      PIXMAN_a4r4g4b4,	0x22,
      PIXMAN_a8r8g8b8,	0x22
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x14500ff,
      PIXMAN_a4r4g4b4,	0x6400,
      PIXMAN_r5g6b5,	0xff78
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ff9d,
      PIXMAN_r3g3b2,	0xcd,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x3ff00ff,
      PIXMAN_a4r4g4b4,	0xf269,
      PIXMAN_a4r4g4b4,	0x200
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ff28b8,
      PIXMAN_a4r4g4b4,	0x33ff,
      PIXMAN_r5g6b5,	0x3000
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1006278,
      PIXMAN_a8r8g8b8,	0x8a7f18,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffcb00,
      PIXMAN_a4r4g4b4,	0x7900,
      PIXMAN_a2r2g2b2,	0x14
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x115ff00,
      PIXMAN_a8r8g8b8,	0x508d,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x3ff30b5,
      PIXMAN_r5g6b5,	0x2e60,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x182fffb,
      PIXMAN_r3g3b2,	0x1,
      PIXMAN_a8r8g8b8,	0x1000054
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x16fff00,
      PIXMAN_r5g6b5,	0x7bc0,
      PIXMAN_a8r8g8b8,	0x367900
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1d95dd8,
      PIXMAN_a4r4g4b4,	0xfff5,
      PIXMAN_r5g6b5,	0xff09
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ff3cdc,
      PIXMAN_a8r8g8b8,	0x3bda45ff,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x13900f8,
      PIXMAN_a8r8g8b8,	0x7e00ffff,
      PIXMAN_a4r4g4b4,	0xff00
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x10ea9ff,
      PIXMAN_a8r8g8b8,	0xff34ff22,
      PIXMAN_r5g6b5,	0xff52
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2002e99,
      PIXMAN_a4r4g4b4,	0x3000,
      PIXMAN_r5g6b5,	0x43
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_r5g6b5,	0x19ff,
      PIXMAN_r3g3b2,	0x3
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_a8r8g8b8,	0xffff4251,
      PIXMAN_a2r2g2b2,	0x4
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x121c9ff,
      PIXMAN_a4r4g4b4,	0xd2,
      PIXMAN_a4r4g4b4,	0x2
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ff4d,
      PIXMAN_a2r2g2b2,	0x5e,
      PIXMAN_a2r2g2b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x29ab4ff,
      PIXMAN_r3g3b2,	0x47,
      PIXMAN_a8r8g8b8,	0x1900
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffc1ac,
      PIXMAN_a8r8g8b8,	0xee4ed0ac,
      PIXMAN_a8r8g8b8,	0x1009d74
    },
    { PIXMAN_OP_CONJOINT_IN_REVERSE,
      PIXMAN_a8r8g8b8,	0x269dffdc,
      PIXMAN_a8r8g8b8,	0xff0b00e0,
      PIXMAN_a8r8g8b8,	0x2a200ff
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffffff,
      PIXMAN_a4r4g4b4,	0x3200,
      PIXMAN_r3g3b2,	0x24
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x100ffed,
      PIXMAN_a8r8g8b8,	0x67004eff,
      PIXMAN_a2r2g2b2,	0x5
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2fffd6a,
      PIXMAN_a8r8g8b8,	0xc9003bff,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x253ff00,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_r5g6b5,	0xe0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x13600ad,
      PIXMAN_r5g6b5,	0x35ae,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffa8ff,
      PIXMAN_a8r8g8b8,	0xff5f00,
      PIXMAN_r3g3b2,	0xe0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x10067ff,
      PIXMAN_a4r4g4b4,	0x450d,
      PIXMAN_a2r2g2b2,	0x1
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1ff01ff,
      PIXMAN_r3g3b2,	0x77,
      PIXMAN_r5g6b5,	0x6800
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x11da4ff,
      PIXMAN_r5g6b5,	0x83c9,
      PIXMAN_a4r4g4b4,	0x44
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffd4ff,
      PIXMAN_r3g3b2,	0xaa,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ff0000,
      PIXMAN_a8r8g8b8,	0x71002a,
      PIXMAN_a4r4g4b4,	0x700
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1d7ffff,
      PIXMAN_r5g6b5,	0x3696,
      PIXMAN_a4r4g4b4,	0x200
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffffc8,
      PIXMAN_r5g6b5,	0xe900,
      PIXMAN_a8r8g8b8,	0x2000
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff004a,
      PIXMAN_r3g3b2,	0x48,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x3ffe969,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_r5g6b5,	0xc0
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x300ff73,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_a8r8g8b8,	0x3000073
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ff93ff,
      PIXMAN_a8r8g8b8,	0x61fc7d2b,
      PIXMAN_a4r4g4b4,	0x2
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x11bffff,
      PIXMAN_a4r4g4b4,	0xffb4,
      PIXMAN_r5g6b5,	0x8
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1e9e100,
      PIXMAN_a2r2g2b2,	0x56,
      PIXMAN_a2r2g2b2,	0x14
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x3ffb656,
      PIXMAN_r3g3b2,	0x4,
      PIXMAN_a4r4g4b4,	0xff99
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ff00,
      PIXMAN_r3g3b2,	0x68,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1006dff,
      PIXMAN_a2r2g2b2,	0x5d,
      PIXMAN_a8r8g8b8,	0xff00ff55
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x11c00cb,
      PIXMAN_a2r2g2b2,	0x44,
      PIXMAN_a4r4g4b4,	0x4
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1d0ff86,
      PIXMAN_r3g3b2,	0x5c,
      PIXMAN_a8r8g8b8,	0x3c0000
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x2f25fff,
      PIXMAN_r3g3b2,	0x36,
      PIXMAN_a8r8g8b8,	0x2a444aa
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x134af85,
      PIXMAN_r3g3b2,	0x29,
      PIXMAN_r5g6b5,	0xf300
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x13398af,
      PIXMAN_r3g3b2,	0xa5,
      PIXMAN_a4r4g4b4,	0x13
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff57ff,
      PIXMAN_a4r4g4b4,	0x252c,
      PIXMAN_r3g3b2,	0x40
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x115ffff,
      PIXMAN_r5g6b5,	0xffe3,
      PIXMAN_r5g6b5,	0x3303
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffff00,
      PIXMAN_r5g6b5,	0x6300,
      PIXMAN_r3g3b2,	0x6c
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x4ccff9c,
      PIXMAN_r5g6b5,	0xcc,
      PIXMAN_a8r8g8b8,	0x400003d
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffc6dd,
      PIXMAN_r5g6b5,	0x9bff,
      PIXMAN_r5g6b5,	0x5bff
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x14fff95,
      PIXMAN_r3g3b2,	0x46,
      PIXMAN_a8r8g8b8,	0x1000063
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1e6b700,
      PIXMAN_r5g6b5,	0xc1ff,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffff54,
      PIXMAN_a8r8g8b8,	0x2e00ff,
      PIXMAN_r5g6b5,	0x2800
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3ffffff,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_r5g6b5,	0xe0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1003550,
      PIXMAN_r5g6b5,	0xffcc,
      PIXMAN_r5g6b5,	0x1e0
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ffff74,
      PIXMAN_r3g3b2,	0x28,
      PIXMAN_a8r8g8b8,	0xfe2f49d7
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1e35100,
      PIXMAN_r3g3b2,	0x57,
      PIXMAN_r5g6b5,	0x4000
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x268ffa3,
      PIXMAN_a4r4g4b4,	0x30,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x35700f8,
      PIXMAN_r5g6b5,	0xa4,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x3ce1dff,
      PIXMAN_r5g6b5,	0x2a5e,
      PIXMAN_a8r8g8b8,	0x210000
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x494a7ff,
      PIXMAN_a8r8g8b8,	0x1bffe400,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x10026d9,
      PIXMAN_a8r8g8b8,	0xec00621f,
      PIXMAN_r5g6b5,	0x63
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ff99,
      PIXMAN_a8r8g8b8,	0xf334ff,
      PIXMAN_a4r4g4b4,	0x30
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffc200,
      PIXMAN_a8r8g8b8,	0x1e0000ff,
      PIXMAN_a8r8g8b8,	0x1e1700
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff00ff,
      PIXMAN_r3g3b2,	0x4b,
      PIXMAN_r5g6b5,	0x4818
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x2e800ff,
      PIXMAN_a4r4g4b4,	0xd3,
      PIXMAN_a4r4g4b4,	0xec
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x19a001f,
      PIXMAN_r3g3b2,	0x76,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1cb00c3,
      PIXMAN_a4r4g4b4,	0x5cff,
      PIXMAN_r5g6b5,	0x4008
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff0000,
      PIXMAN_r3g3b2,	0x2a,
      PIXMAN_r5g6b5,	0xc5fb
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_a8r8g8b8,	0xea005a88,
      PIXMAN_r3g3b2,	0xb3
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ffea,
      PIXMAN_a4r4g4b4,	0x54eb,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x179ffff,
      PIXMAN_r3g3b2,	0xa4,
      PIXMAN_a8r8g8b8,	0x2400
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x17ad226,
      PIXMAN_r3g3b2,	0xa4,
      PIXMAN_r5g6b5,	0xe0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ff01,
      PIXMAN_a2r2g2b2,	0x25,
      PIXMAN_a4r4g4b4,	0x50
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x20000ff,
      PIXMAN_a8r8g8b8,	0x2b00c127,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x200ff96,
      PIXMAN_a4r4g4b4,	0x2300,
      PIXMAN_r3g3b2,	0x6
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x200ffff,
      PIXMAN_r3g3b2,	0x87,
      PIXMAN_r5g6b5,	0x5bc8
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1fffff2,
      PIXMAN_r3g3b2,	0x7e,
      PIXMAN_a2r2g2b2,	0xe
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1ff8b00,
      PIXMAN_a4r4g4b4,	0xd500,
      PIXMAN_r3g3b2,	0x40
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ffffff,
      PIXMAN_a8r8g8b8,	0x1bff38,
      PIXMAN_a4r4g4b4,	0xf0
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x158ff39,
      PIXMAN_a4r4g4b4,	0x75dd,
      PIXMAN_a8r8g8b8,	0xdd31
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1009b70,
      PIXMAN_a4r4g4b4,	0xff40,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x12fb43f,
      PIXMAN_a4r4g4b4,	0x69ff,
      PIXMAN_a2r2g2b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff95,
      PIXMAN_a2r2g2b2,	0x84,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x200d188,
      PIXMAN_r5g6b5,	0xde6,
      PIXMAN_r5g6b5,	0x3
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2c70000,
      PIXMAN_r5g6b5,	0x24fa,
      PIXMAN_a8r8g8b8,	0x21a0000
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x100ff24,
      PIXMAN_a4r4g4b4,	0x835,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x10000cd,
      PIXMAN_a2r2g2b2,	0x7f,
      PIXMAN_a2r2g2b2,	0x1
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x379ffff,
      PIXMAN_a8r8g8b8,	0x23ffff00,
      PIXMAN_r5g6b5,	0x4eda
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x172e3ff,
      PIXMAN_r3g3b2,	0xa6,
      PIXMAN_r5g6b5,	0x100
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100f5ad,
      PIXMAN_a4r4g4b4,	0x7908,
      PIXMAN_a2r2g2b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100fff9,
      PIXMAN_a2r2g2b2,	0xf1,
      PIXMAN_r3g3b2,	0x1
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1abff00,
      PIXMAN_r5g6b5,	0x31ff,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x112ffd1,
      PIXMAN_r3g3b2,	0x9,
      PIXMAN_a2r2g2b2,	0xdd
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ffbf,
      PIXMAN_r3g3b2,	0x2c,
      PIXMAN_a4r4g4b4,	0x60
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffb7ff,
      PIXMAN_r3g3b2,	0x6b,
      PIXMAN_a4r4g4b4,	0x630
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x20005ff,
      PIXMAN_a4r4g4b4,	0x8462,
      PIXMAN_r5g6b5,	0xb1e8
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff5b00,
      PIXMAN_r5g6b5,	0x70ff,
      PIXMAN_r3g3b2,	0x60
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffffc3,
      PIXMAN_r3g3b2,	0x39,
      PIXMAN_a8r8g8b8,	0x200db41
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x306ffff,
      PIXMAN_a8r8g8b8,	0xdcffff1f,
      PIXMAN_a8r8g8b8,	0x306ff00
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x193daff,
      PIXMAN_a8r8g8b8,	0x69000000,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x2a200ff,
      PIXMAN_a8r8g8b8,	0x183aff00,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100f1a5,
      PIXMAN_a8r8g8b8,	0xb5fc21ff,
      PIXMAN_r5g6b5,	0xfe00
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1630019,
      PIXMAN_a8r8g8b8,	0x6affc400,
      PIXMAN_r5g6b5,	0x56ff
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff8bc2,
      PIXMAN_r3g3b2,	0xee,
      PIXMAN_r5g6b5,	0x1c0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x260ffff,
      PIXMAN_a4r4g4b4,	0x3f00,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x169ffed,
      PIXMAN_a8r8g8b8,	0xffffff3f,
      PIXMAN_a8r8g8b8,	0x169ff00
    },
    { PIXMAN_OP_CONJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x154c181,
      PIXMAN_a4r4g4b4,	0x5100,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1e09c00,
      PIXMAN_r5g6b5,	0xca00,
      PIXMAN_a4r4g4b4,	0xb00
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ff8dff,
      PIXMAN_a8r8g8b8,	0x610038ff,
      PIXMAN_a8r8g8b8,	0x1001f02
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1e400ff,
      PIXMAN_a4r4g4b4,	0x66bd,
      PIXMAN_r3g3b2,	0x68
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x25362ff,
      PIXMAN_a4r4g4b4,	0x31ff,
      PIXMAN_a8r8g8b8,	0x111433
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3ad0039,
      PIXMAN_r3g3b2,	0x26,
      PIXMAN_a8r8g8b8,	0x3000026
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2e442ef,
      PIXMAN_r3g3b2,	0x32,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1720000,
      PIXMAN_a8r8g8b8,	0x55fdea00,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x14bb0d7,
      PIXMAN_a8r8g8b8,	0x7fffff47,
      PIXMAN_a2r2g2b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x13dffff,
      PIXMAN_a8r8g8b8,	0xa3860672,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x120495a,
      PIXMAN_a4r4g4b4,	0x407e,
      PIXMAN_a8r8g8b8,	0x54
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff8fff,
      PIXMAN_a2r2g2b2,	0x29,
      PIXMAN_r5g6b5,	0xa
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100a31a,
      PIXMAN_a4r4g4b4,	0xde4c,
      PIXMAN_a4r4g4b4,	0x1
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1d4008c,
      PIXMAN_r3g3b2,	0x79,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ff0000,
      PIXMAN_a4r4g4b4,	0x7de4,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1b27e62,
      PIXMAN_a4r4g4b4,	0x7941,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x300ff00,
      PIXMAN_a8r8g8b8,	0xfcff255e,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x2ff00b8,
      PIXMAN_a8r8g8b8,	0x19ff718d,
      PIXMAN_r5g6b5,	0x1802
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x235ff13,
      PIXMAN_a8r8g8b8,	0x34bcd9ff,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1006400,
      PIXMAN_a4r4g4b4,	0x7000,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff8bff,
      PIXMAN_a4r4g4b4,	0xfff4,
      PIXMAN_a4r4g4b4,	0xf80
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x24630ff,
      PIXMAN_a8r8g8b8,	0x1f00000b,
      PIXMAN_a8r8g8b8,	0x9061f
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff8a00,
      PIXMAN_a8r8g8b8,	0x79ffab00,
      PIXMAN_r5g6b5,	0x7a00
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x19807ff,
      PIXMAN_a4r4g4b4,	0x6794,
      PIXMAN_a8r8g8b8,	0xff002e00
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x10000da,
      PIXMAN_a4r4g4b4,	0xf864,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffffde,
      PIXMAN_a2r2g2b2,	0x94,
      PIXMAN_a8r8g8b8,	0x1000000
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x200c800,
      PIXMAN_r5g6b5,	0xe9d4,
      PIXMAN_a8r8g8b8,	0x2c00
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff00c9,
      PIXMAN_r3g3b2,	0x4c,
      PIXMAN_r5g6b5,	0x4800
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x122d5ff,
      PIXMAN_r5g6b5,	0x418b,
      PIXMAN_a4r4g4b4,	0x25
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ffff55,
      PIXMAN_a2r2g2b2,	0x1c,
      PIXMAN_a8r8g8b8,	0xff00
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x135ffff,
      PIXMAN_r5g6b5,	0x39c4,
      PIXMAN_r5g6b5,	0xb7
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x100d2c3,
      PIXMAN_r3g3b2,	0x2a,
      PIXMAN_a8r8g8b8,	0x3c00
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x17268ff,
      PIXMAN_a8r8g8b8,	0x7c00ffff,
      PIXMAN_r5g6b5,	0x318f
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff00ff,
      PIXMAN_r3g3b2,	0x68,
      PIXMAN_r3g3b2,	0xb4
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x200ffff,
      PIXMAN_r5g6b5,	0xff86,
      PIXMAN_a8r8g8b8,	0x200f300
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x18a23ff,
      PIXMAN_a2r2g2b2,	0x44,
      PIXMAN_a4r4g4b4,	0x205
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x16bff23,
      PIXMAN_a8r8g8b8,	0x31fd00ff,
      PIXMAN_r3g3b2,	0x7
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x137d1ff,
      PIXMAN_a4r4g4b4,	0x56c1,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff5bff,
      PIXMAN_a4r4g4b4,	0xfff4,
      PIXMAN_a4r4g4b4,	0xf50
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x15c6b00,
      PIXMAN_a8r8g8b8,	0x7d008a,
      PIXMAN_a4r4g4b4,	0x200
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x12091ff,
      PIXMAN_a8r8g8b8,	0xb74cff6b,
      PIXMAN_a2r2g2b2,	0x8
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff5bff,
      PIXMAN_a8r8g8b8,	0xff6ddce8,
      PIXMAN_a2r2g2b2,	0x10
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_a4r4g4b4,	0xffb7,
      PIXMAN_a4r4g4b4,	0xb0
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x173ffff,
      PIXMAN_r5g6b5,	0xff2c,
      PIXMAN_a4r4g4b4,	0x6
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x17102ff,
      PIXMAN_a8r8g8b8,	0x955bff66,
      PIXMAN_a8r8g8b8,	0x280066
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x3c7ff24,
      PIXMAN_r5g6b5,	0xc4,
      PIXMAN_r5g6b5,	0x163
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100c2a6,
      PIXMAN_r5g6b5,	0xa9b9,
      PIXMAN_a4r4g4b4,	0x8
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x26049ff,
      PIXMAN_a4r4g4b4,	0xb2,
      PIXMAN_r5g6b5,	0x8904
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2f100ff,
      PIXMAN_r3g3b2,	0x30,
      PIXMAN_a8r8g8b8,	0x2220100
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ffff88,
      PIXMAN_r3g3b2,	0x7e,
      PIXMAN_r3g3b2,	0x60
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x153ffab,
      PIXMAN_a8r8g8b8,	0xfd10725a,
      PIXMAN_r3g3b2,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff00d2,
      PIXMAN_r5g6b5,	0xff6b,
      PIXMAN_a8r8g8b8,	0x101014a
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x100d965,
      PIXMAN_a8r8g8b8,	0xff007b00,
      PIXMAN_r3g3b2,	0xc
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ec0000,
      PIXMAN_r5g6b5,	0x6fff,
      PIXMAN_r5g6b5,	0x6000
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x19d59a2,
      PIXMAN_a8r8g8b8,	0x4a00ff7a,
      PIXMAN_a8r8g8b8,	0x2e1a2f
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1eb0000,
      PIXMAN_a4r4g4b4,	0x72bc,
      PIXMAN_r5g6b5,	0x1800
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_a4r4g4b4,	0xc034,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x195ff15,
      PIXMAN_a4r4g4b4,	0xb7b1,
      PIXMAN_r5g6b5,	0x4000
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffdf94,
      PIXMAN_a4r4g4b4,	0x78,
      PIXMAN_r3g3b2,	0xc
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x26f00ff,
      PIXMAN_a4r4g4b4,	0xff93,
      PIXMAN_r5g6b5,	0x1dd2
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2ff3fc5,
      PIXMAN_r3g3b2,	0x2f,
      PIXMAN_a8r8g8b8,	0x240000
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1ff696e,
      PIXMAN_a4r4g4b4,	0x22ff,
      PIXMAN_r5g6b5,	0x34d
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x10033d9,
      PIXMAN_a8r8g8b8,	0x38650000,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffff00,
      PIXMAN_a4r4g4b4,	0x2070,
      PIXMAN_r5g6b5,	0x2100
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1008746,
      PIXMAN_a8r8g8b8,	0xb56971,
      PIXMAN_r5g6b5,	0xc25c
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x144d200,
      PIXMAN_a4r4g4b4,	0xff42,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1ffffd0,
      PIXMAN_r5g6b5,	0x5b00,
      PIXMAN_r3g3b2,	0x4c
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x10000ff,
      PIXMAN_a8r8g8b8,	0xff006f,
      PIXMAN_r5g6b5,	0xd
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x10666ff,
      PIXMAN_a4r4g4b4,	0x39b2,
      PIXMAN_r5g6b5,	0xa6
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x11a007d,
      PIXMAN_r3g3b2,	0xf9,
      PIXMAN_a8r8g8b8,	0x11a0000
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1eb90ee,
      PIXMAN_r5g6b5,	0xd,
      PIXMAN_a2r2g2b2,	0x1
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ff42d5,
      PIXMAN_a4r4g4b4,	0x3400,
      PIXMAN_r3g3b2,	0x40
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1dfff00,
      PIXMAN_a8r8g8b8,	0x3ffff9d2,
      PIXMAN_r5g6b5,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff6500,
      PIXMAN_a2r2g2b2,	0x56,
      PIXMAN_r3g3b2,	0x44
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x119ffe6,
      PIXMAN_r3g3b2,	0x8d,
      PIXMAN_a4r4g4b4,	0xff00
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x100cd00,
      PIXMAN_r5g6b5,	0x33ff,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x569ffd7,
      PIXMAN_r5g6b5,	0x8cc,
      PIXMAN_r5g6b5,	0xc0
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100876a,
      PIXMAN_a8r8g8b8,	0x575447a5,
      PIXMAN_r5g6b5,	0x164
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x12d00ff,
      PIXMAN_a4r4g4b4,	0x3fff,
      PIXMAN_a4r4g4b4,	0x0
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ff953b,
      PIXMAN_a4r4g4b4,	0x2914,
      PIXMAN_r5g6b5,	0x20a1
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffead4,
      PIXMAN_a8r8g8b8,	0xff00ea4e,
      PIXMAN_r3g3b2,	0x5a
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x1ff6400,
      PIXMAN_a2r2g2b2,	0x99,
      PIXMAN_r5g6b5,	0xa620
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x17b0084,
      PIXMAN_r3g3b2,	0xbd,
      PIXMAN_a4r4g4b4,	0x500
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x4f90bbb,
      PIXMAN_a8r8g8b8,	0xff00d21f,
      PIXMAN_a8r8g8b8,	0xfb00fc4a
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ffbb1d,
      PIXMAN_a8r8g8b8,	0x2dff79ff,
      PIXMAN_r5g6b5,	0x2c0
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ffff,
      PIXMAN_a2r2g2b2,	0x43,
      PIXMAN_a4r4g4b4,	0x6f
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1f000ff,
      PIXMAN_a4r4g4b4,	0xb393,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1c60020,
      PIXMAN_a8r8g8b8,	0x6bffffff,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1727d00,
      PIXMAN_a2r2g2b2,	0x67,
      PIXMAN_a4r4g4b4,	0x400
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x14a5194,
      PIXMAN_a4r4g4b4,	0xd7ff,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x20003fa,
      PIXMAN_a4r4g4b4,	0x24ff,
      PIXMAN_a8r8g8b8,	0xffff1550
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1a6ff83,
      PIXMAN_a4r4g4b4,	0xf400,
      PIXMAN_r5g6b5,	0x2800
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ffcf00,
      PIXMAN_r5g6b5,	0x71ff,
      PIXMAN_a4r4g4b4,	0x30
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x333ffff,
      PIXMAN_a4r4g4b4,	0x2c00,
      PIXMAN_r3g3b2,	0x4
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1c2ffe8,
      PIXMAN_r5g6b5,	0xc200,
      PIXMAN_a8r8g8b8,	0xfeca41ff
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a2r2g2b2,	0x47,
      PIXMAN_a8r8g8b8,	0x2ffff00,
      PIXMAN_a8r8g8b8,	0x3aa0102
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffeb00,
      PIXMAN_a4r4g4b4,	0xb493,
      PIXMAN_a4r4g4b4,	0x400
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2afffff,
      PIXMAN_r5g6b5,	0xcb,
      PIXMAN_r5g6b5,	0xc0
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x183ff00,
      PIXMAN_r3g3b2,	0x87,
      PIXMAN_r5g6b5,	0xae91
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x3ffff00,
      PIXMAN_a4r4g4b4,	0x2ba4,
      PIXMAN_r5g6b5,	0x2100
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x215cbc2,
      PIXMAN_a4r4g4b4,	0xafd3,
      PIXMAN_a8r8g8b8,	0x115b000
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1853f65,
      PIXMAN_a8r8g8b8,	0xc68cdc41,
      PIXMAN_r5g6b5,	0x3
    },
    { PIXMAN_OP_CONJOINT_IN,
      PIXMAN_a8r8g8b8,	0x3ffff8f,
      PIXMAN_a4r4g4b4,	0x8824,
      PIXMAN_a4r4g4b4,	0x20
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x28e08e6,
      PIXMAN_a8r8g8b8,	0x2cffff31,
      PIXMAN_r5g6b5,	0x1805
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x1b500be,
      PIXMAN_r5g6b5,	0xd946,
      PIXMAN_r5g6b5,	0x9800
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x133ffb3,
      PIXMAN_a2r2g2b2,	0x42,
      PIXMAN_a8r8g8b8,	0x11553c
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x21aff81,
      PIXMAN_r3g3b2,	0xc7,
      PIXMAN_r5g6b5,	0x120
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x12e004f,
      PIXMAN_a4r4g4b4,	0xf617,
      PIXMAN_a4r4g4b4,	0x102
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x164861f,
      PIXMAN_r3g3b2,	0x4e,
      PIXMAN_r5g6b5,	0x19c0
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff0eff,
      PIXMAN_a8r8g8b8,	0xff5c00aa,
      PIXMAN_r5g6b5,	0x5800
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x1e4c60f,
      PIXMAN_a8r8g8b8,	0x38ff0e0c,
      PIXMAN_a4r4g4b4,	0xff2a
    },
    { PIXMAN_OP_DISJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff0000,
      PIXMAN_a8r8g8b8,	0x9f3d6700,
      PIXMAN_r5g6b5,	0xf3ff
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x205ffd0,
      PIXMAN_a8r8g8b8,	0xffc22b3b,
      PIXMAN_a8r8g8b8,	0x2040000
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x1ff0059,
      PIXMAN_r5g6b5,	0x74ff,
      PIXMAN_a8r8g8b8,	0x1730101
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x29affb8,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_a8r8g8b8,	0x2d25cff
    },
    { PIXMAN_OP_DISJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x1ffff8b,
      PIXMAN_a4r4g4b4,	0xff7b,
      PIXMAN_r5g6b5,	0x3a0
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2a86ad7,
      PIXMAN_a4r4g4b4,	0xdc22,
      PIXMAN_a8r8g8b8,	0x2860000
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x3ff00ff,
      PIXMAN_r3g3b2,	0x33,
      PIXMAN_r5g6b5,	0x2000
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1e50063,
      PIXMAN_a8r8g8b8,	0x35ff95d7,
      PIXMAN_r3g3b2,	0x20
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x2ffe6ff,
      PIXMAN_a8r8g8b8,	0x153ef297,
      PIXMAN_r5g6b5,	0x6d2
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x34ffeff,
      PIXMAN_a4r4g4b4,	0x2e,
      PIXMAN_r5g6b5,	0x1d
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2ffeada,
      PIXMAN_r5g6b5,	0xabc6,
      PIXMAN_a8r8g8b8,	0xfd15b256
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x100ff00,
      PIXMAN_a8r8g8b8,	0xcff3f32,
      PIXMAN_a8r8g8b8,	0x3f00
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x1e1b0f1,
      PIXMAN_a8r8g8b8,	0xff63ff54,
      PIXMAN_r3g3b2,	0x5d
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0x2ffff23,
      PIXMAN_a8r8g8b8,	0x380094ff,
      PIXMAN_r5g6b5,	0x3a4b
    },
    { PIXMAN_OP_CONJOINT_ATOP,
      PIXMAN_a4r4g4b4,	0x1000,
      PIXMAN_r5g6b5,	0xca,
      PIXMAN_a8r8g8b8,	0x3434500
    },
    { PIXMAN_OP_DISJOINT_IN,
      PIXMAN_a8r8g8b8,	0x195ffe5,
      PIXMAN_a4r4g4b4,	0x3a29,
      PIXMAN_a8r8g8b8,	0x0
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a8r8g8b8,	0x139007a,
      PIXMAN_a4r4g4b4,	0x4979,
      PIXMAN_r5g6b5,	0x84
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa9,
      PIXMAN_a4r4g4b4,	0xfa18,
      PIXMAN_a8r8g8b8,	0xabff67ff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x94,
      PIXMAN_a4r4g4b4,	0x5109,
      PIXMAN_a8r8g8b8,	0x3affffff
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_r5g6b5,	0xd038,
      PIXMAN_r5g6b5,	0xff00,
      PIXMAN_r5g6b5,	0xf9a5
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0x543128ff,
      PIXMAN_a8r8g8b8,	0x7029ff,
      PIXMAN_a8r8g8b8,	0x316b1d7
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_r5g6b5,	0x53ff,
      PIXMAN_r5g6b5,	0x72ff,
      PIXMAN_a8r8g8b8,	0xffffdeff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0x5b00002b,
      PIXMAN_a4r4g4b4,	0xc3,
      PIXMAN_a8r8g8b8,	0x23530be
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0xcefc0041,
      PIXMAN_a8r8g8b8,	0xf60d02,
      PIXMAN_a8r8g8b8,	0x1f2ffe5
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_r5g6b5,	0xffdb,
      PIXMAN_r5g6b5,	0xc700,
      PIXMAN_r5g6b5,	0x654
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_r5g6b5,	0xffc6,
      PIXMAN_r5g6b5,	0xff09,
      PIXMAN_r5g6b5,	0xfe58
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x95,
      PIXMAN_r5g6b5,	0x1b4a,
      PIXMAN_a8r8g8b8,	0xab234cff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x95,
      PIXMAN_a4r4g4b4,	0x5e99,
      PIXMAN_a8r8g8b8,	0x3b1c1cdd
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_r5g6b5,	0x22,
      PIXMAN_r5g6b5,	0xd00,
      PIXMAN_r5g6b5,	0xfbb1
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_r5g6b5,	0xffc8,
      PIXMAN_a8r8g8b8,	0xa1a3ffff,
      PIXMAN_r5g6b5,	0x44a
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0xffff7cff,
      PIXMAN_r5g6b5,	0x900,
      PIXMAN_a8r8g8b8,	0xffff94ec
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xa7,
      PIXMAN_r5g6b5,	0xff,
      PIXMAN_a8r8g8b8,	0xaa00cffe
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0x85,
      PIXMAN_r5g6b5,	0xffb3,
      PIXMAN_a8r8g8b8,	0xaaffff4a
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a8r8g8b8,	0x3500a118,
      PIXMAN_a4r4g4b4,	0x9942,
      PIXMAN_a8r8g8b8,	0x01ff405e
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xb5,
      PIXMAN_x4a4,	0xe,
      PIXMAN_a8r8g8b8,	0xffbaff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xe872,
      PIXMAN_x2r10g10b10, 0xa648ff00,
      PIXMAN_a2r10g10b10, 0x14ff00e8,
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x4d2db34,
      PIXMAN_a8,	0x19,
      PIXMAN_r5g6b5,	0x9700,
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x2ff0076,
      PIXMAN_a8r8g8b8,	0x2a0000,
      PIXMAN_r3g3b2,	0x0,
    },
    { PIXMAN_OP_CONJOINT_OVER_REVERSE,
      PIXMAN_a8r8g8b8,	0x14f00ff,
      PIXMAN_r5g6b5,	0xd48,
      PIXMAN_a4r4g4b4,	0x0,
    },
    { PIXMAN_OP_CONJOINT_OUT,
      PIXMAN_a8r8g8b8,	0x3d8bbff,
      PIXMAN_r5g6b5,	0x6900,
      PIXMAN_a8r8g8b8,	0x0,
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x2ff00ff,
      PIXMAN_a4r4g4b4,	0x2300,
      PIXMAN_r3g3b2,	0x0,
    },
    { PIXMAN_OP_SATURATE,
      PIXMAN_a8r8g8b8,	0x4d2db34,
      PIXMAN_a8r8g8b8,	0xff0019ff,
      PIXMAN_r5g6b5,	0x9700,
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0x100ac05,
      PIXMAN_r3g3b2,	0xef,
      PIXMAN_a2r2g2b2,	0xff,
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a2r2g2b2,	0xbf,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x7e
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r5g6b5,	0xffff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x33
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_a8r8g8b8,	0x84c4ffd7,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xffddff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a8r8g8b8,	0xff6e56,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x20ff1ade
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a4r4g4b4,	0xfe0,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xbdff
    },
    { PIXMAN_OP_SCREEN,
      PIXMAN_a8r8g8b8,	0x9671ff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x43
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a2r2g2b2,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x39ff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_r5g6b5,	0xffff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x1968
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a4r4g4b4,	0x4247,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xd8ffff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_r5g6b5,	0xff00,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x79
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r3g3b2,	0xe0,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x39
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a8r8g8b8,	0xfff8,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0xff
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_r5g6b5,	0x75fc,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x11ff,
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_r3g3b2,	0x52,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xc627
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0x9f2b,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x4b00e7f5
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a8r8g8b8,	0x00dfff5c,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x5e0f,
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_a8r8g8b8,	0xff00121b,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x3776
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_r5g6b5,	0x03e0,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x01003c00,
    },
    { PIXMAN_OP_OVER,
      PIXMAN_a8r8g8b8,	0x0f00c300,
      PIXMAN_null,	0x00,
      PIXMAN_x14r6g6b6,	0x003c0,
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a4r4g4b4,	0xd0c0,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x5300ea00,
    },
    { PIXMAN_OP_OVER,
      PIXMAN_a8r8g8b8,	0x20c6bf00,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xb9ff
    },
    { PIXMAN_OP_OVER,
      PIXMAN_a8r8g8b8,	0x204ac7ff,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xc1ff
    },
    { PIXMAN_OP_OVER_REVERSE,
      PIXMAN_r5g6b5,	0xffc3,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x102d00dd
    },
    { PIXMAN_OP_OVER_REVERSE,
      PIXMAN_r5g6b5,	0x1f00,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x1bdf0c89
    },
    { PIXMAN_OP_OVER_REVERSE,
      PIXMAN_r5g6b5,	0xf9d2,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x1076bcf7
    },
    { PIXMAN_OP_OVER_REVERSE,
      PIXMAN_r5g6b5,	0x00c3,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x1bfe9ae5
    },
    { PIXMAN_OP_OVER_REVERSE,
      PIXMAN_r5g6b5,	0x09ff,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x0b00c16c
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a2r2g2b2,	0xbc,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x9efff1ff
    },
    { PIXMAN_OP_DISJOINT_ATOP,
      PIXMAN_a4r4g4b4,	0xae5f,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xf215b675
    },
    { PIXMAN_OP_DISJOINT_ATOP_REVERSE,
      PIXMAN_a8r8g8b8,	0xce007980,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x80ffe4ad
    },
    { PIXMAN_OP_DISJOINT_XOR,
      PIXMAN_a8r8g8b8,	0xb8b07bea,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x939c
    },
    { PIXMAN_OP_CONJOINT_ATOP_REVERSE,
      PIXMAN_r5g6b5,	0x0063,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x10bb1ed7,
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a2r2g2b2,	0xbf,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x7e
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a8r8g8b8,	0xffffff,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xff3fffff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_r3g3b2,	0x38,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x5b
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_a8r8g8b8,	0x2e9effff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x77
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r5g6b5,	0xffff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x33
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a8r8g8b8,	0xd0089ff,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0xb1
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_r3g3b2,	0x8a,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xcd0004
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_a8r8g8b8,	0xffff1e3a,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xcf00
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_a8r8g8b8,	0x84c4ffd7,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xffddff
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_a4r4g4b4,	0xfd75,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x7f
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_r3g3b2,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x63ff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a8r8g8b8,	0xff6e56,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x20ff1ade
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a4r4g4b4,	0xfe0,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xbdff
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_r5g6b5,	0x9799,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x8d
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_a8r8g8b8,	0xe8ff1c33,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0x6200
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_a8r8g8b8,	0x22ffffff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x63
    },
    { PIXMAN_OP_SCREEN,
      PIXMAN_a8r8g8b8,	0x9671ff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x43
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a2r2g2b2,	0x83,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xff
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_r3g3b2,	0x0,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x97
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_r5g6b5,	0xb900,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x6800ff00
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a4r4g4b4,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0x8e
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xff00,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0xbc
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r5g6b5,	0xfffe,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x90
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_r3g3b2,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xc35f
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a2r2g2b2,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x39ff
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a2r2g2b2,	0x1e,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xbaff
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a8r8g8b8,	0xb4ffff26,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xff
    },
    { PIXMAN_OP_COLOR_DODGE,
      PIXMAN_a4r4g4b4,	0xe3ff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x878b
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a8r8g8b8,	0xff700044,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x6
    },
    { PIXMAN_OP_DARKEN,
      PIXMAN_a2r2g2b2,	0xb6,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xcd00
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_a2r2g2b2,	0xfe,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x12
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a8r8g8b8,	0xb1ff006c,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xff7c
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r3g3b2,	0x4e,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x3c
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_r5g6b5,	0xffff,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0x1968
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_r3g3b2,	0xe7,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x8cced6ac
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a4r4g4b4,	0xa500,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x1bff009d
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r5g6b5,	0x45ff,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x32
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a2r2g2b2,	0x18,
      PIXMAN_null,	0x00,
      PIXMAN_r5g6b5,	0xdc00
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a4r4g4b4,	0x4247,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xd8ffff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_r5g6b5,	0xff00,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x79
    },
    { PIXMAN_OP_COLOR_BURN,
      PIXMAN_r3g3b2,	0xf,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x9fff00ff
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a2r2g2b2,	0x93,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xff
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a2r2g2b2,	0xa3,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0xca
    },
    { PIXMAN_OP_DIFFERENCE,
      PIXMAN_r3g3b2,	0xe0,
      PIXMAN_null,	0x00,
      PIXMAN_a2r2g2b2,	0x39
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r3g3b2,	0x16,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x98ffff
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_r3g3b2,	0x96,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0x225f6c
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_a4r4g4b4,	0x12c7,
      PIXMAN_null,	0x00,
      PIXMAN_a4r4g4b4,	0xb100
    },
    { PIXMAN_OP_LIGHTEN,
      PIXMAN_a8r8g8b8,	0xffda91,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0x6a
    },
    { PIXMAN_OP_EXCLUSION,
      PIXMAN_a8r8g8b8,	0xfff8,
      PIXMAN_null,	0x00,
      PIXMAN_r3g3b2,	0xff
    },
    { PIXMAN_OP_SOFT_LIGHT,
      PIXMAN_a2r2g2b2,	0xff,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xf0ff48ca
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xf1ff,
      PIXMAN_r5g6b5,	0x6eff,
      PIXMAN_a8r8g8b8,	0xffffff,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xf1ff,
      PIXMAN_a8,	0xdf,
      PIXMAN_a8r8g8b8,	0xffffff,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xf1ff,
      PIXMAN_null,	0x00,
      PIXMAN_a8r8g8b8,	0xffffff,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xb867,
      PIXMAN_a4r4g4b4,	0x82d9,
      PIXMAN_a8r8g8b8,	0xffc5,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xa9f5,
      PIXMAN_r5g6b5,	0xadff,
      PIXMAN_a8r8g8b8,	0xffff00,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0x4900,
      PIXMAN_r5g6b5,	0x865c,
      PIXMAN_a8r8g8b8,	0xebff,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xd9ff,
      PIXMAN_a8r8g8b8,	0xffffffff,
      PIXMAN_a8r8g8b8,	0x8ff0d,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0x41ff,
      PIXMAN_a4r4g4b4,	0xcff,
      PIXMAN_a8r8g8b8,	0xe1ff00,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0x91ff,
      PIXMAN_a2r2g2b2,	0xf3,
      PIXMAN_a8r8g8b8,	0xe4ffb4,
    },
    { PIXMAN_OP_HARD_LIGHT,
      PIXMAN_r5g6b5,	0xb9ff,
      PIXMAN_a2r2g2b2,	0xff,
      PIXMAN_a8r8g8b8,	0xffff,
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a8r8g8b8,	0x473affff,
      PIXMAN_r5g6b5,	0x2b00,
      PIXMAN_r5g6b5,	0x1ff,
    },
    { PIXMAN_OP_OVERLAY,
      PIXMAN_a8r8g8b8,	0xe4ff,
      PIXMAN_r3g3b2,	0xff,
      PIXMAN_r5g6b5,	0x89ff,
    },
};

static void
fill (pixman_image_t *image, uint32_t pixel)
{
    uint8_t *data = (uint8_t *)pixman_image_get_data (image);
    int bytes_per_pixel = PIXMAN_FORMAT_BPP (pixman_image_get_format (image)) / 8;
    int n_bytes = pixman_image_get_stride (image) * pixman_image_get_height (image);
    int i;

    switch (bytes_per_pixel)
    {
    case 4:
	for (i = 0; i < n_bytes / 4; ++i)
	    ((uint32_t *)data)[i] = pixel;
	break;

    case 2:
	pixel &= 0xffff;
	for (i = 0; i < n_bytes / 2; ++i)
	    ((uint16_t *)data)[i] = pixel;
	break;

    case 1:
	pixel &= 0xff;
	for (i = 0; i < n_bytes; ++i)
	    ((uint8_t *)data)[i] = pixel;
	break;

    default:
	assert (0);
	break;
    }
}

static uint32_t
access (pixman_image_t *image, int x, int y)
{
    int bytes_per_pixel;
    int stride;
    uint32_t result;
    uint8_t *location;

    if (x < 0 || x >= image->bits.width || y < 0 || y >= image->bits.height)
        return 0;

    bytes_per_pixel = PIXMAN_FORMAT_BPP (image->bits.format) / 8;
    stride = image->bits.rowstride * 4;

    location = (uint8_t *)image->bits.bits + y * stride + x * bytes_per_pixel;

    if (bytes_per_pixel == 4)
        result = *(uint32_t *)location;
    else if (bytes_per_pixel == 2)
        result = *(uint16_t *)location;
    else if (bytes_per_pixel == 1)
        result = *(uint8_t *)location;
    else
	assert (0);

    return result;
}

static pixman_bool_t
verify (int test_no, const pixel_combination_t *combination, int size,
	pixman_bool_t component_alpha)
{
    pixman_image_t *src, *mask, *dest;
    pixel_checker_t src_checker, mask_checker, dest_checker;
    color_t source_color, mask_color, dest_color, reference_color;
    pixman_bool_t have_mask = (combination->mask_format != PIXMAN_null);
    pixman_bool_t result = TRUE;
    int i, j;

    /* Compute reference color */
    pixel_checker_init (&src_checker, combination->src_format);
    if (have_mask)
	pixel_checker_init (&mask_checker, combination->mask_format);
    pixel_checker_init (&dest_checker, combination->dest_format);

    pixel_checker_convert_pixel_to_color (
	&src_checker, combination->src_pixel, &source_color);
    if (combination->mask_format != PIXMAN_null)
    {
	pixel_checker_convert_pixel_to_color (
	    &mask_checker, combination->mask_pixel, &mask_color);
    }
    pixel_checker_convert_pixel_to_color (
	&dest_checker, combination->dest_pixel, &dest_color);

    do_composite (combination->op,
		  &source_color,
		  have_mask? &mask_color : NULL,
		  &dest_color,
		  &reference_color, component_alpha);

    src = pixman_image_create_bits (
	combination->src_format, size, size, NULL, -1);
    if (have_mask)
    {
	mask = pixman_image_create_bits (
	    combination->mask_format, size, size, NULL, -1);

	pixman_image_set_component_alpha (mask, component_alpha);
    }
    dest = pixman_image_create_bits (
	combination->dest_format, size, size, NULL, -1);

    fill (src, combination->src_pixel);
    if (have_mask)
	fill (mask, combination->mask_pixel);
    fill (dest, combination->dest_pixel);

    pixman_image_composite32 (
	combination->op, src, 
	have_mask ? mask : NULL,
	dest, 0, 0, 0, 0, 0, 0, size, size);

    for (j = 0; j < size; ++j)
    {
	for (i = 0; i < size; ++i)
	{
	    uint32_t computed = access (dest, i, j);
	    int32_t a, r, g, b;

	    if (!pixel_checker_check (&dest_checker, computed, &reference_color))
	    {
		printf ("----------- Test %d failed ----------\n", test_no);

		printf ("   operator:         %s (%s)\n", operator_name (combination->op),
			have_mask? component_alpha ? "component alpha" : "unified alpha" : "no mask");
		printf ("   src format:       %s\n", format_name (combination->src_format));
		if (have_mask != PIXMAN_null)
		    printf ("   mask format:      %s\n", format_name (combination->mask_format));
		printf ("   dest format:      %s\n", format_name (combination->dest_format));

                printf (" - source ARGB:      %f  %f  %f  %f   (pixel: %8x)\n",
                        source_color.a, source_color.r, source_color.g, source_color.b,
                        combination->src_pixel);
		pixel_checker_split_pixel (&src_checker, combination->src_pixel,
					   &a, &r, &g, &b);
                printf ("                     %8d  %8d  %8d  %8d\n", a, r, g, b);

		if (have_mask)
		{
		    printf (" - mask ARGB:        %f  %f  %f  %f   (pixel: %8x)\n",
			    mask_color.a, mask_color.r, mask_color.g, mask_color.b,
			    combination->mask_pixel);
		    pixel_checker_split_pixel (&mask_checker, combination->mask_pixel,
					       &a, &r, &g, &b);
		    printf ("                     %8d  %8d  %8d  %8d\n", a, r, g, b);
		}

                printf (" - dest ARGB:        %f  %f  %f  %f   (pixel: %8x)\n",
                        dest_color.a, dest_color.r, dest_color.g, dest_color.b,
                        combination->dest_pixel);
		pixel_checker_split_pixel (&dest_checker, combination->dest_pixel,
					   &a, &r, &g, &b);
                printf ("                     %8d  %8d  %8d  %8d\n", a, r, g, b);

                pixel_checker_split_pixel (&dest_checker, computed, &a, &r, &g, &b);
                printf (" - expected ARGB:    %f  %f  %f  %f\n",
                        reference_color.a, reference_color.r, reference_color.g, reference_color.b);

                pixel_checker_get_min (&dest_checker, &reference_color, &a, &r, &g, &b);
                printf ("   min acceptable:   %8d  %8d  %8d  %8d\n", a, r, g, b);

                pixel_checker_split_pixel (&dest_checker, computed, &a, &r, &g, &b);
                printf ("   got:              %8d  %8d  %8d  %8d   (pixel: %8x)\n", a, r, g, b, computed);

                pixel_checker_get_max (&dest_checker, &reference_color, &a, &r, &g, &b);
                printf ("   max acceptable:   %8d  %8d  %8d  %8d\n", a, r, g, b);

		result = FALSE;
		goto done;
	    }
	}
    }

done:
    pixman_image_unref (src);
    pixman_image_unref (dest);

    return result;
}

int
main (int argc, char **argv)
{
    int result = 0;
    int i, j;
    int lo, hi;

    if (argc > 1)
    {
	lo = atoi (argv[1]);
	hi = lo + 1;
    }
    else
    {
	lo = 0;
	hi = ARRAY_LENGTH (regressions);
    }

    for (i = lo; i < hi; ++i)
    {
	const pixel_combination_t *combination = &(regressions[i]);

	for (j = 1; j < 34; ++j)
	{
	    int k, ca;

	    ca = combination->mask_format == PIXMAN_null ? 1 : 2;

	    for (k = 0; k < ca; ++k)
	    {
		if (!verify (i, combination, j, k))
		{
		    result = 1;
		    goto next_regression;
		}
	    }
	}

    next_regression:
	;
    }

    return result;
}
