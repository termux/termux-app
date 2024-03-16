/* x-hook.c
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "x-hook.h"
#include <stdlib.h>
#include <assert.h>
#include "os.h"

#define CELL_NEW(f, d) X_PFX(list_prepend) ((x_list *)(f), (d))
#define CELL_FREE(c)   X_PFX(list_free_1) (c)
#define CELL_FUN(c)    ((x_hook_function *)((c)->next))
#define CELL_DATA(c)   ((c)->data)

X_EXTERN x_list *
X_PFX(hook_add) (x_list * lst, x_hook_function * fun, void *data) {
    return X_PFX(list_prepend) (lst, CELL_NEW(fun, data));
}

X_EXTERN x_list *
X_PFX(hook_remove) (x_list * lst, x_hook_function * fun, void *data) {
    x_list *node, *cell;
    x_list *to_delete = NULL;

    for (node = lst; node != NULL; node = node->next) {
        cell = node->data;
        if (CELL_FUN(cell) == fun && CELL_DATA(cell) == data)
            to_delete = X_PFX(list_prepend) (to_delete, cell);
    }

    for (node = to_delete; node != NULL; node = node->next) {
        cell = node->data;
        lst = X_PFX(list_remove) (lst, cell);
        CELL_FREE(cell);
    }

    X_PFX(list_free) (to_delete);
    return lst;
}

X_EXTERN void
X_PFX(hook_run) (x_list * lst, void *arg) {
    x_list *node;

    if (!lst)
        return;

    for (node = lst; node != NULL; node = node->next) {
        x_list *cell = node->data;

        x_hook_function *fun = CELL_FUN(cell);
        void *data = CELL_DATA(cell);

        (*fun)(arg, data);
    }
}

X_EXTERN void
X_PFX(hook_free) (x_list * lst) {
    x_list *node;

    for (node = lst; node != NULL; node = node->next) {
        CELL_FREE(node->data);
    }

    X_PFX(list_free) (lst);
}
