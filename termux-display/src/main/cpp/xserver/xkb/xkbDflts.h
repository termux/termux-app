/* This file generated automatically by xkbcomp */
/* DO  NOT EDIT */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef DEFAULT_H
#define DEFAULT_H 1

#define GET_ATOM(d,s)	MakeAtom(s,strlen(s),1)
#define DPYTYPE	char *
#define NUM_KEYS	1

#define	vmod_NumLock	0
#define	vmod_Alt	1
#define	vmod_LevelThree	2
#define	vmod_AltGr	3
#define	vmod_ScrollLock	4

#define	vmod_NumLockMask	(1<<0)
#define	vmod_AltMask	(1<<1)
#define	vmod_LevelThreeMask	(1<<2)
#define	vmod_AltGrMask	(1<<3)
#define	vmod_ScrollLockMask	(1<<4)

/* types name is "default" */
static Atom lnames_ONE_LEVEL[1];

static XkbKTMapEntryRec map_TWO_LEVEL[1] = {
    {1, 1, {ShiftMask, ShiftMask, 0}}
};

static Atom lnames_TWO_LEVEL[2];

static XkbKTMapEntryRec map_ALPHABETIC[2] = {
    {1, 1, {ShiftMask, ShiftMask, 0}},
    {1, 0, {LockMask, LockMask, 0}}
};

static XkbModsRec preserve_ALPHABETIC[2] = {
    {0, 0, 0},
    {LockMask, LockMask, 0}
};

static Atom lnames_ALPHABETIC[2];

static XkbKTMapEntryRec map_KEYPAD[2] = {
    {1, 1, {ShiftMask, ShiftMask, 0}},
    {0, 1, {0, 0, vmod_NumLockMask}}
};

static Atom lnames_KEYPAD[2];

static XkbKTMapEntryRec map_PC_BREAK[1] = {
    {1, 1, {ControlMask, ControlMask, 0}}
};

static Atom lnames_PC_BREAK[2];

static XkbKTMapEntryRec map_PC_SYSRQ[1] = {
    {0, 1, {0, 0, vmod_AltMask}}
};

static Atom lnames_PC_SYSRQ[2];

static XkbKTMapEntryRec map_CTRL_ALT[1] = {
    {0, 1, {ControlMask, ControlMask, vmod_AltMask}}
};

static Atom lnames_CTRL_ALT[2];

static XkbKTMapEntryRec map_THREE_LEVEL[3] = {
    {1, 1, {ShiftMask, ShiftMask, 0}},
    {0, 2, {0, 0, vmod_LevelThreeMask}},
    {0, 2, {ShiftMask, ShiftMask, vmod_LevelThreeMask}}
};

static Atom lnames_THREE_LEVEL[3];

static XkbKTMapEntryRec map_SHIFT_ALT[1] = {
    {0, 1, {ShiftMask, ShiftMask, vmod_AltMask}}
};

static Atom lnames_SHIFT_ALT[2];

static XkbKeyTypeRec dflt_types[] = {
    {
     {0, 0, 0},
     1,
     0, NULL, NULL,
     None, lnames_ONE_LEVEL},
    {
     {ShiftMask, ShiftMask, 0},
     2,
     1, map_TWO_LEVEL, NULL,
     None, lnames_TWO_LEVEL},
    {
     {ShiftMask | LockMask, ShiftMask | LockMask, 0},
     2,
     2, map_ALPHABETIC, preserve_ALPHABETIC,
     None, lnames_ALPHABETIC},
    {
     {ShiftMask, ShiftMask, vmod_NumLockMask},
     2,
     2, map_KEYPAD, NULL,
     None, lnames_KEYPAD},
    {
     {ControlMask, ControlMask, 0},
     2,
     1, map_PC_BREAK, NULL,
     None, lnames_PC_BREAK},
    {
     {0, 0, vmod_AltMask},
     2,
     1, map_PC_SYSRQ, NULL,
     None, lnames_PC_SYSRQ},
    {
     {ControlMask, ControlMask, vmod_AltMask},
     2,
     1, map_CTRL_ALT, NULL,
     None, lnames_CTRL_ALT},
    {
     {ShiftMask, ShiftMask, vmod_LevelThreeMask},
     3,
     3, map_THREE_LEVEL, NULL,
     None, lnames_THREE_LEVEL},
    {
     {ShiftMask, ShiftMask, vmod_AltMask},
     2,
     1, map_SHIFT_ALT, NULL,
     None, lnames_SHIFT_ALT}
};

