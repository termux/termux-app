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
 *  (c) Copyright 1995 FUJITSU LIMITED
 *  This is source code modified by FUJITSU LIMITED under the Joint
 *  Development Agreement for the CDE/Motif PST.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XlcGeneric.h"
#include "reallocarray.h"

static XLCd create (const char *name, XLCdMethods methods);
static Bool initialize (XLCd lcd);
static void destroy (XLCd lcd);

static XLCdPublicMethodsRec genericMethods = {
    { NULL },                   /* use default methods */
    {
	NULL,
	create,
	initialize,
	destroy,
	NULL
    }
};

XLCdMethods _XlcGenericMethods = (XLCdMethods) &genericMethods;

static XLCd
create(
    const char *name,
    XLCdMethods methods)
{
    XLCd lcd;
    XLCdPublicMethods new;

    lcd = Xcalloc(1, sizeof(XLCdRec));
    if (lcd == NULL)
        return (XLCd) NULL;

    lcd->core = Xcalloc(1, sizeof(XLCdGenericRec));
    if (lcd->core == NULL)
	goto err;

    new = Xmalloc(sizeof(XLCdPublicMethodsRec));
    if (new == NULL)
	goto err;
    memcpy(new,methods,sizeof(XLCdPublicMethodsRec));
    lcd->methods = (XLCdMethods) new;

    return lcd;

err:
    Xfree(lcd->core);
    Xfree(lcd);
    return (XLCd) NULL;
}

static Bool
string_to_encoding(
    const char *str,
    char *encoding)
{
    char *next;
    long value;
    int base;

    while (*str) {
	if (*str == '\\') {
	    switch (*(str + 1)) {
		case 'x':
		case 'X':
		    base = 16;
		    break;
		default:
		    base = 8;
		    break;
	    }
	    value = strtol(str + 2, &next, base);
	    if (str + 2 != next) {
		*((unsigned char *) encoding++) = (unsigned char) value;
		str = next;
		continue;
	    }
	}
	*encoding++ = *str++;
    }

    *encoding = '\0';

    return True;
}

static Bool
string_to_ulong(
    const char *str,
    unsigned long *value)
{
     const char *tmp1 = str;
     int base;

     if (*tmp1++ != '\\') {
	  tmp1--;
	  base = 10;
     } else {
	  switch (*tmp1++) {
	  case 'x':
	       base = 16;
	       break;
	  case 'o':
	       base = 8;
	       break;
	  case 'd':
	       base = 10;
	       break;
	  default:
	       return(False);
	  }
     }
     *value = (unsigned long) strtol(tmp1, NULL, base);
     return(True);
}


static Bool
add_charset(
    CodeSet codeset,
    XlcCharSet charset)
{
    XlcCharSet *new_list;
    int num;

    if ((num = codeset->num_charsets))
        new_list = Xreallocarray(codeset->charset_list,
                                 num + 1, sizeof(XlcCharSet));
    else
        new_list = Xmalloc(sizeof(XlcCharSet));

    if (new_list == NULL)
	return False;

    new_list[num] = charset;
    codeset->charset_list = new_list;
    codeset->num_charsets = num + 1;

    return True;
}

static CodeSet
add_codeset(
    XLCdGenericPart *gen)
{
    CodeSet new, *new_list;
    int num;

    new = Xcalloc(1, sizeof(CodeSetRec));
    if (new == NULL)
        return NULL;

    if ((num = gen->codeset_num))
        new_list = Xreallocarray(gen->codeset_list,
                                 num + 1, sizeof(CodeSet));
    else
        new_list = Xmalloc(sizeof(CodeSet));

    if (new_list == NULL)
        goto err;

    new_list[num] = new;
    gen->codeset_list = new_list;
    gen->codeset_num = num + 1;

    return new;

err:
    Xfree(new);

    return NULL;
}

