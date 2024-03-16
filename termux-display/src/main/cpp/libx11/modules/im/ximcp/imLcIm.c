/******************************************************************

          Copyright 1992, 1993, 1994 by FUJITSU LIMITED
          Copyright 1993 by Digital Equipment Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED and
Digital Equipment Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.  FUJITSU LIMITED and Digital Equipment Corporation
makes no representations about the suitability of this software for
any purpose.  It is provided "as is" without express or implied
warranty.

FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

  Author:    Takashi Fujiwara     FUJITSU LIMITED
                               	  fujiwara@a80.tech.yk.fujitsu.co.jp
  Modifier:  Franky Ling          Digital Equipment Corporation
	                          frankyling@hgrd01.enet.dec.com

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>

#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPublic.h"
#include "XlcPubI.h"
#include "Ximint.h"
#include <ctype.h>
#include <assert.h>

#ifdef COMPOSECACHE
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <langinfo.h>
#endif


#ifdef COMPOSECACHE

/* include trailing '/' for cache directory, file prefix otherwise */
#define XIM_GLOBAL_CACHE_DIR "/var/cache/libx11/compose/"
#define XIM_HOME_CACHE_DIR   "/.compose-cache/"
#define XIM_CACHE_MAGIC      ('X' | 'i'<<8 | 'm'<<16 | 'C'<<24)
#define XIM_CACHE_VERSION    4
#define XIM_CACHE_TREE_ALIGNMENT 4

#define XIM_HASH_PRIME_1 13
#define XIM_HASH_PRIME_2 1234096939

typedef INT32 DTStructIndex;
struct _XimCacheStruct {
    INT32           id;
    INT32           version;
    DTStructIndex   tree;
    DTStructIndex   mb;
    DTStructIndex   wc;
    DTStructIndex   utf8;
    DTStructIndex   size;
    DTIndex         top;
    DTIndex         treeused;
    DTCharIndex     mbused;
    DTCharIndex     wcused;
    DTCharIndex     utf8used;
    char            fname[];
    /* char encoding[] */
};

static struct  _XimCacheStruct* _XimCache_mmap = NULL;
static DefTreeBase _XimCachedDefaultTreeBase;
static int     _XimCachedDefaultTreeRefcount = 0;

#endif


Bool
_XimCheckIfLocalProcessing(Xim im)
{
    FILE        *fp;
    char        *name;

    if(strcmp(im->core.im_name, "") == 0) {
	name = _XlcFileName(im->core.lcd, COMPOSE_FILE);
	if (name != (char *)NULL) {
	    fp = _XFopenFile (name, "r");
	    Xfree(name);
	    if (fp != (FILE *)NULL) {
		fclose(fp);
		return(True);
	    }
	}
	return(False);
    } else if(strcmp(im->core.im_name, "local") == 0 ||
	      strcmp(im->core.im_name, "none" ) == 0 ) {
	return(True);
    }
    return(False);
}

static void
XimFreeDefaultTree(
    DefTreeBase *b)
{
    if (!b) return;
    if (b->tree == NULL) return;
#ifdef COMPOSECACHE
    if (b->tree == _XimCachedDefaultTreeBase.tree) {
        _XimCachedDefaultTreeRefcount--;
        /* No deleting, it's a cache after all. */
        return;
    }
#endif
    Xfree (b->tree);
    b->tree = NULL;
    Xfree (b->mb);
    b->mb   = NULL;
    Xfree (b->wc);
    b->wc   = NULL;
    Xfree (b->utf8);
    b->utf8 = NULL;

    b->treeused = b->treesize = 0;
    b->mbused   = b->mbsize   = 0;
    b->wcused   = b->wcsize   = 0;
    b->utf8used = b->utf8size = 0;
}

