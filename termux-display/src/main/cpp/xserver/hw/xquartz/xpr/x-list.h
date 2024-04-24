/* x-list.h -- simple list type
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

#ifndef X_LIST_H
#define X_LIST_H 1

/* This is just a cons. */

typedef struct x_list_struct x_list;

struct x_list_struct {
    void *data;
    x_list *next;
};

#ifndef X_PFX
#define X_PFX(x) x_ ## x
#endif

#ifndef X_EXTERN
#define X_EXTERN __private_extern__
#endif

X_EXTERN void X_PFX(list_free_1) (x_list * node);
X_EXTERN x_list *X_PFX(list_prepend) (x_list * lst, void *data);

X_EXTERN x_list *X_PFX(list_append) (x_list * lst, void *data);
X_EXTERN x_list *X_PFX(list_remove) (x_list * lst, void *data);
X_EXTERN void X_PFX(list_free) (x_list * lst);
X_EXTERN x_list *X_PFX(list_pop) (x_list * lst, void **data_ret);

X_EXTERN x_list *X_PFX(list_copy) (x_list * lst);
X_EXTERN x_list *X_PFX(list_reverse) (x_list * lst);
X_EXTERN x_list *X_PFX(list_find) (x_list * lst, void *data);
X_EXTERN x_list *X_PFX(list_nth) (x_list * lst, int n);
X_EXTERN x_list *X_PFX(list_filter) (x_list * src,
                                     int (*pred)(void *item, void *data),
                                     void *data);
X_EXTERN x_list *X_PFX(list_map) (x_list * src,
                                  void *(*fun)(void *item, void *data),
                                  void *data);

X_EXTERN unsigned int X_PFX(list_length) (x_list * lst);
X_EXTERN void X_PFX(list_foreach) (x_list * lst, void (*fun)
                                   (void *data, void *user_data),
                                   void *user_data);

X_EXTERN x_list *X_PFX(list_sort) (x_list * lst,
                                   int (*less)(const void *, const void *));

#endif /* X_LIST_H */
