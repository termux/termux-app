/*
 * Copyright 2004 Red Hat Inc., Raleigh, North Carolina.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL RED HAT AND/OR THEIR SUPPLIERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <kem@redhat.com>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "os.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec ExtensionsTab[] = {
    {ENDSECTION, "endsection"},
    {OPTION, "option"},
    {-1, ""},
};

#define CLEANUP xf86freeExtensions

XF86ConfExtensionsPtr
xf86parseExtensionsSection(void)
{
    int token;

    parsePrologue(XF86ConfExtensionsPtr, XF86ConfExtensionsRec);

    while ((token = xf86getToken(ExtensionsTab)) != ENDSECTION) {
        switch (token) {
        case OPTION:
            ptr->ext_option_lst = xf86parseOption(ptr->ext_option_lst);
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        case COMMENT:
            ptr->extensions_comment =
                xf86addComment(ptr->extensions_comment, xf86_lex_val.str);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

#ifdef DEBUG
    ErrorF("Extensions section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printExtensionsSection(FILE * cf, XF86ConfExtensionsPtr ptr)
{
    XF86OptionPtr p;

    if (ptr == NULL || ptr->ext_option_lst == NULL)
        return;

    p = ptr->ext_option_lst;
    fprintf(cf, "Section \"Extensions\"\n");
    if (ptr->extensions_comment)
        fprintf(cf, "%s", ptr->extensions_comment);
    xf86printOptionList(cf, p, 1);
    fprintf(cf, "EndSection\n\n");
}

void
xf86freeExtensions(XF86ConfExtensionsPtr ptr)
{
    if (ptr == NULL)
        return;

    xf86optionListFree(ptr->ext_option_lst);
    TestFree(ptr->extensions_comment);
    free(ptr);
}
