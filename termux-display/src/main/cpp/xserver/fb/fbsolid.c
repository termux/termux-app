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

#define FbSelectPart(xor,o,t)    xor

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

void
fbSolid(FbBits * dst,
        FbStride dstStride,
        int dstX, int bpp, int width, int height, FbBits and, FbBits xor)
{
    FbBits startmask, endmask;
    int n, nmiddle;
    int startbyte, endbyte;

    dst += dstX >> FB_SHIFT;
    dstX &= FB_MASK;
    FbMaskBitsBytes(dstX, width, and == 0, startmask, startbyte,
                    nmiddle, endmask, endbyte);
    if (startmask)
        dstStride--;
    dstStride -= nmiddle;
    while (height--) {
        if (startmask) {
            FbDoLeftMaskByteRRop(dst, startbyte, startmask, and, xor);
            dst++;
        }
        n = nmiddle;
        if (!and)
            while (n--)
                WRITE(dst++, xor);
        else
            while (n--) {
                WRITE(dst, FbDoRRop(READ(dst), and, xor));
                dst++;
            }
        if (endmask)
            FbDoRightMaskByteRRop(dst, endbyte, endmask, and, xor);
        dst += dstStride;
    }
}
