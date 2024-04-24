/*

Copyright 1986, 1998  The Open Group

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
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "ImUtil.h"

static int _XDestroyImage(XImage *);
static unsigned long _XGetPixel(XImage *, int, int);
static unsigned long _XGetPixel1(XImage *, int, int);
static unsigned long _XGetPixel8(XImage *, int, int);
static unsigned long _XGetPixel16(XImage *, int, int);
static unsigned long _XGetPixel32(XImage *, int, int);
static int _XPutPixel(XImage *, int, int, unsigned long);
static int _XPutPixel1(XImage *, int, int, unsigned long);
static int _XPutPixel8(XImage *, int, int, unsigned long);
static int _XPutPixel16(XImage *, int, int, unsigned long);
static int _XPutPixel32(XImage *, int, int, unsigned long);
static XImage *_XSubImage(XImage *, int, int, unsigned int, unsigned int);
static int _XAddPixel(XImage *, long);

static unsigned char const _lomask[0x09] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
static unsigned char const _himask[0x09] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00 };

/* These two convenience routines return the scanline_pad and bits_per_pixel
	associated with a specific depth of ZPixmap format image for a
	display. */

int
_XGetScanlinePad(
 Display *dpy,
 int depth)
 {
 	register ScreenFormat *fmt = dpy->pixmap_format;
 	register int i;

 	for (i = dpy->nformats + 1; --i; ++fmt)
 		if (fmt->depth == depth)
 			return(fmt->scanline_pad);

 	return(dpy->bitmap_pad);
 }

int
_XGetBitsPerPixel(
 Display *dpy,
 int depth)
 {
 	register ScreenFormat *fmt = dpy->pixmap_format;
 	register int i;

 	for (i = dpy->nformats + 1; --i; ++fmt)
 		if (fmt->depth == depth)
 			return(fmt->bits_per_pixel);
	if (depth <= 4)
	    return 4;
	if (depth <= 8)
	    return 8;
	if (depth <= 16)
	    return 16;
	return 32;
 }


/*
 * This module provides rudimentary manipulation routines for image data
 * structures.  The functions provided are:
 *
 *	XCreateImage	Creates a default XImage data structure
 *	_XDestroyImage	Deletes an XImage data structure
 *	_XGetPixel	Reads a pixel from an image data structure
 *	_XGetPixel32	Reads a pixel from a 32-bit Z image data structure
 *	_XGetPixel16	Reads a pixel from a 16-bit Z image data structure
 *	_XGetPixel8	Reads a pixel from an 8-bit Z image data structure
 *	_XGetPixel1	Reads a pixel from an 1-bit image data structure
 *	_XPutPixel	Writes a pixel into an image data structure
 *	_XPutPixel32	Writes a pixel into a 32-bit Z image data structure
 *	_XPutPixel16	Writes a pixel into a 16-bit Z image data structure
 *	_XPutPixel8	Writes a pixel into an 8-bit Z image data structure
 *	_XPutPixel1	Writes a pixel into an 1-bit image data structure
 *	_XSubImage	Clones a new (sub)image from an existing one
 *	_XSetImage	Writes an image data pattern into another image
 *	_XAddPixel	Adds a constant value to every pixel in an image
 *
 * The logic contained in these routines makes several assumptions about
 * the image data structures, and at least for current implementations
 * these assumptions are believed to be true.  They are:
 *
 *	For all formats, bits_per_pixel is less than or equal to 32.
 *	For XY formats, bitmap_unit is always less than or equal to bitmap_pad.
 *	For XY formats, bitmap_unit is 8, 16, or 32 bits.
 *	For Z format, bits_per_pixel is 1, 4, 8, 16, 24, or 32 bits.
 */
static void _xynormalizeimagebits (
    register unsigned char *bp,
    register XImage *img)
{
	register unsigned char c;

	if (img->byte_order != img->bitmap_bit_order) {
	    switch (img->bitmap_unit) {

		case 16:
		    c = *bp;
		    *bp = *(bp + 1);
		    *(bp + 1) = c;
		    break;

		case 32:
		    c = *(bp + 3);
		    *(bp + 3) = *bp;
		    *bp = c;
		    c = *(bp + 2);
		    *(bp + 2) = *(bp + 1);
		    *(bp + 1) = c;
		    break;
	    }
	}
	if (img->bitmap_bit_order == MSBFirst)
	    _XReverse_Bytes (bp, img->bitmap_unit >> 3);
}

