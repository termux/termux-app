/* $Xorg: PM.h,v 1.4 2001/02/09 02:05:34 xorgcvs Exp $ */

/*
Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/

/*		Proxy Management Protocol		*/

#ifndef _PM_H_
#define _PM_H_

#define PM_PROTOCOL_NAME "PROXY_MANAGEMENT"

#define PM_MAJOR_VERSION 1
#define PM_MINOR_VERSION 0

/*
 * PM minor opcodes
 */
#define PM_Error		ICE_Error /* == 0 */
#define PM_GetProxyAddr		1
#define PM_GetProxyAddrReply	2
#define PM_StartProxy		3

/*
 * status return codes for GetProxyAddrReply
 */
#define		   PM_Unable	0
#define		   PM_Success	1
#define            PM_Failure	2

#endif /* _PM_H_ */
