/*
 * Copyright (c) 2014 NVIDIA Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "os.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

static const xf86ConfigSymTabRec OutputClassTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {DRIVER, "driver"},
    {MODULEPATH, "modulepath"},
    {OPTION, "option"},
    {MATCH_DRIVER, "matchdriver"},
    {-1, ""},
};

static void
xf86freeOutputClassList(XF86ConfOutputClassPtr ptr)
{
    XF86ConfOutputClassPtr prev;

    while (ptr) {
        xf86MatchGroup *group, *next;
        char **list;

        TestFree(ptr->identifier);
        TestFree(ptr->comment);
        TestFree(ptr->driver);
        TestFree(ptr->modulepath);

        xorg_list_for_each_entry_safe(group, next, &ptr->match_driver, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }

        xf86optionListFree(ptr->option_lst);

        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

#define CLEANUP xf86freeOutputClassList

#define TOKEN_SEP "|"

static void
add_group_entry(struct xorg_list *head, char **values)
{
    xf86MatchGroup *group;

    group = malloc(sizeof(*group));
    if (group) {
        group->values = values;
        xorg_list_add(&group->entry, head);
    }
}

XF86ConfOutputClassPtr
xf86parseOutputClassSection(void)
{
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfOutputClassPtr, XF86ConfOutputClassRec)

    /* Initialize MatchGroup lists */
    xorg_list_init(&ptr->match_driver);

    while ((token = xf86getToken(OutputClassTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->comment = xf86addComment(ptr->comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case DRIVER:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "Driver");
            else
                ptr->driver = xf86_lex_val.str;
            break;
        case MODULEPATH:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "ModulePath");
            if (ptr->modulepath) {
                char *path;
                XNFasprintf(&path, "%s,%s", ptr->modulepath, xf86_lex_val.str);
                free(xf86_lex_val.str);
                free(ptr->modulepath);
                ptr->modulepath = path;
            } else {
                ptr->modulepath = xf86_lex_val.str;
            }
            break;
        case OPTION:
            ptr->option_lst = xf86parseOption(ptr->option_lst);
            break;
        case MATCH_DRIVER:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchDriver");
            add_group_entry(&ptr->match_driver,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP));
            free(xf86_lex_val.str);
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
    printf("OutputClass section parsed\n");
#endif

    return ptr;
}
void
xf86printOutputClassSection(FILE * cf, XF86ConfOutputClassPtr ptr)
{
    const xf86MatchGroup *group;
    char *const *cur;

    while (ptr) {
        fprintf(cf, "Section \"OutputClass\"\n");
        if (ptr->comment)
            fprintf(cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf(cf, "\tIdentifier      \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf(cf, "\tDriver          \"%s\"\n", ptr->driver);

        xorg_list_for_each_entry(group, &ptr->match_driver, entry) {
            fprintf(cf, "\tMatchDriver     \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }

        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}
