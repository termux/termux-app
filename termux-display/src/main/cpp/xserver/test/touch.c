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

#include <stdint.h>
#include "inputstr.h"
#include "assert.h"
#include "scrnintstr.h"

#include "tests-common.h"

static void
touch_grow_queue(void)
{
    DeviceIntRec dev;
    ValuatorClassRec val;
    TouchClassRec touch;
    size_t size, new_size;
    int i;

    memset(&dev, 0, sizeof(dev));
    dev.name = xnfstrdup("test device");
    dev.id = 2;
    dev.valuator = &val;
    val.numAxes = 5;
    dev.touch = &touch;
    inputInfo.devices = &dev;

    size = 5;

    dev.last.num_touches = size;
    dev.last.touches = calloc(dev.last.num_touches, sizeof(*dev.last.touches));
    assert(dev.last.touches);
    for (i = 0; i < size; i++) {
        dev.last.touches[i].active = TRUE;
        dev.last.touches[i].ddx_id = i;
        dev.last.touches[i].client_id = i * 2;
    }

    /* no more space, should've reallocated and succeeded */
    assert(TouchBeginDDXTouch(&dev, 1234) != NULL);

    new_size = size + size / 2 + 1;
    assert(dev.last.num_touches == new_size);

    /* make sure we haven't touched those */
    for (i = 0; i < size; i++) {
        DDXTouchPointInfoPtr t = &dev.last.touches[i];

        assert(t->active == TRUE);
        assert(t->ddx_id == i);
        assert(t->client_id == i * 2);
    }

    assert(dev.last.touches[size].active == TRUE);
    assert(dev.last.touches[size].ddx_id == 1234);
    assert(dev.last.touches[size].client_id == 1);

    /* make sure those are zero-initialized */
    for (i = size + 1; i < new_size; i++) {
        DDXTouchPointInfoPtr t = &dev.last.touches[i];

        assert(t->active == FALSE);
        assert(t->client_id == 0);
        assert(t->ddx_id == 0);
    }

    free(dev.name);
}

static void
touch_find_ddxid(void)
{
    DeviceIntRec dev;
    DDXTouchPointInfoPtr ti, ti2;
    ValuatorClassRec val;
    TouchClassRec touch;
    int size = 5;
    int i;

    memset(&dev, 0, sizeof(dev));
    dev.name = xnfstrdup("test device");
    dev.id = 2;
    dev.valuator = &val;
    val.numAxes = 5;
    dev.touch = &touch;
    dev.last.num_touches = size;
    dev.last.touches = calloc(dev.last.num_touches, sizeof(*dev.last.touches));
    inputInfo.devices = &dev;
    assert(dev.last.touches);

    dev.last.touches[0].active = TRUE;
    dev.last.touches[0].ddx_id = 10;
    dev.last.touches[0].client_id = 20;

    /* existing */
    ti = TouchFindByDDXID(&dev, 10, FALSE);
    assert(ti == &dev.last.touches[0]);

    /* non-existing */
    ti = TouchFindByDDXID(&dev, 20, FALSE);
    assert(ti == NULL);

    /* Non-active */
    dev.last.touches[0].active = FALSE;
    ti = TouchFindByDDXID(&dev, 10, FALSE);
    assert(ti == NULL);

    /* create on number 2 */
    dev.last.touches[0].active = TRUE;

    ti = TouchFindByDDXID(&dev, 20, TRUE);
    assert(ti == &dev.last.touches[1]);
    assert(ti->active);
    assert(ti->ddx_id == 20);

    /* set all to active */
    for (i = 0; i < size; i++)
        dev.last.touches[i].active = TRUE;

    /* Try to create more, succeed */
    ti = TouchFindByDDXID(&dev, 30, TRUE);
    assert(ti != NULL);
    ti2 = TouchFindByDDXID(&dev, 30, TRUE);
    assert(ti == ti2);
    /* make sure we have resized */
    assert(dev.last.num_touches == 8); /* EQ grows from 5 to 8 */

    /* stop one touchpoint, try to create, succeed */
    dev.last.touches[2].active = FALSE;
    ti = TouchFindByDDXID(&dev, 35, TRUE);
    assert(ti == &dev.last.touches[2]);
    ti = TouchFindByDDXID(&dev, 40, TRUE);
    assert(ti == &dev.last.touches[size+1]);

    free(dev.name);
}

