/**
 * Copyright © 2011 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xlib.h>
#include <list.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "tests-common.h"

struct parent {
    int a;
    struct xorg_list children;
    int b;
};

struct child {
    int foo;
    int bar;
    struct xorg_list node;
};

static void
test_xorg_list_init(void)
{
    struct parent parent, tmp;

    memset(&parent, 0, sizeof(parent));
    parent.a = 0xa5a5a5;
    parent.b = ~0xa5a5a5;

    tmp = parent;

    xorg_list_init(&parent.children);

    /* test we haven't touched anything else. */
    assert(parent.a == tmp.a);
    assert(parent.b == tmp.b);

    assert(xorg_list_is_empty(&parent.children));
}

static void
test_xorg_list_add(void)
{
    struct parent parent = { 0 };
    struct child child[3];
    struct child *c;

    xorg_list_init(&parent.children);

    xorg_list_add(&child[0].node, &parent.children);
    assert(!xorg_list_is_empty(&parent.children));

    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);

    /* note: xorg_list_add prepends */
    xorg_list_add(&child[1].node, &parent.children);
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[1], sizeof(struct child)) == 0);

    xorg_list_add(&child[2].node, &parent.children);
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[2], sizeof(struct child)) == 0);
};

static void
test_xorg_list_append(void)
{
    struct parent parent = { 0 };
    struct child child[3];
    struct child *c;
    int i;

    xorg_list_init(&parent.children);

    xorg_list_append(&child[0].node, &parent.children);
    assert(!xorg_list_is_empty(&parent.children));

    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);
    c = xorg_list_last_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);

    xorg_list_append(&child[1].node, &parent.children);
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);
    c = xorg_list_last_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[1], sizeof(struct child)) == 0);

    xorg_list_append(&child[2].node, &parent.children);
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);
    c = xorg_list_last_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[2], sizeof(struct child)) == 0);

    i = 0;
    xorg_list_for_each_entry(c, &parent.children, node) {
        assert(memcmp(c, &child[i++], sizeof(struct child)) == 0);
    }
};

static void
test_xorg_list_del(void)
{
    struct parent parent = { 0 };
    struct child child[2];
    struct child *c;

    xorg_list_init(&parent.children);

    xorg_list_add(&child[0].node, &parent.children);
    assert(!xorg_list_is_empty(&parent.children));

    xorg_list_del(&parent.children);
    assert(xorg_list_is_empty(&parent.children));

    xorg_list_add(&child[0].node, &parent.children);
    xorg_list_del(&child[0].node);
    assert(xorg_list_is_empty(&parent.children));

    xorg_list_add(&child[0].node, &parent.children);
    xorg_list_add(&child[1].node, &parent.children);

    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[1], sizeof(struct child)) == 0);

    /* delete first node */
    xorg_list_del(&child[1].node);
    assert(!xorg_list_is_empty(&parent.children));
    assert(xorg_list_is_empty(&child[1].node));
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[0], sizeof(struct child)) == 0);

    /* delete last node */
    xorg_list_add(&child[1].node, &parent.children);
    xorg_list_del(&child[0].node);
    c = xorg_list_first_entry(&parent.children, struct child, node);

    assert(memcmp(c, &child[1], sizeof(struct child)) == 0);

    /* delete list head */
    xorg_list_add(&child[0].node, &parent.children);
    xorg_list_del(&parent.children);
    assert(xorg_list_is_empty(&parent.children));
    assert(!xorg_list_is_empty(&child[0].node));
    assert(!xorg_list_is_empty(&child[1].node));
}

static void
test_xorg_list_for_each(void)
{
    struct parent parent = { 0 };
    struct child child[3];
    struct child *c;
    int i = 0;

    xorg_list_init(&parent.children);

    xorg_list_add(&child[2].node, &parent.children);
    xorg_list_add(&child[1].node, &parent.children);
    xorg_list_add(&child[0].node, &parent.children);

    xorg_list_for_each_entry(c, &parent.children, node) {
        assert(memcmp(c, &child[i], sizeof(struct child)) == 0);
        i++;
    }

    /* foreach on empty list */
    xorg_list_del(&parent.children);
    assert(xorg_list_is_empty(&parent.children));

    xorg_list_for_each_entry(c, &parent.children, node) {
        assert(0);              /* we must not get here */
    }
}

