/*
 * copied from from linux kernel 2.2.4
 * removed internal stuff (#ifdef __KERNEL__)
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _LINUX_FB_H
#define _LINUX_FB_H

#include <asm/types.h>

/* Definitions of frame buffers						*/

#define FB_MAJOR	29

#define FB_MODES_SHIFT		5       /* 32 modes per framebuffer */
#define FB_NUM_MINORS		256     /* 256 Minors               */
#define FB_MAX			(FB_NUM_MINORS / (1 << FB_MODES_SHIFT))
#define GET_FB_IDX(node)	(MINOR(node) >> FB_MODES_SHIFT)

/* ioctls
   0x46 is 'F'								*/
#define FBIOGET_VSCREENINFO	0x4600
#define FBIOPUT_VSCREENINFO	0x4601
#define FBIOGET_FSCREENINFO	0x4602
#define FBIOGETCMAP		0x4604
#define FBIOPUTCMAP		0x4605
#define FBIOPAN_DISPLAY		0x4606
/* 0x4607-0x460B are defined below */
/* #define FBIOGET_MONITORSPEC	0x460C */
/* #define FBIOPUT_MONITORSPEC	0x460D */
/* #define FBIOSWITCH_MONIBIT	0x460E */
#define FBIOGET_CON2FBMAP	0x460F
#define FBIOPUT_CON2FBMAP	0x4610
#define FBIOBLANK		0x4611

#define FB_TYPE_PACKED_PIXELS		0       /* Packed Pixels        */
#define FB_TYPE_PLANES			1       /* Non interleaved planes */
#define FB_TYPE_INTERLEAVED_PLANES	2       /* Interleaved planes   */
#define FB_TYPE_TEXT			3       /* Text/attributes      */

#define FB_AUX_TEXT_MDA		0       /* Monochrome text */
#define FB_AUX_TEXT_CGA		1       /* CGA/EGA/VGA Color text */
#define FB_AUX_TEXT_S3_MMIO	2       /* S3 MMIO fasttext */
#define FB_AUX_TEXT_MGA_STEP16	3       /* MGA Millenium I: text, attr, 14 reserved bytes */
#define FB_AUX_TEXT_MGA_STEP8	4       /* other MGAs:      text, attr,  6 reserved bytes */

#define FB_VISUAL_MONO01		0       /* Monochr. 1=Black 0=White */
#define FB_VISUAL_MONO10		1       /* Monochr. 1=White 0=Black */
#define FB_VISUAL_TRUECOLOR		2       /* True color   */
#define FB_VISUAL_PSEUDOCOLOR		3       /* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR		4       /* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR	5       /* Pseudo color readonly */

#define FB_ACCEL_NONE		0       /* no hardware accelerator      */
#define FB_ACCEL_ATARIBLITT	1       /* Atari Blitter                */
#define FB_ACCEL_AMIGABLITT	2       /* Amiga Blitter                */
#define FB_ACCEL_S3_TRIO64	3       /* Cybervision64 (S3 Trio64)    */
#define FB_ACCEL_NCR_77C32BLT	4       /* RetinaZ3 (NCR 77C32BLT)      */
#define FB_ACCEL_S3_VIRGE	5       /* Cybervision64/3D (S3 ViRGE)  */
#define FB_ACCEL_ATI_MACH64GX	6       /* ATI Mach 64GX family         */
#define FB_ACCEL_DEC_TGA	7       /* DEC 21030 TGA                */
#define FB_ACCEL_ATI_MACH64CT	8       /* ATI Mach 64CT family         */
#define FB_ACCEL_ATI_MACH64VT	9       /* ATI Mach 64CT family VT class */
#define FB_ACCEL_ATI_MACH64GT	10      /* ATI Mach 64CT family GT class */
#define FB_ACCEL_SUN_CREATOR	11      /* Sun Creator/Creator3D        */
#define FB_ACCEL_SUN_CGSIX	12      /* Sun cg6                      */
#define FB_ACCEL_SUN_LEO	13      /* Sun leo/zx                   */
#define FB_ACCEL_IMS_TWINTURBO	14      /* IMS Twin Turbo               */
#define FB_ACCEL_3DLABS_PERMEDIA2 15    /* 3Dlabs Permedia 2            */
#define FB_ACCEL_MATROX_MGA2064W 16     /* Matrox MGA2064W (Millenium)  */
#define FB_ACCEL_MATROX_MGA1064SG 17    /* Matrox MGA1064SG (Mystique)  */
#define FB_ACCEL_MATROX_MGA2164W 18     /* Matrox MGA2164W (Millenium II) */
#define FB_ACCEL_MATROX_MGA2164W_AGP 19 /* Matrox MGA2164W (Millenium II) */
#define FB_ACCEL_MATROX_MGAG100	20      /* Matrox G100 (Productiva G100) */
#define FB_ACCEL_MATROX_MGAG200	21      /* Matrox G200 (Myst, Mill, ...) */
#define FB_ACCEL_SUN_CG14	22      /* Sun cgfourteen                */
#define FB_ACCEL_SUN_BWTWO	23      /* Sun bwtwo                     */
#define FB_ACCEL_SUN_CGTHREE	24      /* Sun cgthree                   */
#define FB_ACCEL_SUN_TCX	25      /* Sun tcx                       */
#define FB_ACCEL_MATROX_MGAG400	26      /* Matrox G400                  */
#define FB_ACCEL_NV3		27      /* nVidia RIVA 128              */
#define FB_ACCEL_NV4		28      /* nVidia RIVA TNT              */
#define FB_ACCEL_NV5		29      /* nVidia RIVA TNT2             */
#define FB_ACCEL_CT_6555x	30      /* C&T 6555x                    */
#define FB_ACCEL_3DFX_BANSHEE	31      /* 3Dfx Banshee                 */
#define FB_ACCEL_ATI_RAGE128	32      /* ATI Rage128 family           */

