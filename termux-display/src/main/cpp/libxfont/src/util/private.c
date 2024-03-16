/*

Copyright 1991, 1998  The Open Group

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
#include "src/util/replace.h"
#include    <X11/fonts/fontmisc.h>
#include    <X11/fonts/fontstruct.h>

static int _FontPrivateAllocateIndex = 0;

int
xfont2_allocate_font_private_index (void)
{
    return _FontPrivateAllocateIndex++;
}

FontPtr
CreateFontRec (void)
{
    FontPtr pFont;
    int size;

    size = sizeof(FontRec) + (sizeof(pointer) * _FontPrivateAllocateIndex);

    pFont = malloc(size);

    if(pFont) {
	bzero((char*)pFont, size);
	pFont->maxPrivate = _FontPrivateAllocateIndex - 1;
	if(_FontPrivateAllocateIndex)
	    pFont->devPrivates = (pointer)(&pFont[1]);
    }

    return pFont;
}

void
DestroyFontRec (FontPtr pFont)
{
   if (pFont->devPrivates && pFont->devPrivates != (pointer)(&pFont[1]))
	free(pFont->devPrivates);
   free(pFont);
}

void
ResetFontPrivateIndex (void)
{
    _FontPrivateAllocateIndex = 0;
}

Bool
xfont2_font_set_private(FontPtr pFont, int n, pointer ptr)
{
    pointer *new;

    if (n > pFont->maxPrivate) {
	if (pFont->devPrivates && pFont->devPrivates != (pointer)(&pFont[1])) {
	    new = reallocarray (pFont->devPrivates, (n + 1), sizeof (pointer));
	    if (!new)
		return FALSE;
	} else {
	    /* omg realloc */
	    new = reallocarray (NULL, (n + 1), sizeof (pointer));
	    if (!new)
		return FALSE;
	    if (pFont->devPrivates)
		memcpy (new, pFont->devPrivates, (pFont->maxPrivate + 1) * sizeof (pointer));
	}
	pFont->devPrivates = new;
	/* zero out new, uninitialized privates */
	while(++pFont->maxPrivate < n)
	    pFont->devPrivates[pFont->maxPrivate] = (pointer)0;
    }
    pFont->devPrivates[n] = ptr;
    return TRUE;
}