void
_XimLocalIMFree(
    Xim		im)
{
    XimFreeDefaultTree(&im->private.local.base);
    im->private.local.top = 0;

    Xfree(im->core.im_resources);
    im->core.im_resources = NULL;

    Xfree(im->core.ic_resources);
    im->core.ic_resources = NULL;

    Xfree(im->core.im_values_list);
    im->core.im_values_list = NULL;

    Xfree(im->core.ic_values_list);
    im->core.ic_values_list = NULL;

    Xfree(im->core.styles);
    im->core.styles = NULL;

    Xfree(im->core.res_name);
    im->core.res_name = NULL;

    Xfree(im->core.res_class);
    im->core.res_class = NULL;

    Xfree(im->core.im_name);
    im->core.im_name = NULL;

    if (im->private.local.ctom_conv) {
	_XlcCloseConverter(im->private.local.ctom_conv);
        im->private.local.ctom_conv = NULL;
    }
    if (im->private.local.ctow_conv) {
	_XlcCloseConverter(im->private.local.ctow_conv);
	im->private.local.ctow_conv = NULL;
    }
    if (im->private.local.ctoutf8_conv) {
	_XlcCloseConverter(im->private.local.ctoutf8_conv);
	im->private.local.ctoutf8_conv = NULL;
    }
    if (im->private.local.cstomb_conv) {
	_XlcCloseConverter(im->private.local.cstomb_conv);
        im->private.local.cstomb_conv = NULL;
    }
    if (im->private.local.cstowc_conv) {
	_XlcCloseConverter(im->private.local.cstowc_conv);
	im->private.local.cstowc_conv = NULL;
    }
    if (im->private.local.cstoutf8_conv) {
	_XlcCloseConverter(im->private.local.cstoutf8_conv);
	im->private.local.cstoutf8_conv = NULL;
    }
    if (im->private.local.ucstoc_conv) {
	_XlcCloseConverter(im->private.local.ucstoc_conv);
	im->private.local.ucstoc_conv = NULL;
    }
    if (im->private.local.ucstoutf8_conv) {
	_XlcCloseConverter(im->private.local.ucstoutf8_conv);
	im->private.local.ucstoutf8_conv = NULL;
    }
    return;
}

static Status
_XimLocalCloseIM(
    XIM		xim)
{
    Xim		im = (Xim)xim;
    XIC		ic;
    XIC		next;

    ic = im->core.ic_chain;
    im->core.ic_chain = NULL;
    while (ic) {
	(*ic->methods->destroy) (ic);
	next = ic->core.next;
	Xfree (ic);
	ic = next;
    }
    _XimLocalIMFree(im);
    _XimDestroyIMStructureList(im);
    return(True);
}

char *
_XimLocalGetIMValues(
    XIM			 xim,
    XIMArg		*values)
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;

    _XimGetCurrentIMValues(im, &im_values);
    return(_XimGetIMValueData(im, (XPointer)&im_values, values,
			im->core.im_resources, im->core.im_num_resources));
}

char *
_XimLocalSetIMValues(
    XIM			 xim,
    XIMArg		*values)
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;
    char		*name = (char *)NULL;

    _XimGetCurrentIMValues(im, &im_values);
    name = _XimSetIMValueData(im, (XPointer)&im_values, values,
		im->core.im_resources, im->core.im_num_resources);
    _XimSetCurrentIMValues(im, &im_values);
    return(name);
}


#ifdef COMPOSECACHE

static Bool
_XimReadCachedDefaultTree(
    int          fd_cache,
    const char  *name,
    const char  *encoding,
    DTStructIndex size)
{
    struct _XimCacheStruct* m;
    int namelen = strlen (name) + 1;
    int encodinglen = strlen (encoding) + 1;

    m = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd_cache, 0);
    if (m == NULL || m == MAP_FAILED)
        return False;
    assert (m->id == XIM_CACHE_MAGIC);
    assert (m->version == XIM_CACHE_VERSION);
    if (size != m->size ||
	size < sizeof (struct _XimCacheStruct) + namelen + encodinglen) {
	fprintf (stderr, "Ignoring broken XimCache %s [%s]\n", name, encoding);
        munmap (m, size);
        return False;
    }
    if (strncmp (name, m->fname, namelen) != 0) {
	/* m->fname may *not* be terminated - but who cares here */
	fprintf (stderr, "Filename hash clash - expected %s, got %s\n",
		 name, m->fname);
        munmap (m, size);
        return False;
    }
    if (strncmp (encoding, m->fname + namelen, encodinglen) != 0) {
	/* m->fname+namelen may *not* be terminated - but who cares here */
	fprintf (stderr, "Enoding hash clash - expected %s, got %s\n",
		 encoding, m->fname + namelen);
        munmap (m, size);
        return False;
    }
    _XimCache_mmap    = m;
    _XimCachedDefaultTreeBase.tree = (DefTree *) (((char *) m) + m->tree);
    _XimCachedDefaultTreeBase.mb   =             (((char *) m) + m->mb);
    _XimCachedDefaultTreeBase.wc   = (wchar_t *) (((char *) m) + m->wc);
    _XimCachedDefaultTreeBase.utf8 =             (((char *) m) + m->utf8);
    _XimCachedDefaultTreeBase.treeused = m->treeused;
    _XimCachedDefaultTreeBase.mbused   = m->mbused;
    _XimCachedDefaultTreeBase.wcused   = m->wcused;
    _XimCachedDefaultTreeBase.utf8used = m->utf8used;
    /* treesize etc. is ignored because only used during parsing */
    _XimCachedDefaultTreeRefcount = 0;
