/*
 * Copyright © 2012 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Based on the public domain implementation of small noncryptographic PRNG
 * authored by Bob Jenkins: http://burtleburtle.net/bob/rand/smallprng.html
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

#ifndef __UTILS_PRNG_H__
#define __UTILS_PRNG_H__

/*
 * This file provides a fast SIMD-optimized noncryptographic PRNG (pseudorandom
 * number generator), with the output good enough to pass "Big Crush" tests
 * from TestU01 (http://en.wikipedia.org/wiki/TestU01).
 *
 * SIMD code uses http://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html
 * which is a GCC specific extension. There is also a slower alternative
 * code path, which should work with any C compiler.
 *
 * The "prng_t" structure keeps the internal state of the random number
 * generator. It is possible to have multiple instances of the random number
 * generator active at the same time, in this case each of them needs to have
 * its own "prng_t". All the functions take a pointer to "prng_t"
 * as the first argument.
 *
 * Functions:
 *
 * ----------------------------------------------------------------------------
 * void prng_srand_r (prng_t *prng, uint32_t seed);
 *
 * Initialize the pseudorandom number generator. The sequence of preudorandom
 * numbers is deterministic and only depends on "seed". Any two generators
 * initialized with the same seed will produce exactly the same sequence.
 *
 * ----------------------------------------------------------------------------
 * uint32_t prng_rand_r (prng_t *prng);
 *
 * Generate a single uniformly distributed 32-bit pseudorandom value.
 *
 * ----------------------------------------------------------------------------
 * void prng_randmemset_r (prng_t                  *prng,
 *                         void                    *buffer,
 *                         size_t                   size,
 *                         prng_randmemset_flags_t  flags);
 *
 * Fills the memory buffer "buffer" with "size" bytes of pseudorandom data.
 * The "flags" argument may be used to tweak some statistics properties:
 *    RANDMEMSET_MORE_00 - set ~25% of bytes to 0x00
 *    RANDMEMSET_MORE_FF - set ~25% of bytes to 0xFF
 * The flags can be combined. This allows a bit better simulation of typical
 * pixel data, which normally contains a lot of fully transparent or fully
 * opaque pixels.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"

/*****************************************************************************/

#ifdef HAVE_GCC_VECTOR_EXTENSIONS
typedef uint32_t uint32x4 __attribute__ ((vector_size(16)));
typedef uint8_t  uint8x16 __attribute__ ((vector_size(16)));
#endif

typedef struct
{
    uint32_t a, b, c, d;
} smallprng_t;

typedef struct
{
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
    uint32x4 a, b, c, d;
#else
    smallprng_t p1, p2, p3, p4;
#endif
    smallprng_t p0;
} prng_t;

typedef union
{
    uint8_t  b[16];
    uint32_t w[4];
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
    uint8x16 vb;
    uint32x4 vw;
#endif
} prng_rand_128_data_t;

/*****************************************************************************/

static force_inline uint32_t
smallprng_rand_r (smallprng_t *x)
{
    uint32_t e = x->a - ((x->b << 27) + (x->b >> (32 - 27)));
    x->a = x->b ^ ((x->c << 17) ^ (x->c >> (32 - 17)));
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

/* Generate 4 bytes (32-bits) of random data */
static force_inline uint32_t
prng_rand_r (prng_t *x)
{
    return smallprng_rand_r (&x->p0);
}

/* Generate 16 bytes (128-bits) of random data */
static force_inline void
prng_rand_128_r (prng_t *x, prng_rand_128_data_t *data)
{
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
    uint32x4 e = x->a - ((x->b << 27) + (x->b >> (32 - 27)));
    x->a = x->b ^ ((x->c << 17) ^ (x->c >> (32 - 17)));
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    data->vw = x->d;
#else
    data->w[0] = smallprng_rand_r (&x->p1);
    data->w[1] = smallprng_rand_r (&x->p2);
    data->w[2] = smallprng_rand_r (&x->p3);
    data->w[3] = smallprng_rand_r (&x->p4);
#endif
}

typedef enum
{
    RANDMEMSET_MORE_00        = 1, /* ~25% chance for 0x00 bytes */
    RANDMEMSET_MORE_FF        = 2, /* ~25% chance for 0xFF bytes */
    RANDMEMSET_MORE_00000000  = 4, /* ~25% chance for 0x00000000 clusters */
    RANDMEMSET_MORE_FFFFFFFF  = 8, /* ~25% chance for 0xFFFFFFFF clusters */
    RANDMEMSET_MORE_00_AND_FF = (RANDMEMSET_MORE_00 | RANDMEMSET_MORE_00000000 |
                                 RANDMEMSET_MORE_FF | RANDMEMSET_MORE_FFFFFFFF)
} prng_randmemset_flags_t;

/* Set the 32-bit seed for PRNG */
void prng_srand_r (prng_t *prng, uint32_t seed);

/* Fill memory buffer with random data */
void prng_randmemset_r (prng_t                  *prng,
                        void                    *buffer,
                        size_t                   size,
                        prng_randmemset_flags_t  flags);

#endif