static void _znormalizeimagebits (
    register unsigned char *bp,
    register XImage *img)
{
	register unsigned char c;
	switch (img->bits_per_pixel) {

	    case 4:
		*bp = ((*bp >> 4) & 0xF) | ((*bp << 4) & ~0xF);
		break;

	    case 16:
		c = *bp;
		*bp = *(bp + 1);
		*(bp + 1) = c;
		break;

	    case 24:
		c = *(bp + 2);
		*(bp + 2) = *bp;
		*bp = c;
		break;

	    case 32:
		c = *(bp + 3);
		*(bp + 3) = *bp;
		*bp = c;
		c = *(bp + 2);
		*(bp + 2) = *(bp + 1);
		*(bp + 1) = c;
		break;
	}
}

static void _putbits(
    register char *src,	/* address of source bit string */
    int dstoffset,	/* bit offset into destination; range is 0-31 */
    register int numbits,/* number of bits to copy to destination */
    register char *dst)	/* address of destination bit string */
{
	register unsigned char chlo, chhi;
	int hibits;
	dst = dst + (dstoffset >> 3);
	dstoffset = dstoffset & 7;
	hibits = 8 - dstoffset;
	chlo = *dst & _lomask[dstoffset];
	for (;;) {
	    chhi = (*src << dstoffset) & _himask[dstoffset];
	    if (numbits <= hibits) {
		chhi = chhi & _lomask[dstoffset + numbits];
		*dst = (*dst & _himask[dstoffset + numbits]) | chlo | chhi;
		break;
	    }
	    *dst = chhi | chlo;
	    dst++;
	    numbits = numbits - hibits;
	    chlo = (unsigned char) (*src & _himask[hibits]) >> hibits;
	    src++;
	    if (numbits <= dstoffset) {
		chlo = chlo & _lomask[numbits];
		*dst = (*dst & _himask[numbits]) | chlo;
		break;
	    }
	    numbits = numbits - dstoffset;
	}
}


/*
 * Macros
 *
 * The ROUNDUP macro rounds up a quantity to the specified boundary,
 * then truncates to bytes.
 *
 * The XYNORMALIZE macro determines whether XY format data requires
 * normalization and calls a routine to do so if needed. The logic in
 * this module is designed for LSBFirst byte and bit order, so
 * normalization is done as required to present the data in this order.
 *
 * The ZNORMALIZE macro performs byte and nibble order normalization if
 * required for Z format data.
 *
 * The XYINDEX macro computes the index to the starting byte (char) boundary
 * for a bitmap_unit containing a pixel with coordinates x and y for image
 * data in XY format.
 *
 * The ZINDEX macro computes the index to the starting byte (char) boundary
 * for a pixel with coordinates x and y for image data in ZPixmap format.
 *
 */

#if defined(Lynx) && defined(ROUNDUP)
#undef ROUNDUP
#endif

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

#define XYNORMALIZE(bp, img) \
    if ((img->byte_order == MSBFirst) || (img->bitmap_bit_order == MSBFirst)) \
	_xynormalizeimagebits((unsigned char *)(bp), img)

#define ZNORMALIZE(bp, img) \
    if (img->byte_order == MSBFirst) \
	_znormalizeimagebits((unsigned char *)(bp), img)

#define XYINDEX(x, y, img) \
    ((y) * img->bytes_per_line) + \
    (((x) + img->xoffset) / img->bitmap_unit) * (img->bitmap_unit >> 3)

#define ZINDEX(x, y, img) ((y) * img->bytes_per_line) + \
    (((x) * img->bits_per_pixel) >> 3)

