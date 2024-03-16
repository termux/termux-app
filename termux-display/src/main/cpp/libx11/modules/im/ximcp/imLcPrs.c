/******************************************************************

              Copyright 1992 by Oki Technosystems Laboratory, Inc.
              Copyright 1992 by Fuji Xerox Co., Ltd.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Oki Technosystems
Laboratory and Fuji Xerox not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.
Oki Technosystems Laboratory and Fuji Xerox make no representations
about the suitability of this software for any purpose.  It is provided
"as is" without express or implied warranty.

OKI TECHNOSYSTEMS LABORATORY AND FUJI XEROX DISCLAIM ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL OKI TECHNOSYSTEMS
LABORATORY AND FUJI XEROX BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

  Author: Yasuhiro Kawai	Oki Technosystems Laboratory
  Author: Kazunori Nishihara	Fuji Xerox

******************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>
#include "pathmax.h"

#define XLC_BUFSIZE 256

extern int _Xmbstowcs(
    wchar_t	*wstr,
    char	*str,
    int		len
);

extern int _Xmbstoutf8(
    char	*ustr,
    const char	*str,
    int		len
);

static void parsestringfile(FILE *fp, Xim im, int depth);

/*
 *	Parsing File Format:
 *
 *	FILE          ::= { [PRODUCTION] [COMMENT] "\n"}
 *	PRODUCTION    ::= LHS ":" RHS [ COMMENT ]
 *	COMMENT       ::= "#" {<any character except null or newline>}
 *	LHS           ::= EVENT { EVENT }
 *	EVENT         ::= [MODIFIER_LIST] "<" keysym ">"
 *	MODIFIER_LIST ::= (["!"] {MODIFIER} ) | "None"
 *	MODIFIER      ::= ["~"] MODIFIER_NAME
 *	MODIFIER_NAME ::= ("Ctrl"|"Lock"|"Caps"|"Shift"|"Alt"|"Meta")
 *	RHS           ::= ( STRING | keysym | STRING keysym )
 *	STRING        ::= '"' { CHAR } '"'
 *	CHAR          ::= GRAPHIC_CHAR | ESCAPED_CHAR
 *	GRAPHIC_CHAR  ::= locale (codeset) dependent code
 *	ESCAPED_CHAR  ::= ('\\' | '\"' | OCTAL | HEX )
 *	OCTAL         ::= '\' OCTAL_CHAR [OCTAL_CHAR [OCTAL_CHAR]]
 *	OCTAL_CHAR    ::= (0|1|2|3|4|5|6|7)
 *	HEX           ::= '\' (x|X) HEX_CHAR [HEX_CHAR]]
 *	HEX_CHAR      ::= (0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|a|b|c|d|e|f)
 *
 */

static int
nextch(
    FILE *fp,
    int *lastch)
{
    int c;

    if (*lastch != 0) {
	c = *lastch;
	*lastch = 0;
    } else {
	c = getc(fp);
	if (c == '\\') {
	    c = getc(fp);
	    if (c == '\n') {
		c = getc(fp);
	    } else {
		ungetc(c, fp);
		c = '\\';
	    }
	}
    }
    return(c);
}

static void
putbackch(
    int c,
    int *lastch)
{
    *lastch = c;
}

#define ENDOFFILE 0
#define ENDOFLINE 1
#define COLON 2
#define LESS 3
#define GREATER 4
#define EXCLAM 5
#define TILDE 6
#define STRING 7
#define KEY 8
#define ERROR 9

#ifndef isalnum
#define isalnum(c)      \
    (('0' <= (c) && (c) <= '9')  || \
     ('A' <= (c) && (c) <= 'Z')  || \
     ('a' <= (c) && (c) <= 'z'))
#endif

