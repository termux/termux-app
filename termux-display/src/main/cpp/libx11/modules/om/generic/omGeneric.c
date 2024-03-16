/*  #define FONTDEBUG */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 *
 * Modifier:  Takanori Tateno   FUJITSU LIMITED
 *
 */

/*
 * Fixed the algorithms in parse_fontname() and parse_fontdata()
 * to improve the logic for determining which font should be
 * returned for a given CharSet.  We even added some comments
 * so that you can figure out what in the heck we're doing. We
 * realize this is a departure from the norm, but hey, we're
 * rebels! :-) :-)
 *
 * Modifiers: Jeff Walls, Paul Anderson: HEWLETT-PACKARD
 */
/*
 * Cleaned up mess, removed some blabla
 * Egbert Eich, SuSE Linux AG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XomGeneric.h"
#include "XlcGeneric.h"
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXFONTS		100
#define	PIXEL_SIZE_FIELD	 7
#define	POINT_SIZE_FIELD	 8
#define	CHARSET_ENCODING_FIELD	14
#define XLFD_MAX_LEN		255

/* For VW/UDC start */

static FontData
init_fontdata(
    FontData	font_data,
    int		font_data_count)
{
    FontData	fd;
    int		i;

    fd = Xcalloc(font_data_count, sizeof(FontDataRec));
    if(fd == (FontData) NULL)
	return False;

    for(i = 0 ; i < font_data_count ; i++)
	fd[i] = font_data[i];

    return fd;
}

static VRotate
init_vrotate(
    FontData	font_data,
    int		font_data_count,
    int		type,
    CodeRange	code_range,
    int		code_range_num)
{
    VRotate	vrotate;
    int		i;

    if(type == VROTATE_NONE)
	return (VRotate)NULL;

    vrotate = Xcalloc(font_data_count, sizeof(VRotateRec));
    if(vrotate == (VRotate) NULL)
	return False;

    for(i = 0 ; i < font_data_count ; i++) {
	vrotate[i].charset_name = font_data[i].name;
	vrotate[i].side = font_data[i].side;
	if(type == VROTATE_PART) {
	    vrotate[i].num_cr = code_range_num;
	    vrotate[i].code_range = code_range;
	}
    }

    return vrotate;
}

static Bool
init_fontset(
    XOC oc)
{
    XOCGenericPart *gen;
    FontSet font_set;
    OMData data;
    int count;

    count = XOM_GENERIC(oc->core.om)->data_num;
    data = XOM_GENERIC(oc->core.om)->data;

    font_set = Xcalloc(count, sizeof(FontSetRec));
    if (font_set == NULL)
	return False;

    gen = XOC_GENERIC(oc);
    gen->font_set_num = count;
    gen->font_set = font_set;

    for ( ; count-- > 0; data++, font_set++) {
	font_set->charset_count = data->charset_count;
	font_set->charset_list = data->charset_list;

	if((font_set->font_data = init_fontdata(data->font_data,
				  data->font_data_count)) == NULL)
	    goto err;
	font_set->font_data_count = data->font_data_count;
	if((font_set->substitute = init_fontdata(data->substitute,
				   data->substitute_num)) == NULL)
	    goto err;
	font_set->substitute_num = data->substitute_num;
	if((font_set->vmap = init_fontdata(data->vmap,
			     data->vmap_num)) == NULL)
	    goto err;
	font_set->vmap_num       = data->vmap_num;

	if(data->vrotate_type != VROTATE_NONE) {
	    /* A vrotate member is specified primary font data */
	    /* as initial value.                               */
	    if((font_set->vrotate = init_vrotate(data->font_data,
						 data->font_data_count,
						 data->vrotate_type,
						 data->vrotate,
						 data->vrotate_num)) == NULL)
		goto err;
	    font_set->vrotate_num = data->font_data_count;
	}
    }
    return True;

err:

    Xfree(font_set->font_data);
    Xfree(font_set->substitute);
    Xfree(font_set->vmap);
    Xfree(font_set->vrotate);
    Xfree(font_set);
    gen->font_set = (FontSet) NULL;
    gen->font_set_num = 0;
    return False;
}

/* For VW/UDC end */

static char *
get_prop_name(
    Display *dpy,
    XFontStruct	*fs)
{
    unsigned long fp;

    if (XGetFontProperty(fs, XA_FONT, &fp))
	return XGetAtomName(dpy, fp);

    return (char *) NULL;
}

/* For VW/UDC start */

static Bool
load_fontdata(
    XOC		oc,
    FontData	font_data,
    int		font_data_num)
{
    Display	*dpy = oc->core.om->core.display;
    FontData	fd = font_data;

    if(font_data == NULL) return(True);
    for( ; font_data_num-- ; fd++) {
	if(fd->xlfd_name != (char *) NULL && fd->font == NULL) {
	    fd->font = XLoadQueryFont(dpy, fd->xlfd_name);
	    if (fd->font == NULL){
		return False;
	    }
	}
    }
    return True;
}

static Bool
load_fontset_data(
    XOC		oc,
    FontSet	font_set)
{
    Display	*dpy = oc->core.om->core.display;

    if(font_set->font_name == (char *)NULL) return False ;

   /* If font_set->font is not NULL, it contains the *best*
    * match font for this FontSet.
    * -- jjw/pma (HP)
    */
    if(font_set->font == NULL) {
       font_set->font = XLoadQueryFont(dpy, font_set->font_name);
       if (font_set->font == NULL){
		return False;
       }
    }
    return True;
}

static Bool
load_font(
    XOC oc)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet font_set = gen->font_set;
    int num = gen->font_set_num;

    for ( ; num-- > 0; font_set++) {
	if (font_set->font_name == NULL)
	    continue;

        if (load_fontset_data (oc, font_set) != True)
	    return False;
#ifndef TESTVERSION
	if(load_fontdata(oc, font_set->font_data,
			 font_set->font_data_count) != True)
	    return False;

	if(load_fontdata(oc, font_set->substitute,
			 font_set->substitute_num) != True)
	    return False;
#endif

/* Add 1996.05.20 */
        if( oc->core.orientation == XOMOrientation_TTB_RTL ||
            oc->core.orientation == XOMOrientation_TTB_LTR ){
	    if (font_set->vpart_initialize == 0) {
	       load_fontdata(oc, font_set->vmap, font_set->vmap_num);
	       load_fontdata(oc, (FontData) font_set->vrotate,
			 font_set->vrotate_num);
                font_set->vpart_initialize = 1;
	    }
        }

	if (font_set->font->min_byte1 || font_set->font->max_byte1)
	    font_set->is_xchar2b = True;
	else
	    font_set->is_xchar2b = False;
    }

    return True;
}

/* For VW/UDC end */

static Bool
load_font_info(
    XOC oc)
{
    Display *dpy = oc->core.om->core.display;
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet font_set = gen->font_set;
    char **fn_list;
    int fn_num, num = gen->font_set_num;

    for ( ; num-- > 0; font_set++) {
	if (font_set->font_name == NULL)
	    continue;

	if (font_set->info == NULL) {
	    fn_list = XListFontsWithInfo(dpy, font_set->font_name, 1, &fn_num,
					 &font_set->info);
	    if (font_set->info == NULL)
		return False;

	    XFreeFontNames(fn_list);
	}
    }

    return True;
}

