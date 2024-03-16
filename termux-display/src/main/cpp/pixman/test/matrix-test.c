/*
 * Copyright © 2012 Siarhei Siamashka <siarhei.siamashka@gmail.com>
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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef HAVE_FLOAT128

#define pixman_fixed_to_float128(x) (((__float128)(x)) / 65536.0Q)

typedef struct { __float128 v[3]; } pixman_vector_f128_t;
typedef struct { __float128 m[3][3]; } pixman_transform_f128_t;

pixman_bool_t
pixman_transform_point_f128 (const pixman_transform_f128_t *t,
                             const pixman_vector_f128_t    *v,
                             pixman_vector_f128_t          *result)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        result->v[i] = t->m[i][0] * v->v[0] +
                       t->m[i][1] * v->v[1] +
                       t->m[i][2] * v->v[2];
    }
    if (result->v[2] != 0)
    {
        result->v[0] /= result->v[2];
        result->v[1] /= result->v[2];
        result->v[2] = 1;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

pixman_bool_t does_it_fit_fixed_48_16 (__float128 x)
{
    if (x >= 65536.0Q * 65536.0Q * 32768.0Q)
        return FALSE;
    if (x <= -65536.0Q * 65536.0Q * 32768.0Q)
        return FALSE;
    return TRUE;
}

#endif

static inline uint32_t
byteswap32 (uint32_t x)
{
    return ((x & ((uint32_t)0xFF << 24)) >> 24) |
           ((x & ((uint32_t)0xFF << 16)) >>  8) |
           ((x & ((uint32_t)0xFF <<  8)) <<  8) |
           ((x & ((uint32_t)0xFF <<  0)) << 24);
}

static inline uint64_t
byteswap64 (uint64_t x)
{
    return ((x & ((uint64_t)0xFF << 56)) >> 56) |
           ((x & ((uint64_t)0xFF << 48)) >> 40) |
           ((x & ((uint64_t)0xFF << 40)) >> 24) |
           ((x & ((uint64_t)0xFF << 32)) >>  8) |
           ((x & ((uint64_t)0xFF << 24)) <<  8) |
           ((x & ((uint64_t)0xFF << 16)) << 24) |
           ((x & ((uint64_t)0xFF <<  8)) << 40) |
           ((x & ((uint64_t)0xFF <<  0)) << 56);
}

static void
byteswap_transform (pixman_transform_t *t)
{
    int i, j;

    if (is_little_endian ())
        return;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            t->matrix[i][j] = byteswap32 (t->matrix[i][j]);
}

static void
byteswap_vector_48_16 (pixman_vector_48_16_t *v)
{
    int i;

    if (is_little_endian ())
        return;

    for (i = 0; i < 3; i++)
        v->v[i] = byteswap64 (v->v[i]);
}

uint32_t
test_matrix (int testnum, int verbose)
{
    uint32_t crc32 = 0;
    int i, j, k;
    pixman_bool_t is_affine;

    prng_srand (testnum);

    for (i = 0; i < 100; i++)
    {
        pixman_bool_t           transform_ok;
        pixman_transform_t      ti;
        pixman_vector_48_16_t   vi, result_i;
#ifdef HAVE_FLOAT128
        pixman_transform_f128_t tf;
        pixman_vector_f128_t    vf, result_f;
#endif
        prng_randmemset (&ti, sizeof(ti), 0);
        prng_randmemset (&vi, sizeof(vi), 0);
        byteswap_transform (&ti);
        byteswap_vector_48_16 (&vi);

        for (j = 0; j < 3; j++)
        {
            /* make sure that "vi" contains 31.16 fixed point data */
            vi.v[j] >>= 17;
            /* and apply random shift */
            if (prng_rand_n (3) == 0)
                vi.v[j] >>= prng_rand_n (46);
        }

        if (prng_rand_n (2))
        {
            /* random shift for the matrix */
            for (j = 0; j < 3; j++)
                for (k = 0; k < 3; k++)
                    ti.matrix[j][k] >>= prng_rand_n (30);
        }

        if (prng_rand_n (2))
        {
            /* affine matrix */
            ti.matrix[2][0] = 0;
            ti.matrix[2][1] = 0;
            ti.matrix[2][2] = pixman_fixed_1;
        }

        if (prng_rand_n (2))
        {
            /* cartesian coordinates */
            vi.v[2] = pixman_fixed_1;
        }

        is_affine = (ti.matrix[2][0] == 0 && ti.matrix[2][1] == 0 &&
                     ti.matrix[2][2] == pixman_fixed_1 &&
                     vi.v[2] == pixman_fixed_1);

        transform_ok = TRUE;
        if (is_affine && prng_rand_n (2))
            pixman_transform_point_31_16_affine (&ti, &vi, &result_i);
        else
            transform_ok = pixman_transform_point_31_16 (&ti, &vi, &result_i);

#ifdef HAVE_FLOAT128
        /* compare with a reference 128-bit floating point implementation */
        for (j = 0; j < 3; j++)
        {
            vf.v[j] = pixman_fixed_to_float128 (vi.v[j]);
            for (k = 0; k < 3; k++)
            {
                tf.m[j][k] = pixman_fixed_to_float128 (ti.matrix[j][k]);
            }
        }

        if (pixman_transform_point_f128 (&tf, &vf, &result_f))
        {
            if (transform_ok ||
                (does_it_fit_fixed_48_16 (result_f.v[0]) &&
                 does_it_fit_fixed_48_16 (result_f.v[1]) &&
                 does_it_fit_fixed_48_16 (result_f.v[2])))
            {
                for (j = 0; j < 3; j++)
                {
                    double diff = fabs (result_f.v[j] -
                                        pixman_fixed_to_float128 (result_i.v[j]));

                    if (is_affine && diff > (0.51 / 65536.0))
                    {
                        printf ("%d:%d: bad precision for affine (%.12f)\n",
                               testnum, i, diff);
                        abort ();
                    }
                    else if (diff > (0.71 / 65536.0))
                    {
                        printf ("%d:%d: bad precision for projective (%.12f)\n",
                               testnum, i, diff);
                        abort ();
                    }
                }
            }
        }
#endif
        byteswap_vector_48_16 (&result_i);
        crc32 = compute_crc32 (crc32, &result_i, sizeof (result_i));
    }
    return crc32;
}

int
main (int argc, const char *argv[])
{
    return fuzzer_test_main ("matrix", 20000,
			     0xBEBF98C3,
			     test_matrix, argc, argv);
}
