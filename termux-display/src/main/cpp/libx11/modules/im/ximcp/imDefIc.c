/*
 * Copyright (c) 1991, 1992, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
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

  Author: Hideki Hiura (hhiura@Sun.COM) Sun Microsystems, Inc.
          Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"

static Bool
_XimCreateICCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];

    if ((major_opcode == XIM_CREATE_IC_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid))
	return True;
    return False;
}

#ifdef XIM_CONNECTABLE
Bool
_XimReCreateIC(ic)
    Xic			 ic;
{
    Xim			 im = (Xim)ic->core.im;
    Xic			 save_ic;
    XIMResourceList	 res;
    unsigned int         num;
    XIMStyle		 input_style = ic->core.input_style;
    XimDefICValues	 ic_values;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    int			 idx;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;

    if (!(save_ic = Xmalloc(sizeof(XicRec))))
	return False;
    memcpy((char *)save_ic, (char *)ic, sizeof(XicRec));

    ic->core.filter_events = im->private.proto.forward_event_mask;
    ic->private.proto.forward_event_mask =
				im->private.proto.forward_event_mask;
    ic->private.proto.synchronous_event_mask =
				im->private.proto.synchronous_event_mask;

    num = im->core.ic_num_resources;
    buf_size = sizeof(XIMResource) * num;
    if (!(res = Xmalloc(buf_size)))
	goto ErrorOnReCreateIC;
    (void)memcpy((char *)res, (char *)im->core.ic_resources, buf_size);
    ic->private.proto.ic_resources     = res;
    ic->private.proto.ic_num_resources = num;

    num = im->private.proto.ic_num_inner_resources;
    buf_size = sizeof(XIMResource) * num;
    if (!(res = Xmalloc(buf_size)))
	goto ErrorOnReCreateIC;
    (void)memcpy((char *)res,
			(char *)im->private.proto.ic_inner_resources, buf_size);
    ic->private.proto.ic_inner_resources     = res;
    ic->private.proto.ic_num_inner_resources = num;

    _XimSetICMode(ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, input_style);

    _XimSetICMode(ic->private.proto.ic_inner_resources,
			ic->private.proto.ic_num_inner_resources, input_style);

    _XimGetCurrentICValues(ic, &ic_values);
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    idx = 0;
    for (;;) {
	data = &buf[buf_size];
	if (!_XimEncodeSavedICATTRIBUTE(ic, ic->private.proto.ic_resources,
		ic->private.proto.ic_num_resources, &idx, data, data_len,
		&ret_len, (XPointer)&ic_values, XIM_CREATEIC)) {
	    if (buf != tmp_buf)
		Xfree(buf);
	    goto ErrorOnReCreateIC;
	}

	total += ret_len;
	if (idx == -1) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xmalloc(buf_size + data_len))) {
		goto ErrorOnReCreateIC;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		goto ErrorOnReCreateIC;
	    }
	    buf = tmp;
	}
    }

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_CREATE_IC, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	goto ErrorOnReCreateIC;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    ic->private.proto.waitCallback = True;
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
						 _XimCreateICCheck, 0);
    if (ret_code == XIM_TRUE) {
	preply = reply;
    } else if (ret_code == XIM_OVERFLOW) {
	if (len <= 0) {
	    preply = reply;
	} else {
	    buf_size = (int)len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
						 _XimCreateICCheck, 0);
	    if (ret_code != XIM_TRUE) {
		Xfree(preply);
		ic->private.proto.waitCallback = False;
		goto ErrorOnReCreateIC;
	    }
	}
    } else {
	ic->private.proto.waitCallback = False;
	goto ErrorOnReCreateIC;
    }
    ic->private.proto.waitCallback = False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if (reply != preply)
	    Xfree(preply);
	goto ErrorOnReCreateIC;
    }

    ic->private.proto.icid = buf_s[1];		/* icid */
    if (reply != preply)
	Xfree(preply);

    _XimRegisterFilter(ic);
    MARK_IC_CONNECTED(ic);

    Xfree(save_ic->private.proto.ic_resources);
    Xfree(save_ic->private.proto.ic_inner_resources);
    Xfree(save_ic);
    return True;

ErrorOnReCreateIC:
    memcpy((char *)ic, (char *)save_ic, sizeof(XicRec));
    Xfree(save_ic);
    return False;
}

static char *
_XimDelayModeGetICValues(ic, arg)
    Xic			 ic;
    XIMArg		*arg;
{
    XimDefICValues	 ic_values;

    _XimGetCurrentICValues(ic, &ic_values);
    return  _XimGetICValueData(ic, (XPointer)&ic_values,
				ic->private.proto.ic_resources,
				ic->private.proto.ic_num_resources,
				arg, XIM_GETICVALUES);
}
#endif /* XIM_CONNECTABLE */

static Bool
_XimGetICValuesCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];

    if ((major_opcode == XIM_GET_IC_VALUES_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
	return True;
    return False;
}

static char *
_XimProtoGetICValues(
    XIC			 xic,
    XIMArg		*arg)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    register XIMArg	*p;
    register XIMArg	*pp;
    register int	 n;
    CARD8		*buf;
    CARD16		*buf_s;
    INT16		 len;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply = NULL;
    int			 buf_size;
    int			 ret_code;
    char		*makeid_name;
    char		*decode_name;
    CARD16		*data = NULL;
    INT16		 data_len = 0;

#ifndef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic))
	return arg->name;
#else
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
	            return _XimDelayModeGetICValues(ic, arg);
		}
	    } else {
	        return _XimDelayModeGetICValues(ic, arg);
	    }
        } else {
	    return arg->name;
        }
    }