/* For Vertical Writing start */

static void
check_fontset_extents(
    XCharStruct		*overall,
    int			*logical_ascent,
    int                 *logical_descent,
    XFontStruct		*font)
{
    overall->lbearing = min(overall->lbearing, font->min_bounds.lbearing);
    overall->rbearing = max(overall->rbearing, font->max_bounds.rbearing);
    overall->ascent   = max(overall->ascent,   font->max_bounds.ascent);
    overall->descent  = max(overall->descent,  font->max_bounds.descent);
    overall->width    = max(overall->width,    font->max_bounds.width);
    *logical_ascent   = max(*logical_ascent,   font->ascent);
    *logical_descent  = max(*logical_descent,  font->descent);
}

/* For Vertical Writing end */

static void
set_fontset_extents(
    XOC oc)
{
    XRectangle *ink = &oc->core.font_set_extents.max_ink_extent;
    XRectangle *logical = &oc->core.font_set_extents.max_logical_extent;
    XFontStruct **font_list, *font;
    XCharStruct overall;
    int logical_ascent, logical_descent;
    int	num = oc->core.font_info.num_font;

    font_list = oc->core.font_info.font_struct_list;
    font = *font_list++;
    overall = font->max_bounds;
    overall.lbearing = font->min_bounds.lbearing;
    logical_ascent = font->ascent;
    logical_descent = font->descent;

    /* For Vertical Writing start */

    while (--num > 0) {
	font = *font_list++;
	check_fontset_extents(&overall, &logical_ascent, &logical_descent,
			      font);
    }

    {
	XOCGenericPart  *gen = XOC_GENERIC(oc);
	FontSet		font_set = gen->font_set;
	FontData	font_data;
	int		font_set_num = gen->font_set_num;
	int		font_data_count;

	for( ; font_set_num-- ; font_set++) {
	    if(font_set->vmap_num > 0) {
		font_data = font_set->vmap;
		font_data_count = font_set->vmap_num;
		for( ; font_data_count-- ; font_data++) {
		    if(font_data->font != NULL) {
			check_fontset_extents(&overall, &logical_ascent,
					      &logical_descent,
					      font_data->font);
		    }
		}
	    }

	    if(font_set->vrotate_num > 0 && font_set->vrotate != NULL) {
		font_data = (FontData) font_set->vrotate;
		font_data_count = font_set->vrotate_num;
		for( ; font_data_count-- ; font_data++) {
		    if(font_data->font != NULL) {
			check_fontset_extents(&overall, &logical_ascent,
					      &logical_descent,
					      font_data->font);
		    }
		}
	    }
	}
    }

    /* For Vertical Writing start */

    ink->x = overall.lbearing;
    ink->y = -(overall.ascent);
    ink->width = overall.rbearing - overall.lbearing;
    ink->height = overall.ascent + overall.descent;

    logical->x = 0;
    logical->y = -(logical_ascent);
    logical->width = overall.width;
    logical->height = logical_ascent + logical_descent;
}

static Bool
init_core_part(
    XOC oc)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet font_set;
    int font_set_num;
    XFontStruct **font_struct_list;
    char **font_name_list, *font_name_buf;
    int	count, length;

    font_set = gen->font_set;
    font_set_num = gen->font_set_num;
    count = length = 0;

    for ( ; font_set_num-- > 0; font_set++) {
	if (font_set->font_name == NULL)
	    continue;

	length += strlen(font_set->font_name) + 1;

	count++;
    }
    if (count == 0)
        return False;

    font_struct_list = Xmalloc(sizeof(XFontStruct *) * count);
    if (font_struct_list == NULL)
	return False;

    font_name_list = Xmalloc(sizeof(char *) * count);
    if (font_name_list == NULL)
	goto err;

    font_name_buf = Xmalloc(length);
    if (font_name_buf == NULL)
	goto err;

    oc->core.font_info.num_font = count;
    oc->core.font_info.font_name_list = font_name_list;
    oc->core.font_info.font_struct_list = font_struct_list;

    font_set = gen->font_set;
    font_set_num = gen->font_set_num;

    for (count = 0; font_set_num-- > 0; font_set++) {
	if (font_set->font_name == NULL)
	    continue;

	font_set->id = count;
	if (font_set->font)
	    *font_struct_list++ = font_set->font;
	else
	    *font_struct_list++ = font_set->info;
	strcpy(font_name_buf, font_set->font_name);
	Xfree(font_set->font_name);
	*font_name_list++ = font_set->font_name = font_name_buf;
	font_name_buf += strlen(font_name_buf) + 1;

	count++;
    }

    set_fontset_extents(oc);

    return True;

err:

    Xfree(font_name_list);
    Xfree(font_struct_list);

    return False;
}

static char *
get_font_name(
    XOC oc,
    char *pattern)
{
    char **list, *name;
    int count = 0;

    list = XListFonts(oc->core.om->core.display, pattern, 1, &count);
    if (list == NULL)
	return NULL;

    name = strdup(*list);

    XFreeFontNames(list);

    return name;
}

/* For VW/UDC start*/

static char *
get_rotate_fontname(
    char *font_name)
{
    char *pattern = NULL, *ptr = NULL;
    char *fields[CHARSET_ENCODING_FIELD];
    char str_pixel[32], str_point[4];
    char *rotate_font_ptr = NULL;
    int pixel_size = 0;
    int field_num = 0, len = 0;

    if(font_name == (char *) NULL || (len = strlen(font_name)) <= 0
       || len > XLFD_MAX_LEN)
	return NULL;

    pattern = strdup(font_name);
    if(!pattern)
	return NULL;

    memset(fields, 0, sizeof(char *) * 14);
    ptr = pattern;
    while(isspace(*ptr)) {
	ptr++;
    }
    if(*ptr == '-')
	ptr++;

    for(field_num = 0 ; field_num < CHARSET_ENCODING_FIELD && ptr && *ptr ;
			ptr++, field_num++) {
	fields[field_num] = ptr;

	if((ptr = strchr(ptr, '-'))) {
	    *ptr = '\0';
	} else {
	    field_num++;	/* Count last field */
	    break;
	}
    }

    if(field_num < CHARSET_ENCODING_FIELD)
	goto free_pattern;

    /* Pixel Size field : fields[6] */
    for(ptr = fields[PIXEL_SIZE_FIELD - 1] ; ptr && *ptr; ptr++) {
	if(!isdigit(*ptr)) {
	    if(*ptr == '['){ /* 960730 */
	        strcpy(pattern, font_name);
		return(pattern);
	    }
	    goto free_pattern;
	}
    }
    pixel_size = atoi(fields[PIXEL_SIZE_FIELD - 1]);
    snprintf(str_pixel, sizeof(str_pixel),
	     "[ 0 ~%d %d 0 ]", pixel_size, pixel_size);
    fields[6] = str_pixel;

    /* Point Size field : fields[7] */
    strcpy(str_point, "*");
    fields[POINT_SIZE_FIELD - 1] = str_point;

    len = 0;
    for (field_num = 0; field_num < CHARSET_ENCODING_FIELD &&
			fields[field_num]; field_num++) {
	len += 1 + strlen(fields[field_num]);
    }

    /* Max XLFD length is 255 */
    if (len > XLFD_MAX_LEN)
	goto free_pattern;

    rotate_font_ptr = Xmalloc(len + 1);
    if(!rotate_font_ptr)
	goto free_pattern;

    rotate_font_ptr[0] = '\0';

    for(field_num = 0 ; field_num < CHARSET_ENCODING_FIELD &&
			fields[field_num] ; field_num++) {
	strcat(rotate_font_ptr, "-");
	strcat(rotate_font_ptr, fields[field_num]);
    }

free_pattern:
    Xfree(pattern);

    return rotate_font_ptr;
}