/*
 * This routine initializes the image object function pointers.  The
 * intent is to provide native (i.e. fast) routines for native format images
 * only using the generic (i.e. slow) routines when fast ones don't exist.
 * However, with the current rather botched external interface, clients may
 * have to mung image attributes after the image gets created, so the fast
 * routines always have to check to make sure the optimization is still
 * valid, and reinit the functions if not.
 */
void _XInitImageFuncPtrs (
    XImage *image)
{
	image->f.create_image = XCreateImage;
	image->f.destroy_image = _XDestroyImage;
	if ((image->format == ZPixmap) && (image->bits_per_pixel == 8)) {
	    image->f.get_pixel = _XGetPixel8;
	    image->f.put_pixel = _XPutPixel8;
	} else if (((image->bits_per_pixel | image->depth) == 1) &&
		   (image->byte_order == image->bitmap_bit_order)) {
	    image->f.get_pixel = _XGetPixel1;
	    image->f.put_pixel = _XPutPixel1;
	} else if ((image->format == ZPixmap) &&
		   (image->bits_per_pixel == 32)) {
	    image->f.get_pixel = _XGetPixel32;
	    image->f.put_pixel = _XPutPixel32;
	} else if ((image->format == ZPixmap) &&
		   (image->bits_per_pixel == 16)) {
	    image->f.get_pixel = _XGetPixel16;
	    image->f.put_pixel = _XPutPixel16;
	} else {
	    image->f.get_pixel = _XGetPixel;
	    image->f.put_pixel = _XPutPixel;
	}
	image->f.sub_image = _XSubImage;
/*	image->f.set_image = _XSetImage;*/
	image->f.add_pixel = _XAddPixel;
}

/*
 * CreateImage
 *
 * Allocates the memory necessary for an XImage data structure.
 * Initializes the structure with "default" values and returns XImage.
 *
 */

XImage *XCreateImage (
    register Display *dpy,
    register Visual *visual,
    unsigned int depth,
    int format,
    int offset, /*How many pixels from the start of the data does the
		picture to be transmitted start?*/

    char *data,
    unsigned int width,
    unsigned int height,
    int xpad,
    int image_bytes_per_line)
		/*How many bytes between a pixel on one line and the pixel with
		  the same X coordinate on the next line? 0 means
		  XCreateImage can calculate it.*/
{
	register XImage *image;
	int bits_per_pixel = 1;
	int min_bytes_per_line;

	if (depth == 0 || depth > 32 ||
	    (format != XYBitmap && format != XYPixmap && format != ZPixmap) ||
	    (format == XYBitmap && depth != 1) ||
	    (xpad != 8 && xpad != 16 && xpad != 32) ||
	    offset < 0)
	    return (XImage *) NULL;
	if ((image = Xcalloc(1, sizeof(XImage))) == NULL)
	    return (XImage *) NULL;

	image->width = width;
	image->height = height;
	image->format = format;
	image->byte_order = dpy->byte_order;
	image->bitmap_unit = dpy->bitmap_unit;
	image->bitmap_bit_order = dpy->bitmap_bit_order;
	if (visual != NULL) {
		image->red_mask = visual->red_mask;
		image->green_mask = visual->green_mask;
		image->blue_mask = visual->blue_mask;
	}
	else {
		image->red_mask = image->green_mask = image->blue_mask = 0;
	}
	if (format == ZPixmap)
	{
	    bits_per_pixel = _XGetBitsPerPixel(dpy, (int) depth);
	}

	image->xoffset = offset;
	image->bitmap_pad = xpad;
	image->depth = depth;
 	image->data = data;
	/*
	 * compute per line accelerator.
	 */
	{
	if (format == ZPixmap)
	    min_bytes_per_line =
	       ROUNDUP((bits_per_pixel * width), image->bitmap_pad);
	else
	    min_bytes_per_line =
	        ROUNDUP((width + offset), image->bitmap_pad);
	}
	if (image_bytes_per_line == 0) {
	    image->bytes_per_line = min_bytes_per_line;
	} else if (image_bytes_per_line < min_bytes_per_line) {
	    Xfree(image);
	    return NULL;
	} else {
	    image->bytes_per_line = image_bytes_per_line;
	}

	image->bits_per_pixel = bits_per_pixel;
	image->obdata = NULL;
	_XInitImageFuncPtrs (image);

	return image;
}

