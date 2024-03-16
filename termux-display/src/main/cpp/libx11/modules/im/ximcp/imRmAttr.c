/******************************************************************

           Copyright 1992, 1993, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
FUJITSU LIMITED makes no representations about the suitability of
this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <limits.h>

#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"


static XIMResourceList
_XimGetNestedListSeparator(
    XIMResourceList	 res_list,		/* LISTofIMATTR or IMATTR */
    unsigned int	 res_num)
{
    return  _XimGetResourceListRec(res_list, res_num, XNSeparatorofNestedList);
}

static Bool
_XimCheckInnerIMAttributes(
    Xim			 im,
    XIMArg		*arg,
    unsigned long	 mode)
{
    XIMResourceList	 res;
    int			 check;

    if (!(res = _XimGetResourceListRec(im->private.proto.im_inner_resources,
			im->private.proto.im_num_inner_resources, arg->name)))
	return False;

    check = _XimCheckIMMode(res, mode);
    if(check == XIM_CHECK_INVALID)
	return True;
    else if(check == XIM_CHECK_ERROR)
	return False;

    return True;
}

char *
_XimMakeIMAttrIDList(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    XIMArg		*arg,
    CARD16		*buf,
    INT16		*len,
    unsigned long	 mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;

    *len = 0;
    if (!arg)
	return (char *)NULL;

    for (p = arg; p->name; p++) {
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimCheckInnerIMAttributes(im, p, mode))
		continue;
	    return p->name;
	}

	check = _XimCheckIMMode(res, mode);
	if (check == XIM_CHECK_INVALID)
	    continue;
	else if (check == XIM_CHECK_ERROR)
	    return p->name;

	*buf = res->id;
	*len += sizeof(CARD16);
	 buf++;
    }
    return (char *)NULL;
}

static Bool
_XimCheckInnerICAttributes(
    Xic			 ic,
    XIMArg		*arg,
    unsigned long	 mode)
{
    XIMResourceList	 res;
    int			 check;

    if (!(res = _XimGetResourceListRec(ic->private.proto.ic_inner_resources,
			ic->private.proto.ic_num_inner_resources, arg->name)))
	return False;

    check = _XimCheckICMode(res, mode);
    if(check == XIM_CHECK_INVALID)
	return True;
    else if(check == XIM_CHECK_ERROR)
	return False;

    return True;
}

char *
_XimMakeICAttrIDList(
    Xic			 ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    XIMArg		*arg,
    CARD16		*buf,
    INT16		*len,
    unsigned long	 mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;
    char		*name;
    INT16		 new_len;

    *len = 0;
    if (!arg)
	return (char *)NULL;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    for (p = arg; p && p->name; p++) {
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimCheckInnerICAttributes(ic, p, mode))
		continue;
	    *len = -1;
	    return p->name;
	}

	check = _XimCheckICMode(res, mode);
	if(check == XIM_CHECK_INVALID)
	    continue;
	else if(check == XIM_CHECK_ERROR) {
	    *len = -1;
	    return p->name;
	}

	*buf = res->id;
	*len += sizeof(CARD16);
	buf++;
	if (res->resource_size == XimType_NEST) {
	    if (res->xrm_name == pre_quark) {
		if ((name = _XimMakeICAttrIDList(ic, res_list, res_num,
				(XIMArg *)p->value, buf, &new_len,
				(mode | XIM_PREEDIT_ATTR)))) {
		    if (new_len < 0) *len = -1;
		    else *len += new_len;
		    return name;
		}
		*len += new_len;
		buf = (CARD16 *)((char *)buf + new_len);
	    } else if (res->xrm_name == sts_quark) {
		if ((name = _XimMakeICAttrIDList(ic, res_list, res_num,
				(XIMArg *)p->value, buf, &new_len,
				(mode | XIM_STATUS_ATTR)))) {
		    if (new_len < 0) *len = -1;
		    else *len += new_len;
		    return name;
		}
		*len += new_len;
		buf = (CARD16 *)((char *)buf + new_len);
	    }

	    if (!(res = _XimGetNestedListSeparator(res_list, res_num))) {
		p++;
		if (p) {
		    *len = -1;
		    return p->name;
		}
		else {
		    return (char *)NULL;
		}
	    }
	    *buf = res->id;
	    *len += sizeof(CARD16);
	    buf++;
	}
    }
    return (char *)NULL;
}