/* fprintf (stderr, "read cached tree at %p: %s\n", (void *) m, name); */
    return True;
}

static unsigned int strToHash (
    const char *name)
{
    unsigned int hash = 0;
    while (*name)
	hash = hash * XIM_HASH_PRIME_1 + *(unsigned const char *)name++;
    return hash % XIM_HASH_PRIME_2;
}


/* Returns read-only fd of cache file, -1 if none.
 * Sets *res to cache filename if safe. Sets *size to file size of cache. */
static int _XimCachedFileName (
    const char *dir, const char *name,
    const char *intname, const char *encoding,
    uid_t uid, int isglobal, char **res, off_t *size)
{
    struct stat st_name, st;
    int    fd;
    unsigned int len, hash, hash2;
    struct _XimCacheStruct *m;
    /* There are some races here with 'dir', but we are either in our own home
     * or the global cache dir, and not inside some public writable dir */
/* fprintf (stderr, "XimCachedFileName for dir %s name %s intname %s encoding %s uid %d\n", dir, name, intname, encoding, uid); */
    if (stat (name, &st_name) == -1 || ! S_ISREG (st_name.st_mode)
       || stat (dir, &st) == -1 || ! S_ISDIR (st.st_mode) || st.st_uid != uid
       || (st.st_mode & 0022) != 0000) {
       *res = NULL;
       return -1;
    }
    len   = strlen (dir);
    hash  = strToHash (intname);
    hash2 = strToHash (encoding);
    *res  = Xmalloc (len + 1 + 27 + 1);  /* Max VERSION 9999 */

    if (len == 0 || dir [len-1] != '/')
       sprintf (*res, "%s/%c%d_%03x_%08x_%08x", dir, _XimGetMyEndian(),
		XIM_CACHE_VERSION, (unsigned int)sizeof (DefTree), hash, hash2);
    else
       sprintf (*res, "%s%c%d_%03x_%08x_%08x", dir, _XimGetMyEndian(),
		XIM_CACHE_VERSION, (unsigned int)sizeof (DefTree), hash, hash2);

/* fprintf (stderr, "-> %s\n", *res); */
    if ( (fd = _XOpenFile (*res, O_RDONLY)) == -1)
       return -1;

    if (fstat (fd, &st) == -1) {
       Xfree (*res);
       *res = NULL;
       close (fd);
       return -1;
    }
    *size = st.st_size;

    if (! S_ISREG (st.st_mode) || st.st_uid != uid
       || (st.st_mode & 0022) != 0000 || st.st_mtime <= st_name.st_mtime
       || (st.st_mtime < time (NULL) - 24*60*60 && ! isglobal)) {

       close (fd);
       if (unlink (*res) != 0) {
           Xfree (*res);
           *res = NULL;                /* cache is not safe */
       }
       return -1;
    }

    m = mmap (NULL, sizeof (struct _XimCacheStruct), PROT_READ, MAP_PRIVATE,
	      fd, 0);
    if (m == NULL || m == MAP_FAILED) {
	close (fd);
	Xfree (*res);
	*res = NULL;
        return -1;
    }
    if (*size < sizeof (struct _XimCacheStruct) || m->id != XIM_CACHE_MAGIC) {
	munmap (m, sizeof (struct _XimCacheStruct));
	close (fd);
	fprintf (stderr, "Ignoring broken XimCache %s\n", *res);
	Xfree (*res);
	*res = NULL;
        return -1;
    }
    if (m->version != XIM_CACHE_VERSION) {
	munmap (m, sizeof (struct _XimCacheStruct));
	close (fd);
	if (unlink (*res) != 0) {
	    Xfree (*res);
	    *res = NULL;                /* cache is not safe */
	}
	return -1;
    }
    munmap (m, sizeof (struct _XimCacheStruct));

    return fd;
}


static Bool _XimLoadCache (
    int         fd,
    const char *name,
    const char *encoding,
    off_t       size,
    Xim         im)
{
    if (_XimCache_mmap ||
       _XimReadCachedDefaultTree (fd, name, encoding, size)) {
       _XimCachedDefaultTreeRefcount++;
       memcpy (&im->private.local.base, &_XimCachedDefaultTreeBase,
	       sizeof (_XimCachedDefaultTreeBase));
       im->private.local.top = _XimCache_mmap->top;
       return True;
    }

    return False;
}


