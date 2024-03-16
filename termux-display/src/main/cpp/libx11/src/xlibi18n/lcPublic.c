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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XlcPubI.h"

static const char *
default_string(
    XLCd lcd)
{
    return XLC_PUBLIC(lcd, default_string);
}

static XLCd create (const char *name, XLCdMethods methods);
static Bool initialize (XLCd lcd);
static void destroy (XLCd lcd);
static char *get_values (XLCd lcd, XlcArgList args, int num_args);

static XLCdPublicMethodsRec publicMethods = {
    {
	destroy,
	_XlcDefaultMapModifiers,
	NULL,
	NULL,
	_XrmDefaultInitParseInfo,
	_XmbTextPropertyToTextList,
	_XwcTextPropertyToTextList,
	_Xutf8TextPropertyToTextList,
	_XmbTextListToTextProperty,
	_XwcTextListToTextProperty,
	_Xutf8TextListToTextProperty,
	_XwcFreeStringList,
	default_string,
	NULL,
	NULL
    },
    {
	NULL,
	create,
	initialize,
	destroy,
	get_values,
	_XlcGetLocaleDataBase
    }
};

XLCdMethods _XlcPublicMethods = (XLCdMethods) &publicMethods;

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

    lcd->core = Xcalloc(1, sizeof(XLCdPublicRec));
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
load_public(
    XLCd lcd)
{
    XLCdPublicPart *pub = XLC_PUBLIC_PART(lcd);
    char **values;
    const char *str;
    int num;

    if(_XlcCreateLocaleDataBase(lcd) == NULL)
	return False;

    _XlcGetResource(lcd, "XLC_XLOCALE", "mb_cur_max", &values, &num);
    if (num > 0) {
	pub->mb_cur_max = atoi(values[0]);
	if (pub->mb_cur_max < 1)
	    pub->mb_cur_max = 1;
    } else
	pub->mb_cur_max = 1;

    _XlcGetResource(lcd, "XLC_XLOCALE", "state_depend_encoding", &values, &num);
    if (num > 0 && !_XlcCompareISOLatin1(values[0], "True"))
	pub->is_state_depend = True;
    else
	pub->is_state_depend = False;

    _XlcGetResource(lcd, "XLC_XLOCALE", "encoding_name", &values, &num);
    str = (num > 0) ? values[0] : "STRING";
    pub->encoding_name = strdup(str);
    if (pub->encoding_name == NULL)
	return False;

    return True;
}

static Bool
initialize_core(
    XLCd lcd)
{
    XLCdMethods methods = lcd->methods;
    XLCdMethods core = &publicMethods.core;

    if (methods->close == NULL)
	methods->close = core->close;

    if (methods->map_modifiers == NULL)
	methods->map_modifiers = core->map_modifiers;

    if (methods->open_om == NULL)
#ifdef USE_DYNAMIC_LC
	_XInitDefaultOM(lcd);
#else
	_XInitOM(lcd);
#endif

    if (methods->open_im == NULL)
#ifdef USE_DYNAMIC_LC
	_XInitDefaultIM(lcd);
#else
	_XInitIM(lcd);
#endif

    if (methods->init_parse_info == NULL)
	methods->init_parse_info = core->init_parse_info;

    if (methods->mb_text_prop_to_list == NULL)
	methods->mb_text_prop_to_list = core->mb_text_prop_to_list;

    if (methods->wc_text_prop_to_list == NULL)
	methods->wc_text_prop_to_list = core->wc_text_prop_to_list;

    if (methods->utf8_text_prop_to_list == NULL)
	methods->utf8_text_prop_to_list = core->utf8_text_prop_to_list;

    if (methods->mb_text_list_to_prop == NULL)
	methods->mb_text_list_to_prop = core->mb_text_list_to_prop;

    if (methods->wc_text_list_to_prop == NULL)
	methods->wc_text_list_to_prop = core->wc_text_list_to_prop;

    if (methods->utf8_text_list_to_prop == NULL)
	methods->utf8_text_list_to_prop = core->utf8_text_list_to_prop;

    if (methods->wc_free_string_list == NULL)
	methods->wc_free_string_list = core->wc_free_string_list;

    if (methods->default_string == NULL)
	methods->default_string = core->default_string;

    return True;
}