static Bool
_XimAttributeToValue(
    Xic			  ic,
    XIMResourceList	  res,
    CARD16		 *data,
    CARD16		  data_len,
    XPointer		  value,
    BITMASK32		  mode)
{
    switch (res->resource_size) {
    case XimType_SeparatorOfNestedList:
    case XimType_NEST:
	break;

    case XimType_CARD8:
    case XimType_CARD16:
    case XimType_CARD32:
    case XimType_Window:
    case XimType_XIMHotKeyState:
	_XCopyToArg((XPointer)data, (XPointer *)&value, data_len);
	break;

    case XimType_STRING8:
	{
	    char	*str;

	    if (!(value))
		return False;

	    if (!(str = Xmalloc(data_len + 1)))
		return False;

	    (void)memcpy(str, (char *)data, data_len);
	    str[data_len] = '\0';

	    *((char **)value) = str;
	    break;
	}

    case XimType_XIMStyles:
	{
	    CARD16		 num = data[0];
	    register CARD32	*style_list = (CARD32 *)&data[2];
	    XIMStyle		*style;
	    XIMStyles		*rep;
	    register int	 i;
	    char		*p;
	    unsigned int         alloc_len;

	    if (!(value))
		return False;

	    if (num > (USHRT_MAX / sizeof(XIMStyle)))
		return False;
	    if ((2 * sizeof(CARD16) + (num * sizeof(CARD32))) > data_len)
		return False;
	    alloc_len = sizeof(XIMStyles) + sizeof(XIMStyle) * num;
	    if (alloc_len < sizeof(XIMStyles))
		return False;
	    if (!(p = Xmalloc(alloc_len)))
		return False;

	    rep   = (XIMStyles *)p;
	    style = (XIMStyle *)(p + sizeof(XIMStyles));

	    for (i = 0; i < num; i++)
		style[i] = (XIMStyle)style_list[i];

	    rep->count_styles = (unsigned short)num;
	    rep->supported_styles = style;
	    *((XIMStyles **)value) = rep;
	    break;
	}

    case XimType_XRectangle:
	{
	    XRectangle	*rep;

	    if (!(value))
		return False;

	    if (!(rep = Xmalloc(sizeof(XRectangle))))
		return False;

	    rep->x      = data[0];
	    rep->y      = data[1];
	    rep->width  = data[2];
	    rep->height = data[3];
	    *((XRectangle **)value) = rep;
	    break;
	}

    case XimType_XPoint:
	{
	    XPoint	*rep;

	    if (!(value))
		return False;

	    if (!(rep = Xmalloc(sizeof(XPoint))))
		return False;

	    rep->x = data[0];
	    rep->y = data[1];
	    *((XPoint **)value) = rep;
	    break;
	}

    case XimType_XFontSet:
	{
	    CARD16	 len = data[0];
	    char	*base_name;
	    XFontSet	 rep = (XFontSet)NULL;
	    char	**missing_list = NULL;
	    int		 missing_count;
	    char	*def_string;

	    if (!(value))
		return False;
	    if (!ic)
		return False;
	    if (len > data_len)
		return False;
	    if (!(base_name = Xmalloc(len + 1)))
		return False;

	    (void)strncpy(base_name, (char *)&data[1], (size_t)len);
	    base_name[len] = '\0';

	    if (mode & XIM_PREEDIT_ATTR) {
		if (!strcmp(base_name, ic->private.proto.preedit_font)) {
		    rep = ic->core.preedit_attr.fontset;
		} else if (!ic->private.proto.preedit_font_length) {
		    rep = XCreateFontSet(ic->core.im->core.display,
					base_name, &missing_list,
					&missing_count, &def_string);
		}
	    } else if (mode & XIM_STATUS_ATTR) {
		if (!strcmp(base_name, ic->private.proto.status_font)) {
		    rep = ic->core.status_attr.fontset;
		} else if (!ic->private.proto.status_font_length) {
		    rep = XCreateFontSet(ic->core.im->core.display,
					base_name, &missing_list,
					&missing_count, &def_string);
		}
	    }

	    Xfree(base_name);
	    Xfree(missing_list);
	    *((XFontSet *)value) = rep;
	    break;
	}

    case XimType_XIMHotKeyTriggers:
	{
	    CARD32			 num = *((CARD32 *)data);
	    register CARD32		*key_list = (CARD32 *)&data[2];
	    XIMHotKeyTrigger		*key;
	    XIMHotKeyTriggers		*rep;
	    register int		 i;
	    char			*p;
	    unsigned int		 alloc_len;

	    if (!(value))
		return False;

	    if (num > (UINT_MAX / sizeof(XIMHotKeyTrigger)))
		return False;
	    if ((2 * sizeof(CARD16) + (num * 3 * sizeof(CARD32))) > data_len)
		return False;
	    alloc_len = sizeof(XIMHotKeyTriggers)
		      + sizeof(XIMHotKeyTrigger) * num;
	    if (alloc_len < sizeof(XIMHotKeyTriggers))
		return False;
	    if (!(p = Xmalloc(alloc_len)))
		return False;

	    rep = (XIMHotKeyTriggers *)p;
	    key = (XIMHotKeyTrigger *)(p + sizeof(XIMHotKeyTriggers));

	    for (i = 0; i < num; i++, key_list += 3) {
		key[i].keysym        = (KeySym)key_list[0]; /* keysym */
		key[i].modifier      = (int)key_list[1];    /* modifier */
		key[i].modifier_mask = (int)key_list[2];    /* modifier_mask */
	    }

	    rep->num_hot_key = (int)num;
	    rep->key = key;
	    *((XIMHotKeyTriggers **)value) = rep;
	    break;
	}

    case XimType_XIMStringConversion:
	{
	    break;
	}

    default:
	return False;
    }
    return True;
}

