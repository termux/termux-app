/*
 * Copyright (c) 2009 Dan Nicholson
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

#include <string.h>
#include "os.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec InputClassTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {DRIVER, "driver"},
    {MATCH_PRODUCT, "matchproduct"},
    {MATCH_VENDOR, "matchvendor"},
    {MATCH_DEVICE_PATH, "matchdevicepath"},
    {MATCH_OS, "matchos"},
    {MATCH_PNPID, "matchpnpid"},
    {MATCH_USBID, "matchusbid"},
    {MATCH_DRIVER, "matchdriver"},
    {MATCH_TAG, "matchtag"},
    {MATCH_LAYOUT, "matchlayout"},
    {MATCH_IS_KEYBOARD, "matchiskeyboard"},
    {MATCH_IS_POINTER, "matchispointer"},
    {MATCH_IS_JOYSTICK, "matchisjoystick"},
    {MATCH_IS_TABLET, "matchistablet"},
    {MATCH_IS_TABLET_PAD, "matchistabletpad"},
    {MATCH_IS_TOUCHPAD, "matchistouchpad"},
    {MATCH_IS_TOUCHSCREEN, "matchistouchscreen"},
    {NOMATCH_PRODUCT, "nomatchproduct"},
    {NOMATCH_VENDOR, "nomatchvendor"},
    {NOMATCH_DEVICE_PATH, "nomatchdevicepath"},
    {NOMATCH_OS, "nomatchos"},
    {NOMATCH_PNPID, "nomatchpnpid"},
    {NOMATCH_USBID, "nomatchusbid"},
    {NOMATCH_DRIVER, "nomatchdriver"},
    {NOMATCH_TAG, "nomatchtag"},
    {NOMATCH_LAYOUT, "nomatchlayout"},
    {-1, ""},
};

static void
xf86freeInputClassList(XF86ConfInputClassPtr ptr)
{
    XF86ConfInputClassPtr prev;

    while (ptr) {
        xf86MatchGroup *group, *next;
        char **list;

        TestFree(ptr->identifier);
        TestFree(ptr->driver);

        xorg_list_for_each_entry_safe(group, next, &ptr->match_product, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_vendor, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_device, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_os, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_pnpid, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_usbid, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_driver, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_tag, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }
        xorg_list_for_each_entry_safe(group, next, &ptr->match_layout, entry) {
            xorg_list_del(&group->entry);
            for (list = group->values; *list; list++)
                free(*list);
            free(group);
        }

        TestFree(ptr->comment);
        xf86optionListFree(ptr->option_lst);

        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

#define CLEANUP xf86freeInputClassList

#define TOKEN_SEP "|"

enum MatchType {
    MATCH_NORMAL,
    MATCH_NEGATED,
};

static void
add_group_entry(struct xorg_list *head, char **values, enum MatchType type)
{
    xf86MatchGroup *group;

    group = malloc(sizeof(*group));
    if (group) {
        group->is_negated = (type == MATCH_NEGATED);
        group->values = values;
        xorg_list_add(&group->entry, head);
    }
}

XF86ConfInputClassPtr
xf86parseInputClassSection(void)
{
    int has_ident = FALSE;
    int token;
    enum MatchType matchtype;

    parsePrologue(XF86ConfInputClassPtr, XF86ConfInputClassRec)

    /* Initialize MatchGroup lists */
    xorg_list_init(&ptr->match_product);
    xorg_list_init(&ptr->match_vendor);
    xorg_list_init(&ptr->match_device);
    xorg_list_init(&ptr->match_os);
    xorg_list_init(&ptr->match_pnpid);
    xorg_list_init(&ptr->match_usbid);
    xorg_list_init(&ptr->match_driver);
    xorg_list_init(&ptr->match_tag);
    xorg_list_init(&ptr->match_layout);

    while ((token = xf86getToken(InputClassTab)) != ENDSECTION) {
        matchtype = MATCH_NORMAL;

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
            if (strcmp(xf86_lex_val.str, "keyboard") == 0) {
                ptr->driver = strdup("kbd");
                free(xf86_lex_val.str);
            }
            else
                ptr->driver = xf86_lex_val.str;
            break;
        case OPTION:
            ptr->option_lst = xf86parseOption(ptr->option_lst);
            break;
        case NOMATCH_PRODUCT:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_PRODUCT:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchProduct");
            add_group_entry(&ptr->match_product,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_VENDOR:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_VENDOR:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchVendor");
            add_group_entry(&ptr->match_vendor,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_DEVICE_PATH:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_DEVICE_PATH:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchDevicePath");
            add_group_entry(&ptr->match_device,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_OS:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_OS:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchOS");
            add_group_entry(&ptr->match_os, xstrtokenize(xf86_lex_val.str,
                                                         TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_PNPID:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_PNPID:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchPnPID");
            add_group_entry(&ptr->match_pnpid,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_USBID:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_USBID:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchUSBID");
            add_group_entry(&ptr->match_usbid,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_DRIVER:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_DRIVER:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchDriver");
            add_group_entry(&ptr->match_driver,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_TAG:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_TAG:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchTag");
            add_group_entry(&ptr->match_tag, xstrtokenize(xf86_lex_val.str,
                                                          TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case NOMATCH_LAYOUT:
            matchtype = MATCH_NEGATED;
            /* fallthrough */
        case MATCH_LAYOUT:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchLayout");
            add_group_entry(&ptr->match_layout,
                            xstrtokenize(xf86_lex_val.str, TOKEN_SEP),
                            matchtype);
            free(xf86_lex_val.str);
            break;
        case MATCH_IS_KEYBOARD:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsKeyboard");
            ptr->is_keyboard.set = xf86getBoolValue(&ptr->is_keyboard.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_keyboard.set)
                Error(BOOL_MSG, "MatchIsKeyboard");
            break;
        case MATCH_IS_POINTER:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsPointer");
            ptr->is_pointer.set = xf86getBoolValue(&ptr->is_pointer.val,
                                                   xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_pointer.set)
                Error(BOOL_MSG, "MatchIsPointer");
            break;
        case MATCH_IS_JOYSTICK:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsJoystick");
            ptr->is_joystick.set = xf86getBoolValue(&ptr->is_joystick.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_joystick.set)
                Error(BOOL_MSG, "MatchIsJoystick");
            break;
        case MATCH_IS_TABLET:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsTablet");
            ptr->is_tablet.set = xf86getBoolValue(&ptr->is_tablet.val, xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_tablet.set)
                Error(BOOL_MSG, "MatchIsTablet");
            break;
        case MATCH_IS_TABLET_PAD:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsTabletPad");
            ptr->is_tablet_pad.set = xf86getBoolValue(&ptr->is_tablet_pad.val, xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_tablet_pad.set)
                Error(BOOL_MSG, "MatchIsTabletPad");
            break;
        case MATCH_IS_TOUCHPAD:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsTouchpad");
            ptr->is_touchpad.set = xf86getBoolValue(&ptr->is_touchpad.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_touchpad.set)
                Error(BOOL_MSG, "MatchIsTouchpad");
            break;
        case MATCH_IS_TOUCHSCREEN:
            if (xf86getSubToken(&(ptr->comment)) != STRING)
                Error(QUOTE_MSG, "MatchIsTouchscreen");
            ptr->is_touchscreen.set = xf86getBoolValue(&ptr->is_touchscreen.val,
                                                       xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_touchscreen.set)
                Error(BOOL_MSG, "MatchIsTouchscreen");
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
    printf("InputClass section parsed\n");
#endif

    return ptr;
}

void
xf86printInputClassSection(FILE * cf, XF86ConfInputClassPtr ptr)
{
    const xf86MatchGroup *group;
    char *const *cur;

    while (ptr) {
        fprintf(cf, "Section \"InputClass\"\n");
        if (ptr->comment)
            fprintf(cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf(cf, "\tIdentifier      \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf(cf, "\tDriver          \"%s\"\n", ptr->driver);

        xorg_list_for_each_entry(group, &ptr->match_product, entry) {
            fprintf(cf, "\tMatchProduct    \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_vendor, entry) {
            fprintf(cf, "\tMatchVendor     \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_device, entry) {
            fprintf(cf, "\tMatchDevicePath \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_os, entry) {
            fprintf(cf, "\tMatchOS         \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_pnpid, entry) {
            fprintf(cf, "\tMatchPnPID      \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_usbid, entry) {
            fprintf(cf, "\tMatchUSBID      \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_driver, entry) {
            fprintf(cf, "\tMatchDriver     \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_tag, entry) {
            fprintf(cf, "\tMatchTag        \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_layout, entry) {
            fprintf(cf, "\tMatchLayout     \"");
            for (cur = group->values; *cur; cur++)
                fprintf(cf, "%s%s", cur == group->values ? "" : TOKEN_SEP,
                        *cur);
            fprintf(cf, "\"\n");
        }

        if (ptr->is_keyboard.set)
            fprintf(cf, "\tIsKeyboard      \"%s\"\n",
                    ptr->is_keyboard.val ? "yes" : "no");
        if (ptr->is_pointer.set)
            fprintf(cf, "\tIsPointer       \"%s\"\n",
                    ptr->is_pointer.val ? "yes" : "no");
        if (ptr->is_joystick.set)
            fprintf(cf, "\tIsJoystick      \"%s\"\n",
                    ptr->is_joystick.val ? "yes" : "no");
        if (ptr->is_tablet.set)
            fprintf(cf, "\tIsTablet        \"%s\"\n",
                    ptr->is_tablet.val ? "yes" : "no");
        if (ptr->is_tablet_pad.set)
            fprintf(cf, "\tIsTabletPad     \"%s\"\n",
                    ptr->is_tablet_pad.val ? "yes" : "no");
        if (ptr->is_touchpad.set)
            fprintf(cf, "\tIsTouchpad      \"%s\"\n",
                    ptr->is_touchpad.val ? "yes" : "no");
        if (ptr->is_touchscreen.set)
            fprintf(cf, "\tIsTouchscreen   \"%s\"\n",
                    ptr->is_touchscreen.val ? "yes" : "no");
        xf86printOptionList(cf, ptr->option_lst, 1);
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}
