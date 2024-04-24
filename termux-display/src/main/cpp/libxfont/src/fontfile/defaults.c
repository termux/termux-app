/*

Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include    <X11/X.h>
#include    <X11/Xproto.h>

#ifndef DEFAULT_BIT_ORDER
#ifdef BITMAP_BIT_ORDER
#define DEFAULT_BIT_ORDER BITMAP_BIT_ORDER
#else
#define DEFAULT_BIT_ORDER MSBFirst
#endif
#endif

#ifndef DEFAULT_BYTE_ORDER
#ifdef IMAGE_BYTE_ORDER
#define DEFAULT_BYTE_ORDER IMAGE_BYTE_ORDER
#else
#define DEFAULT_BYTE_ORDER MSBFirst
#endif
#endif

#ifndef DEFAULT_GLYPH_PAD
#ifdef GLYPHPADBYTES
#define DEFAULT_GLYPH_PAD GLYPHPADBYTES
#else
#define DEFAULT_GLYPH_PAD 4
#endif
#endif

#ifndef DEFAULT_SCAN_UNIT
#define DEFAULT_SCAN_UNIT 1
#endif

#include <X11/fonts/fntfilst.h>

void
FontDefaultFormat (int *bit, int *byte, int *glyph, int *scan)
{
    *bit = DEFAULT_BIT_ORDER;
    *byte = DEFAULT_BYTE_ORDER;
    *glyph = DEFAULT_GLYPH_PAD;
    *scan = DEFAULT_SCAN_UNIT;
}
