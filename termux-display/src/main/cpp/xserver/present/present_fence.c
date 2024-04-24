/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "present_priv.h"
#include <gcstruct.h>
#include <misync.h>
#include <misyncstr.h>

/*
 * Wraps SyncFence objects so we can add a SyncTrigger to find out
 * when the SyncFence gets destroyed and clean up appropriately
 */

struct present_fence {
    SyncTrigger         trigger;
    SyncFence           *fence;
    void                (*callback)(void *param);
    void                *param;
};

/*
 * SyncTrigger callbacks
 */
static Bool
present_fence_sync_check_trigger(SyncTrigger *trigger, int64_t oldval)
{
    struct present_fence        *present_fence = container_of(trigger, struct present_fence, trigger);

    return present_fence->callback != NULL;
}

static void
present_fence_sync_trigger_fired(SyncTrigger *trigger)
{
    struct present_fence        *present_fence = container_of(trigger, struct present_fence, trigger);

    if (present_fence->callback)
        (*present_fence->callback)(present_fence->param);
}

static void
present_fence_sync_counter_destroyed(SyncTrigger *trigger)
{
    struct present_fence        *present_fence = container_of(trigger, struct present_fence, trigger);

    present_fence->fence = NULL;
}

struct present_fence *
present_fence_create(SyncFence *fence)
{
    struct present_fence        *present_fence;

    present_fence = calloc (1, sizeof (struct present_fence));
    if (!present_fence)
        return NULL;

    present_fence->fence = fence;
    present_fence->trigger.pSync = (SyncObject *) fence;
    present_fence->trigger.CheckTrigger = present_fence_sync_check_trigger;
    present_fence->trigger.TriggerFired = present_fence_sync_trigger_fired;
    present_fence->trigger.CounterDestroyed = present_fence_sync_counter_destroyed;

    if (SyncAddTriggerToSyncObject(&present_fence->trigger) != Success) {
        free (present_fence);
        return NULL;
    }
    return present_fence;
}

void
present_fence_destroy(struct present_fence *present_fence)
{
    if (present_fence) {
        if (present_fence->fence)
            SyncDeleteTriggerFromSyncObject(&present_fence->trigger);
        free(present_fence);
    }
}

void
present_fence_set_triggered(struct present_fence *present_fence)
{
    if (present_fence)
        if (present_fence->fence)
            (*present_fence->fence->funcs.SetTriggered) (present_fence->fence);
}

Bool
present_fence_check_triggered(struct present_fence *present_fence)
{
    if (!present_fence)
        return TRUE;
    if (!present_fence->fence)
        return TRUE;
    return (*present_fence->fence->funcs.CheckTriggered)(present_fence->fence);
}

void
present_fence_set_callback(struct present_fence *present_fence,
                           void (*callback) (void *param),
                           void *param)
{
    present_fence->callback = callback;
    present_fence->param = param;
}

XID
present_fence_id(struct present_fence *present_fence)
{
    if (!present_fence)
        return None;
    if (!present_fence->fence)
        return None;
    return present_fence->fence->sync.id;
}
