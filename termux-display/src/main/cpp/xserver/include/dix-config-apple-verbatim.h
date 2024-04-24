/* Do not include this file directly.  It is included at the end of <dix-config.h> */

/* Correctly set _XSERVER64 for OSX fat binaries */
#if defined(__LP64__) && !defined(_XSERVER64)
#define _XSERVER64 1
#elif !defined(__LP64__) && defined(_XSERVER64)
#undef _XSERVER64
#endif