static Bool
_XimDecodeInnerIMATTRIBUTE(
    Xim			 im,
    XIMArg		*arg)
{
    XIMResourceList	 res;
    XimDefIMValues	 im_values;

    if (!(res = _XimGetResourceListRec(im->private.proto.im_inner_resources,
			im->private.proto.im_num_inner_resources, arg->name)))
	return False;

    _XimGetCurrentIMValues(im, &im_values);
    return _XimDecodeLocalIMAttr(res, (XPointer)&im_values, arg->value);
}

char *
_XimDecodeIMATTRIBUTE(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    CARD16		*data,
    INT16		 data_len,
    XIMArg		*arg,
    BITMASK32		 mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;
    INT16		 len;
    CARD16		*buf;
    INT16		 total;
    INT16		 min_len = sizeof(CARD16)	/* sizeof attributeID */
			 	 + sizeof(INT16);	/* sizeof length */

    for (p = arg; p->name; p++) {
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimDecodeInnerIMATTRIBUTE(im, p))
		continue;
	    return p->name;
	}

	check = _XimCheckIMMode(res, mode);
	if(check == XIM_CHECK_INVALID)
	    continue;
	else if(check == XIM_CHECK_ERROR)
	    return p->name;

	total = data_len;
	buf = data;
	while (total >= min_len) {
	    if (res->id == buf[0])
		break;

	    len = buf[1];
	    len += XIM_PAD(len) + min_len;
	    buf = (CARD16 *)((char *)buf + len);
	    total -= len;
	}
	if (total < min_len)
	    return p->name;

	if (!(_XimAttributeToValue((Xic) im->private.local.current_ic,
				   res, &buf[2], buf[1], p->value, mode)))
	    return p->name;
    }
    return (char *)NULL;
}

static Bool
_XimDecodeInnerICATTRIBUTE(
    Xic			 ic,
    XIMArg		*arg,
    unsigned long	 mode)
{
    XIMResourceList	 res;
    XimDefICValues	 ic_values;

    if (!(res = _XimGetResourceListRec(ic->private.proto.ic_inner_resources,
			ic->private.proto.ic_num_inner_resources, arg->name)))
	return False;

    _XimGetCurrentICValues(ic, &ic_values);
    if (!_XimDecodeLocalICAttr(res, (XPointer)&ic_values, arg->value, mode))
	return False;
    _XimSetCurrentICValues(ic, &ic_values);
    return True;
}

char *
_XimDecodeICATTRIBUTE(
    Xic			 ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    CARD16		*data,
    INT16		 data_len,
    XIMArg		*arg,
    BITMASK32		 mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;
    INT16		 len;
    CARD16		*buf;
    INT16		 total;
    char		*name;
    INT16		 min_len = sizeof(CARD16)	/* sizeof attributeID */
			 	 + sizeof(INT16);	/* sizeof length */
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;

    if (!arg)
	return (char *)NULL;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    for (p = arg; p->name; p++) {
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimDecodeInnerICATTRIBUTE(ic, p, mode))
		continue;
	    return p->name;
	}

	check = _XimCheckICMode(res, mode);
	if (check == XIM_CHECK_INVALID)
	    continue;
	else if (check == XIM_CHECK_ERROR)
	    return p->name;

	total = data_len;
	buf = data;
	while (total >= min_len) {
	    if (res->id == buf[0])
		break;

	    len = buf[1];
	    len += XIM_PAD(len) + min_len;
	    buf = (CARD16 *)((char *)buf + len);
	    total -= len;
	}
	if (total < min_len)
	    return p->name;

	if (res->resource_size == XimType_NEST) {
	    if (res->xrm_name == pre_quark) {
	        if ((name = _XimDecodeICATTRIBUTE(ic, res_list, res_num,
			&buf[2], buf[1], (XIMArg *)p->value,
			(mode | XIM_PREEDIT_ATTR))))
		    return name;
	    } else if (res->xrm_name == sts_quark) {
	        if ((name = _XimDecodeICATTRIBUTE(ic, res_list, res_num,
			&buf[2], buf[1], (XIMArg *)p->value,
			(mode | XIM_STATUS_ATTR))))
		    return name;
	    }
	} else {
	    if (!(_XimAttributeToValue(ic, res, &buf[2], buf[1],
							p->value, mode)))
		return p->name;
	}
    }
    return (char *)NULL;
}

