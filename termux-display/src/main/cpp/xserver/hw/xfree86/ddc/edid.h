/*
 * edid.h: defines to parse an EDID block
 *
 * This file contains all information to interpret a standard EDIC block
 * transmitted by a display device via DDC (Display Data Channel). So far
 * there is no information to deal with optional EDID blocks.
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */

#ifndef _EDID_H_
#define _EDID_H_

#include <X11/Xmd.h>

#ifndef _X_EXPORT
#include <X11/Xfuncproto.h>
#endif

/* read complete EDID record */
#define EDID1_LEN 128
#define BITS_PER_BYTE 9
#define NUM BITS_PER_BYTE*EDID1_LEN
#define HEADER 6

#define STD_TIMINGS 8
#define DET_TIMINGS 4

#ifdef _PARSE_EDID_

/* header: 0x00 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0x00  */
#define HEADER_SECTION 0
#define HEADER_LENGTH 8

/* vendor section */
#define VENDOR_SECTION (HEADER_SECTION + HEADER_LENGTH)
#define V_MANUFACTURER 0
#define V_PROD_ID (V_MANUFACTURER + 2)
#define V_SERIAL (V_PROD_ID + 2)
#define V_WEEK (V_SERIAL + 4)
#define V_YEAR (V_WEEK + 1)
#define VENDOR_LENGTH (V_YEAR + 1)

/* EDID version */
#define VERSION_SECTION (VENDOR_SECTION + VENDOR_LENGTH)
#define V_VERSION 0
#define V_REVISION (V_VERSION + 1)
#define VERSION_LENGTH (V_REVISION + 1)

/* display information */
#define DISPLAY_SECTION (VERSION_SECTION + VERSION_LENGTH)
#define D_INPUT 0
#define D_HSIZE (D_INPUT + 1)
#define D_VSIZE (D_HSIZE + 1)
#define D_GAMMA (D_VSIZE + 1)
#define FEAT_S (D_GAMMA + 1)
#define D_RG_LOW (FEAT_S + 1)
#define D_BW_LOW (D_RG_LOW + 1)
#define D_REDX (D_BW_LOW + 1)
#define D_REDY (D_REDX + 1)
#define D_GREENX (D_REDY + 1)
#define D_GREENY (D_GREENX + 1)
#define D_BLUEX (D_GREENY + 1)
#define D_BLUEY (D_BLUEX + 1)
#define D_WHITEX (D_BLUEY + 1)
#define D_WHITEY (D_WHITEX + 1)
#define DISPLAY_LENGTH (D_WHITEY + 1)

/* supported VESA and other standard timings */
#define ESTABLISHED_TIMING_SECTION (DISPLAY_SECTION + DISPLAY_LENGTH)
#define E_T1 0
#define E_T2 (E_T1 + 1)
#define E_TMANU (E_T2 + 1)
#define E_TIMING_LENGTH (E_TMANU + 1)

/* non predefined standard timings supported by display */
#define STD_TIMING_SECTION (ESTABLISHED_TIMING_SECTION + E_TIMING_LENGTH)
#define STD_TIMING_INFO_LEN 2
#define STD_TIMING_INFO_NUM STD_TIMINGS
#define STD_TIMING_LENGTH (STD_TIMING_INFO_LEN * STD_TIMING_INFO_NUM)

/* detailed timing info of non standard timings */
#define DET_TIMING_SECTION (STD_TIMING_SECTION + STD_TIMING_LENGTH)
#define DET_TIMING_INFO_LEN 18
#define MONITOR_DESC_LEN DET_TIMING_INFO_LEN
#define DET_TIMING_INFO_NUM DET_TIMINGS
#define DET_TIMING_LENGTH (DET_TIMING_INFO_LEN * DET_TIMING_INFO_NUM)

/* number of EDID sections to follow */
#define NO_EDID (DET_TIMING_SECTION + DET_TIMING_LENGTH)
/* one byte checksum */
#define CHECKSUM (NO_EDID + 1)

#if (CHECKSUM != (EDID1_LEN - 1))
#error "EDID1 length != 128!"
#endif

#define SECTION(x,y) (Uchar *)(x + y)
#define GET_ARRAY(y) ((Uchar *)(c + y))
#define GET(y) *(Uchar *)(c + y)

