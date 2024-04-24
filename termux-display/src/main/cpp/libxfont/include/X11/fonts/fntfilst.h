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

#ifndef _FONTFILEST_H_
#define _FONTFILEST_H_

#include <X11/Xos.h>
#ifndef XP_PSTEXT
#include <X11/fonts/fontmisc.h>
#endif
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/fontxlfd.h>
#include <X11/fonts/fntfil.h>

typedef struct _FontName {
    char	*name;
    short	length;
    short	ndashes;
} FontNameRec;

typedef struct _FontScaled {
    FontScalableRec	vals;
    FontEntryPtr	bitmap;
    FontPtr		pFont;
} FontScaledRec;

typedef struct _FontScalableExtra {
    FontScalableRec	defaults;
    int			numScaled;
    int			sizeScaled;
    FontScaledPtr	scaled;
    pointer		private;
} FontScalableExtraRec;

typedef struct _FontScalableEntry {
    FontRendererPtr	    renderer;
    char		    *fileName;
    FontScalableExtraPtr   extra;
} FontScalableEntryRec;

/*
 * This "can't" work yet - the returned alias string must be permanent,
 * but this layer would need to generate the appropriate name from the
 * resolved scalable + the XLFD values passed in.  XXX
 */

typedef struct _FontScaleAliasEntry {
    char		*resolved;
} FontScaleAliasEntryRec;

typedef struct _FontBitmapEntry {
    FontRendererPtr	renderer;
    char		*fileName;
    FontPtr		pFont;
} FontBitmapEntryRec;

typedef struct _FontAliasEntry {
    char	*resolved;
} FontAliasEntryRec;

typedef struct _FontEntry {
    FontNameRec	name;
    int		type;
    union _FontEntryParts {
	FontScalableEntryRec	scalable;
	FontBitmapEntryRec	bitmap;
	FontAliasEntryRec	alias;
    } u;
} FontEntryRec;

typedef struct _FontTable {
    int		    used;
    int		    size;
    FontEntryPtr    entries;
    Bool	    sorted;
} FontTableRec;

typedef struct _FontDirectory {
    char	    *directory;
    unsigned long   dir_mtime;
    unsigned long   alias_mtime;
    FontTableRec    scalable;
    FontTableRec    nonScalable;
    char	    *attributes;
} FontDirectoryRec;

/* Capability bits: for definition of capabilities bitmap in the
   FontRendererRec to indicate support of XLFD enhancements */

#define CAP_MATRIX		0x1
#define CAP_CHARSUBSETTING	0x2

typedef struct _FontRenderer {
    const char    *fileSuffix;
    int	    fileSuffixLen;
    int	    (*OpenBitmap)(FontPathElementPtr /* fpe */,
			  FontPtr * /* pFont */,
			  int /* flags */,
			  FontEntryPtr /* entry */,
			  char * /* fileName */,
			  fsBitmapFormat /* format */,
			  fsBitmapFormatMask /* mask */,
			  FontPtr /* non_cachable_font */);
    int	    (*OpenScalable)(FontPathElementPtr /* fpe */,
			    FontPtr * /* pFont */,
			    int /* flags */,
			    FontEntryPtr /* entry */,
			    char * /* fileName */,
			    FontScalablePtr /* vals */,
			    fsBitmapFormat /* format */,
			    fsBitmapFormatMask /* fmask */,
			    FontPtr /* non_cachable_font */);
    int	    (*GetInfoBitmap)(FontPathElementPtr /* fpe */,
			     FontInfoPtr /* pFontInfo */,
			     FontEntryPtr /* entry */,
			     char * /*fileName */);
    int	    (*GetInfoScalable)(FontPathElementPtr /* fpe */,
			       FontInfoPtr /* pFontInfo */,
			       FontEntryPtr /* entry */,
			       FontNamePtr /* fontName */,
			       char * /* fileName */,
			       FontScalablePtr /* vals */);
    int	    number;
    int     capabilities;	/* Bitmap components defined above */
} FontRendererRec;

typedef struct _FontRenders {
    int		    number;
    struct _FontRenderersElement {
        /* In order to preserve backward compatibility, the
           priority field is made invisible to renderers */
        FontRendererPtr renderer;
        int priority;
    } *renderers;
} FontRenderersRec, *FontRenderersPtr;

typedef struct _BitmapInstance {
    FontScalableRec	vals;
    FontBitmapEntryPtr	bitmap;
} BitmapInstanceRec, *BitmapInstancePtr;

typedef struct _BitmapScalablePrivate {
    int			numInstances;
    BitmapInstancePtr	instances;
} BitmapScalablePrivateRec, *BitmapScalablePrivatePtr;

typedef struct _BitmapSources {
    FontPathElementPtr	*fpe;
    int			size;
    int			count;
} BitmapSourcesRec, *BitmapSourcesPtr;

extern BitmapSourcesRec	FontFileBitmapSources;

/* Defines for FontFileFindNamesInScalableDir() behavior */
#define NORMAL_ALIAS_BEHAVIOR		0
#define LIST_ALIASES_AND_TARGET_NAMES   (1<<0)
#define IGNORE_SCALABLE_ALIASES		(1<<1)

#endif /* _FONTFILEST_H_ */
