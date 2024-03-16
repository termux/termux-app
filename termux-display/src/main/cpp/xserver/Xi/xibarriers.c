/*
 * Copyright 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2002 Keith Packard
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

#include "xibarriers.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixevents.h"
#include "servermd.h"
#include "mipointer.h"
#include "inputstr.h"
#include "windowstr.h"
#include "xace.h"
#include "list.h"
#include "exglobals.h"
#include "eventstr.h"
#include "mi.h"

RESTYPE PointerBarrierType;

static DevPrivateKeyRec BarrierScreenPrivateKeyRec;

#define BarrierScreenPrivateKey (&BarrierScreenPrivateKeyRec)

typedef struct PointerBarrierClient *PointerBarrierClientPtr;

struct PointerBarrierDevice {
    struct xorg_list entry;
    int deviceid;
    Time last_timestamp;
    int barrier_event_id;
    int release_event_id;
    Bool hit;
    Bool seen;
};

struct PointerBarrierClient {
    XID id;
    ScreenPtr screen;
    Window window;
    struct PointerBarrier barrier;
    struct xorg_list entry;
    /* num_devices/device_ids are devices the barrier applies to */
    int num_devices;
    int *device_ids; /* num_devices */

    /* per_device keeps track of devices actually blocked by barriers */
    struct xorg_list per_device;
};

typedef struct _BarrierScreen {
    struct xorg_list barriers;
} BarrierScreenRec, *BarrierScreenPtr;

#define GetBarrierScreen(s) ((BarrierScreenPtr)dixLookupPrivate(&(s)->devPrivates, BarrierScreenPrivateKey))
#define GetBarrierScreenIfSet(s) GetBarrierScreen(s)
#define SetBarrierScreen(s,p) dixSetPrivate(&(s)->devPrivates, BarrierScreenPrivateKey, p)

static struct PointerBarrierDevice *AllocBarrierDevice(void)
{
    struct PointerBarrierDevice *pbd = NULL;

    pbd = malloc(sizeof(struct PointerBarrierDevice));
    if (!pbd)
        return NULL;

    pbd->deviceid = -1; /* must be set by caller */
    pbd->barrier_event_id = 1;
    pbd->release_event_id = 0;
    pbd->hit = FALSE;
    pbd->seen = FALSE;
    xorg_list_init(&pbd->entry);

    return pbd;
}

static void FreePointerBarrierClient(struct PointerBarrierClient *c)
{
    struct PointerBarrierDevice *pbd = NULL, *tmp = NULL;

    xorg_list_for_each_entry_safe(pbd, tmp, &c->per_device, entry) {
        free(pbd);
    }
    free(c);
}

static struct PointerBarrierDevice *GetBarrierDevice(struct PointerBarrierClient *c, int deviceid)
{
    struct PointerBarrierDevice *pbd = NULL;

    xorg_list_for_each_entry(pbd, &c->per_device, entry) {
        if (pbd->deviceid == deviceid)
            break;
    }

    BUG_WARN(!pbd);
    return pbd;
}

static BOOL
barrier_is_horizontal(const struct PointerBarrier *barrier)
{
    return barrier->y1 == barrier->y2;
}

static BOOL
barrier_is_vertical(const struct PointerBarrier *barrier)
{
    return barrier->x1 == barrier->x2;
}

/**
 * @return The set of barrier movement directions the movement vector
 * x1/y1 → x2/y2 represents.
 */
int
barrier_get_direction(int x1, int y1, int x2, int y2)
{
    int direction = 0;

    /* which way are we trying to go */
    if (x2 > x1)
        direction |= BarrierPositiveX;
    if (x2 < x1)
        direction |= BarrierNegativeX;
    if (y2 > y1)
        direction |= BarrierPositiveY;
    if (y2 < y1)
        direction |= BarrierNegativeY;

    return direction;
}

/**
 * Test if the barrier may block movement in the direction defined by
 * x1/y1 → x2/y2. This function only tests whether the directions could be
 * blocked, it does not test if the barrier actually blocks the movement.
 *
 * @return TRUE if the barrier blocks the direction of movement or FALSE
 * otherwise.
 */
BOOL
barrier_is_blocking_direction(const struct PointerBarrier * barrier,
                              int direction)
{
    /* Barriers define which way is ok, not which way is blocking */
    return (barrier->directions & direction) != direction;
}