#endif /* XIM_CONNECTABLE */

    for (n = 0, p = arg; p && p->name; p++) {
	n++;
	if ((strcmp(p->name, XNPreeditAttributes) == 0)
	 || (strcmp(p->name, XNStatusAttributes) == 0)) {
	     n++;
	     for (pp = (XIMArg *)p->value; pp && pp->name; pp++)
	 	n++;
	}
    }

    if (!n)
	return (char *)NULL;

    buf_size =  sizeof(CARD16) * n;
    buf_size += XIM_HEADER_SIZE
	     + sizeof(CARD16)
	     + sizeof(CARD16)
	     + sizeof(INT16)
	     + XIM_PAD(2 + buf_size);

    if (!(buf = Xcalloc(buf_size, 1)))
	return arg->name;
    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

    makeid_name = _XimMakeICAttrIDList(ic, ic->private.proto.ic_resources,
				ic->private.proto.ic_num_resources, arg,
				&buf_s[3], &len, XIM_GETICVALUES);

    if (len > 0) {
	buf_s[0] = im->private.proto.imid;		/* imid */
	buf_s[1] = ic->private.proto.icid;		/* icid */
	buf_s[2] = len;				/* length of ic-attr-id */
	len += sizeof(INT16);                       /* sizeof length of attr */
	XIM_SET_PAD(&buf_s[2], len);		/* pad */
	len += sizeof(CARD16)			/* sizeof imid */
	     + sizeof(CARD16);			/* sizeof icid */

	_XimSetHeader((XPointer)buf, XIM_GET_IC_VALUES, 0, &len);
	if (!(_XimWrite(im, len, (XPointer)buf))) {
	    Xfree(buf);
	    return arg->name;
	}
	_XimFlush(im);
	Xfree(buf);
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
				 _XimGetICValuesCheck, (XPointer)ic);
	if (ret_code == XIM_TRUE) {
	    preply = reply;
	} else if (ret_code == XIM_OVERFLOW) {
	    if (len <= 0) {
		preply = reply;
	    } else {
		buf_size = (int)len;
		preply = Xmalloc(len);
		ret_code = _XimRead(im, &len, preply, buf_size,
				_XimGetICValuesCheck, (XPointer)ic);
		if (ret_code != XIM_TRUE) {
		    if (preply != reply)
		        Xfree(preply);
		    return arg->name;
		}
	    }
	} else {
	    return arg->name;
	}
	buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
	if (*((CARD8 *)preply) == XIM_ERROR) {
	    _XimProcError(im, 0, (XPointer)&buf_s[3]);
	    if (reply != preply)
		Xfree(preply);
	    return arg->name;
	}
	data = &buf_s[4];
	data_len = buf_s[2];
    }
    else if (len < 0) {
	return arg->name;
    }

    decode_name = _XimDecodeICATTRIBUTE(ic, ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, data, data_len,
			arg, XIM_GETICVALUES);
    if (reply != preply)
	Xfree(preply);

    if (decode_name)
	return decode_name;
    else
	return makeid_name;
}

#ifdef XIM_CONNECTABLE
static Bool
_XimCheckNestQuarkList(quark_list, num_quark, quark, separator)
    XrmQuark		*quark_list;
    int			 num_quark;
    XrmQuark		 quark;
    XrmQuark		 separator;
{
    register int	 i;

    for (i = 0; i < num_quark; i++) {
	if (quark_list[i] == separator) {
	    break;
	}
	if (quark_list[i] == quark) {
	    return True;
	}
    }
    return False;
}

static Bool
_XimCheckNestedQuarkList(quark_list, idx, num_quark, arg, separator)
    XrmQuark		**quark_list;
    int			  idx;
    int			 *num_quark;
    XIMArg		 *arg;
    XrmQuark		  separator;
{
    XrmQuark		 *q_list = *quark_list;
    int			  n_quark = *num_quark;
    register XIMArg	 *p;
    XrmQuark		  quark;
    XrmQuark		 *tmp;
    register int	  i;

    for (p = arg; p && p->name; p++) {
	quark = XrmStringToQuark(p->name);
	if (_XimCheckNestQuarkList(&q_list[idx], n_quark - idx,
							quark, separator)) {
	    continue;
	}
	if (!(tmp = Xmalloc((sizeof(XrmQuark) * (n_quark + 1))))) {
	    *quark_list = q_list;
	    *num_quark = n_quark;
	    return False;
	}
	n_quark++;
	for (i = 0; i < idx; i++) {
	    tmp[i] = q_list[i];
	}
	tmp[i] = quark;
	for (i = idx  + 1; i < n_quark; i++) {
	    tmp[i] = q_list[i - 1];
	}
	q_list = tmp;
    }
    *quark_list = q_list;
    *num_quark = n_quark;
    return True;
}

