/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Copyright 1992, 1993 Data General Corporation;
Copyright 1992, 1993 OMRON Corporation

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that the
above copyright notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting documentation, and that
neither the name OMRON or DATA GENERAL be used in advertising or publicity
pertaining to distribution of the software without specific, written prior
permission of the party whose name is to be used.  Neither OMRON or
DATA GENERAL make any representation about the suitability of this software
for any purpose.  It is provided "as is" without express or implied warranty.

OMRON AND DATA GENERAL EACH DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OMRON OR DATA GENERAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

******************************************************************/
#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions
 *
 */

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xdefs.h>

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifndef MAXSCREENS
#define MAXSCREENS	16
#endif
#ifndef MAXGPUSCREENS
#define MAXGPUSCREENS	16
#endif
#define MAXCLIENTS	2048
#define LIMITCLIENTS	256     /* Must be a power of 2 and <= MAXCLIENTS */
#define MAXEXTENSIONS   128
#define MAXFORMATS	8
#ifndef MAXDEVICES
#define MAXDEVICES	256      /* input devices */
#endif
#define GPU_SCREEN_OFFSET 256

/* 128 event opcodes for core + extension events, excluding GE */
#define MAXEVENTS       128
#define EXTENSION_EVENT_BASE 64
#define EXTENSION_BASE 128

typedef uint32_t ATOM;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef _XTYPEDEF_CALLBACKLISTPTR
typedef struct _CallbackList *CallbackListPtr;  /* also in dix.h */

#define _XTYPEDEF_CALLBACKLISTPTR
#endif

typedef struct _xReq *xReqPtr;

#include "os.h"                 /* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */
#include <X11/Xfuncs.h>         /* for bcopy, bzero, and bcmp */

#define NullBox ((BoxPtr)0)
#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)

    /* this next is used with None and ParentRelative to tell
       PaintWin() what to use to paint the background. Also used
       in the macro IS_VALID_PIXMAP */

#define USE_BACKGROUND_PIXEL 3
#define USE_BORDER_PIXEL 3

#undef min
#undef max

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
/* abs() is a function, not a macro; include the file declaring
 * it in case we haven't done that yet.
 */
#include <stdlib.h>
#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
/* this assumes b > 0 */
#define modulus(a, b, d)    if (((d) = (a) % (b)) < 0) (d) += (b)
/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#define lowbit(x) ((x) & (~(x) + 1))

/* XXX Not for modules */
#include <limits.h>
#if !defined(MAXSHORT) || !defined(MINSHORT) || \
    !defined(MAXINT) || !defined(MININT)
/*
 * Some implementations #define these through <math.h>, so preclude
 * #include'ing it later.
 */

#include <math.h>
#undef MAXSHORT
#define MAXSHORT SHRT_MAX
#undef MINSHORT
#define MINSHORT SHRT_MIN
#undef MAXINT
#define MAXINT INT_MAX
#undef MININT
#define MININT INT_MIN

#include <assert.h>
#include <ctype.h>
#include <stdio.h>              /* for fopen, etc... */

#endif

#ifndef PATH_MAX
#include <sys/param.h>
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

/**
 * Calculate the number of bytes needed to hold bits.
 * @param bits The minimum number of bits needed.
 * @return The number of bytes needed to hold bits.
 */
static inline int
bits_to_bytes(const int bits)
{
    return ((bits + 7) >> 3);
}

/**
 * Calculate the number of 4-byte units needed to hold the given number of
 * bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The number of 4-byte units needed to hold bytes.
 */
static inline int
bytes_to_int32(const int bytes)
{
    return (((bytes) + 3) >> 2);
}

/**
 * Calculate the number of bytes (in multiples of 4) needed to hold bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The closest multiple of 4 that is equal or higher than bytes.
 */
static inline int
pad_to_int32(const int bytes)
{
    return (((bytes) + 3) & ~3);
}

/**
 * Calculate padding needed to bring the number of bytes to an even
 * multiple of 4.
 * @param bytes The minimum number of bytes needed.
 * @return The bytes of padding needed to arrive at the closest multiple of 4
 * that is equal or higher than bytes.
 */
static inline int
padding_for_int32(const int bytes)
{
    return ((-bytes) & 3);
}


extern _X_EXPORT char **xstrtokenize(const char *str, const char *separators);
extern void FormatInt64(int64_t num, char *string);
extern void FormatUInt64(uint64_t num, char *string);
extern void FormatUInt64Hex(uint64_t num, char *string);
extern void FormatDouble(double dbl, char *string);

/**
 * Compare the two version numbers comprising of major.minor.
 *
 * @return A value less than 0 if a is less than b, 0 if a is equal to b,
 * or a value greater than 0
 */
static inline int
version_compare(uint32_t a_major, uint32_t a_minor,
                uint32_t b_major, uint32_t b_minor)
{
    if (a_major > b_major)
        return 1;
    if (a_major < b_major)
        return -1;
    if (a_minor > b_minor)
        return 1;
    if (a_minor < b_minor)
        return -1;

    return 0;
}

/* some macros to help swap requests, replies, and events */

#define LengthRestB(stuff) \
    ((client->req_len << 2) - sizeof(*stuff))

#define LengthRestS(stuff) \
    ((client->req_len << 1) - (sizeof(*stuff) >> 1))

#define LengthRestL(stuff) \
    (client->req_len - (sizeof(*stuff) >> 2))

#define SwapRestS(stuff) \
    SwapShorts((short *)(stuff + 1), LengthRestS(stuff))

#define SwapRestL(stuff) \
    SwapLongs((CARD32 *)(stuff + 1), LengthRestL(stuff))

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
void __attribute__ ((error("wrong sized variable passed to swap")))
wrong_size(void);
#else
static inline void
wrong_size(void)
{
}
#endif

