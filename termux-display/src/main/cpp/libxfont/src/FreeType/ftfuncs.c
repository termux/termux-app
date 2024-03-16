/*
Copyright (c) 1997 by Mark Leisher
Copyright (c) 1998-2003 by Juliusz Chroboczek
Copyright (c) 1998 Go Watanabe, All rights reserved.
Copyright (c) 1998 Kazushi (Jam) Marukawa, All rights reserved.
Copyright (c) 1998 Takuya SHIOZAKI, All rights reserved.
Copyright (c) 1998 X-TrueType Server Project, All rights reserved.
Copyright (c) 2003-2004 After X-TT Project, All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontmisc.h>
#include "src/util/replace.h"

#include <string.h>
#include <math.h>
#include <ctype.h>

#include <X11/fonts/fntfilst.h>
#include <X11/fonts/fontutil.h>
#include <X11/fonts/FSproto.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TYPE1_TABLES_H
#include FT_XFREE86_H
#include FT_BBOX_H
#include FT_TRUETYPE_TAGS_H
/*
 *  If you want to use FT_Outline_Get_CBox instead of
 *  FT_Outline_Get_BBox, define here.
 */
/* #define USE_GET_CBOX */
#ifdef USE_GET_CBOX
#include FT_OUTLINE_H
#endif

#include <X11/fonts/fontenc.h>
#include "ft.h"
#include "ftfuncs.h"
#include "xttcap.h"

/* Work around FreeType bug */
#define WORK_AROUND_UPM 2048

#ifndef True
#define True (-1)
#endif /* True */
#ifndef False
#define False (0)
#endif /* False */

#define FLOOR64(x) ((x) & -64)
#define CEIL64(x) (((x) + 64 - 1) & -64)

/*
 *  If you want very lazy method(vl=y) AS DEFAULT when
 *  handling large charset, define here.
 */
/* #define DEFAULT_VERY_LAZY 1 */     	/* Always */
#define DEFAULT_VERY_LAZY 2     	/* Multi-byte only */
/* #define DEFAULT_VERY_LAZY 256 */   	/* Unicode only */

/* Does the X accept noSuchChar? */
#define X_ACCEPTS_NO_SUCH_CHAR
/* Does the XAA accept NULL noSuchChar.bits?(dangerous) */
/* #define XAA_ACCEPTS_NULL_BITS */

#ifdef X_ACCEPTS_NO_SUCH_CHAR
static CharInfoRec noSuchChar = { /* metrics */{0,0,0,0,0,0},
				  /* bits */   NULL };
#endif

/* The property names for all the XLFD properties. */

static const char *xlfd_props[] = {
    "FOUNDRY",
    "FAMILY_NAME",
    "WEIGHT_NAME",
    "SLANT",
    "SETWIDTH_NAME",
    "ADD_STYLE_NAME",
    "PIXEL_SIZE",
    "POINT_SIZE",
    "RESOLUTION_X",
    "RESOLUTION_Y",
    "SPACING",
    "AVERAGE_WIDTH",
    "CHARSET_REGISTRY",
    "CHARSET_ENCODING",
};


/* read 2-byte value from a SFNT table */
static FT_UShort
sfnt_get_ushort( FT_Face     face,
                 FT_ULong    table_tag,
                 FT_ULong    table_offset )
{
  FT_Byte    buff[2];
  FT_ULong   len = sizeof(buff);
  FT_UShort  result = 0;

  if ( !FT_Load_Sfnt_Table( face, table_tag, table_offset, buff, &len ) )
    result = (FT_UShort)( (buff[0] << 8) | buff[1] );

  return result;
}

#define  sfnt_get_short(f,t,o)  ((FT_Short)sfnt_get_ushort((f),(t),(o)))


static int ftypeInitP = 0;      /* is the engine initialised? */
FT_Library ftypeLibrary;

static FTFacePtr faceTable[NUMFACEBUCKETS];

static unsigned
hash(char *string)
{
    int i;
    unsigned u = 0;
    for(i = 0; string[i] != '\0'; i++)
        u = (u<<5) + (u >> (NUMFACEBUCKETS - 5)) + (unsigned char)string[i];
    return u;
}

static int
ifloor(int x, int y)
{
    if(x >= 0)
        return x/y;
    else
        return x/y - 1;
}

static int
iceil(int x, int y)
{
    return ifloor(x + y - 1, y);
}

static int
FreeTypeOpenFace(FTFacePtr *facep, char *FTFileName, char *realFileName, int faceNumber)
{
    FT_Error ftrc;
    int bucket;
    FTFacePtr face, otherFace;

    if (!ftypeInitP) {
        ftrc = FT_Init_FreeType(&ftypeLibrary);
        if (ftrc != 0) {
            ErrorF("FreeType: error initializing ftypeEngine: %d\n", ftrc);
            return AllocError;
        }
        ftypeInitP = 1;
    }

    /* Try to find a matching face in the hashtable */
    bucket = hash(FTFileName)%NUMFACEBUCKETS;
    otherFace = faceTable[bucket];
    while(otherFace) {
        if( strcmp(otherFace->filename, FTFileName) == 0 ) break;
        otherFace = otherFace->next;
    }
    if(otherFace) {
        MUMBLE("Returning cached face: %s\n", otherFace->filename);
        *facep = otherFace;
        return Successful;
    }

    /* No cached match; need to make a new one */
    face = calloc(1, sizeof(FTFaceRec));
    if (face == NULL) {
        return AllocError;
    }

    face->filename = strdup(FTFileName);
    if (face->filename == NULL) {
        free(face);
        return AllocError;
    }

    ftrc = FT_New_Face(ftypeLibrary, realFileName, faceNumber, &face->face);
    if(ftrc != 0) {
        ErrorF("FreeType: couldn't open face %s: %d\n", FTFileName, ftrc);
        free(face->filename);
        free(face);
        return BadFontName;
    }

    face->bitmap = ((face->face->face_flags & FT_FACE_FLAG_SCALABLE) == 0);
    if(!face->bitmap) {
        TT_MaxProfile *maxp;
        maxp = FT_Get_Sfnt_Table(face->face, ft_sfnt_maxp);
        if(maxp && maxp->maxContours == 0)
            face->bitmap = 1;
    }

    face->num_hmetrics = (FT_UInt) sfnt_get_ushort( face->face,
                                                    TTAG_hhea, 34 );

    /* Insert face in hashtable and return it */
    face->next = faceTable[bucket];
    faceTable[bucket] = face;
    *facep = face;
    return Successful;
}

static void
FreeTypeFreeFace(FTFacePtr face)
{
    int bucket;
    FTFacePtr otherFace;

    if(!face->instances) {
        bucket = hash(face->filename) % NUMFACEBUCKETS;
        if(faceTable[bucket] == face)
            faceTable[bucket] = face->next;
        else {
            otherFace = faceTable[bucket];
            while(otherFace) {
                if(otherFace->next == face)
                    break;
                otherFace = otherFace->next;
            }
            if(otherFace && otherFace->next)
                otherFace->next = otherFace->next->next;
            else
                ErrorF("FreeType: freeing unknown face\n");
        }
        MUMBLE("Closing face: %s\n", face->filename);
        FT_Done_Face(face->face);
        free(face->filename);
        free(face);
    }
}

static int
TransEqual(FTNormalisedTransformationPtr t1, FTNormalisedTransformationPtr t2)
{
    if(t1->scale != t2->scale)
        return 0;
    else if(t1->xres != t2->xres || t1->yres != t2->yres)
        return 0;
    else if(t1->nonIdentity != t2->nonIdentity)
        return 0;
    else if(t1->nonIdentity && t2->nonIdentity) {
        return
            t1->matrix.xx == t2->matrix.xx &&
            t1->matrix.yx == t2->matrix.yx &&
            t1->matrix.yy == t2->matrix.yy &&
            t1->matrix.xy == t2->matrix.xy;
    } else
        return 1;
}

static int
BitmapFormatEqual(FontBitmapFormatPtr f1, FontBitmapFormatPtr f2)
{
    return
        f1->bit == f2->bit &&
        f1->byte == f2->byte &&
        f1->glyph == f2->glyph;
}

static int
TTCapEqual(struct TTCapInfo *t1, struct TTCapInfo *t2)
{
    return
	t1->autoItalic == t2->autoItalic &&
	t1->scaleWidth == t2->scaleWidth &&
	t1->scaleBBoxWidth == t2->scaleBBoxWidth &&
	t1->scaleBBoxHeight == t2->scaleBBoxHeight &&
	t1->doubleStrikeShift == t2->doubleStrikeShift &&
	t1->adjustBBoxWidthByPixel == t2->adjustBBoxWidthByPixel &&
	t1->adjustLeftSideBearingByPixel == t2->adjustLeftSideBearingByPixel &&
	t1->adjustRightSideBearingByPixel == t2->adjustRightSideBearingByPixel &&
	t1->flags == t2->flags &&
	t1->scaleBitmap == t2->scaleBitmap &&
	/*
	  If we use forceConstantSpacing,
	  we *MUST* allocate new instance.
	*/
	t1->forceConstantSpacingEnd < 0 &&
	t2->forceConstantSpacingEnd < 0;
}

static int
FTInstanceMatch(FTInstancePtr instance,
		char *FTFileName, FTNormalisedTransformationPtr trans,
		int spacing, FontBitmapFormatPtr bmfmt,
		struct TTCapInfo *tmp_ttcap, FT_Int32 load_flags)
{
    if(strcmp(instance->face->filename, FTFileName) != 0) {
        return 0;
    } else if(!TransEqual(&instance->transformation, trans)) {
        return 0;
    } else if( spacing != instance->spacing ) {
        return 0;
    } else if( load_flags != instance->load_flags ) {
        return 0;
    } else if(!BitmapFormatEqual(&instance->bmfmt, bmfmt)) {
        return 0;
    } else if(!TTCapEqual(&instance->ttcap, tmp_ttcap)) {
        return 0;
    } else {
        return 1;
    }
}

static int
FreeTypeActivateInstance(FTInstancePtr instance)
{
    FT_Error ftrc;
    if(instance->face->active_instance == instance)
        return Successful;

    ftrc = FT_Activate_Size(instance->size);
    if(ftrc != 0) {
        instance->face->active_instance = NULL;
        ErrorF("FreeType: couldn't activate instance: %d\n", ftrc);
        return FTtoXReturnCode(ftrc);
    }
    FT_Set_Transform(instance->face->face,
                     instance->transformation.nonIdentity ?
                     &instance->transformation.matrix : 0,
                     0);

    instance->face->active_instance = instance;
    return Successful;
}

static int
FTFindSize(FT_Face face, FTNormalisedTransformationPtr trans,
           int *x_return, int *y_return)
{
    int tx, ty, x, y;
    int i, j;
    int d, dd;

    if(trans->nonIdentity)
        return BadFontName;

    tx = (int)(trans->scale * trans->xres / 72.0 + 0.5);
    ty = (int)(trans->scale * trans->yres / 72.0 + 0.5);

    d = 100;
    j = -1;
    for(i = 0; i < face->num_fixed_sizes; i++) {
        x = face->available_sizes[i].width;
        y = face->available_sizes[i].height;
        if(ABS(x - tx) <= 1 && ABS(y - ty) <= 1) {
            dd = ABS(x - tx) * ABS(x - tx) + ABS(y - ty) * ABS(y - ty);
            if(dd < d) {
                j = i;
                d = dd;
            }
        }
    }
    if(j < 0)
        return BadFontName;

    *x_return = face->available_sizes[j].width;
    *y_return = face->available_sizes[j].height;
    return Successful;
}

static int
FreeTypeOpenInstance(FTInstancePtr *instance_return, FTFacePtr face,
                     char *FTFileName, FTNormalisedTransformationPtr trans,
                     int spacing, FontBitmapFormatPtr bmfmt,
		     struct TTCapInfo *tmp_ttcap, FT_Int32 load_flags)
{
    FT_Error ftrc;
    int xrc;
    FTInstancePtr instance, otherInstance;

    /* Search for a matching instance */
    for(otherInstance = face->instances;
        otherInstance;
        otherInstance = otherInstance->next) {
        if(FTInstanceMatch(otherInstance, FTFileName, trans, spacing, bmfmt,
			   tmp_ttcap, load_flags)) break;
    }
    if(otherInstance) {
        MUMBLE("Returning cached instance\n");
        otherInstance->refcount++;
        *instance_return = otherInstance;
        return Successful;
    }

    /* None matching found */
    instance = malloc(sizeof(FTInstanceRec));
    if(instance == NULL) {
        return AllocError;
    }

    instance->refcount = 1;
    instance->face = face;

    instance->load_flags = load_flags;
    instance->spacing    = spacing;		/* Actual spacing */
    instance->pixel_size =0;
    instance->pixel_width_unit_x =0;
    instance->pixel_width_unit_y =0;
    instance->charcellMetrics = NULL;
    instance->averageWidth = 0;
    instance->rawAverageWidth = 0;
    instance->forceConstantMetrics = NULL;

    instance->transformation = *trans;
    instance->bmfmt = *bmfmt;
    instance->glyphs = NULL;
    instance->available = NULL;

    if( 0 <= tmp_ttcap->forceConstantSpacingEnd )
	instance->nglyphs = 2 * instance->face->face->num_glyphs;
    else
	instance->nglyphs = instance->face->face->num_glyphs;

    /* Store the TTCap info. */
    memcpy((char*)&instance->ttcap, (char*)tmp_ttcap,
	   sizeof(struct TTCapInfo));

    ftrc = FT_New_Size(instance->face->face, &instance->size);
    if(ftrc != 0) {
        ErrorF("FreeType: couldn't create size object: %d\n", ftrc);
        free(instance);
        return FTtoXReturnCode(ftrc);
    }
    FreeTypeActivateInstance(instance);
    if(!face->bitmap) {
        ftrc = FT_Set_Char_Size(instance->face->face,
                                (int)(trans->scale*(1<<6) + 0.5),
                                (int)(trans->scale*(1<<6) + 0.5),
                                trans->xres, trans->yres);
    } else {
        int xsize, ysize;
        xrc = FTFindSize(face->face, trans, &xsize, &ysize);
        if(xrc != Successful) {
            free(instance);
            return xrc;
        }
        ftrc = FT_Set_Pixel_Sizes(instance->face->face, xsize, ysize);
    }
    if(ftrc != 0) {
        FT_Done_Size(instance->size);
        free(instance);
        return FTtoXReturnCode(ftrc);
    }

    if( FT_IS_SFNT( face->face ) ) {
#if 1
        FT_F26Dot6  tt_char_width, tt_char_height, tt_dim_x, tt_dim_y;
        FT_Int     nn;

        instance->strike_index=0xFFFFU;

        tt_char_width  = (FT_F26Dot6)(trans->scale*(1<<6) + 0.5);
        tt_char_height = (FT_F26Dot6)(trans->scale*(1<<6) + 0.5);

        tt_dim_x = FLOOR64( ( tt_char_width  * trans->xres + 36 ) / 72 + 32 );
        tt_dim_y = FLOOR64( ( tt_char_height * trans->yres + 36 ) / 72 + 32 );

	if ( tt_dim_x && !tt_dim_y )
	    tt_dim_y = tt_dim_x;
	else if ( !tt_dim_x && tt_dim_y )
	    tt_dim_x = tt_dim_y;

        for ( nn = 0; nn < face->face->num_fixed_sizes; nn++ )
        {
          FT_Bitmap_Size*  sz = &face->face->available_sizes[nn];

          if ( tt_dim_x == FLOOR64(sz->x_ppem + 32) && tt_dim_y == FLOOR64(sz->y_ppem + 32) )
          {
            instance->strike_index = nn;
            break;
          }
        }
#else
	/* See Set_Char_Sizes() in ttdriver.c */
	FT_Error err;
	TT_Face tt_face;
	FT_Long tt_dim_x, tt_dim_y;
	FT_UShort tt_x_ppem, tt_y_ppem;
	FT_F26Dot6  tt_char_width, tt_char_height;
	SFNT_Service sfnt;
	tt_face=(TT_Face)face->face;
	tt_char_width  = (int)(trans->scale*(1<<6) + 0.5);
	tt_char_height = (int)(trans->scale*(1<<6) + 0.5);
	if ( ( tt_face->header.Flags & 8 ) != 0 ) {
	    tt_dim_x = ( ( tt_char_width  * trans->xres + (36+32*72) ) / 72 ) & -64;
	    tt_dim_y = ( ( tt_char_height * trans->yres + (36+32*72) ) / 72 ) & -64;
	}
	else{
	    tt_dim_x = ( ( tt_char_width  * trans->xres + 36 ) / 72 );
	    tt_dim_y = ( ( tt_char_height * trans->yres + 36 ) / 72 );
	}
	tt_x_ppem  = (FT_UShort)( tt_dim_x >> 6 );
	tt_y_ppem  = (FT_UShort)( tt_dim_y >> 6 );
	/* See Reset_SBit_Size() in ttobjs.c */
	sfnt   = (SFNT_Service)tt_face->sfnt;
	err = sfnt->set_sbit_strike(tt_face,tt_x_ppem,tt_y_ppem,&instance->strike_index);
	if ( err ) instance->strike_index=0xFFFFU;
#endif
    }

    /* maintain a linked list of instances */
    instance->next = instance->face->instances;
    instance->face->instances = instance;

    *instance_return = instance;
    return Successful;
}

