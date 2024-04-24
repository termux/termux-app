/*

Copyright 1988, 1989, 1998  The Open Group

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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPubI.h"
#include "Ximint.h"
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#define XK_LATIN1
#define XK_PUBLISHING
#include <X11/keysym.h>
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"
#include <X11/Xlocale.h>
#include <ctype.h>
#include <X11/Xos.h>

#ifdef __sgi_not_xconsortium
#define	XKB_EXTEND_LOOKUP_STRING
#endif

static int
_XkbHandleSpecialSym(KeySym keysym, char *buffer, int nbytes, int *extra_rtrn)
{
    /* try to convert to Latin-1, handling ctrl */
    if (!(((keysym >= XK_BackSpace) && (keysym <= XK_Clear)) ||
          (keysym == XK_Return) || (keysym == XK_Escape) ||
          (keysym == XK_KP_Space) || (keysym == XK_KP_Tab) ||
          (keysym == XK_KP_Enter) ||
          ((keysym >= XK_KP_Multiply) && (keysym <= XK_KP_9)) ||
          (keysym == XK_KP_Equal) || (keysym == XK_Delete)))
        return 0;

    if (nbytes < 1) {
        if (extra_rtrn)
            *extra_rtrn = 1;
        return 0;
    }
    /* if X keysym, convert to ascii by grabbing low 7 bits */
    if (keysym == XK_KP_Space)
        buffer[0] = XK_space & 0x7F;            /* patch encoding botch */
    else
        buffer[0] = (char) (keysym & 0x7F);
    return 1;
}

/*ARGSUSED*/
static int
_XkbKSToKnownSet(XPointer priv,
                 KeySym keysym,
                 char *buffer,
                 int nbytes,
                 int *extra_rtrn)
{
    char tbuf[8], *buf;

    if (extra_rtrn)
        *extra_rtrn = 0;

    /* convert "dead" diacriticals for dumb applications */
    if ((keysym & 0xffffff00) == 0xfe00) {
        switch (keysym) {
        case XK_dead_grave:            keysym = XK_grave; break;
        case XK_dead_acute:            keysym = XK_acute; break;
        case XK_dead_circumflex:       keysym = XK_asciicircum; break;
        case XK_dead_tilde:            keysym = XK_asciitilde; break;
        case XK_dead_macron:           keysym = XK_macron; break;
        case XK_dead_breve:            keysym = XK_breve; break;
        case XK_dead_abovedot:         keysym = XK_abovedot; break;
        case XK_dead_diaeresis:        keysym = XK_diaeresis; break;
        case XK_dead_abovering:        keysym = XK_degree; break;
        case XK_dead_doubleacute:      keysym = XK_doubleacute; break;
        case XK_dead_caron:            keysym = XK_caron; break;
        case XK_dead_cedilla:          keysym = XK_cedilla; break;
        case XK_dead_ogonek:           keysym = XK_ogonek; break;
        case XK_dead_iota:             keysym = XK_Greek_iota; break;
#ifdef XK_KATAKANA
        case XK_dead_voiced_sound:     keysym = XK_voicedsound; break;
        case XK_dead_semivoiced_sound: keysym = XK_semivoicedsound; break;
#endif
        }
    }

    if (nbytes < 1)
        buf = tbuf;
    else
        buf = buffer;

    if ((keysym & 0xffffff00) == 0xff00) {
        return _XkbHandleSpecialSym(keysym, buf, nbytes, extra_rtrn);
    }
    return _XimGetCharCode(priv, keysym, (unsigned char *) buf, nbytes);
}

typedef struct _XkbToKS {
    unsigned prefix;
    char *map;
} XkbToKS;

/*ARGSUSED*/
static KeySym
_XkbKnownSetToKS(XPointer priv, char *buffer, int nbytes, Status *status)
{
    if (nbytes != 1)
        return NoSymbol;
    if (((buffer[0] & 0x80) == 0) && (buffer[0] >= 32))
        return buffer[0];
    else if ((buffer[0] & 0x7f) >= 32) {
        XkbToKS *map = (XkbToKS *) priv;

        if (map) {
            if (map->map)
                return map->prefix | map->map[buffer[0] & 0x7f];
            else
                return map->prefix | buffer[0];
        }
        return buffer[0];
    }
    return NoSymbol;
}

static KeySym
__XkbDefaultToUpper(KeySym sym)
{
    KeySym lower, upper;

    XConvertCase(sym, &lower, &upper);
    return upper;
}

#ifdef XKB_EXTEND_LOOKUP_STRING
static int
Strcmp(char *str1, char *str2)
{
    char str[256];
    char c, *s;

    /*
     * unchecked strings from the environment can end up here, so check
     * the length before copying.
     */
    if (strlen(str1) >= sizeof(str))    /* almost certain it's a mismatch */
        return 1;

    for (s = str; (c = *str1++);) {
        if (isupper(c))
            c = tolower(c);
        *s++ = c;
    }
    *s = '\0';
    return (strcmp(str, str2));
}
#endif