static Bool
_XimValueToAttribute(
    XIMResourceList	 res,
    XPointer		 buf,
    int			 buf_size,
    XPointer		 value,
    int			*len,
    unsigned long	 mode,
    XPointer		 param)
{
    int			 ret_len;

    switch (res->resource_size) {
    case XimType_SeparatorOfNestedList:
    case XimType_NEST:
	*len = 0;
	break;

    case XimType_CARD8:
	ret_len = sizeof(CARD8);
	if (buf_size < ret_len + XIM_PAD(ret_len)) {
	    *len = -1;
	    return False;
	}

	*((CARD8 *)buf) = (CARD8)(long)value;
	*len = ret_len;
	break;

    case XimType_CARD16:
	ret_len = sizeof(CARD16);
	if (buf_size < ret_len + XIM_PAD(ret_len)) {
	    *len = -1;
	    return False;
	}

	*((CARD16 *)buf) = (CARD16)(long)value;
	*len = ret_len;
	break;

    case XimType_CARD32:
    case XimType_Window:
    case XimType_XIMHotKeyState:
	ret_len = sizeof(CARD32);
	if (buf_size < ret_len + XIM_PAD(ret_len)) {
	    *len = -1;
	    return False;
	}

	*((CARD32 *)buf) = (CARD32)(long)value;
	*len = ret_len;
	break;

    case XimType_STRING8:
	if (!value) {
	    *len = 0;
	    return False;
	}

	ret_len = strlen((char *)value);
	if (buf_size < ret_len + XIM_PAD(ret_len)) {
	    *len = -1;
	    return False;
	}

	(void)memcpy((char *)buf, (char *)value, ret_len);
	*len = ret_len;
	break;

    case XimType_XRectangle:
	{
	    XRectangle	*rect = (XRectangle *)value;
	    CARD16	*buf_s = (CARD16 *)buf;

	    if (!rect) {
		*len = 0;
		return False;
	    }

	    ret_len = sizeof(INT16)		/* sizeof X */
	    	    + sizeof(INT16)		/* sizeof Y */
	            + sizeof(CARD16)		/* sizeof width */
	            + sizeof(CARD16);		/* sizeof height */
	    if (buf_size < ret_len + XIM_PAD(ret_len)) {
		*len = -1;
		return False;
	    }

	    buf_s[0] = (CARD16)rect->x;		/* X */
	    buf_s[1] = (CARD16)rect->y;		/* Y */
	    buf_s[2] = (CARD16)rect->width;	/* width */
	    buf_s[3] = (CARD16)rect->height;	/* heght */
	    *len = ret_len;
	    break;
	}

    case XimType_XPoint:
	{
	    XPoint	*point = (XPoint *)value;
	    CARD16	*buf_s = (CARD16 *)buf;

	    if (!point) {
		*len = 0;
		return False;
	    }

	    ret_len = sizeof(INT16)		/* sizeof X */
	            + sizeof(INT16);		/* sizeof Y */
	    if (buf_size < ret_len + XIM_PAD(ret_len)) {
		*len = -1;
		return False;
	    }

	    buf_s[0] = (CARD16)point->x;		/* X */
	    buf_s[1] = (CARD16)point->y;		/* Y */
	    *len = ret_len;
	    break;
	}

    case XimType_XFontSet:
	{
	    XFontSet	 font = (XFontSet)value;
	    Xic		 ic = (Xic)param;
	    char	*base_name = NULL;
	    int		 length = 0;
	    CARD16	*buf_s = (CARD16 *)buf;

	    if (!font) {
		*len = 0;
		return False;
	    }

	    if (mode & XIM_PREEDIT_ATTR) {
		base_name = ic->private.proto.preedit_font;
		length	  = ic->private.proto.preedit_font_length;
	    } else if (mode & XIM_STATUS_ATTR) {
		base_name = ic->private.proto.status_font;
		length	  = ic->private.proto.status_font_length;
	    }

	    if (!base_name) {
		*len = 0;
		return False;
	    }

	    ret_len = sizeof(CARD16)		/* sizeof length of Base name */
		    + length;			/* sizeof Base font name list */
	    if (buf_size < ret_len + XIM_PAD(ret_len)) {
		*len = -1;
		return False;
	    }

	    buf_s[0] = (INT16)length;		/* length of Base font name */
	    (void)memcpy((char *)&buf_s[1], base_name, length);
						/* Base font name list */
	    *len = ret_len;
	    break;
	}

    case XimType_XIMHotKeyTriggers:
	{
	    XIMHotKeyTriggers	*hotkey = (XIMHotKeyTriggers *)value;
	    INT32		 num;
	    CARD32		*buf_l = (CARD32 *)buf;
	    register CARD32	*key = (CARD32 *)&buf_l[1];
	    register int	 i;

	    if (!hotkey) {
		*len = 0;
		return False;
	    }
	    num = (INT32)hotkey->num_hot_key;

	    ret_len = sizeof(INT32)		/* sizeof number of key list */
	           + (sizeof(CARD32)		/* sizeof keysyn */
	           +  sizeof(CARD32)		/* sizeof modifier */
	           +  sizeof(CARD32))		/* sizeof modifier_mask */
	           *  num;			/* number of key list */
	    if (buf_size < ret_len + XIM_PAD(ret_len)) {
		*len = -1;
		return False;
	    }

	    buf_l[0] = num;		/* number of key list */
	    for (i = 0; i < num; i++, key += 3) {
		key[0] = (CARD32)(hotkey->key[i].keysym);
						/* keysym */
		key[1] = (CARD32)(hotkey->key[i].modifier);
						/* modifier */
		key[2] = (CARD32)(hotkey->key[i].modifier_mask);
						/* modifier_mask */
	    }
	    *len = ret_len;
	    break;
	}

    case XimType_XIMStringConversion:
	{
	    *len = 0;
	    break;
	}

    default:
	return False;
    }
    return True;
}

static Bool
_XimSetInnerIMAttributes(
    Xim			 im,
    XPointer		 top,
    XIMArg		*arg,
    unsigned long	 mode)
{
    XIMResourceList	 res;
    int			 check;

    if (!(res = _XimGetResourceListRec(im->private.proto.im_inner_resources,
			im->private.proto.im_num_inner_resources, arg->name)))
	return False;

    check = _XimCheckIMMode(res, mode);
    if(check == XIM_CHECK_INVALID)
	return True;
    else if(check == XIM_CHECK_ERROR)
	return False;

    return _XimEncodeLocalIMAttr(res, top, arg->value);
}