static Bool
_XimCheckICQuarkList(quark_list, num_quark, quark, idx)
    XrmQuark		*quark_list;
    int			 num_quark;
    XrmQuark		 quark;
    int			*idx;
{
    register int	 i;

    for (i = 0; i < num_quark; i++) {
	if (quark_list[i] == quark) {
	    *idx = i;
	    return True;
	}
    }
    return False;
}

static Bool
_XimSaveICValues(ic, arg)
    Xic			 ic;
    XIMArg		*arg;
{
    register XIMArg	*p;
    register int	 n;
    XrmQuark		*quark_list;
    XrmQuark		*tmp;
    XrmQuark		 quark;
    int			 num_quark;
    XrmQuark		 pre_quark;
    XrmQuark		 sts_quark;
    XrmQuark		 separator;
    int			 idx;

    pre_quark = XrmStringToQuark(XNPreeditAttributes);
    sts_quark = XrmStringToQuark(XNStatusAttributes);
    separator = XrmStringToQuark(XNSeparatorofNestedList);

    if (quark_list = ic->private.proto.saved_icvalues) {
	num_quark = ic->private.proto.num_saved_icvalues;
	for (p = arg; p && p->name; p++) {
	    quark = XrmStringToQuark(p->name);
	    if ((quark == pre_quark) || (quark == sts_quark)) {
	        if (!_XimCheckICQuarkList(quark_list, num_quark, quark, &idx)) {
		    register XIMArg	*pp;
		    int			 nn;
		    XrmQuark		*q_list;

		    for (pp = (XIMArg *)p->value, nn = 0;
						pp && pp->name; pp++, nn++);
	            if (!(tmp = Xrealloc(quark_list,
				(sizeof(XrmQuark) * (num_quark + nn + 2))))) {
		        ic->private.proto.saved_icvalues = quark_list;
		        ic->private.proto.num_saved_icvalues = num_quark;
		        return False;
	            }
	            quark_list = tmp;
		    q_list = &quark_list[num_quark];
	            num_quark += nn + 2;
		    *q_list++ = quark;
		    for (pp = (XIMArg *)p->value;
					pp && pp->name; pp++, quark_list++) {
			*q_list = XrmStringToQuark(pp->name);
		    }
		    *q_list = separator;
		} else {
		    if (!_XimCheckNestedQuarkList(&quark_list, idx + 1,
				&num_quark, (XIMArg *)p->value, separator)) {
		        ic->private.proto.saved_icvalues = quark_list;
		        ic->private.proto.num_saved_icvalues = num_quark;
		        return False;
		    }
		}
	    } else {
	        if (_XimCheckICQuarkList(quark_list, num_quark, quark, &idx)) {
		    continue;
	        }
	        if (!(tmp = Xrealloc(quark_list,
				(sizeof(XrmQuark) * (num_quark + 1))))) {
		    ic->private.proto.saved_icvalues = quark_list;
		    ic->private.proto.num_saved_icvalues = num_quark;
		    return False;
	        }
	        quark_list = tmp;
	        quark_list[num_quark] = quark;
	        num_quark++;
	    }
	}
	ic->private.proto.saved_icvalues = quark_list;
	ic->private.proto.num_saved_icvalues = num_quark;
	return True;
    }

    for (p = arg, n = 0; p && p->name; p++, n++) {
	if ((!strcmp(p->name, XNPreeditAttributes))
	 || (!strcmp(p->name, XNStatusAttributes))) {
	    register XIMArg	*pp;
	    int			 nn;

	    for (pp = (XIMArg *)p->value, nn = 0; pp && pp->name; pp++, nn++);
	    n += nn + 1;
	}
    }

    if (!(quark_list = Xmalloc(sizeof(XrmQuark) * n))) {
	return False;
    }

    ic->private.proto.saved_icvalues = quark_list;
    ic->private.proto.num_saved_icvalues = n;
    for (p = arg; p && p->name; p++, quark_list++) {
	*quark_list = XrmStringToQuark(p->name);
	if ((*quark_list == pre_quark) || (*quark_list == sts_quark)) {
	    register XIMArg	*pp;

	    quark_list++;
	    for (pp = (XIMArg *)p->value; pp && pp->name; pp++, quark_list++) {
		*quark_list = XrmStringToQuark(pp->name);
	    }
	    *quark_list = separator;
	}
    }
    return True;
}

static char *
_XimDelayModeSetICValues(ic, arg)
    Xic			 ic;
    XIMArg		*arg;
{
    XimDefICValues	 ic_values;
    char		*name;

    _XimGetCurrentICValues(ic, &ic_values);
    name = _XimSetICValueData(ic, (XPointer)&ic_values,
			ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources,
			arg, XIM_SETICVALUES, False);
    _XimSetCurrentICValues(ic, &ic_values);
    return name;
}
#endif /* XIM_CONNECTABLE */

