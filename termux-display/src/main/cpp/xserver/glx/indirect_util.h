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

#ifndef __GLX_INDIRECT_UTIL_H__
#define __GLX_INDIRECT_UTIL_H__

extern GLint __glGetBooleanv_variable_size(GLenum e);

extern void *__glXGetAnswerBuffer(__GLXclientState * cl,
                                  size_t required_size, void *local_buffer,
                                  size_t local_size, unsigned alignment);

extern void __glXSendReply(ClientPtr client, const void *data,
                           size_t elements, size_t element_size,
                           GLboolean always_array, CARD32 retval);

extern void __glXSendReplySwap(ClientPtr client, const void *data,
                               size_t elements, size_t element_size,
                               GLboolean always_array, CARD32 retval);

struct __glXDispatchInfo;

extern void *__glXGetProtocolDecodeFunction(const struct __glXDispatchInfo
                                            *dispatch_info, int opcode,
                                            int swapped_version);

extern int __glXGetProtocolSizeData(const struct __glXDispatchInfo
                                    *dispatch_info, int opcode,
                                    __GLXrenderSizeData * data);

#endif                          /* __GLX_INDIRECT_UTIL_H__ */