static Bool
is_match_charset(
    FontData	font_data,
    char	*font_name)
{
    char *last;
    int length, name_len;

    name_len = strlen(font_name);
    last = font_name + name_len;

    length = strlen(font_data->name);
    if (length > name_len)
	return False;

    if (_XlcCompareISOLatin1(last - length, font_data->name) == 0)
	return True;

    return False;
}

static int
parse_all_name(
    XOC		oc,
    FontData	font_data,
    char	*pattern)
{

#ifdef OLDCODE
    if(is_match_charset(font_data, pattern) != True)
 	return False;

    font_data->xlfd_name = strdup(pattern);
    if(font_data->xlfd_name == NULL)
	return (-1);

    return True;
#else  /* OLDCODE */
    Display *dpy = oc->core.om->core.display;
    char **fn_list = NULL, *prop_fname = NULL;
    int list_num;
    XFontStruct *fs_list;
    if(is_match_charset(font_data, pattern) != True) {
	/*
	 * pattern should not contain any wildcard (execpt '?')
	 * this was probably added to make this case insensitive.
	 */
	if ((fn_list = XListFontsWithInfo(dpy, pattern,
				      MAXFONTS,
				      &list_num, &fs_list)) == NULL) {
            return False;
        }
	/* shouldn't we loop here ? */
        else if ((prop_fname = get_prop_name(dpy, fs_list)) == NULL) {
            XFreeFontInfo(fn_list, fs_list, list_num);
            return False;
        }
        else if ((is_match_charset(font_data, prop_fname) != True)) {
            XFree(prop_fname);
            XFreeFontInfo(fn_list, fs_list, list_num);
            return False;
        }
        else {
	    font_data->xlfd_name = prop_fname;
            XFreeFontInfo(fn_list, fs_list, list_num);
            return True;
        }
    }

    font_data->xlfd_name = strdup(pattern);
    if(font_data->xlfd_name == NULL)
	return (-1);

    return True;
#endif /* OLDCODE */
}

static int
parse_omit_name(
    XOC		oc,
    FontData	font_data,
    char	*pattern)
{
    char*	last = (char *) NULL;
    char*	base_name;
    char	buf[XLFD_MAX_LEN + 1];
    int		length = 0;
    int		num_fields;
   /*
    * If the font specified by "pattern" is expandable to be
    * a member of "font_data"'s FontSet, we've found a match.
    */
    if(is_match_charset(font_data, pattern) == True) {
	if ((font_data->xlfd_name = get_font_name(oc, pattern)) != NULL) {
	    return True;
	}
    }

    length = strlen (pattern);

    if (length > XLFD_MAX_LEN)
	return -1;

    strcpy(buf, pattern);
    last = buf + length - 1;

    /* Replace the original encoding with the encoding for this FontSet. */

    /* Figure out how many fields have been specified in this xlfd. */
    for (num_fields = 0, base_name = buf; *base_name != '\0'; base_name++)
	if (*base_name == '-') num_fields++;

    switch (num_fields) {
    case 12:
	/* This is the best way to have specified the fontset.  In this
	 * case, there is no original encoding. E.g.,
         *       -*-*-*-*-*-*-14-*-*-*-*-*
	 * To this, we'll append a dash:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-
	 * then append the encoding to get:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-JISX0208.1990-0
	 */
	/*
	 * Take care of:
	 *       -*-*-*-*-*-*-14-*-*-*-*-
	 */
	if (*(last) == '-')
	    *++last = '*';

	*++last = '-';
	break;
    case 13:
	/* Got the charset, not the encoding, zap the charset  In this
	 * case, there is no original encoding, but there is a charset. E.g.,
         *       -*-*-*-*-*-*-14-*-*-*-*-*-jisx0212.1990
	 * To this, we remove the charset:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-
	 * then append the new encoding to get:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-JISX0208.1990-0
	 */
	last = strrchr (buf, '-');
	num_fields = 12;
	break;
    case 14:
	/* Both the charset and the encoding are specified.  Get rid
	 * of them so that we can append the new charset encoding.  E.g.,
         *       -*-*-*-*-*-*-14-*-*-*-*-*-jisx0212.1990-0
	 * To this, we'll remove the encoding and charset to get:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-
	 * then append the new encoding to get:
         *       -*-*-*-*-*-*-14-*-*-*-*-*-JISX0208.1990-0
	 */
	last = strrchr (buf, '-');
	*last = '\0';
	last = strrchr (buf, '-');
	num_fields = 12;
	break;
    default:
	if (*last != '-')
	    *++last = '-';
	break;
    }

   /* At this point, "last" is pointing to the last "-" in the
    * xlfd, and all xlfd's at this point take a form similar to:
    *       -*-*-*-*-*-*-14-*-*-*-*-*-
    * (i.e., no encoding).
    * After the strcpy, we'll end up with something similar to:
    *       -*-*-*-*-*-*-14-*-*-*-*-*-JISX0208.1990-0
    *
    * If the modified font is found in the current FontSet,
    * we've found a match.
    */

    last++;

    if ((last - buf) + strlen(font_data->name) > XLFD_MAX_LEN)
	return -1;

    strcpy(last, font_data->name);
    if ((font_data->xlfd_name = get_font_name(oc, buf)) != NULL)
	return True;

    /* This may not be needed anymore as XListFonts() takes care of this */
    if (num_fields < 12) {
	if ((last - buf) > (XLFD_MAX_LEN - 2))
	    return -1;
	*last = '*';
	*(last + 1) = '-';
	strcpy(last + 2, font_data->name);
	num_fields++;
	last+=2;
	if ((font_data->xlfd_name = get_font_name(oc, buf)) != NULL)
	    return True;
    }


    return False;
}


typedef enum{C_PRIMARY, C_SUBSTITUTE, C_VMAP, C_VROTATE } ClassType;

