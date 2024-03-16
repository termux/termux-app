/* Copyright (c) 2008-2012 Apple Inc.
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

#include <launch.h>
#include <asl.h>
#include <errno.h>

#include "launchd_fd.h"

extern aslclient aslc;

int
launchd_display_fd(void)
{
    launch_data_t sockets_dict, checkin_request, checkin_response;
    launch_data_t listening_fd_array, listening_fd;

    /* Get launchd fd */
    if ((checkin_request = launch_data_new_string(LAUNCH_KEY_CHECKIN)) ==
        NULL) {
        asl_log(
            aslc, NULL, ASL_LEVEL_ERR,
            "launch_data_new_string(\"" LAUNCH_KEY_CHECKIN
            "\") Unable to create string.\n");
        return ERROR_FD;
    }

    if ((checkin_response = launch_msg(checkin_request)) == NULL) {
        asl_log(aslc, NULL, ASL_LEVEL_WARNING,
                "launch_msg(\"" LAUNCH_KEY_CHECKIN "\") IPC failure: %s\n",
                strerror(
                    errno));
        return ERROR_FD;
    }

    if (LAUNCH_DATA_ERRNO == launch_data_get_type(checkin_response)) {
        // ignore EACCES, which is common if we weren't started by launchd
        if (launch_data_get_errno(checkin_response) != EACCES)
            asl_log(aslc, NULL, ASL_LEVEL_ERR,
                    "launchd check-in failed: %s\n",
                    strerror(launch_data_get_errno(
                                 checkin_response)));
        return ERROR_FD;
    }

    sockets_dict = launch_data_dict_lookup(checkin_response,
                                           LAUNCH_JOBKEY_SOCKETS);
    if (NULL == sockets_dict) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "launchd check-in: no sockets found to answer requests on!\n");
        return ERROR_FD;
    }

    if (launch_data_dict_get_count(sockets_dict) > 1) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "launchd check-in: some sockets will be ignored!\n");
        return ERROR_FD;
    }

    listening_fd_array = launch_data_dict_lookup(sockets_dict,
                                                 BUNDLE_ID_PREFIX ":0");
    if (NULL == listening_fd_array) {
        listening_fd_array = launch_data_dict_lookup(sockets_dict, ":0");
        if (NULL == listening_fd_array) {
            asl_log(
                aslc, NULL, ASL_LEVEL_ERR,
                "launchd check-in: No known sockets found to answer requests on! \"%s:0\" and \":0\" failed.\n",
                BUNDLE_ID_PREFIX);
            return ERROR_FD;
        }
    }

    if (launch_data_array_get_count(listening_fd_array) != 1) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "launchd check-in: Expected 1 socket from launchd, got %u)\n",
                (unsigned)launch_data_array_get_count(
                    listening_fd_array));
        return ERROR_FD;
    }

    listening_fd = launch_data_array_get_index(listening_fd_array, 0);
    return launch_data_get_fd(listening_fd);
}
