#if !defined(WINMULTIWINDOWCLASS_H)
#define WINMULTIWINDOWCLASS_H
/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:     Earle F. Philhower, III
 */

/*
 * The next block of definitions are for window manager properties that
 * clients and applications use for communication.
 */

/*
 * Structures
 */

/*
 * WM_HINTS structure
 *
 * NOTE: this structure represents the internal format stored in the property
 * after it is marshalled by libX11, converting the flags field from an
 * arch-dependent long to a 32-bit int.
 */

typedef struct {
    int flags;                  /* marks which fields in this structure are defined */
    Bool input;                 /* does this application rely on the window manager to
                                   get keyboard input? */
    int initial_state;          /* see below */
    Pixmap icon_pixmap;         /* pixmap to be used as icon */
    Window icon_window;         /* window to be used as icon */
    int icon_x, icon_y;         /* initial position of icon */
    Pixmap icon_mask;           /* icon mask bitmap */
    XID window_group;           /* id of related window group */
    /* this structure may be extended in the future */
} WinXWMHints;

/* Window manager hints mask bits */
#define	InputHint	(1L << 0)
#define	StateHint	(1L << 1)
#define	IconPixmapHint	(1L << 2)
#define	IconWindowHint	(1L << 3)
#define	IconPositionHint	(1L << 4)
#define	IconMaskHint	(1L << 5)
#define	WindowGroupHint	(1L << 6)
#define	UrgencyHint	(1L << 8)
#define	AllHints 	(InputHint|StateHint|IconPixmapHint|IconWindowHint|IconPositionHint|IconMaskHint|WindowGroupHint)

/*
 * ICCCM 1.0 version containing base_width, base_height, and win_gravity fields;
 * used with WM_NORMAL_HINTS.
 *
 * NOTE: this structure represents the internal format stored in the property
 * after it is marshalled by libX11, converting the flags field from an
 * arch-dependent long to a 32-bit int.
 */
typedef struct {
    int flags;                  /* marks which fields in this structure are defined */
    int x, y;                   /* obsolete for new window mgrs, but clients */
    int width, height;          /* should set so old wm's don't mess up */
    int min_width, min_height;
    int max_width, max_height;
    int width_inc, height_inc;
    struct {
        int x;                  /* numerator */
        int y;                  /* denominator */
    } min_aspect, max_aspect;
    int base_width, base_height;        /* added by ICCCM version 1 */
    int win_gravity;            /* added by ICCCM version 1 */
} WinXSizeHints;

/* flags argument in size hints */
#define USPosition      (1L << 0)       /* user specified x, y */
#define USSize          (1L << 1)       /* user specified width, height */

#define PPosition       (1L << 2)       /* program specified position */
#define PSize           (1L << 3)       /* program specified size */
#define PMinSize        (1L << 4)       /* program specified minimum size */
#define PMaxSize        (1L << 5)       /* program specified maximum size */
#define PResizeInc      (1L << 6)       /* program specified resize increments */
#define PAspect         (1L << 7)       /* program specified min and max aspect ratios */
#define PBaseSize       (1L << 8)       /* program specified base for incrementing */
#define PWinGravity     (1L << 9)       /* program specified window gravity */

/* obsolete */
#define PAllHints (PPosition|PSize|PMinSize|PMaxSize|PResizeInc|PAspect)

/*
 * Function prototypes
 */

int
 winMultiWindowGetWMHints(WindowPtr pWin, WinXWMHints * hints);

int
 winMultiWindowGetClassHint(WindowPtr pWin, char **res_name, char **res_class);

int
 winMultiWindowGetWindowRole(WindowPtr pWin, char **res_role);

int
 winMultiWindowGetWMNormalHints(WindowPtr pWin, WinXSizeHints * hints);

int
 winMultiWindowGetWMName(WindowPtr pWin, char **wmName);

int
 winMultiWindowGetTransientFor(WindowPtr pWin, Window *ppDaddy);

#endif
