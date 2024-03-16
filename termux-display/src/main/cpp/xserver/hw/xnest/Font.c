/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "regionstr.h"
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "scrnintstr.h"

#include "Xnest.h"

#include "Display.h"
#include "XNFont.h"

int xnestFontPrivateIndex;

Bool
xnestRealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    void *priv;
    Atom name_atom, value_atom;
    int nprops;
    FontPropPtr props;
    int i;
    const char *name;

    xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);

    name_atom = MakeAtom("FONT", 4, True);
    value_atom = 0L;

    nprops = pFont->info.nprops;
    props = pFont->info.props;

    for (i = 0; i < nprops; i++)
        if (props[i].name == name_atom) {
            value_atom = props[i].value;
            break;
        }

    if (!value_atom)
        return False;

    name = NameForAtom(value_atom);

    if (!name)
        return False;

    priv = (void *) malloc(sizeof(xnestPrivFont));
    xfont2_font_set_private(pFont, xnestFontPrivateIndex, priv);

    xnestFontPriv(pFont)->font_struct = XLoadQueryFont(xnestDisplay, name);

    if (!xnestFontStruct(pFont))
        return False;

    return True;
}

Bool
xnestUnrealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    if (xnestFontPriv(pFont)) {
        if (xnestFontStruct(pFont))
            XFreeFont(xnestDisplay, xnestFontStruct(pFont));
        free(xnestFontPriv(pFont));
        xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);
    }
    return True;
}