static Bool
add_parse_list(
    XLCdGenericPart *gen,
    EncodingType type,
    const char *encoding,
    CodeSet codeset)
{
    ParseInfo new, *new_list;
    char *str;
    unsigned char ch;
    int num;

    str = strdup(encoding);
    if (str == NULL)
        return False;

    new = Xcalloc(1, sizeof(ParseInfoRec));
    if (new == NULL)
        goto err;

    if (gen->mb_parse_table == NULL) {
        gen->mb_parse_table = Xcalloc(1, 256); /* 2^8 */
        if (gen->mb_parse_table == NULL)
            goto err;
    }

    if ((num = gen->mb_parse_list_num))
        new_list = Xreallocarray(gen->mb_parse_list,
                                 num + 2, sizeof(ParseInfo));
    else {
        new_list = Xmalloc(2 * sizeof(ParseInfo));
    }

    if (new_list == NULL)
        goto err;

    new_list[num] = new;
    new_list[num + 1] = NULL;
    gen->mb_parse_list = new_list;
    gen->mb_parse_list_num = num + 1;

    ch = (unsigned char) *str;
    if (gen->mb_parse_table[ch] == 0)
        gen->mb_parse_table[ch] = num + 1;

    new->type = type;
    new->encoding = str;
    new->codeset = codeset;

    if (codeset->parse_info == NULL)
        codeset->parse_info = new;

    return True;

err:
    Xfree(str);

    Xfree(new);

    return False;
}

static void
free_charset(
    XLCd lcd)
{
    XLCdGenericPart *gen = XLC_GENERIC_PART(lcd);
    ParseInfo *parse_info;
    int num;

    Xfree(gen->mb_parse_table);
    gen->mb_parse_table = NULL;
    if ((num = gen->mb_parse_list_num) > 0) {
        for (parse_info = gen->mb_parse_list; num-- > 0; parse_info++) {
            Xfree((*parse_info)->encoding);
            Xfree(*parse_info);
        }
        Xfree(gen->mb_parse_list);
        gen->mb_parse_list = NULL;
        gen->mb_parse_list_num = 0;
    }

    if ((num = gen->codeset_num) > 0) {
        Xfree(gen->codeset_list);
        gen->codeset_list = NULL;
        gen->codeset_num = 0;
    }
}

/* For VW/UDC */

#define FORWARD  (unsigned long)'+'
#define BACKWARD (unsigned long)'-'

static const char *
getscope(
    const char *str,
    FontScope scp)
{
    unsigned long start = 0;
    unsigned long end = 0;
    unsigned long dest = 0;
    unsigned long shift = 0;
    unsigned long direction = 0;
    sscanf(str,"[\\x%lx,\\x%lx]->\\x%lx", &start, &end, &dest);
    if (dest) {
        if (dest >= start) {
            shift = dest - start;
            direction = FORWARD ;
        } else {
            shift = start - dest;
            direction = BACKWARD;
        }
    }
    scp->start = start      ;
    scp->end   = end        ;
    scp->shift = shift      ;
    scp->shift_direction
               = direction  ;
    /* .......... */
    while (*str) {
        if (*str == ',' && *(str+1) == '[')
            break;
        str++;
    }
    return str+1;
}

static int
count_scopemap(
    const char *str)
{
    const char *ptr;
    int num=0;
    for (ptr=str; *ptr; ptr++) {
        if (*ptr == ']') {
            num++;
        }
    }
    return num;
}

FontScope
_XlcParse_scopemaps(
    const char *str,
    int *size)
{
    int num=0,i;
    FontScope scope,sc_ptr;
    const char *str_sc;

    num = count_scopemap(str);
    scope = Xmallocarray(num, sizeof(FontScopeRec));
    if (scope == NULL)
	return NULL;

    for (i=0, str_sc=str, sc_ptr=scope; i < num; i++, sc_ptr++) {
	str_sc = getscope(str_sc, sc_ptr);
    }
    *size = num;
    return scope;
}

void
_XlcDbg_printValue(
    const char *str,
    char **value,
    int num)
{
/*
    int i;
    for (i = 0; i < num; i++)
        fprintf(stderr, "%s value[%d] = %s\n", str, i, value[i]);
*/
}

