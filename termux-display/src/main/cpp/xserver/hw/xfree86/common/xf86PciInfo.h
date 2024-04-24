
/*
 * Copyright (c) 1995-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains macros for the PCI Vendor and Device IDs for video
 * cards plus a few other things that are needed in drivers or elsewhere.
 * This information is used in several ways:
 *   1. It is used by drivers and/or other code.
 *   2. It is used by the pciid2c.pl script to determine what vendor data to
 *      include in the pcidata module that the X server loads.
 *   3. A side-effect of 2. affects how config-generation works for
 *      otherwise "unknown" cards.
 *
 * Don't add entries here for vendors that don't make video cards,
 * or for non-video devices unless they're needed by a driver or elsewhere.
 * A comprehensive set of PCI vendor, device and subsystem data is
 * auto-generated from the ../etc/pci.ids file using the pciids2c.pl script,
 * and is used in scanpci utility.  Don't modify the pci.ids file.  If
 * new/corrected entries are required, add them to ../etc/extrapci.ids.
 */

#ifndef _XF86_PCIINFO_H
#define _XF86_PCIINFO_H

#warning "xf86PciInfo.h is deprecated.  For greater compatibility, drivers should include necessary PCI IDs locally rather than relying on this file from xorg-server."

/* PCI Pseudo Vendor */
#define PCI_VENDOR_GENERIC		0x00FF

#define PCI_VENDOR_REAL3D		0x003D
#define PCI_VENDOR_COMPAQ		0x0E11
#define PCI_VENDOR_ATI			0x1002
#define PCI_VENDOR_AVANCE		0x1005
#define PCI_VENDOR_TSENG		0x100C
#define PCI_VENDOR_NS			0x100B
#define PCI_VENDOR_WEITEK		0x100E
#define PCI_VENDOR_VIDEOLOGIC		0x1010
#define PCI_VENDOR_DIGITAL		0x1011
#define PCI_VENDOR_CIRRUS		0x1013
#define PCI_VENDOR_AMD			0x1022
#define PCI_VENDOR_TRIDENT		0x1023
#define PCI_VENDOR_ALI			0x1025
#define PCI_VENDOR_DELL			0x1028
#define PCI_VENDOR_MATROX		0x102B
#define PCI_VENDOR_CHIPSTECH		0x102C
#define PCI_VENDOR_MIRO			0x1031
#define PCI_VENDOR_NEC			0x1033
#define PCI_VENDOR_SIS			0x1039
#define PCI_VENDOR_HP			0x103C
#define PCI_VENDOR_SGS			0x104A
#define PCI_VENDOR_TI			0x104C
#define PCI_VENDOR_SONY			0x104D
#define PCI_VENDOR_OAK			0x104E
#define PCI_VENDOR_MOTOROLA		0x1057
#define PCI_VENDOR_NUMNINE		0x105D
#define PCI_VENDOR_CYRIX		0x1078
#define PCI_VENDOR_SUN			0x108E
#define PCI_VENDOR_DIAMOND		0x1092
#define PCI_VENDOR_BROOKTREE		0x109E
#define PCI_VENDOR_NEOMAGIC		0x10C8
#define PCI_VENDOR_NVIDIA		0x10DE
#define PCI_VENDOR_IMS			0x10E0
#define PCI_VENDOR_INTEGRAPHICS 	0x10EA
#define PCI_VENDOR_ALLIANCE		0x1142
#define PCI_VENDOR_RENDITION		0x1163
#define PCI_VENDOR_3DFX			0x121A
#define PCI_VENDOR_SMI			0x126F
#define PCI_VENDOR_TRITECH		0x1292
#define PCI_VENDOR_NVIDIA_SGS		0x12D2
#define PCI_VENDOR_VMWARE		0x15AD
#define PCI_VENDOR_AST			0x1A03
#define PCI_VENDOR_3DLABS		0x3D3D
#define PCI_VENDOR_AVANCE_2		0x4005
#define PCI_VENDOR_HERCULES		0x4843
#define PCI_VENDOR_S3			0x5333
#define PCI_VENDOR_INTEL		0x8086
#define PCI_VENDOR_ARK			0xEDD8

/* Generic */
#define PCI_CHIP_VGA			0x0000
#define PCI_CHIP_8514			0x0001

/* Real 3D */
#define PCI_CHIP_I740_PCI		0x00D1

/* Compaq */
#define PCI_CHIP_QV1280			0x3033