static void
FreeTypeFreeInstance(FTInstancePtr instance)
{
    FTInstancePtr otherInstance;

    if( instance == NULL ) return;

    if(instance->face->active_instance == instance)
        instance->face->active_instance = NULL;
    instance->refcount--;
    if(instance->refcount <= 0) {
        int i,j;

        if(instance->face->instances == instance)
            instance->face->instances = instance->next;
        else {
            for(otherInstance = instance->face->instances;
                otherInstance;
                otherInstance = otherInstance->next)
                if(otherInstance->next == instance) {
                    otherInstance->next = instance->next;
                    break;
                }
        }

        FT_Done_Size(instance->size);
        FreeTypeFreeFace(instance->face);

        if(instance->charcellMetrics) {
            free(instance->charcellMetrics);
        }
        if(instance->forceConstantMetrics) {
            free(instance->forceConstantMetrics);
        }
        if(instance->glyphs) {
            for(i = 0; i < iceil(instance->nglyphs, FONTSEGMENTSIZE); i++) {
                if(instance->glyphs[i]) {
                    for(j = 0; j < FONTSEGMENTSIZE; j++) {
                        if(instance->available[i][j] ==
                           FT_AVAILABLE_RASTERISED)
                            free(instance->glyphs[i][j].bits);
                    }
                    free(instance->glyphs[i]);
                }
            }
            free(instance->glyphs);
        }
        if(instance->available) {
            for(i = 0; i < iceil(instance->nglyphs, FONTSEGMENTSIZE); i++) {
                if(instance->available[i])
                    free(instance->available[i]);
            }
            free(instance->available);
        }
        free(instance);
    }
}

static int
FreeTypeInstanceFindGlyph(unsigned idx_in, int flags, FTInstancePtr instance,
                          CharInfoPtr **glyphs, int ***available,
                          int *found, int *segmentP, int *offsetP)
{
    int segment, offset;
    unsigned idx = idx_in;

    if( 0 <= instance->ttcap.forceConstantSpacingEnd ){
	if( (flags & FT_FORCE_CONSTANT_SPACING) )
	    idx += instance->nglyphs / 2 ;
    }

    if(idx > instance->nglyphs) {
        *found = 0;
        return Successful;
    }

    if(*available == NULL) {
        *available = calloc(iceil(instance->nglyphs, FONTSEGMENTSIZE),
			    sizeof(int *));
        if(*available == NULL)
            return AllocError;
    }

    segment = ifloor(idx, FONTSEGMENTSIZE);
    offset = idx - segment * FONTSEGMENTSIZE;

    if((*available)[segment] == NULL) {
        (*available)[segment] = calloc(FONTSEGMENTSIZE, sizeof(int));
        if((*available)[segment] == NULL)
            return AllocError;
    }

    if(*glyphs == NULL) {
        *glyphs = calloc(iceil(instance->nglyphs, FONTSEGMENTSIZE),
			 sizeof(CharInfoPtr));
        if(*glyphs == NULL)
            return AllocError;
    }

    if((*glyphs)[segment] == NULL) {
        (*glyphs)[segment] = mallocarray(sizeof(CharInfoRec), FONTSEGMENTSIZE);
        if((*glyphs)[segment] == NULL)
            return AllocError;
    }

    *found = 1;
    *segmentP = segment;
    *offsetP = offset;
    return Successful;
}

static int
FreeTypeInstanceGetGlyph(unsigned idx, int flags, CharInfoPtr *g, FTInstancePtr instance)
{
    int found, segment, offset;
    int xrc;
    int ***available;
    CharInfoPtr **glyphs;

    available = &instance->available;
    glyphs = &instance->glyphs;

    xrc = FreeTypeInstanceFindGlyph(idx, flags, instance, glyphs, available,
                                    &found, &segment, &offset);
    if(xrc != Successful)
        return xrc;

    if(!found || (*available)[segment][offset] == FT_AVAILABLE_NO) {
        *g = NULL;
        return Successful;
    }

    if((*available)[segment][offset] == FT_AVAILABLE_RASTERISED) {
	*g = &(*glyphs)[segment][offset];
	return Successful;
    }

    flags |= FT_GET_GLYPH_BOTH;

    xrc = FreeTypeRasteriseGlyph(idx, flags,
				 &(*glyphs)[segment][offset], instance,
				 (*available)[segment][offset] >= FT_AVAILABLE_METRICS);
    if(xrc != Successful && (*available)[segment][offset] >= FT_AVAILABLE_METRICS) {
	ErrorF("Warning: FreeTypeRasteriseGlyph() returns an error,\n");
	ErrorF("\tso the backend tries to set a white space.\n");
	xrc = FreeTypeRasteriseGlyph(idx, flags | FT_GET_DUMMY,
				     &(*glyphs)[segment][offset], instance,
				     (*available)[segment][offset] >= FT_AVAILABLE_METRICS);
    }
    if(xrc == Successful) {
        (*available)[segment][offset] = FT_AVAILABLE_RASTERISED;
	/* return the glyph */
        *g = &(*glyphs)[segment][offset];
    }
    return xrc;
}

static int
FreeTypeInstanceGetGlyphMetrics(unsigned idx, int flags,
				xCharInfo **metrics, FTInstancePtr instance )
{
    int xrc;
    int found, segment, offset;

    /* Char cell */
    if(instance->spacing == FT_CHARCELL) {
	*metrics = instance->charcellMetrics;
	return Successful;
    }
    /* Force constant metrics  */
    if( flags & FT_FORCE_CONSTANT_SPACING) {
	*metrics = instance->forceConstantMetrics;
	return Successful;
    }

    /* Not char cell */

    xrc = FreeTypeInstanceFindGlyph(idx, flags, instance,
                                    &instance->glyphs, &instance->available,
                                    &found, &segment, &offset);
    if(xrc != Successful)
        return xrc;
    if(!found) {
        *metrics = NULL;
        return Successful;
    }
    if( instance->available[segment][offset] == FT_AVAILABLE_NO ) {
        *metrics = NULL;
        return Successful;
    }

    if( instance->available[segment][offset] >= FT_AVAILABLE_METRICS ) {
	*metrics = &instance->glyphs[segment][offset].metrics;
	return Successful;
    }

    flags |= FT_GET_GLYPH_METRICS_ONLY;

    xrc = FreeTypeRasteriseGlyph(idx, flags,
				 &instance->glyphs[segment][offset],
				 instance, 0);
    if(xrc == Successful) {
        instance->available[segment][offset] = FT_AVAILABLE_METRICS;
        *metrics = &instance->glyphs[segment][offset].metrics;
    }
    return xrc;
}

/*
 * Pseudo enbolding similar as Microsoft Windows.
 * It is useful but poor.
 */
static void
ft_make_up_bold_bitmap( char *raster, int bpr, int ht, int ds_mode)
{
    int x, y;
    unsigned char *p = (unsigned char *)raster;
    if ( ds_mode & TTCAP_DOUBLE_STRIKE_MKBOLD_EDGE_LEFT ) {
        for (y=0; y<ht; y++) {
            unsigned char rev_pat=0;
            unsigned char lsb = 0;
            for (x=0; x<bpr; x++) {
                unsigned char tmp = *p<<7;
                if ( (rev_pat & 0x01) && (*p & 0x80) ) p[-1] &= 0xfe;
                rev_pat = ~(*p);
                *p |= (*p>>1) | lsb;
                *p &= ~(rev_pat & (*p << 1));
                lsb = tmp;
                p++;
            }
        }
    }
    else {
        for (y=0; y<ht; y++) {
            unsigned char lsb = 0;
            for (x=0; x<bpr; x++) {
                unsigned char tmp = *p<<7;
                *p |= (*p>>1) | lsb;
                lsb = tmp;
                p++;
            }
        }
    }
}

static void
ft_make_up_italic_bitmap( char *raster, int bpr, int ht, int shift,
			  int h_total, int h_offset, double a_italic)
{
    int x, y;
    unsigned char *p = (unsigned char *)raster;
    if ( a_italic < 0 ) shift = -shift;
    for (y=0; y<ht; y++) {
        unsigned char *tmp_p = p + y*bpr;
        int tmp_shift = shift * (h_total -1 -(y+h_offset)) / h_total;
        int tmp_byte_shift;
	if ( 0 <= tmp_shift ) {
	    tmp_byte_shift = tmp_shift/8;
	    tmp_shift %= 8;
	    if ( tmp_shift ) {
		for (x=bpr-1;0<=x;x--) {
		    if ( x != bpr-1 )
			tmp_p[x+1] |= tmp_p[x]<<(8-tmp_shift);
		    tmp_p[x]>>=tmp_shift;
		}
	    }
	    if ( tmp_byte_shift ) {
		for (x=bpr-1;0<x;x--) {
		    tmp_p[x] = tmp_p[x-1];
		}
		tmp_p[x]=0;
	    }
	}
	else {
	    tmp_shift = -tmp_shift;
	    tmp_byte_shift = tmp_shift/8;
	    tmp_shift %= 8;
	    if ( tmp_shift ) {
		for (x=0;x<bpr;x++) {
		    if ( x != 0 )
			tmp_p[x-1] |= tmp_p[x]>>(8-tmp_shift);
		    tmp_p[x]<<=tmp_shift;
		}
	    }
	    if ( tmp_byte_shift ) {
		for (x=0;x<bpr-1;x++) {
		    tmp_p[x] = tmp_p[x+1];
		}
		tmp_p[x]=0;
	    }
	}
    }
}

/*
 * The very lazy method,
 * parse the htmx field in TrueType font.
 */

static void
tt_get_metrics( FT_Face         face,
		FT_UInt         idx,
		FT_UInt         num_hmetrics,
		FT_Short*       bearing,
		FT_UShort*      advance )
{
   /* read the metrics directly from the horizontal header, we
    * parse the SFNT table directly through the standard FreeType API.
    * this works with any version of the library and doesn't need to
    * peek at its internals. Maybe a bit less
    */
    FT_UInt  count  = num_hmetrics;
    FT_ULong length = 0;
    FT_ULong offset = 0;
    FT_Error error;

    error = FT_Load_Sfnt_Table( face, TTAG_hmtx, 0, NULL, &length );

    if ( count == 0 || error )
    {
      *advance = 0;
      *bearing = 0;
    }
    else if ( idx < count )
    {
	offset = idx * 4L;
	if ( offset + 4 > length )
	{
	    *advance = 0;
	    *bearing = 0;
	}
	else
	{
	    *advance = sfnt_get_ushort( face, TTAG_hmtx, offset );
	    *bearing = sfnt_get_short ( face, TTAG_hmtx, offset+2 );
	}
    }
    else
    {
	offset = 4L * (count - 1);
	if ( offset + 4 > length )
	{
	    *advance = 0;
	    *bearing = 0;
	}
	else
	{
	    *advance = sfnt_get_ushort ( face, TTAG_hmtx, offset );
	    offset += 4 + 2 * ( idx - count );
	    if ( offset + 2 > length)
		*bearing = 0;
	    else
		*bearing = sfnt_get_short ( face, TTAG_hmtx, offset );
    }
    }
}

static int
ft_get_very_lazy_bbox( FT_UInt index,
		       FT_Face face,
		       FT_Size size,
		       FT_UInt num_hmetrics,
		       double slant,
		       FT_Matrix *matrix,
		       FT_BBox *bbox,
		       FT_Long *horiAdvance,
		       FT_Long *vertAdvance)
{
    if ( FT_IS_SFNT( face ) ) {
	FT_Size_Metrics *smetrics = &size->metrics;
	FT_Short  leftBearing = 0;
	FT_UShort advance = 0;
	FT_Vector p0, p1, p2, p3;

	/* horizontal */
	tt_get_metrics( face, index, num_hmetrics,
		       &leftBearing, &advance );

#if 0
	fprintf(stderr,"x_scale=%f y_scale=%f\n",
		(double)smetrics->x_scale,(double)smetrics->y_scale);
#endif
	bbox->xMax = *horiAdvance =
	    FT_MulFix( advance, smetrics->x_scale );
	bbox->xMin =
	    FT_MulFix( leftBearing, smetrics->x_scale );
	/* vertical */
	bbox->yMin = FT_MulFix( face->bbox.yMin,
				smetrics->y_scale );
	bbox->yMax = FT_MulFix( face->bbox.yMax,
				smetrics->y_scale );
	/* slant */
	if( 0 < slant ) {
	    bbox->xMax += slant * bbox->yMax;
	    bbox->xMin += slant * bbox->yMin;
	}
	else if( slant < 0 ) {
	    bbox->xMax += slant * bbox->yMin;
	    bbox->xMin += slant * bbox->yMax;
	}

	*vertAdvance = -1;	/* We don't support */

	p0.x = p2.x = bbox->xMin;
	p1.x = p3.x = bbox->xMax;
	p0.y = p1.y = bbox->yMin;
	p2.y = p3.y = bbox->yMax;

	FT_Vector_Transform(&p0, matrix);
	FT_Vector_Transform(&p1, matrix);
	FT_Vector_Transform(&p2, matrix);
	FT_Vector_Transform(&p3, matrix);

#if 0
	fprintf(stderr,
		"->(%.1f %.1f) (%.1f %.1f)"
		"  (%.1f %.1f) (%.1f %.1f)\n",
		p0.x / 64.0, p0.y / 64.0,
		p1.x / 64.0, p1.y / 64.0,
		p2.x / 64.0, p2.y / 64.0,
		p3.x / 64.0, p3.y / 64.0);
#endif
	bbox->xMin = MIN(p0.x, MIN(p1.x, MIN(p2.x, p3.x)));
	bbox->xMax = MAX(p0.x, MAX(p1.x, MAX(p2.x, p3.x)));
	bbox->yMin = MIN(p0.y, MIN(p1.y, MIN(p2.y, p3.y)));
	bbox->yMax = MAX(p0.y, MAX(p1.y, MAX(p2.y, p3.y)));
	return 0;	/* Successful */
    }
    return -1;
}

static FT_Error
FT_Do_SBit_Metrics( FT_Face ft_face, FT_Size ft_size, FT_ULong strike_index,
		    FT_UShort glyph_index, FT_Glyph_Metrics *metrics_return,
		    int *sbitchk_incomplete_but_exist )
{
#if 1
    if ( strike_index != 0xFFFFU && ft_face->available_sizes != NULL )
    {
      FT_Error         error;
      FT_Bitmap_Size*  sz = &ft_face->available_sizes[strike_index];

      error = FT_Set_Pixel_Sizes( ft_face, sz->x_ppem/64, sz->y_ppem/64 );
      if ( !error )
      {
        error = FT_Load_Glyph( ft_face, glyph_index, FT_LOAD_SBITS_ONLY );
        if ( !error )
        {
          if ( metrics_return != NULL )
            *metrics_return = ft_face->glyph->metrics;

          return 0;
        }
      }
    }
    return -1;
#elif (FREETYPE_VERSION >= 2001008)
    SFNT_Service       sfnt;
    TT_Face            face;
    FT_Error           error;
    FT_Stream          stream;
    TT_SBit_Strike     strike;
    TT_SBit_Range      range;
    TT_SBit_MetricsRec elem_metrics;
    FT_ULong           ebdt_pos;
    FT_ULong           glyph_offset;
    ;

    if ( ! FT_IS_SFNT( ft_face ) )
    {
        error=-1;
        goto Exit;
    }

    face = (TT_Face)ft_face;
    sfnt   = (SFNT_Service)face->sfnt;

    if (strike_index != 0xFFFFU && sfnt && sfnt->find_sbit_image &&
            sfnt->load_sbits) {
        /* Check whether there is a glyph sbit for the current index */
        error = sfnt->find_sbit_image( face, glyph_index, strike_index,
                                       &range, &strike, &glyph_offset );
    }
    else error=-1;
    if ( error ) goto Exit;

    if ( metrics_return == NULL ) goto Exit;

    stream = face->root.stream;

    /* now, find the location of the `EBDT' table in */
    /* the font file                                 */
    error = face->goto_table( face, TTAG_EBDT, stream, 0 );
    if ( error )
      error = face->goto_table( face, TTAG_bdat, stream, 0 );
    if (error)
      goto Exit;

    ebdt_pos = FT_STREAM_POS();

    /* place stream at beginning of glyph data and read metrics */
    if ( FT_STREAM_SEEK( ebdt_pos + glyph_offset ) )
      goto Exit;

    error = sfnt->load_sbit_metrics( stream, range, &elem_metrics );
    if ( error )
      goto Exit;

    metrics_return->width  = (FT_Pos)elem_metrics.width  << 6;
    metrics_return->height = (FT_Pos)elem_metrics.height << 6;

    metrics_return->horiBearingX = (FT_Pos)elem_metrics.horiBearingX << 6;
    metrics_return->horiBearingY = (FT_Pos)elem_metrics.horiBearingY << 6;
    metrics_return->horiAdvance  = (FT_Pos)elem_metrics.horiAdvance  << 6;

    metrics_return->vertBearingX = (FT_Pos)elem_metrics.vertBearingX << 6;
    metrics_return->vertBearingY = (FT_Pos)elem_metrics.vertBearingY << 6;
    metrics_return->vertAdvance  = (FT_Pos)elem_metrics.vertAdvance  << 6;

  Exit:
      return error;
#else	/* if (FREETYPE_VERSION < 2001008) */
    TT_Face            face;
    SFNT_Service       sfnt;
    if ( ! FT_IS_SFNT( ft_face ) ) return -1;
    face = (TT_Face)ft_face;
    sfnt = (SFNT_Service)face->sfnt;
    if ( strike_index != 0xFFFFU && sfnt->load_sbits ) {
        if ( sbitchk_incomplete_but_exist ) *sbitchk_incomplete_but_exist=1;
    }
    return -1;
#endif
}

#pragma GCC diagnostic ignored "-Wbad-function-cast"