static void
dmpscope(
    const char* name,
    FontScope sc,
    int num)
{
/*
    int i;
    fprintf(stderr, "dmpscope %s\n", name);
    for (i=0; i<num; i++)
        fprintf(stderr,"%x %x %x %x \n",
                sc[i].start,
                sc[i].end,
                sc[i].shift,
                sc[i].shift_direction);
    fprintf(stderr, "dmpscope end\n");
*/
}

static XlcCharSet
srch_charset_define(
    const char *name,
    int *new)
{
    XlcCharSet charset;

    *new = 0;
    charset = _XlcGetCharSet(name);
    if (charset == NULL &&
        (charset = _XlcCreateDefaultCharSet(name, ""))) {
        _XlcAddCharSet(charset);
        *new = 1;
        charset->source = CSsrcXLC;
    }
    return charset;
}

static void
read_charset_define(
    XLCd lcd,
    XLCdGenericPart *gen)
{
    int i;
    char csd[16], cset_name[256];
    char name[BUFSIZ];
    XlcCharSet charsetd;
    char **value;
    int num, new = 0;
    XlcSide side = XlcUnknown;
    char *tmp;

    for (i=0; ; i++) { /* loop start */
        charsetd = 0;
        snprintf(csd, sizeof(csd), "csd%d", i);

        /* charset_name  */
        snprintf(name, sizeof(name), "%s.%s", csd, "charset_name");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        _XlcDbg_printValue(name,value,num);
        if (num > 0) {
	    /* hackers will get truncated -- C'est la vie */
            strncpy(cset_name,value[0], sizeof cset_name - 1);
	    cset_name[(sizeof cset_name) - 1] = '\0';
            snprintf(name, sizeof(name), "%s.%s", csd , "side");
            _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
            if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                if (!_XlcNCompareISOLatin1(value[0], "none", 4)) {
                    side =  XlcGLGR;
                } else if (!_XlcNCompareISOLatin1(value[0], "GL", 2)) {
                    side =  XlcGL;
                    strcat(cset_name,":GL");
                } else {
                    side =  XlcGR;
                    strcat(cset_name,":GR");
                }
                if (charsetd == NULL &&
                    (charsetd = srch_charset_define(cset_name,&new)) == NULL)
                    return;
            }
        } else {
            if (i == 0)
                continue;
            else
                break;
        }
        if (new) {
            tmp = strdup(cset_name);
            if (tmp == NULL)
                return;
            charsetd->name = tmp;
        }
        /* side   */
        charsetd->side = side ;
        /* length */
        snprintf(name, sizeof(name), "%s.%s", csd, "length");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
            charsetd->char_size = atoi(value[0]);
        }
        /* gc_number */
        snprintf(name, sizeof(name), "%s.%s", csd, "gc_number");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
            charsetd->set_size = atoi(value[0]);
        }
        /* string_encoding */
        snprintf(name, sizeof(name), "%s.%s", csd, "string_encoding");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
            if (!strcmp("False",value[0])) {
                charsetd->string_encoding = False;
            } else {
                charsetd->string_encoding = True;
            }
        }
        /* sequence */
        snprintf(name, sizeof(name), "%s.%s", csd, "sequence");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
/*
            if (charsetd->ct_sequence) {
                Xfree(charsetd->ct_sequence);
            }
*/
            tmp = Xmalloc(strlen(value[0])+1);
            if (tmp == NULL)
                return;
            charsetd->ct_sequence = tmp;
            string_to_encoding(value[0],tmp);
        }
        /* encoding_name */
        snprintf(name, sizeof(name), "%s.%s", csd, "encoding_name");
        _XlcGetResource(lcd, "XLC_CHARSET_DEFINE", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
/*
            if (charsetd->encoding_name) {
                Xfree(charsetd->encoding_name);
            }
*/
            tmp = strdup(value[0]);
            charsetd->encoding_name = tmp;
            charsetd->xrm_encoding_name = XrmStringToQuark(tmp);
        }
        _XlcAddCT(charsetd->name, charsetd->ct_sequence);
    }
}

