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
#include <X11/fonts/fntfilst.h>
#include <X11/keysym.h>
#ifdef WIN32
#include <ctype.h>
#endif
#include "src/util/replace.h"

static unsigned char
ISOLatin1ToLower(unsigned char source)
{
    if (source >= XK_A && source <= XK_Z)
	return source + (XK_a - XK_A);
    if (source >= XK_Agrave && source <= XK_Odiaeresis)
	return source + (XK_agrave - XK_Agrave);
    if (source >= XK_Ooblique && source <= XK_Thorn)
	return source + (XK_oslash - XK_Ooblique);
    return source;
}

_X_HIDDEN void
CopyISOLatin1Lowered(char *dest, const char *source, int length)
{
    int i;
    for (i = 0; i < length; i++, source++, dest++)
	*dest = ISOLatin1ToLower(*source);
    *dest = '\0';
}

/*
 * Map FPE functions to renderer functions
 */

static int FontFileOpenBitmapNCF (FontPathElementPtr fpe, FontPtr *pFont,
				  int flags, FontEntryPtr entry,
				  fsBitmapFormat format,
				  fsBitmapFormatMask fmask,
				  FontPtr non_cachable_font);

int
FontFileNameCheck (const char *name)
{
#ifndef NCD
#if defined(WIN32)
    /* WIN32 uses D:/... as a path name for fonts, so accept this as a valid
     * path if it starts with a letter and a colon.
     */
    if (isalpha(*name) && name[1]==':')
        return TRUE;
#endif
    return *name == '/';
#else
    return ((strcmp(name, "built-ins") == 0) || (*name == '/'));
#endif
}

int
FontFileInitFPE (FontPathElementPtr fpe)
{
    int			status;
    FontDirectoryPtr	dir;

    status = FontFileReadDirectory (fpe->name, &dir);
    if (status == Successful)
    {
	if (dir->nonScalable.used > 0)
	    if (!FontFileRegisterBitmapSource (fpe))
	    {
		FontFileFreeFPE (fpe);
		return AllocError;
	    }
	fpe->private = (pointer) dir;
    }
    return status;
}

/* ARGSUSED */
int
FontFileResetFPE (FontPathElementPtr fpe)
{
    FontDirectoryPtr	dir;

    dir = (FontDirectoryPtr) fpe->private;
    /*
     * The reset must fail for bitmap fonts because they get cleared when
     * the path is set.
     */
    if (FontFileDirectoryChanged (dir))
    {
	/* can't do it, so tell the caller to close and re-open */
	return FPEResetFailed;
    }
    else
    {
	if (dir->nonScalable.used > 0)
	    if (!FontFileRegisterBitmapSource (fpe))
	    {
	        return FPEResetFailed;
	    }
        return Successful;
    }
}

int
FontFileFreeFPE (FontPathElementPtr fpe)
{
    FontFileUnregisterBitmapSource (fpe);
    FontFileFreeDir ((FontDirectoryPtr) fpe->private);
    return Successful;
}

