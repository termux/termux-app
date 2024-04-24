/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"

static void
_XkbAddDrawable(XkbDrawablePtr *pfirst, XkbDrawablePtr *plast,
                XkbDrawablePtr tmp)
{
    XkbDrawablePtr old;

    if (*pfirst == NULL) {
        *pfirst = *plast = tmp;
    }
    else if (tmp->priority >= (*plast)->priority) {
        (*plast)->next = tmp;
        *plast = tmp;
    }
    else if (tmp->priority < (*pfirst)->priority) {
        tmp->next = (*pfirst);
        (*pfirst) = tmp;
    }
    else {
        old = *pfirst;
        while ((old->next) && (old->next->priority <= tmp->priority)) {
            old = old->next;
        }
        tmp->next = old->next;
        old->next = tmp;
    }
    return;
}

XkbDrawablePtr
XkbGetOrderedDrawables(XkbGeometryPtr geom, XkbSectionPtr section)
{
    XkbDrawablePtr first, last, tmp;
    int i;

    first = last = NULL;
    if (geom != NULL) {
        XkbSectionPtr section;
        XkbDoodadPtr doodad;

        for (i = 0, section = geom->sections; i < geom->num_sections;
             i++, section++) {
            tmp = _XkbTypedCalloc(1, XkbDrawableRec);
            if (!tmp) {
                XkbFreeOrderedDrawables(first);
                return NULL;
            }
            tmp->type = XkbDW_Section;
            tmp->priority = section->priority;
            tmp->u.section = section;
            tmp->next = NULL;
            _XkbAddDrawable(&first, &last, tmp);
        }
        for (i = 0, doodad = geom->doodads; i < geom->num_doodads;
             i++, doodad++) {
            tmp = _XkbTypedCalloc(1, XkbDrawableRec);
            if (!tmp) {
                XkbFreeOrderedDrawables(first);
                return NULL;
            }
            tmp->type = XkbDW_Doodad;
            tmp->priority = doodad->any.priority;
            tmp->u.doodad = doodad;
            tmp->next = NULL;
            _XkbAddDrawable(&first, &last, tmp);
        }
    }
    if (section != NULL) {
        XkbDoodadPtr doodad;

        for (i = 0, doodad = section->doodads; i < section->num_doodads;
             i++, doodad++) {
            tmp = _XkbTypedCalloc(1, XkbDrawableRec);
            if (!tmp) {
                XkbFreeOrderedDrawables(first);
                return NULL;
            }
            tmp->type = XkbDW_Doodad;
            tmp->priority = doodad->any.priority;
            tmp->u.doodad = doodad;
            tmp->next = NULL;
            _XkbAddDrawable(&first, &last, tmp);
        }
    }
    return first;
}

void
XkbFreeOrderedDrawables(XkbDrawablePtr draw)
{
    XkbDrawablePtr tmp;

    for (; draw != NULL; draw = tmp) {
        tmp = draw->next;
        _XkbFree(draw);
    }
    return;
}