/* ATI */
#define PCI_CHIP_RV380_3150             0x3150
#define PCI_CHIP_RV380_3151             0x3151
#define PCI_CHIP_RV380_3152             0x3152
#define PCI_CHIP_RV380_3153             0x3153
#define PCI_CHIP_RV380_3154             0x3154
#define PCI_CHIP_RV380_3156             0x3156
#define PCI_CHIP_RV380_3E50             0x3E50
#define PCI_CHIP_RV380_3E51             0x3E51
#define PCI_CHIP_RV380_3E52             0x3E52
#define PCI_CHIP_RV380_3E53             0x3E53
#define PCI_CHIP_RV380_3E54             0x3E54
#define PCI_CHIP_RV380_3E56             0x3E56
#define PCI_CHIP_RS100_4136		0x4136
#define PCI_CHIP_RS200_4137		0x4137
#define PCI_CHIP_R300_AD		0x4144
#define PCI_CHIP_R300_AE		0x4145
#define PCI_CHIP_R300_AF		0x4146
#define PCI_CHIP_R300_AG		0x4147
#define PCI_CHIP_R350_AH                0x4148
#define PCI_CHIP_R350_AI                0x4149
#define PCI_CHIP_R350_AJ                0x414A
#define PCI_CHIP_R350_AK                0x414B
#define PCI_CHIP_RV350_AP               0x4150
#define PCI_CHIP_RV350_AQ               0x4151
#define PCI_CHIP_RV360_AR               0x4152
#define PCI_CHIP_RV350_AS               0x4153
#define PCI_CHIP_RV350_AT               0x4154
#define PCI_CHIP_RV350_4155             0x4155
#define PCI_CHIP_RV350_AV               0x4156
#define PCI_CHIP_MACH32			0x4158
#define PCI_CHIP_RS250_4237		0x4237
#define PCI_CHIP_R200_BB		0x4242
#define PCI_CHIP_R200_BC		0x4243
#define PCI_CHIP_RS100_4336		0x4336
#define PCI_CHIP_RS200_4337		0x4337
#define PCI_CHIP_MACH64CT		0x4354
#define PCI_CHIP_MACH64CX		0x4358
#define PCI_CHIP_RS250_4437		0x4437
#define PCI_CHIP_MACH64ET		0x4554
#define PCI_CHIP_MACH64GB		0x4742
#define PCI_CHIP_MACH64GD		0x4744
#define PCI_CHIP_MACH64GI		0x4749
#define PCI_CHIP_MACH64GL		0x474C
#define PCI_CHIP_MACH64GM		0x474D
#define PCI_CHIP_MACH64GN		0x474E
#define PCI_CHIP_MACH64GO		0x474F
#define PCI_CHIP_MACH64GP		0x4750
#define PCI_CHIP_MACH64GQ		0x4751
#define PCI_CHIP_MACH64GR		0x4752
#define PCI_CHIP_MACH64GS		0x4753
#define PCI_CHIP_MACH64GT		0x4754
#define PCI_CHIP_MACH64GU		0x4755
#define PCI_CHIP_MACH64GV		0x4756
#define PCI_CHIP_MACH64GW		0x4757
#define PCI_CHIP_MACH64GX		0x4758
#define PCI_CHIP_MACH64GY		0x4759
#define PCI_CHIP_MACH64GZ		0x475A
#define PCI_CHIP_RV250_Id		0x4964
#define PCI_CHIP_RV250_Ie		0x4965
#define PCI_CHIP_RV250_If		0x4966
#define PCI_CHIP_RV250_Ig		0x4967
#define PCI_CHIP_R420_JH                0x4A48
#define PCI_CHIP_R420_JI                0x4A49
#define PCI_CHIP_R420_JJ                0x4A4A
#define PCI_CHIP_R420_JK                0x4A4B
#define PCI_CHIP_R420_JL                0x4A4C
#define PCI_CHIP_R420_JM                0x4A4D
#define PCI_CHIP_R420_JN                0x4A4E
#define PCI_CHIP_R420_4A4F              0x4A4F
#define PCI_CHIP_R420_JP                0x4A50
#define PCI_CHIP_R420_4A54              0x4A54
#define PCI_CHIP_R481_4B49              0x4B49
#define PCI_CHIP_R481_4B4A              0x4B4A
#define PCI_CHIP_R481_4B4B              0x4B4B
#define PCI_CHIP_R481_4B4C              0x4B4C
#define PCI_CHIP_MACH64LB		0x4C42
#define PCI_CHIP_MACH64LD		0x4C44
#define PCI_CHIP_RAGE128LE		0x4C45
#define PCI_CHIP_RAGE128LF		0x4C46
#define PCI_CHIP_MACH64LG		0x4C47
#define PCI_CHIP_MACH64LI		0x4C49
#define PCI_CHIP_MACH64LM		0x4C4D
#define PCI_CHIP_MACH64LN		0x4C4E
#define PCI_CHIP_MACH64LP		0x4C50
#define PCI_CHIP_MACH64LQ		0x4C51
#define PCI_CHIP_MACH64LR		0x4C52
#define PCI_CHIP_MACH64LS		0x4C53
#define PCI_CHIP_RADEON_LW		0x4C57
#define PCI_CHIP_RADEON_LX		0x4C58
#define PCI_CHIP_RADEON_LY		0x4C59
#define PCI_CHIP_RADEON_LZ		0x4C5A
#define PCI_CHIP_RV250_Ld		0x4C64
#define PCI_CHIP_RV250_Le		0x4C65
#define PCI_CHIP_RV250_Lf		0x4C66
#define PCI_CHIP_RV250_Lg		0x4C67
#define PCI_CHIP_RV250_Ln		0x4C6E
#define PCI_CHIP_RAGE128MF		0x4D46
#define PCI_CHIP_RAGE128ML		0x4D4C
#define PCI_CHIP_R300_ND		0x4E44
#define PCI_CHIP_R300_NE		0x4E45
#define PCI_CHIP_R300_NF		0x4E46
#define PCI_CHIP_R300_NG		0x4E47
#define PCI_CHIP_R350_NH                0x4E48
#define PCI_CHIP_R350_NI                0x4E49
#define PCI_CHIP_R360_NJ                0x4E4A
#define PCI_CHIP_R350_NK                0x4E4B
#define PCI_CHIP_RV350_NP               0x4E50
#define PCI_CHIP_RV350_NQ               0x4E51
#define PCI_CHIP_RV350_NR               0x4E52
#define PCI_CHIP_RV350_NS               0x4E53
#define PCI_CHIP_RV350_NT               0x4E54
#define PCI_CHIP_RV350_NV               0x4E56
#define PCI_CHIP_RAGE128PA		0x5041
#define PCI_CHIP_RAGE128PB		0x5042
#define PCI_CHIP_RAGE128PC		0x5043
#define PCI_CHIP_RAGE128PD		0x5044
#define PCI_CHIP_RAGE128PE		0x5045
#define PCI_CHIP_RAGE128PF		0x5046
#define PCI_CHIP_RAGE128PG		0x5047
#define PCI_CHIP_RAGE128PH		0x5048
#define PCI_CHIP_RAGE128PI		0x5049
#define PCI_CHIP_RAGE128PJ		0x504A
#define PCI_CHIP_RAGE128PK		0x504B
#define PCI_CHIP_RAGE128PL		0x504C
#define PCI_CHIP_RAGE128PM		0x504D
#define PCI_CHIP_RAGE128PN		0x504E
#define PCI_CHIP_RAGE128PO		0x504F
#define PCI_CHIP_RAGE128PP		0x5050
#define PCI_CHIP_RAGE128PQ		0x5051
#define PCI_CHIP_RAGE128PR		0x5052
#define PCI_CHIP_RAGE128PS		0x5053
#define PCI_CHIP_RAGE128PT		0x5054
#define PCI_CHIP_RAGE128PU		0x5055
#define PCI_CHIP_RAGE128PV		0x5056
#define PCI_CHIP_RAGE128PW		0x5057
#define PCI_CHIP_RAGE128PX		0x5058
#define PCI_CHIP_RADEON_QD		0x5144
#define PCI_CHIP_RADEON_QE		0x5145
#define PCI_CHIP_RADEON_QF		0x5146
#define PCI_CHIP_RADEON_QG		0x5147
#define PCI_CHIP_R200_QH		0x5148
#define PCI_CHIP_R200_QI		0x5149
#define PCI_CHIP_R200_QJ		0x514A
#define PCI_CHIP_R200_QK		0x514B
#define PCI_CHIP_R200_QL		0x514C
#define PCI_CHIP_R200_QM		0x514D
#define PCI_CHIP_R200_QN		0x514E
#define PCI_CHIP_R200_QO		0x514F
#define PCI_CHIP_RV200_QW		0x5157
#define PCI_CHIP_RV200_QX		0x5158
#define PCI_CHIP_RV100_QY		0x5159
#define PCI_CHIP_RV100_QZ		0x515A
#define PCI_CHIP_RN50_515E		0x515E
#define PCI_CHIP_RAGE128RE		0x5245
#define PCI_CHIP_RAGE128RF		0x5246
#define PCI_CHIP_RAGE128RG		0x5247
#define PCI_CHIP_RAGE128RK		0x524B
#define PCI_CHIP_RAGE128RL		0x524C
#define PCI_CHIP_RAGE128SE		0x5345
#define PCI_CHIP_RAGE128SF		0x5346
#define PCI_CHIP_RAGE128SG		0x5347
#define PCI_CHIP_RAGE128SH		0x5348
#define PCI_CHIP_RAGE128SK		0x534B
#define PCI_CHIP_RAGE128SL		0x534C
#define PCI_CHIP_RAGE128SM		0x534D
#define PCI_CHIP_RAGE128SN		0x534E
#define PCI_CHIP_RAGE128TF		0x5446
#define PCI_CHIP_RAGE128TL		0x544C
#define PCI_CHIP_RAGE128TR		0x5452
#define PCI_CHIP_RAGE128TS		0x5453
#define PCI_CHIP_RAGE128TT		0x5454
#define PCI_CHIP_RAGE128TU		0x5455
#define PCI_CHIP_RV370_5460             0x5460
#define PCI_CHIP_RV370_5461             0x5461
#define PCI_CHIP_RV370_5462             0x5462
#define PCI_CHIP_RV370_5463             0x5463
#define PCI_CHIP_RV370_5464             0x5464
#define PCI_CHIP_RV370_5465             0x5465
#define PCI_CHIP_RV370_5466             0x5466
#define PCI_CHIP_RV370_5467             0x5467
#define PCI_CHIP_R423_UH                0x5548
#define PCI_CHIP_R423_UI                0x5549
#define PCI_CHIP_R423_UJ                0x554A
#define PCI_CHIP_R423_UK                0x554B
#define PCI_CHIP_R430_554C              0x554C
#define PCI_CHIP_R430_554D              0x554D
#define PCI_CHIP_R430_554E              0x554E
#define PCI_CHIP_R430_554F              0x554F
#define PCI_CHIP_R423_5550              0x5550
#define PCI_CHIP_R423_UQ                0x5551
#define PCI_CHIP_R423_UR                0x5552
#define PCI_CHIP_R423_UT                0x5554
#define PCI_CHIP_RV410_564A             0x564A
#define PCI_CHIP_RV410_564B             0x564B
#define PCI_CHIP_RV410_564F             0x564F
#define PCI_CHIP_RV410_5652             0x5652
#define PCI_CHIP_RV410_5653             0x5653
#define PCI_CHIP_MACH64VT		0x5654
#define PCI_CHIP_MACH64VU		0x5655
#define PCI_CHIP_MACH64VV		0x5656
#define PCI_CHIP_RS300_5834		0x5834
#define PCI_CHIP_RS300_5835		0x5835
#define PCI_CHIP_RS300_5836		0x5836
#define PCI_CHIP_RS300_5837		0x5837
#define PCI_CHIP_RS480_5954             0x5954
#define PCI_CHIP_RS480_5955             0x5955
#define PCI_CHIP_RV280_5960		0x5960
#define PCI_CHIP_RV280_5961		0x5961
#define PCI_CHIP_RV280_5962		0x5962
#define PCI_CHIP_RV280_5964		0x5964
#define PCI_CHIP_RV280_5965 		0x5965
#define PCI_CHIP_RN50_5969		0x5969
#define PCI_CHIP_RS482_5974             0x5974
#define PCI_CHIP_RS482_5975             0x5975
#define PCI_CHIP_RS400_5A41             0x5A41
#define PCI_CHIP_RS400_5A42             0x5A42
#define PCI_CHIP_RC410_5A61             0x5A61
#define PCI_CHIP_RC410_5A62             0x5A62
#define PCI_CHIP_RV370_5B60             0x5B60
#define PCI_CHIP_RV370_5B61             0x5B61
#define PCI_CHIP_RV370_5B62             0x5B62
#define PCI_CHIP_RV370_5B63             0x5B63
#define PCI_CHIP_RV370_5B64             0x5B64
#define PCI_CHIP_RV370_5B65             0x5B65
#define PCI_CHIP_RV370_5B66             0x5B66
#define PCI_CHIP_RV370_5B67             0x5B67
#define PCI_CHIP_RV280_5C61		0x5C61
#define PCI_CHIP_RV280_5C63		0x5C63
#define PCI_CHIP_R430_5D48              0x5D48
#define PCI_CHIP_R430_5D49              0x5D49
#define PCI_CHIP_R430_5D4A              0x5D4A
#define PCI_CHIP_R480_5D4C              0x5D4C
#define PCI_CHIP_R480_5D4D              0x5D4D
#define PCI_CHIP_R480_5D4E              0x5D4E
#define PCI_CHIP_R480_5D4F              0x5D4F
#define PCI_CHIP_R480_5D50              0x5D50
#define PCI_CHIP_R480_5D52              0x5D52
#define PCI_CHIP_R423_5D57              0x5D57
#define PCI_CHIP_RV410_5E48             0x5E48
#define PCI_CHIP_RV410_5E4A             0x5E4A
#define PCI_CHIP_RV410_5E4B             0x5E4B
#define PCI_CHIP_RV410_5E4C             0x5E4C
#define PCI_CHIP_RV410_5E4D             0x5E4D
#define PCI_CHIP_RV410_5E4F             0x5E4F
#define PCI_CHIP_RS350_7834             0x7834
#define PCI_CHIP_RS350_7835             0x7835

