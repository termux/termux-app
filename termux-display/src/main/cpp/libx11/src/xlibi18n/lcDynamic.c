/*
Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 *
 * Modifier: Takanori Tateno   FUJITSU LIMITED
 *
 */

/*
 * A dynamically loaded locale.
 * Supports: All locale names.
 * How: Loads $(XLOCALEDIR)/xi18n.so and forwards the request to that library.
 * Platforms: Only those defining USE_DYNAMIC_LOADER (none known).
 */

#ifdef USE_DYNAMIC_LOADER
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "Xlibint.h"
#include "Xlcint.h"

#ifndef XLOCALEDIR
#define XLOCALEDIR "/usr/lib/X11/locale"
#endif

#define LCLIBNAME    "xi18n.so"

XLCd
_XlcDynamicLoader(
    const char *name)
{
    char libpath[1024];
    XLCdMethods _XlcGenericMethods;
    XLCd lcd;
    void *nlshandler;

    snprintf(libpath, sizeof(libpath), "%s/%s/%s",
	     XLOCALEDIR, name, LCLIBNAME);
    nlshandler = dlopen(libpath,LAZY);
    _XlcGenericMethods = (XLCdMethods)dlsym(nlshandler,"genericMethods");
    lcd = _XlcCreateLC(name,_XlcGenericMethods);

    return lcd;
}
#else
typedef int dummy;
#endif /* USE_DYNAMIC_LOADER */