static SegConv
add_conversion(
    XLCdGenericPart *gen)
{
    SegConv new_list;
    int num;

    if ((num = gen->segment_conv_num) > 0) {
        new_list = Xreallocarray(gen->segment_conv,
                                 num + 1, sizeof(SegConvRec));
    } else {
        new_list = Xmalloc(sizeof(SegConvRec));
    }

    if (new_list == NULL)
        return NULL;

    gen->segment_conv = new_list;
    gen->segment_conv_num = num + 1;

    return &new_list[num];

}

static void
read_segmentconversion(
    XLCd lcd,
    XLCdGenericPart *gen)
{
    int i;
    char conv[16];
    char name[BUFSIZ];
    char **value;
    int num,new;
    SegConv conversion;
    for (i=0 ; ; i++) { /* loop start */
        conversion = 0;
        snprintf(conv, sizeof(conv), "conv%d", i);

        /* length                */
        snprintf(name, sizeof(name), "%s.%s", conv, "length");
        _XlcGetResource(lcd, "XLC_SEGMENTCONVERSION", name, &value, &num);
        if (num > 0) {
            if (conversion == NULL &&
                (conversion = add_conversion(gen)) == NULL) {
                return;
            }
            _XlcDbg_printValue(name,value,num);
        } else {
            if (i == 0)
                continue;
            else
                break;
        }
        conversion->length = atoi(value[0]);

        /* source_encoding       */
        snprintf(name, sizeof(name), "%s.%s", conv, "source_encoding");
        _XlcGetResource(lcd, "XLC_SEGMENTCONVERSION", name, &value, &num);
        if (num > 0) {
            char *tmp;
            _XlcDbg_printValue(name,value,num);
            tmp = strdup(value[0]);
            if (tmp == NULL)
                return;
            conversion->source_encoding = tmp;
            conversion->source = srch_charset_define(tmp,&new);
        }
        /* destination_encoding  */
        snprintf(name, sizeof(name), "%s.%s", conv, "destination_encoding");
        _XlcGetResource(lcd, "XLC_SEGMENTCONVERSION", name, &value, &num);
        if (num > 0) {
            char *tmp;
            _XlcDbg_printValue(name,value,num);
            tmp = strdup(value[0]);
            if (tmp == NULL)
                return;
            conversion->destination_encoding = tmp;
            conversion->dest = srch_charset_define(tmp,&new);
        }
        /* range                 */
        snprintf(name, sizeof(name), "%s.%s", conv, "range");
        _XlcGetResource(lcd, "XLC_SEGMENTCONVERSION", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
            sscanf(value[0],"\\x%lx,\\x%lx",
                   &(conversion->range.start), &(conversion->range.end));
        }
        /* conversion            */
        snprintf(name, sizeof(name), "%s.%s", conv, "conversion");
        _XlcGetResource(lcd, "XLC_SEGMENTCONVERSION", name, &value, &num);
        if (num > 0) {
            _XlcDbg_printValue(name,value,num);
            conversion->conv =
                _XlcParse_scopemaps(value[0],&conversion->conv_num);
        }
    }  /* loop end */
}

