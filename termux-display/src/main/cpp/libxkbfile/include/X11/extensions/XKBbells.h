#ifndef _XKBBELLS_H_
#define	_XKBBELLS_H_ 1

/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#define	XkbBN_Info			"Info"
#define	XkbBN_Warning			"Warning"
#define	XkbBN_MinorError		"MinorError"
#define	XkbBN_MajorError		"MajorError"
#define	XkbBN_BadValue			"BadValue"
#define	XkbBN_InvalidLocation		"InvalidLocation"
#define	XkbBN_Question			"Question"
#define	XkbBN_Start			"Start"
#define	XkbBN_End			"End"
#define	XkbBN_Success			"Success"
#define	XkbBN_Failure			"Failure"
#define	XkbBN_Wait			"Wait"
#define	XkbBN_Proceed			"Proceed"
#define	XkbBN_Ignore			"Ignore"
#define	XkbBN_Iconify			"Iconify"
#define	XkbBN_Deiconify			"Deconify"
#define	XkbBN_Open			"Open"
#define	XkbBN_Close			"Close"
#define	XkbBN_TerminalBell		"TerminalBell"
#define	XkbBN_MarginBell		"MarginBell"
#define	XkbBN_CursorStuck		"CursorStuck"
#define	XkbBN_NewMail			"NewMail"
#define	XkbBN_LaunchApp			"LaunchApp"
#define	XkbBN_AppDeath			"AppDeath"
#define	XkbBN_ImAlive			"ImAlive"
#define	XkbBN_ClockChimeHour		"ClockChimeHour"
#define	XkbBN_ClockChimeHalf		"ClockChimeHalf"
#define	XkbBN_ClockChimeQuarter		"ClockChimeQuarter"
#define	XkbBN_RepeatingLastBell		"RepeatingLastBell"
#define XkbBN_ComposeFail		"ComposeFail"
#define	XkbBN_AX_FeatureOn		"AX_FeatureOn"
#define	XkbBN_AX_FeatureOff		"AX_FeatureOff"
#define	XkbBN_AX_FeatureChange		"AX_FeatureChange"
#define	XkbBN_AX_IndicatorOn		"AX_IndicatorOn"
#define	XkbBN_AX_IndicatorOff		"AX_IndicatorOff"
#define	XkbBN_AX_IndicatorChange	"AX_IndicatorChange"
#define	XkbBN_AX_SlowKeysWarning	"AX_SlowKeysWarning"
#define	XkbBN_AX_SlowKeyPress		"AX_SlowKeyPress"
#define	XkbBN_AX_SlowKeyAccept		"AX_SlowKeyAccept"
#define	XkbBN_AX_SlowKeyReject		"AX_SlowKeyReject"
#define	XkbBN_AX_SlowKeyRelease		"AX_SlowKeyRelease"
#define	XkbBN_AX_BounceKeyReject	"AX_BounceKeyReject"
#define	XkbBN_AX_StickyLatch		"AX_StickyLatch"
#define	XkbBN_AX_StickyLock		"AX_StickyLock"
#define	XkbBN_AX_StickyUnlock		"AX_StickyUnlock"

#define	XkbBI_Info			0
#define	XkbBI_Warning			1
#define	XkbBI_MinorError		2
#define	XkbBI_MajorError		3
#define	XkbBI_BadValue			4
#define	XkbBI_InvalidLocation		5
#define	XkbBI_Question			6
#define	XkbBI_Start			7
#define	XkbBI_End			8
#define	XkbBI_Success			9
#define	XkbBI_Failure			10
#define	XkbBI_Wait			11
#define	XkbBI_Proceed			12
#define	XkbBI_Ignore			13
#define	XkbBI_Iconify			14
#define	XkbBI_Deiconify			15
#define	XkbBI_Open			16
#define	XkbBI_Close			17
#define	XkbBI_TerminalBell		18
#define	XkbBI_MarginBell		19
#define	XkbBI_CursorStuck		20
#define	XkbBI_NewMail			21
#define	XkbBI_LaunchApp			22
#define	XkbBI_AppDeath			23
#define	XkbBI_ImAlive			24
#define	XkbBI_ClockChimeHour		25
#define	XkbBI_ClockChimeHalf		26
#define	XkbBI_ClockChimeQuarter		27
#define	XkbBI_RepeatingLastBell		28
#define	XkbBI_ComposeFail		29
#define	XkbBI_AX_FeatureOn		30
#define	XkbBI_AX_FeatureOff		31
#define	XkbBI_AX_FeatureChange		32
#define	XkbBI_AX_IndicatorOn		33
#define	XkbBI_AX_IndicatorOff		34
#define	XkbBI_AX_IndicatorChange	35
#define	XkbBI_AX_SlowKeysWarning	36
#define	XkbBI_AX_SlowKeyPress		37
#define	XkbBI_AX_SlowKeyAccept		38
#define	XkbBI_AX_SlowKeyReject		39
#define	XkbBI_AX_SlowKeyRelease		40
#define	XkbBI_AX_BounceKeyReject	41
#define	XkbBI_AX_StickyLatch		42
#define	XkbBI_AX_StickyLock		43
#define	XkbBI_AX_StickyUnlock		44
#define	XkbBI_NumBells			45

_XFUNCPROTOBEGIN

extern	Bool XkbStdBell(
	Display *	/* dpy */,
	Window		/* win */,
	int		/* percent */,
	int		/* bellDef */
);

extern	Bool XkbStdBellEvent(
	Display *	/* dpy */,
	Window		/* win */,
	int		/* percent */,
	int		/* bellDef */
);

_XFUNCPROTOEND

#endif /* _XKBBELLS_H_ */
