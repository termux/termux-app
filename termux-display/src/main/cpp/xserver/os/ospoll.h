/*
 * Copyright © 2016 Keith Packard
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

#ifndef _OSPOLL_H_
#define _OSPOLL_H_

/* Forward declaration */
struct ospoll;

/**
 * ospoll_wait trigger mode
 *
 * @ospoll_trigger_edge
 *      Trigger only when going from no data available
 *      to data available.
 *
 * @ospoll_trigger_level
 *      Trigger whenever there is data available
 */
enum ospoll_trigger {
    ospoll_trigger_edge,
    ospoll_trigger_level
};

/**
 * Create a new ospoll structure
 */
struct ospoll *
ospoll_create(void);

/**
 * Destroy an ospoll structure
 *
 * @param       ospoll          ospoll to destroy
 */
void
ospoll_destroy(struct ospoll *ospoll);

/**
 * Add a file descriptor to monitor
 *
 * @param       ospoll          ospoll to add to
 * @param       fd              File descriptor to monitor
 * @param       trigger         Trigger mode for ospoll_wait
 * @param       callback        Function to call when triggered
 * @param       data            Extra data to pass callback
 */
Bool
ospoll_add(struct ospoll *ospoll, int fd,
           enum ospoll_trigger trigger,
           void (*callback)(int fd, int xevents, void *data),
           void *data);

/**
 * Remove a monitored file descriptor
 *
 * @param       ospoll          ospoll to remove from
 * @param       fd              File descriptor to stop monitoring
 */
void
ospoll_remove(struct ospoll *ospoll, int fd);

/**
 * Listen on additional events
 *
 * @param       ospoll          ospoll monitoring fd
 * @param       fd              File descriptor to change
 * @param       events          Additional events to trigger on
 */
void
ospoll_listen(struct ospoll *ospoll, int fd, int xevents);

/**
 * Stop listening on events
 *
 * @param       ospoll          ospoll monitoring fd
 * @param       fd              File descriptor to change
 * @param       events          events to stop triggering on
 */
void
ospoll_mute(struct ospoll *ospoll, int fd, int xevents);

/**
 * Wait for events
 *
 * @param       ospoll          ospoll to wait on
 * @param       timeout         < 0 wait forever
 *                              = 0 check and return
 *                              > 0 timeout in milliseconds
 * @return      < 0 error
 *              = 0 timeout
 *              > 0 number of events delivered
 */
int
ospoll_wait(struct ospoll *ospoll, int timeout);

/**
 * Reset edge trigger status
 *
 * @param       ospoll          ospoll monitoring fd
 * @param       fd              file descriptor
 *
 * ospoll_reset_events resets the state of an edge-triggered
 * fd so that ospoll_wait calls will report events again.
 *
 * Call this after a read/recv operation reports no more data available.
 */
void
ospoll_reset_events(struct ospoll *ospoll, int fd);

/**
 * Fetch the data associated with an fd
 *
 * @param       ospoll          ospoll monitoring fd
 * @param       fd              file descriptor
 *
 * @return      data parameter passed to ospoll_add call on
 *              this file descriptor
 */
void *
ospoll_data(struct ospoll *ospoll, int fd);

#endif /* _OSPOLL_H_ */
