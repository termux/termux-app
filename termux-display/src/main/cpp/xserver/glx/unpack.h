#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef __GLX_unpack_h__
#define __GLX_unpack_h__

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#define __GLX_PAD(s) (((s)+3) & (GLuint)~3)

/*
** Fetch the context-id out of a SingleReq request pointed to by pc.
*/
#define __GLX_GET_SINGLE_CONTEXT_TAG(pc) (((xGLXSingleReq*)pc)->contextTag)
#define __GLX_GET_VENDPRIV_CONTEXT_TAG(pc) (((xGLXVendorPrivateReq*)pc)->contextTag)

/*
** Fetch a double from potentially unaligned memory.
*/
#ifdef __GLX_ALIGN64
#define __GLX_MEM_COPY(dst,src,n)	memmove(dst,src,n)
#define __GLX_GET_DOUBLE(dst,src)	__GLX_MEM_COPY(&dst,src,8)
#else
#define __GLX_GET_DOUBLE(dst,src)	(dst) = *((GLdouble*)(src))
#endif

#define __GLX_BEGIN_REPLY(size) \
	reply.length = __GLX_PAD(size) >> 2;	\
	reply.type = X_Reply; 			\
	reply.sequenceNumber = client->sequence;

#define __GLX_SEND_HEADER() \
	WriteToClient (client, sz_xGLXSingleReply, &reply);

#define __GLX_PUT_RETVAL(a) \
	reply.retval = (a);

#define __GLX_PUT_SIZE(a) \
	reply.size = (a);

/*
** Get a buffer to hold returned data, with the given alignment.  If we have
** to realloc, allocate size+align, in case the pointer has to be bumped for
** alignment.  The answerBuffer should already be aligned.
**
** NOTE: the cast (long)res below assumes a long is large enough to hold a
** pointer.
*/
#define __GLX_GET_ANSWER_BUFFER(res,cl,size,align)			 \
    if (size < 0) return BadLength;                                      \
    else if ((size) > sizeof(answerBuffer)) {				 \
	int bump;							 \
	if ((cl)->returnBufSize < (size)+(align)) {			 \
	    (cl)->returnBuf = (GLbyte*)realloc((cl)->returnBuf,	 	 \
						(size)+(align));         \
	    if (!(cl)->returnBuf) {					 \
		return BadAlloc;					 \
	    }								 \
	    (cl)->returnBufSize = (size)+(align);			 \
	}								 \
	res = (char*)cl->returnBuf;					 \
	bump = (long)(res) % (align);					 \
	if (bump) res += (align) - (bump);				 \
    } else {								 \
	res = (char *)answerBuffer;					 \
    }

#define __GLX_SEND_BYTE_ARRAY(len) \
	WriteToClient(client, __GLX_PAD((len)*__GLX_SIZE_INT8), answer)

#define __GLX_SEND_SHORT_ARRAY(len) \
	WriteToClient(client, __GLX_PAD((len)*__GLX_SIZE_INT16), answer)

#define __GLX_SEND_INT_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_INT32, answer)

#define __GLX_SEND_FLOAT_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_FLOAT32, answer)

#define __GLX_SEND_DOUBLE_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_FLOAT64, answer)

#define __GLX_SEND_VOID_ARRAY(len)  __GLX_SEND_BYTE_ARRAY(len)
#define __GLX_SEND_UBYTE_ARRAY(len)  __GLX_SEND_BYTE_ARRAY(len)
#define __GLX_SEND_USHORT_ARRAY(len) __GLX_SEND_SHORT_ARRAY(len)
#define __GLX_SEND_UINT_ARRAY(len)  __GLX_SEND_INT_ARRAY(len)

/*
** PERFORMANCE NOTE:
** Machine dependent optimizations abound here; these swapping macros can
** conceivably be replaced with routines that do the job faster.
*/
#define __GLX_DECLARE_SWAP_VARIABLES \
	GLbyte sw

#define __GLX_DECLARE_SWAP_ARRAY_VARIABLES \
  	GLbyte *swapPC;		\
  	GLbyte *swapEnd

#define __GLX_SWAP_INT(pc) 			\
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[3]; 	\
  	((GLbyte *)(pc))[3] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[2]; 	\
  	((GLbyte *)(pc))[2] = sw;

#define __GLX_SWAP_SHORT(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[1]; 	\
  	((GLbyte *)(pc))[1] = sw;

#define __GLX_SWAP_DOUBLE(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[7]; 	\
  	((GLbyte *)(pc))[7] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[6]; 	\
  	((GLbyte *)(pc))[6] = sw;			\
  	sw = ((GLbyte *)(pc))[2]; 		\
  	((GLbyte *)(pc))[2] = ((GLbyte *)(pc))[5]; 	\
  	((GLbyte *)(pc))[5] = sw;			\
  	sw = ((GLbyte *)(pc))[3]; 		\
  	((GLbyte *)(pc))[3] = ((GLbyte *)(pc))[4]; 	\
  	((GLbyte *)(pc))[4] = sw;

#define __GLX_SWAP_FLOAT(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[3]; 	\
  	((GLbyte *)(pc))[3] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[2]; 	\
  	((GLbyte *)(pc))[2] = sw;

#define __GLX_SWAP_INT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_INT32;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_INT(swapPC);		\
	    swapPC += __GLX_SIZE_INT32;		\
	}

#define __GLX_SWAP_SHORT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_INT16;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_SHORT(swapPC);		\
	    swapPC += __GLX_SIZE_INT16;		\
	}

#define __GLX_SWAP_DOUBLE_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_FLOAT64;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_DOUBLE(swapPC);		\
	    swapPC += __GLX_SIZE_FLOAT64;	\
	}

#define __GLX_SWAP_FLOAT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_FLOAT32;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_FLOAT(swapPC);		\
	    swapPC += __GLX_SIZE_FLOAT32;	\
	}

#define __GLX_SWAP_REPLY_HEADER() \
	__GLX_SWAP_SHORT(&reply.sequenceNumber); \
	__GLX_SWAP_INT(&reply.length);

#define __GLX_SWAP_REPLY_RETVAL() \
	__GLX_SWAP_INT(&reply.retval)

#define __GLX_SWAP_REPLY_SIZE() \
	__GLX_SWAP_INT(&reply.size)

#endif                          /* !__GLX_unpack_h__ */
