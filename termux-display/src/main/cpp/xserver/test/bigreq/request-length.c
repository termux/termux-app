/*
 * Copyright © 2017 Broadcom
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <xcb/xcb.h>
#include <xcb/bigreq.h>
#include <xcb/xinput.h>

int main(int argc, char **argv)
{
    xcb_connection_t *c = xcb_connect(NULL, NULL);
    int fd = xcb_get_file_descriptor(c);

    struct {
        uint8_t extension;
        uint8_t opcode;
        uint16_t length;
        uint32_t length_bigreq;
        uint32_t win;
        int num_masks;
        uint16_t pad;
    } xise_req = {
        .extension = 0,
        .opcode = XCB_INPUT_XI_SELECT_EVENTS,
        /* The server triggers BadValue on a zero num_mask */
        .num_masks = 0,
        .win = 0,

        /* This is the value that triggers the bug. */
        .length_bigreq = 0,
    };

    xcb_query_extension_cookie_t cookie;
    xcb_query_extension_reply_t *rep;

    cookie = xcb_query_extension(c, 15, "XInputExtension");
    rep = xcb_query_extension_reply(c, cookie, NULL);
    xise_req.extension = rep->major_opcode;

    free(xcb_big_requests_enable_reply(c, xcb_big_requests_enable(c), NULL));

    /* Manually write out the bad request.  XCB can't help us here.*/
    write(fd, &xise_req, sizeof(xise_req));

    /* Block until the server has processed our mess and throws an
     * error. If we get disconnected, then the server has noticed what we're
     * up to. If we get an error back from the server, it looked at our fake
     * request - which shouldn't happen.
     */
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
    };
    poll(&pfd, 1, -1);

    if (pfd.revents & POLLHUP) {
        /* We got killed by the server for being naughty. Yay! */
        return 0;
    }

    /* We didn't get disconnected, that's bad. If we get a BadValue from our
     * request, we at least know that the bug triggered.
     *
     * If we get anything else back, something else has gone wrong.
     */
    xcb_generic_error_t error;
    int r = read(fd, &error, sizeof(error));

    if (r == sizeof(error) &&
        error.error_code == 2 /* BadValue */  &&
        error.major_code == xise_req.extension &&
        error.minor_code == XCB_INPUT_XI_SELECT_EVENTS)
        return 1; /* Our request was processed, which shouldn't happen */

    /* Something else bad happened. We got something back but it's not the
     * error we expected. If this happens, it needs to be investigated. */

    return 2;
}
