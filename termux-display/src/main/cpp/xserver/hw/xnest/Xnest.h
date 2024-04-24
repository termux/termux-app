/*

Copyright (c) 1995  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
** Machines with a 64 bit library interface and a 32 bit server require
** name changes to protect the guilty.
*/
#ifdef _XSERVER64
#define _XSERVER64_tmp
#undef _XSERVER64
typedef unsigned long XID64;
typedef unsigned long Mask64;
typedef unsigned long Atom64;
typedef unsigned long VisualID64;
typedef unsigned long Time64;

#define XID     XID64
#define Mask    Mask64
#define Atom    Atom64
#define VisualID VisualID64
#define Time    Time64
typedef XID Window64;
typedef XID Drawable64;
typedef XID Font64;
typedef XID Pixmap64;
typedef XID Cursor64;
typedef XID Colormap64;
typedef XID GContext64;
typedef XID KeySym64;

#define Window          Window64
#define Drawable        Drawable64
#define Font            Font64
#define Pixmap          Pixmap64
#define Cursor          Cursor64
#define Colormap        Colormap64
#define GContext        GContext64
#define KeySym          KeySym64
#endif  /*_XSERVER64*/

#define GC XlibGC
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#undef GC

#ifdef _XSERVER64_tmp
#define _XSERVER64
#undef _XSERVER64_tmp
#undef XID
#undef Mask
#undef Atom
#undef VisualID
#undef Time
#undef Window
#undef Drawable
#undef Font
#undef Pixmap
#undef Cursor
#undef Colormap
#undef GContext
#undef KeySym
#endif /*_XSERVER64_tmp*/
