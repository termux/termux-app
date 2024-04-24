/*
Copyright (c) 1998-2002 by Juliusz Chroboczek
Copyright (c) 2003 After X-TT Project, All rights reserved.

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

/* Number of buckets in the hashtable holding faces */
#define NUMFACEBUCKETS 32

/* Glyphs are held in segments of this size */
#define FONTSEGMENTSIZE 16

/* A structure that holds bitmap order and padding info. */

typedef struct {
    int bit;                    /* bit order */
    int byte;                   /* byte order */
    int glyph;                  /* glyph pad size */
    int scan;                   /* machine word size */
} FontBitmapFormatRec, *FontBitmapFormatPtr;

struct FTSize_s;

/* At the lowest level, there is face; FTFaces are in one-to-one
   correspondence with TrueType faces.  Multiple instance may share
   the same face. */

typedef struct _FTFace {
    char *filename;
    FT_Face face;
    int bitmap;
    FT_UInt num_hmetrics;
    struct _FTInstance *instances;
    struct _FTInstance *active_instance;
    struct _FTFace *next;       /* link to next face in bucket */
} FTFaceRec, *FTFacePtr;

/* A transformation matrix with resolution information */
typedef struct _FTNormalisedTransformation {
    double scale;
    int nonIdentity;            /* if 0, matrix is the identity */
    FT_Matrix matrix;
    int xres, yres;
} FTNormalisedTransformationRec, *FTNormalisedTransformationPtr;

#define FT_PROPORTIONAL 0
#define FT_MONOSPACED 1
#define FT_CHARCELL 2

#define FT_AVAILABLE_UNKNOWN 0
#define FT_AVAILABLE_NO 1
#define FT_AVAILABLE_METRICS 2
#define FT_AVAILABLE_RASTERISED 3

#define FT_GET_GLYPH_BOTH	  0x01
#define FT_GET_GLYPH_METRICS_ONLY 0x02
#define FT_GET_DUMMY		  0x04
#define FT_FORCE_CONSTANT_SPACING 0x08

#define TTCAP_DOUBLE_STRIKE			0x0001
#define TTCAP_DOUBLE_STRIKE_MKBOLD_EDGE_LEFT	0x0002
#define TTCAP_DOUBLE_STRIKE_CORRECT_B_BOX_WIDTH	0x0008
#define TTCAP_IS_VERY_LAZY			0x0010
#define TTCAP_DISABLE_DEFAULT_VERY_LAZY		0x0020
#define TTCAP_FORCE_C_LSB_FLAG			0x0100
#define TTCAP_FORCE_C_RSB_FLAG			0x0200
#define TTCAP_FORCE_C_OUTSIDE			0x0400
#define TTCAP_MONO_CENTER			0x0800

/* TTCap */
struct TTCapInfo {
    long flags;
    double autoItalic;
    double scaleWidth;
    double scaleBBoxWidth;
    double scaleBBoxHeight;
    int doubleStrikeShift;
    int adjustBBoxWidthByPixel;
    int adjustLeftSideBearingByPixel;
    int adjustRightSideBearingByPixel;
    double scaleBitmap;
    int forceConstantSpacingBegin;
    int forceConstantSpacingEnd;
    /* We don't compare */
    int force_c_adjust_width_by_pixel;
    int force_c_adjust_lsb_by_pixel;
    int force_c_adjust_rsb_by_pixel;
    int force_c_representative_metrics_char_code;
    double force_c_scale_b_box_width;
    double force_c_scale_b_box_height;
    double force_c_scale_lsb;
    double force_c_scale_rsb;
    double vl_slant;
    int lsbShiftOfBitmapAutoItalic;
    int rsbShiftOfBitmapAutoItalic;
};

/* An instance builds on a face by specifying the transformation
   matrix.  Multiple fonts may share the same instance. */

/* This structure caches bitmap data */
typedef struct _FTInstance {
    FTFacePtr face;             /* the associated face */
    FT_Size size;
    FTNormalisedTransformationRec transformation;
    FT_Int32 load_flags;
    FT_ULong strike_index;
    int spacing;		/* actual spacing */
    double pixel_size;          /* to calc attributes (actual height) */
    double pixel_width_unit_x;  /* to calc horiz. width (cosine) */
    double pixel_width_unit_y;  /* to calc vert. width (cosine) */
    xCharInfo *charcellMetrics;	/* the header's metrics */
    int averageWidth;		/* the header's metrics */
    long rawAverageWidth;	/* the header's metrics */
    double advance;		/* the header's metrics */
    xCharInfo *forceConstantMetrics;
    FontBitmapFormatRec bmfmt;
    unsigned nglyphs;
    CharInfoPtr *glyphs;        /* glyphs and available are used in parallel */
    int **available;
    struct TTCapInfo ttcap;
    int refcount;
    struct _FTInstance *next;   /* link to next instance */
} FTInstanceRec, *FTInstancePtr;

/* A font is an instance with coding information; fonts are in
   one-to-one correspondence with X fonts */
typedef struct _FTFont{
    FTInstancePtr instance;
    FTMappingRec mapping;
    unsigned zero_idx;
    FontInfoPtr info;
    int nranges;
    CharInfoRec dummy_char;
    fsRange *ranges;
} FTFontRec, *FTFontPtr;

#ifndef NOT_IN_FTFUNCS

/* Prototypes for some local functions */

static int FreeTypeOpenFace(FTFacePtr *facep, char *FTFileName, char *realFileName, int faceNumber);
static void FreeTypeFreeFace(FTFacePtr face);
static int
FreeTypeOpenInstance(FTInstancePtr *instancep, FTFacePtr face,
                     char *FTFileName, FTNormalisedTransformationPtr trans,
                     int spacing, FontBitmapFormatPtr bmfmt,
		     struct TTCapInfo *tmp_ttcap, FT_Int32 load_flags);
static void FreeTypeFreeInstance(FTInstancePtr instance);
static int
FreeTypeInstanceGetGlyph(unsigned idx, int flags, CharInfoPtr *g, FTInstancePtr instance);
static int
FreeTypeInstanceGetGlyphMetrics(unsigned idx, int flags,
				xCharInfo **metrics, FTInstancePtr instance );
static int
FreeTypeRasteriseGlyph(unsigned idx, int flags, CharInfoPtr tgp,
		       FTInstancePtr instance, int hasMetrics );
static void FreeTypeFreeFont(FTFontPtr font);
static void FreeTypeFreeXFont(FontPtr pFont, int freeProps);
static void FreeTypeUnloadXFont(FontPtr pFont);
static int
FreeTypeAddProperties(FTFontPtr font, FontScalablePtr vals, FontInfoPtr info,
                      char *fontname, int rawAverageWidth, Bool font_properties);
static int FreeTypeFontGetGlyph(unsigned code, int flags, CharInfoPtr *g, FTFontPtr font);
static int
FreeTypeLoadFont(FTFontPtr font, FontInfoPtr info, FTFacePtr face,
		 char *FTFileName, FontScalablePtr vals, FontEntryPtr entry,
                 FontBitmapFormatPtr bmfmt, FT_Int32 load_flags,
		 struct TTCapInfo *tmp_ttcap, char *dynStrTTCapCodeRange,
		 int ttcap_spacing );

#endif /* NOT_IN_FTFUNCS */
