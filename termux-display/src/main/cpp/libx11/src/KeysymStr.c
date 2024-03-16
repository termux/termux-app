
/*

Copyright 1990, 1998  The Open Group

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
#include "Xlibint.h"
#include <X11/Xresource.h>
#include <X11/keysymdef.h>
#include "Xresinternal.h"

#include <stdio.h> /* sprintf */

#define NEEDVTABLE
#include "ks_tables.h"
#include "Key.h"


typedef struct _GRNData {
    char *name;
    XrmRepresentation type;
    XrmValuePtr value;
} GRNData;

/*ARGSUSED*/
static Bool
SameValue(
    XrmDatabase*	db,
    XrmBindingList      bindings,
    XrmQuarkList	quarks,
    XrmRepresentation*  type,
    XrmValuePtr		value,
    XPointer		data
)
{
    GRNData *gd = (GRNData *)data;

    if ((*type == gd->type) && (value->size == gd->value->size) &&
	!strncmp((char *)value->addr, (char *)gd->value->addr, value->size))
    {
	gd->name = XrmQuarkToString(*quarks); /* XXX */
	return True;
    }
    return False;
}

char *XKeysymToString(KeySym ks)
{
    XrmDatabase keysymdb;

    if (!ks || (ks & ((unsigned long) ~0x1fffffff)) != 0)
	return ((char *)NULL);
    if (ks == XK_VoidSymbol)
	ks = 0;
    if (ks <= 0x1fffffff)
    {
	unsigned char val1 = ks >> 24;
	unsigned char val2 = (ks >> 16) & 0xff;
	unsigned char val3 = (ks >> 8) & 0xff;
	unsigned char val4 = ks & 0xff;
	int i = ks % VTABLESIZE;
	int h = i + 1;
	int n = VMAXHASH;
	int idx;
	while ((idx = hashKeysym[i]))
	{
	    const unsigned char *entry = &_XkeyTable[idx];
	    if ((entry[0] == val1) && (entry[1] == val2) &&
                (entry[2] == val3) && (entry[3] == val4))
		return ((char *)entry + 4);
	    if (!--n)
		break;
	    i += h;
	    if (i >= VTABLESIZE)
		i -= VTABLESIZE;
	}
    }

    if ((keysymdb = _XInitKeysymDB()))
    {
	char buf[9];
	XrmValue resval;
	XrmQuark empty = NULLQUARK;
	GRNData data;

	snprintf(buf, sizeof(buf), "%lX", ks);
	resval.addr = (XPointer)buf;
	resval.size = (unsigned)strlen(buf) + 1;
	data.name = (char *)NULL;
	data.type = XrmPermStringToQuark("String");
	data.value = &resval;
	(void)XrmEnumerateDatabase(keysymdb, &empty, &empty, XrmEnumAllLevels,
				   SameValue, (XPointer)&data);
        if (data.name)
	    return data.name;
    }
    if (ks >= 0x01000100 && ks <= 0x0110ffff) {
        KeySym val = ks & 0xffffff;
        char *s;
        int i;
        if (val & 0xff0000)
            i = 10;
        else
            i = 6;
        s = Xmalloc(i);
        if (s == NULL)
            return s;
        i--;
        s[i--] = '\0';
        for (; i; i--){
            unsigned char val1 = val & 0xf;
            val >>= 4;
            if (val1 < 10)
                s[i] = '0'+ val1;
            else
                s[i] = 'A'+ val1 - 10;
        }
        s[i] = 'U';
        return s;
    }
    return ((char *) NULL);
}
