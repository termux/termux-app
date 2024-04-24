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

#include <assert.h>
#include <stdlib.h>
#include "utils-prng.h"
#include "utils.h"

/* The original code from http://www.burtleburtle.net/bob/rand/smallprng.html */

typedef uint32_t u4;
typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
u4 ranval( ranctx *x ) {
    u4 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

void raninit( ranctx *x, u4 seed ) {
    u4 i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i) {
        (void)ranval(x);
    }
}

/*****************************************************************************/

#define BUFSIZE (8 * 1024 * 1024)
#define N 50

void bench (void)
{
    double t1, t2;
    int i;
    prng_t prng;
    uint8_t *buf = aligned_malloc (16, BUFSIZE + 1);

    prng_srand_r (&prng, 1234);
    t1 = gettime();
    for (i = 0; i < N; i++)
        prng_randmemset_r (&prng, buf, BUFSIZE, 0);
    t2 = gettime();
    printf ("aligned randmemset                    : %.2f MB/s\n",
            (double)BUFSIZE * N / 1000000. / (t2 - t1));

    t1 = gettime();
    for (i = 0; i < N; i++)
        prng_randmemset_r (&prng, buf + 1, BUFSIZE, 0);
    t2 = gettime();
    printf ("unaligned randmemset                  : %.2f MB/s\n",
            (double)BUFSIZE * N / 1000000. / (t2 - t1));

    t1 = gettime();
    for (i = 0; i < N; i++)
    {
        prng_randmemset_r (&prng, buf, BUFSIZE, RANDMEMSET_MORE_00_AND_FF);
    }
    t2 = gettime ();
    printf ("aligned randmemset (more 00 and FF)   : %.2f MB/s\n",
            (double)BUFSIZE * N / 1000000. / (t2 - t1));

    t1 = gettime();
    for (i = 0; i < N; i++)
    {
        prng_randmemset_r (&prng, buf + 1, BUFSIZE, RANDMEMSET_MORE_00_AND_FF);
    }
    t2 = gettime ();
    printf ("unaligned randmemset (more 00 and FF) : %.2f MB/s\n",
            (double)BUFSIZE * N / 1000000. / (t2 - t1));

    free (buf);
}

#define SMALLBUFSIZE 100

int main (int argc, char *argv[])
{
    const uint32_t ref_crc[RANDMEMSET_MORE_00_AND_FF + 1] =
    {
        0xBA06763D, 0x103FC550, 0x8B59ABA5, 0xD82A0F39,
        0xD2321099, 0xFD8C5420, 0xD3B7C42A, 0xFC098093,
        0x85E01DE0, 0x6680F8F7, 0x4D32DD3C, 0xAE52382B,
        0x149E6CB5, 0x8B336987, 0x15DCB2B3, 0x8A71B781
    };
    uint32_t crc1, crc2;
    uint32_t ref, seed, seed0, seed1, seed2, seed3;
    prng_rand_128_data_t buf;
    uint8_t *bytebuf = aligned_malloc(16, SMALLBUFSIZE + 1);
    ranctx x;
    prng_t prng;
    prng_randmemset_flags_t flags;

    if (argc > 1 && strcmp(argv[1], "-bench") == 0)
    {
        bench ();
        return 0;
    }

    /* basic test */
    raninit (&x, 0);
    prng_srand_r (&prng, 0);
    assert (ranval (&x) == prng_rand_r (&prng));

    /* test for simd code */
    seed = 0;
    prng_srand_r (&prng, seed);
    seed0 = (seed = seed * 1103515245 + 12345);
    seed1 = (seed = seed * 1103515245 + 12345);
    seed2 = (seed = seed * 1103515245 + 12345);
    seed3 = (seed = seed * 1103515245 + 12345);
    prng_rand_128_r (&prng, &buf);

    raninit (&x, seed0);
    ref = ranval (&x);
    assert (ref == buf.w[0]);

    raninit (&x, seed1);
    ref = ranval (&x);
    assert (ref == buf.w[1]);

    raninit (&x, seed2);
    ref = ranval (&x);
    assert (ref == buf.w[2]);

    raninit (&x, seed3);
    ref = ranval (&x);
    assert (ref == buf.w[3]);

    /* test for randmemset */
    for (flags = 0; flags <= RANDMEMSET_MORE_00_AND_FF; flags++)
    {
        prng_srand_r (&prng, 1234);
        prng_randmemset_r (&prng, bytebuf, 16, flags);
        prng_randmemset_r (&prng, bytebuf + 16, SMALLBUFSIZE - 17, flags);
        crc1 = compute_crc32 (0, bytebuf, SMALLBUFSIZE - 1);
        prng_srand_r (&prng, 1234);
        prng_randmemset_r (&prng, bytebuf + 1, SMALLBUFSIZE - 1, flags);
        crc2 = compute_crc32 (0, bytebuf + 1, SMALLBUFSIZE - 1);
        assert (ref_crc[flags] == crc1);
        assert (ref_crc[flags] == crc2);
    }

    free (bytebuf);

    return 0;
}