Status XInitImage (XImage *image)
{
	int min_bytes_per_line;

	if (image->depth == 0 || image->depth > 32 ||
	    image->bits_per_pixel > 32 || image->bitmap_unit > 32 ||
	    image->bits_per_pixel < 0 || image->bitmap_unit < 0 ||
	    (image->format != XYBitmap &&
	     image->format != XYPixmap &&
	     image->format != ZPixmap) ||
	    (image->format == XYBitmap && image->depth != 1) ||
	    (image->bitmap_pad != 8 &&
	     image->bitmap_pad != 16 &&
	     image->bitmap_pad != 32) ||
	    image->xoffset < 0)
	    return 0;

	/*
	 * compute per line accelerator.
	 */
	if (image->format == ZPixmap)
	    min_bytes_per_line =
	       ROUNDUP((image->bits_per_pixel * image->width),
		       image->bitmap_pad);
	else
	    min_bytes_per_line =
	        ROUNDUP((image->width + image->xoffset), image->bitmap_pad);

	if (image->bytes_per_line == 0) {
	    image->bytes_per_line = min_bytes_per_line;
	} else if (image->bytes_per_line < min_bytes_per_line) {
	    return 0;
	}

	_XInitImageFuncPtrs (image);

	return 1;
}

/*
 * _DestroyImage
 *
 * Deallocates the memory associated with the ximage data structure.
 * this version handles the case of the image data being malloc'd
 * entirely by the library.
 */

static int _XDestroyImage (XImage *ximage)
{
	Xfree(ximage->data);
	Xfree(ximage->obdata);
	Xfree(ximage);
	return 1;
}


/*
 * GetPixel
 *
 * Returns the specified pixel.  The X and Y coordinates are relative to
 * the origin (upper left [0,0]) of the image.  The pixel value is returned
 * in normalized format, i.e. the LSB of the long is the LSB of the pixel.
 * The algorithm used is:
 *
 *	copy the source bitmap_unit or Zpixel into temp
 *	normalize temp if needed
 *	extract the pixel bits into return value
 *
 */

static unsigned long const low_bits_table[] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

static unsigned long _XGetPixel (
    register XImage *ximage,
    int x,
    int y)

{
	unsigned long pixel, px;
	register char *src;
	register char *dst;
	register int i, j;
	int bits, nbytes;
	long plane;

	if ((ximage->bits_per_pixel | ximage->depth) == 1) {
		src = &ximage->data[XYINDEX(x, y, ximage)];
		dst = (char *)&pixel;
		pixel = 0;
		for (i = ximage->bitmap_unit >> 3; --i >= 0; ) *dst++ = *src++;
		XYNORMALIZE(&pixel, ximage);
          	bits = (x + ximage->xoffset) % ximage->bitmap_unit;
		pixel = ((((char *)&pixel)[bits>>3])>>(bits&7)) & 1;
	} else if (ximage->format == XYPixmap) {
		pixel = 0;
		plane = 0;
		nbytes = ximage->bitmap_unit >> 3;
		for (i = ximage->depth; --i >= 0; ) {
		    src = &ximage->data[XYINDEX(x, y, ximage)+ plane];
		    dst = (char *)&px;
		    px = 0;
		    for (j = nbytes; --j >= 0; ) *dst++ = *src++;
		    XYNORMALIZE(&px, ximage);
		    bits = (x + ximage->xoffset) % ximage->bitmap_unit;
		    pixel = (pixel << 1) |
			    (((((char *)&px)[bits>>3])>>(bits&7)) & 1);
		    plane = plane + (ximage->bytes_per_line * ximage->height);
		}
	} else if (ximage->format == ZPixmap) {
		src = &ximage->data[ZINDEX(x, y, ximage)];
		dst = (char *)&px;
		px = 0;
		for (i = (ximage->bits_per_pixel + 7) >> 3; --i >= 0; )
		    *dst++ = *src++;
		ZNORMALIZE(&px, ximage);
		pixel = 0;
		for (i=sizeof(unsigned long); --i >= 0; )
		    pixel = (pixel << 8) | ((unsigned char *)&px)[i];
		if (ximage->bits_per_pixel == 4) {
		    if (x & 1)
			pixel >>= 4;
		    else
			pixel &= 0xf;
		}
	} else {
		return 0; /* bad image */
	}
	if (ximage->bits_per_pixel == ximage->depth)
	  return pixel;
	else
	  return (pixel & low_bits_table[ximage->depth]);
}

