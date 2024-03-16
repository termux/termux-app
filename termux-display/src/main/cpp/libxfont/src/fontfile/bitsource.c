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
#include <X11/fonts/fntfilst.h>

BitmapSourcesRec	FontFileBitmapSources;

Bool
FontFileRegisterBitmapSource (FontPathElementPtr fpe)
{
    FontPathElementPtr	*new;
    int			i;
    int			newsize;

    for (i = 0; i < FontFileBitmapSources.count; i++)
	if (FontFileBitmapSources.fpe[i] == fpe)
	    return TRUE;
    if (FontFileBitmapSources.count == FontFileBitmapSources.size)
    {
	newsize = FontFileBitmapSources.size + 4;
	new = reallocarray (FontFileBitmapSources.fpe, newsize, sizeof *new);
	if (!new)
	    return FALSE;
	FontFileBitmapSources.size = newsize;
	FontFileBitmapSources.fpe = new;
    }
    FontFileBitmapSources.fpe[FontFileBitmapSources.count++] = fpe;
    return TRUE;
}

void
FontFileUnregisterBitmapSource (FontPathElementPtr fpe)
{
    int	    i;

    for (i = 0; i < FontFileBitmapSources.count; i++)
	if (FontFileBitmapSources.fpe[i] == fpe)
	{
	    FontFileBitmapSources.count--;
	    if (FontFileBitmapSources.count == 0)
	    {
		FontFileBitmapSources.size = 0;
		free (FontFileBitmapSources.fpe);
		FontFileBitmapSources.fpe = 0;
	    }
	    else
	    {
	    	for (; i < FontFileBitmapSources.count; i++)
		    FontFileBitmapSources.fpe[i] = FontFileBitmapSources.fpe[i+1];
	    }
	    break;
	}
}

/*
 * Our set_path_hook: unregister all bitmap sources.
 * This is necessary because already open fonts will keep their FPEs
 * allocated, but they may not be on the new font path.
 * The bitmap sources in the new path will be registered by the init_func.
 */
void
FontFileEmptyBitmapSource(void)
{
    if (FontFileBitmapSources.count == 0)
	return;

    FontFileBitmapSources.count = 0;
    FontFileBitmapSources.size = 0;
    free (FontFileBitmapSources.fpe);
    FontFileBitmapSources.fpe = 0;
}

int
FontFileMatchBitmapSource (FontPathElementPtr fpe,
			   FontPtr *pFont,
			   int flags,
			   FontEntryPtr entry,
			   FontNamePtr zeroPat,
			   FontScalablePtr vals,
			   fsBitmapFormat format,
			   fsBitmapFormatMask fmask,
			   Bool noSpecificSize)
{
    int			source;
    FontEntryPtr	zero;
    FontBitmapEntryPtr	bitmap;
    int			ret;
    FontDirectoryPtr	dir;
    FontScaledPtr	scaled;

    /*
     * Look through all the registered bitmap sources for
     * the same zero name as ours; entries along that one
     * can be scaled as desired.
     */
    ret = BadFontName;
    for (source = 0; source < FontFileBitmapSources.count; source++)
    {
    	if (FontFileBitmapSources.fpe[source] == fpe)
	    continue;
	dir = (FontDirectoryPtr) FontFileBitmapSources.fpe[source]->private;
	zero = FontFileFindNameInDir (&dir->scalable, zeroPat);
	if (!zero)
	    continue;
    	scaled = FontFileFindScaledInstance (zero, vals, noSpecificSize);
    	if (scaled)
    	{
	    if (scaled->pFont)
	    {
		*pFont = scaled->pFont;
		(*pFont)->fpe = FontFileBitmapSources.fpe[source];
		ret = Successful;
	    }
	    else if (scaled->bitmap)
	    {
		entry = scaled->bitmap;
		bitmap = &entry->u.bitmap;
		if (bitmap->pFont)
		{
		    *pFont = bitmap->pFont;
		    (*pFont)->fpe = FontFileBitmapSources.fpe[source];
		    ret = Successful;
		}
		else
		{
		    ret = FontFileOpenBitmap (
				FontFileBitmapSources.fpe[source],
				pFont, flags, entry, format, fmask);
		    if (ret == Successful && *pFont)
			(*pFont)->fpe = FontFileBitmapSources.fpe[source];
		}
	    }
	    else /* "cannot" happen */
	    {
		ret = BadFontName;
	    }
	    break;
    	}
    }
    return ret;
}