static Bool
_XimSetICValuesCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];

    if ((major_opcode == XIM_SET_IC_VALUES_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
	return True;
    return False;
}

static char *
_XimProtoSetICValues(
    XIC			 xic,
    XIMArg		*arg)
{
    Xic			 ic = (Xic)xic;
    Xim			 im = (Xim)ic->core.im;
    XimDefICValues	 ic_values;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    XIMArg		*arg_ret;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply = NULL;
    int			 ret_code;
    BITMASK32		 flag = 0L;
    char		*name;
    char		*tmp_name = (arg) ? arg->name : NULL;

#ifndef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic))
	return tmp_name;
#else
    if (!_XimSaveICValues(ic, arg))
	return NULL;

    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
	        if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
	            return _XimDelayModeSetICValues(ic, arg);
		}
	    } else {
	        return _XimDelayModeSetICValues(ic, arg);
	    }
        } else {
	    return tmp_name;
        }
    }
#endif /* XIM_CONNECTABLE */

    _XimGetCurrentICValues(ic, &ic_values);
    memset(tmp_buf, 0, sizeof(tmp_buf32));
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE
	+ sizeof(CARD16) + sizeof(CARD16) + sizeof(INT16) + sizeof(CARD16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    arg_ret = arg;
    for (;;) {
	data = &buf[buf_size];
	if ((name = _XimEncodeICATTRIBUTE(ic, ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, arg, &arg_ret,
			data, data_len, &ret_len, (XPointer)&ic_values,
			&flag, XIM_SETICVALUES))) {
	    break;
	}

	total += ret_len;
	if (!(arg = arg_ret)) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xcalloc(buf_size + data_len, 1))) {
		return tmp_name;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
		return tmp_name;
	    }
            memset(&tmp[buf_size], 0, data_len);
	    buf = tmp;
	}
    }
    _XimSetCurrentICValues(ic, &ic_values);

    if (!total) {
        return tmp_name;
    }

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];

#ifdef EXT_MOVE
    if (_XimExtenMove(im, ic, flag, &buf_s[4], (INT16)total))
	return name;
#endif

    buf_s[0] = im->private.proto.imid;
    buf_s[1] = ic->private.proto.icid;
    buf_s[2] = (INT16)total;
    buf_s[3] = 0;
    len = (INT16)(sizeof(CARD16) + sizeof(CARD16)
				+ sizeof(INT16) + sizeof(CARD16) + total);

    _XimSetHeader((XPointer)buf, XIM_SET_IC_VALUES, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	return tmp_name;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    ic->private.proto.waitCallback = True;
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimSetICValuesCheck, (XPointer)ic);
    if (ret_code == XIM_TRUE) {
	preply = reply;
    } else if (ret_code == XIM_OVERFLOW) {
	buf_size = (int)len;
	preply = Xmalloc(buf_size);
	ret_code = _XimRead(im, &len, preply, buf_size,
					_XimSetICValuesCheck, (XPointer)ic);
	if (ret_code != XIM_TRUE) {
	    Xfree(preply);
	    ic->private.proto.waitCallback = False;
	    return tmp_name;
	}
    } else {
	ic->private.proto.waitCallback = False;
	return tmp_name;
    }
    ic->private.proto.waitCallback = False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if (reply != preply)
	    Xfree(preply);
	return tmp_name;
    }
    if (reply != preply)
	Xfree(preply);

    return name;
}

static Bool
_XimDestroyICCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];
    Bool	 ret = False;

    if ((major_opcode == XIM_DESTROY_IC_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	ret = True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
        ret = False;
    return ret;
}

static void
_XimProtoICFree(
    Xic		 ic)
{
#ifdef XIM_CONNECTABLE
    Xim		 im = (Xim)ic->core.im;
#endif


    Xfree(ic->private.proto.preedit_font);
    ic->private.proto.preedit_font = NULL;


    Xfree(ic->private.proto.status_font);
    ic->private.proto.status_font = NULL;

    if (ic->private.proto.commit_info) {
	_XimFreeCommitInfo(ic);
	ic->private.proto.commit_info = NULL;
    }

    Xfree(ic->private.proto.ic_inner_resources);
    ic->private.proto.ic_inner_resources = NULL;


#ifdef XIM_CONNECTABLE
    if (IS_SERVER_CONNECTED(im) && IS_RECONNECTABLE(im)) {
	return;
    }
#endif /* XIM_CONNECTABLE */


    Xfree(ic->private.proto.saved_icvalues);
    ic->private.proto.saved_icvalues = NULL;


    Xfree(ic->private.proto.ic_resources);
    ic->private.proto.ic_resources = NULL;


    Xfree(ic->core.hotkey);
    ic->core.hotkey = NULL;


    return;
}

static void
_XimProtoDestroyIC(
    XIC		 xic)
{
    Xic		 ic = (Xic)xic;
    Xim	 	 im = (Xim)ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;

    if (IS_SERVER_CONNECTED(im)) {
	buf_s[0] = im->private.proto.imid;		/* imid */
	buf_s[1] = ic->private.proto.icid;		/* icid */

	len = sizeof(CARD16)			/* sizeof imid */
	    + sizeof(CARD16);			/* sizeof icid */

	_XimSetHeader((XPointer)buf, XIM_DESTROY_IC, 0, &len);
	(void)_XimWrite(im, len, (XPointer)buf);
	_XimFlush(im);
	buf_size = BUFSIZE;
	ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
					_XimDestroyICCheck, (XPointer)ic);
	if (ret_code == XIM_OVERFLOW) {
	    buf_size = len;
	    preply = Xmalloc(buf_size);
	    (void)_XimRead(im, &len, preply, buf_size,
					_XimDestroyICCheck, (XPointer)ic);
	    Xfree(preply);
	}
    }
    UNMARK_IC_CONNECTED(ic);
    _XimUnregisterFilter(ic);
    _XimProtoICFree(ic);
    return;
}

