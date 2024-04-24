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

#ifndef _FONTFILE_H_
#define _FONTFILE_H_

#include <X11/fonts/fontxlfd.h>

typedef struct _FontEntry	    *FontEntryPtr;
typedef struct _FontTable	    *FontTablePtr;
typedef struct _FontName	    *FontNamePtr;
typedef struct _FontScaled	    *FontScaledPtr;
typedef struct _FontScalableExtra   *FontScalableExtraPtr;
typedef struct _FontScalableEntry   *FontScalableEntryPtr;
typedef struct _FontScaleAliasEntry *FontScaleAliasEntryPtr;
typedef struct _FontBitmapEntry	    *FontBitmapEntryPtr;
typedef struct _FontAliasEntry	    *FontAliasEntryPtr;
typedef struct _FontDirectory	    *FontDirectoryPtr;
typedef struct _FontRenderer	    *FontRendererPtr;

#define NullFontEntry		    ((FontEntryPtr) 0)
#define NullFontTable		    ((FontTablePtr) 0)
#define NullFontName		    ((FontNamePtr) 0)
#define NullFontScaled		    ((FontScaled) 0)
#define NullFontScalableExtra	    ((FontScalableExtra) 0)
#define NullFontscalableEntry	    ((FontScalableEntry) 0)
#define NullFontScaleAliasEntry	    ((FontScaleAliasEntry) 0)
#define NullFontBitmapEntry	    ((FontBitmapEntry) 0)
#define NullFontAliasEntry	    ((FontAliasEntry) 0)
#define NullFontDirectory	    ((FontDirectoryPtr) 0)
#define NullFontRenderer	    ((FontRendererPtr) 0)

#define FONT_ENTRY_SCALABLE	0
#define FONT_ENTRY_SCALE_ALIAS	1
#define FONT_ENTRY_BITMAP	2
#define FONT_ENTRY_ALIAS	3

#define MAXFONTNAMELEN	    1024
#define MAXFONTFILENAMELEN  1024

#define FontDirFile	    "fonts.dir"
#define FontAliasFile	    "fonts.alias"
#define FontScalableFile    "fonts.scale"

extern int FontFileNameCheck ( const char *name );
extern int FontFileInitFPE ( FontPathElementPtr fpe );
extern int FontFileResetFPE ( FontPathElementPtr fpe );
extern int FontFileFreeFPE ( FontPathElementPtr fpe );
extern int FontFileOpenFont ( pointer client, FontPathElementPtr fpe,
			      Mask flags, const char *name, int namelen,
			      fsBitmapFormat format, fsBitmapFormatMask fmask,
			      XID id, FontPtr *pFont, char **aliasName,
			      FontPtr non_cachable_font );
extern void FontFileCloseFont ( FontPathElementPtr fpe, FontPtr pFont );
extern int FontFileOpenBitmap ( FontPathElementPtr fpe, FontPtr *pFont,
				int flags, FontEntryPtr entry,
				fsBitmapFormat format,
				fsBitmapFormatMask fmask );
extern int FontFileListFonts ( pointer client, FontPathElementPtr fpe,
			       const char *pat, int len, int max,
			       FontNamesPtr names );
extern int FontFileStartListFonts ( pointer client, FontPathElementPtr fpe,
				    const char *pat, int len, int max,
				    pointer *privatep, int mark_aliases );
extern int FontFileStartListFontsWithInfo ( pointer client,
					    FontPathElementPtr fpe,
					    const char *pat, int len, int max,
					    pointer *privatep );
extern int FontFileListNextFontWithInfo ( pointer client,
					  FontPathElementPtr fpe,
					  char **namep, int *namelenp,
					  FontInfoPtr *pFontInfo,
					  int *numFonts, pointer private );
extern int FontFileStartListFontsAndAliases ( pointer client,
					      FontPathElementPtr fpe,
					      const char *pat, int len, int max,
					      pointer *privatep );
extern int FontFileListNextFontOrAlias ( pointer client,
					 FontPathElementPtr fpe,
					 char **namep, int *namelenp,
					 char **resolvedp, int *resolvedlenp,
					 pointer private );
extern void FontFileRegisterLocalFpeFunctions ( void );
extern void CatalogueRegisterLocalFpeFunctions ( void );


extern FontEntryPtr FontFileAddEntry ( FontTablePtr table,
				       FontEntryPtr prototype );
extern Bool FontFileAddFontAlias ( FontDirectoryPtr dir, char *aliasName,
				   char *fontName );
extern Bool FontFileAddFontFile ( FontDirectoryPtr dir, char *fontName,
				  char *fileName );
extern int FontFileCountDashes ( char *name, int namelen );
extern FontEntryPtr FontFileFindNameInDir ( FontTablePtr table,
					    FontNamePtr pat );
extern FontEntryPtr FontFileFindNameInScalableDir ( FontTablePtr table,
						    FontNamePtr pat,
						    FontScalablePtr vals );
extern int FontFileFindNamesInDir ( FontTablePtr table, FontNamePtr pat,
				    int max, FontNamesPtr names );
extern int FontFileFindNamesInScalableDir ( FontTablePtr table,
					    FontNamePtr pat, int max,
					    FontNamesPtr names,
					    FontScalablePtr vals,
					    int alias_behavior, int *newmax );

extern void FontFileFreeDir ( FontDirectoryPtr dir );
extern void FontFileFreeEntry ( FontEntryPtr entry );
extern void FontFileFreeTable ( FontTablePtr table );
extern Bool FontFileInitTable ( FontTablePtr table, int size );
extern FontDirectoryPtr FontFileMakeDir ( const char *dirName, int size );
extern Bool FontFileMatchName ( char *name, int length, FontNamePtr pat );
extern char * FontFileSaveString ( char *s );
extern void FontFileSortDir ( FontDirectoryPtr dir );
extern void FontFileSortTable ( FontTablePtr table );

extern void FontDefaultFormat ( int *bit, int *byte, int *glyph, int *scan );

extern Bool FontFileRegisterRenderer ( FontRendererPtr renderer );
extern Bool FontFilePriorityRegisterRenderer ( FontRendererPtr renderer,
                                               int priority );
extern FontRendererPtr FontFileMatchRenderer ( char *fileName );

extern Bool FontFileAddScaledInstance ( FontEntryPtr entry,
					FontScalablePtr vals, FontPtr pFont,
					char *bitmapName );
extern void FontFileSwitchStringsToBitmapPointers ( FontDirectoryPtr dir );
extern void FontFileRemoveScaledInstance ( FontEntryPtr entry, FontPtr pFont );
extern Bool FontFileCompleteXLFD ( FontScalablePtr vals, FontScalablePtr def );
extern FontScaledPtr FontFileFindScaledInstance ( FontEntryPtr entry,
						  FontScalablePtr vals,
						  int noSpecificSize );

extern Bool FontFileRegisterBitmapSource ( FontPathElementPtr fpe );
extern void FontFileUnregisterBitmapSource ( FontPathElementPtr fpe );
extern void FontFileEmptyBitmapSource ( void );
extern int FontFileMatchBitmapSource ( FontPathElementPtr fpe,
				       FontPtr *pFont, int flags,
				       FontEntryPtr entry,
				       FontNamePtr zeroPat,
				       FontScalablePtr vals,
				       fsBitmapFormat format,
				       fsBitmapFormatMask fmask,
				       Bool noSpecificSize );

extern int FontFileReadDirectory ( const char *directory, FontDirectoryPtr *pdir );
extern Bool FontFileDirectoryChanged ( FontDirectoryPtr dir );

#endif /* _FONTFILE_H_ */