static void
_XimWriteCachedDefaultTree(
    const char *name,
    const char *encoding,
    const char *cachename,
    Xim                im)
{
    int   fd;
    FILE *fp;
    struct _XimCacheStruct *m;
    int   msize = (sizeof(struct _XimCacheStruct)
		   + strlen(name) + strlen(encoding) + 2
		   + XIM_CACHE_TREE_ALIGNMENT-1) & -XIM_CACHE_TREE_ALIGNMENT;
    DefTreeBase *b = &im->private.local.base;

    if (! b->tree && ! (b->tree = Xcalloc (1, sizeof(DefTree))) )
	return;
    if (! b->mb   && ! (b->mb   = Xmalloc (1)) )
	return;
    if (! b->wc   && ! (b->wc   = Xmalloc (sizeof(wchar_t))) )
	return;
    if (! b->utf8 && ! (b->utf8 = Xmalloc (1)) )
	return;

    /* First entry is always unused */
    b->mb[0]   = 0;
    b->wc[0]   = 0;
    b->utf8[0] = 0;

    m = Xcalloc (1, msize);
    m->id       = XIM_CACHE_MAGIC;
    m->version  = XIM_CACHE_VERSION;
    m->top      = im->private.local.top;
    m->treeused = b->treeused;
    m->mbused   = b->mbused;
    m->wcused   = b->wcused;
    m->utf8used = b->utf8used;
    /* Tree first, then wide chars, then the rest due to alignment */
    m->tree     = msize;
    m->wc       = msize   + sizeof (DefTree) * m->treeused;
    m->mb       = m->wc   + sizeof (wchar_t) * m->wcused;
    m->utf8     = m->mb   +                    m->mbused;
    m->size     = m->utf8 +                    m->utf8used;
    strcpy (m->fname, name);
    strcpy (m->fname+strlen(name)+1, encoding);

    /* This STILL might be racy on NFS */
    if ( (fd = _XOpenFileMode (cachename, O_WRONLY | O_CREAT | O_EXCL,
			       0600)) < 0) {
       Xfree(m);
       return;
    }
    if (! (fp = fdopen (fd, "wb")) ) {
       close (fd);
       Xfree(m);
       return;
    }
    fwrite (m, msize, 1, fp);
    fwrite (im->private.local.base.tree, sizeof(DefTree), m->treeused, fp);
    fwrite (im->private.local.base.wc,   sizeof(wchar_t), m->wcused,   fp);
    fwrite (im->private.local.base.mb,   1,               m->mbused,   fp);
    fwrite (im->private.local.base.utf8, 1,               m->utf8used, fp);
    if (fclose (fp) != 0)
	unlink (cachename);
    _XimCache_mmap = m;
    memcpy (&_XimCachedDefaultTreeBase, &im->private.local.base,
	    sizeof (_XimCachedDefaultTreeBase));
/* fprintf (stderr, "wrote tree %s size %ld to %s\n", name, m->size, cachename); */
}

#endif