static CARD32 const byteorderpixel = MSBFirst << 24;

static unsigned long _XGetPixel32 (
    register XImage *ximage,
    int x,
    int y)
{
	register unsigned char *addr;
	unsigned long pixel;

	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 32)) {
	    addr = &((unsigned char *)ximage->data)
			[y * ximage->bytes_per_line + (x << 2)];
	    if (*((const char *)&byteorderpixel) == ximage->byte_order)
		pixel = *((CARD32 *)addr);
	    else if (ximage->byte_order == MSBFirst)
		pixel = ((unsigned long)addr[0] << 24 |
			 (unsigned long)addr[1] << 16 |
			 (unsigned long)addr[2] << 8 |
			 addr[3]);
	    else
		pixel = ((unsigned long)addr[3] << 24 |
			 (unsigned long)addr[2] << 16 |
			 (unsigned long)addr[1] << 8 |
			 addr[0]);
	    if (ximage->depth != 32)
		pixel &= low_bits_table[ximage->depth];
	    return pixel;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XGetPixel(ximage, x, y);
	}
}

static unsigned long _XGetPixel16 (
    register XImage *ximage,
    int x,
    int y)
{
	register unsigned char *addr;
	unsigned long pixel;

	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 16)) {
	    addr = &((unsigned char *)ximage->data)
			[y * ximage->bytes_per_line + (x << 1)];
	    if (ximage->byte_order == MSBFirst)
		pixel = addr[0] << 8 | addr[1];
	    else
		pixel = addr[1] << 8 | addr[0];
	    if (ximage->depth != 16)
		pixel &= low_bits_table[ximage->depth];
	    return pixel;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XGetPixel(ximage, x, y);
	}
}

static unsigned long _XGetPixel8 (
    register XImage *ximage,
    int x,
    int y)
{
	unsigned char pixel;

	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 8)) {
	    pixel = ((unsigned char *)ximage->data)
			[y * ximage->bytes_per_line + x];
	    if (ximage->depth != 8)
		pixel &= low_bits_table[ximage->depth];
	    return pixel;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XGetPixel(ximage, x, y);
	}
}

static unsigned long _XGetPixel1 (
    register XImage *ximage,
    int x,
    int y)
{
	unsigned char bit;
	int xoff, yoff;

	if (((ximage->bits_per_pixel | ximage->depth) == 1) &&
	    (ximage->byte_order == ximage->bitmap_bit_order)) {
	    xoff = x + ximage->xoffset;
	    yoff = y * ximage->bytes_per_line + (xoff >> 3);
	    xoff &= 7;
	    if (ximage->bitmap_bit_order == MSBFirst)
	        bit = 0x80 >> xoff;
	    else
		bit = 1 << xoff;
	    return (ximage->data[yoff] & bit) ? 1 : 0;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XGetPixel(ximage, x, y);
	}
}

/*
 * PutPixel
 *
 * Overwrites the specified pixel.  The X and Y coordinates are relative to
 * the origin (upper left [0,0]) of the image.  The input pixel value must be
 * in normalized format, i.e. the LSB of the long is the LSB of the pixel.
 * The algorithm used is:
 *
 *	copy the destination bitmap_unit or Zpixel to temp
 *	normalize temp if needed
 *	copy the pixel bits into the temp
 *	renormalize temp if needed
 *	copy the temp back into the destination image data
 *
 */

static int _XPutPixel (
    register XImage *ximage,
    int x,
    int y,
    unsigned long pixel)

