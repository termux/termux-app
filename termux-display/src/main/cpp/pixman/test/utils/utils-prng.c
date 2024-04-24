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

#include "utils.h"
#include "utils-prng.h"

#if defined(HAVE_GCC_VECTOR_EXTENSIONS) && defined(__SSE2__)
#include <xmmintrin.h>
#endif

void smallprng_srand_r (smallprng_t *x, uint32_t seed)
{
    uint32_t i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i = 0; i < 20; ++i)
        smallprng_rand_r (x);
}

/*
 * Set a 32-bit seed for PRNG
 *
 * LCG is used here for generating independent seeds for different
 * smallprng instances (in the case if smallprng is also used for
 * generating these seeds, "Big Crush" test from TestU01 detects
 * some problems in the glued 'prng_rand_128_r' output data).
 * Actually we might be even better using some cryptographic
 * hash for this purpose, but LCG seems to be also enough for
 * passing "Big Crush".
 */
void prng_srand_r (prng_t *x, uint32_t seed)
{
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
    int i;
    prng_rand_128_data_t dummy;
    smallprng_srand_r (&x->p0, seed);
    x->a[0] = x->a[1] = x->a[2] = x->a[3] = 0xf1ea5eed;
    x->b[0] = x->c[0] = x->d[0] = (seed = seed * 1103515245 + 12345);
    x->b[1] = x->c[1] = x->d[1] = (seed = seed * 1103515245 + 12345);
    x->b[2] = x->c[2] = x->d[2] = (seed = seed * 1103515245 + 12345);
    x->b[3] = x->c[3] = x->d[3] = (seed = seed * 1103515245 + 12345);
    for (i = 0; i < 20; ++i)
        prng_rand_128_r (x, &dummy);
#else
    smallprng_srand_r (&x->p0, seed);
    smallprng_srand_r (&x->p1, (seed = seed * 1103515245 + 12345));
    smallprng_srand_r (&x->p2, (seed = seed * 1103515245 + 12345));
    smallprng_srand_r (&x->p3, (seed = seed * 1103515245 + 12345));
    smallprng_srand_r (&x->p4, (seed = seed * 1103515245 + 12345));
#endif
}

static force_inline void
store_rand_128_data (void *addr, prng_rand_128_data_t *d, int aligned)
{
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
    if (aligned)
    {
        *(uint8x16 *)addr = d->vb;
        return;
    }
    else
    {
#ifdef __SSE2__
        /* workaround for http://gcc.gnu.org/PR55614 */
        _mm_storeu_si128 (addr, _mm_loadu_si128 ((__m128i *)d));
        return;
#endif
    }
#endif
    /* we could try something better for unaligned writes (packed attribute),
     * but GCC is not very reliable: http://gcc.gnu.org/PR55454 */
    memcpy (addr, d, 16);
}

/*
 * Helper function and the actual code for "prng_randmemset_r" function
 */