char *
_XimEncodeIMATTRIBUTE(
    Xim			  im,
    XIMResourceList	  res_list,
    unsigned int	  res_num,
    XIMArg		 *arg,
    XIMArg		**arg_ret,
    char		 *buf,
    int			  size,
    int			 *ret_len,
    XPointer		  top,
    unsigned long	  mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;
    CARD16		*buf_s;
    int			 len;
    int			 min_len = sizeof(CARD16) /* sizeof attribute ID */
				 + sizeof(INT16); /* sizeof value length */

    *ret_len = 0;
    for (p = arg; p->name; p++) {
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimSetInnerIMAttributes(im, top, p, mode))
		continue;
	    return p->name;
	}

	check = _XimCheckIMMode(res, mode);
	if (check == XIM_CHECK_INVALID)
	    continue;
	else if (check == XIM_CHECK_ERROR)
	    return p->name;

	if (!(_XimEncodeLocalIMAttr(res, top, p->value)))
	    return p->name;

	buf_s = (CARD16 *)buf;
	if (!(_XimValueToAttribute(res, (XPointer)&buf_s[2], (size - min_len),
				p->value, &len, mode, (XPointer)NULL)))
	    return p->name;

	if (len == 0) {
	    continue;
	} else if (len < 0) {
	    *arg_ret = p;
	    return (char *)NULL;
	}

	buf_s[0] = res->id;			/* attribute ID */
	buf_s[1] = len;				/* value length */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += min_len;

	buf += len;
	*ret_len += len;
	size -= len;
    }
    *arg_ret = (XIMArg *)NULL;
    return (char *)NULL;
}

#ifdef XIM_CONNECTABLE
Bool
_XimEncodeSavedIMATTRIBUTE(
    Xim			 im,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    int			*idx,
    char		*buf,
    int			 size,
    int			*ret_len,
    XPointer		 top,
    unsigned long	 mode)
{
    register int	 i;
    int			 num = im->private.proto.num_saved_imvalues;
    XrmQuark		*quark_list = im->private.proto.saved_imvalues;
    XIMResourceList	 res;
    XPointer		 value;
    CARD16		*buf_s;
    int			 len;
    int			 min_len = sizeof(CARD16) /* sizeof attribute ID */
				 + sizeof(INT16); /* sizeof value length */

    if (!im->private.proto.saved_imvalues) {
	*idx = -1;
	*ret_len = 0;
	return True;
    }

    *ret_len = 0;
    for (i = *idx; i < num; i++) {
	if (!(res = _XimGetResourceListRecByQuark(res_list,
						res_num, quark_list[i])))
	    continue;

	if (!_XimDecodeLocalIMAttr(res, top, value))
	    return False;

	buf_s = (CARD16 *)buf;
	if (!(_XimValueToAttribute(res, (XPointer)&buf_s[2],
			(size - min_len), value, &len, mode, (XPointer)NULL)))
	    return False;

	if (len == 0) {
	    continue;
	} else if (len < 0) {
	    *idx = i;
	    return True;
	}

	buf_s[0] = res->id;			/* attribute ID */
	buf_s[1] = len;				/* value length */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += min_len;

	buf += len;
	*ret_len += len;
	size -= len;
    }
    *idx = -1;
    return True;
}
#endif /* XIM_CONNECTABLE */

static Bool
_XimEncodeTopValue(
    Xic			 ic,
    XIMResourceList	 res,
    XIMArg		*p)
{
    if (res->xrm_name == XrmStringToQuark(XNClientWindow)) {
	ic->core.client_window = (Window)p->value;
	if (ic->core.focus_window == (Window)0)
	    ic->core.focus_window = ic->core.client_window;
	_XimRegisterFilter(ic);

    } else if (res->xrm_name == XrmStringToQuark(XNFocusWindow)) {
	if (ic->core.client_window) {
	    _XimUnregisterFilter(ic);
	    ic->core.focus_window = (Window)p->value;
	    _XimRegisterFilter(ic);
	} else /* client_window not yet */
	    ic->core.focus_window = (Window)p->value;
    }
    return True;
}

static Bool
_XimEncodePreeditValue(
    Xic			 ic,
    XIMResourceList	 res,
    XIMArg		*p)
{
    if (res->xrm_name == XrmStringToQuark(XNStdColormap)) {
	XStandardColormap	*colormap_ret;
	int			 count;

	if (!(XGetRGBColormaps(ic->core.im->core.display,
				ic->core.focus_window, &colormap_ret,
				&count, (Atom)p->value)))
	    return False;

	XFree(colormap_ret);
    } else if (res->xrm_name == XrmStringToQuark(XNFontSet)) {
	int		  list_ret;
	XFontStruct	**struct_list;
	char		**name_list;
	char		 *tmp;
	int		  len;
	register int	  i;

	if (!p->value)
	    return False;

	Xfree(ic->private.proto.preedit_font);

	list_ret = XFontsOfFontSet((XFontSet)p->value,
						 &struct_list, &name_list);
	for (i = 0, len = 0; i < list_ret; i++) {
	     len += (strlen(name_list[i]) + sizeof(char));
	}
	if (!(tmp = Xmalloc(len + 1))) {
	    ic->private.proto.preedit_font = NULL;
	    return False;
	}

	tmp[0] = '\0';
	for (i = 0; i < list_ret; i++) {
	    strcat(tmp, name_list[i]);
	    strcat(tmp, ",");
	}
	tmp[len - 1] = 0;
	ic->private.proto.preedit_font        = tmp;
	ic->private.proto.preedit_font_length = len - 1;
    }
    return True;
}