static void
touch_begin_ddxtouch(void)
{
    DeviceIntRec dev;
    DDXTouchPointInfoPtr ti;
    ValuatorClassRec val;
    TouchClassRec touch;
    int ddx_id = 123;
    unsigned int last_client_id = 0;
    int size = 5;

    memset(&dev, 0, sizeof(dev));
    dev.name = xnfstrdup("test device");
    dev.id = 2;
    dev.valuator = &val;
    val.numAxes = 5;
    touch.mode = XIDirectTouch;
    dev.touch = &touch;
    dev.last.num_touches = size;
    dev.last.touches = calloc(dev.last.num_touches, sizeof(*dev.last.touches));
    inputInfo.devices = &dev;
    assert(dev.last.touches);

    ti = TouchBeginDDXTouch(&dev, ddx_id);
    assert(ti);
    assert(ti->ddx_id == ddx_id);
    /* client_id == ddx_id can happen in real life, but not in this test */
    assert(ti->client_id != ddx_id);
    assert(ti->active);
    assert(ti->client_id > last_client_id);
    assert(ti->emulate_pointer);
    last_client_id = ti->client_id;

    ddx_id += 10;
    ti = TouchBeginDDXTouch(&dev, ddx_id);
    assert(ti);
    assert(ti->ddx_id == ddx_id);
    /* client_id == ddx_id can happen in real life, but not in this test */
    assert(ti->client_id != ddx_id);
    assert(ti->active);
    assert(ti->client_id > last_client_id);
    assert(!ti->emulate_pointer);
    last_client_id = ti->client_id;

    free(dev.name);
}

static void
touch_begin_touch(void)
{
    DeviceIntRec dev;
    TouchClassRec touch;
    ValuatorClassRec val;
    TouchPointInfoPtr ti;
    int touchid = 12434;
    int sourceid = 23;
    SpriteInfoRec sprite;
    ScreenRec screen;

    screenInfo.screens[0] = &screen;

    memset(&dev, 0, sizeof(dev));
    dev.name = xnfstrdup("test device");
    dev.id = 2;

    memset(&sprite, 0, sizeof(sprite));
    dev.spriteInfo = &sprite;

    memset(&touch, 0, sizeof(touch));
    touch.num_touches = 0;

    memset(&val, 0, sizeof(val));
    dev.valuator = &val;
    val.numAxes = 2;

    ti = TouchBeginTouch(&dev, sourceid, touchid, TRUE);
    assert(!ti);

    dev.touch = &touch;
    ti = TouchBeginTouch(&dev, sourceid, touchid, TRUE);
    assert(ti);
    assert(ti->client_id == touchid);
    assert(ti->active);
    assert(ti->sourceid == sourceid);
    assert(ti->emulate_pointer);

    assert(touch.num_touches == 1);

    free(dev.name);
}

static void
touch_init(void)
{
    DeviceIntRec dev;
    Atom labels[2] = { 0 };
    int rc;
    SpriteInfoRec sprite;
    ScreenRec screen;

    screenInfo.screens[0] = &screen;

    memset(&dev, 0, sizeof(dev));
    dev.name = xnfstrdup("test device");

    memset(&sprite, 0, sizeof(sprite));
    dev.spriteInfo = &sprite;

    InitAtoms();
    rc = InitTouchClassDeviceStruct(&dev, 1, XIDirectTouch, 2);
    assert(rc == FALSE);

    InitValuatorClassDeviceStruct(&dev, 2, labels, 10, Absolute);
    rc = InitTouchClassDeviceStruct(&dev, 1, XIDirectTouch, 2);
    assert(rc == TRUE);
    assert(dev.touch);

    free(dev.name);
}

int
touch_test(void)
{
    printf("touch_test: start...\n");
    touch_grow_queue();
    touch_find_ddxid();
    touch_begin_ddxtouch();
    touch_init();
    touch_begin_touch();

    printf("touch_test: exiting successfully\n");
    return 0;
}