#define num_dflt_types ARRAY_SIZE(dflt_types)

static void
initTypeNames(DPYTYPE dpy)
{
    dflt_types[0].name = GET_ATOM(dpy, "ONE_LEVEL");
    lnames_ONE_LEVEL[0] = GET_ATOM(dpy, "Any");
    dflt_types[1].name = GET_ATOM(dpy, "TWO_LEVEL");
    lnames_TWO_LEVEL[0] = GET_ATOM(dpy, "Base");
    lnames_TWO_LEVEL[1] = GET_ATOM(dpy, "Shift");
    dflt_types[2].name = GET_ATOM(dpy, "ALPHABETIC");
    lnames_ALPHABETIC[0] = GET_ATOM(dpy, "Base");
    lnames_ALPHABETIC[1] = GET_ATOM(dpy, "Caps");
    dflt_types[3].name = GET_ATOM(dpy, "KEYPAD");
    lnames_KEYPAD[0] = GET_ATOM(dpy, "Base");
    lnames_KEYPAD[1] = GET_ATOM(dpy, "Number");
    dflt_types[4].name = GET_ATOM(dpy, "PC_BREAK");
    lnames_PC_BREAK[0] = GET_ATOM(dpy, "Base");
    lnames_PC_BREAK[1] = GET_ATOM(dpy, "Control");
    dflt_types[5].name = GET_ATOM(dpy, "PC_SYSRQ");
    lnames_PC_SYSRQ[0] = GET_ATOM(dpy, "Base");
    lnames_PC_SYSRQ[1] = GET_ATOM(dpy, "Alt");
    dflt_types[6].name = GET_ATOM(dpy, "CTRL+ALT");
    lnames_CTRL_ALT[0] = GET_ATOM(dpy, "Base");
    lnames_CTRL_ALT[1] = GET_ATOM(dpy, "Ctrl+Alt");
    dflt_types[7].name = GET_ATOM(dpy, "THREE_LEVEL");
    lnames_THREE_LEVEL[0] = GET_ATOM(dpy, "Base");
    lnames_THREE_LEVEL[1] = GET_ATOM(dpy, "Shift");
    lnames_THREE_LEVEL[2] = GET_ATOM(dpy, "Level3");
    dflt_types[8].name = GET_ATOM(dpy, "SHIFT+ALT");
    lnames_SHIFT_ALT[0] = GET_ATOM(dpy, "Base");
    lnames_SHIFT_ALT[1] = GET_ATOM(dpy, "Shift+Alt");
}