static ExtdSegment
create_ctextseg(
    char **value,
    int num)
{
    ExtdSegment ret;
    char* ptr;
    char* cset_name = NULL;
    size_t cset_len;
    int i,new;
    FontScope scope;
    ret = Xmalloc(sizeof(ExtdSegmentRec));
    if (ret == NULL)
        return NULL;
    ret->name = strdup(value[0]);
    if (ret->name == NULL) {
        Xfree (ret);
        return NULL;
    }
    cset_len = strlen(ret->name) + 1;
    cset_name = Xmalloc (cset_len);
    if (cset_name == NULL) {
        Xfree (ret->name);
        Xfree (ret);
        return NULL;
    }
    if (strchr(value[0],':')) {
        ptr = strchr(ret->name,':');
        *ptr = '\0';
        ptr++;
        if (!_XlcNCompareISOLatin1(ptr, "GL", 2)) {
            ret->side =  XlcGL;
            snprintf(cset_name, cset_len, "%s:%s", ret->name, "GL");
        } else {
            ret->side =  XlcGR;
            snprintf(cset_name, cset_len, "%s:%s", ret->name, "GR");
        }
    } else {
        ret->side =  XlcGLGR;
        strcpy(cset_name,ret->name);
    }
    ret->area = Xmallocarray(num - 1, sizeof(FontScopeRec));
    if (ret->area == NULL) {
	Xfree (cset_name);
	Xfree (ret->name);
	Xfree (ret);
        return NULL;
    }
    ret->area_num = num - 1;
    scope = ret->area ;
    for (i = 1; i < num; i++) {
        sscanf(value[i],"\\x%lx,\\x%lx",
               &scope[i-1].start, &scope[i-1].end);
    }
    ret->charset = srch_charset_define(cset_name,&new);
    Xfree (cset_name);

    return ret;
}
/* For VW/UDC end */

