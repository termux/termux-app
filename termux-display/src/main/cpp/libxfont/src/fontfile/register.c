/*

Copyright 1994, 1998  The Open Group

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
#else
#define XFONT_BITMAP 1
#endif
#include "libxfontint.h"

#include <X11/fonts/fontmisc.h>
#include <X11/fonts/fntfilst.h>
#include <X11/fonts/bitmap.h>

/*
 * Translate monolithic build symbols to modular build symbols.
 * I chose to make the modular symbols 'canonical' because they
 * are prefixed with XFONT_, neatly avoiding name collisions
 * with other packages.
 */

#ifdef BUILD_FREETYPE
# define XFONT_FREETYPE 1
#endif

void
FontFileRegisterFpeFunctions(void)
{
#ifdef XFONT_BITMAP
    /* bitmap is always builtin to libXfont now */
    BitmapRegisterFontFileFunctions ();
#endif
#ifdef XFONT_FREETYPE
    FreeTypeRegisterFontFileFunctions();
#endif

    FontFileRegisterLocalFpeFunctions ();
#ifdef HAVE_READLINK
    CatalogueRegisterLocalFpeFunctions ();
#endif
}

