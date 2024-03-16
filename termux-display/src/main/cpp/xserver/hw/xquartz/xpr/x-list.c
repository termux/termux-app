/* x-list.c
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

#include "x-list.h"
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

/* Allocate in ~4k blocks */
#define NODES_PER_BLOCK 508

typedef struct x_list_block_struct x_list_block;

struct x_list_block_struct {
    x_list l[NODES_PER_BLOCK];
};

static x_list *freelist;

static pthread_mutex_t freelist_lock = PTHREAD_MUTEX_INITIALIZER;

static inline void
list_free_1(x_list *node)
{
    node->next = freelist;
    freelist = node;
}

X_EXTERN void
X_PFX(list_free_1) (x_list * node) {
    assert(node != NULL);

    pthread_mutex_lock(&freelist_lock);

    list_free_1(node);

    pthread_mutex_unlock(&freelist_lock);
}

X_EXTERN void
X_PFX(list_free) (x_list * lst) {
    x_list *next;

    pthread_mutex_lock(&freelist_lock);

    for (; lst != NULL; lst = next) {
        next = lst->next;
        list_free_1(lst);
    }

    pthread_mutex_unlock(&freelist_lock);
}

X_EXTERN x_list *
X_PFX(list_prepend) (x_list * lst, void *data) {
    x_list *node;

    pthread_mutex_lock(&freelist_lock);

    if (freelist == NULL) {
        x_list_block *b;
        int i;

        b = malloc(sizeof(x_list_block));
        assert(b != NULL);

        for (i = 0; i < NODES_PER_BLOCK - 1; i++)
            b->l[i].next = &(b->l[i + 1]);
        b->l[i].next = NULL;

        freelist = b->l;
    }

    node = freelist;
    freelist = node->next;

    pthread_mutex_unlock(&freelist_lock);

    node->next = lst;
    node->data = data;

    return node;
}

X_EXTERN x_list *
X_PFX(list_append) (x_list * lst, void *data) {
    x_list *head = lst;

    if (lst == NULL)
        return X_PFX(list_prepend) (NULL, data);

    while (lst->next != NULL)
        lst = lst->next;

    lst->next = X_PFX(list_prepend) (NULL, data);

    return head;
}

X_EXTERN x_list *
X_PFX(list_reverse) (x_list * lst) {
    x_list *head = NULL, *next;

    while (lst != NULL)
    {
        next = lst->next;
        lst->next = head;
        head = lst;
        lst = next;
    }

    return head;
}

X_EXTERN x_list *
X_PFX(list_find) (x_list * lst, void *data) {
    for (; lst != NULL; lst = lst->next) {
        if (lst->data == data)
            return lst;
    }

    return NULL;
}

X_EXTERN x_list *
X_PFX(list_nth) (x_list * lst, int n) {
    while (n-- > 0 && lst != NULL)
        lst = lst->next;

    return lst;
}

X_EXTERN x_list *
X_PFX(list_pop) (x_list * lst, void **data_ret) {
    void *data = NULL;

    if (lst != NULL) {
        x_list *tem = lst;
        data = lst->data;
        lst = lst->next;
        X_PFX(list_free_1) (tem);
    }

    if (data_ret != NULL)
        *data_ret = data;

    return lst;
}

X_EXTERN x_list *
X_PFX(list_filter) (x_list * lst,
                    int (*pred)(void *item, void *data), void *data) {
    x_list *ret = NULL, *node;

    for (node = lst; node != NULL; node = node->next) {
        if ((*pred)(node->data, data))
            ret = X_PFX(list_prepend) (ret, node->data);
    }

    return X_PFX(list_reverse) (ret);
}

X_EXTERN x_list *
X_PFX(list_map) (x_list * lst,
                 void *(*fun)(void *item, void *data), void *data) {
    x_list *ret = NULL, *node;

    for (node = lst; node != NULL; node = node->next) {
        X_PFX(list_prepend) (ret, fun(node->data, data));
    }

    return X_PFX(list_reverse) (ret);
}

X_EXTERN x_list *
X_PFX(list_copy) (x_list * lst) {
    x_list *copy = NULL;

    for (; lst != NULL; lst = lst->next) {
        copy = X_PFX(list_prepend) (copy, lst->data);
    }

    return X_PFX(list_reverse) (copy);
}

X_EXTERN x_list *
X_PFX(list_remove) (x_list * lst, void *data) {
    x_list **ptr, *node;

    for (ptr = &lst; *ptr != NULL;) {
        node = *ptr;

        if (node->data == data) {
            *ptr = node->next;
            X_PFX(list_free_1) (node);
        }
        else
            ptr = &((*ptr)->next);
    }

    return lst;
}

X_EXTERN unsigned int
X_PFX(list_length) (x_list * lst) {
    unsigned int n;

    n = 0;
    for (; lst != NULL; lst = lst->next)
        n++;

    return n;
}

X_EXTERN void
X_PFX(list_foreach) (x_list * lst,
                     void (*fun)(void *data, void *user_data),
                     void *user_data) {
    for (; lst != NULL; lst = lst->next) {
        (*fun)(lst->data, user_data);
    }
}

static x_list *
list_sort_1(x_list *lst, int length,
            int (*less)(const void *, const void *))
{
    x_list *mid, *ptr;
    x_list *out_head, *out;
    int mid_point, i;

    /* This is a standard (stable) list merge sort */

    if (length < 2)
        return lst;

    /* Calculate the halfway point. Split the list into two sub-lists. */

    mid_point = length / 2;
    ptr = lst;
    for (i = mid_point - 1; i > 0; i--)
        ptr = ptr->next;
    mid = ptr->next;
    ptr->next = NULL;

    /* Sort each sub-list. */

    lst = list_sort_1(lst, mid_point, less);
    mid = list_sort_1(mid, length - mid_point, less);

    /* Then merge them back together. */

    assert(lst != NULL);
    assert(mid != NULL);

    if ((*less)(mid->data, lst->data))
        out = out_head = mid, mid = mid->next;
    else
        out = out_head = lst, lst = lst->next;

    while (lst != NULL && mid != NULL)
    {
        if ((*less)(mid->data, lst->data))
            out = out->next = mid, mid = mid->next;
        else
            out = out->next = lst, lst = lst->next;
    }

    if (lst != NULL)
        out->next = lst;
    else
        out->next = mid;

    return out_head;
}

X_EXTERN x_list *
X_PFX(list_sort) (x_list * lst, int (*less)(const void *, const void *)) {
    int length;

    length = X_PFX(list_length) (lst);

    return list_sort_1(lst, length, less);
}