static BOOL
inside_segment(int v, int v1, int v2)
{
    if (v1 < 0 && v2 < 0) /* line */
        return TRUE;
    else if (v1 < 0)      /* ray */
        return v <= v2;
    else if (v2 < 0)      /* ray */
        return v >= v1;
    else                  /* line segment */
        return v >= v1 && v <= v2;
}

#define T(v, a, b) (((float)v) - (a)) / ((b) - (a))
#define F(t, a, b) ((t) * ((a) - (b)) + (a))

/**
 * Test if the movement vector x1/y1 → x2/y2 is intersecting with the
 * barrier. A movement vector with the startpoint or endpoint adjacent to
 * the barrier itself counts as intersecting.
 *
 * @param x1 X start coordinate of movement vector
 * @param y1 Y start coordinate of movement vector
 * @param x2 X end coordinate of movement vector
 * @param y2 Y end coordinate of movement vector
 * @param[out] distance The distance between the start point and the
 * intersection with the barrier (if applicable).
 * @return TRUE if the barrier intersects with the given vector
 */
BOOL
barrier_is_blocking(const struct PointerBarrier * barrier,
                    int x1, int y1, int x2, int y2, double *distance)
{
    if (barrier_is_vertical(barrier)) {
        float t, y;
        t = T(barrier->x1, x1, x2);
        if (t < 0 || t > 1)
            return FALSE;

        /* Edge case: moving away from barrier. */
        if (x2 > x1 && t == 0)
            return FALSE;

        y = F(t, y1, y2);
        if (!inside_segment(y, barrier->y1, barrier->y2))
            return FALSE;

        *distance = sqrt((pow(y - y1, 2) + pow(barrier->x1 - x1, 2)));
        return TRUE;
    }
    else {
        float t, x;
        t = T(barrier->y1, y1, y2);
        if (t < 0 || t > 1)
            return FALSE;

        /* Edge case: moving away from barrier. */
        if (y2 > y1 && t == 0)
            return FALSE;

        x = F(t, x1, x2);
        if (!inside_segment(x, barrier->x1, barrier->x2))
            return FALSE;

        *distance = sqrt((pow(x - x1, 2) + pow(barrier->y1 - y1, 2)));
        return TRUE;
    }
}

#define HIT_EDGE_EXTENTS 2
static BOOL
barrier_inside_hit_box(struct PointerBarrier *barrier, int x, int y)
{
    int x1, x2, y1, y2;
    int dir;

    x1 = barrier->x1;
    x2 = barrier->x2;
    y1 = barrier->y1;
    y2 = barrier->y2;
    dir = ~(barrier->directions);

    if (barrier_is_vertical(barrier)) {
        if (dir & BarrierPositiveX)
            x1 -= HIT_EDGE_EXTENTS;
        if (dir & BarrierNegativeX)
            x2 += HIT_EDGE_EXTENTS;
    }
    if (barrier_is_horizontal(barrier)) {
        if (dir & BarrierPositiveY)
            y1 -= HIT_EDGE_EXTENTS;
        if (dir & BarrierNegativeY)
            y2 += HIT_EDGE_EXTENTS;
    }

    return x >= x1 && x <= x2 && y >= y1 && y <= y2;
}

static BOOL
barrier_blocks_device(struct PointerBarrierClient *client,
                      DeviceIntPtr dev)
{
    int i;
    int master_id;

    /* Clients with no devices are treated as
     * if they specified XIAllDevices. */
    if (client->num_devices == 0)
        return TRUE;

    master_id = GetMaster(dev, POINTER_OR_FLOAT)->id;

    for (i = 0; i < client->num_devices; i++) {
        int device_id = client->device_ids[i];
        if (device_id == XIAllDevices ||
            device_id == XIAllMasterDevices ||
            device_id == master_id)
            return TRUE;
    }

    return FALSE;
}

/**
 * Find the nearest barrier client that is blocking movement from x1/y1 to x2/y2.
 *
 * @param dir Only barriers blocking movement in direction dir are checked
 * @param x1 X start coordinate of movement vector
 * @param y1 Y start coordinate of movement vector
 * @param x2 X end coordinate of movement vector
 * @param y2 Y end coordinate of movement vector
 * @return The barrier nearest to the movement origin that blocks this movement.
 */