/*
 * Some functions require the request queue from the server to be flushed
 * so that the ordering of client initiated status changes and those requested
 * by the server is well defined.
 * _XimSync() would be the function of choice here as it should get a
 * XIM_SYNC_REPLY back from the server.
 * This however isn't implemented in the piece of junk that is used by most
 * input servers as the server side protocol if to XIM.
 * Since this code is not shipped as a library together with the client side
 * XIM code but is duplicated by every input server around the world there
 * is no easy fix to this but this ugly hack below.
 * Obtaining an IC value from the server sends a request and empties out the
 * event/server request queue until the answer to this request is found.
 * Thus it is guaranteed that any pending server side request gets processed.
 * This is what the hack below is doing.
 */

static void
BrokenSyncWithServer(XIC xic)
{
    CARD32 dummy;
    XGetICValues(xic, XNFilterEvents, &dummy, NULL);
}

static void
_XimProtoSetFocus(
    XIC		 xic)
{
    Xic		 ic = (Xic)xic;
    Xim		 im = (Xim)ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;

#ifndef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic))
	return;
#else
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
		    return;
		}
	    } else {
		return;
	    }
	} else {
	    return;
	}
    }
#endif /* XIM_CONNECTABLE */
    BrokenSyncWithServer(xic);

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */

    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16);			/* sizeof icid */

    _XimSetHeader((XPointer)buf, XIM_SET_IC_FOCUS, 0, &len);
    (void)_XimWrite(im, len, (XPointer)buf);
    _XimFlush(im);

    _XimRegisterFilter(ic);
    return;
}

static void
_XimProtoUnsetFocus(
    XIC		 xic)
{
    Xic		 ic = (Xic)xic;
    Xim		 im = (Xim)ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;

#ifndef XIM_CONNECTABLE
    if (!IS_IC_CONNECTED(ic))
	return;
#else
    if (!IS_IC_CONNECTED(ic)) {
	if (IS_CONNECTABLE(im)) {
	    if (_XimConnectServer(im)) {
		if (!_XimReCreateIC(ic)) {
		    _XimDelayModeSetAttr(im);
		    return;
		}
	    } else {
		return;
	    }
	} else {
	    return;
	}
    }
#endif /* XIM_CONNECTABLE */

    BrokenSyncWithServer(xic);

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */

    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16);			/* sizeof icid */

    _XimSetHeader((XPointer)buf, XIM_UNSET_IC_FOCUS, 0, &len);
    (void)_XimWrite(im, len, (XPointer)buf);
    _XimFlush(im);

    _XimUnregisterFilter(ic);
    return;
}

static Bool
_XimResetICCheck(
    Xim          im,
    INT16        len,
    XPointer	 data,
    XPointer     arg)
{
    Xic		 ic = (Xic)arg;
    CARD16	*buf_s = (CARD16 *)((CARD8 *)data + XIM_HEADER_SIZE);
    CARD8	 major_opcode = *((CARD8 *)data);
    CARD8	 minor_opcode = *((CARD8 *)data + 1);
    XIMID	 imid = buf_s[0];
    XICID	 icid = buf_s[1];

    if ((major_opcode == XIM_RESET_IC_REPLY)
     && (minor_opcode == 0)
     && (imid == im->private.proto.imid)
     && (icid == ic->private.proto.icid))
	return True;
    if ((major_opcode == XIM_ERROR)
     && (minor_opcode == 0)
     && (buf_s[2] & XIM_IMID_VALID)
     && (imid == im->private.proto.imid)
     && (buf_s[2] & XIM_ICID_VALID)
     && (icid == ic->private.proto.icid))
	return True;
    return False;
}

static char *
_XimProtoReset(
    XIC		 xic,
    char *     (*retfunc) (Xim im, Xic ic, XPointer buf) )
{
    Xic		 ic = (Xic)xic;
    Xim	 	 im = (Xim)ic->core.im;
    CARD32	 buf32[BUFSIZE/4];
    CARD8	*buf = (CARD8 *)buf32;
    CARD16	*buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    INT16	 len;
    CARD32	 reply32[BUFSIZE/4];
    char	*reply = (char *)reply32;
    XPointer	 preply;
    int		 buf_size;
    int		 ret_code;
    char	*commit;

    if (!IS_IC_CONNECTED(ic))
	return (char *)NULL;

    buf_s[0] = im->private.proto.imid;		/* imid */
    buf_s[1] = ic->private.proto.icid;		/* icid */

    len = sizeof(CARD16)			/* sizeof imid */
	+ sizeof(CARD16);			/* sizeof icid */

    _XimSetHeader((XPointer)buf, XIM_RESET_IC, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf)))
	return NULL;
    _XimFlush(im);
    ic->private.proto.waitCallback = True;
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
    					_XimResetICCheck, (XPointer)ic);
    if (ret_code == XIM_TRUE) {
    	preply = reply;
    } else if (ret_code == XIM_OVERFLOW) {
    	if (len < 0) {
    	    preply = reply;
    	} else {
    	    buf_size = len;
	    preply = Xmalloc(buf_size);
    	    ret_code = _XimRead(im, &len, preply, buf_size,
    					_XimResetICCheck, (XPointer)ic);
    	    if (ret_code != XIM_TRUE) {
		Xfree(preply);
    		ic->private.proto.waitCallback = False;
    		return NULL;
    	    }
    	}
    } else {
	ic->private.proto.waitCallback = False;
	return NULL;
    }
    ic->private.proto.waitCallback = False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
    	if (reply != preply)
    	    free(preply);
	return NULL;
    }

    commit = retfunc(im, ic, (XPointer)&buf_s[2]);

    if (reply != preply)
    	Xfree(preply);
    return commit;
}