static Bool
_XimEncodeStatusValue(
    Xic			 ic,
    XIMResourceList	 res,
    XIMArg		*p)
{
    if (res->xrm_name == XrmStringToQuark(XNStdColormap)) {
	XStandardColormap	*colormap_ret = NULL;
	int			 count;

	if (!(XGetRGBColormaps(ic->core.im->core.display,
				ic->core.focus_window, &colormap_ret,
				&count, (Atom)p->value)))
	    return False;

	XFree(colormap_ret);
    } else if (res->xrm_name == XrmStringToQuark(XNFontSet)) {
	int		  list_ret;
	XFontStruct	**struct_list;
	char		**name_list;
	char		 *tmp;
	int		  len;
	register int	  i;

	if (!p->value)
	    return False;

	Xfree(ic->private.proto.status_font);

	list_ret = XFontsOfFontSet((XFontSet)p->value,
						 &struct_list, &name_list);
	for (i = 0, len = 0; i < list_ret; i++) {
	     len += (strlen(name_list[i]) + sizeof(char));
	}
	if (!(tmp = Xmalloc(len+1))) {
	    ic->private.proto.status_font = NULL;
	    return False;
	}

	tmp[0] = '\0';
	for(i = 0; i < list_ret; i++) {
	    strcat(tmp, name_list[i]);
	    strcat(tmp, ",");
	}
	tmp[len - 1] = 0;
	ic->private.proto.status_font        = tmp;
	ic->private.proto.status_font_length = len - 1;
    }
    return True;
}

static Bool
_XimSetInnerICAttributes(
    Xic			 ic,
    XPointer		 top,
    XIMArg		*arg,
    unsigned long	 mode)
{
    XIMResourceList	 res;
    int			 check;

    if (!(res = _XimGetResourceListRec(ic->private.proto.ic_inner_resources,
			ic->private.proto.ic_num_inner_resources, arg->name)))
	return False;

    check = _XimCheckICMode(res, mode);
    if(check == XIM_CHECK_INVALID)
	return True;
    else if(check == XIM_CHECK_ERROR)
	return False;

    return _XimEncodeLocalICAttr(ic, res, top, arg, mode);
}

char *
_XimEncodeICATTRIBUTE(
    Xic			  ic,
    XIMResourceList	  res_list,
    unsigned int	  res_num,
    XIMArg		 *arg,
    XIMArg		**arg_ret,
    char		 *buf,
    int			  size,
    int			 *ret_len,
    XPointer		  top,
    BITMASK32		 *flag,
    unsigned long	  mode)
{
    register XIMArg	*p;
    XIMResourceList	 res;
    int			 check;
    CARD16		*buf_s;
    int			 len;
    int			 min_len = sizeof(CARD16) /* sizeof attribute ID */
				 + sizeof(INT16); /* sizeof value length */
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;
    char		*name;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);

    *ret_len = 0;
    for (p = arg; p && p->name; p++) {
	buf_s = (CARD16 *)buf;
	if (!(res = _XimGetResourceListRec(res_list, res_num, p->name))) {
	    if (_XimSetInnerICAttributes(ic, top, p, mode))
		continue;
	    return p->name;
	}

	check = _XimCheckICMode(res, mode);
	if (check == XIM_CHECK_INVALID)
	    continue;
	else if (check == XIM_CHECK_ERROR)
	    return p->name;

	if (mode & XIM_PREEDIT_ATTR) {
	    if (!(_XimEncodePreeditValue(ic, res, p)))
		return p->name;
	} else if (mode & XIM_STATUS_ATTR) {
	    if (!(_XimEncodeStatusValue(ic, res, p)))
		return p->name;
	} else {
	    if (!(_XimEncodeTopValue(ic, res, p)))
		return p->name;
	}

	if (res->resource_size == XimType_NEST) {
	    XimDefICValues	*ic_attr = (XimDefICValues *)top;

	    if (res->xrm_name == pre_quark) {
		XIMArg		*arg_rt;
		if ((name = _XimEncodeICATTRIBUTE(ic, res_list, res_num,
				(XIMArg *)p->value, &arg_rt,
				(char *)&buf_s[2], (size - min_len),
				 &len, (XPointer)&ic_attr->preedit_attr, flag,
				(mode | XIM_PREEDIT_ATTR)))) {
		    return name;
		}

	    } else if (res->xrm_name == sts_quark) {
		XIMArg		*arg_rt;
		if ((name = _XimEncodeICATTRIBUTE(ic, res_list, res_num,
				(XIMArg *)p->value,  &arg_rt,
				(char *)&buf_s[2], (size - min_len),
				 &len, (XPointer)&ic_attr->status_attr, flag,
				(mode | XIM_STATUS_ATTR)))) {
		    return name;
		}
	    }
	} else {
#ifdef EXT_MOVE
	    if (flag)
		*flag |= _XimExtenArgCheck(p);
#endif
    	    if (!(_XimEncodeLocalICAttr(ic, res, top, p, mode)))
		return p->name;

	    if (!(_XimValueToAttribute(res, (XPointer)&buf_s[2],
			 	(size - min_len), p->value,
				&len, mode, (XPointer)ic)))
		return p->name;
	}

	if (len == 0) {
	    continue;
	} else if (len < 0) {
	    *arg_ret = p;
	    return (char *)NULL;
	}

	buf_s[0] = res->id;			/* attribute ID */
	buf_s[1] = len;				/* value length */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += min_len;

	buf += len;
	*ret_len += len;
	size -= len;
    }
    *arg_ret = (XIMArg *)NULL;
    return (char *)NULL;
}