/* ASPEED Technology (AST) */
#define PCI_CHIP_AST2000		0x2000

/* Avance Logic */
#define PCI_CHIP_ALG2064		0x2064
#define PCI_CHIP_ALG2301		0x2301
#define PCI_CHIP_ALG2501		0x2501

/* Tseng */
#define PCI_CHIP_ET4000_W32P_A		0x3202
#define PCI_CHIP_ET4000_W32P_B		0x3205
#define PCI_CHIP_ET4000_W32P_D		0x3206
#define PCI_CHIP_ET4000_W32P_C		0x3207
#define PCI_CHIP_ET6000			0x3208
#define PCI_CHIP_ET6300			0x4702

/* Weitek */
#define PCI_CHIP_P9000			0x9001
#define PCI_CHIP_P9100			0x9100

/* Digital */
#define PCI_CHIP_DC21050		0x0001
#define PCI_CHIP_DEC21030		0x0004
#define PCI_CHIP_TGA2			0x000D

/* Cirrus Logic */
#define PCI_CHIP_GD7548			0x0038
#define PCI_CHIP_GD7555			0x0040
#define PCI_CHIP_GD5430			0x00A0
#define PCI_CHIP_GD5434_4		0x00A4
#define PCI_CHIP_GD5434_8		0x00A8
#define PCI_CHIP_GD5436			0x00AC
#define PCI_CHIP_GD5446			0x00B8
#define PCI_CHIP_GD5480			0x00BC
#define PCI_CHIP_GD5462			0x00D0
#define PCI_CHIP_GD5464			0x00D4
#define PCI_CHIP_GD5464BD		0x00D5
#define PCI_CHIP_GD5465			0x00D6
#define PCI_CHIP_6729			0x1100
#define PCI_CHIP_6832			0x1110
#define PCI_CHIP_GD7542			0x1200
#define PCI_CHIP_GD7543			0x1202
#define PCI_CHIP_GD7541			0x1204

