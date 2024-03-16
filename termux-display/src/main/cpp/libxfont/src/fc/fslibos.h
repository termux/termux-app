/*
 * Copyright 1990 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation
 */

/*

Copyright 1987, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/*
 * FSlib networking & os include file
 */

#include <X11/Xtrans/Xtrans.h>

#ifndef WIN32

/*
 * makedepend screws up on #undef OPEN_MAX, so we define a new symbol
 */

# ifndef FONT_OPEN_MAX

#  ifdef _POSIX_SOURCE
#   include <limits.h>
#  else
#   define _POSIX_SOURCE
#   include <limits.h>
#   undef _POSIX_SOURCE
#  endif
#  ifndef SIZE_MAX
#   ifdef ULONG_MAX
#    define SIZE_MAX ULONG_MAX
#   else
#    define SIZE_MAX UINT_MAX
#   endif
#  endif
#  ifndef OPEN_MAX
#   if defined(SVR4)
#    define OPEN_MAX 256
#   else
#    include <sys/param.h>
#    ifndef OPEN_MAX
#     ifdef __OSF1__
#      define OPEN_MAX 256
#     else
#      ifdef NOFILE
#       define OPEN_MAX NOFILE
#      else
#       define OPEN_MAX NOFILES_MAX
#      endif
#     endif
#    endif
#   endif
#  endif

#  if OPEN_MAX > 256
#   define FONT_OPEN_MAX 256
#  else
#   define FONT_OPEN_MAX OPEN_MAX
#  endif

# endif /* FONT_OPEN_MAX */

# ifdef WORD64
#  define NMSKBITS 64
# else
#  define NMSKBITS 32
# endif

# define MSKCNT ((FONT_OPEN_MAX + NMSKBITS - 1) / NMSKBITS)

typedef unsigned long FdSet[MSKCNT];
typedef FdSet FdSetPtr;

# if (MSKCNT==1)
#  define BITMASK(i) (1 << (i))
#  define MASKIDX(i) 0
# endif

# if (MSKCNT>1)
#  define BITMASK(i) (1 << ((i) & (NMSKBITS - 1)))
#  define MASKIDX(i) ((i) / NMSKBITS)
# endif

# define MASKWORD(buf, i) buf[MASKIDX(i)]
# define BITSET(buf, i) MASKWORD(buf, i) |= BITMASK(i)
# define BITCLEAR(buf, i) MASKWORD(buf, i) &= ~BITMASK(i)
# define GETBIT(buf, i) (MASKWORD(buf, i) & BITMASK(i))

# if (MSKCNT==1)
#  define COPYBITS(src, dst) dst[0] = src[0]
#  define CLEARBITS(buf) buf[0] = 0
#  define MASKANDSETBITS(dst, b1, b2) dst[0] = (b1[0] & b2[0])
#  define ORBITS(dst, b1, b2) dst[0] = (b1[0] | b2[0])
#  define UNSETBITS(dst, b1) (dst[0] &= ~b1[0])
#  define ANYSET(src) (src[0])
# endif

# if (MSKCNT==2)
#  define COPYBITS(src, dst) { dst[0] = src[0]; dst[1] = src[1]; }
#  define CLEARBITS(buf) { buf[0] = 0; buf[1] = 0; }
#  define MASKANDSETBITS(dst, b1, b2)  {\
		      dst[0] = (b1[0] & b2[0]);\
		      dst[1] = (b1[1] & b2[1]); }
#  define ORBITS(dst, b1, b2)  {\
		      dst[0] = (b1[0] | b2[0]);\
		      dst[1] = (b1[1] | b2[1]); }
#  define UNSETBITS(dst, b1) {\
                      dst[0] &= ~b1[0]; \
                      dst[1] &= ~b1[1]; }
#  define ANYSET(src) (src[0] || src[1])
# endif

# if (MSKCNT==3)
#  define COPYBITS(src, dst) { dst[0] = src[0]; dst[1] = src[1]; \
			     dst[2] = src[2]; }
#  define CLEARBITS(buf) { buf[0] = 0; buf[1] = 0; buf[2] = 0; }
#  define MASKANDSETBITS(dst, b1, b2)  {\
		      dst[0] = (b1[0] & b2[0]);\
		      dst[1] = (b1[1] & b2[1]);\
		      dst[2] = (b1[2] & b2[2]); }
#  define ORBITS(dst, b1, b2)  {\
		      dst[0] = (b1[0] | b2[0]);\
		      dst[1] = (b1[1] | b2[1]);\
		      dst[2] = (b1[2] | b2[2]); }
#  define UNSETBITS(dst, b1) {\
                      dst[0] &= ~b1[0]; \
                      dst[1] &= ~b1[1]; \
                      dst[2] &= ~b1[2]; }
#  define ANYSET(src) (src[0] || src[1] || src[2])
# endif

# if (MSKCNT==4)
#  define COPYBITS(src, dst) dst[0] = src[0]; dst[1] = src[1]; \
			   dst[2] = src[2]; dst[3] = src[3]
#  define CLEARBITS(buf) buf[0] = 0; buf[1] = 0; buf[2] = 0; buf[3] = 0
#  define MASKANDSETBITS(dst, b1, b2)  \
                      dst[0] = (b1[0] & b2[0]);\
                      dst[1] = (b1[1] & b2[1]);\
                      dst[2] = (b1[2] & b2[2]);\
                      dst[3] = (b1[3] & b2[3])
#  define ORBITS(dst, b1, b2)  \
                      dst[0] = (b1[0] | b2[0]);\
                      dst[1] = (b1[1] | b2[1]);\
                      dst[2] = (b1[2] | b2[2]);\
                      dst[3] = (b1[3] | b2[3])
#  define UNSETBITS(dst, b1) \
                      dst[0] &= ~b1[0]; \
                      dst[1] &= ~b1[1]; \
                      dst[2] &= ~b1[2]; \
                      dst[3] &= ~b1[3]
#  define ANYSET(src) (src[0] || src[1] || src[2] || src[3])
# endif

# if (MSKCNT>4)
#  define COPYBITS(src, dst) memmove((caddr_t) dst, (caddr_t) src,\
				   MSKCNT*sizeof(long))
#  define CLEARBITS(buf) bzero((caddr_t) buf, MSKCNT*sizeof(long))
#  define MASKANDSETBITS(dst, b1, b2)  \
		      { int cri;			\
			for (cri=MSKCNT; --cri>=0; )	\
		          dst[cri] = (b1[cri] & b2[cri]); }
#  define ORBITS(dst, b1, b2)  \
		      { int cri;			\
		      for (cri=MSKCNT; --cri>=0; )	\
		          dst[cri] = (b1[cri] | b2[cri]); }
#  define UNSETBITS(dst, b1) \
		      { int cri;			\
		      for (cri=MSKCNT; --cri>=0; )	\
		          dst[cri] &= ~b1[cri];  }
#  if (MSKCNT==8)
#   define ANYSET(src) (src[0] || src[1] || src[2] || src[3] || \
		     src[4] || src[5] || src[6] || src[7])
#  endif
# endif

#else /* not WIN32 */

# include <X11/Xwinsock.h>
# include <X11/Xw32defs.h>

typedef fd_set FdSet;
typedef FdSet *FdSetPtr;

# define CLEARBITS(set) FD_ZERO(&set)
# define BITSET(set,s) FD_SET(s,&set)
# define BITCLEAR(set,s) FD_CLR(s,&set)
# define GETBIT(set,s) FD_ISSET(s,&set)
# define ANYSET(set) set->fd_count

#endif