{
	unsigned long px, npixel;
	register char *src;
	register char *dst;
	register int i;
	int j, nbytes;
	long plane;

	if (ximage->depth == 4)
	    pixel &= 0xf;
        npixel = pixel;
	for (i=0, px=pixel; i<sizeof(unsigned long); i++, px>>=8)
	    ((unsigned char *)&pixel)[i] = px;
	if ((ximage->bits_per_pixel | ximage->depth) == 1) {
		src = &ximage->data[XYINDEX(x, y, ximage)];
		dst = (char *)&px;
		px = 0;
		nbytes = ximage->bitmap_unit >> 3;
		for (i = nbytes; --i >= 0; ) *dst++ = *src++;
		XYNORMALIZE(&px, ximage);
		i = ((x + ximage->xoffset) % ximage->bitmap_unit);
		_putbits ((char *)&pixel, i, 1, (char *)&px);
		XYNORMALIZE(&px, ximage);
		src = (char *) &px;
		dst = &ximage->data[XYINDEX(x, y, ximage)];
		for (i = nbytes; --i >= 0; ) *dst++ = *src++;
	} else if (ximage->format == XYPixmap) {
		plane = (ximage->bytes_per_line * ximage->height) *
		    (ximage->depth - 1); /* do least signif plane 1st */
		nbytes = ximage->bitmap_unit >> 3;
		for (j = ximage->depth; --j >= 0; ) {
		    src = &ximage->data[XYINDEX(x, y, ximage) + plane];
		    dst = (char *) &px;
		    px = 0;
		    for (i = nbytes; --i >= 0; ) *dst++ = *src++;
		    XYNORMALIZE(&px, ximage);
		    i = ((x + ximage->xoffset) % ximage->bitmap_unit);
		    _putbits ((char *)&pixel, i, 1, (char *)&px);
		    XYNORMALIZE(&px, ximage);
		    src = (char *)&px;
		    dst = &ximage->data[XYINDEX(x, y, ximage) + plane];
		    for (i = nbytes; --i >= 0; ) *dst++ = *src++;
		    npixel = npixel >> 1;
		    for (i=0, px=npixel; i<sizeof(unsigned long); i++, px>>=8)
			((unsigned char *)&pixel)[i] = px;
		    plane = plane - (ximage->bytes_per_line * ximage->height);
		}
	} else if (ximage->format == ZPixmap) {
		src = &ximage->data[ZINDEX(x, y, ximage)];
		dst = (char *)&px;
		px = 0;
		nbytes = (ximage->bits_per_pixel + 7) >> 3;
		for (i = nbytes; --i >= 0; ) *dst++ = *src++;
		ZNORMALIZE(&px, ximage);
		_putbits ((char *)&pixel,
			  (x * ximage->bits_per_pixel) & 7,
			  ximage->bits_per_pixel, (char *)&px);
		ZNORMALIZE(&px, ximage);
		src = (char *)&px;
		dst = &ximage->data[ZINDEX(x, y, ximage)];
		for (i = nbytes; --i >= 0; ) *dst++ = *src++;
	} else {
		return 0; /* bad image */
	}
	return 1;
}

static int _XPutPixel32 (
    register XImage *ximage,
    int x,
    int y,
    unsigned long pixel)
{
	unsigned char *addr;

	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 32)) {
	    addr = &((unsigned char *)ximage->data)
			[y * ximage->bytes_per_line + (x << 2)];
	    if (*((const char *)&byteorderpixel) == ximage->byte_order)
		*((CARD32 *)addr) = pixel;
	    else if (ximage->byte_order == MSBFirst) {
		addr[0] = pixel >> 24;
		addr[1] = pixel >> 16;
		addr[2] = pixel >> 8;
		addr[3] = pixel;
	    } else {
		addr[3] = pixel >> 24;
		addr[2] = pixel >> 16;
		addr[1] = pixel >> 8;
		addr[0] = pixel;
	    }
	    return 1;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XPutPixel(ximage, x, y, pixel);
	}
}