/* AMD */
#define PCI_CHIP_AMD761			0x700E

/* Trident */
#define PCI_CHIP_2100			0x2100
#define PCI_CHIP_8400			0x8400
#define PCI_CHIP_8420			0x8420
#define PCI_CHIP_8500			0x8500
#define PCI_CHIP_8520			0x8520
#define PCI_CHIP_8600			0x8600
#define PCI_CHIP_8620			0x8620
#define PCI_CHIP_8820			0x8820
#define PCI_CHIP_9320			0x9320
#define PCI_CHIP_9388			0x9388
#define PCI_CHIP_9397			0x9397
#define PCI_CHIP_939A			0x939A
#define PCI_CHIP_9420			0x9420
#define PCI_CHIP_9440			0x9440
#define PCI_CHIP_9520			0x9520
#define PCI_CHIP_9525			0x9525
#define PCI_CHIP_9540			0x9540
#define PCI_CHIP_9660			0x9660
#define PCI_CHIP_9750			0x9750
#define PCI_CHIP_9850			0x9850
#define PCI_CHIP_9880			0x9880
#define PCI_CHIP_9910			0x9910

/* ALI */
#define PCI_CHIP_M1435			0x1435

/* Matrox */
#define PCI_CHIP_MGA2085		0x0518
#define PCI_CHIP_MGA2064		0x0519
#define PCI_CHIP_MGA1064		0x051A
#define PCI_CHIP_MGA2164		0x051B
#define PCI_CHIP_MGA2164_AGP		0x051F
#define PCI_CHIP_MGAG200_PCI		0x0520
#define PCI_CHIP_MGAG200		0x0521
#define PCI_CHIP_MGAG400		0x0525
#define PCI_CHIP_MGAG550		0x2527
#define PCI_CHIP_IMPRESSION		0x0D10
#define PCI_CHIP_MGAG100_PCI		0x1000
#define PCI_CHIP_MGAG100		0x1001

