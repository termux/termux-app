/*
 * Copyright © 2014 Keith Packard
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


#include "glamor_priv.h"
#include "misyncshm.h"
#include "misyncstr.h"

#if XSYNC
/*
 * This whole file exists to wrap a sync fence trigger operation so
 * that we can flush GL to provide serialization between the server
 * and the shm fence client
 */

static DevPrivateKeyRec glamor_sync_fence_key;

struct glamor_sync_fence {
        SyncFenceSetTriggeredFunc set_triggered;
};

static inline struct glamor_sync_fence *
glamor_get_sync_fence(SyncFence *fence)
{
    return (struct glamor_sync_fence *) dixLookupPrivate(&fence->devPrivates, &glamor_sync_fence_key);
}

static void
glamor_sync_fence_set_triggered (SyncFence *fence)
{
	ScreenPtr screen = fence->pScreen;
	glamor_screen_private *glamor = glamor_get_screen_private(screen);
	struct glamor_sync_fence *glamor_fence = glamor_get_sync_fence(fence);

	/* Flush pending rendering operations */
        glamor_make_current(glamor);
        glFlush();

	fence->funcs.SetTriggered = glamor_fence->set_triggered;
	fence->funcs.SetTriggered(fence);
	glamor_fence->set_triggered = fence->funcs.SetTriggered;
	fence->funcs.SetTriggered = glamor_sync_fence_set_triggered;
}

static void
glamor_sync_create_fence(ScreenPtr screen,
                        SyncFence *fence,
                        Bool initially_triggered)
{
	glamor_screen_private *glamor = glamor_get_screen_private(screen);
	SyncScreenFuncsPtr screen_funcs = miSyncGetScreenFuncs(screen);
	struct glamor_sync_fence *glamor_fence = glamor_get_sync_fence(fence);

	screen_funcs->CreateFence = glamor->saved_procs.sync_screen_funcs.CreateFence;
	screen_funcs->CreateFence(screen, fence, initially_triggered);
	glamor->saved_procs.sync_screen_funcs.CreateFence = screen_funcs->CreateFence;
	screen_funcs->CreateFence = glamor_sync_create_fence;

	glamor_fence->set_triggered = fence->funcs.SetTriggered;
	fence->funcs.SetTriggered = glamor_sync_fence_set_triggered;
}
#endif

Bool
glamor_sync_init(ScreenPtr screen)
{
#if XSYNC
	glamor_screen_private   *glamor = glamor_get_screen_private(screen);
	SyncScreenFuncsPtr      screen_funcs;

	if (!dixPrivateKeyRegistered(&glamor_sync_fence_key)) {
		if (!dixRegisterPrivateKey(&glamor_sync_fence_key,
					   PRIVATE_SYNC_FENCE,
					   sizeof (struct glamor_sync_fence)))
			return FALSE;
	}

#ifdef HAVE_XSHMFENCE
	if (!miSyncShmScreenInit(screen))
		return FALSE;
#else
	if (!miSyncSetup(screen))
		return FALSE;
#endif

	screen_funcs = miSyncGetScreenFuncs(screen);
	glamor->saved_procs.sync_screen_funcs.CreateFence = screen_funcs->CreateFence;
	screen_funcs->CreateFence = glamor_sync_create_fence;
#endif
	return TRUE;
}

void
glamor_sync_close(ScreenPtr screen)
{
#if XSYNC
        glamor_screen_private   *glamor = glamor_get_screen_private(screen);
        SyncScreenFuncsPtr      screen_funcs = miSyncGetScreenFuncs(screen);

        if (screen_funcs)
                screen_funcs->CreateFence = glamor->saved_procs.sync_screen_funcs.CreateFence;
#endif
}
