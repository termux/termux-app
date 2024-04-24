/************************************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/*

Copyright 1994, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "src/util/replace.h"

#include <ctype.h>
#include <X11/fonts/fntfilst.h>
#include <X11/fonts/bitmap.h>
#include "snfstr.h"

#include <stdarg.h>

static void _X_ATTRIBUTE_PRINTF(1, 2)
snfError(const char* message, ...)
{
    va_list args;

    va_start(args, message);

    fprintf(stderr, "SNF Error: ");
    vfprintf(stderr, message, args);
    va_end(args);
}

static void snfUnloadFont(FontPtr pFont);

static int
snfReadCharInfo(FontFilePtr file, CharInfoPtr charInfo, char *base)
{
    snfCharInfoRec snfCharInfo;

#define Width(m)    ((m).rightSideBearing - (m).leftSideBearing)
#define Height(m)   ((m).ascent + (m).descent)

    if (FontFileRead(file, (char *) &snfCharInfo, sizeof snfCharInfo) !=
	    sizeof(snfCharInfo)) {
	return BadFontName;
    }
    charInfo->metrics = snfCharInfo.metrics;
    if (snfCharInfo.exists)
	charInfo->bits = base + snfCharInfo.byteOffset;
    else
	charInfo->bits = 0;
    return Successful;
}

static int
snfReadxCharInfo(FontFilePtr file, xCharInfo *charInfo)
{
    snfCharInfoRec snfCharInfo;

    if (FontFileRead(file, (char *) &snfCharInfo, sizeof snfCharInfo) !=
	    sizeof(snfCharInfo)) {
	return BadFontName;
    }
    *charInfo = snfCharInfo.metrics;
    return Successful;
}

static void
snfCopyInfo(snfFontInfoPtr snfInfo, FontInfoPtr pFontInfo)
{
    pFontInfo->firstCol = snfInfo->firstCol;
    pFontInfo->lastCol = snfInfo->lastCol;
    pFontInfo->firstRow = snfInfo->firstRow;
    pFontInfo->lastRow = snfInfo->lastRow;
    pFontInfo->defaultCh = snfInfo->chDefault;
    pFontInfo->noOverlap = snfInfo->noOverlap;
    pFontInfo->terminalFont = snfInfo->terminalFont;
    pFontInfo->constantMetrics = snfInfo->constantMetrics;
    pFontInfo->constantWidth = snfInfo->constantWidth;
    pFontInfo->inkInside = snfInfo->inkInside;
    pFontInfo->inkMetrics = snfInfo->inkMetrics;
    pFontInfo->allExist = snfInfo->allExist;
    pFontInfo->drawDirection = snfInfo->drawDirection;
    pFontInfo->anamorphic = FALSE;
    pFontInfo->cachable = TRUE;
    pFontInfo->maxOverlap = 0;
    pFontInfo->minbounds = snfInfo->minbounds.metrics;
    pFontInfo->maxbounds = snfInfo->maxbounds.metrics;
    pFontInfo->fontAscent = snfInfo->fontAscent;
    pFontInfo->fontDescent = snfInfo->fontDescent;
    pFontInfo->nprops = snfInfo->nProps;
}

static int
snfReadProps(snfFontInfoPtr snfInfo, FontInfoPtr pFontInfo, FontFilePtr file)
{
    char       *strings;
    FontPropPtr pfp;
    snfFontPropPtr psnfp;
    char       *propspace;
    int         bytestoalloc;
    int         i;

    bytestoalloc = snfInfo->nProps * sizeof(snfFontPropRec) +
	BYTESOFSTRINGINFO(snfInfo);
    propspace = malloc(bytestoalloc);
    if (!propspace) {
      snfError("snfReadProps(): Couldn't allocate propspace (%d)\n", bytestoalloc);
	return AllocError;
    }

    if (FontFileRead(file, propspace, bytestoalloc) != bytestoalloc) {
	free(propspace);
	return BadFontName;
    }
    psnfp = (snfFontPropPtr) propspace;

    strings = propspace + BYTESOFPROPINFO(snfInfo);

    for (i = 0, pfp = pFontInfo->props; i < snfInfo->nProps; i++, pfp++, psnfp++) {
	pfp->name = MakeAtom(&strings[psnfp->name],
			     (unsigned) strlen(&strings[psnfp->name]), 1);
	pFontInfo->isStringProp[i] = psnfp->indirect;
	if (psnfp->indirect)
	    pfp->value = (INT32) MakeAtom(&strings[psnfp->value],
			       (unsigned) strlen(&strings[psnfp->value]), 1);
	else
	    pfp->value = psnfp->value;
    }

    free(propspace);
    return Successful;
}

static int
snfReadHeader(snfFontInfoPtr snfInfo, FontFilePtr file)
{
    if (FontFileRead(file, (char *) snfInfo, sizeof *snfInfo) != sizeof *snfInfo)
	return BadFontName;

    if (snfInfo->version1 != FONT_FILE_VERSION ||
	    snfInfo->version2 != FONT_FILE_VERSION)
	return BadFontName;
    return Successful;
}

static int  snf_set;
static int  snf_bit, snf_byte, snf_glyph, snf_scan;

void
SnfSetFormat (int bit, int byte, int glyph, int scan)
{
    snf_bit = bit;
    snf_byte = byte;
    snf_glyph = glyph;
    snf_scan = scan;
    snf_set = 1;
}

static void
SnfGetFormat (int *bit, int *byte, int *glyph, int *scan)
{
    if (!snf_set)
	FontDefaultFormat (&snf_bit, &snf_byte, &snf_glyph, &snf_scan);
    *bit = snf_bit;
    *byte = snf_byte;
    *glyph = snf_glyph;
    *scan = snf_scan;
}

int
snfReadFont(FontPtr pFont, FontFilePtr file,
	    int bit, int byte, int glyph, int scan)
{
    snfFontInfoRec fi;
    unsigned    bytestoalloc;
    int         i, j;
    char       *fontspace;
    BitmapFontPtr  bitmapFont;
    int         num_chars;
    int         bitmapsSize;
    int         ret;
    int         metrics_off;
    int         encoding_off;
    int         props_off;
    int         isStringProp_off;
    int         ink_off;
    char	*bitmaps;
    int		def_bit, def_byte, def_glyph, def_scan;

    ret = snfReadHeader(&fi, file);
    if (ret != Successful)
	return ret;

    SnfGetFormat (&def_bit, &def_byte, &def_glyph, &def_scan);

    /*
     * we'll allocate one chunk of memory and split it among the various parts
     * of the font:
     *
     * BitmapFontRec CharInfoRec's Glyphs Encoding DIX Properties Ink CharInfoRec's
     *
     * If the glyphpad is not the same as the font file, then the glyphs
     * are allocated separately, to be later realloc'ed when we know
     * how big to make them.
     */

    bitmapsSize = BYTESOFGLYPHINFO(&fi);
    num_chars = n2dChars(&fi);
    bytestoalloc = sizeof(BitmapFontRec);	/* bitmapFont */
    metrics_off = bytestoalloc;
    bytestoalloc += num_chars * sizeof(CharInfoRec);	/* metrics */
    encoding_off = bytestoalloc;
    bytestoalloc += NUM_SEGMENTS(num_chars) * sizeof(CharInfoPtr**);
                                                /* encoding */
    props_off = bytestoalloc;
    bytestoalloc += fi.nProps * sizeof(FontPropRec);	/* props */
    isStringProp_off = bytestoalloc;
    bytestoalloc += fi.nProps * sizeof(char);	/* isStringProp */
    bytestoalloc = (bytestoalloc + 3) & ~3;
    ink_off = bytestoalloc;
    if (fi.inkMetrics)
	bytestoalloc += num_chars * sizeof(xCharInfo);	/* ink_metrics */

    fontspace = malloc(bytestoalloc);
    if (!fontspace) {
      snfError("snfReadFont(): Couldn't allocate fontspace (%d)\n", bytestoalloc);
	return AllocError;
    }
    bitmaps = malloc (bitmapsSize);
    if (!bitmaps)
    {
      snfError("snfReadFont(): Couldn't allocate bitmaps (%d)\n", bitmapsSize);
	free (fontspace);
	return AllocError;
    }
    /*
     * now fix up pointers
     */

    bitmapFont = (BitmapFontPtr) fontspace;
    bitmapFont->num_chars = num_chars;
    bitmapFont->metrics = (CharInfoPtr) (fontspace + metrics_off);
    bitmapFont->encoding = (CharInfoPtr **) (fontspace + encoding_off);
    bitmapFont->bitmaps = bitmaps;
    bitmapFont->pDefault = NULL;
    bitmapFont->bitmapExtra = NULL;
    pFont->info.props = (FontPropPtr) (fontspace + props_off);
    pFont->info.isStringProp = (char *) (fontspace + isStringProp_off);
    if (fi.inkMetrics)
	bitmapFont->ink_metrics = (xCharInfo *) (fontspace + ink_off);
    else
	bitmapFont->ink_metrics = 0;

    /*
     * read the CharInfo
     */

    ret = Successful;
    memset(bitmapFont->encoding, 0,
           NUM_SEGMENTS(num_chars)*sizeof(CharInfoPtr*));
    for (i = 0; ret == Successful && i < num_chars; i++) {
	ret = snfReadCharInfo(file, &bitmapFont->metrics[i], bitmaps);
	if (bitmapFont->metrics[i].bits) {
            if (!bitmapFont->encoding[SEGMENT_MAJOR(i)]) {
                bitmapFont->encoding[SEGMENT_MAJOR(i)]=
                    calloc(BITMAP_FONT_SEGMENT_SIZE, sizeof(CharInfoPtr));
                if (!bitmapFont->encoding[SEGMENT_MAJOR(i)]) {
                    ret = AllocError;
                    break;
                }
            }
            ACCESSENCODINGL(bitmapFont->encoding,i) = &bitmapFont->metrics[i];
        }
    }

    if (ret != Successful) {
	free(bitmaps);
        if(bitmapFont->encoding) {
            for(j=0; j<SEGMENT_MAJOR(i); j++)
                free(bitmapFont->encoding[i]);
        }
	free(fontspace);
	return ret;
    }
    /*
     * read the glyphs
     */

    if (FontFileRead(file, bitmaps, bitmapsSize) != bitmapsSize) {
	free(bitmaps);
	free(fontspace);
	return BadFontName;
    }

    if (def_bit != bit)
	BitOrderInvert((unsigned char *)bitmaps, bitmapsSize);
    if ((def_byte == def_bit) != (bit == byte)) {
	switch (bit == byte ? def_scan : scan) {
	case 1:
	    break;
	case 2:
	    TwoByteSwap((unsigned char *)bitmaps, bitmapsSize);
	    break;
	case 4:
	    FourByteSwap((unsigned char *)bitmaps, bitmapsSize);
	    break;
	}
    }
    if (def_glyph != glyph) {
	char	    *padbitmaps;
	int         sizepadbitmaps;
	int	    sizechar;
	CharInfoPtr metric;

	sizepadbitmaps = 0;
	metric = bitmapFont->metrics;
	for (i = 0; i < num_chars; i++)
	{
	    if (metric->bits)
		sizepadbitmaps += BYTES_FOR_GLYPH(metric,glyph);
	    metric++;
	}
	padbitmaps = malloc(sizepadbitmaps);
	if (!padbitmaps) {
	    snfError("snfReadFont(): Couldn't allocate padbitmaps (%d)\n", sizepadbitmaps);
	    free (bitmaps);
	    free (fontspace);
	    return AllocError;
	}
	metric = bitmapFont->metrics;
	bitmapFont->bitmaps = padbitmaps;
	for (i = 0; i < num_chars; i++) {
	    sizechar = RepadBitmap(metric->bits, padbitmaps,
			       def_glyph, glyph,
			       metric->metrics.rightSideBearing -
			       metric->metrics.leftSideBearing,
			       metric->metrics.ascent + metric->metrics.descent);
	    metric->bits = padbitmaps;
	    padbitmaps += sizechar;
	    metric++;
	}
	free(bitmaps);
    }

    /* now read and atom'ize properties */

    ret = snfReadProps(&fi, &pFont->info, file);
    if (ret != Successful) {
	free(fontspace);
	return ret;
    }
    snfCopyInfo(&fi, &pFont->info);

    /* finally, read the ink metrics if the exist */

    if (fi.inkMetrics) {
	ret = Successful;
	ret = snfReadxCharInfo(file, &pFont->info.ink_minbounds);
	ret = snfReadxCharInfo(file, &pFont->info.ink_maxbounds);
	for (i = 0; ret == Successful && i < num_chars; i++)
	    ret = snfReadxCharInfo(file, &bitmapFont->ink_metrics[i]);
	if (ret != Successful) {
	    free(fontspace);
	    return ret;
	}
    } else {
	pFont->info.ink_minbounds = pFont->info.minbounds;
	pFont->info.ink_maxbounds = pFont->info.maxbounds;
    }

    if (pFont->info.defaultCh != (unsigned short) NO_SUCH_CHAR) {
	unsigned int r,
	            c,
	            cols;

	r = pFont->info.defaultCh >> 8;
	c = pFont->info.defaultCh & 0xFF;
	if (pFont->info.firstRow <= r && r <= pFont->info.lastRow &&
		pFont->info.firstCol <= c && c <= pFont->info.lastCol) {
	    cols = pFont->info.lastCol - pFont->info.firstCol + 1;
	    r = r - pFont->info.firstRow;
	    c = c - pFont->info.firstCol;
	    bitmapFont->pDefault = &bitmapFont->metrics[r * cols + c];
	}
    }
    bitmapFont->bitmapExtra = (BitmapExtraPtr) 0;
    pFont->fontPrivate = (pointer) bitmapFont;
    pFont->get_glyphs = bitmapGetGlyphs;
    pFont->get_metrics = bitmapGetMetrics;
    pFont->unload_font = snfUnloadFont;
    pFont->unload_glyphs = NULL;
    pFont->bit = bit;
    pFont->byte = byte;
    pFont->glyph = glyph;
    pFont->scan = scan;
    return Successful;
}