#define PCI_CARD_G400_TH		0x2179
#define PCI_CARD_MILL_G200_SD		0xFF00
#define PCI_CARD_PROD_G100_SD		0xFF01
#define PCI_CARD_MYST_G200_SD		0xFF02
#define PCI_CARD_MILL_G200_SG		0xFF03
#define PCI_CARD_MARV_G200_SD		0xFF04

/* Chips & Tech */
#define PCI_CHIP_65545			0x00D8
#define PCI_CHIP_65548			0x00DC
#define PCI_CHIP_65550			0x00E0
#define PCI_CHIP_65554			0x00E4
#define PCI_CHIP_65555			0x00E5
#define PCI_CHIP_68554			0x00F4
#define PCI_CHIP_69000			0x00C0
#define PCI_CHIP_69030			0x0C30

/* Miro */
#define PCI_CHIP_ZR36050		0x5601

/* NEC */
#define PCI_CHIP_POWER_VR		0x0046

/* SiS */
#define PCI_CHIP_SG86C201		0x0001
#define PCI_CHIP_SG86C202		0x0002
#define PCI_CHIP_SG85C503		0x0008
#define PCI_CHIP_SIS5597		0x0200
/* Agregado por Carlos Duclos & Manuel Jander */
#define PCI_CHIP_SIS82C204		0x0204
#define PCI_CHIP_SG86C205		0x0205
#define PCI_CHIP_SG86C215		0x0215
#define PCI_CHIP_SG86C225		0x0225
#define PCI_CHIP_85C501			0x0406
#define PCI_CHIP_85C496			0x0496
#define PCI_CHIP_85C601			0x0601
#define PCI_CHIP_85C5107		0x5107
#define PCI_CHIP_85C5511		0x5511
#define PCI_CHIP_85C5513		0x5513
#define PCI_CHIP_SIS5571		0x5571
#define PCI_CHIP_SIS5597_2		0x5597
#define PCI_CHIP_SIS530			0x6306
#define PCI_CHIP_SIS6326		0x6326
#define PCI_CHIP_SIS7001		0x7001
#define PCI_CHIP_SIS300			0x0300
#define PCI_CHIP_SIS315H		0x0310
#define PCI_CHIP_SIS315PRO		0x0325
#define PCI_CHIP_SIS330			0x0330
#define PCI_CHIP_SIS630			0x6300
#define PCI_CHIP_SIS540			0x5300
#define PCI_CHIP_SIS550			0x5315
#define PCI_CHIP_SIS650			0x6325
#define PCI_CHIP_SIS730			0x7300

