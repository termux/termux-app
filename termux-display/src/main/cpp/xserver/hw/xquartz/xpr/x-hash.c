/* x-hash.c - basic hash tables
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

#include "x-hash.h"
#include "x-list.h"
#include <stdlib.h>
#include <assert.h>

#define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))

struct x_hash_table_struct {
    unsigned int bucket_index;
    unsigned int total_keys;
    x_list **buckets;

    x_hash_fun *hash_key;
    x_compare_fun *compare_keys;
    x_destroy_fun *destroy_key;
    x_destroy_fun *destroy_value;
};

#define ITEM_NEW(k, v) X_PFX(list_prepend) ((x_list *)(k), v)
#define ITEM_FREE(i)   X_PFX(list_free_1) (i)
#define ITEM_KEY(i)    ((void *)(i)->next)
#define ITEM_VALUE(i)  ((i)->data)

#define SPLIT_THRESHOLD_FACTOR 2

/* http://planetmath.org/?op=getobj&from=objects&name=GoodHashTablePrimes */
static const unsigned int bucket_sizes[] = {
    29,       53,        97,        193,        389,        769,       1543,
    3079,     6151, 12289, 24593, 49157,
    98317,    196613,   393241,    786433,    1572869,   3145739,   6291469,
    12582917,
    25165843, 50331653, 100663319, 201326611, 402653189, 805306457,
    1610612741
};

static inline unsigned int
hash_table_total_buckets(x_hash_table *h)
{
    return bucket_sizes[h->bucket_index];
}

static inline void
hash_table_destroy_item(x_hash_table *h, void *k, void *v)
{
    if (h->destroy_key != 0)
        (*h->destroy_key)(k);

    if (h->destroy_value != 0)
        (*h->destroy_value)(v);
}

static inline size_t
hash_table_hash_key(x_hash_table *h, void *k)
{
    if (h->hash_key != 0)
        return (*h->hash_key)(k);
    else
        return (size_t)k;
}

static inline int
hash_table_compare_keys(x_hash_table *h, void *k1, void *k2)
{
    if (h->compare_keys == 0)
        return k1 == k2;
    else
        return (*h->compare_keys)(k1, k2) == 0;
}

static void
hash_table_split(x_hash_table *h)
{
    x_list **new, **old;
    x_list *node, *item, *next;
    int new_size, old_size;
    size_t b;
    int i;

    if (h->bucket_index == ARRAY_SIZE(bucket_sizes) - 1)
        return;

    old_size = hash_table_total_buckets(h);
    old = h->buckets;

    h->bucket_index++;

    new_size = hash_table_total_buckets(h);
    new = calloc(new_size, sizeof(x_list *));

    if (new == 0) {
        h->bucket_index--;
        return;
    }

    for (i = 0; i < old_size; i++) {
        for (node = old[i]; node != 0; node = next) {
            next = node->next;
            item = node->data;

            b = hash_table_hash_key(h, ITEM_KEY(item)) % new_size;

            node->next = new[b];
            new[b] = node;
        }
    }

    h->buckets = new;
    free(old);
}

X_EXTERN x_hash_table *
X_PFX(hash_table_new) (x_hash_fun * hash,
                       x_compare_fun * compare,
                       x_destroy_fun * key_destroy,
                       x_destroy_fun * value_destroy) {
    x_hash_table *h;

    h = calloc(1, sizeof(x_hash_table));
    if (h == 0)
        return 0;

    h->bucket_index = 0;
    h->buckets = calloc(hash_table_total_buckets(h), sizeof(x_list *));

    if (h->buckets == 0) {
        free(h);
        return 0;
    }

    h->hash_key = hash;
    h->compare_keys = compare;
    h->destroy_key = key_destroy;
    h->destroy_value = value_destroy;

    return h;
}