struct foo {
    char a;
    struct foo *next;
    char b;
};

static void
test_nt_list_init(void)
{
    struct foo foo;

    foo.a = 10;
    foo.b = 20;
    nt_list_init(&foo, next);

    assert(foo.a == 10);
    assert(foo.b == 20);
    assert(foo.next == NULL);
    assert(nt_list_next(&foo, next) == NULL);
}

static void
test_nt_list_append(void)
{
    int i;
    struct foo *foo = calloc(10, sizeof(struct foo));
    struct foo *item;

    for (item = foo, i = 1; i <= 10; i++, item++) {
        item->a = i;
        item->b = i * 2;
        nt_list_init(item, next);

        if (item != foo)
            nt_list_append(item, foo, struct foo, next);
    }

    /* Test using nt_list_next */
    for (item = foo, i = 1; i <= 10; i++, item = nt_list_next(item, next)) {
        assert(item->a == i);
        assert(item->b == i * 2);
    }

    /* Test using nt_list_for_each_entry */
    i = 1;
    nt_list_for_each_entry(item, foo, next) {
        assert(item->a == i);
        assert(item->b == i * 2);
        i++;
    }
    assert(i == 11);
}

static void
test_nt_list_insert(void)
{
    int i;
    struct foo *foo = calloc(10, sizeof(struct foo));
    struct foo *item;

    foo->a = 1;
    foo->b = 2;
    nt_list_init(foo, next);

    for (item = &foo[1], i = 10; i > 1; i--, item++) {
        item->a = i;
        item->b = i * 2;
        nt_list_init(item, next);
        nt_list_insert(item, foo, struct foo, next);
    }

    /* Test using nt_list_next */
    for (item = foo, i = 1; i <= 10; i++, item = nt_list_next(item, next)) {
        assert(item->a == i);
        assert(item->b == i * 2);
    }

    /* Test using nt_list_for_each_entry */
    i = 1;
    nt_list_for_each_entry(item, foo, next) {
        assert(item->a == i);
        assert(item->b == i * 2);
        i++;
    }
    assert(i == 11);
}

static void
test_nt_list_delete(void)
{
    int i = 1;
    struct foo *list = calloc(10, sizeof(struct foo));
    struct foo *foo = list;
    struct foo *item, *tmp;
    struct foo *empty_list = foo;

    nt_list_init(empty_list, next);
    nt_list_del(empty_list, empty_list, struct foo, next);

    assert(!empty_list);

    for (item = foo, i = 1; i <= 10; i++, item++) {
        item->a = i;
        item->b = i * 2;
        nt_list_init(item, next);

        if (item != foo)
            nt_list_append(item, foo, struct foo, next);
    }

    i = 0;
    nt_list_for_each_entry(item, foo, next) {
        i++;
    }
    assert(i == 10);

    /* delete last item */
    nt_list_del(&foo[9], foo, struct foo, next);

    i = 0;
    nt_list_for_each_entry(item, foo, next) {
        assert(item->a != 10);  /* element 10 is gone now */
        i++;
    }
    assert(i == 9);             /* 9 elements left */

    /* delete second item */
    nt_list_del(foo->next, foo, struct foo, next);

    assert(foo->next->a == 3);

    i = 0;
    nt_list_for_each_entry(item, foo, next) {
        assert(item->a != 10);  /* element 10 is gone now */
        assert(item->a != 2);   /* element 2 is gone now */
        i++;
    }
    assert(i == 8);             /* 9 elements left */

    item = foo;
    /* delete first item */
    nt_list_del(foo, foo, struct foo, next);

    assert(item != foo);
    assert(item->next == NULL);
    assert(foo->a == 3);
    assert(foo->next->a == 4);

    nt_list_for_each_entry_safe(item, tmp, foo, next) {
        nt_list_del(item, foo, struct foo, next);
    }

    assert(!foo);
    assert(!item);

    free(list);
}

int
list_test(void)
{
    test_xorg_list_init();
    test_xorg_list_add();
    test_xorg_list_append();
    test_xorg_list_del();
    test_xorg_list_for_each();

    test_nt_list_init();
    test_nt_list_append();
    test_nt_list_insert();
    test_nt_list_delete();

    return 0;
}