static force_inline void
randmemset_internal (prng_t                  *prng,
                     uint8_t                 *buf,
                     size_t                   size,
                     prng_randmemset_flags_t  flags,
                     int                      aligned)
{
    prng_t local_prng = *prng;
    prng_rand_128_data_t randdata;
    size_t i;

    while (size >= 16)
    {
        prng_rand_128_data_t t;
        if (flags == 0)
        {
            prng_rand_128_r (&local_prng, &randdata);
        }
        else
        {
            prng_rand_128_r (&local_prng, &t);
            prng_rand_128_r (&local_prng, &randdata);
#ifdef HAVE_GCC_VECTOR_EXTENSIONS
            if (flags & RANDMEMSET_MORE_FF)
            {
                const uint8x16 const_C0 =
                {
                    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
                    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0
                };
                randdata.vb |= (t.vb >= const_C0);
            }
            if (flags & RANDMEMSET_MORE_00)
            {
                const uint8x16 const_40 =
                {
                    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
                };
                randdata.vb &= (t.vb >= const_40);
            }
            if (flags & RANDMEMSET_MORE_FFFFFFFF)
            {
                const uint32x4 const_C0000000 =
                {
                    0xC0000000, 0xC0000000, 0xC0000000, 0xC0000000
                };
                randdata.vw |= ((t.vw << 30) >= const_C0000000);
            }
            if (flags & RANDMEMSET_MORE_00000000)
            {
                const uint32x4 const_40000000 =
                {
                    0x40000000, 0x40000000, 0x40000000, 0x40000000
                };
                randdata.vw &= ((t.vw << 30) >= const_40000000);
            }
#else
            #define PROCESS_ONE_LANE(i)                                       \
                if (flags & RANDMEMSET_MORE_FF)                               \
                {                                                             \
                    uint32_t mask_ff = (t.w[i] & (t.w[i] << 1)) & 0x80808080; \
                    mask_ff |= mask_ff >> 1;                                  \
                    mask_ff |= mask_ff >> 2;                                  \
                    mask_ff |= mask_ff >> 4;                                  \
                    randdata.w[i] |= mask_ff;                                 \
                }                                                             \
                if (flags & RANDMEMSET_MORE_00)                               \
                {                                                             \
                    uint32_t mask_00 = (t.w[i] | (t.w[i] << 1)) & 0x80808080; \
                    mask_00 |= mask_00 >> 1;                                  \
                    mask_00 |= mask_00 >> 2;                                  \
                    mask_00 |= mask_00 >> 4;                                  \
                    randdata.w[i] &= mask_00;                                 \
                }                                                             \
                if (flags & RANDMEMSET_MORE_FFFFFFFF)                         \
                {                                                             \
                    int32_t mask_ff = ((t.w[i] << 30) & (t.w[i] << 31)) &     \
                                       0x80000000;                            \
                    randdata.w[i] |= mask_ff >> 31;                           \
                }                                                             \
                if (flags & RANDMEMSET_MORE_00000000)                         \
                {                                                             \
                    int32_t mask_00 = ((t.w[i] << 30) | (t.w[i] << 31)) &     \
                                       0x80000000;                            \
                    randdata.w[i] &= mask_00 >> 31;                           \
                }

            PROCESS_ONE_LANE (0)
            PROCESS_ONE_LANE (1)
            PROCESS_ONE_LANE (2)
            PROCESS_ONE_LANE (3)
#endif
        }
        if (is_little_endian ())
        {
            store_rand_128_data (buf, &randdata, aligned);
            buf += 16;
        }
        else
        {

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifdef HAVE_GCC_VECTOR_EXTENSIONS
# if __has_builtin(__builtin_shufflevector)
            randdata.vb =
                __builtin_shufflevector (randdata.vb, randdata.vb,
                                          3,  2,  1,  0,  7,  6 , 5,  4,
                                         11, 10,  9,  8, 15, 14, 13, 12);
# else
            static const uint8x16 bswap_shufflemask =
            {
                3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12
            };
            randdata.vb = __builtin_shuffle (randdata.vb, bswap_shufflemask);
# endif

            store_rand_128_data (buf, &randdata, aligned);
            buf += 16;
#else
            uint8_t t1, t2, t3, t4;
            #define STORE_ONE_LANE(i)                                         \
                t1 = randdata.b[i * 4 + 3];                                   \
                t2 = randdata.b[i * 4 + 2];                                   \
                t3 = randdata.b[i * 4 + 1];                                   \
                t4 = randdata.b[i * 4 + 0];                                   \
                *buf++ = t1;                                                  \
                *buf++ = t2;                                                  \
                *buf++ = t3;                                                  \
                *buf++ = t4;

            STORE_ONE_LANE (0)
            STORE_ONE_LANE (1)
            STORE_ONE_LANE (2)
            STORE_ONE_LANE (3)
#endif
        }
        size -= 16;
    }
    i = 0;
    while (i < size)
    {
        uint8_t randbyte = prng_rand_r (&local_prng) & 0xFF;
        if (flags != 0)
        {
            uint8_t t = prng_rand_r (&local_prng) & 0xFF;
            if ((flags & RANDMEMSET_MORE_FF) && (t >= 0xC0))
                randbyte = 0xFF;
            if ((flags & RANDMEMSET_MORE_00) && (t < 0x40))
                randbyte = 0x00;
            if (i % 4 == 0 && i + 4 <= size)
            {
                t = prng_rand_r (&local_prng) & 0xFF;
                if ((flags & RANDMEMSET_MORE_FFFFFFFF) && (t >= 0xC0))
                {
                    memset(&buf[i], 0xFF, 4);
                    i += 4;
                    continue;
                }
                if ((flags & RANDMEMSET_MORE_00000000) && (t < 0x40))
                {
                    memset(&buf[i], 0x00, 4);
                    i += 4;
                    continue;
                }
            }
        }
        buf[i] = randbyte;
        i++;
    }
    *prng = local_prng;
}

/*
 * Fill memory buffer with random data. Flags argument may be used
 * to tweak some statistics properties:
 *    RANDMEMSET_MORE_00        - set ~25% of bytes to 0x00
 *    RANDMEMSET_MORE_FF        - set ~25% of bytes to 0xFF
 *    RANDMEMSET_MORE_00000000  - ~25% chance for 00000000 4-byte clusters
 *    RANDMEMSET_MORE_FFFFFFFF  - ~25% chance for FFFFFFFF 4-byte clusters
 */
void prng_randmemset_r (prng_t                  *prng,
                        void                    *voidbuf,
                        size_t                   size,
                        prng_randmemset_flags_t  flags)
{
    uint8_t *buf = (uint8_t *)voidbuf;
    if ((uintptr_t)buf & 15)
    {
        /* unaligned buffer */
        if (flags == 0)
            randmemset_internal (prng, buf, size, 0, 0);
        else if (flags == RANDMEMSET_MORE_00_AND_FF)
            randmemset_internal (prng, buf, size, RANDMEMSET_MORE_00_AND_FF, 0);
        else
            randmemset_internal (prng, buf, size, flags, 0);
    }
    else
    {
        /* aligned buffer */
        if (flags == 0)
            randmemset_internal (prng, buf, size, 0, 1);
        else if (flags == RANDMEMSET_MORE_00_AND_FF)
            randmemset_internal (prng, buf, size, RANDMEMSET_MORE_00_AND_FF, 1);
        else
            randmemset_internal (prng, buf, size, flags, 1);
    }
}