static struct PointerBarrierClient *
barrier_find_nearest(BarrierScreenPtr cs, DeviceIntPtr dev,
                     int dir,
                     int x1, int y1, int x2, int y2)
{
    struct PointerBarrierClient *c, *nearest = NULL;
    double min_distance = INT_MAX;      /* can't get higher than that in X anyway */

    xorg_list_for_each_entry(c, &cs->barriers, entry) {
        struct PointerBarrier *b = &c->barrier;
        struct PointerBarrierDevice *pbd;
        double distance;

        pbd = GetBarrierDevice(c, dev->id);
        if (pbd->seen)
            continue;

        if (!barrier_is_blocking_direction(b, dir))
            continue;

        if (!barrier_blocks_device(c, dev))
            continue;

        if (barrier_is_blocking(b, x1, y1, x2, y2, &distance)) {
            if (min_distance > distance) {
                min_distance = distance;
                nearest = c;
            }
        }
    }

    return nearest;
}

/**
 * Clamp to the given barrier given the movement direction specified in dir.
 *
 * @param barrier The barrier to clamp to
 * @param dir The movement direction
 * @param[out] x The clamped x coordinate.
 * @param[out] y The clamped x coordinate.
 */
void
barrier_clamp_to_barrier(struct PointerBarrier *barrier, int dir, int *x,
                         int *y)
{
    if (barrier_is_vertical(barrier)) {
        if ((dir & BarrierNegativeX) & ~barrier->directions)
            *x = barrier->x1;
        if ((dir & BarrierPositiveX) & ~barrier->directions)
            *x = barrier->x1 - 1;
    }
    if (barrier_is_horizontal(barrier)) {
        if ((dir & BarrierNegativeY) & ~barrier->directions)
            *y = barrier->y1;
        if ((dir & BarrierPositiveY) & ~barrier->directions)
            *y = barrier->y1 - 1;
    }
}

void
input_constrain_cursor(DeviceIntPtr dev, ScreenPtr screen,
                       int current_x, int current_y,
                       int dest_x, int dest_y,
                       int *out_x, int *out_y,
                       int *nevents, InternalEvent* events)
{
    /* Clamped coordinates here refer to screen edge clamping. */
    BarrierScreenPtr cs = GetBarrierScreen(screen);
    int x = dest_x,
        y = dest_y;
    int dir;
    struct PointerBarrier *nearest = NULL;
    PointerBarrierClientPtr c;
    Time ms = GetTimeInMillis();
    BarrierEvent ev = {
        .header = ET_Internal,
        .type = 0,
        .length = sizeof (BarrierEvent),
        .time = ms,
        .deviceid = dev->id,
        .sourceid = dev->id,
        .dx = dest_x - current_x,
        .dy = dest_y - current_y,
        .root = screen->root->drawable.id,
    };
    InternalEvent *barrier_events = events;
    DeviceIntPtr master;

    if (nevents)
        *nevents = 0;

    if (xorg_list_is_empty(&cs->barriers) || IsFloating(dev))
        goto out;

    /**
     * This function is only called for slave devices, but pointer-barriers
     * are for master-devices only. Flip the device to the master here,
     * continue with that.
     */
    master = GetMaster(dev, MASTER_POINTER);

    /* How this works:
     * Given the origin and the movement vector, get the nearest barrier
     * to the origin that is blocking the movement.
     * Clamp to that barrier.
     * Then, check from the clamped intersection to the original
     * destination, again finding the nearest barrier and clamping.
     */
    dir = barrier_get_direction(current_x, current_y, x, y);

    while (dir != 0) {
        int new_sequence;
        struct PointerBarrierDevice *pbd;

        c = barrier_find_nearest(cs, master, dir, current_x, current_y, x, y);
        if (!c)
            break;

        nearest = &c->barrier;

        pbd = GetBarrierDevice(c, master->id);
        new_sequence = !pbd->hit;

        pbd->seen = TRUE;
        pbd->hit = TRUE;

        if (pbd->barrier_event_id == pbd->release_event_id)
            continue;

        ev.type = ET_BarrierHit;
        barrier_clamp_to_barrier(nearest, dir, &x, &y);

        if (barrier_is_vertical(nearest)) {
            dir &= ~(BarrierNegativeX | BarrierPositiveX);
            current_x = x;
        }
        else if (barrier_is_horizontal(nearest)) {
            dir &= ~(BarrierNegativeY | BarrierPositiveY);
            current_y = y;
        }

        ev.flags = 0;
        ev.event_id = pbd->barrier_event_id;
        ev.barrierid = c->id;

        ev.dt = new_sequence ? 0 : ms - pbd->last_timestamp;
        ev.window = c->window;
        pbd->last_timestamp = ms;

        /* root x/y is filled in later */

        barrier_events->barrier_event = ev;
        barrier_events++;
        *nevents += 1;
    }

    xorg_list_for_each_entry(c, &cs->barriers, entry) {
        struct PointerBarrierDevice *pbd;
        int flags = 0;

        pbd = GetBarrierDevice(c, master->id);
        pbd->seen = FALSE;
        if (!pbd->hit)
            continue;

        if (barrier_inside_hit_box(&c->barrier, x, y))
            continue;

        pbd->hit = FALSE;

        ev.type = ET_BarrierLeave;

        if (pbd->barrier_event_id == pbd->release_event_id)
            flags |= XIBarrierPointerReleased;

        ev.flags = flags;
        ev.event_id = pbd->barrier_event_id;
        ev.barrierid = c->id;

        ev.dt = ms - pbd->last_timestamp;
        ev.window = c->window;
        pbd->last_timestamp = ms;

        /* root x/y is filled in later */

        barrier_events->barrier_event = ev;
        barrier_events++;
        *nevents += 1;

        /* If we've left the hit box, this is the
         * start of a new event ID. */
        pbd->barrier_event_id++;
    }

 out:
    *out_x = x;
    *out_y = y;
}

