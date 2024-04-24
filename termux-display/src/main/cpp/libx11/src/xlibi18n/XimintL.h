/******************************************************************

          Copyright 1991, 1992, 1993, 1994 by FUJITSU LIMITED
          Copyright 1993 by Digital Equipment Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED and
Digital Equipment Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.  FUJITSU LIMITED and Digital Equipment Corporation
makes no representations about the suitability of this software for
any purpose.  It is provided "as is" without express or implied
warranty.

FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

  Author:    Takashi Fujiwara     FUJITSU LIMITED
                               	  fujiwara@a80.tech.yk.fujitsu.co.jp
  Modifier:  Franky Ling          Digital Equipment Corporation
	                          frankyling@hgrd01.enet.dec.com

******************************************************************/

#ifndef _XIMINTL_H
#define _XIMINTL_H

#define	COMPOSE_FILE	"Compose"

/*
 * Data Structures for Local Processing
 */
typedef INT32  DTIndex;
typedef INT32  DTCharIndex;
typedef BITS32 DTModifier;

typedef struct _DefTree {
    DTIndex          next;
    DTIndex          succession;	/* successive Key Sequence */
					/* Key definitions */
    DTModifier       modifier_mask;
    DTModifier       modifier;
    KeySym           keysym;		/* leaf only */
    DTCharIndex      mb;
    DTCharIndex      wc;		/* make from mb */
    DTCharIndex      utf8;		/* make from mb */
    KeySym           ks;
} DefTree;

typedef struct _DefTreeBase {
    DefTree         *tree;
    char            *mb;
    wchar_t         *wc;
    char            *utf8;
    DTIndex          treeused, treesize;
    DTCharIndex      mbused,   mbsize;
    DTCharIndex      wcused,   wcsize;
    DTCharIndex      utf8used, utf8size;
} DefTreeBase;

typedef struct _XimLocalPrivateRec {
	/* The first fields are identical with XimCommonPrivateRec. */
	XlcConv		 ctom_conv;
	XlcConv		 ctow_conv;
	XlcConv		 ctoutf8_conv;
	XlcConv		 cstomb_conv;
	XlcConv		 cstowc_conv;
	XlcConv		 cstoutf8_conv;
	XlcConv		 ucstoc_conv;
	XlcConv		 ucstoutf8_conv;

	XIC		 current_ic;
	DefTreeBase	 base;
	DTIndex          top;
} XimLocalPrivateRec;

typedef struct _XicThaiPart {
	int		comp_state;
	KeySym		keysym;
	int		input_mode;
} XicThaiPart;

typedef struct _XicLocalPrivateRec {
	long			 value_mask;
	DefTreeBase              base;
	DTIndex			 context;
	DTIndex			 composed;
	XicThaiPart		 thai;

	XIMResourceList		 ic_resources;
	unsigned int		 ic_num_resources;

	unsigned char	 	 brl_pressed, brl_committing, brl_committed;
	Time		 	 brl_release_start;
} XicLocalPrivateRec;
#endif /* _XIMINTL_H */