static int
parse_fontdata(
    XOC		 oc,
    FontSet      font_set,
    FontData	 font_data,
    int		 font_data_count,
    char	 **name_list,
    int		 name_list_count,
    ClassType	 class,
    FontDataRec *font_data_return)
{

    char	**cur_name_list = name_list;
    char	*font_name      = (char *) NULL;
    char	*pattern        = (char *) NULL;
    int		found_num       = 0, ret = 0;
    int		count           = name_list_count;

    if(name_list == NULL || count <= 0) {
	return False;
    }

    if(font_data == NULL || font_data_count <= 0) {
	return False;
    }

    /* Loop through each font encoding defined in the "font_data" FontSet. */
    for ( ; font_data_count-- > 0; font_data++) {
	Bool	is_found = False;
	font_name = (char *) NULL;
	count = name_list_count;
	cur_name_list = name_list;

       /*
	* Loop through each font specified by the user
	* in the call to XCreateFontset().
	*/
	while (count-- > 0) {
            pattern = *cur_name_list++;
	    if (pattern == NULL || *pattern == '\0')
		continue;
#ifdef FONTDEBUG
		fprintf(stderr,"Font pattern: %s %s\n",
		pattern,font_data->name);
#endif

	    /*
	     * If the current font is fully specified (i.e., the
	     * xlfd contains no wildcards) and the font exists on
	     * the X Server, we have a match.
	     */
	    if (strchr(pattern, '*') == NULL &&
		(font_name = get_font_name(oc, pattern))) {
               /*
		* Find the full xlfd name for this font. If the font is
		* already in xlfd format, it is simply returned.  If the
		* font is an alias for another font, the xlfd of the
		* aliased font is returned.
		*/
		ret = parse_all_name(oc, font_data, font_name);
		Xfree(font_name);

                if (ret == -1)    return -1;
	        if (ret == False) continue;
               /*
		* Since there was an exact match of a fully-specified font
		* or a font alias, we can return now since the desired font
		* was found for the current font encoding for this FontSet.
		*
		* Previous implementations of this algorithm would
		* not return here. Instead, they continued searching
		* through the font encodings for this FontSet. The side-effect
		* of that behavior is you may return a "substitute" match
		* instead of an "exact" match.  We believe there should be a
		* preference on exact matches.  Therefore, as soon as we
		* find one, we bail.
		*
		* Also, previous implementations seemed to think it was
		* important to find either a primary or substitute font
		* for each Font encoding in the FontSet before returning an
		* acceptable font.  We don't believe this is necessary.
		* All the client cares about is finding a reasonable font
		* for what was passed in.  If we find an exact match,
		* there's no reason to look any further.
		*
		* -- jjw/pma (HP)
		*/
		if (font_data_return) {
		    font_data_return->xlfd_name = strdup(font_data->xlfd_name);
		    if (!font_data_return->xlfd_name) return -1;

		    font_data_return->side      = font_data->side;
		}
#ifdef FONTDEBUG
		fprintf(stderr,"XLFD name: %s\n",font_data->xlfd_name);
#endif

		return True;
	    }
	    /*
	     * If the font name is not fully specified
	     * (i.e., it has wildcards), we have more work to do.
	     * See the comments in parse_omit_name()
	     * for the list of things to do.
	     */
	    ret = parse_omit_name(oc, font_data, pattern);

            if (ret == -1)    return -1;
	    if (ret == False) continue;

           /*
	    * A font which matched the wild-carded specification was found.
	    * Only update the return data if a font has not yet been found.
	    * This maintains the convention that FontSets listed higher in
	    * a CodeSet in the Locale Database have higher priority than
	    * those FontSets listed lower in the CodeSet.  In the following
	    * example:
	    *
	    * fs1 {
	    *        charset     HP-JIS:GR
	    *        font        JISX0208.1990-0:GL;\
	    *                    JISX0208.1990-1:GR;\
	    *                    JISX0208.1983-0:GL;\
	    *                    JISX0208.1983-1:GR
	    * }
	    *
	    * a font found in the JISX0208.1990-0 FontSet will have a
	    * higher priority than a font found in the JISX0208.1983-0
	    * FontSet.
	    */
	    if (font_data_return && font_data_return->xlfd_name == NULL) {

#ifdef FONTDEBUG
		fprintf(stderr,"XLFD name: %s\n",font_data->xlfd_name);
#endif
		font_data_return->xlfd_name = strdup(font_data->xlfd_name);
                if (!font_data_return->xlfd_name) return -1;

	        font_data_return->side      = font_data->side;
	    }

	    found_num++;
	    is_found = True;

	    break;
	}

	switch(class) {
	  case C_PRIMARY:
	       if(is_found == False) {
		 /*
		  * Did not find a font for the current FontSet.  Check the
		  * FontSet's "substitute" font for a match.  If we find a
		  * match, we'll keep searching in hopes of finding an exact
		  * match later down the FontSet list.
		  *
		  * when we return and we have found a font font_data_return
		  * contains the first (ie. best) match no matter if this
		  * is a C_PRIMARY or a C_SUBSTITUTE font
		  */
		  ret = parse_fontdata(oc, font_set, font_set->substitute,
				       font_set->substitute_num, name_list,
				       name_list_count, C_SUBSTITUTE,
				       font_data_return);
                  if (ret == -1)    return -1;
		  if (ret == False) continue;

		  found_num++;
		  is_found = True;
               }
#ifdef TESTVERSION
	       else
		   return True;
#endif
	       break;

	  case C_SUBSTITUTE:
	  case C_VMAP:
	       if(is_found == True)
		  return True;
	       break;

	  case C_VROTATE:
	       if(is_found == True) {
		  char	*rotate_name;

		  if((rotate_name = get_rotate_fontname(font_data->xlfd_name))
		     != NULL) {
		      Xfree(font_data->xlfd_name);
		      font_data->xlfd_name = rotate_name;

		      return True;
		  }
		  Xfree(font_data->xlfd_name);
		  font_data->xlfd_name = NULL;
		  return False;
	       }
	       break;
	}
    }

    if(class == C_PRIMARY && found_num >= 1)
	return True;

    return False;
}


static int
parse_vw(
    XOC		oc,
    FontSet	font_set,
    char	**name_list,
    int		count)
{
    FontData	vmap = font_set->vmap;
    VRotate	vrotate = font_set->vrotate;
    int		vmap_num = font_set->vmap_num;
    int		vrotate_num = font_set->vrotate_num;
    int		ret = 0, i = 0;

    if(vmap_num > 0) {
	if(parse_fontdata(oc, font_set, vmap, vmap_num, name_list,
			  count, C_VMAP,NULL) == -1)
	    return (-1);
    }

    if(vrotate_num > 0) {
	ret = parse_fontdata(oc, font_set, (FontData) vrotate, vrotate_num,
			     name_list, count, C_VROTATE, NULL);
	if(ret == -1) {
	    return (-1);
	} else if(ret == False) {
	    CodeRange	code_range;
	    int		num_cr;
	    int		sub_num = font_set->substitute_num;

	    code_range = vrotate[0].code_range; /* ? */
	    num_cr = vrotate[0].num_cr;         /* ? */
	    for(i = 0 ; i < vrotate_num ; i++) {
		if(vrotate[i].xlfd_name)
		    Xfree(vrotate[i].xlfd_name);
	    }
	    Xfree(vrotate);

	    if(sub_num > 0) {
		vrotate = font_set->vrotate = Xcalloc(sub_num,
                                                      sizeof(VRotateRec));
		if(font_set->vrotate == (VRotate)NULL)
		    return (-1);

		for(i = 0 ; i < sub_num ; i++) {
		    vrotate[i].charset_name = font_set->substitute[i].name;
		    vrotate[i].side = font_set->substitute[i].side;
		    vrotate[i].code_range = code_range;
		    vrotate[i].num_cr = num_cr;
		}
		vrotate_num = font_set->vrotate_num = sub_num;
	    } else {
		vrotate = font_set->vrotate = (VRotate)NULL;
	    }

	    ret = parse_fontdata(oc, font_set, (FontData) vrotate, vrotate_num,
				 name_list, count, C_VROTATE, NULL);
	    if(ret == -1)
		return (-1);
	}
    }

    return True;
}