/* extract information from vendor section */
#define _PROD_ID(x) x[0] + (x[1] << 8);
#define PROD_ID _PROD_ID(GET_ARRAY(V_PROD_ID))
#define _SERIAL_NO(x) x[0] + (x[1] << 8) + (x[2] << 16) + (x[3] << 24)
#define SERIAL_NO _SERIAL_NO(GET_ARRAY(V_SERIAL))
#define _YEAR(x) (x & 0xFF) + 1990
#define YEAR _YEAR(GET(V_YEAR))
#define WEEK GET(V_WEEK) & 0xFF
#define _L1(x) ((x[0] & 0x7C) >> 2) + '@'
#define _L2(x) ((x[0] & 0x03) << 3) + ((x[1] & 0xE0) >> 5) + '@'
#define _L3(x) (x[1] & 0x1F) + '@';
#define L1 _L1(GET_ARRAY(V_MANUFACTURER))
#define L2 _L2(GET_ARRAY(V_MANUFACTURER))
#define L3 _L3(GET_ARRAY(V_MANUFACTURER))

/* extract information from version section */
#define VERSION GET(V_VERSION)
#define REVISION GET(V_REVISION)

/* extract information from display section */
#define _INPUT_TYPE(x) ((x & 0x80) >> 7)
#define INPUT_TYPE _INPUT_TYPE(GET(D_INPUT))
#define _INPUT_VOLTAGE(x) ((x & 0x60) >> 5)
#define INPUT_VOLTAGE _INPUT_VOLTAGE(GET(D_INPUT))
#define _SETUP(x) ((x & 0x10) >> 4)
#define SETUP _SETUP(GET(D_INPUT))
#define _SYNC(x) (x  & 0x0F)
#define SYNC _SYNC(GET(D_INPUT))
#define _DFP(x) (x & 0x01)
#define DFP _DFP(GET(D_INPUT))
#define _BPC(x) ((x & 0x70) >> 4)
#define BPC _BPC(GET(D_INPUT))
#define _DIGITAL_INTERFACE(x) (x & 0x0F)
#define DIGITAL_INTERFACE _DIGITAL_INTERFACE(GET(D_INPUT))
#define _GAMMA(x) (x == 0xff ? 0.0 : ((x + 100.0)/100.0))
#define GAMMA _GAMMA(GET(D_GAMMA))
#define HSIZE_MAX GET(D_HSIZE)
#define VSIZE_MAX GET(D_VSIZE)
#define _DPMS(x) ((x & 0xE0) >> 5)
#define DPMS _DPMS(GET(FEAT_S))
#define _DISPLAY_TYPE(x) ((x & 0x18) >> 3)
#define DISPLAY_TYPE _DISPLAY_TYPE(GET(FEAT_S))
#define _MSC(x) (x & 0x7)
#define MSC _MSC(GET(FEAT_S))

/* color characteristics */
#define CC_L(x,y) ((x & (0x03 << y)) >> y)
#define CC_H(x) (x << 2)
#define I_CC(x,y,z) CC_H(y) | CC_L(x,z)
#define F_CC(x) ((x)/1024.0)
#define REDX F_CC(I_CC((GET(D_RG_LOW)),(GET(D_REDX)),6))
#define REDY F_CC(I_CC((GET(D_RG_LOW)),(GET(D_REDY)),4))
#define GREENX F_CC(I_CC((GET(D_RG_LOW)),(GET(D_GREENX)),2))
#define GREENY F_CC(I_CC((GET(D_RG_LOW)),(GET(D_GREENY)),0))
#define BLUEX F_CC(I_CC((GET(D_BW_LOW)),(GET(D_BLUEX)),6))
#define BLUEY F_CC(I_CC((GET(D_BW_LOW)),(GET(D_BLUEY)),4))
#define WHITEX F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEX)),2))
#define WHITEY F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEY)),0))

/* extract information from standard timing section */
#define T1 GET(E_T1)
#define T2 GET(E_T2)
#define T_MANU GET(E_TMANU)