static int
transfer_values_to_alias(char *entryname, int entrynamelength,
			 char *resolvedname,
			 char **aliasName, FontScalablePtr vals)
{
    static char		aliasname[MAXFONTNAMELEN];
    int			nameok = 1, len;
    char		lowerName[MAXFONTNAMELEN];

    *aliasName = resolvedname;
    if ((len = strlen(*aliasName)) <= MAXFONTNAMELEN &&
	(entrynamelength < MAXFONTNAMELEN) &&
	FontFileCountDashes (*aliasName, len) == 14)
    {
	FontScalableRec tmpVals;
	FontScalableRec tmpVals2;

	tmpVals2 = *vals;

	/* If we're aliasing a scalable name, transfer values
	   from the name into the destination alias, multiplying
	   by matrices that appear in the alias. */

	CopyISOLatin1Lowered (lowerName, entryname,
			      entrynamelength);
	lowerName[entrynamelength] = '\0';

	if (FontParseXLFDName(lowerName, &tmpVals,
			      FONT_XLFD_REPLACE_NONE) &&
	    !tmpVals.values_supplied &&
	    FontParseXLFDName(*aliasName, &tmpVals,
			      FONT_XLFD_REPLACE_NONE))
	{
	    double *matrix = 0, tempmatrix[4];

	    /* Use a matrix iff exactly one is defined */
	    if ((tmpVals.values_supplied & PIXELSIZE_MASK) ==
		PIXELSIZE_ARRAY &&
		!(tmpVals.values_supplied & POINTSIZE_MASK))
		matrix = tmpVals.pixel_matrix;
	    else if ((tmpVals.values_supplied & POINTSIZE_MASK) ==
		     POINTSIZE_ARRAY &&
		     !(tmpVals.values_supplied & PIXELSIZE_MASK))
		matrix = tmpVals.point_matrix;

	    /* If matrix given in the alias, compute new point
	       and/or pixel matrices */
	    if (matrix)
	    {
		/* Complete the XLFD name to avoid potential
		   gotchas */
		if (FontFileCompleteXLFD(&tmpVals2, &tmpVals2))
		{
		    tempmatrix[0] =
			matrix[0] * tmpVals2.point_matrix[0] +
			matrix[1] * tmpVals2.point_matrix[2];
		    tempmatrix[1] =
			matrix[0] * tmpVals2.point_matrix[1] +
			matrix[1] * tmpVals2.point_matrix[3];
		    tempmatrix[2] =
			matrix[2] * tmpVals2.point_matrix[0] +
			matrix[3] * tmpVals2.point_matrix[2];
		    tempmatrix[3] =
			matrix[2] * tmpVals2.point_matrix[1] +
			matrix[3] * tmpVals2.point_matrix[3];
		    tmpVals2.point_matrix[0] = tempmatrix[0];
		    tmpVals2.point_matrix[1] = tempmatrix[1];
		    tmpVals2.point_matrix[2] = tempmatrix[2];
		    tmpVals2.point_matrix[3] = tempmatrix[3];

		    tempmatrix[0] =
			matrix[0] * tmpVals2.pixel_matrix[0] +
			matrix[1] * tmpVals2.pixel_matrix[2];
		    tempmatrix[1] =
			matrix[0] * tmpVals2.pixel_matrix[1] +
			matrix[1] * tmpVals2.pixel_matrix[3];
		    tempmatrix[2] =
			matrix[2] * tmpVals2.pixel_matrix[0] +
			matrix[3] * tmpVals2.pixel_matrix[2];
		    tempmatrix[3] =
			matrix[2] * tmpVals2.pixel_matrix[1] +
			matrix[3] * tmpVals2.pixel_matrix[3];
		    tmpVals2.pixel_matrix[0] = tempmatrix[0];
		    tmpVals2.pixel_matrix[1] = tempmatrix[1];
		    tmpVals2.pixel_matrix[2] = tempmatrix[2];
		    tmpVals2.pixel_matrix[3] = tempmatrix[3];

		    tmpVals2.values_supplied =
			(tmpVals2.values_supplied &
			 ~(PIXELSIZE_MASK | POINTSIZE_MASK)) |
			PIXELSIZE_ARRAY | POINTSIZE_ARRAY;
		}
		else
		    nameok = 0;
	    }

	    CopyISOLatin1Lowered (aliasname, *aliasName, len + 1);
	    if (nameok && FontParseXLFDName(aliasname, &tmpVals2,
				  FONT_XLFD_REPLACE_VALUE))
		/* Return a version of the aliasname that has
		   had the vals stuffed into it.  To avoid
		   memory leak, this alias name lives in a
		   static buffer.  The caller needs to be done
		   with this buffer before this procedure is
		   called again to avoid reentrancy problems. */
		    *aliasName = aliasname;
	}
    }
    return nameok;
}