static int
parse_fontname(
    XOC oc)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet     font_set;
    FontDataRec font_data_return;
    char *base_name, **name_list;
    int font_set_num = 0;
    int found_num = 0;
    int count = 0;
    int	ret;
    int i;

    name_list = _XParseBaseFontNameList(oc->core.base_name_list, &count);
    if (name_list == NULL)
	return -1;

    font_set = gen->font_set;
    font_set_num = gen->font_set_num;

   /* Loop through all of the CharSets defined in the Locale
    * database for the current Locale.
    */
    for( ; font_set_num-- > 0 ; font_set++) {
	if(font_set->font_name)
	    continue;

	if(font_set->font_data_count > 0) {

           /*
	    * If there are a non-zero number of FontSets defined
	    * for this CharSet.
            * Try to find a font for this CharSet.  If we find an
	    * acceptable font, we save the information for return
	    * to the client.  If we do not find an acceptable font,
	    * a "missing_charset" will be reported to the client
	    * for this CharSet.
	    */
	    font_data_return.xlfd_name  = NULL;
	    font_data_return.side       = XlcUnknown;

	    ret = parse_fontdata(oc, font_set, font_set->font_data,
				 font_set->font_data_count,
				 name_list, count, C_PRIMARY,
				 &font_data_return);
	    if(ret == -1) {
		goto err;
	    } else if(ret == True) {
		/*
		 * We can't just loop through fontset->font_data to
		 * find the first (ie. best) match: parse_fontdata
		 * will try a substitute font if no primary one could
		 * be matched. It returns the required information in
		 * font_data_return.
		 */
		font_set->font_name = strdup(font_data_return.xlfd_name);
		if(font_set->font_name == (char *) NULL)
		    goto err;

		font_set->side = font_data_return.side;

                Xfree (font_data_return.xlfd_name);
                font_data_return.xlfd_name = NULL;

		if(parse_vw(oc, font_set, name_list, count) == -1)
		    goto err;
		found_num++;
	    }

	} else if(font_set->substitute_num > 0) {
           /*
	    * If there are no FontSets defined for this
	    * CharSet.  We can only find "substitute" fonts.
	    */
	    ret = parse_fontdata(oc, font_set, font_set->substitute,
				 font_set->substitute_num,
				 name_list, count, C_SUBSTITUTE, NULL);
	    if(ret == -1) {
		goto err;
	    } else if(ret == True) {
		for(i=0;i<font_set->substitute_num;i++){
		    if(font_set->substitute[i].xlfd_name != NULL){
			break;
		    }
		}
		font_set->font_name = strdup(font_set->substitute[i].xlfd_name);
		if(font_set->font_name == (char *) NULL)
		    goto err;

		font_set->side = font_set->substitute[i].side;
		if(parse_vw(oc, font_set, name_list, count) == -1)
		    goto err;

		found_num++;
	    }
	}
    }

    base_name = strdup(oc->core.base_name_list);
    if (base_name == NULL)
	goto err;

    oc->core.base_name_list = base_name;

    XFreeStringList(name_list);

    return found_num;

err:
    XFreeStringList(name_list);
    /* Prevent this from being freed twice */
    oc->core.base_name_list = NULL;

    return -1;
}

/* For VW/UDC end*/

static Bool
set_missing_list(
    XOC oc)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet font_set;
    char **charset_list, *charset_buf;
    int	count, length, font_set_num;
    int result = 1;

    font_set = gen->font_set;
    font_set_num = gen->font_set_num;
    count = length = 0;

    for ( ; font_set_num-- > 0; font_set++) {
	if (font_set->info || font_set->font) {
	    continue;
	}

	/* Change 1996.01.23 start */
	if(font_set->font_data_count <= 0 ||
	   font_set->font_data == (FontData)NULL) {
	    if(font_set->substitute_num <= 0 ||
	       font_set->substitute == (FontData)NULL) {
		if(font_set->charset_list != NULL){
		 length +=
		  strlen(font_set->charset_list[0]->encoding_name) + 1;
		} else {
		  length += 1;
		}
	    } else {
		length += strlen(font_set->substitute->name) + 1;
	    }
	} else {
	    length += strlen(font_set->font_data->name) + 1;
	}
	/* Change 1996.01.23 end */
	count++;
    }

    if (count < 1) {
	return True;
    }

    charset_list = Xmalloc(sizeof(char *) * count);
    if (charset_list == NULL) {
	return False;
    }

    charset_buf = Xmalloc(length);
    if (charset_buf == NULL) {
	Xfree(charset_list);
	return False;
    }

    oc->core.missing_list.charset_list = charset_list;
    oc->core.missing_list.charset_count = count;

    font_set = gen->font_set;
    font_set_num = gen->font_set_num;

    for ( ; font_set_num-- > 0; font_set++) {
	if (font_set->info || font_set->font) {
	    continue;
	}

	/* Change 1996.01.23 start */
	if(font_set->font_data_count <= 0 ||
	   font_set->font_data == (FontData)NULL) {
	    if(font_set->substitute_num <= 0 ||
	       font_set->substitute == (FontData)NULL) {
		if(font_set->charset_list != NULL){
		 strcpy(charset_buf,
			font_set->charset_list[0]->encoding_name);
		} else {
		 strcpy(charset_buf, "");
		}
		result = 0;
	    } else {
		strcpy(charset_buf, font_set->substitute->name);
	    }
	} else {
	    strcpy(charset_buf, font_set->font_data->name);
	}
	/* Change 1996.01.23 end */
	*charset_list++ = charset_buf;
	charset_buf += strlen(charset_buf) + 1;
    }

    if(result == 0) {
	return(False);
    }

    return True;
}

static Bool
create_fontset(
    XOC oc)
{
    XOMGenericPart *gen = XOM_GENERIC(oc->core.om);
    int found_num;

    if (init_fontset(oc) == False)
        return False;

    found_num = parse_fontname(oc);
    if (found_num <= 0) {
	if (found_num == 0)
	    set_missing_list(oc);
	return False;
    }

    if (gen->on_demand_loading == True) {
	if (load_font_info(oc) == False)
	    return False;
    } else {
	if (load_font(oc) == False)
	    return False;
    }

    if (init_core_part(oc) == False)
	return False;

    if (set_missing_list(oc) == False)
	return False;

    return True;
}

/* For VW/UDC start */
static void
free_fontdataOC(
    Display	*dpy,
    FontData	font_data,
    int		font_data_count)
{
    for( ; font_data_count-- ; font_data++) {
	if(font_data->xlfd_name){
	    Xfree(font_data->xlfd_name);
	    font_data->xlfd_name = NULL;
	}
	if(font_data->font){				/* ADD 1996.01.7 */
	    if(font_data->font->fid)			/* Add 1996.01.23 */
		XFreeFont(dpy,font_data->font);		/* ADD 1996.01.7 */
	    else					/* Add 1996.01.23 */
		XFreeFontInfo(NULL, font_data->font, 1);/* Add 1996.01.23 */
	    font_data->font = NULL;
	}
/*
 * font_data->name and font_data->scopes belong to the OM not OC.
 * To save space this data is shared between OM and OC. We are
 * not allowed to free it here.
 * It has been moved to free_fontdataOM()
 */
/*
	if(font_data->scopes){
	    Xfree(font_data->scopes);
	    font_data->scopes = NULL;
	}
	if(font_data->name){
	    Xfree(font_data->name);
	    font_data->name = NULL;
	}
*/
    }
}

