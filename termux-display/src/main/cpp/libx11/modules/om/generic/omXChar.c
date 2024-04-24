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
 * Modifier: Takanori Tateno   FUJITSU LIMITED
 *
 */
/*
 * Modifiers: Jeff Walls, Paul Anderson (HEWLETT-PACKARD)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XlcPublic.h"
#include "XomGeneric.h"
#include <stdio.h>

/* for VW/UDC start */
static Bool
ismatch_scopes(
    FontData      fontdata,
    unsigned long *value,
    Bool	  is_shift)
{
    register int scopes_num = fontdata->scopes_num;
    FontScope scopes = fontdata->scopes;
    if (!scopes_num)
        return False;

    if(fontdata->font == NULL)
	return False;

    for(;scopes_num--;scopes++)
        if ((scopes->start <= (*value & 0x7f7f)) &&
                        ((scopes->end) >= (*value & 0x7f7f))){
	    if(is_shift == True) {
                if(scopes->shift){
                    if(scopes->shift_direction == '+'){
                        *value += scopes->shift ;
                    } else if( scopes->shift_direction == '-'){
                        *value -= scopes->shift ;
                    }
                }
            }
            return True;
        }

    return False;
}

static int
check_vertical_fonttype(
    char	*name)
{
    char	*ptr;
    int		type = 0;

    if(name == (char *)NULL || (int) strlen(name) <= 0)
	return False;

    /* Obtains the pointer of CHARSET_ENCODING_FIELD. */
    if((ptr = strchr(name, '-')) == (char *) NULL)
	return False;
    ptr++;

    /* Obtains the pointer of vertical_map font type. */
    if((ptr = strchr(ptr, '.')) == (char *) NULL)
	return False;
    ptr++;

    switch(*ptr) {
      case '1':
	type = 1;	break;
      case '2':
	type = 2;	break;
      case '3':
	type = 3;	break;
    }
    return type;
}

/*
*/
#define VMAP          0
#define VROTATE       1
#define FONTSCOPE     2

FontData
_XomGetFontDataFromFontSet(
    FontSet fs,
    unsigned char *str,
    int len,
    int *len_ret,
    int is2b,
    int type)          /* VMAP , VROTATE , else */
{
    unsigned long value;
    int num,i,hit,csize;
    FontData fontdata;
    unsigned char *c;
    int vfont_type;

    c = str;
    hit = -1;
    if(type == VMAP){
	fontdata = fs->vmap;
	num      = fs->vmap_num;
    } else if(type == VROTATE){
        fontdata = (FontData)fs->vrotate;
	num      = fs->vrotate_num;
    } else {
	if(fs->font_data_count <= 0 || fs->font_data == (FontData)NULL) {
	    fontdata = fs->substitute;
	    num      = fs->substitute_num;
	}else {
            fontdata = fs->font_data;
	    num      = fs->font_data_count;
	}
	/* CDExc20229 fix */
	if(fontdata == NULL || num == 0){
	    return(NULL);
	}
    }


    if(is2b){
        csize = 2;
    } else {
        csize = 1;
    }

    for(;len;len--){
        if(is2b){
            value = (((unsigned long)*c) << 8)|(unsigned long)*(c + 1);
        } else {
            value = (unsigned long)*c;
        }

       /* ### NOTE: This routine DOES NOT WORK!
	* ###       We can work around the problem in the calling routine,
	* ###       but we really need to understand this better.  As it
	* ###       stands, the algorithm ALWAYS returns "fontdata[0]"
	* ###       for non-VW text!  This is clearly wrong.  In fact,
	* ###       given the new parse_font[name|data]() algorithms,
	* ###       we may not even need this routine to do anything
	* ###       for non-VW text (since font_set->font always contains
	* ###       the best font for this fontset).  -- jjw/pma (HP)
	*/
        for (i=0;i<num;i++) {
	    if(type == VROTATE) {
		if(fontdata[i].font) {
		    /* If the num_cr equal zero, all character is rotated. */
		    if(fontdata[i].scopes_num == 0) {
			break;
		    } else {
			/* The vertical rotate glyph is not have code shift. */
			if (ismatch_scopes(&(fontdata[i]),&value,False)) {
			    break;
			}
		    }
		}
	    } else if(type == VMAP) {
		if(fontdata[i].font) {
		    vfont_type = check_vertical_fonttype(fontdata[i].name);
		    if(vfont_type == 0 || vfont_type == 1) {
			break;
		    } else if(vfont_type == 2 || vfont_type == 3) {
			if(fontdata[i].scopes_num <= 0)
			    break;

			if (ismatch_scopes(&(fontdata[i]),&value,True)) {
			    break;
			}
		    }
		}
	    } else { /* FONTSCOPE */
		if(fontdata[i].font) {
		    if(fontdata[i].scopes_num <= 0)
                        break;
		    if (ismatch_scopes(&(fontdata[i]),&value,True)){
		        break;
                    }
		}
	    }
        }
        if((hit != -1) && (i != hit)){
            break;
        }
        if(i == num){
            if( type == VROTATE || type == VMAP){
		/* Change 1996.01.23 start */
		if(fs->font_data_count <= 0 ||
		   fs->font_data == (FontData)NULL)
		    fontdata = fs->substitute;
		else
		    fontdata = fs->font_data;
		/* Change 1996.01.23 end */
	    }
	    hit = 0;
            c += csize;
	    break;
        }
        if( hit == -1 ) hit = i;
        if(is2b){
            *c = (unsigned char)(value >> 8);
            *(c + 1) = (unsigned char)(value);
        } else {
            *c = (unsigned char)value;
        }
        c += csize;
    }
    *len_ret = (c - str);
    return(&(fontdata[hit]));
}
/* for VW/UDC end   */