/* ARGSUSED */
int
FontFileOpenFont (pointer client, FontPathElementPtr fpe, Mask flags,
		  const char *name, int namelen,
		  fsBitmapFormat format, fsBitmapFormatMask fmask,
		  XID id, FontPtr *pFont, char **aliasName,
		  FontPtr non_cachable_font)
{
    FontDirectoryPtr	dir;
    char		lowerName[MAXFONTNAMELEN];
    char		fileName[MAXFONTFILENAMELEN*2 + 1];
    FontNameRec		tmpName;
    FontEntryPtr	entry;
    FontScalableRec	vals;
    FontScalableEntryPtr   scalable;
    FontScaledPtr	scaled;
    FontBitmapEntryPtr	bitmap;
    int			ret;
    Bool		noSpecificSize;
    int			nranges;
    fsRange		*ranges;

    if (namelen >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;

    /* Match non-scalable pattern */
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    ranges = FontParseRanges(lowerName, &nranges);
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
    if (!FontParseXLFDName(lowerName, &vals, FONT_XLFD_REPLACE_NONE))
	bzero(&vals, sizeof(vals));
    if (!(entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName)) &&
	tmpName.ndashes == 14 &&
	FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO))
    {
        tmpName.length = strlen(lowerName);
	entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName);
    }

    if (entry)
    {
	switch (entry->type) {
	case FONT_ENTRY_BITMAP:
	    bitmap = &entry->u.bitmap;
	    if (bitmap->pFont)
	    {
	    	*pFont = bitmap->pFont;
		(*pFont)->fpe = fpe;
	    	ret = Successful;
	    }
	    else
	    {
		ret = FontFileOpenBitmapNCF (fpe, pFont, flags, entry, format,
					     fmask, non_cachable_font);
		if (ret == Successful && *pFont)
		    (*pFont)->fpe = fpe;
	    }
	    break;
	case FONT_ENTRY_ALIAS:
	    vals.nranges = nranges;
	    vals.ranges = ranges;
	    transfer_values_to_alias(entry->name.name, entry->name.length,
				     entry->u.alias.resolved, aliasName, &vals);
	    ret = FontNameAlias;
	    break;
	default:
	    ret = BadFontName;
	}
    }
    else
    {
	ret = BadFontName;
    }

    if (ret != BadFontName)
    {
	if (ranges) free(ranges);
	return ret;
    }

    /* Match XLFD patterns */
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
    if (!FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO) ||
	!(tmpName.length = strlen (lowerName),
	  entry = FontFileFindNameInScalableDir (&dir->scalable, &tmpName,
						 &vals))) {
	CopyISOLatin1Lowered (lowerName, name, namelen);
	lowerName[namelen] = '\0';
	tmpName.name = lowerName;
	tmpName.length = namelen;
	tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
	entry = FontFileFindNameInScalableDir (&dir->scalable, &tmpName, &vals);
	if (entry)
	{
	    strlcpy(lowerName, entry->name.name, sizeof(lowerName));
	    tmpName.name = lowerName;
	    tmpName.length = entry->name.length;
	    tmpName.ndashes = entry->name.ndashes;
	}
    }

    if (entry)
    {
	noSpecificSize = FALSE;	/* TRUE breaks XLFD enhancements */
    	if (entry->type == FONT_ENTRY_SCALABLE &&
	    FontFileCompleteXLFD (&vals, &entry->u.scalable.extra->defaults))
	{
	    scalable = &entry->u.scalable;
	    if ((vals.values_supplied & PIXELSIZE_MASK) == PIXELSIZE_ARRAY ||
		(vals.values_supplied & POINTSIZE_MASK) == POINTSIZE_ARRAY ||
		(vals.values_supplied &
		 ~SIZE_SPECIFY_MASK & ~CHARSUBSET_SPECIFIED))
		scaled = 0;
	    else
	        scaled = FontFileFindScaledInstance (entry, &vals,
						     noSpecificSize);
	    /*
	     * A scaled instance can occur one of two ways:
	     *
	     *  Either the font has been scaled to this
	     *   size already, in which case scaled->pFont
	     *   will point at that font.
	     *
	     *  Or a bitmap instance in this size exists,
	     *   which is handled as if we got a pattern
	     *   matching the bitmap font name.
	     */
	    if (scaled)
	    {
		if (scaled->pFont)
		{
		    *pFont = scaled->pFont;
		    (*pFont)->fpe = fpe;
		    ret = Successful;
		}
		else if (scaled->bitmap)
		{
		    entry = scaled->bitmap;
		    bitmap = &entry->u.bitmap;
		    if (bitmap->pFont)
		    {
			*pFont = bitmap->pFont;
			(*pFont)->fpe = fpe;
			ret = Successful;
		    }
		    else
		    {
			ret = FontFileOpenBitmapNCF (fpe, pFont, flags, entry,
						     format, fmask,
						     non_cachable_font);
			if (ret == Successful && *pFont)
			    (*pFont)->fpe = fpe;
		    }
		}
		else /* "cannot" happen */
		{
		    ret = BadFontName;
		}
	    }
	    else
	    {
		ret = FontFileMatchBitmapSource (fpe, pFont, flags, entry, &tmpName, &vals, format, fmask, noSpecificSize);
		if (ret != Successful)
		{
		    char origName[MAXFONTNAMELEN];

		    CopyISOLatin1Lowered (origName, name, namelen);
		    origName[namelen] = '\0';

		    /* Pass the original XLFD name in the vals
		       structure; the rasterizer is free to examine it
		       for hidden meanings.  This information will not
		       be saved in the scaled-instances table.  */

		    vals.xlfdName = origName;
		    vals.ranges = ranges;
		    vals.nranges = nranges;

		    if (strlen(dir->directory) + strlen(scalable->fileName) >=
			sizeof(fileName)) {
			ret = BadFontName;
		    } else {
			strlcpy (fileName, dir->directory, sizeof(fileName));
			strlcat (fileName, scalable->fileName, sizeof(fileName));
                        if (scalable->renderer->OpenScalable) {
			    ret = (*scalable->renderer->OpenScalable) (fpe, pFont,
			       flags, entry, fileName, &vals, format, fmask,
			       non_cachable_font);
                        }
                        else if (scalable->renderer->OpenBitmap) {
                            ret = (*scalable->renderer->OpenBitmap) (fpe, pFont,
                               flags, entry, fileName, format, fmask,
                               non_cachable_font);
                        }
		    }

		    /* In case rasterizer does something bad because of
		       charset subsetting... */
		    if (ret == Successful &&
			((*pFont)->info.firstCol > (*pFont)->info.lastCol ||
			 (*pFont)->info.firstRow > (*pFont)->info.lastRow))
		    {
			(*(*pFont)->unload_font)(*pFont);
			ret = BadFontName;
		    }
		    /* Save the instance */
		    if (ret == Successful)
		    {
		    	if (FontFileAddScaledInstance (entry, &vals,
						    *pFont, (char *) 0))
			    ranges = 0;
			else
			    (*pFont)->fpePrivate = (pointer) 0;
			(*pFont)->fpe = fpe;
		    }
		}
	    }
	}
    }
    else
	ret = BadFontName;

    if (ranges)
	free(ranges);
    return ret;
}