/* Hewlett-Packard */
#define PCI_CHIP_ELROY			0x1054
#define PCI_CHIP_ZX1_SBA		0x1229
#define PCI_CHIP_ZX1_IOC		0x122A
#define PCI_CHIP_ZX1_LBA		0x122E  /* a.k.a. Mercury */
#define PCI_CHIP_ZX1_AGP8		0x12B4  /* a.k.a. QuickSilver */
#define PCI_CHIP_ZX2_LBA		0x12EE
#define PCI_CHIP_ZX2_SBA		0x4030
#define PCI_CHIP_ZX2_IOC		0x4031
#define PCI_CHIP_ZX2_PCIE		0x4037

/* SGS */
#define PCI_CHIP_STG2000		0x0008
#define PCI_CHIP_STG1764		0x0009
#define PCI_CHIP_KYROII			0x0010

/* Texas Instruments */
#define PCI_CHIP_TI_PERMEDIA		0x3D04
#define PCI_CHIP_TI_PERMEDIA2		0x3D07

/* Oak */
#define PCI_CHIP_OTI107			0x0107

/* Number Nine */
#define PCI_CHIP_I128			0x2309
#define PCI_CHIP_I128_2			0x2339
#define PCI_CHIP_I128_T2R		0x493D
#define PCI_CHIP_I128_T2R4		0x5348

/* Sun */
#define PCI_CHIP_EBUS			0x1000
#define PCI_CHIP_HAPPY_MEAL		0x1001
#define PCI_CHIP_SIMBA			0x5000
#define PCI_CHIP_PSYCHO			0x8000
#define PCI_CHIP_SCHIZO			0x8001
#define PCI_CHIP_SABRE			0xA000
#define PCI_CHIP_HUMMINGBIRD		0xA001

/* BrookTree */
#define PCI_CHIP_BT848			0x0350
#define PCI_CHIP_BT849			0x0351