static int _XPutPixel16 (
    register XImage *ximage,
    int x,
    int y,
    unsigned long pixel)
{
	unsigned char *addr;

	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 16)) {
	    addr = &((unsigned char *)ximage->data)
			[y * ximage->bytes_per_line + (x << 1)];
	    if (ximage->byte_order == MSBFirst) {
		addr[0] = pixel >> 8;
		addr[1] = pixel;
	    } else {
		addr[1] = pixel >> 8;
		addr[0] = pixel;
	    }
	    return 1;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XPutPixel(ximage, x, y, pixel);
	}
}

static int _XPutPixel8 (
    register XImage *ximage,
    int x,
    int y,
    unsigned long pixel)
{
	if ((ximage->format == ZPixmap) && (ximage->bits_per_pixel == 8)) {
	    ximage->data[y * ximage->bytes_per_line + x] = pixel;
	    return 1;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XPutPixel(ximage, x, y, pixel);
	}
}

static int _XPutPixel1 (
    register XImage *ximage,
    int x,
    int y,
    unsigned long pixel)
{
	unsigned char bit;
	int xoff, yoff;

	if (((ximage->bits_per_pixel | ximage->depth) == 1) &&
	    (ximage->byte_order == ximage->bitmap_bit_order)) {
	    xoff = x + ximage->xoffset;
	    yoff = y * ximage->bytes_per_line + (xoff >> 3);
	    xoff &= 7;
	    if (ximage->bitmap_bit_order == MSBFirst)
		bit = 0x80 >> xoff;
	    else
		bit = 1 << xoff;
	    if (pixel & 1)
		ximage->data[yoff] |= bit;
	    else
		ximage->data[yoff] &= ~bit;
	    return 1;
	} else {
	    _XInitImageFuncPtrs(ximage);
	    return XPutPixel(ximage, x, y, pixel);
	}
}

/*
 * SubImage
 *
 * Creates a new image that is a subsection of an existing one.
 * Allocates the memory necessary for the new XImage data structure.
 * Pointer to new image is returned.  The algorithm used is repetitive
 * calls to get and put pixel.
 *
 */

static XImage *_XSubImage (
    XImage *ximage,
    register int x,	/* starting x coordinate in existing image */
    register int y,	/* starting y coordinate in existing image */
    unsigned int width,	/* width in pixels of new subimage */
    unsigned int height)/* height in pixels of new subimage */

{
	register XImage *subimage;
	int dsize;
	register int row, col;
	register unsigned long pixel;
	char *data;

	if ((subimage = Xcalloc (1, sizeof (XImage))) == NULL)
	    return (XImage *) NULL;
	subimage->width = width;
	subimage->height = height;
	subimage->xoffset = 0;
	subimage->format = ximage->format;
	subimage->byte_order = ximage->byte_order;
	subimage->bitmap_unit = ximage->bitmap_unit;
	subimage->bitmap_bit_order = ximage->bitmap_bit_order;
	subimage->bitmap_pad = ximage->bitmap_pad;
	subimage->bits_per_pixel = ximage->bits_per_pixel;
	subimage->depth = ximage->depth;
	/*
	 * compute per line accelerator.
	 */
	if (subimage->format == ZPixmap)
	    subimage->bytes_per_line =
		ROUNDUP(subimage->bits_per_pixel * width,
			subimage->bitmap_pad);
	else
	    subimage->bytes_per_line =
		ROUNDUP(width, subimage->bitmap_pad);
	subimage->obdata = NULL;
	_XInitImageFuncPtrs (subimage);
	dsize = subimage->bytes_per_line * height;
	if (subimage->format == XYPixmap) dsize = dsize * subimage->depth;
	if (((data = Xcalloc (1, dsize)) == NULL) && (dsize > 0)) {
	    Xfree(subimage);
	    return (XImage *) NULL;
	}
	subimage->data = data;

	/*
	 * Test for cases where the new subimage is larger than the region
	 * that we are copying from the existing data.  In those cases,
	 * copy the area of the existing image, and allow the "uncovered"
	 * area of new subimage to remain with zero filled pixels.
	 */
	if (height > ximage->height - y ) height = ximage->height - y;
	if (width > ximage->width - x ) width = ximage->width - x;

	for (row = y; row < (y + height); row++) {
	    for (col = x; col < (x + width); col++) {
		pixel = XGetPixel(ximage, col, row);
		XPutPixel(subimage, (col - x), (row - y), pixel);
	    }
	}
	return subimage;
}


