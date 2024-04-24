/*
 *  Fast C2P (Chunky-to-Planar) Conversion
 *
 *  NOTES:
 *    - This code was inspired by Scout's C2P tutorial
 *    - It assumes to run on a big endian system
 *
 *  Copyright © 2003-2008 Geert Uytterhoeven
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */


    /*
     *  Basic transpose step
     */

static inline void _transp(CARD32 d[], unsigned int i1, unsigned int i2,
                           unsigned int shift, CARD32 mask)
{
    CARD32 t = (d[i1] ^ (d[i2] >> shift)) & mask;

    d[i1] ^= t;
    d[i2] ^= t << shift;
}


static inline void c2p_unsupported(void) {
    BUG_WARN(1);
}

static inline CARD32 get_mask(unsigned int n)
{
    switch (n) {
    case 1:
        return 0x55555555;

    case 2:
        return 0x33333333;

    case 4:
        return 0x0f0f0f0f;

    case 8:
        return 0x00ff00ff;

    case 16:
        return 0x0000ffff;
    }

    c2p_unsupported();
    return 0;
}


    /*
     *  Transpose operations on 8 32-bit words
     */

static inline void transp8(CARD32 d[], unsigned int n, unsigned int m)
{
    CARD32 mask = get_mask(n);

    switch (m) {
    case 1:
        /* First n x 1 block */
        _transp(d, 0, 1, n, mask);
        /* Second n x 1 block */
        _transp(d, 2, 3, n, mask);
        /* Third n x 1 block */
        _transp(d, 4, 5, n, mask);
        /* Fourth n x 1 block */
        _transp(d, 6, 7, n, mask);
        return;

    case 2:
        /* First n x 2 block */
        _transp(d, 0, 2, n, mask);
        _transp(d, 1, 3, n, mask);
        /* Second n x 2 block */
        _transp(d, 4, 6, n, mask);
        _transp(d, 5, 7, n, mask);
        return;

    case 4:
        /* Single n x 4 block */
        _transp(d, 0, 4, n, mask);
        _transp(d, 1, 5, n, mask);
        _transp(d, 2, 6, n, mask);
        _transp(d, 3, 7, n, mask);
        return;
    }

    c2p_unsupported();
}


    /*
     *  Transpose operations on 4 32-bit words
     */

static inline void transp4(CARD32 d[], unsigned int n, unsigned int m)
{
    CARD32 mask = get_mask(n);

    switch (m) {
    case 1:
        /* First n x 1 block */
        _transp(d, 0, 1, n, mask);
        /* Second n x 1 block */
        _transp(d, 2, 3, n, mask);
        return;

    case 2:
        /* Single n x 2 block */
        _transp(d, 0, 2, n, mask);
        _transp(d, 1, 3, n, mask);
        return;
    }

    c2p_unsupported();
}


    /*
     *  Transpose operations on 4 32-bit words (reverse order)
     */

static inline void transp4x(CARD32 d[], unsigned int n, unsigned int m)
{
    CARD32 mask = get_mask(n);

    switch (m) {
    case 2:
        /* Single n x 2 block */
        _transp(d, 2, 0, n, mask);
        _transp(d, 3, 1, n, mask);
        return;
    }

    c2p_unsupported();
}


    /*
     *  Transpose operations on 2 32-bit words
     */

static inline void transp2(CARD32 d[], unsigned int n)
{
    CARD32 mask = get_mask(n);

    /* Single n x 1 block */
    _transp(d, 0, 1, n, mask);
    return;
}


    /*
     *  Transpose operations on 2 32-bit words (reverse order)
     */

static inline void transp2x(CARD32 d[], unsigned int n)
{
    CARD32 mask = get_mask(n);

    /* Single n x 1 block */
    _transp(d, 1, 0, n, mask);
    return;
}