/* extract information from established timing section */
#define _VALID_TIMING(x) !(((x[0] == 0x01) && (x[1] == 0x01)) \
                        || ((x[0] == 0x00) && (x[1] == 0x00)) \
                        || ((x[0] == 0x20) && (x[1] == 0x20)) )
#define VALID_TIMING _VALID_TIMING(c)
#define _HSIZE1(x) ((x[0] + 31) * 8)
#define HSIZE1 _HSIZE1(c)
#define RATIO(x) ((x[1] & 0xC0) >> 6)
#define RATIO1_1 0
/* EDID Ver. 1.3 redefined this */
#define RATIO16_10 RATIO1_1
#define RATIO4_3 1
#define RATIO5_4 2
#define RATIO16_9 3
#define _VSIZE1(x,y,r) switch(RATIO(x)){ \
  case RATIO1_1: y =  ((v->version > 1 || v->revision > 2) \
		       ? (_HSIZE1(x) * 10) / 16 : _HSIZE1(x)); break; \
  case RATIO4_3: y = _HSIZE1(x) * 3 / 4; break; \
  case RATIO5_4: y = _HSIZE1(x) * 4 / 5; break; \
  case RATIO16_9: y = _HSIZE1(x) * 9 / 16; break; \
  }
#define VSIZE1(x) _VSIZE1(c,x,v)
#define _REFRESH_R(x) (x[1] & 0x3F) + 60
#define REFRESH_R  _REFRESH_R(c)
#define _ID_LOW(x) x[0]
#define ID_LOW _ID_LOW(c)
#define _ID_HIGH(x) (x[1] << 8)
#define ID_HIGH _ID_HIGH(c)
#define STD_TIMING_ID (ID_LOW | ID_HIGH)
#define _NEXT_STD_TIMING(x)  (x = (x + STD_TIMING_INFO_LEN))
#define NEXT_STD_TIMING _NEXT_STD_TIMING(c)

/* EDID Ver. >= 1.2 */
/**
 * Returns true if the pointer is the start of a monitor descriptor block
 * instead of a detailed timing descriptor.
 *
 * Checking the reserved pad fields for zeroes fails on some monitors with
 * broken empty ASCII strings.  Only the first two bytes are reliable.
 */
#define _IS_MONITOR_DESC(x) (x[0] == 0 && x[1] == 0)
#define IS_MONITOR_DESC _IS_MONITOR_DESC(c)
#define _PIXEL_CLOCK(x) (x[0] + (x[1] << 8)) * 10000
#define PIXEL_CLOCK _PIXEL_CLOCK(c)
#define _H_ACTIVE(x) (x[2] + ((x[4] & 0xF0) << 4))
#define H_ACTIVE _H_ACTIVE(c)
#define _H_BLANK(x) (x[3] + ((x[4] & 0x0F) << 8))
#define H_BLANK _H_BLANK(c)
#define _V_ACTIVE(x) (x[5] + ((x[7] & 0xF0) << 4))
#define V_ACTIVE _V_ACTIVE(c)
#define _V_BLANK(x) (x[6] + ((x[7] & 0x0F) << 8))
#define V_BLANK _V_BLANK(c)
#define _H_SYNC_OFF(x) (x[8] + ((x[11] & 0xC0) << 2))
#define H_SYNC_OFF _H_SYNC_OFF(c)
#define _H_SYNC_WIDTH(x) (x[9] + ((x[11] & 0x30) << 4))
#define H_SYNC_WIDTH _H_SYNC_WIDTH(c)
#define _V_SYNC_OFF(x) ((x[10] >> 4) + ((x[11] & 0x0C) << 2))
#define V_SYNC_OFF _V_SYNC_OFF(c)
#define _V_SYNC_WIDTH(x) ((x[10] & 0x0F) + ((x[11] & 0x03) << 4))
#define V_SYNC_WIDTH _V_SYNC_WIDTH(c)
#define _H_SIZE(x) (x[12] + ((x[14] & 0xF0) << 4))
#define H_SIZE _H_SIZE(c)
#define _V_SIZE(x) (x[13] + ((x[14] & 0x0F) << 8))
#define V_SIZE _V_SIZE(c)
#define _H_BORDER(x) (x[15])
#define H_BORDER _H_BORDER(c)
#define _V_BORDER(x) (x[16])
#define V_BORDER _V_BORDER(c)
#define _INTERLACED(x) ((x[17] & 0x80) >> 7)
#define INTERLACED _INTERLACED(c)
#define _STEREO(x) ((x[17] & 0x60) >> 5)
#define STEREO _STEREO(c)
#define _STEREO1(x) (x[17] & 0x1)
#define STEREO1 _STEREO(c)
#define _SYNC_T(x) ((x[17] & 0x18) >> 3)
#define SYNC_T _SYNC_T(c)
#define _MISC(x) ((x[17] & 0x06) >> 1)
#define MISC _MISC(c)

