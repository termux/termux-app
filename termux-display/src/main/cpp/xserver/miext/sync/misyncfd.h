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

#ifndef _MISYNCFD_H_
#define _MISYNCFD_H_

typedef int (*SyncScreenCreateFenceFromFdFunc) (ScreenPtr screen,
                                                SyncFence *fence,
                                                int fd,
                                                Bool initially_triggered);

typedef int (*SyncScreenGetFenceFdFunc) (ScreenPtr screen,
                                      SyncFence *fence);

#define SYNC_FD_SCREEN_FUNCS_VERSION    1

typedef struct _syncFdScreenFuncs {
    int                                 version;
    SyncScreenCreateFenceFromFdFunc     CreateFenceFromFd;
    SyncScreenGetFenceFdFunc            GetFenceFd;
} SyncFdScreenFuncsRec, *SyncFdScreenFuncsPtr;

extern _X_EXPORT Bool miSyncFdScreenInit(ScreenPtr pScreen,
                                         const SyncFdScreenFuncsRec *funcs);

#endif /* _MISYNCFD_H_ */