struct fb_fix_screeninfo {
    char id[16];                /* identification string eg "TT Builtin" */
    char *smem_start;           /* Start of frame buffer mem */
    /* (physical address) */
    __u32 smem_len;             /* Length of frame buffer mem */
    __u32 type;                 /* see FB_TYPE_*                */
    __u32 type_aux;             /* Interleave for interleaved Planes */
    __u32 visual;               /* see FB_VISUAL_*              */
    __u16 xpanstep;             /* zero if no hardware panning  */
    __u16 ypanstep;             /* zero if no hardware panning  */
    __u16 ywrapstep;            /* zero if no hardware ywrap    */
    __u32 line_length;          /* length of a line in bytes    */
    char *mmio_start;           /* Start of Memory Mapped I/O   */
    /* (physical address) */
    __u32 mmio_len;             /* Length of Memory Mapped I/O  */
    __u32 accel;                /* Type of acceleration available */
    __u16 reserved[3];          /* Reserved for future compatibility */
};

/* Interpretation of offset for color fields: All offsets are from the right,
 * inside a "pixel" value, which is exactly 'bits_per_pixel' wide (means: you
 * can use the offset as right argument to <<). A pixel afterwards is a bit
 * stream and is written to video memory as that unmodified. This implies
 * big-endian byte order if bits_per_pixel is greater than 8.
 */
struct fb_bitfield {
    __u32 offset;               /* beginning of bitfield        */
    __u32 length;               /* length of bitfield           */
    __u32 msb_right;            /* != 0 : Most significant bit is */
    /* right */
};

#define FB_NONSTD_HAM		1       /* Hold-And-Modify (HAM)        */

#define FB_ACTIVATE_NOW		0       /* set values immediately (or vbl) */
#define FB_ACTIVATE_NXTOPEN	1       /* activate on next open        */
#define FB_ACTIVATE_TEST	2       /* don't set, round up impossible */
#define FB_ACTIVATE_MASK       15
                                        /* values                       */
#define FB_ACTIVATE_VBL	       16       /* activate values on next vbl  */
#define FB_CHANGE_CMAP_VBL     32       /* change colormap on vbl       */
#define FB_ACTIVATE_ALL	       64       /* change all VCs on this fb    */

#define FB_ACCELF_TEXT		1       /* text mode acceleration */

#define FB_SYNC_HOR_HIGH_ACT	1       /* horizontal sync high active  */
#define FB_SYNC_VERT_HIGH_ACT	2       /* vertical sync high active    */
#define FB_SYNC_EXT		4       /* external sync                */
#define FB_SYNC_COMP_HIGH_ACT	8       /* composite sync high active   */
#define FB_SYNC_BROADCAST	16      /* broadcast video timings      */
                                        /* vtotal = 144d/288n/576i => PAL  */
                                        /* vtotal = 121d/242n/484i => NTSC */
#define FB_SYNC_ON_GREEN	32      /* sync on green */