#if !(defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)))
static inline int
__builtin_constant_p(int x)
{
    return 0;
}
#endif

static inline uint64_t
bswap_64(uint64_t x)
{
    return (((x & 0xFF00000000000000ull) >> 56) |
            ((x & 0x00FF000000000000ull) >> 40) |
            ((x & 0x0000FF0000000000ull) >> 24) |
            ((x & 0x000000FF00000000ull) >>  8) |
            ((x & 0x00000000FF000000ull) <<  8) |
            ((x & 0x0000000000FF0000ull) << 24) |
            ((x & 0x000000000000FF00ull) << 40) |
            ((x & 0x00000000000000FFull) << 56));
}

#define swapll(x) do { \
		if (sizeof(*(x)) != 8) \
			wrong_size(); \
		*(x) = bswap_64(*(x));          \
	} while (0)

static inline uint32_t
bswap_32(uint32_t x)
{
    return (((x & 0xFF000000) >> 24) |
            ((x & 0x00FF0000) >> 8) |
            ((x & 0x0000FF00) << 8) |
            ((x & 0x000000FF) << 24));
}

static inline Bool
checked_int64_add(int64_t *out, int64_t a, int64_t b)
{
    /* Do the potentially overflowing math as uint64_t, as signed
     * integers in C are undefined on overflow (and the compiler may
     * optimize out our overflow check below, otherwise)
     */
    int64_t result = (uint64_t)a + (uint64_t)b;
    /* signed addition overflows if operands have the same sign, and
     * the sign of the result doesn't match the sign of the inputs.
     */
    Bool overflow = (a < 0) == (b < 0) && (a < 0) != (result < 0);

    *out = result;

    return overflow;
}

static inline Bool
checked_int64_subtract(int64_t *out, int64_t a, int64_t b)
{
    int64_t result = (uint64_t)a - (uint64_t)b;
    Bool overflow = (a < 0) != (b < 0) && (a < 0) != (result < 0);

    *out = result;

    return overflow;
}

#define swapl(x) do { \
		if (sizeof(*(x)) != 4) \
			wrong_size(); \
		*(x) = bswap_32(*(x)); \
	} while (0)

static inline uint16_t
bswap_16(uint16_t x)
{
    return (((x & 0xFF00) >> 8) |
            ((x & 0x00FF) << 8));
}

#define swaps(x) do { \
		if (sizeof(*(x)) != 2) \
			wrong_size(); \
		*(x) = bswap_16(*(x)); \
	} while (0)

/* copy 32-bit value from src to dst byteswapping on the way */
#define cpswapl(src, dst) do { \
		if (sizeof((src)) != 4 || sizeof((dst)) != 4) \
			wrong_size(); \
		(dst) = bswap_32((src)); \
	} while (0)

/* copy short from src to dst byteswapping on the way */
#define cpswaps(src, dst) do { \
		if (sizeof((src)) != 2 || sizeof((dst)) != 2) \
			wrong_size(); \
		(dst) = bswap_16((src)); \
	} while (0)

extern _X_EXPORT void SwapLongs(CARD32 *list, unsigned long count);

extern _X_EXPORT void SwapShorts(short *list, unsigned long count);

extern _X_EXPORT void MakePredeclaredAtoms(void);

extern _X_EXPORT int Ones(unsigned long /*mask */ );

typedef struct _xPoint *DDXPointPtr;
typedef struct pixman_box16 *BoxPtr;
typedef struct _xEvent *xEventPtr;
typedef struct _xRectangle *xRectanglePtr;
typedef struct _GrabRec *GrabPtr;

/*  typedefs from other places - duplicated here to minimize the amount
 *  of unnecessary junk that one would normally have to include to get
 *  these symbols defined
 */

#ifndef _XTYPEDEF_CHARINFOPTR
typedef struct _CharInfo *CharInfoPtr;  /* also in fonts/include/font.h */

#define _XTYPEDEF_CHARINFOPTR
#endif

extern _X_EXPORT unsigned long globalSerialNumber;
extern _X_EXPORT unsigned long serverGeneration;

/* Don't use this directly, use BUG_WARN or BUG_WARN_MSG instead */
#define __BUG_WARN_MSG(cond, with_msg, ...)                                \
          do { if (cond) {                                                \
              ErrorFSigSafe("BUG: triggered 'if (" #cond ")'\n");          \
              ErrorFSigSafe("BUG: %s:%u in %s()\n",                        \
                           __FILE__, __LINE__, __func__);                 \
              if (with_msg) ErrorFSigSafe(__VA_ARGS__);                    \
              xorg_backtrace();                                           \
          } } while(0)

#define BUG_WARN_MSG(cond, ...)                                           \
          __BUG_WARN_MSG(cond, 1, __VA_ARGS__)

#define BUG_WARN(cond)  __BUG_WARN_MSG(cond, 0, NULL)

#define BUG_RETURN(cond) \
        do { if (cond) { __BUG_WARN_MSG(cond, 0, NULL); return; } } while(0)

#define BUG_RETURN_MSG(cond, ...) \
        do { if (cond) { __BUG_WARN_MSG(cond, 1, __VA_ARGS__); return; } } while(0)

#define BUG_RETURN_VAL(cond, val) \
        do { if (cond) { __BUG_WARN_MSG(cond, 0, NULL); return (val); } } while(0)

#define BUG_RETURN_VAL_MSG(cond, val, ...) \
        do { if (cond) { __BUG_WARN_MSG(cond, 1, __VA_ARGS__); return (val); } } while(0)

#endif                          /* MISC_H */