/*
 * SetImage
 *
 * Overwrites a section of one image with all of the data from another.
 * If the two images are not of the same format (i.e. XYPixmap and ZPixmap),
 * the image data is converted to the destination format.  The following
 * restrictions apply:
 *
 *	1. The depths of the source and destination images must be equal.
 *
 *	2. If the height of the source image is too large to fit between
 *	   the specified y starting point and the bottom of the image,
 *	   then scanlines are truncated on the bottom.
 *
 *	3. If the width of the source image is too large to fit between
 *	   the specified x starting point and the end of the scanline,
 *	   then pixels are truncated on the right.
 *
 * The images need not have the same bitmap_bit_order, byte_order,
 * bitmap_unit, bits_per_pixel, bitmap_pad, or xoffset.
 *
 */

int _XSetImage(
    XImage *srcimg,
    XImage *dstimg,
    int x,
    int y)
{
	register unsigned long pixel;
	register int row, col;
	int width, height, startrow, startcol;
	if (x < 0) {
	    startcol = -x;
	    x = 0;
	} else
	    startcol = 0;
	if (y < 0) {
	    startrow = -y;
	    y = 0;
	} else
	    startrow = 0;
	width = dstimg->width - x;
	if (srcimg->width < width)
	    width = srcimg->width;
	height = dstimg->height - y;
	if (srcimg->height < height)
	    height = srcimg->height;

	/* this is slow, will do better later */
	for (row = startrow; row < height; row++) {
	    for (col = startcol; col < width; col++) {
		pixel = XGetPixel(srcimg, col, row);
		XPutPixel(dstimg, x + col, y + row, pixel);
	    }
	}
	return 1;
}

/*
 * AddPixel
 *
 * Adds a constant value to every pixel in a pixmap.
 *
 */

static int
_XAddPixel (
    register XImage *ximage,
    register long value)
{
	register int x;
	register int y;

	if (!value)
	    return 0;
	if ((ximage->bits_per_pixel | ximage->depth) == 1) {
	    /* The only value that we can add here to an XYBitmap
	     * is one.  Since 1 + value = ~value for one bit wide
	     * data, we do this quickly by taking the ones complement
	     * of the entire bitmap data (offset and pad included!).
	     * Note that we don't need to be concerned with bit or
	     * byte order at all.
	     */
	    register unsigned char *dp = (unsigned char *) ximage->data;
	    x = ximage->bytes_per_line * ximage->height;
	    while (--x >= 0) {
		*dp = ~*dp;
		dp++;
	    }
	} else if ((ximage->format == ZPixmap) &&
		   (ximage->bits_per_pixel == 8)) {
	    register unsigned char *dp = (unsigned char *) ximage->data;
	    x = ximage->bytes_per_line * ximage->height;
	    while (--x >= 0)
		*dp++ += value;
	} else if ((ximage->format == ZPixmap) &&
		   (ximage->bits_per_pixel == 16) &&
		   (*((const char *)&byteorderpixel) == ximage->byte_order)) {
	    register unsigned short *dp = (unsigned short *) ximage->data;
	    x = (ximage->bytes_per_line >> 1) * ximage->height;
	    while (--x >= 0)
		*dp++ += value;
	} else if ((ximage->format == ZPixmap) &&
		   (ximage->bits_per_pixel == 32) &&
		   (*((const char *)&byteorderpixel) == ximage->byte_order)) {
	    register CARD32 *dp = (CARD32 *) ximage->data;
	    x = (ximage->bytes_per_line >> 2) * ximage->height;
	    while (--x >= 0)
		*dp++ += value;
	} else {
	    for (y = ximage->height; --y >= 0; ) {
		for (x = ximage->width; --x >= 0; ) {
		    register unsigned long pixel = XGetPixel(ximage, x, y);
		    pixel = pixel + value;
		    XPutPixel(ximage, x, y, pixel);
		}
	    }
	}
	return 0;
}