static int
nexttoken(
    FILE *fp,
    char *tokenbuf,
    int *lastch)
{
    int c;
    int token;
    char *p;
    int i, j;

    while ((c = nextch(fp, lastch)) == ' ' || c == '\t') {
    }
    switch (c) {
      case EOF:
	token = ENDOFFILE;
	break;
      case '\n':
	token = ENDOFLINE;
	break;
      case '<':
	token = LESS;
	break;
      case '>':
	token = GREATER;
	break;
      case ':':
	token = COLON;
	break;
      case '!':
	token = EXCLAM;
	break;
      case '~':
	token = TILDE;
	break;
      case '"':
	p = tokenbuf;
	while ((c = nextch(fp, lastch)) != '"') {
	    if (c == '\n' || c == EOF) {
		putbackch(c, lastch);
		token = ERROR;
		goto string_error;
	    } else if (c == '\\') {
		c = nextch(fp, lastch);
		switch (c) {
		  case '\\':
		  case '"':
		    *p++ = c;
		    break;
		  case 'n':
		    *p++ = '\n';
		    break;
		  case 'r':
		    *p++ = '\r';
		    break;
		  case 't':
		    *p++ = '\t';
		    break;
		  case '0':
		  case '1':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		    i = c - '0';
		    c = nextch(fp, lastch);
		    for (j = 0; j < 2 && c >= '0' && c <= '7'; j++) {
			i <<= 3;
			i += c - '0';
			c = nextch(fp, lastch);
		    }
		    putbackch(c, lastch);
		    *p++ = (char)i;
		    break;
		  case 'X':
		  case 'x':
		    i = 0;
		    for (j = 0; j < 2; j++) {
			c = nextch(fp, lastch);
 			i <<= 4;
			if (c >= '0' && c <= '9') {
			    i += c - '0';
			} else if (c >= 'A' && c <= 'F') {
			    i += c - 'A' + 10;
			} else if (c >= 'a' && c <= 'f') {
			    i += c - 'a' + 10;
			} else {
			    putbackch(c, lastch);
			    i >>= 4;
			    break;
		        }
		    }
		    if (j == 0) {
		        token = ERROR;
		        goto string_error;
		    }
		    *p++ = (char)i;
		    break;
		  case EOF:
		    putbackch(c, lastch);
		    token = ERROR;
		    goto string_error;
		  default:
		    *p++ = c;
		    break;
		}
	    } else {
		*p++ = c;
	    }
	}
	*p = '\0';
	token = STRING;
	break;
      case '#':
	while ((c = nextch(fp, lastch)) != '\n' && c != EOF) {
	}
	if (c == '\n') {
	    token = ENDOFLINE;
	} else {
	    token = ENDOFFILE;
	}
	break;
      default:
	if (isalnum(c) || c == '_' || c == '-') {
	    p = tokenbuf;
	    *p++ = c;
	    c = nextch(fp, lastch);
	    while (isalnum(c) || c == '_' || c == '-') {
		*p++ = c;
		c = nextch(fp, lastch);
	    }
	    *p = '\0';
	    putbackch(c, lastch);
	    token = KEY;
	} else {
	    token = ERROR;
	}
	break;
    }
string_error:
    return(token);
}

static long
modmask(
    char *name)
{
    struct _modtbl {
	const char name[6];
	long mask;
    };

    static const struct _modtbl tbl[] = {
	{ "Ctrl",	ControlMask	},
        { "Lock",	LockMask	},
        { "Caps",	LockMask	},
        { "Shift",	ShiftMask	},
        { "Alt",	Mod1Mask	},
        { "Meta",	Mod1Mask	}};

    int i, num_entries = sizeof (tbl) / sizeof (tbl[0]);

    for (i = 0; i < num_entries; i++)
        if (!strcmp (name, tbl[i].name))
            return tbl[i].mask;

    return 0;
}