#ifdef XIM_CONNECTABLE
static Bool
_XimEncodeSavedPreeditValue(
    Xic			  ic,
    XIMResourceList	  res,
    XPointer		  value)
{
    int			  list_ret;
    XFontStruct		**struct_list;
    char		**name_list;
    char		 *tmp;
    int			  len;
    register int	  i;

    if (res->xrm_name == XrmStringToQuark(XNFontSet)) {
	if (!value)
	    return False;

	if (ic->private.proto.preedit_font)
	    Xfree(ic->private.proto.preedit_font);

	list_ret = XFontsOfFontSet((XFontSet)value,
						&struct_list, &name_list);
	for(i = 0, len = 0; i < list_ret; i++) {
	    len += (strlen(name_list[i]) + sizeof(char));
	}
	if(!(tmp = Xmalloc(len + 1))) {
	    ic->private.proto.preedit_font = NULL;
	    return False;
	}

	tmp[0] = '\0';
	for(i = 0; i < list_ret; i++) {
	    strcat(tmp, name_list[i]);
	    strcat(tmp, ",");
	}
	tmp[len - 1] = 0;
	ic->private.proto.preedit_font        = tmp;
	ic->private.proto.preedit_font_length = len - 1;
    }
    return True;
}

static Bool
_XimEncodeSavedStatusValue(
    Xic			  ic,
    XIMResourceList	  res,
    XPointer		  value)
{
    int			  list_ret;
    XFontStruct		**struct_list;
    char		**name_list;
    char		 *tmp;
    int			  len;
    register int	  i;

    if (res->xrm_name == XrmStringToQuark(XNFontSet)) {
	if (!value)
	    return False;

	Xfree(ic->private.proto.status_font);

	list_ret = XFontsOfFontSet((XFontSet)value,
						&struct_list, &name_list);
	for(i = 0, len = 0; i < list_ret; i++) {
	    len += (strlen(name_list[i]) + sizeof(char));
	}
	if(!(tmp = Xmalloc(len + 1))) {
	    ic->private.proto.status_font = NULL;
	    return False;
	}

	tmp[0] = '\0';
	for(i = 0; i < list_ret; i++) {
	    strcat(tmp, name_list[i]);
	    strcat(tmp, ",");
	}
	tmp[len - 1] = 0;
	ic->private.proto.status_font        = tmp;
	ic->private.proto.status_font_length = len - 1;
    }
    return True;
}

Bool
_XimEncodeSavedICATTRIBUTE(
    Xic			 ic,
    XIMResourceList	 res_list,
    unsigned int	 res_num,
    int			*idx,
    char		*buf,
    int			 size,
    int			*ret_len,
    XPointer		 top,
    unsigned long	 mode)
{
    int			 i;
    int			 num = ic->private.proto.num_saved_icvalues;
    XrmQuark		*quark_list = ic->private.proto.saved_icvalues;
    XIMResourceList	 res;
    XPointer		 value;
    CARD16		*buf_s;
    int			 len;
    int			 min_len = sizeof(CARD16) /* sizeof attribute ID */
				 + sizeof(INT16); /* sizeof value length */
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;
    XrmQuark		 separator;

    if (!ic->private.proto.saved_icvalues) {
	*idx = -1;
	*ret_len = 0;
	return True;
    }

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);
    separator = XrmStringToQuark(XNSeparatorofNestedList);

    *ret_len = 0;
    for (i = *idx; i < num; i++) {
	if (quark_list[i] == separator) {
	    *idx = i;
	    return True;
	}

	if (!(res = _XimGetResourceListRecByQuark(res_list,
						res_num, quark_list[i])))
	    continue;

	if (!_XimDecodeLocalICAttr(res, top,(XPointer)&value, mode))
	    return False;

	if (mode & XIM_PREEDIT_ATTR) {
	    if (!(_XimEncodeSavedPreeditValue(ic, res, value))) {
		return False;
	    }
	} else if (mode & XIM_STATUS_ATTR) {
	    if (!(_XimEncodeSavedStatusValue(ic, res, value))) {
		return False;
	    }
	}

	buf_s = (CARD16 *)buf;
	if (res->resource_size == XimType_NEST) {
	    XimDefICValues	*ic_attr = (XimDefICValues *)top;

	    i++;
	    if (res->xrm_name == pre_quark) {
		if (!_XimEncodeSavedICATTRIBUTE(ic, res_list, res_num,
				 &i, (char *)&buf_s[2], (size - min_len),
				 &len, (XPointer)&ic_attr->preedit_attr,
				(mode | XIM_PREEDIT_ATTR))) {
		    return False;
		}

	    } else if (res->xrm_name == sts_quark) {
		if (!_XimEncodeSavedICATTRIBUTE(ic, res_list, res_num,
				&i, (char *)&buf_s[2], (size - min_len),
				&len, (XPointer)&ic_attr->status_attr,
				(mode | XIM_STATUS_ATTR))) {
		    return False;
		}
	    }
	} else {
	    if (!(_XimValueToAttribute(res, (XPointer)&buf_s[2],
			 	(size - min_len), value,
				&len, mode, (XPointer)ic))) {
		return False;
	    }
	}

	if (len == 0) {
	    continue;
	} else if (len < 0) {
	    if (quark_list[i] == separator)
		i++;
	    *idx = i;
	    return True;
	}

	buf_s[0] = res->id;			/* attribute ID */
	buf_s[1] = len;				/* value length */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += min_len;

	buf += len;
	*ret_len += len;
	size -= len;
    }
    *idx = -1;
    return True;
}
#endif /* XIM_CONNECTABLE */

