/*
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

FbBits
fbReplicatePixel(Pixel p, int bpp)
{
    FbBits b = p;

    b &= FbFullMask(bpp);
    while (bpp < FB_UNIT) {
        b |= b << bpp;
        bpp <<= 1;
    }
    return b;
}

#define O 0
#define I FB_ALLONES

const FbMergeRopRec FbMergeRopBits[16] = {
    {O, O, O, O},               /* clear         0x0         0 */
    {I, O, O, O},               /* and           0x1         src AND dst */
    {I, O, I, O},               /* andReverse    0x2         src AND NOT dst */
    {O, O, I, O},               /* copy          0x3         src */
    {I, I, O, O},               /* andInverted   0x4         NOT src AND dst */
    {O, I, O, O},               /* noop          0x5         dst */
    {O, I, I, O},               /* xor           0x6         src XOR dst */
    {I, I, I, O},               /* or            0x7         src OR dst */
    {I, I, I, I},               /* nor           0x8         NOT src AND NOT dst */
    {O, I, I, I},               /* equiv         0x9         NOT src XOR dst */
    {O, I, O, I},               /* invert        0xa         NOT dst */
    {I, I, O, I},               /* orReverse     0xb         src OR NOT dst */
    {O, O, I, I},               /* copyInverted  0xc         NOT src */
    {I, O, I, I},               /* orInverted    0xd         NOT src OR dst */
    {I, O, O, I},               /* nand          0xe         NOT src OR NOT dst */
    {O, O, O, I},               /* set           0xf         1 */
};