static FontSet
_XomGetFontSetFromCharSet(
    XOC oc,
    XlcCharSet charset)
{
    register FontSet font_set = XOC_GENERIC(oc)->font_set;
    register int num = XOC_GENERIC(oc)->font_set_num;
    XlcCharSet *charset_list;
    int charset_count;

    for ( ; num-- > 0; font_set++) {
	charset_count = font_set->charset_count;
	charset_list = font_set->charset_list;
	for ( ; charset_count-- > 0; charset_list++)
	    if (*charset_list == charset)
		return font_set;
    }

    return (FontSet) NULL;
}

static void
shift_to_gl(
    register char *text,
    register int length)
{
    while (length-- > 0)
	*text++ &= 0x7f;
}

static void
shift_to_gr(
    register char *text,
    register int length)
{
    while (length-- > 0)
	*text++ |= 0x80;
}

static Bool
load_font(
    XOC oc,
    FontSet font_set)
{
    font_set->font = XLoadQueryFont(oc->core.om->core.display,
			oc->core.font_info.font_name_list[font_set->id]);
    if (font_set->font == NULL)
	return False;

    oc->core.font_info.font_struct_list[font_set->id] = font_set->font;
    XFreeFontInfo(NULL, font_set->info, 1);
    font_set->info = NULL;

    if (font_set->font->min_byte1 || font_set->font->max_byte1)
	font_set->is_xchar2b = True;
    else
	font_set->is_xchar2b = False;

    return True;
}

int
_XomConvert(
    XOC oc,
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    XPointer cs, lc_args[1];
    XlcCharSet charset;
    int length, cs_left, ret;
    FontSet font_set;

    cs = *to;
    cs_left = *to_left;
    lc_args[0] = (XPointer) &charset;

    ret = _XlcConvert(conv, from, from_left, &cs, &cs_left, lc_args, 1);
    if (ret < 0)
	return -1;

    font_set = _XomGetFontSetFromCharSet(oc, charset);
    if (font_set == NULL)
	return -1;

    if (font_set->font == NULL && load_font(oc, font_set) == False)
	return -1;

    length = *to_left - cs_left;

    if (font_set->side != charset->side) {
	if (font_set->side == XlcGL)
	    shift_to_gl(*to, length);
	else if (font_set->side == XlcGR)
	    shift_to_gr(*to, length);
    }

    if (font_set->is_xchar2b)
	length >>= 1;
    *to = cs;
    *to_left -= length;

    *((XFontStruct **) args[0]) = font_set->font;
    *((Bool *) args[1]) = font_set->is_xchar2b;
    if(num_args >= 3){
        *((FontSet *) args[2]) = font_set;
    }

    return ret;
}

XlcConv
_XomInitConverter(
    XOC oc,
    XOMTextType type)
{
    XOCGenericPart *gen = XOC_GENERIC(oc);
    XlcConv *convp;
    const char *conv_type;
    XlcConv conv;
    XLCd lcd;

    switch (type) {
    case XOMWideChar:
	convp = &gen->wcs_to_cs;
	conv_type = XlcNWideChar;
	break;
    case XOMMultiByte:
	convp = &gen->mbs_to_cs;
	conv_type = XlcNMultiByte;
	break;
    case XOMUtf8String:
	convp = &gen->utf8_to_cs;
	conv_type = XlcNUtf8String;
	break;
    default:
	return (XlcConv) NULL;
    }

    conv = *convp;
    if (conv) {
	_XlcResetConverter(conv);
	return conv;
    }

    lcd = oc->core.om->core.lcd;

    conv = _XlcOpenConverter(lcd, conv_type, lcd, XlcNFontCharSet);
    if (conv == (XlcConv) NULL) {
        conv = _XlcOpenConverter(lcd, conv_type, lcd, XlcNCharSet);
        if (conv == (XlcConv) NULL)
	    return (XlcConv) NULL;
    }

    *convp = conv;
    return conv;
}