#define FB_VMODE_NONINTERLACED  0       /* non interlaced */
#define FB_VMODE_INTERLACED	1       /* interlaced   */
#define FB_VMODE_DOUBLE		2       /* double scan */
#define FB_VMODE_MASK		255

#define FB_VMODE_YWRAP		256     /* ywrap instead of panning     */
#define FB_VMODE_SMOOTH_XPAN	512     /* smooth xpan possible (internally used) */
#define FB_VMODE_CONUPDATE	512     /* don't update x/yoffset       */

struct fb_var_screeninfo {
    __u32 xres;                 /* visible resolution           */
    __u32 yres;
    __u32 xres_virtual;         /* virtual resolution           */
    __u32 yres_virtual;
    __u32 xoffset;              /* offset from virtual to visible */
    __u32 yoffset;              /* resolution                   */

    __u32 bits_per_pixel;       /* guess what                   */
    __u32 grayscale;            /* != 0 Graylevels instead of colors */

    struct fb_bitfield red;     /* bitfield in fb mem if true color, */
    struct fb_bitfield green;   /* else only length is significant */
    struct fb_bitfield blue;
    struct fb_bitfield transp;  /* transparency                 */

    __u32 nonstd;               /* != 0 Non standard pixel format */

    __u32 activate;             /* see FB_ACTIVATE_*            */

    __u32 height;               /* height of picture in mm    */
    __u32 width;                /* width of picture in mm     */

    __u32 accel_flags;          /* acceleration flags (hints)   */

    /* Timing: All values in pixclocks, except pixclock (of course) */
    __u32 pixclock;             /* pixel clock in ps (pico seconds) */
    __u32 left_margin;          /* time from sync to picture    */
    __u32 right_margin;         /* time from picture to sync    */
    __u32 upper_margin;         /* time from sync to picture    */
    __u32 lower_margin;
    __u32 hsync_len;            /* length of horizontal sync    */
    __u32 vsync_len;            /* length of vertical sync      */
    __u32 sync;                 /* see FB_SYNC_*                */
    __u32 vmode;                /* see FB_VMODE_*               */
    __u32 reserved[6];          /* Reserved for future compatibility */
};

struct fb_cmap {
    __u32 start;                /* First entry  */
    __u32 len;                  /* Number of entries */
    __u16 *red;                 /* Red values   */
    __u16 *green;
    __u16 *blue;
    __u16 *transp;              /* transparency, can be NULL */
};

struct fb_con2fbmap {
    __u32 console;
    __u32 framebuffer;
};

struct fb_monspecs {
    __u32 hfmin;                /* hfreq lower limit (Hz) */
    __u32 hfmax;                /* hfreq upper limit (Hz) */
    __u16 vfmin;                /* vfreq lower limit (Hz) */
    __u16 vfmax;                /* vfreq upper limit (Hz) */
    unsigned dpms:1;            /* supports DPMS */
};

#if 1

#define FBCMD_GET_CURRENTPAR	0xDEAD0005
#define FBCMD_SET_CURRENTPAR	0xDEAD8005

#endif

#if 1                           /* Preliminary */

   /*
    *    Hardware Cursor
    */

#define FBIOGET_FCURSORINFO     0x4607
#define FBIOGET_VCURSORINFO     0x4608
#define FBIOPUT_VCURSORINFO     0x4609
#define FBIOGET_CURSORSTATE     0x460A
#define FBIOPUT_CURSORSTATE     0x460B

struct fb_fix_cursorinfo {
    __u16 crsr_width;           /* width and height of the cursor in */
    __u16 crsr_height;          /* pixels (zero if no cursor)   */
    __u16 crsr_xsize;           /* cursor size in display pixels */
    __u16 crsr_ysize;
    __u16 crsr_color1;          /* colormap entry for cursor color1 */
    __u16 crsr_color2;          /* colormap entry for cursor color2 */
};

struct fb_var_cursorinfo {
    __u16 width;
    __u16 height;
    __u16 xspot;
    __u16 yspot;
    __u8 data[1];               /* field with [height][width]        */
};

struct fb_cursorstate {
    __s16 xoffset;
    __s16 yoffset;
    __u16 mode;
};

#define FB_CURSOR_OFF		0
#define FB_CURSOR_ON		1
#define FB_CURSOR_FLASH		2

#endif                          /* Preliminary */

#endif                          /* _LINUX_FB_H */
