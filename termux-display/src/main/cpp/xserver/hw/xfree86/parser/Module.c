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


static const xf86ConfigSymTabRec SubModuleTab[] = {
    {ENDSUBSECTION, "endsubsection"},
    {OPTION, "option"},
    {-1, ""},
};

static const xf86ConfigSymTabRec ModuleTab[] = {
    {ENDSECTION, "endsection"},
    {LOAD, "load"},
    {DISABLE, "disable"},
    {LOAD_DRIVER, "loaddriver"},
    {SUBSECTION, "subsection"},
    {-1, ""},
};

#define CLEANUP xf86freeModules

static XF86LoadPtr
xf86parseModuleSubSection(XF86LoadPtr head, char *name)
{
    int token;

    parsePrologue(XF86LoadPtr, XF86LoadRec)

        ptr->load_name = name;
    ptr->load_type = XF86_LOAD_MODULE;
    ptr->ignore = 0;
    ptr->load_opt = NULL;
    ptr->list.next = NULL;

    while ((token = xf86getToken(SubModuleTab)) != ENDSUBSECTION) {
        switch (token) {
        case COMMENT:
            ptr->load_comment = xf86addComment(ptr->load_comment, xf86_lex_val.str);
            break;
        case OPTION:
            ptr->load_opt = xf86parseOption(ptr->load_opt);
            break;
        case EOF_TOKEN:
            xf86parseError(UNEXPECTED_EOF_MSG);
            free(ptr);
            return NULL;
        default:
            xf86parseError(INVALID_KEYWORD_MSG, xf86tokenString());
            free(ptr);
            return NULL;
            break;
        }

    }

    return ((XF86LoadPtr) xf86addListItem((glp) head, (glp) ptr));
}

XF86ConfModulePtr
xf86parseModuleSection(void)
{
    int token;

    parsePrologue(XF86ConfModulePtr, XF86ConfModuleRec)

        while ((token = xf86getToken(ModuleTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->mod_comment = xf86addComment(ptr->mod_comment, xf86_lex_val.str);
            break;
        case LOAD:
            if (xf86getSubToken(&(ptr->mod_comment)) != STRING)
                Error(QUOTE_MSG, "Load");
            ptr->mod_load_lst =
                xf86addNewLoadDirective(ptr->mod_load_lst, xf86_lex_val.str,
                                        XF86_LOAD_MODULE, NULL);
            break;
        case DISABLE:
            if (xf86getSubToken(&(ptr->mod_comment)) != STRING)
                Error(QUOTE_MSG, "Disable");
            ptr->mod_disable_lst =
                xf86addNewLoadDirective(ptr->mod_disable_lst, xf86_lex_val.str,
                                        XF86_DISABLE_MODULE, NULL);
            break;
        case LOAD_DRIVER:
            if (xf86getSubToken(&(ptr->mod_comment)) != STRING)
                Error(QUOTE_MSG, "LoadDriver");
            ptr->mod_load_lst =
                xf86addNewLoadDirective(ptr->mod_load_lst, xf86_lex_val.str,
                                        XF86_LOAD_DRIVER, NULL);
            break;
        case SUBSECTION:
            if (xf86getSubToken(&(ptr->mod_comment)) != STRING)
                Error(QUOTE_MSG, "SubSection");
            ptr->mod_load_lst =
                xf86parseModuleSubSection(ptr->mod_load_lst, xf86_lex_val.str);
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
    printf("Module section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printModuleSection(FILE * cf, XF86ConfModulePtr ptr)
{
    XF86LoadPtr lptr;

    if (ptr == NULL)
        return;

    if (ptr->mod_comment)
        fprintf(cf, "%s", ptr->mod_comment);
    for (lptr = ptr->mod_load_lst; lptr; lptr = lptr->list.next) {
        switch (lptr->load_type) {
        case XF86_LOAD_MODULE:
            if (lptr->load_opt == NULL) {
                fprintf(cf, "\tLoad  \"%s\"", lptr->load_name);
                if (lptr->load_comment)
                    fprintf(cf, "%s", lptr->load_comment);
                else
                    fputc('\n', cf);
            }
            else {
                fprintf(cf, "\tSubSection \"%s\"\n", lptr->load_name);
                if (lptr->load_comment)
                    fprintf(cf, "%s", lptr->load_comment);
                xf86printOptionList(cf, lptr->load_opt, 2);
                fprintf(cf, "\tEndSubSection\n");
            }
            break;
        case XF86_LOAD_DRIVER:
            fprintf(cf, "\tLoadDriver  \"%s\"", lptr->load_name);
            if (lptr->load_comment)
                fprintf(cf, "%s", lptr->load_comment);
            else
                fputc('\n', cf);
            break;
#if 0
        default:
            fprintf(cf, "#\tUnknown type  \"%s\"\n", lptr->load_name);
            break;
#endif
        }
    }
}

XF86LoadPtr
xf86addNewLoadDirective(XF86LoadPtr head, const char *name, int type,
                        XF86OptionPtr opts)
{
    XF86LoadPtr new;
    int token;

    new = calloc(1, sizeof(XF86LoadRec));
    new->load_name = name;
    new->load_type = type;
    new->load_opt = opts;
    new->ignore = 0;
    new->list.next = NULL;

    if ((token = xf86getToken(NULL)) == COMMENT)
        new->load_comment = xf86addComment(new->load_comment, xf86_lex_val.str);
    else
        xf86unGetToken(token);

    return ((XF86LoadPtr) xf86addListItem((glp) head, (glp) new));
}

void
xf86freeModules(XF86ConfModulePtr ptr)
{
    XF86LoadPtr lptr;
    XF86LoadPtr prev;

    if (ptr == NULL)
        return;
    lptr = ptr->mod_load_lst;
    while (lptr) {
        TestFree(lptr->load_name);
        TestFree(lptr->load_comment);
        prev = lptr;
        lptr = lptr->list.next;
        free(prev);
    }
    lptr = ptr->mod_disable_lst;
    while (lptr) {
        TestFree(lptr->load_name);
        TestFree(lptr->load_comment);
        prev = lptr;
        lptr = lptr->list.next;
        free(prev);
    }
    TestFree(ptr->mod_comment);
    free(ptr);
}