int
snfReadFontInfo(FontInfoPtr pFontInfo, FontFilePtr file)
{
    int         ret;
    snfFontInfoRec fi;
    int         bytestoskip;
    int         num_chars;

    ret = snfReadHeader(&fi, file);
    if (ret != Successful)
	return ret;
    snfCopyInfo(&fi, pFontInfo);

    pFontInfo->props = mallocarray(fi.nProps, sizeof(FontPropRec));
    if (!pFontInfo->props) {
	snfError("snfReadFontInfo(): Couldn't allocate props (%d*%d)\n",
		 fi.nProps, (int) sizeof(FontPropRec));
	return AllocError;
    }
    pFontInfo->isStringProp = mallocarray(fi.nProps, sizeof(char));
    if (!pFontInfo->isStringProp) {
	snfError("snfReadFontInfo(): Couldn't allocate isStringProp (%d*%d)\n",
		 fi.nProps, (int) sizeof(char));
	free(pFontInfo->props);
	return AllocError;
    }
    num_chars = n2dChars(&fi);
    bytestoskip = num_chars * sizeof(snfCharInfoRec);	/* charinfos */
    bytestoskip += BYTESOFGLYPHINFO(&fi);
    (void)FontFileSkip(file, bytestoskip);

    ret = snfReadProps(&fi, pFontInfo, file);
    if (ret != Successful) {
	free(pFontInfo->props);
	free(pFontInfo->isStringProp);
	return ret;
    }
    if (fi.inkMetrics) {
	ret = snfReadxCharInfo(file, &pFontInfo->ink_minbounds);
	if (ret != Successful) {
	    free(pFontInfo->props);
	    free(pFontInfo->isStringProp);
	    return ret;
	}
	ret = snfReadxCharInfo(file, &pFontInfo->ink_maxbounds);
	if (ret != Successful) {
	    free(pFontInfo->props);
	    free(pFontInfo->isStringProp);
	    return ret;
	}
    } else {
	pFontInfo->ink_minbounds = pFontInfo->minbounds;
	pFontInfo->ink_maxbounds = pFontInfo->maxbounds;
    }
    return Successful;

}

static void
snfUnloadFont(FontPtr pFont)
{
    BitmapFontPtr   bitmapFont;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
    free (bitmapFont->bitmaps);
    free (bitmapFont);
    DestroyFontRec (pFont);
}

