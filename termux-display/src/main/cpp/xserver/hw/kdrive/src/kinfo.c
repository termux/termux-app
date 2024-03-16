/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "kdrive.h"

KdCardInfo *kdCardInfo;

KdCardInfo *
KdCardInfoAdd(KdCardFuncs * funcs, void *closure)
{
    KdCardInfo *ci, **prev;

    ci = calloc(1, sizeof(KdCardInfo));
    if (!ci)
        return 0;
    for (prev = &kdCardInfo; *prev; prev = &(*prev)->next);
    *prev = ci;
    ci->cfuncs = funcs;
    ci->closure = closure;
    ci->screenList = 0;
    ci->selected = 0;
    ci->next = 0;
    return ci;
}

KdCardInfo *
KdCardInfoLast(void)
{
    KdCardInfo *ci;

    if (!kdCardInfo)
        return 0;
    for (ci = kdCardInfo; ci->next; ci = ci->next);
    return ci;
}

void
KdCardInfoDispose(KdCardInfo * ci)
{
    KdCardInfo **prev;

    for (prev = &kdCardInfo; *prev; prev = &(*prev)->next)
        if (*prev == ci) {
            *prev = ci->next;
            free(ci);
            break;
        }
}

KdScreenInfo *
KdScreenInfoAdd(KdCardInfo * ci)
{
    KdScreenInfo *si, **prev;
    int n;

    si = calloc(1, sizeof(KdScreenInfo));
    if (!si)
        return 0;
    for (prev = &ci->screenList, n = 0; *prev; prev = &(*prev)->next, n++);
    *prev = si;
    si->next = 0;
    si->card = ci;
    si->mynum = n;
    return si;
}

void
KdScreenInfoDispose(KdScreenInfo * si)
{
    KdCardInfo *ci = si->card;
    KdScreenInfo **prev;

    for (prev = &ci->screenList; *prev; prev = &(*prev)->next) {
        if (*prev == si) {
            *prev = si->next;
            free(si);
            if (!ci->screenList)
                KdCardInfoDispose(ci);
            break;
        }
    }
}

KdPointerInfo *
KdNewPointer(void)
{
    KdPointerInfo *pi;
    int i;

    pi = (KdPointerInfo *) calloc(1, sizeof(KdPointerInfo));
    if (!pi)
        return NULL;

    pi->name = strdup("Generic Pointer");
    pi->path = NULL;
    pi->inputClass = KD_MOUSE;
    pi->driver = NULL;
    pi->driverPrivate = NULL;
    pi->next = NULL;
    pi->options = NULL;
    pi->nAxes = 3;
    pi->nButtons = KD_MAX_BUTTON;
    for (i = 1; i < KD_MAX_BUTTON; i++)
        pi->map[i] = i;

    return pi;
}

void
KdFreePointer(KdPointerInfo * pi)
{
    free(pi->name);
    free(pi->path);
    input_option_free_list(&pi->options);
    pi->next = NULL;
    free(pi);
}

void
KdFreeKeyboard(KdKeyboardInfo * ki)
{
    free(ki->name);
    free(ki->path);
    free(ki->xkbRules);
    free(ki->xkbModel);
    free(ki->xkbLayout);
    free(ki->xkbVariant);
    free(ki->xkbOptions);
    input_option_free_list(&ki->options);
    ki->next = NULL;
    free(ki);
}