/* ARGSUSED */
void
FontFileCloseFont (FontPathElementPtr fpe, FontPtr pFont)
{
    FontEntryPtr    entry;

    if ((entry = (FontEntryPtr) pFont->fpePrivate)) {
	switch (entry->type) {
	case FONT_ENTRY_SCALABLE:
	    FontFileRemoveScaledInstance (entry, pFont);
	    break;
	case FONT_ENTRY_BITMAP:
	    entry->u.bitmap.pFont = 0;
	    break;
	default:
	    /* "cannot" happen */
	    break;
	}
	pFont->fpePrivate = 0;
    }
    (*pFont->unload_font) (pFont);
}

static int
FontFileOpenBitmapNCF (FontPathElementPtr fpe, FontPtr *pFont,
		       int flags, FontEntryPtr entry,
		       fsBitmapFormat format, fsBitmapFormatMask fmask,
		       FontPtr non_cachable_font)
{
    FontBitmapEntryPtr	bitmap;
    char		fileName[MAXFONTFILENAMELEN*2+1];
    int			ret;
    FontDirectoryPtr	dir;

    dir = (FontDirectoryPtr) fpe->private;
    bitmap = &entry->u.bitmap;
    if(!bitmap || !bitmap->renderer->OpenBitmap)
        return BadFontName;
    if (strlen(dir->directory) + strlen(bitmap->fileName) >= sizeof(fileName))
	return BadFontName;
    strlcpy (fileName, dir->directory, sizeof(fileName));
    strlcat (fileName, bitmap->fileName, sizeof(fileName));
    ret = (*bitmap->renderer->OpenBitmap)
			(fpe, pFont, flags, entry, fileName, format, fmask,
			 non_cachable_font);
    if (ret == Successful)
    {
	bitmap->pFont = *pFont;
	(*pFont)->fpePrivate = (pointer) entry;
    }
    return ret;
}

int
FontFileOpenBitmap (FontPathElementPtr fpe, FontPtr *pFont,
		    int flags, FontEntryPtr entry,
		    fsBitmapFormat format, fsBitmapFormatMask fmask)
{
    return FontFileOpenBitmapNCF (fpe, pFont, flags, entry, format, fmask,
				  (FontPtr)0);
}

static int
FontFileGetInfoBitmap (FontPathElementPtr fpe, FontInfoPtr pFontInfo,
		       FontEntryPtr entry)
{
    FontBitmapEntryPtr	bitmap;
    char		fileName[MAXFONTFILENAMELEN*2+1];
    int			ret;
    FontDirectoryPtr	dir;

    dir = (FontDirectoryPtr) fpe->private;
    bitmap = &entry->u.bitmap;
    if (!bitmap || !bitmap->renderer->GetInfoBitmap)
	return BadFontName;
    if (strlen(dir->directory) + strlen(bitmap->fileName) >= sizeof(fileName))
	return BadFontName;
    strlcpy (fileName, dir->directory, sizeof(fileName));
    strlcat (fileName, bitmap->fileName, sizeof(fileName));
    ret = (*bitmap->renderer->GetInfoBitmap) (fpe, pFontInfo, entry, fileName);
    return ret;
}