int
_XkbGetConverters(const char *encoding_name, XkbConverters * cvt_rtrn)
{
    if (!cvt_rtrn)
        return 0;

    cvt_rtrn->KSToMB = _XkbKSToKnownSet;
    cvt_rtrn->KSToMBPriv = _XimGetLocaleCode(encoding_name);
    cvt_rtrn->MBToKS = _XkbKnownSetToKS;
    cvt_rtrn->MBToKSPriv = NULL;
    cvt_rtrn->KSToUpper = __XkbDefaultToUpper;
    return 1;
}

/***====================================================================***/

/*
 * The function _XkbGetCharset seems to be missnamed as what it seems to
 * be used for is to determine the encoding-name for the locale. ???
 */

#ifdef XKB_EXTEND_LOOKUP_STRING

/*
 * XKB_EXTEND_LOOKUP_STRING is not used by the SI. It is used by various
 * X Consortium/X Project Team members, so we leave it in the source as
 * an simplify integration by these companies.
 */

#define	CHARSET_FILE	"/usr/lib/X11/input/charsets"
static char *_XkbKnownLanguages =
    "c=ascii:da,de,en,es,fr,is,it,nl,no,pt,sv=iso8859-1:hu,pl,cs=iso8859-2:"
    "eo=iso8859-3:sp=iso8859-5:ar,ara=iso8859-6:el=iso8859-7:he=iso8859-8:"
    "tr=iso8859-9:lt,lv=iso8859-13:et,fi=iso8859-15:ru=koi8-r:uk=koi8-u:"
    "th,th_TH,th_TH.iso8859-11=iso8859-11:th_TH.TIS620=tis620:hy=armscii-8:"
    "vi=tcvn-5712:ka=georgian-academy:be,bg=microsoft-cp1251";

char *
_XkbGetCharset(void)
{
    /*
     * PAGE USAGE TUNING: explicitly initialize to move these to data
     * instead of bss
     */
    static char buf[100] = { 0 };
    char lang[256];
    char *start, *tmp, *end, *next, *set;
    char *country, *charset;
    char *locale;

    tmp = getenv("_XKB_CHARSET");
    if (tmp)
        return tmp;
    locale = setlocale(LC_CTYPE, NULL);

    if (locale == NULL)
        return NULL;

    if (strlen(locale) >= sizeof(lang))
        return NULL;

    for (tmp = lang; *tmp = *locale++; tmp++) {
        if (isupper(*tmp))
            *tmp = tolower(*tmp);
    }
    country = strchr(lang, '_');
    if (country) {
        *country++ = '\0';
        charset = strchr(country, '.');
        if (charset)
            *charset++ = '\0';
        if (charset) {
            strncpy(buf, charset, 99);
            buf[99] = '\0';
            return buf;
        }
    }
    else {
        charset = NULL;
    }

    if ((tmp = getenv("_XKB_LOCALE_CHARSETS")) != NULL) {
        start = _XkbAlloc(strlen(tmp) + 1);
        strcpy(start, tmp);
        tmp = start;
    }
    else {
        struct stat sbuf;
        FILE *file;
#ifndef __UNIXOS2__
        char *cf = CHARSET_FILE;
#else
        char *cf = __XOS2RedirRoot(CHARSET_FILE);
#endif

#ifndef S_ISREG
# define S_ISREG(mode)   (((mode) & S_IFMT) == S_IFREG)
#endif

        if ((stat(cf, &sbuf) == 0) && S_ISREG(sbuf.st_mode) &&
            (file = fopen(cf, "r"))) {
            tmp = _XkbAlloc(sbuf.st_size + 1);
            if (tmp != NULL) {
                sbuf.st_size = (long) fread(tmp, 1, sbuf.st_size, file);
                tmp[sbuf.st_size] = '\0';
            }
            fclose(file);
        }
    }

    if (tmp == NULL) {
        tmp = _XkbAlloc(strlen(_XkbKnownLanguages) + 1);
        if (!tmp)
            return NULL;
        strcpy(tmp, _XkbKnownLanguages);
    }
    start = tmp;
    do {
        if ((set = strchr(tmp, '=')) == NULL)
            break;
        *set++ = '\0';
        if ((next = strchr(set, ':')) != NULL)
            *next++ = '\0';
        while (tmp && *tmp) {
            if ((end = strchr(tmp, ',')) != NULL)
                *end++ = '\0';
            if (Strcmp(tmp, lang) == 0) {
                strncpy(buf, set, 100);
                buf[99] = '\0';
                Xfree(start);
                return buf;
            }
            tmp = end;
        }
        tmp = next;
    } while (tmp && *tmp);
    Xfree(start);
    return NULL;
}
#else
char *
_XkbGetCharset(void)
{
    char *tmp;
    XLCd lcd;

    tmp = getenv("_XKB_CHARSET");
    if (tmp)
        return tmp;

    lcd = _XlcCurrentLC();
    if (lcd)
        return XLC_PUBLIC(lcd, encoding_name);

    return NULL;
}
#endif
