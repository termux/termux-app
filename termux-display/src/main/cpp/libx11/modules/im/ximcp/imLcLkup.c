/******************************************************************

              Copyright 1992 by Fuji Xerox Co., Ltd.
              Copyright 1992, 1994  by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Fuji Xerox,
FUJITSU LIMITED not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  Fuji Xerox, FUJITSU LIMITED  make no representations
about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJI XEROX, FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJI XEROX,
FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Kazunori Nishihara   Fuji Xerox
          Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPubI.h"
#include "Ximint.h"

int
_XimLocalMbLookupString(XIC xic, XKeyEvent *ev, char *buffer, int bytes,
			KeySym *keysym, Status *status)
{
    Xic		 ic = (Xic)xic;
    int		 ret;
    DefTree	*b  = ic->private.local.base.tree;
    char	*mb = ic->private.local.base.mb;

    if(ev->type != KeyPress) {
	if(status) *status = XLookupNone;
	return(0);
    }
    if(ev->keycode == 0 &&
	   (  (ic->private.local.composed != 0)
	    ||(ic->private.local.brl_committed != 0))) {
	if (ic->private.local.brl_committed != 0) { /* Braille Event */
	    unsigned char pattern = ic->private.local.brl_committed;
	    char mb2[XLC_PUBLIC(ic->core.im->core.lcd, mb_cur_max)];
	    ret = _Xlcwctomb(ic->core.im->core.lcd, mb2, BRL_UC_ROW | pattern);
	    if(ret > bytes) {
		if(status) *status = XBufferOverflow;
		return(ret);
	    }
	    if(keysym) *keysym = XK_braille_blank | pattern;
	    if(ret > 0) {
		if (keysym) {
		    if(status) *status = XLookupBoth;
		} else {
		    if(status) *status = XLookupChars;
		}
		memcpy(buffer, mb2, ret);
	    } else {
		if(keysym) {
		    if(status) *status = XLookupKeySym;
		} else {
		    if(status) *status = XLookupNone;
		}
	    }
	} else { /* Composed Event */
	    ret = strlen(&mb[b[ic->private.local.composed].mb]);
	    if(ret > bytes) {
		if(status) *status = XBufferOverflow;
		return(ret);
	    }
	    memcpy(buffer, &mb[b[ic->private.local.composed].mb], ret);
	    if(keysym) *keysym = b[ic->private.local.composed].ks;
	    if (ret > 0) {
		if (keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupBoth;
		} else {
		    if(status) *status = XLookupChars;
		}
	    } else {
		if(keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupKeySym;
		} else {
		    if(status) *status = XLookupNone;
		}
	    }
	}
	return (ret);
    } else { /* Throughed Event */
	ret = _XimLookupMBText(ic, ev, buffer, bytes, keysym, NULL);
	if(ret > 0) {
	    if (ret > bytes) {
		if (status) *status = XBufferOverflow;
	    } else if (keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}

int
_XimLocalWcLookupString(XIC xic, XKeyEvent *ev, wchar_t *buffer, int wlen,
			KeySym *keysym, Status *status)
{
    Xic		 ic = (Xic)xic;
    int		 ret;
    DefTree	*b  = ic->private.local.base.tree;
    wchar_t	*wc = ic->private.local.base.wc;

    if(ev->type != KeyPress) {
	if(status) *status = XLookupNone;
	return(0);
    }
    if(ev->keycode == 0) {
	if (ic->private.local.brl_committed != 0) { /* Braille Event */
	    unsigned char pattern = ic->private.local.brl_committed;
	    ret = 1;
	    if (ret > wlen) {
		if(status) *status = XBufferOverflow;
		return (ret);
	    }
	    *buffer = BRL_UC_ROW | pattern;
	    if(keysym) {
		*keysym = XK_braille_blank | pattern;
		if(status) *status = XLookupBoth;
	    } else
		if(status) *status = XLookupChars;
	} else { /* Composed Event */
	    ret = _Xwcslen(&wc[b[ic->private.local.composed].wc]);
	    if(ret > wlen) {
		if(status) *status = XBufferOverflow;
		return (ret);
	    }
	    memcpy((char *)buffer, (char *)&wc[b[ic->private.local.composed].wc],
		   ret * sizeof(wchar_t));
	    if(keysym) *keysym = b[ic->private.local.composed].ks;
	    if (ret > 0) {
		if (keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupBoth;
		} else {
		    if(status) *status = XLookupChars;
		}
	    } else {
		if(keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupKeySym;
		} else {
		    if(status) *status = XLookupNone;
		}
	    }
	}
	return (ret);
    } else { /* Throughed Event */
	ret = _XimLookupWCText(ic, ev, buffer, wlen, keysym, NULL);
	if(ret > 0) {
	    if (ret > wlen) {
		if (status) *status = XBufferOverflow;
	    } else if (keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}

int
_XimLocalUtf8LookupString(XIC xic, XKeyEvent *ev, char *buffer, int bytes,
			  KeySym *keysym, Status *status)
{
    Xic		 ic = (Xic)xic;
    int		 ret;
    DefTree	*b    = ic->private.local.base.tree;
    char	*utf8 = ic->private.local.base.utf8;

    if(ev->type != KeyPress) {
	if(status) *status = XLookupNone;
	return(0);
    }
    if(ev->keycode == 0) {
	if (ic->private.local.brl_committed != 0) { /* Braille Event */
	    unsigned char pattern = ic->private.local.brl_committed;
	    ret = 3;
	    if (ret > bytes) {
		if(status) *status = XBufferOverflow;
		return (ret);
	    }
	    buffer[0] = 0xe0 | ((BRL_UC_ROW >> 12) & 0x0f);
	    buffer[1] = 0x80 | ((BRL_UC_ROW >> 8) & 0x30) | (pattern >> 6);
	    buffer[2] = 0x80 | (pattern & 0x3f);
	    if(keysym) {
		*keysym = XK_braille_blank | pattern;
		if(status) *status = XLookupBoth;
	    } else
		if(status) *status = XLookupChars;
	} else { /* Composed Event */
	    ret = strlen(&utf8[b[ic->private.local.composed].utf8]);
	    if(ret > bytes) {
		if(status) *status = XBufferOverflow;
		return (ret);
	    }
	    memcpy(buffer, &utf8[b[ic->private.local.composed].utf8], ret);
	    if(keysym) *keysym = b[ic->private.local.composed].ks;
	    if (ret > 0) {
		if (keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupBoth;
		} else {
		    if(status) *status = XLookupChars;
		}
	    } else {
		if(keysym && *keysym != NoSymbol) {
		    if(status) *status = XLookupKeySym;
		} else {
		    if(status) *status = XLookupNone;
		}
	    }
	}
	return (ret);
    } else { /* Throughed Event */
	ret = _XimLookupUTF8Text(ic, ev, buffer, bytes, keysym, NULL);
	if(ret > 0) {
	    if (ret > bytes) {
		if (status) *status = XBufferOverflow;
	    } else if (keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}

static int
_XimLcctsconvert(
    XlcConv	 conv,
    char	*from,
    int		 from_len,
    char	*to,
    int		 to_len,
    Status	*state)
{
    int		 from_left;
    int		 to_left;
    int		 from_savelen;
    int		 to_savelen;
    int		 from_cnvlen;
    int		 to_cnvlen;
    char	*from_buf;
    char	*to_buf;
    char	 scratchbuf[BUFSIZ];
    Status	 tmp_state;

    if (!state)
	state = &tmp_state;

    if (!conv || !from || !from_len) {
	*state = XLookupNone;
	return 0;
    }

    /* Reset the converter.  The CompoundText at 'from' starts in
       initial state.  */
    _XlcResetConverter(conv);

    from_left = from_len;
    to_left = BUFSIZ;
    from_cnvlen = 0;
    to_cnvlen = 0;
    for (;;) {
	from_buf = &from[from_cnvlen];
	from_savelen = from_left;
	to_buf = &scratchbuf[to_cnvlen];
	to_savelen = to_left;
	if (_XlcConvert(conv, (XPointer *)&from_buf, &from_left,
				 (XPointer *)&to_buf, &to_left, NULL, 0) < 0) {
	    *state = XLookupNone;
	    return 0;
	}
	from_cnvlen += (from_savelen - from_left);
	to_cnvlen += (to_savelen - to_left);
	if (from_left == 0) {
	    if (!to_cnvlen) {
		*state = XLookupNone;
		return 0;
           }
	   break;
	}
    }

    if (!to || !to_len || (to_len < to_cnvlen)) {
       *state = XBufferOverflow;
    } else {
       memcpy(to, scratchbuf, to_cnvlen);
       *state = XLookupChars;
    }
    return to_cnvlen;
}

int
_XimLcctstombs(XIM xim, char *from, int from_len,
	       char *to, int to_len, Status *state)
{
    return _XimLcctsconvert(((Xim)xim)->private.local.ctom_conv,
			    from, from_len, to, to_len, state);
}

int
_XimLcctstowcs(XIM xim, char *from, int from_len,
	       wchar_t *to, int to_len, Status *state)
{
    Xim		 im = (Xim)xim;
    XlcConv	 conv = im->private.local.ctow_conv;
    int		 from_left;
    int		 to_left;
    int		 from_savelen;
    int		 to_savelen;
    int		 from_cnvlen;
    int		 to_cnvlen;
    char	*from_buf;
    wchar_t	*to_buf;
    wchar_t	 scratchbuf[BUFSIZ];
    Status	 tmp_state;

    if (!state)
	state = &tmp_state;

    if (!conv || !from || !from_len) {
	*state = XLookupNone;
	return 0;
    }

    /* Reset the converter.  The CompoundText at 'from' starts in
       initial state.  */
    _XlcResetConverter(conv);

    from_left = from_len;
    to_left = BUFSIZ;
    from_cnvlen = 0;
    to_cnvlen = 0;
    for (;;) {
	from_buf = &from[from_cnvlen];
       from_savelen = from_left;
       to_buf = &scratchbuf[to_cnvlen];
       to_savelen = to_left;
	if (_XlcConvert(conv, (XPointer *)&from_buf, &from_left,
				 (XPointer *)&to_buf, &to_left, NULL, 0) < 0) {
	    *state = XLookupNone;
	    return 0;
	}
	from_cnvlen += (from_savelen - from_left);
       to_cnvlen += (to_savelen - to_left);
	if (from_left == 0) {
           if (!to_cnvlen){
		*state = XLookupNone;
               return 0;
           }
	    break;
	}
    }

    if (!to || !to_len || (to_len < to_cnvlen)) {
       *state = XBufferOverflow;
    } else {
       memcpy(to, scratchbuf, to_cnvlen * sizeof(wchar_t));
       *state = XLookupChars;
    }
    return to_cnvlen;
}

int
_XimLcctstoutf8(XIM xim, char *from, int from_len,
		char *to, int to_len, Status *state)
{
    return _XimLcctsconvert(((Xim)xim)->private.local.ctoutf8_conv,
			    from, from_len, to, to_len, state);
}