#define _MONITOR_DESC_TYPE(x) x[3]
#define MONITOR_DESC_TYPE _MONITOR_DESC_TYPE(c)
#define SERIAL_NUMBER 0xFF
#define ASCII_STR 0xFE
#define MONITOR_RANGES 0xFD
#define _MIN_V_OFFSET(x) ((!!(x[4] & 0x01)) * 255)
#define _MAX_V_OFFSET(x) ((!!(x[4] & 0x02)) * 255)
#define _MIN_H_OFFSET(x) ((!!(x[4] & 0x04)) * 255)
#define _MAX_H_OFFSET(x) ((!!(x[4] & 0x08)) * 255)
#define _MIN_V(x) x[5]
#define MIN_V (_MIN_V(c) + _MIN_V_OFFSET(c))
#define _MAX_V(x) x[6]
#define MAX_V (_MAX_V(c) + _MAX_V_OFFSET(c))
#define _MIN_H(x) x[7]
#define MIN_H (_MIN_H(c) + _MIN_H_OFFSET(c))
#define _MAX_H(x) x[8]
#define MAX_H (_MAX_H(c) + _MAX_H_OFFSET(c))
#define _MAX_CLOCK(x) x[9]
#define MAX_CLOCK _MAX_CLOCK(c)
#define _DEFAULT_GTF(x) (x[10] == 0x00)
#define DEFAULT_GTF _DEFAULT_GTF(c)
#define _RANGE_LIMITS_ONLY(x) (x[10] == 0x01)
#define RANGE_LIMITS_ONLY _RANGE_LIMITS_ONLY(c)
#define _HAVE_2ND_GTF(x) (x[10] == 0x02)
#define HAVE_2ND_GTF _HAVE_2ND_GTF(c)
#define _F_2ND_GTF(x) (x[12] * 2)
#define F_2ND_GTF _F_2ND_GTF(c)
#define _C_2ND_GTF(x) (x[13] / 2)
#define C_2ND_GTF _C_2ND_GTF(c)
#define _M_2ND_GTF(x) (x[14] + (x[15] << 8))
#define M_2ND_GTF _M_2ND_GTF(c)
#define _K_2ND_GTF(x) (x[16])
#define K_2ND_GTF _K_2ND_GTF(c)
#define _J_2ND_GTF(x) (x[17] / 2)
#define J_2ND_GTF _J_2ND_GTF(c)
#define _HAVE_CVT(x) (x[10] == 0x04)
#define HAVE_CVT _HAVE_CVT(c)
#define _MAX_CLOCK_KHZ(x) (x[12] >> 2)
#define MAX_CLOCK_KHZ (MAX_CLOCK * 10000) - (_MAX_CLOCK_KHZ(c) * 250)
#define _MAXWIDTH(x) ((x[13] == 0 ? 0 : x[13] + ((x[12] & 0x03) << 8)) * 8)
#define MAXWIDTH _MAXWIDTH(c)
#define _SUPPORTED_ASPECT(x) x[14]
#define SUPPORTED_ASPECT _SUPPORTED_ASPECT(c)
#define  SUPPORTED_ASPECT_4_3   0x80
#define  SUPPORTED_ASPECT_16_9  0x40
#define  SUPPORTED_ASPECT_16_10 0x20
#define  SUPPORTED_ASPECT_5_4   0x10
#define  SUPPORTED_ASPECT_15_9  0x08
#define _PREFERRED_ASPECT(x) ((x[15] & 0xe0) >> 5)
#define PREFERRED_ASPECT _PREFERRED_ASPECT(c)
#define  PREFERRED_ASPECT_4_3   0
#define  PREFERRED_ASPECT_16_9  1
#define  PREFERRED_ASPECT_16_10 2
#define  PREFERRED_ASPECT_5_4   3
#define  PREFERRED_ASPECT_15_9  4
#define _SUPPORTED_BLANKING(x) ((x[15] & 0x18) >> 3)
#define SUPPORTED_BLANKING _SUPPORTED_BLANKING(c)
#define  CVT_STANDARD 0x01
#define  CVT_REDUCED  0x02
#define _SUPPORTED_SCALING(x) ((x[16] & 0xf0) >> 4)
#define SUPPORTED_SCALING _SUPPORTED_SCALING(c)
#define  SCALING_HSHRINK  0x08
#define  SCALING_HSTRETCH 0x04
#define  SCALING_VSHRINK  0x02
#define  SCALING_VSTRETCH 0x01
#define _PREFERRED_REFRESH(x) x[17]
#define PREFERRED_REFRESH _PREFERRED_REFRESH(c)

