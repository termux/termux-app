/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include <X11/Xmd.h>
#include <GL/gl.h>
#include <GL/glxproto.h>
#include <inttypes.h>
#include "indirect_size.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"
#include "glxserver.h"
#include "glxbyteorder.h"
#include "singlesize.h"
#include "glxext.h"
#include "indirect_table.h"
#include "indirect_util.h"

#define __GLX_PAD(a) (((a)+3)&~3)

GLint
__glGetBooleanv_variable_size(GLenum e)
{
    if (e == GL_COMPRESSED_TEXTURE_FORMATS) {
        GLint temp;

        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &temp);
        return temp;
    }
    else {
        return 0;
    }
}

/**
 * Get a properly aligned buffer to hold reply data.
 *
 * \warning
 * This function assumes that \c local_buffer is already properly aligned.
 * It also assumes that \c alignment is a power of two.
 */
void *
__glXGetAnswerBuffer(__GLXclientState * cl, size_t required_size,
                     void *local_buffer, size_t local_size, unsigned alignment)
{
    void *buffer = local_buffer;
    const intptr_t mask = alignment - 1;

    if (local_size < required_size) {
        size_t worst_case_size;
        intptr_t temp_buf;

        if (required_size < SIZE_MAX - alignment)
            worst_case_size = required_size + alignment;
        else
            return NULL;

        if (cl->returnBufSize < worst_case_size) {
            void *temp = realloc(cl->returnBuf, worst_case_size);

            if (temp == NULL) {
                return NULL;
            }

            cl->returnBuf = temp;
            cl->returnBufSize = worst_case_size;
        }

        temp_buf = (intptr_t) cl->returnBuf;
        temp_buf = (temp_buf + mask) & ~mask;
        buffer = (void *) temp_buf;
    }

    return buffer;
}

/**
 * Send a GLX reply to the client.
 *
 * Technically speaking, there are several different ways to encode a GLX
 * reply.  The primary difference is whether or not certain fields (e.g.,
 * retval, size, and "pad3") are set.  This function gets around that by
 * always setting all of the fields to "reasonable" values.  This does no
 * harm to clients, but it does make the server-side code much more compact.
 */
void
__glXSendReply(ClientPtr client, const void *data, size_t elements,
               size_t element_size, GLboolean always_array, CARD32 retval)
{
    size_t reply_ints = 0;
    xGLXSingleReply reply = { 0, };

    if (__glXErrorOccured()) {
        elements = 0;
    }
    else if ((elements > 1) || always_array) {
        reply_ints = bytes_to_int32(elements * element_size);
    }

    reply.length = reply_ints;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.size = elements;
    reply.retval = retval;

    /* It is faster on almost always every architecture to just copy the 8
     * bytes, even when not necessary, than check to see of the value of
     * elements requires it.  Copying the data when not needed will do no
     * harm.
     */

    (void) memcpy(&reply.pad3, data, 8);
    WriteToClient(client, sz_xGLXSingleReply, &reply);

    if (reply_ints != 0) {
        WriteToClient(client, reply_ints * 4, data);
    }
}

/**
 * Send a GLX reply to the client.
 *
 * Technically speaking, there are several different ways to encode a GLX
 * reply.  The primary difference is whether or not certain fields (e.g.,
 * retval, size, and "pad3") are set.  This function gets around that by
 * always setting all of the fields to "reasonable" values.  This does no
 * harm to clients, but it does make the server-side code much more compact.
 *
 * \warning
 * This function assumes that values stored in \c data will be byte-swapped
 * by the caller if necessary.
 */
void
__glXSendReplySwap(ClientPtr client, const void *data, size_t elements,
                   size_t element_size, GLboolean always_array, CARD32 retval)
{
    size_t reply_ints = 0;
    xGLXSingleReply reply = { 0, };

    if (__glXErrorOccured()) {
        elements = 0;
    }
    else if ((elements > 1) || always_array) {
        reply_ints = bytes_to_int32(elements * element_size);
    }

    reply.length = bswap_32(reply_ints);
    reply.type = X_Reply;
    reply.sequenceNumber = bswap_16(client->sequence);
    reply.size = bswap_32(elements);
    reply.retval = bswap_32(retval);

    /* It is faster on almost always every architecture to just copy the 8
     * bytes, even when not necessary, than check to see of the value of
     * elements requires it.  Copying the data when not needed will do no
     * harm.
     */

    (void) memcpy(&reply.pad3, data, 8);
    WriteToClient(client, sz_xGLXSingleReply, &reply);

    if (reply_ints != 0) {
        WriteToClient(client, reply_ints * 4, data);
    }
}

static int
get_decode_index(const struct __glXDispatchInfo *dispatch_info, unsigned opcode)
{
    int remaining_bits;
    int next_remain;
    const int_fast16_t *const tree = dispatch_info->dispatch_tree;
    int_fast16_t index;

    remaining_bits = dispatch_info->bits;
    if (opcode >= (1U << remaining_bits)) {
        return -1;
    }

    index = 0;
    for ( /* empty */ ; remaining_bits > 0; remaining_bits = next_remain) {
        unsigned mask;
        unsigned child_index;

        /* Calculate the slice of bits used by this node.
         *
         * If remaining_bits = 8 and tree[index] = 3, the mask of just the
         * remaining bits is 0x00ff and the mask for the remaining bits after
         * this node is 0x001f.  By taking 0x00ff & ~0x001f, we get 0x00e0.
         * This masks the 3 bits that we would want for this node.
         */

        next_remain = remaining_bits - tree[index];
        mask = ((1 << remaining_bits) - 1) & ~((1 << next_remain) - 1);

        /* Using the mask, calculate the index of the opcode in the node.
         * With that index, fetch the index of the next node.
         */

        child_index = (opcode & mask) >> next_remain;
        index = tree[index + 1 + child_index];

        /* If the next node is an empty leaf, the opcode is for a non-existent
         * function.  We're done.
         *
         * If the next node is a non-empty leaf, look up the function pointer
         * and return it.
         */

        if (index == EMPTY_LEAF) {
            return -1;
        }
        else if (IS_LEAF_INDEX(index)) {
            unsigned func_index;

            /* The value stored in the tree for a leaf node is the base of
             * the function pointers for that leaf node.  The offset for the
             * function for a particular opcode is the remaining bits in the
             * opcode.
             */

            func_index = -index;
            func_index += opcode & ((1 << next_remain) - 1);
            return func_index;
        }
    }

    /* We should *never* get here!!!
     */
    return -1;
}

void *
__glXGetProtocolDecodeFunction(const struct __glXDispatchInfo *dispatch_info,
                               int opcode, int swapped_version)
{
    const int func_index = get_decode_index(dispatch_info, opcode);

    return (func_index < 0)
        ? NULL
        : (void *) dispatch_info->
        dispatch_functions[func_index][swapped_version];
}

int
__glXGetProtocolSizeData(const struct __glXDispatchInfo *dispatch_info,
                         int opcode, __GLXrenderSizeData * data)
{
    if (dispatch_info->size_table != NULL) {
        const int func_index = get_decode_index(dispatch_info, opcode);

        if ((func_index >= 0)
            && (dispatch_info->size_table[func_index][0] != 0)) {
            const int var_offset = dispatch_info->size_table[func_index][1];

            data->bytes = dispatch_info->size_table[func_index][0];
            data->varsize = (var_offset != ~0)
                ? dispatch_info->size_func_table[var_offset]
                : NULL;

            return 0;
        }
    }

    return -1;
}
