/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:     Earle F. Philhower, III
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include <X11/Xatom.h>
#include "propertyst.h"
#include "windowstr.h"
#include "winmultiwindowclass.h"
#include "win.h"

/*
 * Local function
 */

DEFINE_ATOM_HELPER(AtmWmWindowRole, "WM_WINDOW_ROLE")

int
winMultiWindowGetClassHint(WindowPtr pWin, char **res_name, char **res_class)
{
    struct _Window *pwin;
    struct _Property *prop;
    int len_name, len_class;

    if (!pWin || !res_name || !res_class) {
        ErrorF("winMultiWindowGetClassHint - pWin, res_name, or res_class was "
               "NULL\n");
        return 0;
    }

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    *res_name = *res_class = NULL;

    while (prop) {
        if (prop->propertyName == XA_WM_CLASS
            && prop->type == XA_STRING && prop->format == 8 && prop->data) {
            /*
              WM_CLASS property should consist of 2 null terminated strings, but we
              must handle the cases when one or both is absent or not null terminated
            */
            len_name = strlen((char *) prop->data);
            if (len_name > prop->size) len_name = prop->size;

            (*res_name) = malloc(len_name + 1);

            if (!*res_name) {
                ErrorF("winMultiWindowGetClassHint - *res_name was NULL\n");
                return 0;
            }

            /* Copy name and ensure null terminated */
            strncpy((*res_name), prop->data, len_name);
            (*res_name)[len_name] = '\0';

            /* Compute length of class name, it could be that it is absent or not null terminated */
            len_class = (len_name >= prop->size) ? 0 : (strlen(((char *) prop->data) + 1 + len_name));
            if (len_class > prop->size - 1 - len_name) len_class = prop->size - 1 - len_name;

            (*res_class) = malloc(len_class + 1);

            if (!*res_class) {
                ErrorF("winMultiWindowGetClassHint - *res_class was NULL\n");

                /* Free the previously allocated res_name */
                free(*res_name);
                return 0;
            }

            /* Copy class name and ensure null terminated */
            strncpy((*res_class), ((char *) prop->data) + 1 + len_name, len_class);
            (*res_class)[len_class] = '\0';

            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}

int
winMultiWindowGetWMHints(WindowPtr pWin, WinXWMHints * hints)
{
    struct _Window *pwin;
    struct _Property *prop;

    if (!pWin || !hints) {
        ErrorF("winMultiWindowGetWMHints - pWin or hints was NULL\n");
        return 0;
    }

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    memset(hints, 0, sizeof(WinXWMHints));

    while (prop) {
        if (prop->propertyName == XA_WM_HINTS && prop->data) {
            memcpy(hints, prop->data, sizeof(WinXWMHints));
            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}

int
winMultiWindowGetWindowRole(WindowPtr pWin, char **res_role)
{
    struct _Window *pwin;
    struct _Property *prop;
    int len_role;

    if (!pWin || !res_role)
        return 0;

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    *res_role = NULL;
    while (prop) {
        if (prop->propertyName == AtmWmWindowRole()
            && prop->type == XA_STRING && prop->format == 8 && prop->data) {
            len_role = prop->size;

            (*res_role) = malloc(len_role + 1);

            if (!*res_role) {
                ErrorF("winMultiWindowGetWindowRole - *res_role was NULL\n");
                return 0;
            }

            strncpy((*res_role), prop->data, len_role);
            (*res_role)[len_role] = 0;

            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}

int
winMultiWindowGetWMNormalHints(WindowPtr pWin, WinXSizeHints * hints)
{
    struct _Window *pwin;
    struct _Property *prop;

    if (!pWin || !hints) {
        ErrorF("winMultiWindowGetWMNormalHints - pWin or hints was NULL\n");
        return 0;
    }

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    memset(hints, 0, sizeof(WinXSizeHints));

    while (prop) {
        if (prop->propertyName == XA_WM_NORMAL_HINTS && prop->data) {
            memcpy(hints, prop->data, sizeof(WinXSizeHints));
            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}

int
winMultiWindowGetTransientFor(WindowPtr pWin, Window *pDaddyId)
{
    struct _Window *pwin;
    struct _Property *prop;

    if (!pWin) {
        ErrorF("winMultiWindowGetTransientFor - pWin was NULL\n");
        return 0;
    }

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    if (pDaddyId)
        *pDaddyId = 0;

    while (prop) {
        if (prop->propertyName == XA_WM_TRANSIENT_FOR) {
            if (pDaddyId)
                memcpy(pDaddyId, prop->data, sizeof(Window));
            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}

int
winMultiWindowGetWMName(WindowPtr pWin, char **wmName)
{
    struct _Window *pwin;
    struct _Property *prop;
    int len_name;

    if (!pWin || !wmName) {
        ErrorF("winMultiWindowGetClassHint - pWin, res_name, or res_class was "
               "NULL\n");
        return 0;
    }

    pwin = (struct _Window *) pWin;

    if (pwin->optional)
        prop = (struct _Property *) pwin->optional->userProps;
    else
        prop = NULL;

    *wmName = NULL;

    while (prop) {
        if (prop->propertyName == XA_WM_NAME
            && prop->type == XA_STRING && prop->data) {
            len_name = prop->size;

            (*wmName) = malloc(len_name + 1);

            if (!*wmName) {
                ErrorF("winMultiWindowGetWMName - *wmName was NULL\n");
                return 0;
            }

            strncpy((*wmName), prop->data, len_name);
            (*wmName)[len_name] = 0;

            return 1;
        }
        else
            prop = prop->next;
    }

    return 0;
}