static void
_FontFileAddScalableNames(FontNamesPtr names, FontNamesPtr scaleNames,
			  FontNamePtr nameptr, char *zeroChars,
			  FontScalablePtr vals, fsRange *ranges,
			  int nranges, int *max)
{
    int i;
    FontScalableRec	zeroVals, tmpVals;
    for (i = 0; i < scaleNames->nnames; i++)
    {
	char nameChars[MAXFONTNAMELEN];
	if (!*max)
	    return;
	FontParseXLFDName (scaleNames->names[i], &zeroVals,
			   FONT_XLFD_REPLACE_NONE);
	tmpVals = *vals;
	if (FontFileCompleteXLFD (&tmpVals, &zeroVals))
	{
	    --*max;

	    strlcpy (nameChars, scaleNames->names[i], sizeof(nameChars));
	    if ((vals->values_supplied & PIXELSIZE_MASK) ||
		!(vals->values_supplied & PIXELSIZE_WILDCARD) ||
		vals->y == 0)
	    {
		tmpVals.values_supplied =
		    (tmpVals.values_supplied & ~PIXELSIZE_MASK) |
		    (vals->values_supplied & PIXELSIZE_MASK);
		tmpVals.pixel_matrix[0] = vals->pixel_matrix[0];
		tmpVals.pixel_matrix[1] = vals->pixel_matrix[1];
		tmpVals.pixel_matrix[2] = vals->pixel_matrix[2];
		tmpVals.pixel_matrix[3] = vals->pixel_matrix[3];
	    }
	    if ((vals->values_supplied & POINTSIZE_MASK) ||
		!(vals->values_supplied & POINTSIZE_WILDCARD) ||
		vals->y == 0)
	    {
		tmpVals.values_supplied =
		    (tmpVals.values_supplied & ~POINTSIZE_MASK) |
		    (vals->values_supplied & POINTSIZE_MASK);
		tmpVals.point_matrix[0] = vals->point_matrix[0];
		tmpVals.point_matrix[1] = vals->point_matrix[1];
		tmpVals.point_matrix[2] = vals->point_matrix[2];
		tmpVals.point_matrix[3] = vals->point_matrix[3];
	    }
	    if (vals->width <= 0)
		tmpVals.width = 0;
	    if (vals->x == 0)
		tmpVals.x = 0;
	    if (vals->y == 0)
		tmpVals.y = 0;
	    tmpVals.ranges = ranges;
	    tmpVals.nranges = nranges;
	    FontParseXLFDName (nameChars, &tmpVals,
			       FONT_XLFD_REPLACE_VALUE);
	    /* If we're marking aliases with negative lengths, we
	       need to concoct a valid target name to follow it.
	       Otherwise we're done.  */
	    if (scaleNames->length[i] >= 0)
	    {
		(void) xfont2_add_font_names_name (names, nameChars,
					 strlen (nameChars));
		/* If our original pattern matches the name from
		   the table and that name doesn't duplicate what
		   we just added, add the name from the table */
		if (strcmp(nameChars, scaleNames->names[i]) &&
		    FontFileMatchName(scaleNames->names[i],
				      scaleNames->length[i],
				      nameptr) &&
		    *max)
		{
		    --*max;
		    (void) xfont2_add_font_names_name (names, scaleNames->names[i],
					     scaleNames->length[i]);
		}
	    }
	    else
	    {
		char *aliasName;
		vals->ranges = ranges;
		vals->nranges = nranges;
		if (transfer_values_to_alias(zeroChars,
					     strlen(zeroChars),
					     scaleNames->names[++i],
					     &aliasName, vals))
		{
		    (void) xfont2_add_font_names_name (names, nameChars,
					     strlen (nameChars));
		    names->length[names->nnames - 1] =
			-names->length[names->nnames - 1];
		    (void) xfont2_add_font_names_name (names, aliasName,
					     strlen (aliasName));
		    /* If our original pattern matches the name from
		       the table and that name doesn't duplicate what
		       we just added, add the name from the table */
		    if (strcmp(nameChars, scaleNames->names[i - 1]) &&
			FontFileMatchName(scaleNames->names[i - 1],
					  -scaleNames->length[i - 1],
					  nameptr) &&
			*max)
		    {
			--*max;
			(void) xfont2_add_font_names_name (names,
						 scaleNames->names[i - 1],
						 -scaleNames->length[i - 1]);
			names->length[names->nnames - 1] =
			    -names->length[names->nnames - 1];
			(void) xfont2_add_font_names_name (names, aliasName,
						 strlen (aliasName));
		    }
		}
	    }
	}
    }
}