#define MONITOR_NAME 0xFC
#define ADD_COLOR_POINT 0xFB
#define WHITEX F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEX)),2))
#define WHITEY F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEY)),0))
#define _WHITEX_ADD(x,y) F_CC(I_CC(((*(x + y))),(*(x + y + 1)),2))
#define _WHITEY_ADD(x,y) F_CC(I_CC(((*(x + y))),(*(x + y + 2)),0))
#define _WHITE_INDEX1(x) x[5]
#define WHITE_INDEX1 _WHITE_INDEX1(c)
#define _WHITE_INDEX2(x) x[10]
#define WHITE_INDEX2 _WHITE_INDEX2(c)
#define WHITEX1 _WHITEX_ADD(c,6)
#define WHITEY1 _WHITEY_ADD(c,6)
#define WHITEX2 _WHITEX_ADD(c,12)
#define WHITEY2 _WHITEY_ADD(c,12)
#define _WHITE_GAMMA1(x) _GAMMA(x[9])
#define WHITE_GAMMA1 _WHITE_GAMMA1(c)
#define _WHITE_GAMMA2(x) _GAMMA(x[14])
#define WHITE_GAMMA2 _WHITE_GAMMA2(c)
#define ADD_STD_TIMINGS 0xFA
#define COLOR_MANAGEMENT_DATA 0xF9
#define CVT_3BYTE_DATA 0xF8
#define ADD_EST_TIMINGS 0xF7
#define ADD_DUMMY 0x10

#define _NEXT_DT_MD_SECTION(x) (x = (x + DET_TIMING_INFO_LEN))
#define NEXT_DT_MD_SECTION _NEXT_DT_MD_SECTION(c)

#endif                          /* _PARSE_EDID_ */

/* input type */
#define DIGITAL(x) x

/* DFP */
#define DFP1(x) x

/* input voltage level */
#define V070 0                  /* 0.700V/0.300V */
#define V071 1                  /* 0.714V/0.286V */
#define V100 2                  /* 1.000V/0.400V */
#define V007 3                  /* 0.700V/0.000V */

/* Signal level setup */
#define SIG_SETUP(x) (x)

/* sync characteristics */
#define SEP_SYNC(x) (x & 0x08)
#define COMP_SYNC(x) (x & 0x04)
#define SYNC_O_GREEN(x) (x & 0x02)
#define SYNC_SERR(x) (x & 0x01)

/* DPMS features */
#define DPMS_STANDBY(x) (x & 0x04)
#define DPMS_SUSPEND(x) (x & 0x02)
#define DPMS_OFF(x) (x & 0x01)

/* display type, analog */
#define DISP_MONO 0
#define DISP_RGB 1
#define DISP_MULTCOLOR 2

/* display color encodings, digital */
#define DISP_YCRCB444 0x01
#define DISP_YCRCB422 0x02

/* Msc stuff EDID Ver > 1.1 */
#define STD_COLOR_SPACE(x) (x & 0x4)
#define PREFERRED_TIMING_MODE(x) (x & 0x2)
#define GFT_SUPPORTED(x) (x & 0x1)
#define GTF_SUPPORTED(x) (x & 0x1)
#define CVT_SUPPORTED(x) (x & 0x1)