static unsigned int
_XimCountNumberOfAttr(
    CARD16	  total,
    CARD16	 *attr,
    unsigned int *names_len)
{
    unsigned int n;
    CARD16	 len;
    CARD16	 min_len = sizeof(CARD16)	/* sizeof attribute ID */
			 + sizeof(CARD16)	/* sizeof type of value */
			 + sizeof(INT16);	/* sizeof length of attribute */

    n = 0;
    *names_len = 0;
    while (total > min_len) {
	len = attr[2];
	if (len > (total - min_len)) {
	    return 0;
	}
	*names_len += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	total -= len;
	attr = (CARD16 *)((char *)attr + len);
	n++;
    }
    return n;
}

Bool
_XimGetAttributeID(
    Xim			  im,
    CARD16		 *buf)
{
    unsigned int	  n, names_len, values_len;
    XIMResourceList	  res;
    char		 *names;
    XPointer		  tmp;
    XIMValuesList	 *values_list;
    char		**values;
    register int	  i;
    CARD16		  len;
    CARD16		  min_len = sizeof(CARD16) /* sizeof attribute ID */
				  + sizeof(CARD16) /* sizeof type of value */
				  + sizeof(INT16); /* sizeof length of attr */
    /*
     * IM attribute ID
     */

    if (!(n = _XimCountNumberOfAttr(buf[0], &buf[1], &names_len)))
	return False;

    if (!(res = Xcalloc(n, sizeof(XIMResource))))
	return False;

    values_len = sizeof(XIMValuesList) + (sizeof(char **) * n) + names_len;
    if (!(tmp = Xcalloc(1, values_len))) {
	Xfree(res);
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    values = (char **)((char *)tmp + sizeof(XIMValuesList));
    names = (char *)((char *)values + (sizeof(char **) * n));

    values_list->count_values = n;
    values_list->supported_values = values;

    buf++;
    for (i = 0; i < n; i++) {
	len = buf[2];
	(void)memcpy(names, (char *)&buf[3], len);
	values[i] = names;
	names[len] = '\0';
	res[i].resource_name = names;
	res[i].resource_size = buf[1];
	res[i].id	     = buf[0];
	names += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	buf = (CARD16 *)((char *)buf + len);
    }
    _XIMCompileResourceList(res, n);

    Xfree(im->core.im_resources);
    Xfree(im->core.im_values_list);

    im->core.im_resources     = res;
    im->core.im_num_resources = n;
    im->core.im_values_list   = values_list;

    /*
     * IC attribute ID
     */

    if (!(n = _XimCountNumberOfAttr(buf[0], &buf[2], &names_len)))
	return False;

    if (!(res = Xcalloc(n, sizeof(XIMResource))))
	return False;

    values_len = sizeof(XIMValuesList) + (sizeof(char **) * n) + names_len;
    if (!(tmp = Xcalloc(1, values_len))) {
	Xfree(res);
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    values = (char **)((char *)tmp + sizeof(XIMValuesList));
    names = (char *)((char *)values + (sizeof(char **) * n));

    values_list->count_values = n;
    values_list->supported_values = values;

    buf += 2;
    for (i = 0; i < n; i++) {
	len = buf[2];
	(void)memcpy(names, (char *)&buf[3], len);
	values[i] = names;
	names[len] = '\0';
	res[i].resource_name = names;
	res[i].resource_size = buf[1];
	res[i].id	     = buf[0];
	names += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	buf = (CARD16 *)((char *)buf + len);
    }
    _XIMCompileResourceList(res, n);


    Xfree(im->core.ic_resources);
    Xfree(im->core.ic_values_list);

    im->core.ic_resources     = res;
    im->core.ic_num_resources = n;
    im->core.ic_values_list   = values_list;

    return True;
}