static Bool
load_generic(
    XLCd lcd)
{
    XLCdGenericPart *gen = XLC_GENERIC_PART(lcd);
    char **value;
    int num;
    unsigned long l;
    int i;
    int M,ii;
    XlcCharSet charset;

    gen->codeset_num = 0;

    /***** wc_encoding_mask *****/
    _XlcGetResource(lcd, "XLC_XLOCALE", "wc_encoding_mask", &value, &num);
    if (num > 0) {
	if (string_to_ulong(value[0], &l) == False)
	    goto err;
	gen->wc_encode_mask = l;
    }
    /***** wc_shift_bits *****/
    _XlcGetResource(lcd, "XLC_XLOCALE", "wc_shift_bits", &value, &num);
    if (num > 0)
	gen->wc_shift_bits = atoi(value[0]);
    if (gen->wc_shift_bits < 1)
	gen->wc_shift_bits = 8;
    /***** use_stdc_env *****/
    _XlcGetResource(lcd, "XLC_XLOCALE", "use_stdc_env", &value, &num);
    if (num > 0 && !_XlcCompareISOLatin1(value[0], "True"))
	gen->use_stdc_env = True;
    else
	gen->use_stdc_env = False;
    /***** force_convert_to_mb *****/
    _XlcGetResource(lcd, "XLC_XLOCALE", "force_convert_to_mb", &value, &num);
    if (num > 0 && !_XlcCompareISOLatin1(value[0], "True"))
	gen->force_convert_to_mb = True;
    else
	gen->force_convert_to_mb = False;

    for (i = 0; ; i++) {
	CodeSetRec *codeset = NULL;
	char cs[16];
	char name[BUFSIZ];

	snprintf(cs, sizeof(cs), "cs%d", i);

	/***** codeset.side *****/
	snprintf(name, sizeof(name), "%s.%s", cs , "side");
	_XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
	if (num > 0) {
	    char *tmp;

	    if (codeset == NULL && (codeset = add_codeset(gen)) == NULL)
		goto err;

            /* 3.4.1 side */
            if (!_XlcNCompareISOLatin1(value[0], "none", 4)) {
                codeset->side =  XlcNONE;
            } else if (!_XlcNCompareISOLatin1(value[0], "GL", 2)) {
                codeset->side =  XlcGL;
            } else {
                codeset->side =  XlcGR;
            }

	    tmp = strrchr(value[0], ':');
	    if (tmp != NULL && !_XlcCompareISOLatin1(tmp + 1, "Default")) {
		if (codeset->side == XlcGR)
		    gen->initial_state_GR = codeset;
		else
		    gen->initial_state_GL = codeset;
	    }
	}

	/***** codeset.length *****/
	snprintf(name, sizeof(name), "%s.%s", cs , "length");
	_XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
	if (num > 0) {
	    if (codeset == NULL && (codeset = add_codeset(gen)) == NULL)
		goto err;
	    codeset->length = atoi(value[0]);
	    if (codeset->length < 1)
		codeset->length = 1;
	}

	/***** codeset.mb_encoding *****/
	snprintf(name, sizeof(name), "%s.%s", cs, "mb_encoding");
	_XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
	if (num > 0) {
	    static struct {
		const char *str;
		EncodingType type;
	    } shifts[] = {
		{"<SS>", E_SS},
		{"<LSL>", E_LSL},
		{"<LSR>", E_LSR},
		{0}
	    };
	    int j;

	    if (codeset == NULL && (codeset = add_codeset(gen)) == NULL)
		goto err;
	    for ( ; num-- > 0; value++) {
		char encoding[256];
		char *tmp = *value;
		EncodingType type = E_SS;    /* for BC */
		for (j = 0; shifts[j].str; j++) {
		    if (!_XlcNCompareISOLatin1(tmp, shifts[j].str,
					       (int) strlen(shifts[j].str))) {
			type = shifts[j].type;
			tmp += strlen(shifts[j].str);
			break;
		    }
		}
		if (strlen (tmp) > sizeof encoding ||
		    string_to_encoding(tmp, encoding) == False)
			goto err;
		add_parse_list(gen, type, encoding, codeset);
	    }
	}

	/***** codeset.wc_encoding *****/
	snprintf(name, sizeof(name), "%s.%s", cs, "wc_encoding");
	_XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
	if (num > 0) {
	    if (codeset == NULL && (codeset = add_codeset(gen)) == NULL)
		goto err;
	    if (string_to_ulong(value[0], &l) == False)
		goto err;
	    codeset->wc_encoding = l;
	}

	/***** codeset.ct_encoding *****/
	snprintf(name, sizeof(name), "%s.%s", cs, "ct_encoding");
	_XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
	if (num > 0) {
	    char *encoding;

	    if (codeset == NULL && (codeset = add_codeset(gen)) == NULL)
		goto err;
	    for ( ; num-- > 0; value++) {
		if (strlen (*value) > sizeof name)
		    goto err;
		string_to_encoding(*value, name);
		charset = NULL;
		if ((encoding = strchr(name, ':')) &&
		    (encoding = strchr(encoding + 1, ':'))) {
		    *encoding++ = '\0';
		    charset = _XlcAddCT(name, encoding);
		}
		if (charset == NULL) {
		    charset = _XlcGetCharSet(name);
		    if (charset == NULL &&
			(charset = _XlcCreateDefaultCharSet(name, ""))) {
			charset->side = codeset->side;
			charset->char_size = codeset->length;
			_XlcAddCharSet(charset);
		    }
		}
		if (charset) {
		    if (add_charset(codeset, charset) == False)
			goto err;
		}
	    }
	}

	if (codeset == NULL)
	    break;
	codeset->cs_num = i;
        /* For VW/UDC */
        /***** 3.4.2 byteM (1 <= M <= length)*****/
        for (M=1; M-1  < codeset->length; M++) {
            unsigned long start,end;
            ByteInfo tmpb;

            snprintf(name, sizeof(name),"%s.%s%d",cs,"byte",M);
            _XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);

            if (M == 1) {
                if (num < 1) {
                    codeset->byteM = NULL;
                    break ;
                }
                codeset->byteM = Xmallocarray(codeset->length,
                                              sizeof(ByteInfoListRec));
                if (codeset->byteM == NULL) {
                    goto err;
                }
            }

            if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                (codeset->byteM)[M-1].M = M;
                (codeset->byteM)[M-1].byteinfo_num = num;
                (codeset->byteM)[M-1].byteinfo =
		    Xmallocarray(num, sizeof(ByteInfoRec));
                for (ii = 0 ; ii < num ; ii++) {
                    tmpb = (codeset->byteM)[M-1].byteinfo ;
                    /* default 0x00 - 0xff */
                    sscanf(value[ii],"\\x%lx,\\x%lx",&start,&end);
                    tmpb[ii].start = (unsigned char)start;
                    tmpb[ii].end  = (unsigned char)end;
                }
            }
            /* .... */
        }


        /***** codeset.mb_conversion *****/
        snprintf(name, sizeof(name), "%s.%s", cs, "mb_conversion");
        _XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
        if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                codeset->mbconv = Xmalloc(sizeof(ConversionRec));
                codeset->mbconv->convlist =
                _XlcParse_scopemaps(value[0],&(codeset->mbconv->conv_num));
                dmpscope("mb_conv",codeset->mbconv->convlist,
                         codeset->mbconv->conv_num);
                /* [\x%x,\x%x]->\x%x,... */
        }
        /***** codeset.ct_conversion *****/
        snprintf(name, sizeof(name), "%s.%s", cs, "ct_conversion");
        _XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
        if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                codeset->ctconv = Xmalloc(sizeof(ConversionRec));
                codeset->ctconv->convlist =
                _XlcParse_scopemaps(value[0],&(codeset->ctconv->conv_num));
                dmpscope("ctconv",codeset->ctconv->convlist,
                         codeset->ctconv->conv_num);
                /* [\x%x,\x%x]->\x%x,... */
        }
        /***** codeset.ct_conversion_file *****/
        snprintf(name, sizeof(name), "%s.%s", cs, "ct_conversion_file");
        _XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
        if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                /* [\x%x,\x%x]->\x%x,... */
        }
        /***** codeset.ct_extended_segment *****/
        snprintf(name, sizeof(name), "%s.%s", cs, "ct_extended_segment");
        _XlcGetResource(lcd, "XLC_XLOCALE", name, &value, &num);
        if (num > 0) {
                _XlcDbg_printValue(name,value,num);
                codeset->ctextseg = create_ctextseg(value,num);
                /* [\x%x,\x%x]->\x%x,... */
        }
        /* For VW/UDC end */

    }

    read_charset_define(lcd,gen);       /* For VW/UDC */
    read_segmentconversion(lcd,gen);    /* For VW/UDC */

    if (gen->initial_state_GL == NULL) {
       CodeSetRec *codeset;
       for (i = 0; i < gen->codeset_num; i++) {
          codeset = gen->codeset_list[i];
          if (codeset->side == XlcGL)
             gen->initial_state_GL = codeset;
       }
    }

    if (gen->initial_state_GR == NULL) {
       CodeSetRec *codeset;
       for (i = 0; i < gen->codeset_num; i++) {
          codeset = gen->codeset_list[i];
          if (codeset->side == XlcGR)
             gen->initial_state_GR = codeset;
       }
    }

    for (i = 0; i < gen->codeset_num; i++) {
       CodeSetRec *codeset = gen->codeset_list[i];
       for (ii = 0; ii < codeset->num_charsets; ii++) {
          charset = codeset->charset_list[ii];
          if (! strcmp(charset->encoding_name, "ISO8859-1"))
              charset->string_encoding = True;
          if ( charset->string_encoding )
              codeset->string_encoding = True;
       }
    }
    return True;

