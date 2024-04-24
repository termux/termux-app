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

#include "xf86Config.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec TopLevelTab[] = {
    {SECTION, "section"},
    {-1, ""},
};

#define CLEANUP xf86freeConfig

/*
 * This function resolves name references and reports errors if the named
 * objects cannot be found.
 */
static int
xf86validateConfig(XF86ConfigPtr p)
{
    if (!xf86validateScreen(p))
        return FALSE;
    if (!xf86validateInput(p))
        return FALSE;
    if (!xf86validateLayout(p))
        return FALSE;

    return TRUE;
}

XF86ConfigPtr
xf86readConfigFile(void)
{
    int token;
    XF86ConfigPtr ptr = NULL;

    if ((ptr = xf86allocateConfig()) == NULL) {
        return NULL;
    }

    while ((token = xf86getToken(TopLevelTab)) != EOF_TOKEN) {
        switch (token) {
        case COMMENT:
            ptr->conf_comment = xf86addComment(ptr->conf_comment, xf86_lex_val.str);
            break;
        case SECTION:
            if (xf86getSubToken(&(ptr->conf_comment)) != STRING) {
                xf86parseError(QUOTE_MSG, "Section");
                CLEANUP(ptr);
                return NULL;
            }
            xf86setSection(xf86_lex_val.str);
            if (xf86nameCompare(xf86_lex_val.str, "files") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_RETURN(conf_files, xf86parseFilesSection());
            }
            else if (xf86nameCompare(xf86_lex_val.str, "serverflags") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_RETURN(conf_flags, xf86parseFlagsSection());
            }
            else if (xf86nameCompare(xf86_lex_val.str, "pointer") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_input_lst, xf86parsePointerSection,
                            XF86ConfInputPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "videoadaptor") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_videoadaptor_lst, xf86parseVideoAdaptorSection,
                            XF86ConfVideoAdaptorPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "device") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_device_lst, xf86parseDeviceSection,
                            XF86ConfDevicePtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "monitor") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_monitor_lst, xf86parseMonitorSection,
                            XF86ConfMonitorPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "modes") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_modes_lst, xf86parseModesSection,
                            XF86ConfModesPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "screen") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_screen_lst, xf86parseScreenSection,
                            XF86ConfScreenPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "inputdevice") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_input_lst, xf86parseInputSection,
                            XF86ConfInputPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "inputclass") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_inputclass_lst,
                            xf86parseInputClassSection, XF86ConfInputClassPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "outputclass") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_outputclass_lst, xf86parseOutputClassSection,
                            XF86ConfOutputClassPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "module") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_RETURN(conf_modules, xf86parseModuleSection());
            }
            else if (xf86nameCompare(xf86_lex_val.str, "serverlayout") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_layout_lst, xf86parseLayoutSection,
                            XF86ConfLayoutPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "vendor") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_LIST(conf_vendor_lst, xf86parseVendorSection,
                            XF86ConfVendorPtr);
            }
            else if (xf86nameCompare(xf86_lex_val.str, "dri") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_RETURN(conf_dri, xf86parseDRISection());
            }
            else if (xf86nameCompare(xf86_lex_val.str, "extensions") == 0) {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                HANDLE_RETURN(conf_extensions, xf86parseExtensionsSection());
            }
            else {
                free(xf86_lex_val.str);
                xf86_lex_val.str = NULL;
                Error(INVALID_SECTION_MSG, xf86tokenString());
            }
            break;
        default:
            free(xf86_lex_val.str);
            xf86_lex_val.str = NULL;
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
        }
    }

    if (xf86validateConfig(ptr))
        return ptr;
    else {
        CLEANUP(ptr);
        return NULL;
    }
}

#undef CLEANUP

/*
 * adds an item to the end of the linked list. Any record whose first field
 * is a GenericListRec can be cast to this type and used with this function.
 * A pointer to the head of the list is returned to handle the addition of
 * the first item.
 */
GenericListPtr
xf86addListItem(GenericListPtr head, GenericListPtr new)
{
    GenericListPtr p = head;
    GenericListPtr last = NULL;

    while (p) {
        last = p;
        p = p->next;
    }

    if (last) {
        last->next = new;
        return head;
    }
    else
        return new;
}

/*
 * Test if one chained list contains the other.
 * In this case both list have the same endpoint (provided they don't loop)
 */
int
xf86itemNotSublist(GenericListPtr list_1, GenericListPtr list_2)
{
    GenericListPtr p = list_1;
    GenericListPtr last_1 = NULL, last_2 = NULL;

    while (p) {
        last_1 = p;
        p = p->next;
    }

    p = list_2;
    while (p) {
        last_2 = p;
        p = p->next;
    }

    return (!(last_1 == last_2));
}

/*
 * Conditionally allocate config struct, but only allocate it
 * if it's not already there.  In either event, return the pointer
 * to the global config struct.
 */
XF86ConfigPtr xf86allocateConfig(void)
{
    if (!xf86configptr) {
        xf86configptr = calloc(1, sizeof(XF86ConfigRec));
    }
    return xf86configptr;
}

void
xf86freeConfig(XF86ConfigPtr p)
{
    if (p == NULL)
        return;

    xf86freeFiles(p->conf_files);
    xf86freeModules(p->conf_modules);
    xf86freeFlags(p->conf_flags);
    xf86freeMonitorList(p->conf_monitor_lst);
    xf86freeModesList(p->conf_modes_lst);
    xf86freeVideoAdaptorList(p->conf_videoadaptor_lst);
    xf86freeDeviceList(p->conf_device_lst);
    xf86freeScreenList(p->conf_screen_lst);
    xf86freeLayoutList(p->conf_layout_lst);
    xf86freeInputList(p->conf_input_lst);
    xf86freeVendorList(p->conf_vendor_lst);
    xf86freeDRI(p->conf_dri);
    xf86freeExtensions(p->conf_extensions);
    TestFree(p->conf_comment);

    free(p);
}