int
FreeTypeRasteriseGlyph(unsigned idx, int flags, CharInfoPtr tgp,
		       FTInstancePtr instance, int hasMetrics)
{
    FTFacePtr face;
    FT_BBox bbox;
    FT_Long outline_hori_advance, outline_vert_advance;
    FT_Glyph_Metrics sbit_metrics;
    FT_Glyph_Metrics *bitmap_metrics=NULL, *metrics = NULL;
    char *raster;
    int wd, ht, bpr;            /* width, height, bytes per row */
    int wd_actual, ht_actual;
    int ftrc, is_outline, correct, b_shift=0;
    int dx, dy;
    int leftSideBearing, rightSideBearing, characterWidth, rawCharacterWidth,
        ascent, descent;
    int sbitchk_incomplete_but_exist;
    double bbox_center_raw;

    face = instance->face;

    FreeTypeActivateInstance(instance);

    if(!tgp) return AllocError;

    /*
     * PREPARE METRICS
     */

    if(!hasMetrics) {
	if( instance->spacing == FT_CHARCELL || flags & FT_GET_DUMMY ){
	    memcpy((char*)&tgp->metrics,
		   (char*)instance->charcellMetrics,
		   sizeof(xCharInfo));
	}
	else if( flags & FT_FORCE_CONSTANT_SPACING ) {
	    memcpy((char*)&tgp->metrics,
		   (char*)instance->forceConstantMetrics,
		   sizeof(xCharInfo));
	}
	/* mono or prop. */
	else{
	    int new_width;
	    double ratio;

	    sbitchk_incomplete_but_exist=0;
	    if( ! (instance->load_flags & FT_LOAD_NO_BITMAP) ) {
		if( FT_Do_SBit_Metrics(face->face,instance->size,instance->strike_index,
				       idx,&sbit_metrics,&sbitchk_incomplete_but_exist)==0 ) {
		    bitmap_metrics = &sbit_metrics;
		}
	    }
	    if( bitmap_metrics == NULL ) {
		if ( sbitchk_incomplete_but_exist==0 && (instance->ttcap.flags & TTCAP_IS_VERY_LAZY) ) {
		    if( ft_get_very_lazy_bbox( idx, face->face, instance->size,
					       face->num_hmetrics,
					       instance->ttcap.vl_slant,
					       &instance->transformation.matrix,
					       &bbox, &outline_hori_advance,
					       &outline_vert_advance ) == 0 ) {
			goto bbox_ok;	/* skip exact calculation */
		    }
		}
		ftrc = FT_Load_Glyph(instance->face->face, idx,
				     instance->load_flags);
		if(ftrc != 0) return FTtoXReturnCode(ftrc);
		metrics = &face->face->glyph->metrics;
		if( face->face->glyph->format == FT_GLYPH_FORMAT_BITMAP ) {
		    bitmap_metrics = metrics;
		}
	    }

	    if( bitmap_metrics ) {
		FT_Pos factor;

		leftSideBearing = bitmap_metrics->horiBearingX / 64;
		rightSideBearing = (bitmap_metrics->width + bitmap_metrics->horiBearingX) / 64;
		bbox_center_raw = (2.0 * bitmap_metrics->horiBearingX + bitmap_metrics->width)/2.0/64.0;
		characterWidth = (int)floor(bitmap_metrics->horiAdvance
					    * instance->ttcap.scaleBBoxWidth / 64.0 + .5);
		ascent = bitmap_metrics->horiBearingY / 64;
		descent = (bitmap_metrics->height - bitmap_metrics->horiBearingY) / 64 ;
		/* */
		new_width = characterWidth;
		if( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH )
		    new_width += instance->ttcap.doubleStrikeShift;
		new_width += instance->ttcap.adjustBBoxWidthByPixel;
		ratio = (double)new_width/characterWidth;
		characterWidth = new_width;
		/* adjustment by pixel unit */
		if( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE )
		    rightSideBearing += instance->ttcap.doubleStrikeShift;
		rightSideBearing += instance->ttcap.adjustRightSideBearingByPixel;
		leftSideBearing  += instance->ttcap.adjustLeftSideBearingByPixel;
		rightSideBearing += instance->ttcap.rsbShiftOfBitmapAutoItalic;
		leftSideBearing  += instance->ttcap.lsbShiftOfBitmapAutoItalic;
		/* */
		factor = bitmap_metrics->horiAdvance;
		rawCharacterWidth = (unsigned short)(short)(floor(1000 * factor
						  * instance->ttcap.scaleBBoxWidth * ratio / 64.
						  / instance->pixel_size));
	    }
	    else {
		/* Outline */
#ifdef USE_GET_CBOX
		/* Very fast?? */
		FT_Outline_Get_CBox(&face->face->glyph->outline, &bbox);
		ftrc=0;		/* FT_Outline_Get_CBox returns nothing. */
#else
		/* Calculate exact metrics */
		ftrc=FT_Outline_Get_BBox(&face->face->glyph->outline, &bbox);
#endif
		if( ftrc != 0 ) return FTtoXReturnCode(ftrc);
		outline_hori_advance = metrics->horiAdvance;
		outline_vert_advance = metrics->vertAdvance;
	    bbox_ok:
		descent  = CEIL64(-bbox.yMin - 32) / 64;
		leftSideBearing  = FLOOR64(bbox.xMin + 32) / 64;
		ascent   = FLOOR64(bbox.yMax + 32) / 64;
		rightSideBearing = FLOOR64(bbox.xMax + 32) / 64;
		bbox_center_raw = (double)(bbox.xMax + bbox.xMin)/2.0/64.;
		if ( instance->pixel_width_unit_x != 0 )
		    characterWidth =
			(int)floor( outline_hori_advance
				    * instance->ttcap.scaleBBoxWidth
				    * instance->pixel_width_unit_x / 64. + .5);
		else {
		    characterWidth =
			(int)floor( outline_vert_advance
				    * instance->ttcap.scaleBBoxHeight
				    * instance->pixel_width_unit_y / 64. + .5);
		    if(characterWidth <= 0)
			characterWidth = instance->charcellMetrics->characterWidth;
		}
		/* */
		new_width = characterWidth;
		if( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH )
		    new_width += instance->ttcap.doubleStrikeShift;
		new_width += instance->ttcap.adjustBBoxWidthByPixel;
		ratio = (double)new_width/characterWidth;
		characterWidth = new_width;
		if ( instance->pixel_width_unit_x != 0 )
		    rawCharacterWidth =
			(unsigned short)(short)(floor(1000 * outline_hori_advance
						      * instance->ttcap.scaleBBoxWidth * ratio
						      * instance->pixel_width_unit_x / 64.));
		else {
		    rawCharacterWidth =
			(unsigned short)(short)(floor(1000 * outline_vert_advance
						      * instance->ttcap.scaleBBoxHeight * ratio
						      * instance->pixel_width_unit_y / 64.));
		    if(rawCharacterWidth <= 0)
			rawCharacterWidth = instance->charcellMetrics->attributes;
		}
		/* adjustment by pixel unit */
		if( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE )
		    rightSideBearing += instance->ttcap.doubleStrikeShift;
		rightSideBearing += instance->ttcap.adjustRightSideBearingByPixel;
		leftSideBearing  += instance->ttcap.adjustLeftSideBearingByPixel;
	    }

	    /* Set the glyph metrics. */
	    tgp->metrics.attributes = (unsigned short)((short)rawCharacterWidth);
	    tgp->metrics.leftSideBearing = leftSideBearing;
	    tgp->metrics.rightSideBearing = rightSideBearing;
	    tgp->metrics.characterWidth = characterWidth;
	    tgp->metrics.ascent = ascent;
	    tgp->metrics.descent = descent;
	    /* Update the width to match the width of the font */
	    if( instance->spacing != FT_PROPORTIONAL )
		tgp->metrics.characterWidth = instance->charcellMetrics->characterWidth;
	    if(instance->ttcap.flags & TTCAP_MONO_CENTER){
		b_shift   = (int)floor((instance->advance/2.0-bbox_center_raw) + .5);
		tgp->metrics.leftSideBearing  += b_shift;
		tgp->metrics.rightSideBearing += b_shift;
	    }
	}
    }

    if( flags & FT_GET_GLYPH_METRICS_ONLY ) return Successful;

    /*
     * CHECK THE NECESSITY OF BITMAP POSITION'S CORRECTION
     */

    correct=0;
    if( instance->spacing == FT_CHARCELL ) correct=1;
    else if( flags & FT_FORCE_CONSTANT_SPACING ) correct=1;
    else{
	int sbit_available=0;
	sbitchk_incomplete_but_exist=0;
	if( !(instance->load_flags & FT_LOAD_NO_BITMAP) ) {
	    if( FT_Do_SBit_Metrics(face->face,instance->size,
				   instance->strike_index,idx,NULL,
				   &sbitchk_incomplete_but_exist)==0 ) {
		sbit_available=1;
	    }
	}
	if( sbit_available == 0 ) {
	    if ( sbitchk_incomplete_but_exist==0 && (instance->ttcap.flags & TTCAP_IS_VERY_LAZY) ) {
		if( FT_IS_SFNT(face->face) ) correct=1;
	    }
	}
    }

    /*
     * RENDER AND ALLOCATE BUFFER
     */

    if( flags & FT_GET_DUMMY ) is_outline = -1;
    else {
	if( !metrics ) {
	    ftrc = FT_Load_Glyph(instance->face->face, idx,
				 instance->load_flags);
	    metrics = &face->face->glyph->metrics;

	    if(ftrc != 0) return FTtoXReturnCode(ftrc);
	}

	if( face->face->glyph->format != FT_GLYPH_FORMAT_BITMAP ) {
#ifdef USE_GET_CBOX
	    FT_Outline_Get_CBox(&face->face->glyph->outline, &bbox);
	    ftrc = 0;
#else
	    ftrc = FT_Outline_Get_BBox(&face->face->glyph->outline, &bbox);
#endif
	    if( ftrc != 0 ) return FTtoXReturnCode(ftrc);
	    bbox.yMin = FLOOR64( bbox.yMin );
	    bbox.yMax = CEIL64 ( bbox.yMax );
	    ht_actual = ( bbox.yMax - bbox.yMin ) >> 6;
	    /* FreeType think a glyph with 0 height control box is invalid.
	     * So just let X to create a empty bitmap instead. */
	    if ( ht_actual == 0 )
		is_outline = -1;
	    else
	    {
	    ftrc = FT_Render_Glyph(face->face->glyph,FT_RENDER_MODE_MONO);
	    if( ftrc != 0 ) return FTtoXReturnCode(ftrc);
	    is_outline = 1;
	}
	}
	else{
	    is_outline=0;
	}
    }

    /* Spacial case */
    if( (instance->ttcap.flags & TTCAP_MONO_CENTER) && hasMetrics ) {
	if( is_outline == 1 ){
	    if( correct ){
		if( ft_get_very_lazy_bbox( idx, face->face, instance->size,
					   face->num_hmetrics,
					   instance->ttcap.vl_slant,
					   &instance->transformation.matrix,
					   &bbox, &outline_hori_advance,
					   &outline_vert_advance ) != 0 ){
		    is_outline = -1;	/* <- error */
		}
	    }
	    else {
#ifdef USE_GET_CBOX
		FT_Outline_Get_CBox(&face->face->glyph->outline, &bbox);
		ftrc=0;
#else
		ftrc=FT_Outline_Get_BBox(&face->face->glyph->outline, &bbox);
#endif
		if( ftrc != 0 ) return FTtoXReturnCode(ftrc);
	    }
	    bbox_center_raw = (double)(bbox.xMax + bbox.xMin)/2.0/64.;
	}
	else if( is_outline == 0 )
	    bbox_center_raw = (2.0 * metrics->horiBearingX + metrics->width)/2.0/64.0;
	else
	    bbox_center_raw = 0;
	b_shift = (int)floor((instance->advance/2.0-bbox_center_raw) + .5);
    }

    wd_actual = tgp->metrics.rightSideBearing - tgp->metrics.leftSideBearing;
    ht_actual = tgp->metrics.ascent + tgp->metrics.descent;

    /* The X convention is to consider a character with an empty
     * bounding box as undefined.  This convention is broken. */

    if(wd_actual <= 0) wd = 1;
    else wd=wd_actual;
    if(ht_actual <= 0) ht = 1;
    else ht=ht_actual;

    bpr = (((wd + (instance->bmfmt.glyph<<3) - 1) >> 3) &
           -instance->bmfmt.glyph);
    raster = calloc(1, ht * bpr);
    if(raster == NULL)
	return AllocError;

    tgp->bits = raster;

    /* If FT_GET_DUMMY is set, we return white space. */
    if ( is_outline == -1 ) return Successful;

    if ( wd_actual <= 0 || ht_actual <= 0 ) return Successful;

    /*
     * CALCULATE OFFSET, dx AND dy.
     */

    dx = face->face->glyph->bitmap_left - tgp->metrics.leftSideBearing;
    dy = tgp->metrics.ascent - face->face->glyph->bitmap_top;

    if(instance->ttcap.flags & TTCAP_MONO_CENTER)
	dx += b_shift;

    /* To prevent chipped bitmap, we correct dx and dy if needed. */
    if( correct && is_outline==1 ){
	int lsb, rsb, asc, des;
	int chip_left,chip_right,chip_top,chip_bot;
#ifdef USE_GET_CBOX
	FT_Outline_Get_CBox(&face->face->glyph->outline, &bbox);
	ftrc=0;
#else
	ftrc=FT_Outline_Get_BBox(&face->face->glyph->outline, &bbox);
#endif
	if( ftrc != 0 ) return FTtoXReturnCode(ftrc);
	des = CEIL64(-bbox.yMin - 32) / 64;
	lsb = FLOOR64(bbox.xMin + 32) / 64;
	asc = FLOOR64(bbox.yMax + 32) / 64;
	rsb = FLOOR64(bbox.xMax + 32) / 64;
	rightSideBearing = tgp->metrics.rightSideBearing;
	leftSideBearing  = tgp->metrics.leftSideBearing;
	if( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE )
	    rightSideBearing -= instance->ttcap.doubleStrikeShift;
	/* special case */
	if(instance->ttcap.flags & TTCAP_MONO_CENTER){
	    leftSideBearing  -= b_shift;
	    rightSideBearing -= b_shift;
	}
	chip_left  = lsb - leftSideBearing;
	chip_right = rightSideBearing - rsb;
	if( flags & FT_FORCE_CONSTANT_SPACING ){
	    if( instance->ttcap.force_c_adjust_lsb_by_pixel != 0 ||
		instance->ttcap.force_c_adjust_rsb_by_pixel != 0 ){
		chip_left=0;
		chip_right=0;
	    }
	}
	else{
	    if( instance->ttcap.adjustRightSideBearingByPixel != 0 ||
		instance->ttcap.adjustLeftSideBearingByPixel != 0 ){
		chip_left=0;
		chip_right=0;
	    }
	}
	chip_top   = tgp->metrics.ascent - asc;
	chip_bot   = tgp->metrics.descent - des;
	if( chip_left < 0 && 0 < chip_right ) dx++;
	else if( chip_right < 0 && 0 < chip_left ) dx--;
	if( chip_top < 0 && 0 < chip_bot ) dy++;
	else if( chip_bot < 0 && 0 < chip_top ) dy--;
    }

    /*
     * COPY RASTER
     */

    {
	FT_Bitmap *bitmap;
	int i, j;
	unsigned char *current_raster;
	unsigned char *current_buffer;
	int mod_dx0,mod_dx1;
	int div_dx;
	bitmap = &face->face->glyph->bitmap;
	if( 0 <= dx ){
	    div_dx = dx / 8;
	    mod_dx0 = dx % 8;
	    mod_dx1 = 8-mod_dx0;
	}
	else{
	    div_dx = dx / 8 -1;
	    mod_dx1 = -dx % 8;
	    mod_dx0 = 8-mod_dx1;
	}
	for( i = MAX(0, dy) ; i<ht ; i++ ){
	    int prev_jj,jj;
	    if( bitmap->rows <= (unsigned) (i-dy) ) break;
	    current_buffer=(unsigned char *)(bitmap->buffer+bitmap->pitch*(i-dy));
	    current_raster=(unsigned char *)(raster+i*bpr);
	    j       = MAX(0,div_dx);
	    jj      = j-div_dx;
	    prev_jj = jj-1;
	    if( j<bpr ){
		if( 0 <= prev_jj && prev_jj < bitmap->pitch )
		    current_raster[j]|=current_buffer[prev_jj]<<mod_dx1;
		if( 0 <= jj && jj < bitmap->pitch ){
		    current_raster[j]|=current_buffer[jj]>>mod_dx0;
		    j++; prev_jj++; jj++;
		    for( ; j<bpr ; j++,prev_jj++,jj++ ){
			current_raster[j]|=current_buffer[prev_jj]<<mod_dx1;
			if( bitmap->pitch <= jj ) break;
			current_raster[j]|=current_buffer[jj]>>mod_dx0;
		    }
		}
	    }
	}
    }

    /* by TTCap */
    if ( instance->ttcap.flags & TTCAP_DOUBLE_STRIKE ) {
	int i;
	for( i=0 ; i < instance->ttcap.doubleStrikeShift ; i++ )
	    ft_make_up_bold_bitmap( raster, bpr, ht, instance->ttcap.flags);
    }
    if ( is_outline == 0 &&
	 ( instance->ttcap.lsbShiftOfBitmapAutoItalic != 0 ||
	   instance->ttcap.rsbShiftOfBitmapAutoItalic != 0 ) ) {
	ft_make_up_italic_bitmap( raster, bpr, ht,
				  - instance->ttcap.lsbShiftOfBitmapAutoItalic
				  + instance->ttcap.rsbShiftOfBitmapAutoItalic,
				  instance->charcellMetrics->ascent
				  + instance->charcellMetrics->descent,
				  instance->charcellMetrics->ascent
				  - tgp->metrics.ascent,
				  instance->ttcap.autoItalic);
    }

    if(instance->bmfmt.bit == LSBFirst) {
        BitOrderInvert((unsigned char*)(tgp->bits), ht*bpr);
    }

    if(instance->bmfmt.byte != instance->bmfmt.bit) {
        switch(instance->bmfmt.scan) {
        case 1:
            break;
        case 2:
            TwoByteSwap((unsigned char*)(tgp->bits), ht*bpr);
            break;
        case 4:
            FourByteSwap((unsigned char*)(tgp->bits), ht*bpr);
            break;
        default:
            ;
        }
    }

    return Successful;
}