/* detailed timing misc */
#define IS_INTERLACED(x)  (x)
#define IS_STEREO(x)  (x)
#define IS_RIGHT_STEREO(x) (x & 0x01)
#define IS_LEFT_STEREO(x) (x & 0x02)
#define IS_4WAY_STEREO(x) (x & 0x03)
#define IS_RIGHT_ON_SYNC(x) IS_RIGHT_STEREO(x)
#define IS_LEFT_ON_SYNC(x) IS_LEFT_STEREO(x)

typedef unsigned int Uint;
typedef unsigned char Uchar;

struct vendor {
    char name[4];
    int prod_id;
    Uint serial;
    int week;
    int year;
};

struct edid_version {
    int version;
    int revision;
};

struct disp_features {
    unsigned int input_type:1;
    unsigned int input_voltage:2;
    unsigned int input_setup:1;
    unsigned int input_sync:5;
    unsigned int input_dfp:1;
    unsigned int input_bpc:3;
    unsigned int input_interface:4;
    /* 15 bit hole */
    int hsize;
    int vsize;
    float gamma;
    unsigned int dpms:3;
    unsigned int display_type:2;
    unsigned int msc:3;
    float redx;
    float redy;
    float greenx;
    float greeny;
    float bluex;
    float bluey;
    float whitex;
    float whitey;
};

struct established_timings {
    Uchar t1;
    Uchar t2;
    Uchar t_manu;
};

struct std_timings {
    int hsize;
    int vsize;
    int refresh;
    CARD16 id;
};

struct detailed_timings {
    int clock;
    int h_active;
    int h_blanking;
    int v_active;
    int v_blanking;
    int h_sync_off;
    int h_sync_width;
    int v_sync_off;
    int v_sync_width;
    int h_size;
    int v_size;
    int h_border;
    int v_border;
    unsigned int interlaced:1;
    unsigned int stereo:2;
    unsigned int sync:2;
    unsigned int misc:2;
    unsigned int stereo_1:1;
};

#define DT 0
#define DS_SERIAL 0xFF
#define DS_ASCII_STR 0xFE
#define DS_NAME 0xFC
#define DS_RANGES 0xFD
#define DS_WHITE_P 0xFB
#define DS_STD_TIMINGS 0xFA
#define DS_CMD 0xF9
#define DS_CVT 0xF8
#define DS_EST_III 0xF7
#define DS_DUMMY 0x10
#define DS_UNKOWN 0x100         /* type is an int */
#define DS_VENDOR 0x101
#define DS_VENDOR_MAX 0x110

/*
 * Display range limit Descriptor of EDID version1, reversion 4
 */
typedef enum {
	DR_DEFAULT_GTF,
	DR_LIMITS_ONLY,
	DR_SECONDARY_GTF,
	DR_CVT_SUPPORTED = 4,
} DR_timing_flags;

struct monitor_ranges {
    int min_v;
    int max_v;
    int min_h;
    int max_h;
    int max_clock;              /* in mhz */
    int gtf_2nd_f;
    int gtf_2nd_c;
    int gtf_2nd_m;
    int gtf_2nd_k;
    int gtf_2nd_j;
    int max_clock_khz;
    int maxwidth;               /* in pixels */
    char supported_aspect;
    char preferred_aspect;
    char supported_blanking;
    char supported_scaling;
    int preferred_refresh;      /* in hz */
    DR_timing_flags display_range_timing_flags;
};

struct whitePoints {
    int index;
    float white_x;
    float white_y;
    float white_gamma;
};

struct cvt_timings {
    int width;
    int height;
    int rate;
    int rates;
};

/*
 * Be careful when adding new sections; this structure can't grow, it's
 * embedded in the middle of xf86Monitor which is ABI.  Sizes below are
 * in bytes, for ILP32 systems.  If all else fails just copy the section
 * literally like serial and friends.
 */
struct detailed_monitor_section {
    int type;
    union {
        struct detailed_timings d_timings;      /* 56 */
        Uchar serial[13];
        Uchar ascii_data[13];
        Uchar name[13];
        struct monitor_ranges ranges;   /* 60 */
        struct std_timings std_t[5];    /* 80 */
        struct whitePoints wp[2];       /* 32 */
        /* color management data */
        struct cvt_timings cvt[4];      /* 64 */
        Uchar est_iii[6];       /* 6 */
    } section;                  /* max: 80 */
};

