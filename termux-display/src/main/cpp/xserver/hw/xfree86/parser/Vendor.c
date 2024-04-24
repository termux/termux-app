/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec VendorSubTab[] = {
    {ENDSUBSECTION, "endsubsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {-1, ""},
};

static void
xf86freeVendorSubList(XF86ConfVendSubPtr ptr)
{
    XF86ConfVendSubPtr prev;

    while (ptr) {
        TestFree(ptr->vs_identifier);
        TestFree(ptr->vs_name);
        TestFree(ptr->vs_comment);
        xf86optionListFree(ptr->vs_option_lst);
        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

#define CLEANUP xf86freeVendorSubList

static XF86ConfVendSubPtr
xf86parseVendorSubSection(void)
{
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfVendSubPtr, XF86ConfVendSubRec)

        while ((token = xf86getToken(VendorSubTab)) != ENDSUBSECTION) {
        switch (token) {
        case COMMENT:
            ptr->vs_comment = xf86addComment(ptr->vs_comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->vs_comment)))
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->vs_identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case OPTION:
            ptr->vs_option_lst = xf86parseOption(ptr->vs_option_lst);
            break;

        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

#ifdef DEBUG
    printf("Vendor subsection parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

static const xf86ConfigSymTabRec VendorTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {SUBSECTION, "subsection"},
    {-1, ""},
};

#define CLEANUP xf86freeVendorList

XF86ConfVendorPtr
xf86parseVendorSection(void)
{
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfVendorPtr, XF86ConfVendorRec)

        while ((token = xf86getToken(VendorTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->vnd_comment = xf86addComment(ptr->vnd_comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->vnd_comment)) != STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->vnd_identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case OPTION:
            ptr->vnd_option_lst = xf86parseOption(ptr->vnd_option_lst);
            break;
        case SUBSECTION:
            if (xf86getSubToken(&(ptr->vnd_comment)) != STRING)
                Error(QUOTE_MSG, "SubSection");
            {
                HANDLE_LIST(vnd_sub_lst, xf86parseVendorSubSection,
                            XF86ConfVendSubPtr);
            }
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }

    }

    if (!has_ident)
        Error(NO_IDENT_MSG);

#ifdef DEBUG
    printf("Vendor section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printVendorSection(FILE * cf, XF86ConfVendorPtr ptr)
{
    XF86ConfVendSubPtr pptr;

    while (ptr) {
        fprintf(cf, "Section \"Vendor\"\n");
        if (ptr->vnd_comment)
            fprintf(cf, "%s", ptr->vnd_comment);
        if (ptr->vnd_identifier)
            fprintf(cf, "\tIdentifier     \"%s\"\n", ptr->vnd_identifier);

        xf86printOptionList(cf, ptr->vnd_option_lst, 1);
        for (pptr = ptr->vnd_sub_lst; pptr; pptr = pptr->list.next) {
            fprintf(cf, "\tSubSection \"Vendor\"\n");
            if (pptr->vs_comment)
                fprintf(cf, "%s", pptr->vs_comment);
            if (pptr->vs_identifier)
                fprintf(cf, "\t\tIdentifier \"%s\"\n", pptr->vs_identifier);
            xf86printOptionList(cf, pptr->vs_option_lst, 2);
            fprintf(cf, "\tEndSubSection\n");
        }
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}

void
xf86freeVendorList(XF86ConfVendorPtr p)
{
    if (p == NULL)
        return;
    xf86freeVendorSubList(p->vnd_sub_lst);
    TestFree(p->vnd_identifier);
    TestFree(p->vnd_comment);
    xf86optionListFree(p->vnd_option_lst);
    free(p);
}