static void
FreeTypeFreeFont(FTFontPtr font)
{
    FreeTypeFreeInstance(font->instance);
    if(font->ranges)
        free(font->ranges);
    if(font->dummy_char.bits)
	free(font->dummy_char.bits);
    free(font);
}

/* Free a font.  If freeProps is 0, don't free the properties. */

static void
FreeTypeFreeXFont(FontPtr pFont, int freeProps)
{
    FTFontPtr tf;

    if(pFont) {
        if((tf = (FTFontPtr)pFont->fontPrivate)) {
            FreeTypeFreeFont(tf);
        }
        if(freeProps && pFont->info.nprops>0) {
            free(pFont->info.isStringProp);
            free(pFont->info.props);
        }
        DestroyFontRec(pFont);
    }
}


/* Unload a font */

static void
FreeTypeUnloadXFont(FontPtr pFont)
{
    MUMBLE("Unloading\n");
    FreeTypeFreeXFont(pFont, 1);
}

/* Add the font properties, including the Font name, the XLFD
   properties, some strings from the font, and various typographical
   data.  We only provide data readily available in the tables in the
   font for now, although FIGURE_WIDTH would be a good idea as it is
   used by Xaw. */

static int
FreeTypeAddProperties(FTFontPtr font, FontScalablePtr vals, FontInfoPtr info,
                      char *fontname, int rawAverageWidth, Bool font_properties)
{
    int i, j, maxprops;
    char *sp, *ep, val[MAXFONTNAMELEN], *vp;
    FTFacePtr face;
    FTInstancePtr instance;
    FTNormalisedTransformationPtr trans;
    int upm;
    TT_OS2 *os2;
    TT_Postscript *post;
    PS_FontInfoRec t1info_rec, *t1info;
    int xlfdProps = 0;
    int ftrc;

    instance = font->instance;
    face = instance->face;
    trans = &instance->transformation;
    upm = face->face->units_per_EM;
    if(upm == 0) {
        /* Work around FreeType bug */
        upm = WORK_AROUND_UPM;
    }

    os2 = FT_Get_Sfnt_Table(face->face, ft_sfnt_os2);
    post = FT_Get_Sfnt_Table(face->face, ft_sfnt_post);
    ftrc = FT_Get_PS_Font_Info(face->face, &t1info_rec);
    if(ftrc == 0)
        t1info = &t1info_rec;
    else
        t1info = NULL;

    if(t1info) {
        os2 = NULL;
        post = NULL;
    }

    info->nprops = 0;           /* in case we abort */

    strlcpy(val, fontname, sizeof(val));
    if(FontParseXLFDName(val, vals, FONT_XLFD_REPLACE_VALUE)) {
        xlfdProps = 1;
    } else {
        MUMBLE("Couldn't parse XLFD\n");
        xlfdProps = 0;
    }

    maxprops=
        1 +                     /* NAME */
        (xlfdProps ? 14 : 0) +  /* from XLFD */
        5 +
        ( !face->bitmap ? 3 : 0 ) +	/* raw_av,raw_asc,raw_dec */
        ( font_properties ? 2 : 0 ) +	/* asc,dec */
        ( (font_properties && os2) ? 6 : 0 ) +
        ( (font_properties && (post || t1info)) ? 3 : 0 ) +
        2;                      /* type */

    info->props = mallocarray(maxprops, sizeof(FontPropRec));
    if(info->props == NULL)
        return AllocError;

    info->isStringProp = malloc(maxprops);
    if(info->isStringProp == NULL) {
        free(info->props);
        return AllocError;
    }

    memset((char *)info->isStringProp, 0, maxprops);

    i = 0;

    info->props[i].name = MakeAtom("FONT", 4, TRUE);
    info->props[i].value = MakeAtom(val, strlen(val), TRUE);
    info->isStringProp[i] = 1;
    i++;

    if(*val && *(sp = val + 1)) {
        for (j = 0, sp = val + 1; j < 14; j++) {
            if (j == 13)
                /* Handle the case of the final field containing a subset
                   specification. */
                for (ep = sp; *ep && *ep != '['; ep++);
            else
                for (ep = sp; *ep && *ep != '-'; ep++);

            info->props[i].name =
                MakeAtom(xlfd_props[j], strlen(xlfd_props[j]), TRUE);

            switch(j) {
            case 6:                   /* pixel size */
                info->props[i].value =
                    (int)(fabs(vals->pixel_matrix[3]) + 0.5);
                i++;
                break;
            case 7:                   /* point size */
                info->props[i].value =
                    (int)(fabs(vals->point_matrix[3])*10.0 + 0.5);
                i++;
                break;
            case 8:                   /* resolution x */
                info->props[i].value = vals->x;
                i++;
                break;
            case 9:                   /* resolution y */
                info->props[i].value = vals->y;
                i++;
                break;
            case 11:                  /* average width */
                info->props[i].value = vals->width;
                i++;
                break;
            default:                  /* a string */
                info->props[i].value = MakeAtom(sp, ep - sp, TRUE);
                info->isStringProp[i] = 1;
                i++;
            }
            sp = ++ep;
        }
    }

    info->props[i].name = MakeAtom("RAW_PIXEL_SIZE", 14, TRUE);
    info->props[i].value = 1000;
    i++;

    info->props[i].name = MakeAtom("RAW_POINT_SIZE", 14, TRUE);
    info->props[i].value = (long)(72270.0 / (double)vals->y + .5);
    i++;

    if(!face->bitmap) {
        info->props[i].name = MakeAtom("RAW_AVERAGE_WIDTH", 17, TRUE);
        info->props[i].value = rawAverageWidth;
        i++;
    }

    if ( font_properties ) {
	info->props[i].name = MakeAtom("FONT_ASCENT", 11, TRUE);
	info->props[i].value = info->fontAscent;
	i++;
    }

    if(!face->bitmap) {
        info->props[i].name = MakeAtom("RAW_ASCENT", 10, TRUE);
        info->props[i].value =
            ((double)face->face->ascender/(double)upm*1000.0);
        i++;
    }

    if ( font_properties ) {
	info->props[i].name = MakeAtom("FONT_DESCENT", 12, TRUE);
	info->props[i].value = info->fontDescent;
	i++;
    }

    if(!face->bitmap) {
        info->props[i].name = MakeAtom("RAW_DESCENT", 11, TRUE);
        info->props[i].value =
            -((double)face->face->descender/(double)upm*1000.0);
        i++;
    }

    j = FTGetEnglishName(face->face, TT_NAME_ID_COPYRIGHT,
                         val, MAXFONTNAMELEN);
    vp = val;
    if (j < 0) {
        if(t1info && t1info->notice) {
            vp = t1info->notice;
            j = strlen(vp);
        }
    }
    if(j > 0) {
        info->props[i].name = MakeAtom("COPYRIGHT", 9, TRUE);
        info->props[i].value = MakeAtom(vp, j, TRUE);
        info->isStringProp[i] = 1;
        i++;
    }

    j = FTGetEnglishName(face->face, TT_NAME_ID_FULL_NAME,
                         val, MAXFONTNAMELEN);
    vp = val;
    if (j < 0) {
        if(t1info && t1info->full_name) {
            vp = t1info->full_name;
            j = strlen(vp);
        }
    }
    if(j > 0) {
        info->props[i].name = MakeAtom("FACE_NAME", 9, TRUE);
        info->props[i].value = MakeAtom(vp, j, TRUE);
        info->isStringProp[i] = 1;
        i++;
    }

    vp = (char *)FT_Get_Postscript_Name(face->face);
    if (vp) {
	j = strlen(vp);
    } else {
	j = -1;
    }
    if (j < 0) {
	j = FTGetEnglishName(face->face, TT_NAME_ID_PS_NAME,
                         val, MAXFONTNAMELEN);
	vp = val;
    }
    if (j < 0) {
        if(t1info && t1info->full_name) {
            vp = t1info->full_name;
            j = strlen(vp);
        }
    }
    if(j > 0) {
        info->props[i].name = MakeAtom("_ADOBE_POSTSCRIPT_FONTNAME", 26, TRUE);
        info->props[i].value = MakeAtom(vp, j, TRUE);
        info->isStringProp[i] = 1;
        i++;
    }

  /* These macros handle the case of a diagonal matrix.  They convert
     FUnits into pixels. */
#define TRANSFORM_FUNITS_X(xval) \
  ((int) \
   floor( ((double)(xval)/(double)upm) * (double)vals->pixel_matrix[0] + 0.5 ) )

#define TRANSFORM_FUNITS_Y(yval) \
  ((int) \
   floor( ((double)(yval)/(double)upm) * (double)vals->pixel_matrix[3] + 0.5 ) )

  /* In what follows, we assume the matrix is diagonal.  In the rare
     case when it is not, the values will be somewhat wrong. */

    if( font_properties && os2 ) {
        info->props[i].name = MakeAtom("SUBSCRIPT_SIZE",14,TRUE);
        info->props[i].value =
            TRANSFORM_FUNITS_Y(os2->ySubscriptYSize);
        i++;
        info->props[i].name = MakeAtom("SUBSCRIPT_X",11,TRUE);
        info->props[i].value =
            TRANSFORM_FUNITS_X(os2->ySubscriptXOffset);
        i++;
        info->props[i].name = MakeAtom("SUBSCRIPT_Y",11,TRUE);
        info->props[i].value =
            TRANSFORM_FUNITS_Y(os2->ySubscriptYOffset);
        i++;
        info->props[i].name = MakeAtom("SUPERSCRIPT_SIZE",16,TRUE);
        info->props[i].value =
            TRANSFORM_FUNITS_Y(os2->ySuperscriptYSize);
        i++;
        info->props[i].name = MakeAtom("SUPERSCRIPT_X",13,TRUE);
        info->props[i].value =
            TRANSFORM_FUNITS_X(os2->ySuperscriptXOffset);
        i++;
        info->props[i].name = MakeAtom("SUPERSCRIPT_Y",13,TRUE);
        info->props[i].value =
        TRANSFORM_FUNITS_Y(os2->ySuperscriptYOffset);
        i++;
    }

    if( font_properties && (post || t1info) ) {
        int underlinePosition, underlineThickness;

	/* Raw underlineposition counts upwards,
	   but UNDERLINE_POSITION counts downwards. */
        if(post) {
            underlinePosition = TRANSFORM_FUNITS_Y(-post->underlinePosition);
            underlineThickness = TRANSFORM_FUNITS_Y(post->underlineThickness);
        } else {
            underlinePosition =
                TRANSFORM_FUNITS_Y(-t1info->underline_position);
            underlineThickness =
                TRANSFORM_FUNITS_Y(t1info->underline_thickness);
        }
        if(underlineThickness <= 0)
            underlineThickness = 1;

        info->props[i].name = MakeAtom("UNDERLINE_THICKNESS",19,TRUE);
        info->props[i].value = underlineThickness;
        i++;

        info->props[i].name = MakeAtom("UNDERLINE_POSITION",18,TRUE);

        info->props[i].value = underlinePosition;

        i++;

        /* The italic angle is often unreliable for Type 1 fonts */
        if(post && trans->matrix.xx == trans->matrix.yy) {
            info->props[i].name = MakeAtom("ITALIC_ANGLE",12,TRUE);
            info->props[i].value =
                /* Convert from TT_Fixed to
                   64th of a degree counterclockwise from 3 o'clock */
                90*64+(post->italicAngle >> 10);
            i++;
        }
#undef TRANSFORM_FUNITS_X
#undef TRANSFORM_FUNITS_Y
    }

    info->props[i].name  = MakeAtom("FONT_TYPE", 9, TRUE);
    vp = (char *)FT_Get_X11_Font_Format(face->face);
    info->props[i].value = MakeAtom(vp, strlen(vp), TRUE);
    info->isStringProp[i] = 1;
    i++;

    info->props[i].name  = MakeAtom("RASTERIZER_NAME", 15, TRUE);
    info->props[i].value = MakeAtom("FreeType", 8, TRUE);
    info->isStringProp[i] = 1;
    i++;

    info->nprops = i;
    return Successful;
}

static int
ft_get_index(unsigned code, FTFontPtr font, unsigned *idx)
{

    /* As a special case, we pass 0 even when it is not in the ranges;
       this will allow for the default glyph, which should exist in any
       TrueType font. */

    /* This is not required...
    if(code > 0 && font->nranges) {
        int i;
        for(i = 0; i < font->nranges; i++)
            if((code >=
                font->ranges[i].min_char_low+
                (font->ranges[i].min_char_high<<8)) &&
               (code <=
                font->ranges[i].max_char_low +
                (font->ranges[i].max_char_high<<8)))
                break;
        if(i == font->nranges) {
	    *idx = font->zero_idx;
            return -1;
        }
    }
    */
    if( font->info ) {
	if( !( font->info->firstCol <= (code & 0x000ff) &&
	       (code & 0x000ff) <= font->info->lastCol &&
	       font->info->firstRow <= (code >> 8) &&
	       (code >> 8) <= font->info->lastRow ) ) {
	    *idx = font->zero_idx;
	    /* Error: The code has not been parsed in ft_compute_bounds()!
	       We should not return any metrics. */
	    return -1;
	}
    }

    *idx = FTRemap(font->instance->face->face, &font->mapping, code);

    return 0;
}

static int
FreeTypeFontGetGlyph(unsigned code, int flags, CharInfoPtr *g, FTFontPtr font)
{
    unsigned idx = 0;
    int xrc;

#ifdef X_ACCEPTS_NO_SUCH_CHAR
    if( ft_get_index(code,font,&idx) || idx == 0 || idx == font->zero_idx ) {
	*g = NULL;
	flags &= ~FT_FORCE_CONSTANT_SPACING;
	/* if( font->instance->spacing != FT_CHARCELL ) */
	return Successful;
    }
#else
    if( ft_get_index(code,font,&idx) ) {
	/* The code has not been parsed! */
	*g = NULL;
	flags &= ~FT_FORCE_CONSTANT_SPACING;
    }
#endif

    xrc = FreeTypeInstanceGetGlyph(idx, flags, g, font->instance);
    if( xrc == Successful && *g != NULL )
	return Successful;
    if( font->zero_idx != idx ) {
	xrc = FreeTypeInstanceGetGlyph(font->zero_idx, flags, g, font->instance);
	if( xrc == Successful && *g != NULL )
	    return Successful;
    }
    return FreeTypeInstanceGetGlyph(font->zero_idx, flags|FT_GET_DUMMY, g, font->instance);
}

static int
FreeTypeFontGetGlyphMetrics(unsigned code, int flags, xCharInfo **metrics, FTFontPtr font)
{
    unsigned idx = 0;
    int xrc;

#ifdef X_ACCEPTS_NO_SUCH_CHAR
    if ( ft_get_index(code,font,&idx) || idx == 0 || idx == font->zero_idx ) {
	*metrics = NULL;
	flags &= ~FT_FORCE_CONSTANT_SPACING;
	/* if( font->instance->spacing != FT_CHARCELL ) */
	return Successful;
    }
#else
    if ( ft_get_index(code,font,&idx) || idx == 0 || idx == font->zero_idx ) {
	/* The code has not been parsed! */
	*metrics = NULL;
	flags &= ~FT_FORCE_CONSTANT_SPACING;
    }
#endif

    xrc = FreeTypeInstanceGetGlyphMetrics(idx, flags, metrics, font->instance);
    if( xrc == Successful && *metrics != NULL )
	return Successful;
    if( font->zero_idx != idx ) {
	xrc = FreeTypeInstanceGetGlyphMetrics(font->zero_idx, flags,
					      metrics, font->instance);
	if( xrc == Successful && *metrics != NULL )
	    return Successful;
    }
    return FreeTypeInstanceGetGlyphMetrics(font->zero_idx, flags|FT_GET_DUMMY, metrics, font->instance);
}

/*
 * restrict code range
 *
 * boolean for the numeric zone:
 *   results = results & (ranges[0] | ranges[1] | ... ranges[nranges-1])
 */

static void
restrict_code_range(unsigned short *refFirstCol,
                    unsigned short *refFirstRow,
                    unsigned short *refLastCol,
                    unsigned short *refLastRow,
                    fsRange const *ranges, int nRanges)
{
    if (nRanges) {
        int minCol = 256, minRow = 256, maxCol = -1, maxRow = -1;
        fsRange const *r = ranges;
        int i;

        for (i=0; i<nRanges; i++) {
            if (r->min_char_high != r->max_char_high) {
                minCol = 0x00;
                maxCol = 0xff;
            } else {
                if (minCol > r->min_char_low)
                    minCol = r->min_char_low;
                if (maxCol < r->max_char_low)
                    maxCol = r->max_char_low;
            }
            if (minRow > r->min_char_high)
                minRow = r->min_char_high;
            if (maxRow < r->max_char_high)
                maxRow = r->max_char_high;
            r++;
        }

        if (minCol > *refLastCol)
            *refFirstCol = *refLastCol;
        else if (minCol > *refFirstCol)
            *refFirstCol = minCol;

        if (maxCol < *refFirstCol)
            *refLastCol = *refFirstCol;
        else if (maxCol < *refLastCol)
            *refLastCol = maxCol;

        if (minRow > *refLastRow) {
            *refFirstRow = *refLastRow;
            *refFirstCol = *refLastCol;
        } else if (minRow > *refFirstRow)
            *refFirstRow = minRow;

        if (maxRow < *refFirstRow) {
            *refLastRow = *refFirstRow;
            *refLastCol = *refFirstCol;
        } else if (maxRow < *refLastRow)
            *refLastRow = maxRow;
    }
}


