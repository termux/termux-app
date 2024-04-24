/*
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
#include <X11/Xfuncproto.h>
#include "Xprintf.h"
#include "optionstr.h"


static const xf86ConfigSymTabRec ServerFlagsTab[] = {
    {ENDSECTION, "endsection"},
    {DONTZAP, "dontzap"},
    {DONTZOOM, "dontzoom"},
    {DISABLEVIDMODE, "disablevidmodeextension"},
    {ALLOWNONLOCAL, "allownonlocalxvidtune"},
    {DISABLEMODINDEV, "disablemodindev"},
    {MODINDEVALLOWNONLOCAL, "allownonlocalmodindev"},
    {ALLOWMOUSEOPENFAIL, "allowmouseopenfail"},
    {OPTION, "option"},
    {BLANKTIME, "blanktime"},
    {STANDBYTIME, "standbytime"},
    {SUSPENDTIME, "suspendtime"},
    {OFFTIME, "offtime"},
    {DEFAULTLAYOUT, "defaultserverlayout"},
    {-1, ""},
};

#define CLEANUP xf86freeFlags

XF86ConfFlagsPtr
xf86parseFlagsSection(void)
{
    int token;

    parsePrologue(XF86ConfFlagsPtr, XF86ConfFlagsRec)

        while ((token = xf86getToken(ServerFlagsTab)) != ENDSECTION) {
        int hasvalue = FALSE;
        int strvalue = FALSE;
        int tokentype;

        switch (token) {
        case COMMENT:
            ptr->flg_comment = xf86addComment(ptr->flg_comment, xf86_lex_val.str);
            break;
            /*
             * these old keywords are turned into standard generic options.
             * we fall through here on purpose
             */
        case DEFAULTLAYOUT:
            strvalue = TRUE;
        case BLANKTIME:
        case STANDBYTIME:
        case SUSPENDTIME:
        case OFFTIME:
            hasvalue = TRUE;
        case DONTZAP:
        case DONTZOOM:
        case DISABLEVIDMODE:
        case ALLOWNONLOCAL:
        case DISABLEMODINDEV:
        case MODINDEVALLOWNONLOCAL:
        case ALLOWMOUSEOPENFAIL:
        {
            int i = 0;

            while (ServerFlagsTab[i].token != -1) {
                char *tmp;

                if (ServerFlagsTab[i].token == token) {
                    char *valstr = NULL;

                    tmp = strdup(ServerFlagsTab[i].name);
                    if (hasvalue) {
                        tokentype = xf86getSubToken(&(ptr->flg_comment));
                        if (strvalue) {
                            if (tokentype != STRING)
                                Error(QUOTE_MSG, tmp);
                            valstr = xf86_lex_val.str;
                        }
                        else {
                            if (tokentype != NUMBER)
                                Error(NUMBER_MSG, tmp);
                            if (asprintf(&valstr, "%d", xf86_lex_val.num) == -1)
                                valstr = NULL;
                        }
                    }
                    ptr->flg_option_lst = xf86addNewOption
                        (ptr->flg_option_lst, tmp, valstr);
                }
                i++;
            }
        }
            break;
        case OPTION:
            ptr->flg_option_lst = xf86parseOption(ptr->flg_option_lst);
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
    printf("Flags section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printServerFlagsSection(FILE * f, XF86ConfFlagsPtr flags)
{
    XF86OptionPtr p;

    if ((!flags) || (!flags->flg_option_lst))
        return;
    p = flags->flg_option_lst;
    fprintf(f, "Section \"ServerFlags\"\n");
    if (flags->flg_comment)
        fprintf(f, "%s", flags->flg_comment);
    xf86printOptionList(f, p, 1);
    fprintf(f, "EndSection\n\n");
}

static XF86OptionPtr
addNewOption2(XF86OptionPtr head, char *name, char *_val, int used)
{
    XF86OptionPtr new, old = NULL;

    /* Don't allow duplicates, free old strings */
    if (head != NULL && (old = xf86findOption(head, name)) != NULL) {
        new = old;
        free(new->opt_name);
        free(new->opt_val);
    }
    else
        new = calloc(1, sizeof(*new));
    new->opt_name = name;
    new->opt_val = _val;
    new->opt_used = used;

    if (old)
        return head;
    return ((XF86OptionPtr) xf86addListItem((glp) head, (glp) new));
}

XF86OptionPtr
xf86addNewOption(XF86OptionPtr head, char *name, char *_val)
{
    return addNewOption2(head, name, _val, 0);
}

void
xf86freeFlags(XF86ConfFlagsPtr flags)
{
    if (flags == NULL)
        return;
    xf86optionListFree(flags->flg_option_lst);
    TestFree(flags->flg_comment);
    free(flags);
}

XF86OptionPtr
xf86optionListDup(XF86OptionPtr opt)
{
    XF86OptionPtr newopt = NULL;
    char *_val;

    while (opt) {
        _val = opt->opt_val ? strdup(opt->opt_val) : NULL;
        newopt = xf86addNewOption(newopt, strdup(opt->opt_name), _val);
        newopt->opt_used = opt->opt_used;
        if (opt->opt_comment)
            newopt->opt_comment = strdup(opt->opt_comment);
        opt = opt->list.next;
    }
    return newopt;
}

void
xf86optionListFree(XF86OptionPtr opt)
{
    XF86OptionPtr prev;

    while (opt) {
        TestFree(opt->opt_name);
        TestFree(opt->opt_val);
        TestFree(opt->opt_comment);
        prev = opt;
        opt = opt->list.next;
        free(prev);
    }
}

char *
xf86optionName(XF86OptionPtr opt)
{
    if (opt)
        return opt->opt_name;
    return 0;
}

char *
xf86optionValue(XF86OptionPtr opt)
{
    if (opt)
        return opt->opt_val;
    return 0;
}

XF86OptionPtr
xf86newOption(char *name, char *value)
{
    XF86OptionPtr opt;

    opt = calloc(1, sizeof(*opt));
    if (!opt)
        return NULL;

    opt->opt_used = 0;
    opt->list.next = 0;
    opt->opt_name = name;
    opt->opt_val = value;

    return opt;
}

XF86OptionPtr
xf86nextOption(XF86OptionPtr list)
{
    if (!list)
        return NULL;
    return list->list.next;
}

/*
 * this function searches the given option list for the named option and
 * returns a pointer to the option rec if found. If not found, it returns
 * NULL
 */

XF86OptionPtr
xf86findOption(XF86OptionPtr list, const char *name)
{
    while (list) {
        if (xf86nameCompare(list->opt_name, name) == 0)
            return list;
        list = list->list.next;
    }
    return NULL;
}

/*
 * this function searches the given option list for the named option. If
 * found and the option has a parameter, a pointer to the parameter is
 * returned.  If the option does not have a parameter an empty string is
 * returned.  If the option is not found, a NULL is returned.
 */

const char *
xf86findOptionValue(XF86OptionPtr list, const char *name)
{
    XF86OptionPtr p = xf86findOption(list, name);

    if (p) {
        if (p->opt_val)
            return p->opt_val;
        else
            return "";
    }
    return NULL;
}

XF86OptionPtr
xf86optionListCreate(const char **options, int count, int used)
{
    XF86OptionPtr p = NULL;
    char *t1, *t2;
    int i;

    if (count == -1) {
        for (count = 0; options[count]; count++);
    }
    if ((count % 2) != 0) {
        fprintf(stderr,
                "xf86optionListCreate: count must be an even number.\n");
        return NULL;
    }
    for (i = 0; i < count; i += 2) {
        t1 = strdup(options[i]);
        t2 = strdup(options[i + 1]);
        p = addNewOption2(p, t1, t2, used);
    }

    return p;
}

/* the 2 given lists are merged. If an option with the same name is present in
 * both, the option from the user list - specified in the second argument -
 * is used. The end result is a single valid list of options. Duplicates
 * are freed, and the original lists are no longer guaranteed to be complete.
 */
XF86OptionPtr
xf86optionListMerge(XF86OptionPtr head, XF86OptionPtr tail)
{
    XF86OptionPtr a, b, ap = NULL, bp = NULL;

    a = tail;
    b = head;
    while (tail && b) {
        if (xf86nameCompare(a->opt_name, b->opt_name) == 0) {
            if (b == head)
                head = a;
            else
                bp->list.next = a;
            if (a == tail)
                tail = a->list.next;
            else
                ap->list.next = a->list.next;
            a->list.next = b->list.next;
            b->list.next = NULL;
            xf86optionListFree(b);
            b = a->list.next;
            bp = a;
            a = tail;
            ap = NULL;
        }
        else {
            ap = a;
            if (!(a = a->list.next)) {
                a = tail;
                bp = b;
                b = b->list.next;
                ap = NULL;
            }
        }
    }

    if (head) {
        for (a = head; a->list.next; a = a->list.next);
        a->list.next = tail;
    }
    else
        head = tail;

    return head;
}

char *
xf86uLongToString(unsigned long i)
{
    char *s;

    if (asprintf(&s, "%lu", i) == -1)
        return NULL;
    return s;
}

XF86OptionPtr
xf86parseOption(XF86OptionPtr head)
{
    XF86OptionPtr option, cnew, old;
    char *name, *comment = NULL;
    int token;

    if ((token = xf86getSubToken(&comment)) != STRING) {
        xf86parseError(BAD_OPTION_MSG);
        free(comment);
        return head;
    }

    name = xf86_lex_val.str;
    if ((token = xf86getSubToken(&comment)) == STRING) {
        option = xf86newOption(name, xf86_lex_val.str);
        option->opt_comment = comment;
        if ((token = xf86getToken(NULL)) == COMMENT)
            option->opt_comment = xf86addComment(option->opt_comment, xf86_lex_val.str);
        else
            xf86unGetToken(token);
    }
    else {
        option = xf86newOption(name, NULL);
        option->opt_comment = comment;
        if (token == COMMENT)
            option->opt_comment = xf86addComment(option->opt_comment, xf86_lex_val.str);
        else
            xf86unGetToken(token);
    }

    old = NULL;

    /* Don't allow duplicates */
    if (head != NULL && (old = xf86findOption(head, name)) != NULL) {
        cnew = old;
        free(option->opt_name);
        TestFree(option->opt_val);
        TestFree(option->opt_comment);
        free(option);
    }
    else
        cnew = option;

    if (old == NULL)
        return ((XF86OptionPtr) xf86addListItem((glp) head, (glp) cnew));

    return head;
}

void
xf86printOptionList(FILE * fp, XF86OptionPtr list, int tabs)
{
    int i;

    if (!list)
        return;
    while (list) {
        for (i = 0; i < tabs; i++)
            fputc('\t', fp);
        if (list->opt_val)
            fprintf(fp, "Option	    \"%s\" \"%s\"", list->opt_name,
                    list->opt_val);
        else
            fprintf(fp, "Option	    \"%s\"", list->opt_name);
        if (list->opt_comment)
            fprintf(fp, "%s", list->opt_comment);
        else
            fputc('\n', fp);
        list = list->list.next;
    }
}