static void
sort_min_max(INT16 *a, INT16 *b)
{
    INT16 A, B;
    if (*a < 0 || *b < 0)
        return;
    A = *a;
    B = *b;
    *a = min(A, B);
    *b = max(A, B);
}

static int
CreatePointerBarrierClient(ClientPtr client,
                           xXFixesCreatePointerBarrierReq * stuff,
                           PointerBarrierClientPtr *client_out)
{
    WindowPtr pWin;
    ScreenPtr screen;
    BarrierScreenPtr cs;
    int err;
    int size;
    int i;
    struct PointerBarrierClient *ret;
    CARD16 *in_devices;
    DeviceIntPtr dev;

    size = sizeof(*ret) + sizeof(DeviceIntPtr) * stuff->num_devices;
    ret = malloc(size);

    if (!ret) {
        return BadAlloc;
    }

    xorg_list_init(&ret->per_device);

    err = dixLookupWindow(&pWin, stuff->window, client, DixReadAccess);
    if (err != Success) {
        client->errorValue = stuff->window;
        goto error;
    }

    screen = pWin->drawable.pScreen;
    cs = GetBarrierScreen(screen);

    ret->screen = screen;
    ret->window = stuff->window;
    ret->num_devices = stuff->num_devices;
    if (ret->num_devices > 0)
        ret->device_ids = (int*)&ret[1];
    else
        ret->device_ids = NULL;

    in_devices = (CARD16 *) &stuff[1];
    for (i = 0; i < stuff->num_devices; i++) {
        int device_id = in_devices[i];
        DeviceIntPtr device;

        if ((err = dixLookupDevice (&device, device_id,
                                    client, DixReadAccess))) {
            client->errorValue = device_id;
            goto error;
        }

        if (!IsMaster (device)) {
            client->errorValue = device_id;
            err = BadDevice;
            goto error;
        }

        ret->device_ids[i] = device_id;
    }

    /* Alloc one per master pointer, they're the ones that can be blocked */
    xorg_list_init(&ret->per_device);
    nt_list_for_each_entry(dev, inputInfo.devices, next) {
        struct PointerBarrierDevice *pbd;

        if (dev->type != MASTER_POINTER)
            continue;

        pbd = AllocBarrierDevice();
        if (!pbd) {
            err = BadAlloc;
            goto error;
        }
        pbd->deviceid = dev->id;

        input_lock();
        xorg_list_add(&pbd->entry, &ret->per_device);
        input_unlock();
    }

    ret->id = stuff->barrier;
    ret->barrier.x1 = stuff->x1;
    ret->barrier.x2 = stuff->x2;
    ret->barrier.y1 = stuff->y1;
    ret->barrier.y2 = stuff->y2;
    sort_min_max(&ret->barrier.x1, &ret->barrier.x2);
    sort_min_max(&ret->barrier.y1, &ret->barrier.y2);
    ret->barrier.directions = stuff->directions & 0x0f;
    if (barrier_is_horizontal(&ret->barrier))
        ret->barrier.directions &= ~(BarrierPositiveX | BarrierNegativeX);
    if (barrier_is_vertical(&ret->barrier))
        ret->barrier.directions &= ~(BarrierPositiveY | BarrierNegativeY);
    input_lock();
    xorg_list_add(&ret->entry, &cs->barriers);
    input_unlock();

    *client_out = ret;
    return Success;

 error:
    *client_out = NULL;
    FreePointerBarrierClient(ret);
    return err;
}