/* NVIDIA */
#define PCI_CHIP_NV1			0x0008
#define PCI_CHIP_DAC64			0x0009
#define PCI_CHIP_TNT			0x0020
#define PCI_CHIP_TNT2			0x0028
#define PCI_CHIP_UTNT2			0x0029
#define PCI_CHIP_VTNT2			0x002C
#define PCI_CHIP_UVTNT2			0x002D
#define PCI_CHIP_ITNT2			0x00A0
#define PCI_CHIP_GEFORCE_256		0x0100
#define PCI_CHIP_GEFORCE_DDR		0x0101
#define PCI_CHIP_QUADRO			0x0103
#define PCI_CHIP_GEFORCE2_MX		0x0110
#define PCI_CHIP_GEFORCE2_MX_100	0x0111
#define PCI_CHIP_GEFORCE2_GO		0x0112
#define PCI_CHIP_QUADRO2_MXR		0x0113
#define PCI_CHIP_GEFORCE2_GTS		0x0150
#define PCI_CHIP_GEFORCE2_TI		0x0151
#define PCI_CHIP_GEFORCE2_ULTRA		0x0152
#define PCI_CHIP_QUADRO2_PRO		0x0153
#define PCI_CHIP_GEFORCE4_MX_460	0x0170
#define PCI_CHIP_GEFORCE4_MX_440	0x0171
#define PCI_CHIP_GEFORCE4_MX_420	0x0172
#define PCI_CHIP_GEFORCE4_440_GO	0x0174
#define PCI_CHIP_GEFORCE4_420_GO	0x0175
#define PCI_CHIP_GEFORCE4_420_GO_M32	0x0176
#define PCI_CHIP_QUADRO4_500XGL		0x0178
#define PCI_CHIP_GEFORCE4_440_GO_M64	0x0179
#define PCI_CHIP_QUADRO4_200		0x017A
#define PCI_CHIP_QUADRO4_550XGL		0x017B
#define PCI_CHIP_QUADRO4_500_GOGL	0x017C
#define PCI_CHIP_IGEFORCE2		0x01A0
#define PCI_CHIP_GEFORCE3		0x0200
#define PCI_CHIP_GEFORCE3_TI_200	0x0201
#define PCI_CHIP_GEFORCE3_TI_500	0x0202
#define PCI_CHIP_QUADRO_DCC		0x0203
#define PCI_CHIP_GEFORCE4_TI_4600	0x0250
#define PCI_CHIP_GEFORCE4_TI_4400	0x0251
#define PCI_CHIP_GEFORCE4_TI_4200	0x0253
#define PCI_CHIP_QUADRO4_900XGL		0x0258
#define PCI_CHIP_QUADRO4_750XGL		0x0259
#define PCI_CHIP_QUADRO4_700XGL		0x025B

/* NVIDIA & SGS */
#define PCI_CHIP_RIVA128		0x0018

/* IMS */
#define PCI_CHIP_IMSTT128		0x9128
#define PCI_CHIP_IMSTT3D		0x9135

/* Alliance Semiconductor */
#define PCI_CHIP_AP6410			0x3210
#define PCI_CHIP_AP6422			0x6422
#define PCI_CHIP_AT24			0x6424
#define PCI_CHIP_AT3D			0x643D

/* 3dfx Interactive */
#define PCI_CHIP_VOODOO_GRAPHICS	0x0001
#define PCI_CHIP_VOODOO2		0x0002
#define PCI_CHIP_BANSHEE		0x0003
#define PCI_CHIP_VOODOO3		0x0005
#define PCI_CHIP_VOODOO5		0x0009

#define PCI_CARD_VOODOO3_2000		0x0036
#define PCI_CARD_VOODOO3_3000		0x003A

/* Rendition */
#define PCI_CHIP_V1000			0x0001
#define PCI_CHIP_V2x00			0x2000

/* 3Dlabs */
#define PCI_CHIP_300SX			0x0001
#define PCI_CHIP_500TX			0x0002
#define PCI_CHIP_DELTA			0x0003
#define PCI_CHIP_PERMEDIA		0x0004
#define PCI_CHIP_MX			0x0006
#define PCI_CHIP_PERMEDIA2		0x0007
#define PCI_CHIP_GAMMA			0x0008
#define PCI_CHIP_PERMEDIA2V		0x0009
#define PCI_CHIP_PERMEDIA3		0x000A
#define PCI_CHIP_PERMEDIA4		0x000C
#define PCI_CHIP_R4			0x000D
#define PCI_CHIP_GAMMA2			0x000E
#define PCI_CHIP_R4ALT			0x0011

