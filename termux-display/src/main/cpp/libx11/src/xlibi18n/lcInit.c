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

/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/*
 *  (c) Copyright 1995 FUJITSU LIMITED
 *  This is source code modified by FUJITSU LIMITED under the Joint
 *  Development Agreement for the CDE/Motif PST.
 *
 *   Modifier: Masayoshi Shimamura      FUJITSU LIMITED
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xlcint.h"

#ifdef USE_DYNAMIC_LC
#undef USE_DEFAULT_LOADER
#undef USE_GENERIC_LOADER
#undef USE_UTF8_LOADER
#else
#define USE_GENERIC_LOADER
#define USE_DEFAULT_LOADER
#define USE_UTF8_LOADER
#endif

/*
 * The _XlcInitLoader function initializes the locale object loader list
 * with vendor specific manner.
 */

void
_XlcInitLoader(void)
{

#ifdef USE_DYNAMIC_LC
    _XlcAddLoader(_XlcDynamicLoad, XlcHead);
#else /* USE_DYNAMIC_LC */

#ifdef USE_GENERIC_LOADER
    _XlcAddLoader(_XlcGenericLoader, XlcHead);
#endif

#ifdef USE_DEFAULT_LOADER
    _XlcAddLoader(_XlcDefaultLoader, XlcHead);
#endif

#ifdef USE_UTF8_LOADER
    _XlcAddLoader(_XlcUtf8Loader, XlcHead);
#endif

#ifdef USE_DYNAMIC_LOADER
    _XlcAddLoader(_XlcDynamicLoader, XlcHead);
#endif

#endif /* USE_DYNAMIC_LC */
}

void
_XlcDeInitLoader(void)
{

#ifdef USE_DYNAMIC_LC
    _XlcRemoveLoader(_XlcDynamicLoad);
#else /* USE_DYNAMIC_LC */

#ifdef USE_GENERIC_LOADER
    _XlcRemoveLoader(_XlcGenericLoader);
#endif

#ifdef USE_DEFAULT_LOADER
    _XlcRemoveLoader(_XlcDefaultLoader);
#endif

#ifdef USE_UTF8_LOADER
    _XlcRemoveLoader(_XlcUtf8Loader);
#endif

#ifdef USE_DYNAMIC_LOADER
    _XlcRemoveLoader(_XlcDynamicLoader);
#endif

#endif /* USE_DYNAMIC_LC */
}