static int
BarrierFreeBarrier(void *data, XID id)
{
    struct PointerBarrierClient *c;
    Time ms = GetTimeInMillis();
    DeviceIntPtr dev = NULL;
    ScreenPtr screen;

    c = container_of(data, struct PointerBarrierClient, barrier);
    screen = c->screen;

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        struct PointerBarrierDevice *pbd;
        int root_x, root_y;
        BarrierEvent ev = {
            .header = ET_Internal,
            .type = ET_BarrierLeave,
            .length = sizeof (BarrierEvent),
            .time = ms,
            /* .deviceid */
            .sourceid = 0,
            .barrierid = c->id,
            .window = c->window,
            .root = screen->root->drawable.id,
            .dx = 0,
            .dy = 0,
            /* .root_x */
            /* .root_y */
            /* .dt */
            /* .event_id */
            .flags = XIBarrierPointerReleased,
        };


        if (dev->type != MASTER_POINTER)
            continue;

        pbd = GetBarrierDevice(c, dev->id);
        if (!pbd->hit)
            continue;

        ev.deviceid = dev->id;
        ev.event_id = pbd->barrier_event_id;
        ev.dt = ms - pbd->last_timestamp;

        GetSpritePosition(dev, &root_x, &root_y);
        ev.root_x = root_x;
        ev.root_y = root_y;

        mieqEnqueue(dev, (InternalEvent *) &ev);
    }

    input_lock();
    xorg_list_del(&c->entry);
    input_unlock();

    FreePointerBarrierClient(c);
    return Success;
}

static void add_master_func(void *res, XID id, void *devid)
{
    struct PointerBarrier *b;
    struct PointerBarrierClient *barrier;
    struct PointerBarrierDevice *pbd;
    int *deviceid = devid;

    b = res;
    barrier = container_of(b, struct PointerBarrierClient, barrier);


    pbd = AllocBarrierDevice();
    pbd->deviceid = *deviceid;

    input_lock();
    xorg_list_add(&pbd->entry, &barrier->per_device);
    input_unlock();
}

static void remove_master_func(void *res, XID id, void *devid)
{
    struct PointerBarrierDevice *pbd;
    struct PointerBarrierClient *barrier;
    struct PointerBarrier *b;
    DeviceIntPtr dev;
    int *deviceid = devid;
    int rc;
    Time ms = GetTimeInMillis();

    rc = dixLookupDevice(&dev, *deviceid, serverClient, DixSendAccess);
    if (rc != Success)
        return;

    b = res;
    barrier = container_of(b, struct PointerBarrierClient, barrier);

    pbd = GetBarrierDevice(barrier, *deviceid);

    if (pbd->hit) {
        BarrierEvent ev = {
            .header = ET_Internal,
            .type =ET_BarrierLeave,
            .length = sizeof (BarrierEvent),
            .time = ms,
            .deviceid = *deviceid,
            .sourceid = 0,
            .dx = 0,
            .dy = 0,
            .root = barrier->screen->root->drawable.id,
            .window = barrier->window,
            .dt = ms - pbd->last_timestamp,
            .flags = XIBarrierPointerReleased,
            .event_id = pbd->barrier_event_id,
            .barrierid = barrier->id,
        };

        mieqEnqueue(dev, (InternalEvent *) &ev);
    }

    input_lock();
    xorg_list_del(&pbd->entry);
    input_unlock();
    free(pbd);
}

void XIBarrierNewMasterDevice(ClientPtr client, int deviceid)
{
    FindClientResourcesByType(client, PointerBarrierType, add_master_func, &deviceid);
}

void XIBarrierRemoveMasterDevice(ClientPtr client, int deviceid)
{
    FindClientResourcesByType(client, PointerBarrierType, remove_master_func, &deviceid);
}

int
XICreatePointerBarrier(ClientPtr client,
                       xXFixesCreatePointerBarrierReq * stuff)
{
    int err;
    struct PointerBarrierClient *barrier;
    struct PointerBarrier b;

    b.x1 = stuff->x1;
    b.x2 = stuff->x2;
    b.y1 = stuff->y1;
    b.y2 = stuff->y2;

    if (!barrier_is_horizontal(&b) && !barrier_is_vertical(&b))
        return BadValue;

    /* no 0-sized barriers */
    if (barrier_is_horizontal(&b) && barrier_is_vertical(&b))
        return BadValue;

    /* no infinite barriers on the wrong axis */
    if (barrier_is_horizontal(&b) && (b.y1 < 0 || b.y2 < 0))
        return BadValue;

    if (barrier_is_vertical(&b) && (b.x1 < 0 || b.x2 < 0))
        return BadValue;

    if ((err = CreatePointerBarrierClient(client, stuff, &barrier)))
        return err;

    if (!AddResource(stuff->barrier, PointerBarrierType, &barrier->barrier))
        return BadAlloc;

    return Success;
}

