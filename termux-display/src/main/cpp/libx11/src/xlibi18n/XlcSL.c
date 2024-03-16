/*
Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions: The above copyright notice and this
permission notice shall be included in all copies or substantial
portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
EVEN IF ADVISED IN ADVANCE OF THE POSSIBILITY OF SUCH DAMAGES.


Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


X Window System is a trademark of The Open Group

OSF/1, OSF/Motif and Motif are registered trademarks, and OSF, the OSF
logo, LBX, X Window System, and Xinerama are trademarks of the Open
Group. All other trademarks and registered trademarks mentioned herein
are the property of their respective owners. No right, title or
interest in or to any trademark, service mark, logo or trade name of
Sun Microsystems, Inc. or its licensors is granted.

*/
/*
 * Copyright (c) 2000, Oracle and/or its affiliates.
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xlcint.h"

extern XIM _XDefaultOpenIM(
	XLCd, Display *, XrmDatabase, char *, char *
			   );

Bool
_XInitDefaultIM(XLCd lcd)
{
    if(lcd == (XLCd)NULL)
	return False;

    lcd->methods->open_im = _XDefaultOpenIM;
    lcd->methods->register_callback = NULL;
    lcd->methods->unregister_callback = NULL;
    return True;
}

extern XOM _XDefaultOpenOM(
    XLCd, Display*, XrmDatabase, _Xconst char*, _Xconst char*
			   );

Bool
_XInitDefaultOM(XLCd lcd)
{
    lcd->methods->open_om = _XDefaultOpenOM;
    return True;
}