X_EXTERN void
X_PFX(hash_table_free) (x_hash_table * h) {
    int n, i;
    x_list *node, *item;

    assert(h != NULL);

    n = hash_table_total_buckets(h);

    for (i = 0; i < n; i++) {
        for (node = h->buckets[i]; node != 0; node = node->next) {
            item = node->data;
            hash_table_destroy_item(h, ITEM_KEY(item), ITEM_VALUE(item));
            ITEM_FREE(item);
        }
        X_PFX(list_free) (h->buckets[i]);
    }

    free(h->buckets);
    free(h);
}

X_EXTERN unsigned int
X_PFX(hash_table_size) (x_hash_table * h) {
    assert(h != NULL);

    return h->total_keys;
}

static void
hash_table_modify(x_hash_table *h, void *k, void *v, int replace)
{
    size_t hash_value;
    x_list *node, *item;

    assert(h != NULL);

    hash_value = hash_table_hash_key(h, k);

    for (node = h->buckets[hash_value % hash_table_total_buckets(h)];
         node != 0; node = node->next) {
        item = node->data;

        if (hash_table_compare_keys(h, ITEM_KEY(item), k)) {
            if (replace) {
                hash_table_destroy_item(h, ITEM_KEY(item),
                                        ITEM_VALUE(item));
                item->next = k;
                ITEM_VALUE(item) = v;
            }
            else {
                hash_table_destroy_item(h, k, ITEM_VALUE(item));
                ITEM_VALUE(item) = v;
            }
            return;
        }
    }

    /* Key isn't already in the table. Insert it. */

    if (h->total_keys + 1
        > hash_table_total_buckets(h) * SPLIT_THRESHOLD_FACTOR) {
        hash_table_split(h);
    }

    hash_value = hash_value % hash_table_total_buckets(h);
    h->buckets[hash_value] = X_PFX(list_prepend) (h->buckets[hash_value],
                                                  ITEM_NEW(k, v));
    h->total_keys++;
}

X_EXTERN void
X_PFX(hash_table_insert) (x_hash_table * h, void *k, void *v) {
    hash_table_modify(h, k, v, 0);
}

X_EXTERN void
X_PFX(hash_table_replace) (x_hash_table * h, void *k, void *v) {
    hash_table_modify(h, k, v, 1);
}

X_EXTERN void
X_PFX(hash_table_remove) (x_hash_table * h, void *k) {
    size_t hash_value;
    x_list **ptr, *item;

    assert(h != NULL);

    hash_value = hash_table_hash_key(h, k);

    for (ptr = &h->buckets[hash_value % hash_table_total_buckets(h)];
         *ptr != 0; ptr = &((*ptr)->next)) {
        item = (*ptr)->data;

        if (hash_table_compare_keys(h, ITEM_KEY(item), k)) {
            hash_table_destroy_item(h, ITEM_KEY(item), ITEM_VALUE(item));
            ITEM_FREE(item);
            item = *ptr;
            *ptr = item->next;
            X_PFX(list_free_1) (item);
            h->total_keys--;
            return;
        }
    }
}

X_EXTERN void *
X_PFX(hash_table_lookup) (x_hash_table * h, void *k, void **k_ret) {
    size_t hash_value;
    x_list *node, *item;

    assert(h != NULL);

    hash_value = hash_table_hash_key(h, k);

    for (node = h->buckets[hash_value % hash_table_total_buckets(h)];
         node != 0; node = node->next) {
        item = node->data;

        if (hash_table_compare_keys(h, ITEM_KEY(item), k)) {
            if (k_ret != 0)
                *k_ret = ITEM_KEY(item);

            return ITEM_VALUE(item);
        }
    }

    if (k_ret != 0)
        *k_ret = 0;

    return 0;
}

X_EXTERN void
X_PFX(hash_table_foreach) (x_hash_table * h,
                           x_hash_foreach_fun * fun, void *data) {
    int i, n;
    x_list *node, *item;

    assert(h != NULL);

    n = hash_table_total_buckets(h);

    for (i = 0; i < n; i++) {
        for (node = h->buckets[i]; node != 0; node = node->next) {
            item = node->data;
            (*fun)(ITEM_KEY(item), ITEM_VALUE(item), data);
        }
    }
}