int
XIDestroyPointerBarrier(ClientPtr client,
                        xXFixesDestroyPointerBarrierReq * stuff)
{
    int err;
    void *barrier;

    err = dixLookupResourceByType((void **) &barrier, stuff->barrier,
                                  PointerBarrierType, client, DixDestroyAccess);
    if (err != Success) {
        client->errorValue = stuff->barrier;
        return err;
    }

    if (CLIENT_ID(stuff->barrier) != client->index)
        return BadAccess;

    FreeResource(stuff->barrier, RT_NONE);
    return Success;
}

int _X_COLD
SProcXIBarrierReleasePointer(ClientPtr client)
{
    xXIBarrierReleasePointerInfo *info;
    REQUEST(xXIBarrierReleasePointerReq);
    int i;

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXIBarrierReleasePointerReq);

    swapl(&stuff->num_barriers);
    if (stuff->num_barriers > UINT32_MAX / sizeof(xXIBarrierReleasePointerInfo))
        return BadLength;
    REQUEST_FIXED_SIZE(xXIBarrierReleasePointerReq, stuff->num_barriers * sizeof(xXIBarrierReleasePointerInfo));

    info = (xXIBarrierReleasePointerInfo*) &stuff[1];
    for (i = 0; i < stuff->num_barriers; i++, info++) {
        swaps(&info->deviceid);
        swapl(&info->barrier);
        swapl(&info->eventid);
    }

    return (ProcXIBarrierReleasePointer(client));
}

int
ProcXIBarrierReleasePointer(ClientPtr client)
{
    int i;
    int err;
    struct PointerBarrierClient *barrier;
    struct PointerBarrier *b;
    xXIBarrierReleasePointerInfo *info;

    REQUEST(xXIBarrierReleasePointerReq);
    REQUEST_AT_LEAST_SIZE(xXIBarrierReleasePointerReq);
    if (stuff->num_barriers > UINT32_MAX / sizeof(xXIBarrierReleasePointerInfo))
        return BadLength;
    REQUEST_FIXED_SIZE(xXIBarrierReleasePointerReq, stuff->num_barriers * sizeof(xXIBarrierReleasePointerInfo));

    info = (xXIBarrierReleasePointerInfo*) &stuff[1];
    for (i = 0; i < stuff->num_barriers; i++, info++) {
        struct PointerBarrierDevice *pbd;
        DeviceIntPtr dev;
        CARD32 barrier_id, event_id;
        _X_UNUSED CARD32 device_id;

        barrier_id = info->barrier;
        event_id = info->eventid;

        err = dixLookupDevice(&dev, info->deviceid, client, DixReadAccess);
        if (err != Success) {
            client->errorValue = BadDevice;
            return err;
        }

        err = dixLookupResourceByType((void **) &b, barrier_id,
                                      PointerBarrierType, client, DixReadAccess);
        if (err != Success) {
            client->errorValue = barrier_id;
            return err;
        }

        if (CLIENT_ID(barrier_id) != client->index)
            return BadAccess;


        barrier = container_of(b, struct PointerBarrierClient, barrier);

        pbd = GetBarrierDevice(barrier, dev->id);

        if (pbd->barrier_event_id == event_id)
            pbd->release_event_id = event_id;
    }

    return Success;
}

Bool
XIBarrierInit(void)
{
    int i;

    if (!dixRegisterPrivateKey(&BarrierScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        BarrierScreenPtr cs;

        cs = (BarrierScreenPtr) calloc(1, sizeof(BarrierScreenRec));
        if (!cs)
            return FALSE;
        xorg_list_init(&cs->barriers);
        SetBarrierScreen(pScreen, cs);
    }

    PointerBarrierType = CreateNewResourceType(BarrierFreeBarrier,
                                               "XIPointerBarrier");

    return PointerBarrierType;
}

void
XIBarrierReset(void)
{
    int i;
    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        BarrierScreenPtr cs = GetBarrierScreen(pScreen);
        free(cs);
        SetBarrierScreen(pScreen, NULL);
    }
}