static void destroy_fontdata(
    XOCGenericPart *gen,
    Display *dpy)
{
    FontSet	font_set = (FontSet) NULL;
    int		font_set_num = 0;

    if (gen->font_set) {
	font_set = gen->font_set;
	font_set_num = gen->font_set_num;
	for( ; font_set_num-- ; font_set++) {
	    if (font_set->font) {
		if(font_set->font->fid)
		    XFreeFont(dpy,font_set->font);
		else
		    XFreeFontInfo(NULL, font_set->font, 1);
		font_set->font = NULL;
	    }
	    if(font_set->font_data) {
		if (font_set->info)
		    XFreeFontInfo(NULL, font_set->info, 1);
		free_fontdataOC(dpy,
			font_set->font_data, font_set->font_data_count);
		Xfree(font_set->font_data);
		font_set->font_data = NULL;
	    }
	    if(font_set->substitute) {
		free_fontdataOC(dpy,
			font_set->substitute, font_set->substitute_num);
		Xfree(font_set->substitute);
		font_set->substitute = NULL;
	    }
	    if(font_set->vmap) {
		free_fontdataOC(dpy,
			font_set->vmap, font_set->vmap_num);
		Xfree(font_set->vmap);
		font_set->vmap = NULL;
	    }
	    if(font_set->vrotate) {
		free_fontdataOC(dpy,
			(FontData)font_set->vrotate,
			      font_set->vrotate_num);
		Xfree(font_set->vrotate);
		font_set->vrotate = NULL;
	    }
	}
	Xfree(gen->font_set);
	gen->font_set = NULL;
    }
}
/* For VW/UDC end */

static void
destroy_oc(
    XOC oc)
{
    Display *dpy = oc->core.om->core.display;
    XOCGenericPart *gen = XOC_GENERIC(oc);

    if (gen->mbs_to_cs)
	_XlcCloseConverter(gen->mbs_to_cs);

    if (gen->wcs_to_cs)
	_XlcCloseConverter(gen->wcs_to_cs);

    if (gen->utf8_to_cs)
	_XlcCloseConverter(gen->utf8_to_cs);

/* For VW/UDC start */ /* Change 1996.01.8 */
    destroy_fontdata(gen,dpy);
/*
*/
/* For VW/UDC end */

    Xfree(oc->core.base_name_list);
    XFreeStringList(oc->core.font_info.font_name_list);
    Xfree(oc->core.font_info.font_struct_list);
    XFreeStringList(oc->core.missing_list.charset_list);

#ifdef notdef

    Xfree(oc->core.res_name);
    Xfree(oc->core.res_class);
#endif

    Xfree(oc);
}

static char *
set_oc_values(
    XOC oc,
    XlcArgList args,
    int num_args)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    FontSet font_set = gen->font_set;
    char *ret;
    int num = gen->font_set_num;

    if (oc->core.resources == NULL)
	return NULL;

    ret = _XlcSetValues((XPointer) oc, oc->core.resources,
			oc->core.num_resources, args, num_args, XlcSetMask);
    if(ret != NULL){
	return(ret);
    } else {
	for ( ; num-- > 0; font_set++) {
	    if (font_set->font_name == NULL)
	        continue;
	    if (font_set->vpart_initialize != 0)
	        continue;
	    if( oc->core.orientation == XOMOrientation_TTB_RTL ||
		oc->core.orientation == XOMOrientation_TTB_LTR ){
	    	load_fontdata(oc, font_set->vmap, font_set->vmap_num);
		load_fontdata(oc, (FontData) font_set->vrotate,
			    font_set->vrotate_num);
		font_set->vpart_initialize = 1;
	    }
	}
	return(NULL);
    }
}

static char *
get_oc_values(
    XOC oc,
    XlcArgList args,
    int num_args)
{
    if (oc->core.resources == NULL)
	return NULL;

    return _XlcGetValues((XPointer) oc, oc->core.resources,
			 oc->core.num_resources, args, num_args, XlcGetMask);
}

static XOCMethodsRec oc_default_methods = {
    destroy_oc,
    set_oc_values,
    get_oc_values,
    _XmbDefaultTextEscapement,
    _XmbDefaultTextExtents,
    _XmbDefaultTextPerCharExtents,
    _XmbDefaultDrawString,
    _XmbDefaultDrawImageString,
    _XwcDefaultTextEscapement,
    _XwcDefaultTextExtents,
    _XwcDefaultTextPerCharExtents,
    _XwcDefaultDrawString,
    _XwcDefaultDrawImageString,
    _Xutf8DefaultTextEscapement,
    _Xutf8DefaultTextExtents,
    _Xutf8DefaultTextPerCharExtents,
    _Xutf8DefaultDrawString,
    _Xutf8DefaultDrawImageString
};

static XOCMethodsRec oc_generic_methods = {
    destroy_oc,
    set_oc_values,
    get_oc_values,
    _XmbGenericTextEscapement,
    _XmbGenericTextExtents,
    _XmbGenericTextPerCharExtents,
    _XmbGenericDrawString,
    _XmbGenericDrawImageString,
    _XwcGenericTextEscapement,
    _XwcGenericTextExtents,
    _XwcGenericTextPerCharExtents,
    _XwcGenericDrawString,
    _XwcGenericDrawImageString,
    _Xutf8GenericTextEscapement,
    _Xutf8GenericTextExtents,
    _Xutf8GenericTextPerCharExtents,
    _Xutf8GenericDrawString,
    _Xutf8GenericDrawImageString
};

typedef struct _XOCMethodsListRec {
    const char *name;
    XOCMethods methods;
} XOCMethodsListRec, *XOCMethodsList;

static XOCMethodsListRec oc_methods_list[] = {
    { "default", &oc_default_methods },
    { "generic", &oc_generic_methods }
};

static XlcResource oc_resources[] = {
    { XNBaseFontName, NULLQUARK, sizeof(char *),
      XOffsetOf(XOCRec, core.base_name_list), XlcCreateMask | XlcGetMask },
    { XNOMAutomatic, NULLQUARK, sizeof(Bool),
      XOffsetOf(XOCRec, core.om_automatic), XlcGetMask },
    { XNMissingCharSet, NULLQUARK, sizeof(XOMCharSetList),
      XOffsetOf(XOCRec, core.missing_list), XlcGetMask },
    { XNDefaultString, NULLQUARK, sizeof(char *),
      XOffsetOf(XOCRec, core.default_string), XlcGetMask },
    { XNOrientation, NULLQUARK, sizeof(XOrientation),
      XOffsetOf(XOCRec, core.orientation), XlcDefaultMask | XlcSetMask | XlcGetMask },
    { XNResourceName, NULLQUARK, sizeof(char *),
      XOffsetOf(XOCRec, core.res_name), XlcSetMask | XlcGetMask },
    { XNResourceClass, NULLQUARK, sizeof(char *),
      XOffsetOf(XOCRec, core.res_class), XlcSetMask | XlcGetMask },
    { XNFontInfo, NULLQUARK, sizeof(XOMFontInfo),
      XOffsetOf(XOCRec, core.font_info), XlcGetMask }
};

