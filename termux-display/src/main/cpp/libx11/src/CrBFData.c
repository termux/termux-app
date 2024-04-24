/*

Copyright 1987, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlib.h"

/*
 * XCreateBitmapFromData: Routine to make a pixmap of depth 1 from user
 *	                  supplied data.
 *	D is any drawable on the same screen that the pixmap will be used in.
 *	Data is a pointer to the bit data, and
 *	width & height give the size in bits of the pixmap.
 *
 * The following format is assumed for data:
 *
 *    format=XYPixmap
 *    bit_order=LSBFirst
 *    byte_order=LSBFirst
 *    padding=8
 *    bitmap_unit=8
 *    xoffset=0
 *    no extra bytes per line
 */
Pixmap XCreateBitmapFromData(
     Display *display,
     Drawable d,
     _Xconst char *data,
     unsigned int width,
     unsigned int height)
{
    Pixmap pix = XCreatePixmap(display, d, width, height, 1);
    GC gc = XCreateGC(display, pix, (unsigned long) 0, (XGCValues *) 0);
    if (gc == NULL) {
        XFreePixmap(display, pix);
        return (Pixmap) None;
    } else {
        XImage ximage = {
            .height = height,
            .width = width,
            .depth = 1,
            .bits_per_pixel = 1,
            .xoffset = 0,
            .format = XYPixmap,
            .data = (char *) data,
            .byte_order = LSBFirst,
            .bitmap_unit = 8,
            .bitmap_bit_order = LSBFirst,
            .bitmap_pad = 8,
            .bytes_per_line = (width + 7) / 8,
        };
        XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
        XFreeGC(display, gc);
        return(pix);
    }
}
