/*

Copyright 1985, 1987, 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <limits.h>
#include "Xlibint.h"
#include <X11/Xresource.h>
#include <X11/keysymdef.h>
#include "Xresinternal.h"

#define NEEDKTABLE
#include "ks_tables.h"
#include "Key.h"

#ifndef KEYSYMDB
#ifndef XKEYSYMDB
#define KEYSYMDB "/usr/lib/X11/XKeysymDB"
#else
#define KEYSYMDB XKEYSYMDB
#endif
#endif

static Bool initialized;
static XrmDatabase keysymdb;
static XrmQuark Qkeysym[2];

XrmDatabase
_XInitKeysymDB(void)
{
    if (!initialized)
    {
	const char *dbname;

	XrmInitialize();
	/* use and name of this env var is not part of the standard */
	/* implementation-dependent feature */
	dbname = getenv("XKEYSYMDB");
	if (!dbname)
	    dbname = KEYSYMDB;
	keysymdb = XrmGetFileDatabase(dbname);
	if (keysymdb)
	    Qkeysym[0] = XrmStringToQuark("Keysym");
	initialized = True;
    }
    return keysymdb;
}

KeySym
XStringToKeysym(_Xconst char *s)
{
    register int i, n;
    int h;
    register Signature sig = 0;
    register const char *p = s;
    register int c;
    register int idx;
    const unsigned char *entry;
    unsigned char sig1, sig2;
    KeySym val;

    while ((c = *p++))
	sig = (sig << 1) + c;
    i = sig % KTABLESIZE;
    h = i + 1;
    sig1 = (sig >> 8) & 0xff;
    sig2 = sig & 0xff;
    n = KMAXHASH;
    while ((idx = hashString[i]))
    {
	entry = &_XkeyTable[idx];
	if ((entry[0] == sig1) && (entry[1] == sig2) &&
	    !strcmp(s, (const char *)entry + 6))
	{
	    val = (entry[2] << 24) | (entry[3] << 16) |
	          (entry[4] << 8)  | entry[5];
	    if (!val)
		val = XK_VoidSymbol;
	    return val;
	}
	if (!--n)
	    break;
	i += h;
	if (i >= KTABLESIZE)
	    i -= KTABLESIZE;
    }

    if (!initialized)
	(void)_XInitKeysymDB();
    if (keysymdb)
    {
	XrmValue result;
	XrmRepresentation from_type;
	char d;
	XrmQuark names[2];

	names[0] = _XrmInternalStringToQuark(s, p - s - 1, sig, False);
	names[1] = NULLQUARK;
	(void)XrmQGetResource(keysymdb, names, Qkeysym, &from_type, &result);
	if (result.addr && (result.size > 1))
	{
	    val = 0;
	    for (i = 0; i < result.size - 1; i++)
	    {
		d = ((char *)result.addr)[i];
		if ('0' <= d && d <= '9') val = (val<<4)+d-'0';
		else if ('a' <= d && d <= 'f') val = (val<<4)+d-'a'+10;
		else if ('A' <= d && d <= 'F') val = (val<<4)+d-'A'+10;
		else return NoSymbol;
	    }
	    return val;
	}
    }

    if (*s == 'U') {
    	val = 0;
        for (p = &s[1]; *p; p++) {
            c = *p;
	    if ('0' <= c && c <= '9') val = (val<<4)+c-'0';
	    else if ('a' <= c && c <= 'f') val = (val<<4)+c-'a'+10;
	    else if ('A' <= c && c <= 'F') val = (val<<4)+c-'A'+10;
	    else return NoSymbol;
	    if (val > 0x10ffff)
		return NoSymbol;
	}
	if (val < 0x20 || (val > 0x7e && val < 0xa0))
	    return NoSymbol;
	if (val < 0x100)
	    return val;
        return val | 0x01000000;
    }

    if (strlen(s) > 2 && s[0] == '0' && s[1] == 'x') {
        char *tmp = NULL;
        val = strtoul(s, &tmp, 16);
        if (val == ULONG_MAX || (tmp && *tmp != '\0'))
            return NoSymbol;
        else
            return val;
    }

    /* Stupid inconsistency between the headers and XKeysymDB: the former has
     * no separating underscore, while some XF86* syms in the latter did.
     * As a last ditch effort, try without. */
    if (strncmp(s, "XF86_", 5) == 0) {
        KeySym ret;
        char *tmp = strdup(s);
        if (!tmp)
            return NoSymbol;
        memmove(&tmp[4], &tmp[5], strlen(s) - 5 + 1);
        ret = XStringToKeysym(tmp);
        free(tmp);
        return ret;
    }

    return NoSymbol;
}