/* ARGSUSED */
static int
_FontFileListFonts (pointer client, FontPathElementPtr fpe,
		    const char *pat, int len, int max, FontNamesPtr names,
		    int mark_aliases)
{
    FontDirectoryPtr	dir;
    char		lowerChars[MAXFONTNAMELEN], zeroChars[MAXFONTNAMELEN];
    FontNameRec		lowerName;
    FontNameRec		zeroName;
    FontNamesPtr	scaleNames;
    FontScalableRec	vals;
    fsRange		*ranges;
    int			nranges;
    int			result = BadFontName;

    if (len >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;
    CopyISOLatin1Lowered (lowerChars, pat, len);
    lowerChars[len] = '\0';
    lowerName.name = lowerChars;
    lowerName.length = len;
    lowerName.ndashes = FontFileCountDashes (lowerChars, len);

    /* Match XLFD patterns */

    strlcpy (zeroChars, lowerChars, sizeof(zeroChars));
    if (lowerName.ndashes == 14 &&
	FontParseXLFDName (zeroChars, &vals, FONT_XLFD_REPLACE_ZERO))
    {
	ranges = FontParseRanges(lowerChars, &nranges);
        result = FontFileFindNamesInScalableDir (&dir->nonScalable,
				&lowerName, max, names,
				(FontScalablePtr)0,
				(mark_aliases ?
				 LIST_ALIASES_AND_TARGET_NAMES :
				 NORMAL_ALIAS_BEHAVIOR) |
				IGNORE_SCALABLE_ALIASES,
				&max);
	zeroName.name = zeroChars;
	zeroName.length = strlen (zeroChars);
	zeroName.ndashes = lowerName.ndashes;

	/* Look for scalable names and aliases, adding scaled instances of
	   them to the output */

	/* Scalable names... */
	scaleNames = xfont2_make_font_names_record (0);
	if (!scaleNames)
	{
	    if (ranges) free(ranges);
	    return AllocError;
	}
	FontFileFindNamesInScalableDir (&dir->scalable, &zeroName, max,
					scaleNames, &vals,
					mark_aliases ?
					LIST_ALIASES_AND_TARGET_NAMES :
					NORMAL_ALIAS_BEHAVIOR, (int *)0);
	_FontFileAddScalableNames(names, scaleNames, &lowerName,
				  zeroChars, &vals, ranges, nranges,
				  &max);
	xfont2_free_font_names (scaleNames);

	/* Scalable aliases... */
	scaleNames = xfont2_make_font_names_record (0);
	if (!scaleNames)
	{
	    if (ranges) free(ranges);
	    return AllocError;
	}
	FontFileFindNamesInScalableDir (&dir->nonScalable, &zeroName,
					max, scaleNames, &vals,
					mark_aliases ?
					LIST_ALIASES_AND_TARGET_NAMES :
					NORMAL_ALIAS_BEHAVIOR, (int *)0);
	_FontFileAddScalableNames(names, scaleNames, &lowerName,
				  zeroChars, &vals, ranges, nranges,
				  &max);
	xfont2_free_font_names (scaleNames);

	if (ranges) free(ranges);
    }
    else
    {
        result = FontFileFindNamesInScalableDir (&dir->nonScalable,
				&lowerName, max, names,
				(FontScalablePtr)0,
				mark_aliases ?
				LIST_ALIASES_AND_TARGET_NAMES :
				NORMAL_ALIAS_BEHAVIOR,
				&max);
	if (result == Successful)
    	    result = FontFileFindNamesInScalableDir (&dir->scalable,
				&lowerName, max, names,
				(FontScalablePtr)0,
				mark_aliases ?
				LIST_ALIASES_AND_TARGET_NAMES :
				NORMAL_ALIAS_BEHAVIOR, (int *)0);
    }
    return result;
}

typedef struct _LFWIData {
    FontNamesPtr    names;
    int                   current;
} LFWIDataRec, *LFWIDataPtr;

int
FontFileListFonts (pointer client, FontPathElementPtr fpe, const char *pat,
		   int len, int max, FontNamesPtr names)
{
    return _FontFileListFonts (client, fpe, pat, len, max, names, 0);
}

int
FontFileStartListFonts(pointer client, FontPathElementPtr fpe,
		       const char *pat, int len, int max,
		       pointer *privatep, int mark_aliases)
{
    LFWIDataPtr	data;
    int		ret;

    data = malloc (sizeof *data);
    if (!data)
	return AllocError;
    data->names = xfont2_make_font_names_record (0);
    if (!data->names)
    {
	free (data);
	return AllocError;
    }
    ret = _FontFileListFonts (client, fpe, pat, len,
			      max, data->names, mark_aliases);
    if (ret != Successful)
    {
	xfont2_free_font_names (data->names);
	free (data);
	return ret;
    }
    data->current = 0;
    *privatep = (pointer) data;
    return Successful;
}


int
FontFileStartListFontsWithInfo(pointer client, FontPathElementPtr fpe,
			       const char *pat, int len, int max,
			       pointer *privatep)
{
    return FontFileStartListFonts(client, fpe, pat, len, max, privatep, 0);
}

/* ARGSUSED */
static int
FontFileListOneFontWithInfo (pointer client, FontPathElementPtr fpe,
			     char **namep, int *namelenp,
			     FontInfoPtr *pFontInfo)
{
    FontDirectoryPtr	dir;
    char		lowerName[MAXFONTNAMELEN];
    char		fileName[MAXFONTFILENAMELEN*2 + 1];
    FontNameRec		tmpName;
    FontEntryPtr	entry;
    FontScalableRec	vals;
    FontScalableEntryPtr   scalable;
    FontScaledPtr	scaled;
    FontBitmapEntryPtr	bitmap;
    int			ret;
    Bool		noSpecificSize;
    int			nranges;
    fsRange		*ranges;

    char		*name = *namep;
    int			namelen = *namelenp;

    if (namelen >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;

    /* Match non-scalable pattern */
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    ranges = FontParseRanges(lowerName, &nranges);
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
    if (!FontParseXLFDName(lowerName, &vals, FONT_XLFD_REPLACE_NONE))
	bzero(&vals, sizeof(vals));
    if (!(entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName)) &&
	tmpName.ndashes == 14 &&
	FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO))
    {
        tmpName.length = strlen(lowerName);
	entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName);
    }

    if (entry)
    {
	switch (entry->type) {
	case FONT_ENTRY_BITMAP:
	    bitmap = &entry->u.bitmap;
	    if (bitmap->pFont)
	    {
	    	*pFontInfo = &bitmap->pFont->info;
	    	ret = Successful;
	    }
	    else
	    {
		ret = FontFileGetInfoBitmap (fpe, *pFontInfo, entry);
	    }
	    break;
	case FONT_ENTRY_ALIAS:
	    vals.nranges = nranges;
	    vals.ranges = ranges;
	    transfer_values_to_alias(entry->name.name, entry->name.length,
				     entry->u.alias.resolved, namep, &vals);
	    *namelenp = strlen (*namep);
	    ret = FontNameAlias;
	    break;
	default:
	    ret = BadFontName;
	}
    }
    else
    {
      ret = BadFontName;
    }

    if (ret != BadFontName)
    {
	if (ranges) free(ranges);
	return ret;
    }

    /* Match XLFD patterns */
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
    if (!FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO) ||
	!(tmpName.length = strlen (lowerName),
	  entry = FontFileFindNameInScalableDir (&dir->scalable, &tmpName,
						 &vals))) {
	CopyISOLatin1Lowered (lowerName, name, namelen);
	lowerName[namelen] = '\0';
	tmpName.name = lowerName;
	tmpName.length = namelen;
	tmpName.ndashes = FontFileCountDashes (lowerName, namelen);
	entry = FontFileFindNameInScalableDir (&dir->scalable, &tmpName, &vals);
	if (entry)
	{
	    strlcpy(lowerName, entry->name.name, sizeof(lowerName));
	    tmpName.name = lowerName;
	    tmpName.length = entry->name.length;
	    tmpName.ndashes = entry->name.ndashes;
	}
    }

    if (entry)
    {
	noSpecificSize = FALSE;	/* TRUE breaks XLFD enhancements */
    	if (entry && entry->type == FONT_ENTRY_SCALABLE &&
	    FontFileCompleteXLFD (&vals, &entry->u.scalable.extra->defaults))
	{
	    scalable = &entry->u.scalable;
	    scaled = FontFileFindScaledInstance (entry, &vals, noSpecificSize);
	    /*
	     * A scaled instance can occur one of two ways:
	     *
	     *  Either the font has been scaled to this
	     *   size already, in which case scaled->pFont
	     *   will point at that font.
	     *
	     *  Or a bitmap instance in this size exists,
	     *   which is handled as if we got a pattern
	     *   matching the bitmap font name.
	     */
	    if (scaled)
	    {
		if (scaled->pFont)
		{
		    *pFontInfo = &scaled->pFont->info;
		    ret = Successful;
		}
		else if (scaled->bitmap)
		{
		    entry = scaled->bitmap;
		    bitmap = &entry->u.bitmap;
		    if (bitmap->pFont)
		    {
			*pFontInfo = &bitmap->pFont->info;
			ret = Successful;
		    }
		    else
		    {
			ret = FontFileGetInfoBitmap (fpe, *pFontInfo, entry);
		    }
		}
		else /* "cannot" happen */
		{
		    ret = BadFontName;
		}
	    }
	    else
	    {
		{
		    char origName[MAXFONTNAMELEN];

		    CopyISOLatin1Lowered (origName, name, namelen);
		    origName[namelen] = '\0';
		    vals.xlfdName = origName;
		    vals.ranges = ranges;
		    vals.nranges = nranges;

		    /* Make a new scaled instance */
		    if (strlen(dir->directory) + strlen(scalable->fileName) >=
			sizeof(fileName)) {
			ret = BadFontName;
		    } else {
			strlcpy (fileName, dir->directory, sizeof(fileName));
			strlcat (fileName, scalable->fileName, sizeof(fileName));
                        if (scalable->renderer->GetInfoScalable)
			    ret = (*scalable->renderer->GetInfoScalable)
			        (fpe, *pFontInfo, entry, &tmpName, fileName,
                                 &vals);
                        else if (scalable->renderer->GetInfoBitmap)
                            ret = (*scalable->renderer->GetInfoBitmap)
                                (fpe, *pFontInfo, entry, fileName);
		    }
		    if (ranges) {
			free(ranges);
			ranges = NULL;
		    }
		}
	    }
	    if (ret == Successful) return ret;
	}
	CopyISOLatin1Lowered (lowerName, name, namelen);
	tmpName.length = namelen;
    }
    else
	ret = BadFontName;

    if (ranges)
	free(ranges);
    return ret;
}

