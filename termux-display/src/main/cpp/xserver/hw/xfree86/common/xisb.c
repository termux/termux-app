/*
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */

/*
	X Input Serial Buffer routines for use in any XInput driver that accesses
	a serial device.
*/

/*****************************************************************************
 *	Standard Headers
 ****************************************************************************/

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <misc.h>
#include <xf86.h>
#include <xf86_OSproc.h>
#include <xf86_OSlib.h>
#include <xf86Xinput.h>
#include "xisb.h"

/*****************************************************************************
 *	Local Headers
 ****************************************************************************/

/*****************************************************************************
 *	Variables without includable headers
 ****************************************************************************/

/*****************************************************************************
 *	Local Variables
 ****************************************************************************/

/*****************************************************************************
 *	Function Definitions
 ****************************************************************************/

XISBuffer *
XisbNew(int fd, ssize_t size)
{
    XISBuffer *b;

    b = malloc(sizeof(XISBuffer));
    if (!b)
        return NULL;
    b->buf = malloc((sizeof(unsigned char) * size));
    if (!b->buf) {
        free(b);
        return NULL;
    }

    b->fd = fd;
    b->trace = 0;
    b->block_duration = 0;
    b->current = 1;             /* force it to be past the end to trigger initial read */
    b->end = 0;
    b->buffer_size = size;
    return b;
}

void
XisbFree(XISBuffer * b)
{
    free(b->buf);
    free(b);
}

int
XisbRead(XISBuffer * b)
{
    int ret;

    if (b->current >= b->end) {
        if (b->block_duration >= 0) {
            if (xf86WaitForInput(b->fd, b->block_duration) < 1)
                return -1;
        }
        else {
            /*
             * automatically clear it so if XisbRead is called in a loop
             * the next call will make sure there is data with select and
             * thus prevent a blocking read
             */
            b->block_duration = 0;
        }

        ret = xf86ReadSerial(b->fd, b->buf, b->buffer_size);
        switch (ret) {
        case 0:
            return -1;          /* timeout */
        case -1:
            return -2;          /* error */
        default:
            b->end = ret;
            b->current = 0;
            break;
        }
    }
    if (b->trace)
        ErrorF("read 0x%02x (%c)\n", b->buf[b->current],
               isprint(b->buf[b->current]) ? b->buf[b->current] : '.');

    return b->buf[b->current++];
}

/* the only purpose of this function is to provide output tracing */
ssize_t
XisbWrite(XISBuffer * b, unsigned char *msg, ssize_t len)
{
    if (b->trace) {
        int i = 0;

        for (i = 0; i < len; i++)
            ErrorF("\t\twrote 0x%02x (%c)\n", msg[i], msg[i]);
    }
    return (xf86WriteSerial(b->fd, msg, len));
}

/* turn tracing of this buffer on (1) or off (0) */
void
XisbTrace(XISBuffer * b, int trace)
{
    b->trace = trace;
}

/*
 * specify a block_duration of -1 when you know the buffer's fd is ready to
 * read. After a read, it is automatically set to 0 so that the next read
 * will use check to select for data and prevent a block.
 * It is the caller's responsibility to set the block_duration to -1 if it
 * knows that there is data to read (because the main select loop triggered
 * the read) and wants to avoid the unnecessary overhead of the select call
 *
 * a zero or positive block duration will cause the select to block for the
 * give duration in usecs.
 */

void
XisbBlockDuration(XISBuffer * b, int block_duration)
{
    b->block_duration = block_duration;
}