/* S3 */
#define PCI_CHIP_PLATO			0x0551
#define PCI_CHIP_VIRGE			0x5631
#define PCI_CHIP_TRIO			0x8811
#define PCI_CHIP_AURORA64VP		0x8812
#define PCI_CHIP_TRIO64UVP		0x8814
#define PCI_CHIP_VIRGE_VX		0x883D
#define PCI_CHIP_868			0x8880
#define PCI_CHIP_928			0x88B0
#define PCI_CHIP_864_0			0x88C0
#define PCI_CHIP_864_1			0x88C1
#define PCI_CHIP_964_0			0x88D0
#define PCI_CHIP_964_1			0x88D1
#define PCI_CHIP_968			0x88F0
#define PCI_CHIP_TRIO64V2_DXGX		0x8901
#define PCI_CHIP_PLATO_PX		0x8902
#define PCI_CHIP_Trio3D			0x8904
#define PCI_CHIP_VIRGE_DXGX		0x8A01
#define PCI_CHIP_VIRGE_GX2		0x8A10
#define PCI_CHIP_Trio3D_2X		0x8A13
#define PCI_CHIP_SAVAGE3D		0x8A20
#define PCI_CHIP_SAVAGE3D_MV		0x8A21
#define PCI_CHIP_SAVAGE4		0x8A22
#define PCI_CHIP_PROSAVAGE_PM		0x8A25
#define PCI_CHIP_PROSAVAGE_KM		0x8A26
#define PCI_CHIP_VIRGE_MX		0x8C01
#define PCI_CHIP_VIRGE_MXPLUS		0x8C02
#define PCI_CHIP_VIRGE_MXP		0x8C03
#define PCI_CHIP_SAVAGE_MX_MV		0x8C10
#define PCI_CHIP_SAVAGE_MX		0x8C11
#define PCI_CHIP_SAVAGE_IX_MV		0x8C12
#define PCI_CHIP_SAVAGE_IX		0x8C13
#define PCI_CHIP_SUPSAV_MX128		0x8C22
#define PCI_CHIP_SUPSAV_MX64		0x8C24
#define PCI_CHIP_SUPSAV_MX64C		0x8C26
#define PCI_CHIP_SUPSAV_IX128SDR	0x8C2A
#define PCI_CHIP_SUPSAV_IX128DDR	0x8C2B
#define PCI_CHIP_SUPSAV_IX64SDR		0x8C2C
#define PCI_CHIP_SUPSAV_IX64DDR		0x8C2D
#define PCI_CHIP_SUPSAV_IXCSDR		0x8C2E
#define PCI_CHIP_SUPSAV_IXCDDR		0x8C2F
#define PCI_CHIP_S3TWISTER_P		0x8D01
#define PCI_CHIP_S3TWISTER_K		0x8D02
#define PCI_CHIP_PROSAVAGE_DDR		0x8D03
#define PCI_CHIP_PROSAVAGE_DDRK		0x8D04
#define PCI_CHIP_SAVAGE2000		0x9102

/* ARK Logic */
#define PCI_CHIP_1000PV			0xA091
#define PCI_CHIP_2000PV			0xA099
#define PCI_CHIP_2000MT			0xA0A1
#define PCI_CHIP_2000MI			0xA0A9

/* Tritech Microelectronics */
#define PCI_CHIP_TR25202		0xFC02

/* Neomagic */
#define PCI_CHIP_NM2070			0x0001
#define PCI_CHIP_NM2090			0x0002
#define PCI_CHIP_NM2093			0x0003
#define PCI_CHIP_NM2097			0x0083
#define PCI_CHIP_NM2160			0x0004
#define PCI_CHIP_NM2200			0x0005
#define PCI_CHIP_NM2230			0x0025
#define PCI_CHIP_NM2360			0x0006
#define PCI_CHIP_NM2380			0x0016

/* Intel */
#define PCI_CHIP_I815_BRIDGE		0x1130
#define PCI_CHIP_I815			0x1132
#define PCI_CHIP_82801_P2P		0x244E
#define PCI_CHIP_845_G_BRIDGE		0x2560
#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M_BRIDGE		0x3575
#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_I810_BRIDGE		0x7120
#define PCI_CHIP_I810			0x7121
#define PCI_CHIP_I810_DC100_BRIDGE	0x7122
#define PCI_CHIP_I810_DC100		0x7123
#define PCI_CHIP_I810_E_BRIDGE		0x7124
#define PCI_CHIP_I810_E			0x7125
#define PCI_CHIP_I740_AGP		0x7800
#define PCI_CHIP_460GX_PXB		0x84CB
#define PCI_CHIP_460GX_SAC		0x84E0
#define PCI_CHIP_460GX_GXB_2		0x84E2  /* PCI function 2 */
#define PCI_CHIP_460GX_WXB		0x84E6
#define PCI_CHIP_460GX_GXB_1		0x84EA  /* PCI function 1 */

/* Silicon Motion Inc. */
#define PCI_CHIP_SMI910			0x0910
#define PCI_CHIP_SMI810			0x0810
#define PCI_CHIP_SMI820			0x0820
#define PCI_CHIP_SMI710			0x0710
#define PCI_CHIP_SMI712			0x0712
#define PCI_CHIP_SMI720			0x0720
#define PCI_CHIP_SMI731			0x0730

/* VMware */
#define PCI_CHIP_VMWARE0405		0x0405
#define PCI_CHIP_VMWARE0710		0x0710

#endif                          /* _XF86_PCIINFO_H */
