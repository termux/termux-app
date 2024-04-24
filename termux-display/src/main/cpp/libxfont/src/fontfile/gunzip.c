/* lib/font/fontfile/gunzip.c
   written by Mark Eichin <eichin@kitten.gen.ma.us> September 1996.
   intended for inclusion in X11 public releases. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontmisc.h>
#include <X11/fonts/bufio.h>
#include <zlib.h>

typedef struct _xzip_buf {
  z_stream z;
  int zstat;
  BufChar b[BUFFILESIZE];
  BufChar b_in[BUFFILESIZE];
  BufFilePtr f;
} xzip_buf;

static int BufZipFileClose ( BufFilePtr f, int flag );
static int BufZipFileFill ( BufFilePtr f );
static int BufZipFileSkip ( BufFilePtr f, int c );
static int BufCheckZipHeader ( BufFilePtr f );

BufFilePtr
BufFilePushZIP (BufFilePtr f)
{
  xzip_buf *x;

  x = malloc (sizeof (xzip_buf));
  if (!x) return 0;
  /* these are just for raw calloc/free */
  x->z.zalloc = Z_NULL;
  x->z.zfree = Z_NULL;
  x->z.opaque = Z_NULL;
  x->f = f;

  /* force inflateInit to allocate it's own history buffer */
  x->z.next_in = Z_NULL;
  x->z.next_out = Z_NULL;
  x->z.avail_in = x->z.avail_out = 0;

  /* using negative windowBits sets "nowrap" mode, which turns off
     zlib header checking [undocumented, for gzip compatibility only?] */
  x->zstat = inflateInit2(&(x->z), -MAX_WBITS);
  if (x->zstat != Z_OK) {
    free(x);
    return 0;
  }

  /* now that the history buffer is allocated, we provide the data buffer */
  x->z.next_out = x->b;
  x->z.avail_out = BUFFILESIZE;
  x->z.next_out = x->b_in;
  x->z.avail_in = 0;

  if (BufCheckZipHeader(x->f)) {
    free(x);
    return 0;
  }

  return BufFileCreate((char *)x,
		       BufZipFileFill,
		       0,
		       BufZipFileSkip,
		       BufZipFileClose);
}

static int
BufZipFileClose(BufFilePtr f, int flag)
{
  xzip_buf *x = (xzip_buf *)f->private;
  inflateEnd (&(x->z));
  BufFileClose (x->f, flag);
  free (x);
  return 1;
}

/* here's the real work.
   -- we need to put stuff in f.buffer, update f.left and f.bufp,
      then return the first byte (or BUFFILEEOF).
   -- to do this, we need to get stuff into avail_in, and next_in,
      and call inflate appropriately.
   -- we may also need to add CRC maintenance - if inflate tells us
      Z_STREAM_END, we then have 4bytes CRC and 4bytes length...
   gzio.c:gzread shows most of the mechanism.
   */
static int
BufZipFileFill (BufFilePtr f)
{
  xzip_buf *x = (xzip_buf *)f->private;

  /* we only get called when left == 0... */
  /* but just in case, deal */
  if (f->left >= 0) {
    f->left--;
    return *(f->bufp++);
  }
  /* did we run out last time? */
  switch (x->zstat) {
  case Z_OK:
    break;
  case Z_STREAM_END:
  case Z_DATA_ERROR:
  case Z_ERRNO:
      f->left = 0;
      return BUFFILEEOF;
  default:
    return BUFFILEEOF;
  }
  /* now we work to consume what we can */
  /* let zlib know what we can handle */
  x->z.next_out = x->b;
  x->z.avail_out = BUFFILESIZE;

  /* and try to consume all of it */
  while (x->z.avail_out > 0) {
    /* if we don't have anything to work from... */
    if (x->z.avail_in == 0) {
      /* ... fill the z buf from underlying file */
      int i, c;
      for (i = 0; i < sizeof(x->b_in); i++) {
	c = BufFileGet(x->f);
	if (c == BUFFILEEOF) break;
	x->b_in[i] = c;
      }
      x->z.avail_in += i;
      x->z.next_in = x->b_in;
    }
    /* so now we have some output space and some input data */
    x->zstat = inflate(&(x->z), Z_NO_FLUSH);
    /* the inflation output happens in the f buffer directly... */
    if (x->zstat == Z_STREAM_END) {
      /* deal with EOF, crc */
      break;
    }
    if (x->zstat != Z_OK) {
      break;
    }
  }
  f->bufp = x->b;
  f->left = BUFFILESIZE - x->z.avail_out;

  if (f->left >= 0) {
    f->left--;
    return *(f->bufp++);
  } else {
    return BUFFILEEOF;
  }
}

/* there should be a BufCommonSkip... */
static int
BufZipFileSkip (BufFilePtr f, int c)
{
  /* BufFileRawSkip returns the count unchanged.
     BufCompressedSkip returns 0.
     That means it probably never gets called... */
  int retval = c;
  while(c--) {
    int get = BufFileGet(f);
    if (get == BUFFILEEOF) return get;
  }
  return retval;
}

/* now we need to duplicate check_header */
/* contents:
     0x1f, 0x8b	-- magic number
     1 byte	-- method (Z_DEFLATED)
     1 byte	-- flags (mask with RESERVED -> fail)
     4 byte	-- time (discard)
     1 byte	-- xflags (discard)
     1 byte	-- "os" code (discard)
     [if flags & EXTRA_FIELD:
         2 bytes -- LSBfirst length n
	 n bytes -- extra data (discard)]
     [if flags & ORIG_NAME:
	 n bytes -- null terminated name (discard)]
     [if flags & COMMENT:
	 n bytes -- null terminated comment (discard)]
     [if flags & HEAD_CRC:
         2 bytes -- crc of headers? (discard)]
 */

/* gzip flag byte -- from gzio.c */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

#define GET(f) do {c = BufFileGet(f); if (c == BUFFILEEOF) return c;} while(0)
static int
BufCheckZipHeader(BufFilePtr f)
{
  int c, flags;
  GET(f); if (c != 0x1f) return 1; /* magic 1 */
  GET(f); if (c != 0x8b) return 2; /* magic 2 */
  GET(f); if (c != Z_DEFLATED) return 3; /* method */
  GET(f); if (c & RESERVED) return 4; /* reserved flags */
  flags = c;
  GET(f); GET(f); GET(f); GET(f); /* time */
  GET(f);			/* xflags */
  GET(f);			/* os code */
  if (flags & EXTRA_FIELD) {
    int len;
    GET(f); len = c;
    GET(f); len += (c<<8);
    while (len-- >= 0) {
      GET(f);
    }
  }
  if (flags & ORIG_NAME) {
    do { GET(f); } while (c != 0);
  }
  if (flags & COMMENT) {
    do { GET(f); } while (c != 0);
  }
  if (flags & HEAD_CRC) {
    GET(f); GET(f);		/* header crc */
  }
  return 0;
}