err:
    free_charset(lcd);

    return False;
}

#ifdef USE_DYNAMIC_LC
/* override the open_om and open_im methods which were set by
   super_class's initialize method() */

static Bool
initialize_core(
    XLCd lcd)
{
    _XInitDynamicOM(lcd);

    _XInitDynamicIM(lcd);

    return True;
}
#endif

static Bool
initialize(XLCd lcd)
{
    XLCdPublicMethods superclass = (XLCdPublicMethods) _XlcPublicMethods;

    XLC_PUBLIC_METHODS(lcd)->superclass = superclass;

    if (superclass->pub.initialize) {
	if ((*superclass->pub.initialize)(lcd) == False)
	    return False;
    }

#ifdef USE_DYNAMIC_LC
    if (initialize_core(lcd) == False)
	return False;
#endif

    if (load_generic(lcd) == False)
	return False;

    return True;
}

/* VW/UDC start 95.01.08 */
static void
freeByteM(
    CodeSet codeset)
{
    int i;
    ByteInfoList blst;
    if (codeset->byteM == NULL) {
	return ;
    }
    blst = codeset->byteM;
    for (i = 0; i < codeset->length; i++) {
	    Xfree(blst[i].byteinfo);
	    blst[i].byteinfo = NULL;
    }
    Xfree(codeset->byteM);
    codeset->byteM = NULL;
}