int
FontFileListNextFontWithInfo(pointer client, FontPathElementPtr fpe,
			     char **namep, int *namelenp,
			     FontInfoPtr *pFontInfo,
			     int *numFonts, pointer private)
{
    LFWIDataPtr	data = (LFWIDataPtr) private;
    int		ret;
    char	*name;
    int		namelen;

    if (data->current == data->names->nnames)
    {
	xfont2_free_font_names (data->names);
	free (data);
	return BadFontName;
    }
    name = data->names->names[data->current];
    namelen = data->names->length[data->current];
    ret = FontFileListOneFontWithInfo (client, fpe, &name, &namelen, pFontInfo);
    if (ret == BadFontName)
	ret = AllocError;
    *namep = name;
    *namelenp = namelen;
    ++data->current;
    *numFonts = data->names->nnames - data->current;
    return ret;
}

int
FontFileStartListFontsAndAliases(pointer client, FontPathElementPtr fpe,
				 const char *pat, int len, int max,
				 pointer *privatep)
{
    return FontFileStartListFonts(client, fpe, pat, len, max, privatep, 1);
}

int
FontFileListNextFontOrAlias(pointer client, FontPathElementPtr fpe,
			    char **namep, int *namelenp, char **resolvedp,
			    int *resolvedlenp, pointer private)
{
    LFWIDataPtr	data = (LFWIDataPtr) private;
    int		ret;
    char	*name;
    int		namelen;

    if (data->current == data->names->nnames)
    {
	xfont2_free_font_names (data->names);
	free (data);
	return BadFontName;
    }
    name = data->names->names[data->current];
    namelen = data->names->length[data->current];

    /* If this is a real font name... */
    if (namelen >= 0)
    {
	*namep = name;
	*namelenp = namelen;
	ret = Successful;
    }
    /* Else if an alias */
    else
    {
	/* Tell the caller that this is an alias... let him resolve it to
	   see if it's valid */
	*namep = name;
	*namelenp = -namelen;
	*resolvedp = data->names->names[++data->current];
	*resolvedlenp = data->names->length[data->current];
	ret = FontNameAlias;
    }

    ++data->current;
    return ret;
}

static const xfont2_fpe_funcs_rec fontfile_fpe_funcs = {
	.version = XFONT2_FPE_FUNCS_VERSION,
	.name_check = FontFileNameCheck,
	.init_fpe = FontFileInitFPE,
	.free_fpe = FontFileFreeFPE,
	.reset_fpe = FontFileResetFPE,
	.open_font = FontFileOpenFont,
	.close_font = FontFileCloseFont,
	.list_fonts = FontFileListFonts,
	.start_list_fonts_with_info = FontFileStartListFontsWithInfo,
	.list_next_font_with_info = FontFileListNextFontWithInfo,
	.wakeup_fpe = 0,
	.client_died = 0,
	.load_glyphs = 0,
	.start_list_fonts_and_aliases = FontFileStartListFontsAndAliases,
	.list_next_font_or_alias = FontFileListNextFontOrAlias,
	.set_path_hook = FontFileEmptyBitmapSource,
};

void
FontFileRegisterLocalFpeFunctions (void)
{
    register_fpe_funcs(&fontfile_fpe_funcs);
}