static char*
TransFileName(Xim im, char *name)
{
   char *home = NULL, *lcCompose = NULL;
   char dir[XLC_BUFSIZE] = "";
   char *i = name, *ret = NULL, *j;
   size_t l = 0;

   while (*i) {
      if (*i == '%') {
   	  i++;
   	  switch (*i) {
   	      case '%':
                 l++;
   	         break;
   	      case 'H':
                 if (home == NULL)
                     home = getenv("HOME");
                 if (home) {
                     size_t Hsize = strlen(home);
                     if (Hsize > PATH_MAX)
                         /* your home directory length is ridiculous */
                         goto end;
                     l += Hsize;
                 }
   	         break;
   	      case 'L':
                 if (lcCompose == NULL)
                     lcCompose = _XlcFileName(im->core.lcd, COMPOSE_FILE);
                 if (lcCompose) {
                     size_t Lsize = strlen(lcCompose);
                     if (Lsize > PATH_MAX)
                         /* your compose pathname length is ridiculous */
                         goto end;
                     l += Lsize;
                 }
   	         break;
   	      case 'S':
                 if (dir[0] == '\0')
                     xlocaledir(dir, XLC_BUFSIZE);
                 if (dir[0]) {
                     size_t Ssize = strlen(dir);
                     if (Ssize > PATH_MAX)
                         /* your locale directory path length is ridiculous */
                         goto end;
                     l += Ssize;
                 }
   	         break;
   	  }
      } else {
      	  l++;
      }
      i++;
      if (l > PATH_MAX)
          /* your expanded path length is ridiculous */
          goto end;
   }

   j = ret = Xmalloc(l+1);
   if (ret == NULL)
      goto end;
   i = name;
   while (*i) {
      if (*i == '%') {
   	  i++;
   	  switch (*i) {
   	      case '%':
                 *j++ = '%';
   	         break;
   	      case 'H':
   	         if (home) {
   	             strcpy(j, home);
   	             j += strlen(home);
   	         }
   	         break;
   	      case 'L':
   	         if (lcCompose) {
                    strcpy(j, lcCompose);
                    j += strlen(lcCompose);
                 }
   	         break;
   	      case 'S':
                 strcpy(j, dir);
                 j += strlen(dir);
   	         break;
   	  }
          i++;
      } else {
      	  *j++ = *i++;
      }
   }
   *j = '\0';
end:
   Xfree(lcCompose);
   return ret;
}

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 6
#endif

static int
get_mb_string (Xim im, char *buf, KeySym ks)
{
    XPointer from, to;
    int from_len, to_len, len;
    XPointer args[1];
    XlcCharSet charset;
    char local_buf[MB_LEN_MAX];
    unsigned int ucs;
    ucs = KeySymToUcs4(ks);

    from = (XPointer) &ucs;
    to =   (XPointer) local_buf;
    from_len = 1;
    to_len = MB_LEN_MAX;
    args[0] = (XPointer) &charset;
    if (_XlcConvert(im->private.local.ucstoc_conv,
                    &from, &from_len, &to, &to_len, args, 1 ) != 0) {
         return 0;
    }

    from = (XPointer) local_buf;
    to =   (XPointer) buf;
    from_len = MB_LEN_MAX - to_len;
    to_len = MB_LEN_MAX + 1;
    args[0] = (XPointer) charset;
    if (_XlcConvert(im->private.local.cstomb_conv,
                    &from, &from_len, &to, &to_len, args, 1 ) != 0) {
         return 0;
    }
    len = MB_LEN_MAX + 1 - to_len;
    buf[len] = '\0';
    return len;
}

#define AllMask (ShiftMask | LockMask | ControlMask | Mod1Mask)
#define LOCAL_WC_BUFSIZE 128
#define LOCAL_UTF8_BUFSIZE 256
#define SEQUENCE_MAX	10