static XOC
create_oc(
    XOM om,
    XlcArgList args,
    int num_args)
{
    XOC oc;
    XOMGenericPart *gen = XOM_GENERIC(om);
    XOCMethodsList methods_list = oc_methods_list;
    int count;

    oc = Xcalloc(1, sizeof(XOCGenericRec));
    if (oc == NULL)
	return (XOC) NULL;

    oc->core.om = om;

    if (oc_resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(oc_resources, XlcNumber(oc_resources));

    if (_XlcSetValues((XPointer) oc, oc_resources, XlcNumber(oc_resources),
		      args, num_args, XlcCreateMask | XlcDefaultMask))
	goto err;

    if (oc->core.base_name_list == NULL)
	goto err;

    oc->core.resources = oc_resources;
    oc->core.num_resources = XlcNumber(oc_resources);

    if (create_fontset(oc) == False)
	goto err;

    oc->methods = &oc_generic_methods;

    if (gen->object_name) {
	count = XlcNumber(oc_methods_list);

	for ( ; count-- > 0; methods_list++) {
	    if (!_XlcCompareISOLatin1(gen->object_name, methods_list->name)) {
		oc->methods = methods_list->methods;
		break;
	    }
	}
    }

    return oc;

err:
    destroy_oc(oc);

    return (XOC) NULL;
}

static void
free_fontdataOM(
    FontData	font_data,
    int		font_data_count)
{
    if (!font_data)
	return;

    for( ; font_data_count-- ; font_data++) {
	    Xfree(font_data->name);
	    font_data->name = NULL;
	    Xfree(font_data->scopes);
	    font_data->scopes = NULL;
    }
}

static Status
close_om(
    XOM om)
{
    XOMGenericPart *gen = XOM_GENERIC(om);
    OMData data;
    int count;

    if ((data = gen->data)) {
	for (count = gen->data_num; count-- > 0; data++) {
		Xfree(data->charset_list);
		data->charset_list = NULL;
	
	    /* free font_data for om */
		free_fontdataOM(data->font_data,data->font_data_count);
		Xfree(data->font_data);
		data->font_data = NULL;

	    /* free substitute for om */
		free_fontdataOM(data->substitute,data->substitute_num);
		Xfree(data->substitute);
		data->substitute = NULL;

	    /* free vmap for om */
		free_fontdataOM(data->vmap,data->vmap_num);
		Xfree(data->vmap);
		data->vmap = NULL;

	    /* free vrotate for om */
		Xfree(data->vrotate);
		data->vrotate = NULL;
	}
	Xfree(gen->data);
	gen->data = NULL;
    }

	Xfree(gen->object_name);
	gen->object_name = NULL;

	Xfree(om->core.res_name);
	om->core.res_name = NULL;

	Xfree(om->core.res_class);
	om->core.res_class = NULL;

    if (om->core.required_charset.charset_list &&
	om->core.required_charset.charset_count > 0){
	XFreeStringList(om->core.required_charset.charset_list);
	om->core.required_charset.charset_list = NULL;
    } else {
	Xfree((char*)om->core.required_charset.charset_list);
	om->core.required_charset.charset_list = NULL;
    }

	Xfree(om->core.orientation_list.orientation);
	om->core.orientation_list.orientation = NULL;

    Xfree(om);

    return 1;
}

static char *
set_om_values(
    XOM om,
    XlcArgList args,
    int num_args)
{
    if (om->core.resources == NULL)
	return NULL;

    return _XlcSetValues((XPointer) om, om->core.resources,
			 om->core.num_resources, args, num_args, XlcSetMask);
}

static char *
get_om_values(
    XOM om,
    XlcArgList args,
    int num_args)
{
    if (om->core.resources == NULL)
	return NULL;

    return _XlcGetValues((XPointer) om, om->core.resources,
			 om->core.num_resources, args, num_args, XlcGetMask);
}

static XOMMethodsRec methods = {
    close_om,
    set_om_values,
    get_om_values,
    create_oc
};

static XlcResource om_resources[] = {
    { XNRequiredCharSet, NULLQUARK, sizeof(XOMCharSetList),
      XOffsetOf(XOMRec, core.required_charset), XlcGetMask },
    { XNQueryOrientation, NULLQUARK, sizeof(XOMOrientation),
      XOffsetOf(XOMRec, core.orientation_list), XlcGetMask },
    { XNDirectionalDependentDrawing, NULLQUARK, sizeof(Bool),
      XOffsetOf(XOMRec, core.directional_dependent), XlcGetMask },
    { XNContextualDrawing, NULLQUARK, sizeof(Bool),
      XOffsetOf(XOMRec, core.contextual_drawing), XlcGetMask }
};

static XOM
create_om(
    XLCd lcd,
    Display *dpy,
    XrmDatabase rdb,
    _Xconst char *res_name,
    _Xconst char *res_class)
{
    XOM om;

    om = Xcalloc(1, sizeof(XOMGenericRec));
    if (om == NULL)
	return (XOM) NULL;

    om->methods = &methods;
    om->core.lcd = lcd;
    om->core.display = dpy;
    om->core.rdb = rdb;
    if (res_name) {
	om->core.res_name = strdup(res_name);
	if (om->core.res_name == NULL)
	    goto err;
    }
    if (res_class) {
	om->core.res_class = strdup(res_class);
	if (om->core.res_class == NULL)
	    goto err;
    }

    if (om_resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(om_resources, XlcNumber(om_resources));

    om->core.resources = om_resources;
    om->core.num_resources = XlcNumber(om_resources);

    return om;

err:
    close_om(om);

    return (XOM) NULL;
}

static OMData
add_data(
    XOM om)
{
    XOMGenericPart *gen = XOM_GENERIC(om);
    OMData new;
    int num;

    if ((num = gen->data_num))
        new = Xrealloc(gen->data, (num + 1) * sizeof(OMDataRec));
    else
        new = Xmalloc(sizeof(OMDataRec));

    if (new == NULL)
        return NULL;

    gen->data_num = num + 1;
    gen->data = new;

    new += num;
    bzero((char *) new, sizeof(OMDataRec));

    return new;
}

/* For VW/UDC */

FontData
read_EncodingInfo(
    int count,
    char **value)
{
    FontData font_data,ret;
    char *buf, *bufptr,*scp;
    int len, i;
    font_data = Xcalloc(count, sizeof(FontDataRec));
    if (font_data == NULL)
        return NULL;

    ret = font_data;
    for (i = 0; i < count; i++, font_data++) {
/*
        strcpy(buf, *value++);
*/
	buf = *value; value++;
        if ((bufptr = strchr(buf, ':'))) {
	    len = (int)(bufptr - buf);
            bufptr++ ;
	} else
            len = strlen(buf);
        font_data->name = Xmalloc(len + 1);
        if (font_data->name == NULL) {
            free_fontdataOM(ret, i + 1);
            Xfree(ret);
            return NULL;
	}
        strncpy(font_data->name, buf,len);
	font_data->name[len] = 0;
        if (bufptr && _XlcCompareISOLatin1(bufptr, "GL") == 0)
            font_data->side = XlcGL;
        else if (bufptr && _XlcCompareISOLatin1(bufptr, "GR") == 0)
            font_data->side = XlcGR;
        else
            font_data->side = XlcGLGR;

        if (bufptr && (scp = strchr(bufptr, '['))){
            font_data->scopes = _XlcParse_scopemaps(scp,&(font_data->scopes_num));
        }
    }
    return(ret);
}

static CodeRange read_vrotate(
    int count,
    char **value,
    int *type,
    int *vrotate_num)
{
    CodeRange   range;
    if(!strcmp(value[0],"all")){
	*type 	     = VROTATE_ALL ;
	*vrotate_num = 0 ;
	return (NULL);
    } else if(*(value[0]) == '['){
	*type 	     = VROTATE_PART ;
        range = (CodeRange) _XlcParse_scopemaps(value[0],vrotate_num);
	return (range);
    } else {
	*type 	     = VROTATE_NONE ;
	*vrotate_num = 0 ;
	return (NULL);
    }
}

static void read_vw(
    XLCd    lcd,
    OMData  font_set,
    int     num)
{
    char **value, buf[BUFSIZ];
    int count;

    snprintf(buf, sizeof(buf), "fs%d.font.vertical_map", num);
    _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
    if (count > 0){
        _XlcDbg_printValue(buf,value,count);
        font_set->vmap_num = count;
        font_set->vmap = read_EncodingInfo(count,value);
    }

    snprintf(buf, sizeof(buf), "fs%d.font.vertical_rotate", num);
    _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
    if (count > 0){
        _XlcDbg_printValue(buf,value,count);
        font_set->vrotate = read_vrotate(count,value,&(font_set->vrotate_type),
				&(font_set->vrotate_num));
    }
}
/* VW/UDC end */
static Bool
init_om(
    XOM om)
{
    XLCd lcd = om->core.lcd;
    XOMGenericPart *gen = XOM_GENERIC(om);
    OMData data;
    XlcCharSet *charset_list;
    FontData font_data;
    char **required_list;
    XOrientation *orientation;
    char **value, buf[BUFSIZ], *bufptr;
    int count = 0, num = 0;
    unsigned int length = 0;

    _XlcGetResource(lcd, "XLC_FONTSET", "on_demand_loading", &value, &count);
    if (count > 0 && _XlcCompareISOLatin1(*value, "True") == 0)
	gen->on_demand_loading = True;

    _XlcGetResource(lcd, "XLC_FONTSET", "object_name", &value, &count);
    if (count > 0) {
	gen->object_name = strdup(*value);
	if (gen->object_name == NULL)
	    return False;
    }

    for (num = 0; ; num++) {

        snprintf(buf, sizeof(buf), "fs%d.charset.name", num);
        _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);

        if( count < 1){
            snprintf(buf, sizeof(buf), "fs%d.charset", num);
            _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
            if (count < 1)
                break;
        }

	data = add_data(om);
	if (data == NULL)
	    return False;

	charset_list = Xmalloc(sizeof(XlcCharSet) * count);
	if (charset_list == NULL)
	    return False;
	data->charset_list = charset_list;
	data->charset_count = count;

	while (count-- > 0){
	    *charset_list++ = _XlcGetCharSet(*value++);
        }
        snprintf(buf, sizeof(buf), "fs%d.charset.udc_area", num);
        _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
        if( count > 0){
            UDCArea udc;
            int i,flag = 0;
            udc = Xmalloc(count * sizeof(UDCAreaRec));
	    if (udc == NULL)
	        return False;
            for(i=0;i<count;i++){
                sscanf(value[i],"\\x%lx,\\x%lx", &(udc[i].start),
		       &(udc[i].end));
            }
            for(i=0;i<data->charset_count;i++){
		if(data->charset_list[i]->udc_area == NULL){
		    data->charset_list[i]->udc_area     = udc;
		    data->charset_list[i]->udc_area_num = count;
		    flag = 1;
		}
            }
	    if(flag == 0){
		Xfree(udc);
	    }
        }

        snprintf(buf, sizeof(buf), "fs%d.font.primary", num);
        _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
        if (count < 1){
            snprintf(buf, sizeof(buf), "fs%d.font", num);
            _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
            if (count < 1)
                return False;
        }

	font_data = read_EncodingInfo(count,value);
	if (font_data == NULL)
	    return False;

	data->font_data = font_data;
	data->font_data_count = count;

        snprintf(buf, sizeof(buf), "fs%d.font.substitute", num);
        _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
        if (count > 0){
            font_data = read_EncodingInfo(count,value);
            if (font_data == NULL)
	        return False;
            data->substitute      = font_data;
            data->substitute_num = count;
        } else {
            snprintf(buf, sizeof(buf), "fs%d.font", num);
            _XlcGetResource(lcd, "XLC_FONTSET", buf, &value, &count);
            if (count < 1) {
                data->substitute      = NULL;
                data->substitute_num = 0;
	    } else {
                font_data = read_EncodingInfo(count,value);
                data->substitute      = font_data;
                data->substitute_num = count;
	    }
	}
        read_vw(lcd,data,num);
	length += strlen(data->font_data->name) + 1;
    }

    /* required charset list */
    required_list = Xmalloc(sizeof(char *) * gen->data_num);
    if (required_list == NULL)
	return False;

    om->core.required_charset.charset_list = required_list;
    om->core.required_charset.charset_count = gen->data_num;

    count = gen->data_num;
    data = gen->data;

    if (count > 0) {
	bufptr = Xmalloc(length);
	if (bufptr == NULL) {
	    Xfree(required_list);
	    return False;
	}

	for ( ; count-- > 0; data++) {
	    strcpy(bufptr, data->font_data->name);
	    *required_list++ = bufptr;
	    bufptr += strlen(bufptr) + 1;
	}
    }

    /* orientation list */
    orientation = Xmalloc(sizeof(XOrientation) * 2);
    if (orientation == NULL)
	return False;

    orientation[0] = XOMOrientation_LTR_TTB;
    orientation[1] = XOMOrientation_TTB_RTL;
    om->core.orientation_list.orientation = orientation;
    om->core.orientation_list.num_orientation = 2;

    /* directional dependent drawing */
    om->core.directional_dependent = False;

    /* contextual drawing */
    om->core.contextual_drawing = False;

    /* context dependent */
    om->core.context_dependent = False;

    return True;
}

XOM
_XomGenericOpenOM(XLCd lcd, Display *dpy, XrmDatabase rdb,
		  _Xconst char *res_name, _Xconst char *res_class)
{
    XOM om;

    om = create_om(lcd, dpy, rdb, res_name, res_class);
    if (om == NULL)
	return (XOM) NULL;

    if (init_om(om) == False)
	goto err;

    return om;

err:
    close_om(om);

    return (XOM) NULL;
}

Bool
_XInitOM(
    XLCd lcd)
{
    lcd->methods->open_om = _XomGenericOpenOM;

    return True;
}