static int
restrict_code_range_by_str(int count,unsigned short *refFirstCol,
			   unsigned short *refFirstRow,
			   unsigned short *refLastCol,
			   unsigned short *refLastRow,
			   char const *str)
{
    int nRanges = 0;
    int result = 0;
    fsRange *ranges = NULL, *oldRanges;
    char const *p, *q;

    p = q = str;
    for (;;) {
        int minpoint=0, maxpoint=65535;
        long val;

        /* skip comma and/or space */
        while (',' == *p || isspace((unsigned char)*p))
            p++;

        /* begin point */
        if ('-' != *p) {
            val = strtol(p, (char **)&q, 0);
            if (p == q)
                /* end or illegal */
                break;
            if (val<0 || val>65535) {
                /* out of zone */
                break;
            }
            minpoint = val;
            p=q;
        }

        /* skip space */
        while (isspace((unsigned char)*p))
            p++;

        if (',' != *p && '\0' != *p) {
            /* contiune */
            if ('-' == *p)
                /* hyphon */
                p++;
            else
                /* end or illegal */
                break;

            /* skip space */
            while (isspace((unsigned char)*p))
                p++;

            val = strtol(p, (char **)&q, 0);
            if (p != q) {
                if (val<0 || val>65535)
                    break;
                maxpoint = val;
            } else if (',' != *p && '\0' != *p)
                /* end or illegal */
                break;
            p=q;
        } else
            /* comma - single code */
            maxpoint = minpoint;

        if ( count <= 0 && minpoint>maxpoint ) {
            int tmp;
            tmp = minpoint;
            minpoint = maxpoint;
            maxpoint = tmp;
        }

        /* add range */
#if 0
        fprintf(stderr, "zone: 0x%04X - 0x%04X\n", minpoint, maxpoint);
        fflush(stderr);
#endif
        nRanges++;
        oldRanges = ranges;
        ranges = reallocarray(ranges, nRanges, sizeof(*ranges));
        if (NULL == ranges) {
            free(oldRanges);
            break;
        }
        else {
            fsRange *r = ranges+nRanges-1;

            r->min_char_low = minpoint & 0xff;
            r->max_char_low = maxpoint & 0xff;
            r->min_char_high = (minpoint>>8) & 0xff;
            r->max_char_high = (maxpoint>>8) & 0xff;
        }
    }

    if (ranges) {
        if ( count <= 0 ) {
            restrict_code_range(refFirstCol, refFirstRow, refLastCol, refLastRow,
                                ranges, nRanges);
        }
        else {
            int i;
            fsRange *r;
            for ( i=0 ; i<nRanges ; i++ ) {
                if ( count <= i ) break;
                r = ranges+i;
                refFirstCol[i] = r->min_char_low;
                refLastCol[i] = r->max_char_low;
                refFirstRow[i] = r->min_char_high;
                refLastRow[i] = r->max_char_high;
            }
            result=i;
        }
        free(ranges);
    }
    return result;
}

/* *face_number and *spacing are initialized but *load_flags is NOT. */
static int
FreeTypeSetUpTTCap( char *fileName, FontScalablePtr vals,
		    char **dynStrRealFileName, char **dynStrFTFileName,
		    struct TTCapInfo *ret, int *face_number, FT_Int32 *load_flags,
		    int *spacing, Bool *font_properties, char **dynStrTTCapCodeRange )
{
    int result = Successful;
    SDynPropRecValList listPropRecVal;
    SPropRecValContainer contRecValue;
    Bool hinting=True;
    Bool isEmbeddedBitmap = True;
    Bool alwaysEmbeddedBitmap = False;
    int pixel = vals->pixel;

    *font_properties=True;
    *dynStrRealFileName=NULL;
    *dynStrFTFileName=NULL;
    *dynStrTTCapCodeRange=NULL;

    if (SPropRecValList_new(&listPropRecVal)) {
        return AllocError;
    }

    {
        int len = strlen(fileName);
        char *capHead = NULL;
        {
            /* font cap */
            char *p1=NULL, *p2=NULL;

	    p1=strrchr(fileName, '/');
	    if ( p1 == NULL ) p1 = fileName;
	    else p1++;
	    if (NULL != (p2=strrchr(p1, ':'))) {
		/* colon exist in the right side of slash. */
		int dirLen = p1-fileName;
		int baseLen = fileName+len - p2 -1;
		int fullLen = dirLen+baseLen+1;

		*dynStrRealFileName = malloc(fullLen);
		if( *dynStrRealFileName == NULL ) {
		    result = AllocError;
		    goto quit;
		}
		if ( 0 < dirLen )
		    memcpy(*dynStrRealFileName, fileName, dirLen);
		strlcpy(*dynStrRealFileName+dirLen, p2+1, fullLen - dirLen);
		capHead = p1;
	    } else {
		*dynStrRealFileName = strdup(fileName);
		if( *dynStrRealFileName == NULL ) {
		    result = AllocError;
		    goto quit;
		}
	    }
        }

	/* font cap */
	if (capHead) {
	    if (SPropRecValList_add_by_font_cap(&listPropRecVal,
						capHead)) {
		result = BadFontPath;
		goto quit;
	    }
	}
    }

    *face_number=0;
    *spacing=0;
    ret->autoItalic=0.0;
    ret->scaleWidth=1.0;
    ret->scaleBBoxWidth = 1.0;
    ret->scaleBBoxHeight = 1.0;
    ret->doubleStrikeShift = 1;
    ret->adjustBBoxWidthByPixel = 0;
    ret->adjustLeftSideBearingByPixel = 0;
    ret->adjustRightSideBearingByPixel = 0;
    ret->flags = 0;
    ret->scaleBitmap = 0.0;
    ret->forceConstantSpacingBegin = -1;
    ret->forceConstantSpacingEnd = -1;
    ret->force_c_representative_metrics_char_code = -2;
    ret->force_c_scale_b_box_width = 1.0;
    ret->force_c_scale_b_box_height = 1.0;
    ret->force_c_adjust_width_by_pixel = 0;
    ret->force_c_adjust_lsb_by_pixel = 0;
    ret->force_c_adjust_rsb_by_pixel = 0;
    ret->force_c_scale_lsb = 0.0;
    ret->force_c_scale_rsb = 1.0;
    /* */
    ret->vl_slant=0;
    ret->lsbShiftOfBitmapAutoItalic=0;
    ret->rsbShiftOfBitmapAutoItalic=0;
    /* face number */
    {
	char *beginptr=NULL,*endptr;
	if ( SPropRecValList_search_record(&listPropRecVal,
                                       &contRecValue,
                                       "FaceNumber")) {
	    int lv;
	    beginptr = SPropContainer_value_str(contRecValue);
	    lv=strtol(beginptr, &endptr, 10);
	    if ( *beginptr != '\0' && *endptr == '\0' ) {
		if ( 0 < lv ) *face_number = lv;
	    }
	}
	if( beginptr && 0 < *face_number ) {
	    char *slash;
	    size_t dsftlen = 	/* add ->  ':'+strlen0+':'+strlen1+'\0' */
		1 + strlen(beginptr) + 1 + strlen(*dynStrRealFileName) + 1;
	    *dynStrFTFileName =	malloc(dsftlen);
	    if( *dynStrFTFileName == NULL ){
		result = AllocError;
		goto quit;
	    }
	    **dynStrFTFileName = '\0';
	    slash = strrchr(*dynStrRealFileName,'/');
	    if( slash ) {
		char *p;
		strlcat(*dynStrFTFileName, *dynStrRealFileName, dsftlen);
		p = strrchr(*dynStrFTFileName,'/');
		p[1] = '\0';
		strlcat(*dynStrFTFileName, ":", dsftlen);
		strlcat(*dynStrFTFileName, beginptr, dsftlen);
		strlcat(*dynStrFTFileName, ":", dsftlen);
		strlcat(*dynStrFTFileName, slash+1, dsftlen);
	    }
	    else{
		strlcat(*dynStrFTFileName, ":", dsftlen);
		strlcat(*dynStrFTFileName, beginptr, dsftlen);
		strlcat(*dynStrFTFileName, ":", dsftlen);
		strlcat(*dynStrFTFileName, *dynStrRealFileName, dsftlen);
	    }
	}
	else{
	    *dynStrFTFileName = strdup(*dynStrRealFileName);
	    if( *dynStrFTFileName == NULL ){
		result = AllocError;
		goto quit;
	    }
	}
    }
    /*
    fprintf(stderr,"[Filename:%s]\n",fileName);
    fprintf(stderr,"[RealFilename:%s]\n",*dynStrRealFileName);
    fprintf(stderr,"[FTFilename:%s]\n",*dynStrFTFileName);
    */
    /* slant control */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "AutoItalic"))
        ret->autoItalic = SPropContainer_value_dbl(contRecValue);
    /* hinting control */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "Hinting"))
        hinting = SPropContainer_value_bool(contRecValue);
    /* scaling */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "ScaleWidth")) {
        ret->scaleWidth = SPropContainer_value_dbl(contRecValue);
        if (ret->scaleWidth<=0.0) {
            fprintf(stderr, "ScaleWitdh needs plus.\n");
	    result = BadFontName;
	    goto quit;
	}
    }
    /* bbox adjustment */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "ScaleBBoxWidth")) {
        /* Scaling to Bounding Box Width */
        int lv;
        char *endptr,*beginptr;
        double v,scaleBBoxWidth=1.0,scaleBBoxHeight=1.0;
        beginptr = SPropContainer_value_str(contRecValue);
        do {
            if ( strlen(beginptr) < 1 ) break;
            v=strtod(beginptr, &endptr);
            if ( endptr!=beginptr ) {
                scaleBBoxWidth = v;
            }
            if ( *endptr != ';' && *endptr != ',' ) break;
	    if ( *endptr == ',' ) {
		beginptr=endptr+1;
		v=strtod(beginptr, &endptr);
		if ( endptr!=beginptr ) {
		    scaleBBoxHeight = v;
		}
	    }
            if ( *endptr != ';' && *endptr != ',' ) break;
            beginptr=endptr+1;
            lv=strtol(beginptr, &endptr, 10);
            if ( endptr!=beginptr ) {
                ret->adjustBBoxWidthByPixel = lv;
            }
            if ( *endptr != ',' ) break;
            beginptr=endptr+1;
            lv=strtol(beginptr, &endptr, 10);
            if ( endptr!=beginptr ) {
                ret->adjustLeftSideBearingByPixel = lv;
            }
            if ( *endptr != ',' ) break;
            beginptr=endptr+1;
            lv=strtol(beginptr, &endptr, 10);
            if ( endptr!=beginptr ) {
                ret->adjustRightSideBearingByPixel = lv;
            }
        } while ( 0 );
        if (scaleBBoxWidth<=0.0) {
            fprintf(stderr, "ScaleBBoxWitdh needs plus.\n");
	    result = BadFontName;
	    goto quit;
        }
        if (scaleBBoxHeight<=0.0) {
            fprintf(stderr, "ScaleBBoxHeight needs plus.\n");
	    result = BadFontName;
	    goto quit;
        }
        ret->scaleBBoxWidth  = scaleBBoxWidth;
        ret->scaleBBoxHeight = scaleBBoxHeight;
    }
    /* spacing */
    if (SPropRecValList_search_record(&listPropRecVal,
				      &contRecValue,
				      "ForceSpacing")) {
	char *strSpace = SPropContainer_value_str(contRecValue);
	Bool err = False;
	if (1 != strlen(strSpace))
	    err = True;
	else
	    switch (strSpace[0]) {
	    case 'M':
		ret->flags |= TTCAP_MONO_CENTER;
		*spacing = 'm';
		break;
	    case 'm':
	    case 'p':
	    case 'c':
		*spacing = strSpace[0];
		break;
	    default:
		err = True;
	    }
	if (err) {
	    result = BadFontName;
	    goto quit;
	}
    }
    /* double striking */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "DoubleStrike")) {
        /* Set or Reset Auto Bold Flag */
        char *strDoubleStrike = SPropContainer_value_str(contRecValue);
        Bool err = False;
        if ( 0 < strlen(strDoubleStrike) ) {
            switch (strDoubleStrike[0]) {
            case 'm':
            case 'M':
            case 'l':
            case 'L':
                ret->flags |= TTCAP_DOUBLE_STRIKE;
                ret->flags |= TTCAP_DOUBLE_STRIKE_MKBOLD_EDGE_LEFT;
                break;
            case 'y':
            case 'Y':
                ret->flags |= TTCAP_DOUBLE_STRIKE;
                break;
            case 'n':
            case 'N':
                ret->flags &= ~TTCAP_DOUBLE_STRIKE;
                ret->flags &= ~TTCAP_DOUBLE_STRIKE_MKBOLD_EDGE_LEFT;
                ret->flags &= ~TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH;
                break;
            default:
                err = True;
            }
            if ( err != True ) {
                if ( strDoubleStrike[1] ) {
                    switch (strDoubleStrike[1]) {
                    case 'b':
                    case 'B':
                    case 'p':
                    case 'P':
                    case 'y':
                    case 'Y':
                        ret->flags |= TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH;
                        break;
                    default:
                        break;
                    }
                }
                do {
                    char *comma_ptr=strchr(strDoubleStrike,';');
                    if ( !comma_ptr ) comma_ptr=strchr(strDoubleStrike,',');
                    if ( !comma_ptr ) break;
		    if ( comma_ptr[1] ) {
			char *endptr;
			int mkboldMaxPixel;
			mkboldMaxPixel=strtol(comma_ptr+1, &endptr, 10);
			if ( endptr != comma_ptr+1 && mkboldMaxPixel <= pixel ) {
			    ret->flags &= ~TTCAP_DOUBLE_STRIKE_MKBOLD_EDGE_LEFT;
			}
		    }
		    comma_ptr=strchr(comma_ptr+1,',');
		    if ( !comma_ptr ) break;
		    if ( comma_ptr[1] ) {
			char *endptr;
			int max_pixel;
			max_pixel=strtol(comma_ptr+1, &endptr, 10);
			if ( endptr != comma_ptr+1 && max_pixel <= pixel ) {
			  if( ret->flags & TTCAP_DOUBLE_STRIKE )
			    ret->doubleStrikeShift += pixel / max_pixel;
			}
		    }
                } while(0);
            }
        }
        else
            err = True;
        if (err) {
            result = BadFontName;
            goto quit;
        }
    }
    /* very lazy metrics */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "VeryLazyMetrics")){
	Bool isVeryLazy = SPropContainer_value_bool(contRecValue);
	ret->flags |= TTCAP_DISABLE_DEFAULT_VERY_LAZY;
	if( isVeryLazy == True )
	    ret->flags |= TTCAP_IS_VERY_LAZY;
	else
	    ret->flags &= ~TTCAP_IS_VERY_LAZY;
    }
    /* embedded bitmap */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "EmbeddedBitmap")) {
        char *strEmbeddedBitmap = SPropContainer_value_str(contRecValue);
        Bool err = False;
        if ( 1 == strlen(strEmbeddedBitmap) ) {
            switch (strEmbeddedBitmap[0]) {
            case 'y':
            case 'Y':
                isEmbeddedBitmap = True;
                alwaysEmbeddedBitmap = True;
                break;
            case 'u':
            case 'U':
                isEmbeddedBitmap = True;
                alwaysEmbeddedBitmap = False;
                break;
            case 'n':
            case 'N':
                isEmbeddedBitmap = False;
                break;
            default:
                err = True;
            }
        }
        else
            err = True;
        if (err) {
            result = BadFontName;
            goto quit;
        }
    }
    /* scale bitmap */
    if((ret->flags & TTCAP_IS_VERY_LAZY) &&
       SPropRecValList_search_record(&listPropRecVal,
                                     &contRecValue,
                                     "VeryLazyBitmapWidthScale")) {
        /* Scaling to Bitmap Bounding Box Width */
        double scaleBitmapWidth = SPropContainer_value_dbl(contRecValue);

	fprintf(stderr, "Warning: `bs' option is not required in X-TT version 2.\n");
#if 0
        if (scaleBitmapWidth<=0.0) {
            fprintf(stderr, "ScaleBitmapWitdh needs plus.\n");
            result = BadFontName;
            goto quit;
        }
#endif
        ret->scaleBitmap = scaleBitmapWidth;
    }
    /* restriction of the code range */
    if (SPropRecValList_search_record(&listPropRecVal,
				      &contRecValue,
				      "CodeRange")) {
	*dynStrTTCapCodeRange = strdup(SPropContainer_value_str(contRecValue));
	if( *dynStrTTCapCodeRange == NULL ) {
	    result = AllocError;
	    goto quit;
	}
    }
    /* forceConstantSpacing{Begin,End} */
    if ( 1 /* ft->spacing == 'p' */ ){
        unsigned short first_col=0,last_col=0x00ff;
        unsigned short first_row=0,last_row=0x00ff;
        if (SPropRecValList_search_record(&listPropRecVal,
                                           &contRecValue,
                                           "ForceConstantSpacingCodeRange")) {
            if ( restrict_code_range_by_str(1,&first_col, &first_row,
                                            &last_col, &last_row,
                                            SPropContainer_value_str(contRecValue)) == 1 ) {
              ret->forceConstantSpacingBegin = (int)( first_row<<8 | first_col );
              ret->forceConstantSpacingEnd = (int)( last_row<<8 | last_col );
	      if ( ret->forceConstantSpacingBegin <= ret->forceConstantSpacingEnd )
		  ret->flags &= ~TTCAP_FORCE_C_OUTSIDE;
	      else ret->flags |= TTCAP_FORCE_C_OUTSIDE;
            }
        }
    }
    /* */
    if ( 1 ){
        unsigned short first_col=0, last_col=0x0ff;
        unsigned short first_row=0, last_row=0x0ff;
        if ( SPropRecValList_search_record(&listPropRecVal,
                                           &contRecValue,
                                           "ForceConstantSpacingMetrics")) {
            char *strMetrics;
            strMetrics = SPropContainer_value_str(contRecValue);
            if ( strMetrics ) {
                char *comma_ptr,*period_ptr,*semic_ptr;
                semic_ptr=strchr(strMetrics,';');
                comma_ptr=strchr(strMetrics,',');
                period_ptr=strchr(strMetrics,'.');
                if ( semic_ptr && comma_ptr )
                    if ( semic_ptr < comma_ptr ) comma_ptr=NULL;
                if ( semic_ptr && period_ptr )
                    if ( semic_ptr < period_ptr ) period_ptr=NULL;
                if ( !comma_ptr && !period_ptr && strMetrics != semic_ptr ) {
                    if ( restrict_code_range_by_str(1,&first_col, &first_row,
                                                    &last_col, &last_row,
                                                    SPropContainer_value_str(contRecValue)) == 1 ) {
                      ret->force_c_representative_metrics_char_code =
                          (int)( first_row<<8 | first_col );
                    }
                }
                else {
                    double v;
                    char *endptr,*beginptr=strMetrics;
                    do {
                        v=strtod(beginptr, &endptr);
                        if ( endptr!=beginptr ) {
                            ret->force_c_scale_b_box_width = v;
                        }
                        if ( *endptr != ',' ) break;
                        beginptr=endptr+1;
                        v=strtod(beginptr, &endptr);
                        if ( endptr!=beginptr ) {
                            ret->force_c_scale_lsb = v;
                            ret->flags |= TTCAP_FORCE_C_LSB_FLAG;
                        }
                        if ( *endptr != ',' ) break;
                        beginptr=endptr+1;
                        v=strtod(beginptr, &endptr);
                        if ( endptr!=beginptr ) {
                            ret->force_c_scale_rsb = v;
                            ret->flags |= TTCAP_FORCE_C_RSB_FLAG;
                        }
                        if ( *endptr != ',' ) break;
                        beginptr=endptr+1;
                        v=strtod(beginptr, &endptr);
                        if ( endptr!=beginptr ) {
                            ret->force_c_scale_b_box_height = v;
			}
                    } while (0);
                }
                if ( semic_ptr ) {
                    int lv;
                    char *endptr,*beginptr=semic_ptr+1;
                    do {
                        lv=strtol(beginptr, &endptr, 10);
                        if ( endptr!=beginptr ) {
                            ret->force_c_adjust_width_by_pixel=lv;
                        }
                        if ( *endptr != ',' ) break;
                        beginptr=endptr+1;
                        lv=strtol(beginptr, &endptr, 10);
                        if ( endptr!=beginptr ) {
                            ret->force_c_adjust_lsb_by_pixel=lv;
                        }
                        if ( *endptr != ',' ) break;
                        beginptr=endptr+1;
                        lv=strtol(beginptr, &endptr, 10);
                        if ( endptr!=beginptr ) {
                            ret->force_c_adjust_rsb_by_pixel=lv;
                        }
                    } while (0);
                }
            }
        }
    }

    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "FontProperties")) {
        /* Set or Reset the Flag of FontProperties */
        *font_properties=SPropContainer_value_bool(contRecValue);
    }

    ret->force_c_scale_b_box_width *= ret->scaleBBoxWidth;
    ret->force_c_scale_b_box_height *= ret->scaleBBoxHeight;

    ret->force_c_scale_b_box_width *= ret->scaleWidth;
    ret->scaleBBoxWidth            *= ret->scaleWidth;

    ret->force_c_adjust_rsb_by_pixel += ret->adjustRightSideBearingByPixel;
    ret->force_c_adjust_lsb_by_pixel += ret->adjustLeftSideBearingByPixel;

    /* scaleWidth, scaleBBoxWidth, force_c_scale_b_box_width, force_c_scale_b_box_width */

    /* by TTCap */
    if( hinting == False ) *load_flags |= FT_LOAD_NO_HINTING;
    if( isEmbeddedBitmap == False ) *load_flags |= FT_LOAD_NO_BITMAP;
    if( ret->autoItalic != 0 && alwaysEmbeddedBitmap == False )
	*load_flags |= FT_LOAD_NO_BITMAP;

 quit:
    return result;
}