static int
parseline(
    FILE *fp,
    Xim   im,
    char* tokenbuf,
    int   depth)
{
    int token;
    DTModifier modifier_mask;
    DTModifier modifier;
    DTModifier tmp;
    KeySym keysym = NoSymbol;
    DTIndex *top = &im->private.local.top;
    DefTreeBase *b   = &im->private.local.base;
    DTIndex t;
    DefTree *p = NULL;
    Bool exclam, tilde;
    KeySym rhs_keysym = 0;
    char *rhs_string_mb;
    int l;
    int lastch = 0;
    char local_mb_buf[MB_LEN_MAX+1];
    wchar_t local_wc_buf[LOCAL_WC_BUFSIZE], *rhs_string_wc;
    char local_utf8_buf[LOCAL_UTF8_BUFSIZE], *rhs_string_utf8;

    struct DefBuffer {
	DTModifier modifier_mask;
	DTModifier modifier;
	KeySym keysym;
    };

    struct DefBuffer buf[SEQUENCE_MAX];
    int i, n;

    do {
	token = nexttoken(fp, tokenbuf, &lastch);
    } while (token == ENDOFLINE);

    if (token == ENDOFFILE) {
	return(-1);
    }

    n = 0;
    do {
    	if ((token == KEY) && (strcmp("include", tokenbuf) == 0)) {
            char *filename;
            FILE *infp;
            token = nexttoken(fp, tokenbuf, &lastch);
            if (token != KEY && token != STRING)
                goto error;
            if (++depth > 100)
                goto error;
            if ((filename = TransFileName(im, tokenbuf)) == NULL)
                goto error;
            infp = _XFopenFile(filename, "r");
            Xfree(filename);
            if (infp == NULL)
                goto error;
            parsestringfile(infp, im, depth);
            fclose(infp);
            return (0);
	} else if ((token == KEY) && (strcmp("None", tokenbuf) == 0)) {
	    modifier = 0;
	    modifier_mask = AllMask;
	    token = nexttoken(fp, tokenbuf, &lastch);
	} else {
	    modifier_mask = modifier = 0;
	    exclam = False;
	    if (token == EXCLAM) {
		exclam = True;
		token = nexttoken(fp, tokenbuf, &lastch);
	    }
	    while (token == TILDE || token == KEY) {
		tilde = False;
		if (token == TILDE) {
		    tilde = True;
		    token = nexttoken(fp, tokenbuf, &lastch);
		    if (token != KEY)
			goto error;
		}
		tmp = modmask(tokenbuf);
		if (!tmp) {
		    goto error;
		}
		modifier_mask |= tmp;
		if (tilde) {
		    modifier &= ~tmp;
		} else {
		    modifier |= tmp;
		}
		token = nexttoken(fp, tokenbuf, &lastch);
	    }
	    if (exclam) {
		modifier_mask = AllMask;
	    }
	}

	if (token != LESS) {
	    goto error;
	}

	token = nexttoken(fp, tokenbuf, &lastch);
	if (token != KEY) {
	    goto error;
	}

	token = nexttoken(fp, tokenbuf, &lastch);
	if (token != GREATER) {
	    goto error;
	}

	keysym = XStringToKeysym(tokenbuf);
	if (keysym == NoSymbol) {
	    goto error;
	}

	buf[n].keysym = keysym;
	buf[n].modifier = modifier;
	buf[n].modifier_mask = modifier_mask;
	n++;
	if( n >= SEQUENCE_MAX )
	    goto error;
	token = nexttoken(fp, tokenbuf, &lastch);
    } while (token != COLON);

    token = nexttoken(fp, tokenbuf, &lastch);
    if (token == STRING) {
	l = strlen(tokenbuf) + 1;
	while (b->mbused + l > b->mbsize) {
	    DTCharIndex newsize = b->mbsize ? b->mbsize * 1.5 : 1024;
	    char *newmb = Xrealloc (b->mb, newsize);
	    if (newmb == NULL)
		goto error;
	    b->mb = newmb;
	    b->mbsize = newsize;
	}
	rhs_string_mb = &b->mb[b->mbused];
	b->mbused    += l;
	strcpy(rhs_string_mb, tokenbuf);
	token = nexttoken(fp, tokenbuf, &lastch);
	if (token == KEY) {
	    rhs_keysym = XStringToKeysym(tokenbuf);
	    if (rhs_keysym == NoSymbol) {
		goto error;
	    }
	    token = nexttoken(fp, tokenbuf, &lastch);
	}
	if (token != ENDOFLINE && token != ENDOFFILE) {
	    goto error;
	}
    } else if (token == KEY) {
	rhs_keysym = XStringToKeysym(tokenbuf);
	if (rhs_keysym == NoSymbol) {
	    goto error;
	}
	token = nexttoken(fp, tokenbuf, &lastch);
	if (token != ENDOFLINE && token != ENDOFFILE) {
	    goto error;
	}

        l = get_mb_string(im, local_mb_buf, rhs_keysym);
	while (b->mbused + l + 1 > b->mbsize) {
	    DTCharIndex newsize = b->mbsize ? b->mbsize * 1.5 : 1024;
	    char *newmb = Xrealloc (b->mb, newsize);
	    if (newmb == NULL)
		goto error;
	    b->mb = newmb;
	    b->mbsize = newsize;
	}
	rhs_string_mb = &b->mb[b->mbused];
	b->mbused    += l + 1;
        memcpy(rhs_string_mb, local_mb_buf, l);
	rhs_string_mb[l] = '\0';
    } else {
	goto error;
    }

    l = _Xmbstowcs(local_wc_buf, rhs_string_mb, LOCAL_WC_BUFSIZE - 1);
    if (l == LOCAL_WC_BUFSIZE - 1) {
	local_wc_buf[l] = (wchar_t)'\0';
    }
    while (b->wcused + l + 1 > b->wcsize) {
	DTCharIndex newsize = b->wcsize ? b->wcsize * 1.5 : 512;
	wchar_t *newwc = Xrealloc (b->wc, sizeof(wchar_t) * newsize);
	if (newwc == NULL)
	    goto error;
	b->wc = newwc;
	b->wcsize = newsize;
    }
    rhs_string_wc = &b->wc[b->wcused];
    b->wcused    += l + 1;
    memcpy((char *)rhs_string_wc, (char *)local_wc_buf, (l + 1) * sizeof(wchar_t) );

    l = _Xmbstoutf8(local_utf8_buf, rhs_string_mb, LOCAL_UTF8_BUFSIZE - 1);
    if (l == LOCAL_UTF8_BUFSIZE - 1) {
	local_utf8_buf[l] = '\0';
    }
    while (b->utf8used + l + 1 > b->utf8size) {
	DTCharIndex newsize = b->utf8size ? b->utf8size * 1.5 : 1024;
	char *newutf8 = Xrealloc (b->utf8, newsize);
	if (newutf8 == NULL)
	    goto error;
	b->utf8 = newutf8;
	b->utf8size = newsize;
    }
    rhs_string_utf8 = &b->utf8[b->utf8used];
    b->utf8used    += l + 1;
    memcpy(rhs_string_utf8, local_utf8_buf, l + 1);

    for (i = 0; i < n; i++) {
	for (t = *top; t; t = b->tree[t].next) {
	    if (buf[i].keysym        == b->tree[t].keysym &&
		buf[i].modifier      == b->tree[t].modifier &&
		buf[i].modifier_mask == b->tree[t].modifier_mask) {
		break;
	    }
	}
	if (t) {
	    p = &b->tree[t];
	    top = &p->succession;
	} else {
	    while (b->treeused >= b->treesize) {
		DefTree *old     = b->tree;
		int      oldsize = b->treesize;
		int      newsize = b->treesize ? b->treesize * 1.5 : 256;
		DefTree *new     = Xrealloc (b->tree, sizeof(DefTree) * newsize);
		if (new == NULL)
		    goto error;
		b->tree = new;
		b->treesize = newsize;
		/* Re-derive top after realloc() to avoid undefined behaviour
		   (and crashes on architectures that track pointer bounds). */
		if (old && top >= (DTIndex *) old && top < (DTIndex *) &old[oldsize])
		    top = (DTIndex *) (((char *)new) + (((char *)top)-(char *)old));
	    }
	    p = &b->tree[b->treeused];
	    p->keysym        = buf[i].keysym;
	    p->modifier      = buf[i].modifier;
	    p->modifier_mask = buf[i].modifier_mask;
	    p->succession    = 0;
	    p->next          = *top;
	    p->mb            = 0;
	    p->wc            = 0;
	    p->utf8          = 0;
	    p->ks            = NoSymbol;
	    *top = b->treeused;
	    top = &p->succession;
	    b->treeused++;
	}
    }

    /* old entries no longer freed... */
    p->mb   = rhs_string_mb   - b->mb;
    p->wc   = rhs_string_wc   - b->wc;
    p->utf8 = rhs_string_utf8 - b->utf8;
    p->ks   = rhs_keysym;
    return(n);
error:
    while (token != ENDOFLINE && token != ENDOFFILE) {
	token = nexttoken(fp, tokenbuf, &lastch);
    }
    return(0);
}

void
_XimParseStringFile(
    FILE *fp,
    Xim   im)
{
    parsestringfile(fp, im, 0);
}

static void
parsestringfile(
    FILE *fp,
    Xim   im,
    int   depth)
{
    char tb[8192];
    char* tbp;
    struct stat st;

    if (fstat (fileno (fp), &st) != -1) {
	unsigned long size = (unsigned long) st.st_size;
	if (st.st_size >= INT_MAX)
	    return;
	if (size <= sizeof tb) tbp = tb;
	else tbp = malloc (size);

	if (tbp != NULL) {
	    while (parseline(fp, im, tbp, depth) >= 0) {}
	    if (tbp != tb) free (tbp);
	}
    }
}