static void
freeConversion(
    CodeSet codeset)
{
    Conversion mbconv,ctconv;
    if (codeset->mbconv) {
	mbconv = codeset->mbconv;
	/*  ...  */
	Xfree(mbconv->convlist);
	mbconv->convlist = NULL;

	Xfree(mbconv);
	codeset->mbconv = NULL;
    }
    if (codeset->ctconv) {
	ctconv = codeset->ctconv;
	/*  ...  */
	Xfree(ctconv->convlist);
	ctconv->convlist = NULL;
	
	Xfree(ctconv);
	codeset->ctconv = NULL;
    }
}

static void
freeExtdSegment(
    CodeSet codeset)
{
    ExtdSegment ctextseg;
    if (codeset->ctextseg == NULL) {
	return;
    }
    ctextseg = codeset->ctextseg;
    Xfree(ctextseg->name);
    ctextseg->name = NULL;

    Xfree(ctextseg->area);
    ctextseg->area = NULL;

    Xfree(codeset->ctextseg);
    codeset->ctextseg = NULL;
}

static void
freeParseInfo(
    CodeSet codeset)
{
    ParseInfo parse_info;
    if (codeset->parse_info == NULL) {
	return;
    }
    parse_info = codeset->parse_info;

    Xfree(parse_info->encoding);
    parse_info->encoding = NULL;

    Xfree(codeset->parse_info);
    codeset->parse_info = NULL;
}

static void
destroy_CodeSetList(
    XLCdGenericPart *gen)
{
    CodeSet *codeset = gen->codeset_list;
    int i;
    if (gen->codeset_num == 0) {
	return;
    }
    for (i=0;i<gen->codeset_num;i++) {
        freeByteM(codeset[i]);
	freeConversion(codeset[i]);
	freeExtdSegment(codeset[i]);
	freeParseInfo(codeset[i]);

	Xfree(codeset[i]->charset_list);
	codeset[i]->charset_list = NULL;

	Xfree(codeset[i]); codeset[i]=NULL;
    }
    Xfree(codeset); gen->codeset_list = NULL;
}

static void
destroy_SegConv(
    XLCdGenericPart *gen)
{
    SegConv seg = gen->segment_conv;
    int i;

    if (gen->segment_conv_num == 0) {
	return;
    }
    for (i=0;i<gen->segment_conv_num;i++) {

	    Xfree(seg[i].source_encoding);
	    seg[i].source_encoding = NULL;

	    Xfree(seg[i].destination_encoding);
	    seg[i].destination_encoding = NULL;

	    Xfree(seg[i].conv);
            seg[i].conv = NULL;
    }
    Xfree(seg); gen->segment_conv = NULL;
}

static void
destroy_gen(
    XLCd lcd)
{
    XLCdGenericPart *gen = XLC_GENERIC_PART(lcd);
    destroy_SegConv(gen);
    destroy_CodeSetList(gen);

    Xfree(gen->mb_parse_table);
    gen->mb_parse_table = NULL;

    Xfree(gen->mb_parse_list);
    gen->mb_parse_list = NULL;

}
/* VW/UDC end 95.01.08 */

static void
destroy(
    XLCd lcd)
{
    XLCdPublicMethods superclass = XLC_PUBLIC_METHODS(lcd)->superclass;

    destroy_gen(lcd); /* ADD 1996.01.08 */
    if (superclass && superclass->pub.destroy)
	(*superclass->pub.destroy)(lcd);
}