static int
ft_get_trans_from_vals( FontScalablePtr vals, FTNormalisedTransformationPtr trans )
{
    /* Compute the transformation matrix.  We use floating-point
       arithmetic for simplicity */

    trans->xres = vals->x;
    trans->yres = vals->y;

    /* This value cannot be 0. */
    trans->scale = hypot(vals->point_matrix[2], vals->point_matrix[3]);
    trans->nonIdentity = 0;

    /* Try to round stuff.  We want approximate zeros to be exact zeros,
       and if the elements on the diagonal are approximately equal, we
       want them equal.  We do this to avoid breaking hinting. */
    if(DIFFER(vals->point_matrix[0], vals->point_matrix[3])) {
        trans->nonIdentity = 1;
        trans->matrix.xx =
            (int)((vals->point_matrix[0]*(double)TWO_SIXTEENTH)/trans->scale);
        trans->matrix.yy =
            (int)((vals->point_matrix[3]*(double)TWO_SIXTEENTH)/trans->scale);
    } else {
        trans->matrix.xx = trans->matrix.yy =
            ((vals->point_matrix[0] + vals->point_matrix[3])/2*
             (double)TWO_SIXTEENTH)/trans->scale;
    }

    if(DIFFER0(vals->point_matrix[1], trans->scale)) {
        trans->matrix.yx =
            (int)((vals->point_matrix[1]*(double)TWO_SIXTEENTH)/trans->scale);
        trans->nonIdentity = 1;
    } else
        trans->matrix.yx = 0;

    if(DIFFER0(vals->point_matrix[2], trans->scale)) {
        trans->matrix.xy =
            (int)((vals->point_matrix[2]*(double)TWO_SIXTEENTH)/trans->scale);
        trans->nonIdentity = 1;
    } else
        trans->matrix.xy=0;
    return 0;
}


static int
is_fixed_width(FT_Face face)
{
    PS_FontInfoRec t1info_rec;
    int ftrc;

    if(FT_IS_FIXED_WIDTH(face)) {
        return 1;
    }

    ftrc = FT_Get_PS_Font_Info(face, &t1info_rec);
    if(ftrc == 0 && t1info_rec.is_fixed_pitch) {
        return 1;
    }

    return 0;
}

static int
FreeTypeLoadFont(FTFontPtr font, FontInfoPtr info, FTFacePtr face,
		 char *FTFileName, FontScalablePtr vals, FontEntryPtr entry,
                 FontBitmapFormatPtr bmfmt, FT_Int32 load_flags,
		 struct TTCapInfo *tmp_ttcap, char *dynStrTTCapCodeRange,
		 int ttcap_spacing )
{
    int xrc;
    FTNormalisedTransformationRec trans;
    int spacing, actual_spacing, zero_code;
    long  lastCode, firstCode;
    TT_Postscript *post;

    ft_get_trans_from_vals(vals,&trans);

    /* Check for charcell in XLFD */
    spacing = FT_PROPORTIONAL;
    if(entry->name.ndashes == 14) {
        char *p;
        int dashes = 0;
        for(p = entry->name.name;
            p <= entry->name.name + entry->name.length - 2;
            p++) {
            if(*p == '-') {
                dashes++;
                if(dashes == 11) {
                    if(p[1]=='c' && p[2]=='-')
                        spacing=FT_CHARCELL;
		    else if(p[1]=='m' && p[2]=='-')
                        spacing=FT_MONOSPACED;
                    break;
                }
            }
        }
    }
    /* by TTCap  */
    if( ttcap_spacing != 0 ) {
	if( ttcap_spacing == 'c' ) spacing=FT_CHARCELL;
	else if( ttcap_spacing == 'm' ) spacing=FT_MONOSPACED;
	else spacing=FT_PROPORTIONAL;
    }

    actual_spacing = spacing;
    if( spacing == FT_PROPORTIONAL ) {
	if( is_fixed_width(face->face) )
	    actual_spacing = FT_MONOSPACED;
    }

    if(entry->name.ndashes == 14) {
        xrc = FTPickMapping(entry->name.name, entry->name.length, FTFileName,
			    face->face, &font->mapping);
	if (xrc != Successful)
	    return xrc;
    } else {
        xrc = FTPickMapping(0, 0, FTFileName,
			    face->face, &font->mapping);
	if (xrc != Successful)
	    return xrc;
    }

    font->nranges = vals->nranges;
    font->ranges = 0;
    if(font->nranges) {
        font->ranges = mallocarray(vals->nranges, sizeof(fsRange));
        if(font->ranges == NULL)
            return AllocError;
        memcpy((char*)font->ranges, (char*)vals->ranges,
               vals->nranges*sizeof(fsRange));
    }

    zero_code=-1;
    if(info) {
        firstCode = 0;
        lastCode = 0xFFFFL;
        if(!font->mapping.mapping ||
           font->mapping.mapping->encoding->row_size == 0) {
            /* linear indexing */
            lastCode=MIN(lastCode,
                         font->mapping.mapping ?
                         font->mapping.mapping->encoding->size-1 :
                         0xFF);
            if(font->mapping.mapping && font->mapping.mapping->encoding->first)
                firstCode = font->mapping.mapping->encoding->first;
            info->firstRow = firstCode/0x100;
            info->lastRow = lastCode/0x100;
            info->firstCol =
                (info->firstRow || info->lastRow) ? 0 : (firstCode & 0xFF);
            info->lastCol = info->lastRow ? 0xFF : (lastCode & 0xFF);
	    if ( firstCode == 0 ) zero_code=0;
        } else {
            /* matrix indexing */
            info->firstRow = font->mapping.mapping->encoding->first;
            info->lastRow = MIN(font->mapping.mapping->encoding->size-1,
                                lastCode/0x100);
            info->firstCol = font->mapping.mapping->encoding->first_col;
            info->lastCol = MIN(font->mapping.mapping->encoding->row_size-1,
                                lastCode<0x100?lastCode:0xFF);
	    if( info->firstRow == 0 && info->firstCol == 0 ) zero_code=0;
        }

        /* firstCode and lastCode are not valid in case of a matrix
           encoding */

	if( dynStrTTCapCodeRange ) {
	    restrict_code_range_by_str(0,&info->firstCol, &info->firstRow,
				       &info->lastCol, &info->lastRow,
				       dynStrTTCapCodeRange);
	}
	restrict_code_range(&info->firstCol, &info->firstRow,
			    &info->lastCol, &info->lastRow,
			    font->ranges, font->nranges);
    }
    font->info = info;

    /* zero code is frequently used. */
    if ( zero_code < 0 ) {
	/* The fontenc should have the information of DefaultCh.
	   But we do not have such a information.
	   So we cannot but set 0. */
	font->zero_idx = 0;
    }
    else
	font->zero_idx = FTRemap(face->face,
				 &font->mapping, zero_code);

    post = FT_Get_Sfnt_Table(face->face, ft_sfnt_post);

#ifdef DEFAULT_VERY_LAZY
    if( !( tmp_ttcap->flags & TTCAP_DISABLE_DEFAULT_VERY_LAZY ) )
	if( DEFAULT_VERY_LAZY <= 1 + info->lastRow - info->firstRow ) {
	    if( post ){
		tmp_ttcap->flags |= TTCAP_IS_VERY_LAZY;
	    }
	}
#endif
    /* We should always reset. */
    tmp_ttcap->flags &= ~TTCAP_DISABLE_DEFAULT_VERY_LAZY;

    if ( face->bitmap || actual_spacing == FT_CHARCELL )
	tmp_ttcap->flags &= ~TTCAP_IS_VERY_LAZY;
    /* "vl=y" is available when TrueType or OpenType only */
    if ( !face->bitmap && !(FT_IS_SFNT( face->face )) )
	tmp_ttcap->flags &= ~TTCAP_IS_VERY_LAZY;

    if( post ) {
	if( post->italicAngle != 0 )
	    tmp_ttcap->vl_slant = -sin( (post->italicAngle/1024./5760.)*1.57079632679489661923 );
	/* fprintf(stderr,"angle=%g(%g)\n",tmp_ttcap->vl_slant,(post->italicAngle/1024./5760.)*90); */
    }

    xrc = FreeTypeOpenInstance(&font->instance, face,
                               FTFileName, &trans, actual_spacing, bmfmt,
			       tmp_ttcap, load_flags );
    return xrc;
}

static void
adjust_min_max(xCharInfo *minc, xCharInfo *maxc, xCharInfo *tmp)
{
#define MINMAX(field,ci) \
    if (minc->field > (ci)->field) \
    minc->field = (ci)->field; \
    if (maxc->field < (ci)->field) \
    maxc->field = (ci)->field;

    MINMAX(ascent, tmp);
    MINMAX(descent, tmp);
    MINMAX(leftSideBearing, tmp);
    MINMAX(rightSideBearing, tmp);
    MINMAX(characterWidth, tmp);

    if ((INT16)minc->attributes > (INT16)tmp->attributes)
        minc->attributes = tmp->attributes;
    if ((INT16)maxc->attributes < (INT16)tmp->attributes)
        maxc->attributes = tmp->attributes;
#undef  MINMAX
}

static void
ft_compute_bounds(FTFontPtr font, FontInfoPtr pinfo, FontScalablePtr vals )
{
    FTInstancePtr instance;
    int row, col;
    unsigned int c;
    xCharInfo minchar, maxchar, *tmpchar = NULL;
    int overlap, maxOverlap;
    long swidth      = 0;
    long total_width = 0;
    int num_cols, num_chars = 0;
    int flags, skip_ok = 0;
    int force_c_outside ;

    instance = font->instance;
    force_c_outside = instance->ttcap.flags & TTCAP_FORCE_C_OUTSIDE;

    minchar.ascent = minchar.descent =
    minchar.leftSideBearing = minchar.rightSideBearing =
    minchar.characterWidth = minchar.attributes = 32767;
    maxchar.ascent = maxchar.descent =
    maxchar.leftSideBearing = maxchar.rightSideBearing =
    maxchar.characterWidth = maxchar.attributes = -32767;
    maxOverlap = -32767;

    /* Parse all glyphs */
    num_cols = 1 + pinfo->lastCol - pinfo->firstCol;
    for (row = pinfo->firstRow; row <= pinfo->lastRow; row++) {
      if ( skip_ok && tmpchar ) {
        if ( !force_c_outside ) {
          if ( instance->ttcap.forceConstantSpacingBegin < row<<8
	       && row<<8 < (instance->ttcap.forceConstantSpacingEnd & 0x0ff00) ) {
            if (tmpchar->characterWidth) {
              num_chars += num_cols;
              swidth += ABS(tmpchar->characterWidth)*num_cols;
              total_width += tmpchar->characterWidth*num_cols;
              continue;
            }
          }
          else skip_ok=0;
        }
        else {          /* for GB18030 proportional */
          if ( instance->ttcap.forceConstantSpacingBegin < row<<8
	       || row<<8 < (instance->ttcap.forceConstantSpacingEnd & 0x0ff00) ) {
            if (tmpchar->characterWidth) {
              num_chars += num_cols;
              swidth += ABS(tmpchar->characterWidth)*num_cols;
              total_width += tmpchar->characterWidth*num_cols;
              continue;
            }
          }
          else skip_ok=0;
        }
      }
      for (col = pinfo->firstCol; col <= pinfo->lastCol; col++) {
          c = row<<8|col;
          flags=0;
          if ( !force_c_outside ) {
	      if ( (signed) c <= instance->ttcap.forceConstantSpacingEnd
		   && instance->ttcap.forceConstantSpacingBegin <= (signed) c )
                  flags|=FT_FORCE_CONSTANT_SPACING;
          }
          else {        /* for GB18030 proportional */
              if ( (signed) c <= instance->ttcap.forceConstantSpacingEnd
		   || instance->ttcap.forceConstantSpacingBegin <= (signed) c )
                  flags|=FT_FORCE_CONSTANT_SPACING;
          }
#if 0
          fprintf(stderr, "comp_bounds: %x ->", c);
#endif
          if ( skip_ok == 0 || flags == 0 ){
              tmpchar=NULL;
#if 0
              fprintf(stderr, "%x\n", c);
#endif
	      if( FreeTypeFontGetGlyphMetrics(c, flags, &tmpchar, font) != Successful )
		  continue;
          }
          if ( !tmpchar ) continue;
          adjust_min_max(&minchar, &maxchar, tmpchar);
          overlap = tmpchar->rightSideBearing - tmpchar->characterWidth;
          if (maxOverlap < overlap)
              maxOverlap = overlap;

          if (!tmpchar->characterWidth)
              continue;
          num_chars++;
          swidth += ABS(tmpchar->characterWidth);
          total_width += tmpchar->characterWidth;

          if ( flags & FT_FORCE_CONSTANT_SPACING ) skip_ok=1;
      }
    }

#ifndef X_ACCEPTS_NO_SUCH_CHAR
    /* Check code 0 */
    if( FreeTypeInstanceGetGlyphMetrics(font->zero_idx, 0, &tmpchar, font->instance) != Successful || tmpchar == NULL)
	if( FreeTypeInstanceGetGlyphMetrics(font->zero_idx, FT_GET_DUMMY, &tmpchar, font->instance) != Successful )
	    tmpchar = NULL;
    if ( tmpchar ) {
	adjust_min_max(&minchar, &maxchar, tmpchar);
	overlap = tmpchar->rightSideBearing - tmpchar->characterWidth;
	if (maxOverlap < overlap)
	    maxOverlap = overlap;
    }
#endif

    /* AVERAGE_WIDTH ... 1/10 pixel unit */
    if (num_chars > 0) {
        swidth = (swidth * 10.0 + num_chars / 2.0) / num_chars;
        if (total_width < 0)
            swidth = -swidth;
        vals->width = swidth;
    } else
        vals->width = 0;

    /*
    if (char_width.pixel) {
        maxchar.characterWidth = char_width.pixel;
        minchar.characterWidth = char_width.pixel;
    }
    */

    pinfo->maxbounds     = maxchar;
    pinfo->minbounds     = minchar;
    pinfo->ink_maxbounds = maxchar;
    pinfo->ink_minbounds = minchar;
    pinfo->maxOverlap    = maxOverlap;
}

