/*
 *
 * Copyright IBM Corporation 1993
 *
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS, AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
/************************************************************************/
/*	Xaixlcint.h							*/
/*----------------------------------------------------------------------*/
/*	This file contains Xlcint.h extension for AIX.			*/
/************************************************************************/
#ifndef	_Xaixlcint_h
#define	_Xaixlcint_h

#include "Xlcint.h"
#include <sys/lc_core.h>

#define	_LC_LDX		11
#define	_LC_LDX_R6	(_LC_LDX+1)
#define	_LC_VERSION_R5	5
#define	_LC_VERSION_R6	6

typedef	struct	_LC_core_ldx_t	{
    _LC_object_t	lc_object_header;
    XLCd		(*default_loader)();
    Bool		sticky;
} _XlcCoreObjRec, *_XlcCoreObj;

#if _LC_VERSION < 0x40000000
#define __type_id type_id
#define __magic magic
#define __version version
#endif

#endif	/*_Xaixlcint_h*/