static char *
_XimCommitedMbString(
    Xim			 im,
    Xic			 ic,
    XPointer		 buf)
{
    CARD16		*buf_s = (CARD16 *)buf;
    XimCommitInfo	 info;
    int			 len;
    int			 new_len;
    char		*commit;
    char		*new_commit = NULL;
    char		*str;
    Status		 status;

    len = 0;
    for (info = ic->private.proto.commit_info; info; info = info->next)
	len += info->string_len;
    len += buf_s[0];
    if ( len == 0 )
	return( NULL );

    if (!(commit = Xmalloc(len + 1)))
	goto Error_On_Reset;

    str = commit;
    for (info = ic->private.proto.commit_info; info; info = info->next) {
	(void)memcpy(str, info->string, info->string_len);
	str += info->string_len;
    }
    (void)memcpy(str, (char *)&buf_s[1], buf_s[0]);
    commit[len] = '\0';

    new_len = im->methods->ctstombs((XIM)im, commit, len, NULL, 0, &status);
    if (status != XLookupNone) {
	if (!(new_commit = Xmalloc(new_len + 1))) {
	    Xfree(commit);
	    goto Error_On_Reset;
	}
	(void)im->methods->ctstombs((XIM)im, commit, len,
						new_commit, new_len, NULL);
	new_commit[new_len] = '\0';
    }
    Xfree(commit);

Error_On_Reset:
    _XimFreeCommitInfo( ic );
    return new_commit;
}

static char *
_XimProtoMbReset(
    XIC		 xic)
{
    return _XimProtoReset(xic, _XimCommitedMbString);
}

static wchar_t *
_XimCommitedWcString(
    Xim		 im,
    Xic		 ic,
    XPointer	 buf)
{
    CARD16		*buf_s = (CARD16 *)buf;
    XimCommitInfo	 info;
    int			 len;
    int			 new_len;
    char		*commit;
    wchar_t		*new_commit = (wchar_t *)NULL;
    char		*str;
    Status		 status;

    len = 0;
    for (info = ic->private.proto.commit_info; info; info = info->next)
	len += info->string_len;
    len += buf_s[0];
    if ( len == 0 )
	return( (wchar_t *)NULL );

    if (!(commit = Xmalloc(len + 1)))
	goto Error_On_Reset;

    str = commit;
    for (info = ic->private.proto.commit_info; info; info = info->next) {
	(void)memcpy(str, info->string, info->string_len);
	str += info->string_len;
    }
    (void)memcpy(str, (char *)&buf_s[1], buf_s[0]);
    commit[len] = '\0';

    new_len = im->methods->ctstowcs((XIM)im, commit, len, NULL, 0, &status);
    if (status != XLookupNone) {
	if (!(new_commit =
		     (wchar_t *)Xmalloc(sizeof(wchar_t) * (new_len + 1)))) {
	    Xfree(commit);
	    goto Error_On_Reset;
	}
	(void)im->methods->ctstowcs((XIM)im, commit, len,
						new_commit, new_len, NULL);
	new_commit[new_len] = (wchar_t)'\0';
    }
    Xfree(commit);

Error_On_Reset:
    _XimFreeCommitInfo( ic );
    return new_commit;
}

static wchar_t *
_XimProtoWcReset(
    XIC		 xic)
{
    return (wchar_t *) _XimProtoReset(xic,
			(char * (*) (Xim, Xic, XPointer)) _XimCommitedWcString);
}

static char *
_XimCommitedUtf8String(
    Xim			 im,
    Xic			 ic,
    XPointer		 buf)
{
    CARD16		*buf_s = (CARD16 *)buf;
    XimCommitInfo	 info;
    int			 len;
    int			 new_len;
    char		*commit;
    char		*new_commit = NULL;
    char		*str;
    Status		 status;

    len = 0;
    for (info = ic->private.proto.commit_info; info; info = info->next)
	len += info->string_len;
    len += buf_s[0];
    if ( len == 0 )
	return( NULL );

    if (!(commit = Xmalloc(len + 1)))
	goto Error_On_Reset;

    str = commit;
    for (info = ic->private.proto.commit_info; info; info = info->next) {
	(void)memcpy(str, info->string, info->string_len);
	str += info->string_len;
    }
    (void)memcpy(str, (char *)&buf_s[1], buf_s[0]);
    commit[len] = '\0';

    new_len = im->methods->ctstoutf8((XIM)im, commit, len, NULL, 0, &status);
    if (status != XLookupNone) {
	if (!(new_commit = Xmalloc(new_len + 1))) {
	    Xfree(commit);
	    goto Error_On_Reset;
	}
	(void)im->methods->ctstoutf8((XIM)im, commit, len,
						new_commit, new_len, NULL);
	new_commit[new_len] = '\0';
    }
    Xfree(commit);

Error_On_Reset:
    _XimFreeCommitInfo( ic );
    return new_commit;
}

