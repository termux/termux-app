
/*
 * Copyright (c) 2000-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
   This header file contains listings of STANDARD guids for video formats.
   Please do not place non-registered, or incomplete entries in this file.
   A list of some popular fourcc's are at: http://www.webartz.com/fourcc/
   For an explanation of fourcc <-> guid mappings see RFC2361.
*/

#ifndef _XF86_FOURCC_H_
#define _XF86_FOURCC_H_ 1

#define FOURCC_YUY2 0x32595559
#define XVIMAGE_YUY2 \
   { \
	FOURCC_YUY2, \
        XvYUV, \
	LSBFirst, \
	{'Y','U','Y','2', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	16, \
	XvPacked, \
	1, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 1, 1, \
	{'Y','U','Y','V', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

#define FOURCC_YV12 0x32315659
#define XVIMAGE_YV12 \
   { \
	FOURCC_YV12, \
        XvYUV, \
	LSBFirst, \
	{'Y','V','1','2', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	12, \
	XvPlanar, \
	3, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 2, 2, \
	{'Y','V','U', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

#define FOURCC_I420 0x30323449
#define XVIMAGE_I420 \
   { \
	FOURCC_I420, \
        XvYUV, \
	LSBFirst, \
	{'I','4','2','0', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	12, \
	XvPlanar, \
	3, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 2, 2, \
	{'Y','U','V', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

#define FOURCC_UYVY 0x59565955
#define XVIMAGE_UYVY \
   { \
	FOURCC_UYVY, \
        XvYUV, \
	LSBFirst, \
	{'U','Y','V','Y', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	16, \
	XvPacked, \
	1, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 1, 1, \
	{'U','Y','V','Y', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

#define FOURCC_IA44 0x34344149
#define XVIMAGE_IA44 \
   { \
        FOURCC_IA44, \
        XvYUV, \
        LSBFirst, \
        {'I','A','4','4', \
          0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
        8, \
        XvPacked, \
        1, \
        0, 0, 0, 0, \
        8, 8, 8, \
        1, 1, 1, \
        1, 1, 1, \
        {'A','I', \
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
        XvTopToBottom \
   }

#define FOURCC_AI44 0x34344941
#define XVIMAGE_AI44 \
   { \
        FOURCC_AI44, \
        XvYUV, \
        LSBFirst, \
        {'A','I','4','4', \
          0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
        8, \
        XvPacked, \
        1, \
        0, 0, 0, 0, \
        8, 8, 8, \
        1, 1, 1, \
        1, 1, 1, \
        {'I','A', \
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
        XvTopToBottom \
   }

#define FOURCC_NV12 0x3231564e
#define XVIMAGE_NV12 \
   { \
        FOURCC_NV12, \
        XvYUV, \
        LSBFirst, \
        {'N','V','1','2', \
          0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
        12, \
        XvPlanar, \
        2, \
        0, 0, 0, 0, \
        8, 8, 8, \
        1, 2, 2, \
        1, 2, 2, \
        {'Y','U','V', \
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
        XvTopToBottom \
   }

#endif                          /* _XF86_FOURCC_H_ */