static void
_XimCreateDefaultTree(
    Xim		im)
{
    FILE *fp = NULL;
    char *name, *tmpname = NULL, *intname;
    char *cachename = NULL;
    /* Should use getpwent() instead of $HOME (cross-platform?) */
    char *home = getenv("HOME");
    char *cachedir = NULL;
    char *tmpcachedir = NULL;
    int   hl = home ? strlen (home) : 0;
#ifdef COMPOSECACHE
    const char *encoding = nl_langinfo (CODESET);
    uid_t euid = geteuid ();
    gid_t egid = getegid ();
    int   cachefd = -1;
    off_t size;
#endif

    name = getenv("XCOMPOSEFILE");
    if (name == (char *) NULL) {
    	if (home != (char *) NULL) {
            tmpname = name = Xmalloc(hl + 10 + 1);
            if (name != (char *) NULL) {
		int fd;
            	strcpy(name, home);
            	strcpy(name + hl, "/.XCompose");
		if ( (fd = _XOpenFile (name, O_RDONLY)) < 0) {
		    Xfree (name);
		    name = tmpname = NULL;
		} else
		    close (fd);
            }
        }
    }

    if (name == (char *) NULL) {
        tmpname = name = _XlcFileName(im->core.lcd, COMPOSE_FILE);
    }
    intname = name;

#ifdef COMPOSECACHE
    if (getuid () == euid && getgid () == egid && euid != 0) {
	char *c;
	/* Usage: XCOMPOSECACHE=<cachedir>[=<filename>]
	 * cachedir: directory of cache files
	 * filename: internally used name for cache file */
        cachedir = getenv("XCOMPOSECACHE");
	if (cachedir && (c = strchr (cachedir, '='))) {
	    tmpcachedir = strdup (cachedir);
	    intname = tmpcachedir + (c-cachedir) + 1;
	    tmpcachedir[c-cachedir] = '\0';
	    cachedir = tmpcachedir;
	}
    }

    if (! cachedir) {
	cachefd = _XimCachedFileName (XIM_GLOBAL_CACHE_DIR, name, intname,
				      encoding, 0, 1, &cachename, &size);
	if (cachefd != -1) {
	    if (_XimLoadCache (cachefd, intname, encoding, size, im)) {
	        Xfree (tmpcachedir);
		Xfree (tmpname);
		Xfree (cachename);
		close (cachefd);
		return;
	    }
	    close (cachefd);
	}
	Xfree (cachename);
	cachename = NULL;
    }

    if (getuid () == euid && getgid () == egid && euid != 0 && home) {

	if (! cachedir) {
	    tmpcachedir = cachedir = Xmalloc (hl+strlen(XIM_HOME_CACHE_DIR)+1);
	    strcpy (cachedir, home);
	    strcat (cachedir, XIM_HOME_CACHE_DIR);
	}
	cachefd = _XimCachedFileName (cachedir, name, intname, encoding,
				      euid, 0, &cachename, &size);
	if (cachefd != -1) {
	    if (_XimLoadCache (cachefd, intname, encoding, size, im)) {
	        Xfree (tmpcachedir);
		Xfree (tmpname);
		Xfree (cachename);
		close (cachefd);
		return;
	    }
	    close (cachefd);
	}
    }
#endif

    if (! (fp = _XFopenFile (name, "r"))) {
	Xfree (tmpcachedir);
	Xfree (tmpname);
	Xfree (cachename);
        return;
    }
    _XimParseStringFile(fp, im);
    fclose(fp);

#ifdef COMPOSECACHE
    if (cachename) {
	assert (euid != 0);
	_XimWriteCachedDefaultTree (intname, encoding, cachename, im);
    }
#endif

    Xfree (tmpcachedir);
    Xfree (tmpname);
    Xfree (cachename);
}

static XIMMethodsRec      Xim_im_local_methods = {
    _XimLocalCloseIM,           /* close */
    _XimLocalSetIMValues,       /* set_values */
    _XimLocalGetIMValues,       /* get_values */
    _XimLocalCreateIC,          /* create_ic */
    _XimLcctstombs,		/* ctstombs */
    _XimLcctstowcs,		/* ctstowcs */
    _XimLcctstoutf8		/* ctstoutf8 */
};

Bool
_XimLocalOpenIM(
    Xim			 im)
{
    XLCd		 lcd = im->core.lcd;
    XlcConv		 conv;
    XimDefIMValues	 im_values;
    XimLocalPrivateRec*  private = &im->private.local;

    _XimInitialResourceInfo();
    if(_XimSetIMResourceList(&im->core.im_resources,
		 		&im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    if(_XimSetICResourceList(&im->core.ic_resources,
				&im->core.ic_num_resources) == False) {
	goto Open_Error;
    }

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);

    _XimGetCurrentIMValues(im, &im_values);
    if(_XimSetLocalIMDefaults(im, (XPointer)&im_values,
		im->core.im_resources, im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    _XimSetCurrentIMValues(im, &im_values);

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNMultiByte)))
	goto Open_Error;
    private->ctom_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNWideChar)))
	goto Open_Error;
    private->ctow_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCompoundText, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->ctoutf8_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNMultiByte)))
	goto Open_Error;
    private->cstomb_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNWideChar)))
	goto Open_Error;
    private->cstowc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNCharSet, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->cstoutf8_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNChar)))
	goto Open_Error;
    private->ucstoc_conv = conv;

    if (!(conv = _XlcOpenConverter(lcd,	XlcNUcsChar, lcd, XlcNUtf8String)))
	goto Open_Error;
    private->ucstoutf8_conv = conv;

    private->base.treeused = 1;
    private->base.mbused   = 1;
    private->base.wcused   = 1;
    private->base.utf8used = 1;

    _XimCreateDefaultTree(im);

    im->methods = &Xim_im_local_methods;
    private->current_ic = (XIC)NULL;

    return(True);

Open_Error :
    _XimLocalIMFree(im);
    return(False);
}
