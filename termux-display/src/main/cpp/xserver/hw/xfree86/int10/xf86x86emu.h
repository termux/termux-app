/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef XF86X86EMU_H_
#define XF86X86EMU_H_
#include <x86emu.h>

#define M _X86EMU_env

#define X86_EAX M.x86.R_EAX
#define X86_EBX M.x86.R_EBX
#define X86_ECX M.x86.R_ECX
#define X86_EDX M.x86.R_EDX
#define X86_ESI M.x86.R_ESI
#define X86_EDI M.x86.R_EDI
#define X86_EBP M.x86.R_EBP
#define X86_EIP M.x86.R_EIP
#define X86_ESP M.x86.R_ESP
#define X86_EFLAGS M.x86.R_EFLG

#define X86_FLAGS M.x86.R_FLG
#define X86_AX M.x86.R_AX
#define X86_BX M.x86.R_BX
#define X86_CX M.x86.R_CX
#define X86_DX M.x86.R_DX
#define X86_SI M.x86.R_SI
#define X86_DI M.x86.R_DI
#define X86_BP M.x86.R_BP
#define X86_IP M.x86.R_IP
#define X86_SP M.x86.R_SP
#define X86_CS M.x86.R_CS
#define X86_DS M.x86.R_DS
#define X86_ES M.x86.R_ES
#define X86_SS M.x86.R_SS
#define X86_FS M.x86.R_FS
#define X86_GS M.x86.R_GS

#define X86_AL M.x86.R_AL
#define X86_BL M.x86.R_BL
#define X86_CL M.x86.R_CL
#define X86_DL M.x86.R_DL

#define X86_AH M.x86.R_AH
#define X86_BH M.x86.R_BH
#define X86_CH M.x86.R_CH
#define X86_DH M.x86.R_DH

#endif