/* flags */
#define MONITOR_EDID_COMPLETE_RAWDATA	0x01
/* old, don't use */
#define EDID_COMPLETE_RAWDATA		0x01

/*
 * For DisplayID devices, only the scrnIndex, flags, and rawData fields
 * are meaningful.  For EDID, they all are.
 */
typedef struct {
    int scrnIndex;
    struct vendor vendor;
    struct edid_version ver;
    struct disp_features features;
    struct established_timings timings1;
    struct std_timings timings2[8];
    struct detailed_monitor_section det_mon[4];
    unsigned long flags;
    int no_sections;
    Uchar *rawData;
} xf86Monitor, *xf86MonPtr;

extern _X_EXPORT xf86MonPtr ConfiguredMonitor;

#define EXT_TAG 0
#define EXT_REV 1
#define CEA_EXT   0x02
#define VTB_EXT   0x10
#define DI_EXT    0x40
#define LS_EXT    0x50
#define MI_EXT    0x60

#define CEA_EXT_MIN_DATA_OFFSET 4
#define CEA_EXT_MAX_DATA_OFFSET 127
#define CEA_EXT_DET_TIMING_NUM 6

#define IEEE_ID_HDMI    0x000C03
#define CEA_AUDIO_BLK   1
#define CEA_VIDEO_BLK   2
#define CEA_VENDOR_BLK  3
#define CEA_SPEAKER_ALLOC_BLK 4
#define CEA_VESA_DTC_BLK 5
#define VENDOR_SUPPORT_AI(x) ((x) >> 7)
#define VENDOR_SUPPORT_DC_48bit(x)  ( ( (x) >> 6) & 0x01)
#define VENDOR_SUPPORT_DC_36bit(x)  ( ( (x) >> 5) & 0x01)
#define VENDOR_SUPPORT_DC_30bit(x)  ( ( (x) >> 4) & 0x01)
#define VENDOR_SUPPORT_DC_Y444(x)   ( ( (x) >> 3) & 0x01)
#define VENDOR_LATENCY_PRESENT(x)     ( (x) >> 7)
#define VENDOR_LATENCY_PRESENT_I(x) ( ( (x) >> 6) & 0x01)
#define HDMI_MAX_TMDS_UNIT   (5000)

struct cea_video_block {
    Uchar video_code;
};

struct cea_audio_block_descriptor {
    Uchar audio_code[3];
};

struct cea_audio_block {
    struct cea_audio_block_descriptor descriptor[10];
};

struct cea_vendor_block_hdmi {
    Uchar portB:4;
    Uchar portA:4;
    Uchar portD:4;
    Uchar portC:4;
    Uchar support_flags;
    Uchar max_tmds_clock;
    Uchar latency_present;
    Uchar video_latency;
    Uchar audio_latency;
    Uchar interlaced_video_latency;
    Uchar interlaced_audio_latency;
};

struct cea_vendor_block {
    unsigned char ieee_id[3];
    union {
        struct cea_vendor_block_hdmi hdmi;
        /* any other vendor blocks we know about */
    };
};

struct cea_speaker_block {
    Uchar FLR:1;
    Uchar LFE:1;
    Uchar FC:1;
    Uchar RLR:1;
    Uchar RC:1;
    Uchar FLRC:1;
    Uchar RLRC:1;
    Uchar FLRW:1;
    Uchar FLRH:1;
    Uchar TC:1;
    Uchar FCH:1;
    Uchar Resv:5;
    Uchar ResvByte;
};

struct cea_data_block {
    Uchar len:5;
    Uchar tag:3;
    union {
        struct cea_video_block video;
        struct cea_audio_block audio;
        struct cea_vendor_block vendor;
        struct cea_speaker_block speaker;
    } u;
};

struct cea_ext_body {
    Uchar tag;
    Uchar rev;
    Uchar dt_offset;
    Uchar flags;
    struct cea_data_block data_collection;
};

#endif                          /* _EDID_H_ */