static int
compute_new_extents( FontScalablePtr vals, double scale, double lsb, double rsb, double desc, double asc,
		     int *lsb_result, int *rsb_result, int *desc_result, int *asc_result )
{
#define TRANSFORM_POINT(matrix, x, y, dest) \
    ((dest)[0] = (matrix)[0] * (x) + (matrix)[2] * (y), \
     (dest)[1] = (matrix)[1] * (x) + (matrix)[3] * (y))

#define CHECK_EXTENT(lsb, rsb, desc, asc, data) \
    ((lsb) > (data)[0] ? (lsb) = (data)[0] : 0 , \
     (rsb) < (data)[0] ? (rsb) = (data)[0] : 0, \
     (-desc) > (data)[1] ? (desc) = -(data)[1] : 0 , \
     (asc) < (data)[1] ? (asc) = (data)[1] : 0)
    double newlsb, newrsb, newdesc, newasc;
    double point[2];

    /* Compute new extents for this glyph */
    TRANSFORM_POINT(vals->pixel_matrix, lsb, -desc, point);
    newlsb  = point[0];
    newrsb  = newlsb;
    newdesc = -point[1];
    newasc  = -newdesc;
    TRANSFORM_POINT(vals->pixel_matrix, lsb, asc, point);
    CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);
    TRANSFORM_POINT(vals->pixel_matrix, rsb, -desc, point);
    CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);
    TRANSFORM_POINT(vals->pixel_matrix, rsb, asc, point);
    CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);

    /* ???: lsb = (int)floor(newlsb * scale); */
    *lsb_result   = (int)floor(newlsb * scale + 0.5);
    *rsb_result   = (int)floor(newrsb * scale + 0.5);
    *desc_result  = (int)ceil(newdesc * scale - 0.5);
    *asc_result   = (int)floor(newasc * scale + 0.5);

    return 0;
#undef CHECK_EXTENT
#undef TRANSFORM_POINT
}

static int
is_matrix_unit(FontScalablePtr vals)
{
    double base_size;
    FT_Matrix m;

    base_size = hypot(vals->point_matrix[2], vals->point_matrix[3]);

    m.xx = vals->point_matrix[0] / base_size * 65536;
    m.xy = vals->point_matrix[2] / base_size * 65536;
    m.yx = vals->point_matrix[1] / base_size * 65536;
    m.yy = vals->point_matrix[3] / base_size * 65536;

    return (m.xx == 65536) && (m.yx == 0) &&
	   (m.xy == 0) && (m.yy == 65536);
}

/* Do all the real work for OpenFont or FontInfo */
/* xf->info is only accessed through info, and xf might be null */

static int
FreeTypeLoadXFont(char *fileName,
                  FontScalablePtr vals, FontPtr xf, FontInfoPtr info,
                  FontBitmapFormatPtr bmfmt, FontEntryPtr entry)
{
    FTFontPtr font = NULL;
    FTFacePtr face = NULL;
    FTInstancePtr instance;
    FT_Size_Metrics *smetrics;
    int xrc=Successful;
    int charcell;
    long rawWidth = 0, rawAverageWidth = 0;
    int upm, minLsb, maxRsb, ascent, descent, width, averageWidth;
    double scale, base_width, base_height;
    Bool orig_is_matrix_unit, font_properties;
    int face_number, ttcap_spacing;
    struct TTCapInfo tmp_ttcap;
    struct TTCapInfo *ins_ttcap;
    FT_Int32 load_flags = FT_LOAD_DEFAULT;	/* orig: FT_LOAD_RENDER | FT_LOAD_MONOCHROME */
    char *dynStrRealFileName   = NULL;	/* foo.ttc */
    char *dynStrFTFileName     = NULL;	/* :1:foo.ttc */
    char *dynStrTTCapCodeRange = NULL;

    font = calloc(1, sizeof(FTFontRec));
    if(font == NULL) {
        xrc = AllocError;
        goto quit;
    }

    xrc = FreeTypeSetUpTTCap(fileName, vals,
			     &dynStrRealFileName, &dynStrFTFileName,
			     &tmp_ttcap, &face_number,
			     &load_flags, &ttcap_spacing,
			     &font_properties, &dynStrTTCapCodeRange);
    if ( xrc != Successful ) {
	goto quit;
    }

    xrc = FreeTypeOpenFace(&face, dynStrFTFileName, dynStrRealFileName, face_number);
    if(xrc != Successful) {
        goto quit;
    }

    if( is_matrix_unit(vals) )
	orig_is_matrix_unit = True;
    else {
	orig_is_matrix_unit = False;
	/* Turn off EmbeddedBitmap when original matrix is not diagonal. */
	load_flags |= FT_LOAD_NO_BITMAP;
    }

    if( face->bitmap ) load_flags &= ~FT_LOAD_NO_BITMAP;

    /* Slant control by TTCap */
    if(!face->bitmap) {
	vals->pixel_matrix[2] +=
	    vals->pixel_matrix[0] * tmp_ttcap.autoItalic;
	vals->point_matrix[2] +=
	    vals->point_matrix[0] * tmp_ttcap.autoItalic;
	vals->pixel_matrix[3] +=
	    vals->pixel_matrix[1] * tmp_ttcap.autoItalic;
	vals->point_matrix[3] +=
	    vals->point_matrix[1] * tmp_ttcap.autoItalic;
    }

    base_width=hypot(vals->pixel_matrix[0], vals->pixel_matrix[1]);
    base_height=hypot(vals->pixel_matrix[2], vals->pixel_matrix[3]);
    if(MAX(base_width, base_height) < 1.0 ) {
        xrc = BadFontName;
	goto quit;
    }

    xrc = FreeTypeLoadFont(font, info, face, dynStrFTFileName, vals, entry, bmfmt,
			   load_flags, &tmp_ttcap, dynStrTTCapCodeRange,
			   ttcap_spacing );
    if(xrc != Successful) {
        goto quit;
    }

    instance = font->instance;
    smetrics = &instance->size->metrics;
    ins_ttcap = &instance->ttcap;

    upm = face->face->units_per_EM;
    if(upm == 0) {
        /* Work around FreeType bug */
        upm = WORK_AROUND_UPM;
    }
    scale = 1.0 / upm;

    charcell = (instance->spacing == FT_CHARCELL);

    if( instance->charcellMetrics == NULL ) {

	/* New instance */

	long force_c_rawWidth = 0;
	int force_c_lsb,force_c_rsb,force_c_width;
	double unit_x=0,unit_y=0,advance;
	CharInfoPtr tmpglyph;

	/*
	 * CALCULATE HEADER'S METRICS
	 */

	/* for OUTLINE fonts */
	if(!face->bitmap) {
	    int new_width;
	    double ratio,force_c_ratio;
	    double width_x=0,width_y=0;
	    double force_c_width_x, force_c_rsb_x, force_c_lsb_x;
	    double tmp_rsb,tmp_lsb,tmp_asc,tmp_des;
	    double max_advance_height;
	    tmp_asc = face->face->bbox.yMax;
	    tmp_des = -(face->face->bbox.yMin);
	    if ( tmp_asc < face->face->ascender ) tmp_asc = face->face->ascender;
	    if ( tmp_des < -(face->face->descender) ) tmp_des = -(face->face->descender);
	    tmp_lsb = face->face->bbox.xMin;
	    tmp_rsb = face->face->bbox.xMax;
	    if ( tmp_rsb < face->face->max_advance_width ) tmp_rsb = face->face->max_advance_width;
	    /* apply scaleBBoxWidth */
	    /* we should not ...??? */
	    tmp_lsb *= ins_ttcap->scaleBBoxWidth;
	    tmp_rsb *= ins_ttcap->scaleBBoxWidth;
	    /* transform and rescale */
	    compute_new_extents( vals, scale, tmp_lsb, tmp_rsb, tmp_des, tmp_asc,
				 &minLsb, &maxRsb, &descent, &ascent );
	    /* */
	    /* Consider vertical layouts */
	    if( 0 < face->face->max_advance_height )
		max_advance_height = face->face->max_advance_height;
	    else
		max_advance_height = tmp_asc + tmp_des;
	    if( vals->pixel_matrix[1] == 0 ){
		unit_x = fabs(vals->pixel_matrix[0]);
		unit_y = 0;
		width_x = face->face->max_advance_width * ins_ttcap->scaleBBoxWidth * unit_x;
	    }
	    else if( vals->pixel_matrix[3] == 0 ){
		unit_y = fabs(vals->pixel_matrix[2]);
		unit_x = 0;
		width_x = max_advance_height * ins_ttcap->scaleBBoxHeight * unit_y;
	    }
	    else{
		unit_x = fabs(vals->pixel_matrix[0] -
			      vals->pixel_matrix[1]*vals->pixel_matrix[2]/vals->pixel_matrix[3]);
		unit_y = fabs(vals->pixel_matrix[2] -
			      vals->pixel_matrix[3]*vals->pixel_matrix[0]/vals->pixel_matrix[1]);
		width_x = face->face->max_advance_width * ins_ttcap->scaleBBoxWidth * unit_x;
		width_y = max_advance_height * ins_ttcap->scaleBBoxHeight * unit_y;
		if( width_y < width_x ){
		    width_x = width_y;
		    unit_x = 0;
		}
		else{
		    unit_y = 0;
		}
	    }
	    /* calculate correction ratio */
	    width = (int)floor( (advance = width_x * scale) + 0.5);
	    new_width = width;
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH )
		new_width += ins_ttcap->doubleStrikeShift;
	    new_width += ins_ttcap->adjustBBoxWidthByPixel;
	    ratio = (double)new_width/width;
	    width = new_width;
	    /* force constant */
	    if( unit_x != 0 ) {
		force_c_width_x = face->face->max_advance_width
		    * ins_ttcap->force_c_scale_b_box_width * unit_x;
		force_c_lsb_x = face->face->max_advance_width
		    * ins_ttcap->force_c_scale_lsb * unit_x;
		force_c_rsb_x = face->face->max_advance_width
		    * ins_ttcap->force_c_scale_rsb * unit_x;
	    }
	    else {
		force_c_width_x = max_advance_height
		    * ins_ttcap->force_c_scale_b_box_height * unit_y;
		force_c_lsb_x = max_advance_height
		    * ins_ttcap->force_c_scale_lsb * unit_y;
		force_c_rsb_x = max_advance_height
		    * ins_ttcap->force_c_scale_rsb * unit_y;
	    }
	    /* calculate correction ratio */
	    force_c_width = (int)floor(force_c_width_x * scale + 0.5);
	    new_width = force_c_width;
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH )
		force_c_width += ins_ttcap->doubleStrikeShift;
	    new_width += ins_ttcap->force_c_adjust_width_by_pixel;
	    force_c_ratio = (double)new_width/force_c_width;
	    force_c_width = new_width;
	    /* force_c_lsb, force_c_rsb */
	    if( ins_ttcap->flags & TTCAP_FORCE_C_LSB_FLAG )
		force_c_lsb = (int)floor( force_c_lsb_x * scale + 0.5 );
	    else
		force_c_lsb = minLsb;
	    if( ins_ttcap->flags & TTCAP_FORCE_C_RSB_FLAG )
		force_c_rsb = (int)floor( force_c_rsb_x * scale + 0.5 );
	    else
		force_c_rsb = maxRsb;
	    /* calculate shift of BitmapAutoItalic
	       (when diagonal matrix only) */
	    if( orig_is_matrix_unit == True ) {
		if( ins_ttcap->autoItalic != 0 ) {
		    double ai;
		    int ai_lsb,ai_rsb,ai_total;
		    if( 0 < ins_ttcap->autoItalic ) ai=ins_ttcap->autoItalic;
		    else ai = -ins_ttcap->autoItalic;
		    ai_total = (int)( (ascent+descent) * ai + 0.5);
		    ai_rsb = (int)((double)ai_total * ascent / ( ascent + descent ) + 0.5 );
		    ai_lsb = -(ai_total - ai_rsb);
		    if( 0 < ins_ttcap->autoItalic ) {
			ins_ttcap->lsbShiftOfBitmapAutoItalic = ai_lsb;
			ins_ttcap->rsbShiftOfBitmapAutoItalic = ai_rsb;
		    }
		    else {
			ins_ttcap->lsbShiftOfBitmapAutoItalic = -ai_rsb;
			ins_ttcap->rsbShiftOfBitmapAutoItalic = -ai_lsb;
		    }
		}
	    }
	    /* integer adjustment by TTCap */
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE )
		maxRsb += ins_ttcap->doubleStrikeShift;
	    maxRsb += ins_ttcap->adjustRightSideBearingByPixel;
	    minLsb += ins_ttcap->adjustLeftSideBearingByPixel;
	    /* */
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE )
		force_c_rsb += ins_ttcap->doubleStrikeShift;
	    force_c_rsb += ins_ttcap->force_c_adjust_rsb_by_pixel;
	    force_c_lsb += ins_ttcap->force_c_adjust_lsb_by_pixel;
	    /* apply to rawWidth */
	    averageWidth = (int)floor(10 * width_x * scale
				      * ratio + 0.5);
	    rawWidth = floor(width_x * scale
			     * ratio * 1000. / base_height + 0.5);
	    rawAverageWidth = floor(width_x * scale * ratio * 10.
				    * 1000. / base_height + 0.5);
	    force_c_rawWidth = floor(force_c_width_x * scale
				     * force_c_ratio * 1000. / base_height + 0.5);
	    /* */
	}
	/* for BITMAP fonts [if(face->bitmap)] */
	else {
	    /* These values differ from actual when outline,
	       so we must use them ONLY FOR BITMAP. */
	    width = (int)floor(smetrics->max_advance * ins_ttcap->scaleBBoxWidth / 64.0 + .5);
	    descent = -smetrics->descender / 64;
	    ascent = smetrics->ascender / 64;
	    /* force constant */
	    force_c_width = (int)floor(smetrics->max_advance
				       * ins_ttcap->force_c_scale_b_box_width / 64.0 + .5);
	    /* Preserve average width for bitmap fonts */
	    if(vals->width != 0)
		averageWidth = (int)floor(vals->width * ins_ttcap->scaleBBoxWidth +.5);
	    else
		averageWidth = (int)floor(10.0 * smetrics->max_advance
					  * ins_ttcap->scaleBBoxWidth / 64.0 + .5);
	    rawWidth = 0;
	    rawAverageWidth = 0;
	    force_c_rawWidth = 0;
	    /* We don't consider vertical layouts */
	    advance = (int)floor(smetrics->max_advance / 64.0 +.5);
	    unit_x = vals->pixel_matrix[0];
	    unit_y = 0;
	    /* We can use 'width' only when bitmap.
	       This should not be set when outline. */
	    minLsb = 0;
	    maxRsb = width;
	    /* force constant */
	    if( ins_ttcap->flags & TTCAP_FORCE_C_LSB_FLAG )
		force_c_lsb = (int)floor(smetrics->max_advance
					 * ins_ttcap->force_c_scale_lsb / 64.0 + .5);
	    else
		force_c_lsb = minLsb;
	    if( ins_ttcap->flags & TTCAP_FORCE_C_RSB_FLAG )
		force_c_rsb = (int)floor(smetrics->max_advance
					 * ins_ttcap->force_c_scale_rsb / 64.0 + .5);
	    else
		force_c_rsb = maxRsb;
	    /* calculate shift of BitmapAutoItalic */
	    if( ins_ttcap->autoItalic != 0 ) {
		double ai;
		int ai_lsb,ai_rsb,ai_total;
		if( 0 < ins_ttcap->autoItalic ) ai=ins_ttcap->autoItalic;
		else ai = -ins_ttcap->autoItalic;
		ai_total = (int)( (ascent+descent) * ai + 0.5);
		ai_rsb = (int)((double)ai_total * ascent / ( ascent + descent ) + 0.5 );
		ai_lsb = -(ai_total - ai_rsb);
		if( 0 < ins_ttcap->autoItalic ) {
		    ins_ttcap->lsbShiftOfBitmapAutoItalic = ai_lsb;
		    ins_ttcap->rsbShiftOfBitmapAutoItalic = ai_rsb;
		}
		else {
		    ins_ttcap->lsbShiftOfBitmapAutoItalic = -ai_rsb;
		    ins_ttcap->rsbShiftOfBitmapAutoItalic = -ai_lsb;
		}
	    }
	    /* integer adjustment by TTCap */
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH )
		width += ins_ttcap->doubleStrikeShift;
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE )
		maxRsb += ins_ttcap->doubleStrikeShift;
	    maxRsb += ins_ttcap->adjustRightSideBearingByPixel;
	    minLsb += ins_ttcap->adjustLeftSideBearingByPixel;
	    /* We have not carried out matrix calculation, so this is done. */
	    maxRsb += ins_ttcap->rsbShiftOfBitmapAutoItalic;
	    minLsb += ins_ttcap->lsbShiftOfBitmapAutoItalic;
	    /* force constant */
	    if( ins_ttcap->flags & TTCAP_DOUBLE_STRIKE )
		force_c_rsb += ins_ttcap->doubleStrikeShift;
	    force_c_rsb += ins_ttcap->force_c_adjust_rsb_by_pixel;
	    force_c_lsb += ins_ttcap->force_c_adjust_lsb_by_pixel;
	    force_c_rsb += ins_ttcap->rsbShiftOfBitmapAutoItalic;
	    force_c_lsb += ins_ttcap->lsbShiftOfBitmapAutoItalic;
	}

	/* SET CALCULATED VALUES TO INSTANCE */

	/* Set actual height and cosine */
	instance->pixel_size = base_height;
	instance->advance = advance;
	if ( unit_x != 0 ){
	    instance->pixel_width_unit_x = unit_x/base_height;
	    instance->pixel_width_unit_y = 0;
	}
	else{
	    instance->pixel_width_unit_x = 0;
	    instance->pixel_width_unit_y = unit_y/base_height;
	}

	/* header's metrics */
	instance->charcellMetrics = malloc(sizeof(xCharInfo));
	if(instance->charcellMetrics == NULL) {
	    xrc = AllocError;
	    goto quit;
	}
	instance->charcellMetrics->ascent = ascent;
	instance->charcellMetrics->descent = descent;
	instance->charcellMetrics->attributes = rawWidth;
	instance->charcellMetrics->rightSideBearing = maxRsb;
	instance->charcellMetrics->leftSideBearing = minLsb;
	instance->charcellMetrics->characterWidth = width;
	instance->averageWidth = averageWidth;
	instance->rawAverageWidth = rawAverageWidth;

	/* Check code 0 */
	if( FreeTypeInstanceGetGlyph(font->zero_idx, 0, &tmpglyph, font->instance) != Successful
	    || tmpglyph == NULL)
	    if( FreeTypeInstanceGetGlyph(font->zero_idx, FT_GET_DUMMY, &tmpglyph, font->instance)
		!= Successful )
		tmpglyph = NULL;
	if ( !tmpglyph ) {
	    xrc = AllocError;
	    goto quit;
	}

	/* FORCE CONSTANT METRICS */
	if( 0 <= ins_ttcap->forceConstantSpacingEnd ) {
            xCharInfo *tmpchar = NULL;
            int c = ins_ttcap->force_c_representative_metrics_char_code;
	    /* header's metrics */
	    if( instance->forceConstantMetrics == NULL ){
		instance->forceConstantMetrics = malloc(sizeof(xCharInfo));
		if(instance->forceConstantMetrics == NULL) {
		    xrc = AllocError;
		    goto quit;
		}
	    }
            /* Get Representative Metrics */
            if ( 0 <= c ) {
		if( FreeTypeFontGetGlyphMetrics(c, 0, &tmpchar, font) != Successful )
		    tmpchar = NULL;
            }
            if ( tmpchar && 0 < tmpchar->characterWidth ) {
		instance->forceConstantMetrics->leftSideBearing  = tmpchar->leftSideBearing;
		instance->forceConstantMetrics->rightSideBearing = tmpchar->rightSideBearing;
		instance->forceConstantMetrics->characterWidth   = tmpchar->characterWidth;
		instance->forceConstantMetrics->ascent           = tmpchar->ascent;
		instance->forceConstantMetrics->descent          = tmpchar->descent;
		instance->forceConstantMetrics->attributes       = tmpchar->attributes;
            }
            else {
                instance->forceConstantMetrics->leftSideBearing  = force_c_lsb;
                instance->forceConstantMetrics->rightSideBearing = force_c_rsb;
                instance->forceConstantMetrics->characterWidth   = force_c_width;
                instance->forceConstantMetrics->ascent           = ascent;
                instance->forceConstantMetrics->descent          = descent;
                instance->forceConstantMetrics->attributes       = force_c_rawWidth;
            }
	    /* Check code 0 */
	    if( FreeTypeInstanceGetGlyph(font->zero_idx, FT_FORCE_CONSTANT_SPACING,
					 &tmpglyph, font->instance) != Successful
		|| tmpglyph == NULL)
		if( FreeTypeInstanceGetGlyph(font->zero_idx, FT_FORCE_CONSTANT_SPACING | FT_GET_DUMMY,
					     &tmpglyph, font->instance)
		    != Successful )
		    tmpglyph = NULL;
	    if ( !tmpglyph ) {
		xrc = AllocError;
		goto quit;
	    }
        }
    }
    else{

	/*
	 * CACHED VALUES
	 */

	width = instance->charcellMetrics->characterWidth;
	ascent = instance->charcellMetrics->ascent;
	descent = instance->charcellMetrics->descent;
	rawWidth = instance->charcellMetrics->attributes;
	maxRsb = instance->charcellMetrics->rightSideBearing;
	minLsb = instance->charcellMetrics->leftSideBearing;
	averageWidth = instance->averageWidth;
	rawAverageWidth = instance->rawAverageWidth;

    }

    /*
     * SET maxbounds, minbounds ...
     */

    if( !charcell ) {		/* NOT CHARCELL */
	if( info ){
	    /*
	       Calculate all glyphs' metrics.
	       maxbounds.ascent and maxbounds.descent are quite important values
	       for XAA.  If ascent/descent of each glyph exceeds
	       maxbounds.ascent/maxbounds.descent, XAA causes SERVER CRASH.
	       Therefore, THIS MUST BE DONE.
	    */
	    ft_compute_bounds(font,info,vals);
	}
    }
    else{			/* CHARCELL */

    /*
     * SET CALCULATED OR CACHED VARIABLES
     */

	vals->width = averageWidth;

	if( info ){

	    info->maxbounds.leftSideBearing = minLsb;
	    info->maxbounds.rightSideBearing = maxRsb;
	    info->maxbounds.characterWidth = width;
	    info->maxbounds.ascent = ascent;
	    info->maxbounds.descent = descent;
	    info->maxbounds.attributes =
		(unsigned short)(short)rawWidth;

	    info->minbounds = info->maxbounds;
	}
    }

    /* set info */

    if( info ){
	/*
	info->fontAscent = ascent;
	info->fontDescent = descent;
	*/
	info->fontAscent = info->maxbounds.ascent;
	info->fontDescent = info->maxbounds.descent;
	/* Glyph metrics are accurate */
	info->inkMetrics=1;

	memcpy((char *)&info->ink_maxbounds,
	       (char *)&info->maxbounds, sizeof(xCharInfo));
	memcpy((char *)&info->ink_minbounds,
	       (char *)&info->minbounds, sizeof(xCharInfo));

	/* XXX - hack */
	info->defaultCh=0;

	/* Set the pInfo flags */
	/* Properties set by FontComputeInfoAccelerators:
	   pInfo->noOverlap;
	   pInfo->terminalFont;
	   pInfo->constantMetrics;
	   pInfo->constantWidth;
	   pInfo->inkInside;
	*/
	/* from lib/font/util/fontaccel.c */
	FontComputeInfoAccelerators(info);
    }

    if(xf)
        xf->fontPrivate = (void*)font;

    if(info) {
        xrc = FreeTypeAddProperties(font, vals, info, entry->name.name,
                                    rawAverageWidth, font_properties);
        if (xrc != Successful) {
            goto quit;
        }
    }

 quit:
    if ( dynStrTTCapCodeRange ) free(dynStrTTCapCodeRange);
    if ( dynStrFTFileName ) free(dynStrFTFileName);
    if ( dynStrRealFileName ) free(dynStrRealFileName);
    if ( xrc != Successful ) {
	if( font ){
	    if( face && font->instance == NULL ) FreeTypeFreeFace(face);
	    FreeTypeFreeFont(font);
	}
    }
    return xrc;
}