static Bool
initialize(
    XLCd lcd)
{
    XLCdPublicMethodsPart *methods = XLC_PUBLIC_METHODS(lcd);
    XLCdPublicMethodsPart *pub_methods = &publicMethods.pub;
    XLCdPublicPart *pub = XLC_PUBLIC_PART(lcd);
    char *name;
#if !defined(X_LOCALE)
    int len;
    char sinamebuf[256];
    char* siname;
#endif

    _XlcInitCTInfo();

    if (initialize_core(lcd) == False)
	return False;

    name = lcd->core->name;
#if !defined(X_LOCALE)
    /*
     * _XlMapOSLocaleName will return the same string or a substring
     * of name, so strlen(name) is okay
     */
    if ((len = (int) strlen(name)) < sizeof sinamebuf)
        siname = sinamebuf;
    else
        siname = Xmalloc (len + 1);
    if (siname == NULL)
        return False;
    name = _XlcMapOSLocaleName(name, siname);
#endif
    /* _XlcResolveLocaleName will lookup the SI's name for the locale */
    if (_XlcResolveLocaleName(name, pub) == 0) {
#if !defined(X_LOCALE)
	if (siname != sinamebuf) Xfree (siname);
#endif
	return False;
    }
#if !defined(X_LOCALE)
    if (siname != sinamebuf)
        Xfree (siname);
#endif

    if (pub->default_string == NULL)
	pub->default_string = "";

    if (methods->get_values == NULL)
	methods->get_values = pub_methods->get_values;

    if (methods->get_resource == NULL)
	methods->get_resource = pub_methods->get_resource;

    return load_public(lcd);
}

static void
destroy_core(
    XLCd lcd)
{
    if (lcd) {
        if (lcd->core) {
            Xfree(lcd->core->name);
            Xfree(lcd->core->modifiers);
            Xfree(lcd->core);
        }
        Xfree(lcd->methods);
        Xfree(lcd);
    }
}

static void
destroy(
    XLCd lcd)
{
    XLCdPublicPart *pub = XLC_PUBLIC_PART(lcd);

    _XlcDestroyLocaleDataBase(lcd);

    Xfree(pub->siname);
    Xfree(pub->encoding_name);

    destroy_core(lcd);
}

static XlcResource resources[] = {
    { XlcNCodeset, NULLQUARK, sizeof(char *),
      XOffsetOf(XLCdPublicRec, pub.codeset), XlcGetMask },
    { XlcNDefaultString, NULLQUARK, sizeof(char *),
      XOffsetOf(XLCdPublicRec, pub.default_string), XlcGetMask },
    { XlcNEncodingName, NULLQUARK, sizeof(char *),
      XOffsetOf(XLCdPublicRec, pub.encoding_name), XlcGetMask },
    { XlcNLanguage, NULLQUARK, sizeof(char *),
      XOffsetOf(XLCdPublicRec, pub.language), XlcGetMask },
    { XlcNMbCurMax, NULLQUARK, sizeof(int),
      XOffsetOf(XLCdPublicRec, pub.mb_cur_max), XlcGetMask },
    { XlcNStateDependentEncoding, NULLQUARK, sizeof(Bool),
      XOffsetOf(XLCdPublicRec, pub.is_state_depend), XlcGetMask },
    { XlcNTerritory, NULLQUARK, sizeof(char *),
      XOffsetOf(XLCdPublicRec, pub.territory), XlcGetMask }
};

static char *
get_values(
    XLCd lcd,
    XlcArgList args,
    int num_args)
{
    XLCdPublic pub = (XLCdPublic) lcd->core;

    if (resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(resources, XlcNumber(resources));

    return _XlcGetValues((XPointer) pub, resources, XlcNumber(resources), args,
			 num_args, XlcGetMask);
}