static char *
_XimProtoUtf8Reset(
    XIC		 xic)
{
    return _XimProtoReset(xic, _XimCommitedUtf8String);
}

static XICMethodsRec ic_methods = {
    _XimProtoDestroyIC,		/* destroy */
    _XimProtoSetFocus,		/* set_focus */
    _XimProtoUnsetFocus,	/* unset_focus */
    _XimProtoSetICValues,	/* set_values */
    _XimProtoGetICValues,	/* get_values */
    _XimProtoMbReset,		/* mb_reset */
    _XimProtoWcReset,		/* wc_reset */
    _XimProtoUtf8Reset,		/* utf8_reset */
    _XimProtoMbLookupString,	/* mb_lookup_string */
    _XimProtoWcLookupString,	/* wc_lookup_string */
    _XimProtoUtf8LookupString	/* utf8_lookup_string */
};

static Bool
_XimGetInputStyle(
    XIMArg		*arg,
    XIMStyle		*input_style)
{
    register XIMArg	*p;

    for (p = arg; p && p->name; p++) {
	if (!(strcmp(p->name, XNInputStyle))) {
	    *input_style = (XIMStyle)p->value;
	    return True;
	}
    }
    return False;
}

#ifdef XIM_CONNECTABLE
static Bool
_XimDelayModeCreateIC(
    Xic			 ic,
    XIMArg		*values,
    XIMResourceList	 res,
    unsigned int	 num)
{
    Xim			 im = (Xim)ic->core.im;
    XimDefICValues	 ic_values;
    int			 len;
    XIMStyle		 input_style;

    bzero((char *)&ic_values, sizeof(XimDefICValues));
    _XimGetCurrentICValues(ic, &ic_values);
    if (!(_XimGetInputStyle(values, &input_style)))
	return False;

    _XimSetICMode(res, num, input_style);

    if (_XimSetICValueData(ic, (XPointer)&ic_values, res, num,
					values, XIM_CREATEIC, False)) {
	return False;
    }
    _XimSetCurrentICValues(ic, &ic_values);
    if (!_XimSetICDefaults(ic, (XPointer)&ic_values,
					XIM_SETICDEFAULTS, res, num)) {
	return False;
    }
    ic_values.filter_events = KeyPressMask;
    _XimSetCurrentICValues(ic, &ic_values);
    _XimRegisterFilter(ic);

    return True;
}

Bool
_XimReconnectModeCreateIC(ic)
    Xic			 ic;
{
    Xim			 im = (Xim)ic->core.im;
    int			 len;
    XIMStyle		 input_style = ic->core.input_style;
    XIMResourceList	 res;
    unsigned int	 num;

    num = im->core.ic_num_resources;
    len = sizeof(XIMResource) * num;
    if (!(res = Xmalloc(len)))
	return False;
    (void)memcpy((char *)res, (char *)im->core.ic_resources, len);
    ic->private.proto.ic_resources     = res;
    ic->private.proto.ic_num_resources = num;

    _XimSetICMode(res, num, input_style);

    ic->core.filter_events = KeyPressMask;

    return True;
}
#endif /* XIM_CONNECTABLE */