/* Routines used by X11 to get info and glyphs from the font. */

static int
FreeTypeGetMetrics(FontPtr pFont, unsigned long count, unsigned char *chars,
                   FontEncoding charEncoding, unsigned long *metricCount,
                   xCharInfo **metrics)
{
    unsigned int code = 0;
    int flags = 0;
    FTFontPtr tf;
    struct TTCapInfo *ttcap;
    xCharInfo **mp, *m;

    /*  MUMBLE("Get metrics for %ld characters\n", count);*/

    tf = (FTFontPtr)pFont->fontPrivate;
    ttcap = &tf->instance->ttcap;
    mp = metrics;

    while (count-- > 0) {
        switch (charEncoding) {
        case Linear8Bit:
        case TwoD8Bit:
            code = *chars++;
            break;
        case Linear16Bit:
        case TwoD16Bit:
            code = (*chars++ << 8);
            code |= *chars++;
	    /* */
            if ( !(ttcap->flags & TTCAP_FORCE_C_OUTSIDE) ) {
                if ( (int)code <= ttcap->forceConstantSpacingEnd
		     && ttcap->forceConstantSpacingBegin <= (int)code )
                    flags|=FT_FORCE_CONSTANT_SPACING;
		else flags=0;
            }
            else {      /* for GB18030 proportional */
                if ( (int)code <= ttcap->forceConstantSpacingEnd
		     || ttcap->forceConstantSpacingBegin <= (int)code )
                    flags|=FT_FORCE_CONSTANT_SPACING;
		else flags=0;
            }
            break;
        }

        if(FreeTypeFontGetGlyphMetrics(code, flags, &m, tf) == Successful && m!=NULL) {
            *mp++ = m;
        }
#ifdef X_ACCEPTS_NO_SUCH_CHAR
	else *mp++ = &noSuchChar.metrics;
#endif
    }

    *metricCount = mp - metrics;
    return Successful;
}

static int
FreeTypeGetGlyphs(FontPtr pFont, unsigned long count, unsigned char *chars,
                  FontEncoding charEncoding, unsigned long *glyphCount,
                  CharInfoPtr *glyphs)
{
    unsigned int code = 0;
    int flags = 0;
    FTFontPtr tf;
    CharInfoPtr *gp;
    CharInfoPtr g;
    struct TTCapInfo *ttcap;

    tf = (FTFontPtr)pFont->fontPrivate;
    ttcap = &tf->instance->ttcap;
    gp = glyphs;

    while (count-- > 0) {
        switch (charEncoding) {
        case Linear8Bit: case TwoD8Bit:
            code = *chars++;
            break;
        case Linear16Bit: case TwoD16Bit:
            code = *chars++ << 8;
            code |= *chars++;
	    /* */
            if ( !(ttcap->flags & TTCAP_FORCE_C_OUTSIDE) ) {
                if ( (int)code <= ttcap->forceConstantSpacingEnd
		     && ttcap->forceConstantSpacingBegin <= (int)code )
                    flags|=FT_FORCE_CONSTANT_SPACING;
		else flags=0;
            }
            else {      /* for GB18030 proportional */
                if ( (int)code <= ttcap->forceConstantSpacingEnd
		     || ttcap->forceConstantSpacingBegin <= (int)code )
                    flags|=FT_FORCE_CONSTANT_SPACING;
		else flags=0;
            }
            break;
        }

        if(FreeTypeFontGetGlyph(code, flags, &g, tf) == Successful && g!=NULL) {
            *gp++ = g;
        }
#ifdef X_ACCEPTS_NO_SUCH_CHAR
	else {
#ifdef XAA_ACCEPTS_NULL_BITS
	    *gp++ = &noSuchChar;
#else
	    if ( tf->dummy_char.bits ) {
		*gp++ = &tf->dummy_char;
	    }
	    else {
		char *raster = NULL;
		int wd_actual, ht_actual, wd, ht, bpr;
		wd_actual = tf->info->maxbounds.rightSideBearing - tf->info->maxbounds.leftSideBearing;
		ht_actual = tf->info->maxbounds.ascent + tf->info->maxbounds.descent;
		if(wd_actual <= 0) wd = 1;
		else wd=wd_actual;
		if(ht_actual <= 0) ht = 1;
		else ht=ht_actual;
		bpr = (((wd + (tf->instance->bmfmt.glyph<<3) - 1) >> 3) &
		       -tf->instance->bmfmt.glyph);
		raster = calloc(1, ht * bpr);
		if(raster) {
		    tf->dummy_char.bits = raster;
		    *gp++ = &tf->dummy_char;
		}
	    }
#endif
	}
#endif
    }

    *glyphCount = gp - glyphs;
    return Successful;
}

static int
FreeTypeSetUpFont(FontPathElementPtr fpe, FontPtr xf, FontInfoPtr info,
                  fsBitmapFormat format, fsBitmapFormatMask fmask,
                  FontBitmapFormatPtr bmfmt)
{
    int xrc;
    int image;

    /* Get the default bitmap format information for this X installation.
       Also update it for the client if running in the font server. */
    FontDefaultFormat(&bmfmt->bit, &bmfmt->byte, &bmfmt->glyph, &bmfmt->scan);
    if ((xrc = CheckFSFormat(format, fmask, &bmfmt->bit, &bmfmt->byte,
                             &bmfmt->scan, &bmfmt->glyph,
                             &image)) != Successful) {
        MUMBLE("Aborting after checking FS format: %d\n", xrc);
        return xrc;
    }

    if(xf) {
        xf->refcnt = 0;
        xf->bit = bmfmt->bit;
        xf->byte = bmfmt->byte;
        xf->glyph = bmfmt->glyph;
        xf->scan = bmfmt->scan;
        xf->format = format;
        xf->get_glyphs = FreeTypeGetGlyphs;
        xf->get_metrics = FreeTypeGetMetrics;
        xf->unload_font = FreeTypeUnloadXFont;
        xf->unload_glyphs = 0;
        xf->fpe = fpe;
        xf->svrPrivate = 0;
        xf->fontPrivate = 0;        /* we'll set it later */
        xf->fpePrivate = 0;
    }

    info->defaultCh = 0;
    info->noOverlap = 0;          /* not updated */
    info->terminalFont = 0;       /* not updated */
    info->constantMetrics = 0;    /* we'll set it later */
    info->constantWidth = 0;      /* we'll set it later */
    info->inkInside = 1;
    info->inkMetrics = 1;
    info->allExist=0;             /* not updated */
    info->drawDirection = LeftToRight; /* we'll set it later */
    info->cachable = 1;           /* we don't do licensing */
    info->anamorphic = 0;         /* can hinting lead to anamorphic scaling? */
    info->maxOverlap = 0;         /* we'll set it later. */
    info->pad = 0;                /* ??? */
    return Successful;
}

/* Functions exported by the backend */

static int
FreeTypeOpenScalable(FontPathElementPtr fpe, FontPtr *ppFont, int flags,
                     FontEntryPtr entry, char *fileName, FontScalablePtr vals,
                     fsBitmapFormat format, fsBitmapFormatMask fmask,
                     FontPtr non_cachable_font)
{
    int xrc;
    FontPtr xf;
    FontBitmapFormatRec bmfmt;

    MUMBLE("Open Scalable %s, XLFD=%s\n",fileName, entry->name.length ? entry->name.name : "");

    xf = CreateFontRec();
    if (xf == NULL)
        return AllocError;

    xrc = FreeTypeSetUpFont(fpe, xf, &xf->info, format, fmask, &bmfmt);
    if(xrc != Successful) {
        DestroyFontRec(xf);
        return xrc;
    }
    xrc = FreeTypeLoadXFont(fileName, vals, xf, &xf->info, &bmfmt, entry);
    if(xrc != Successful) {
        MUMBLE("Error during load: %d\n",xrc);
        DestroyFontRec(xf);
        return xrc;
    }

    *ppFont = xf;

    return xrc;
}

/* Routine to get requested font info. */

static int
FreeTypeGetInfoScalable(FontPathElementPtr fpe, FontInfoPtr info,
                        FontEntryPtr entry, FontNamePtr fontName,
                        char *fileName, FontScalablePtr vals)
{
    int xrc;
    FontBitmapFormatRec bmfmt;

    MUMBLE("Get info, XLFD=%s\n", entry->name.length ? entry->name.name : "");

    xrc = FreeTypeSetUpFont(fpe, 0, info, 0, 0, &bmfmt);
    if(xrc != Successful) {
        return xrc;
    }

    bmfmt.glyph <<= 3;

    xrc = FreeTypeLoadXFont(fileName, vals, 0, info, &bmfmt, entry);
    if(xrc != Successful) {
        MUMBLE("Error during load: %d\n", xrc);
        return xrc;
    }

    return Successful;
}

/* Renderer registration. */

/* Set the capabilities of this renderer. */
#define CAPABILITIES (CAP_CHARSUBSETTING | CAP_MATRIX)

/* Set it up so file names with either upper or lower case can be
   loaded.  We don't support compressed fonts. */
static FontRendererRec renderers[] = {
    {".ttf", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".ttc", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".otf", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".otc", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".pfa", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".pfb", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
};
static int num_renderers = sizeof(renderers) / sizeof(renderers[0]);

static FontRendererRec alt_renderers[] = {
    {".bdf", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
    {".pcf", 4, 0, FreeTypeOpenScalable, 0,
     FreeTypeGetInfoScalable, 0, CAPABILITIES},
};

static int num_alt_renderers =
sizeof(alt_renderers) / sizeof(alt_renderers[0]);


void
FreeTypeRegisterFontFileFunctions(void)
{
    int i;

    for (i = 0; i < num_renderers; i++)
        FontFileRegisterRenderer(&renderers[i]);

    for (i = 0; i < num_alt_renderers; i++)
        FontFilePriorityRegisterRenderer(&alt_renderers[i], -10);
}