/* compat name is "default" */
static XkbSymInterpretRec dfltSI[69] = {
    {XK_ISO_Level2_Latch, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_Exactly, ShiftMask,
     255,
     {XkbSA_LatchMods, {0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Eisu_Shift, 0x0000,
     XkbSI_Exactly, LockMask,
     255,
     {XkbSA_NoAction, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Eisu_toggle, 0x0000,
     XkbSI_Exactly, LockMask,
     255,
     {XkbSA_NoAction, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Kana_Shift, 0x0000,
     XkbSI_Exactly, LockMask,
     255,
     {XkbSA_NoAction, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Kana_Lock, 0x0000,
     XkbSI_Exactly, LockMask,
     255,
     {XkbSA_NoAction, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Shift_Lock, 0x0000,
     XkbSI_AnyOf, ShiftMask | LockMask,
     255,
     {XkbSA_LockMods, {0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Num_Lock, 0x0000,
     XkbSI_AnyOf, 0xff,
     0,
     {XkbSA_LockMods, {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}}},
    {XK_Alt_L, 0x0000,
     XkbSI_AnyOf, 0xff,
     1,
     {XkbSA_SetMods, {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Alt_R, 0x0000,
     XkbSI_AnyOf, 0xff,
     1,
     {XkbSA_SetMods, {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Scroll_Lock, 0x0000,
     XkbSI_AnyOf, 0xff,
     4,
     {XkbSA_LockMods, {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Lock, 0x0000,
     XkbSI_AnyOf, 0xff,
     255,
     {XkbSA_ISOLock, {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Level3_Shift, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOf, 0xff,
     2,
     {XkbSA_SetMods, {0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00}}},
    {XK_ISO_Level3_Latch, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOf, 0xff,
     2,
     {XkbSA_LatchMods, {0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00}}},
    {XK_Mode_switch, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOfOrNone, 0xff,
     3,
     {XkbSA_SetGroup, {0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_1, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_End, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_2, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_Down, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_3, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_Next, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00}}},
    {XK_KP_4, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Left, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_6, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Right, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_7, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_Home, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_8, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_Up, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_9, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_Prior, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_MovePtr, {0x00, 0x00, 0x01, 0xff, 0xff, 0x00, 0x00}}},
    {XK_KP_5, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Begin, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_F1, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Divide, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_F2, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Multiply, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_F3, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Subtract, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x04, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Separator, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Add, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_0, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Insert, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Decimal, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_KP_Delete, 0x0001,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Button_Dflt, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Button1, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Button2, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Button3, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_DblClick_Dflt, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_DblClick1, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_DblClick2, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_DblClick3, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_PtrBtn, {0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Drag_Dflt, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Drag1, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Drag2, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_Drag3, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockPtrBtn, {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_EnableKeys, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockControls, {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00}}},
    {XK_Pointer_Accelerate, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockControls, {0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00}}},
    {XK_Pointer_DfltBtnNext, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}},
    {XK_Pointer_DfltBtnPrev, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_SetPtrDflt, {0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00}}},
    {XK_AccessX_Enable, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockControls, {0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00}}},
    {XK_Terminate_Server, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_Terminate, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Group_Latch, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOfOrNone, 0xff,
     3,
     {XkbSA_LatchGroup, {0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Next_Group, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOfOrNone, 0xff,
     3,
     {XkbSA_LockGroup, {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Prev_Group, 0x0000,
     XkbSI_LevelOneOnly | XkbSI_AnyOfOrNone, 0xff,
     3,
     {XkbSA_LockGroup, {0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_First_Group, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockGroup, {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {XK_ISO_Last_Group, 0x0000,
     XkbSI_AnyOfOrNone, 0xff,
     255,
     {XkbSA_LockGroup, {0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {NoSymbol, 0x0000,
     XkbSI_Exactly, LockMask,
     255,
     {XkbSA_LockMods, {0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00}}},
    {NoSymbol, 0x0000,
     XkbSI_AnyOf, 0xff,
     255,
     {XkbSA_SetMods, {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}
};

#define num_dfltSI ARRAY_SIZE(dfltSI)

static XkbCompatMapRec compatMap = {
    dfltSI,
    {                           /* group compatibility */
     {0, 0, 0},
     {0, 0, vmod_AltGrMask},
     {0, 0, vmod_AltGrMask},
     {0, 0, vmod_AltGrMask}
     },
    num_dfltSI, num_dfltSI
};

static void
initIndicatorNames(DPYTYPE dpy, XkbDescPtr xkb)
{
    xkb->names->indicators[0] = GET_ATOM(dpy, "Caps Lock");
    xkb->names->indicators[1] = GET_ATOM(dpy, "Num Lock");
    xkb->names->indicators[2] = GET_ATOM(dpy, "Shift Lock");
    xkb->names->indicators[3] = GET_ATOM(dpy, "Mouse Keys");
    xkb->names->indicators[4] = GET_ATOM(dpy, "Scroll Lock");
    xkb->names->indicators[5] = GET_ATOM(dpy, "Group 2");
}
#endif                          /* DEFAULT_H */