XIC
_XimProtoCreateIC(
    XIM			 xim,
    XIMArg		*arg)
{
    Xim			 im = (Xim)xim;
    Xic			 ic;
    XimDefICValues	 ic_values;
    XIMResourceList	 res;
    unsigned int         num;
    XIMStyle		 input_style;
    INT16		 len;
    CARD16		*buf_s;
    char		*tmp;
    CARD32		 tmp_buf32[BUFSIZE/4];
    char		*tmp_buf = (char *)tmp_buf32;
    char		*buf;
    int			 buf_size;
    char		*data;
    int			 data_len;
    int			 ret_len;
    int			 total;
    XIMArg		*arg_ret;
    CARD32		 reply32[BUFSIZE/4];
    char		*reply = (char *)reply32;
    XPointer		 preply;
    int			 ret_code;

#ifdef XIM_CONNECTABLE
    if (!IS_SERVER_CONNECTED(im) && !IS_CONNECTABLE(im))
	return (XIC)NULL;
#else
    if (!IS_SERVER_CONNECTED(im))
	return (XIC)NULL;
#endif /* XIM_CONNECTABLE */

    if (!(_XimGetInputStyle(arg, &input_style)))
	return (XIC)NULL;

    if ((ic = Xcalloc(1, sizeof(XicRec))) == (Xic)NULL)
	return (XIC)NULL;

    ic->methods = &ic_methods;
    ic->core.im = (XIM)im;
    ic->core.input_style = input_style;

    num = im->core.ic_num_resources;
    len = sizeof(XIMResource) * num;
    if (!(res = Xmalloc(len)))
	goto ErrorOnCreatingIC;
    (void)memcpy((char *)res, (char *)im->core.ic_resources, len);
    ic->private.proto.ic_resources     = res;
    ic->private.proto.ic_num_resources = num;

#ifdef XIM_CONNECTABLE
    if (!_XimSaveICValues(ic, arg))
	return False;

    if (!IS_SERVER_CONNECTED(im)) {
	if (!_XimConnectServer(im)) {
	    if (_XimDelayModeCreateIC(ic, arg, res, num)) {
		return (XIC)ic;
	    }
	    goto ErrorOnCreatingIC;
	}
    }
#endif /* XIM_CONNECTABLE */

    ic->core.filter_events = im->private.proto.forward_event_mask;
    ic->private.proto.forward_event_mask =
				im->private.proto.forward_event_mask;
    ic->private.proto.synchronous_event_mask =
				im->private.proto.synchronous_event_mask;

    num = im->private.proto.ic_num_inner_resources;
    len = sizeof(XIMResource) * num;
    if (!(res = Xmalloc(len)))
	goto ErrorOnCreatingIC;
    (void)memcpy((char *)res,
			 (char *)im->private.proto.ic_inner_resources, len);
    ic->private.proto.ic_inner_resources     = res;
    ic->private.proto.ic_num_inner_resources = num;

    _XimSetICMode(ic->private.proto.ic_resources,
			ic->private.proto.ic_num_resources, input_style);

    _XimSetICMode(ic->private.proto.ic_inner_resources,
			ic->private.proto.ic_num_inner_resources, input_style);

    _XimGetCurrentICValues(ic, &ic_values);
    buf = tmp_buf;
    buf_size = XIM_HEADER_SIZE + sizeof(CARD16) + sizeof(INT16);
    data_len = BUFSIZE - buf_size;
    total = 0;
    arg_ret = arg;
    for (;;) {
	data = &buf[buf_size];
	if (_XimEncodeICATTRIBUTE(ic, ic->private.proto.ic_resources,
		ic->private.proto.ic_num_resources, arg, &arg_ret, data,
		data_len, &ret_len, (XPointer)&ic_values, 0, XIM_CREATEIC)) {
	    goto ErrorOnCreatingIC;
	}

	total += ret_len;
	if (!(arg = arg_ret)) {
	    break;
	}

	buf_size += ret_len;
	if (buf == tmp_buf) {
	    if (!(tmp = Xmalloc(buf_size + data_len))) {
	        goto ErrorOnCreatingIC;
	    }
	    memcpy(tmp, buf, buf_size);
	    buf = tmp;
	} else {
	    if (!(tmp = Xrealloc(buf, (buf_size + data_len)))) {
		Xfree(buf);
	        goto ErrorOnCreatingIC;
	    }
	    buf = tmp;
	}
    }
    _XimSetCurrentICValues(ic, &ic_values);

    if (!(_XimCheckCreateICValues(ic->private.proto.ic_resources,
					ic->private.proto.ic_num_resources)))
	goto ErrorOnCreatingIC;

    _XimRegisterFilter(ic);

    buf_s = (CARD16 *)&buf[XIM_HEADER_SIZE];
    buf_s[0] = im->private.proto.imid;
    buf_s[1] = (INT16)total;

    len = (INT16)(sizeof(CARD16) + sizeof(INT16) + total);
    _XimSetHeader((XPointer)buf, XIM_CREATE_IC, 0, &len);
    if (!(_XimWrite(im, len, (XPointer)buf))) {
	if (buf != tmp_buf)
	    Xfree(buf);
	goto ErrorOnCreatingIC;
    }
    _XimFlush(im);
    if (buf != tmp_buf)
	Xfree(buf);
    ic->private.proto.waitCallback = True;
    buf_size = BUFSIZE;
    ret_code = _XimRead(im, &len, (XPointer)reply, buf_size,
						 _XimCreateICCheck, 0);
    if (ret_code == XIM_TRUE) {
	preply = reply;
    } else if (ret_code == XIM_OVERFLOW) {
	if (len <= 0) {
	    preply = reply;
	} else {
	    buf_size = (int)len;
	    preply = Xmalloc(buf_size);
	    ret_code = _XimRead(im, &len, preply, buf_size,
						 _XimCreateICCheck, 0);
	    if (ret_code != XIM_TRUE) {
		Xfree(preply);
		ic->private.proto.waitCallback = False;
		goto ErrorOnCreatingIC;
	    }
	}
    } else {
	ic->private.proto.waitCallback = False;
	goto ErrorOnCreatingIC;
    }
    ic->private.proto.waitCallback = False;
    buf_s = (CARD16 *)((char *)preply + XIM_HEADER_SIZE);
    if (*((CARD8 *)preply) == XIM_ERROR) {
	_XimProcError(im, 0, (XPointer)&buf_s[3]);
	if (reply != preply)
	    Xfree(preply);
	goto ErrorOnCreatingIC;
    }

    ic->private.proto.icid = buf_s[1];		/* icid */
    if (reply != preply)
	Xfree(preply);
    MARK_IC_CONNECTED(ic);
    return (XIC)ic;

ErrorOnCreatingIC:
    _XimUnregisterFilter(ic);

    Xfree(ic->private.proto.ic_resources);
    Xfree(ic->private.proto.ic_inner_resources);
    Xfree(ic);
    return (XIC)NULL;
}
