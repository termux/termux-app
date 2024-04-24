/***********************************************************

Copyright 1987, 1989, 1998  The Open Group

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

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

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

******************************************************************/

#ifndef RESOURCE_H
#define RESOURCE_H 1
#include "misc.h"
#include "dixaccess.h"

/*****************************************************************
 * STUFF FOR RESOURCES
 *****************************************************************/

/* classes for Resource routines */

typedef uint32_t RESTYPE;

#define RC_VANILLA	((RESTYPE)0)
#define RC_CACHED	((RESTYPE)1<<31)
#define RC_DRAWABLE	((RESTYPE)1<<30)
/*  Use class RC_NEVERRETAIN for resources that should not be retained
 *  regardless of the close down mode when the client dies.  (A client's
 *  event selections on objects that it doesn't own are good candidates.)
 *  Extensions can use this too!
 */
#define RC_NEVERRETAIN	((RESTYPE)1<<29)
#define RC_LASTPREDEF	RC_NEVERRETAIN
#define RC_ANY		(~(RESTYPE)0)

/* types for Resource routines */

#define RT_WINDOW	((RESTYPE)1|RC_DRAWABLE)
#define RT_PIXMAP	((RESTYPE)2|RC_DRAWABLE)
#define RT_GC		((RESTYPE)3)
#undef RT_FONT
#undef RT_CURSOR
#define RT_FONT		((RESTYPE)4)
#define RT_CURSOR	((RESTYPE)5)
#define RT_COLORMAP	((RESTYPE)6)
#define RT_CMAPENTRY	((RESTYPE)7)
#define RT_OTHERCLIENT	((RESTYPE)8|RC_NEVERRETAIN)
#define RT_PASSIVEGRAB	((RESTYPE)9|RC_NEVERRETAIN)
#define RT_LASTPREDEF	((RESTYPE)9)
#define RT_NONE		((RESTYPE)0)

extern _X_EXPORT unsigned int ResourceClientBits(void);
/* bits and fields within a resource id */
#define RESOURCE_AND_CLIENT_COUNT   29  /* 29 bits for XIDs */
#define RESOURCE_CLIENT_BITS        ResourceClientBits() /* client field offset */
#define CLIENTOFFSET	    (RESOURCE_AND_CLIENT_COUNT - RESOURCE_CLIENT_BITS)
/* resource field */
#define RESOURCE_ID_MASK	((1 << CLIENTOFFSET) - 1)
/* client field */
#define RESOURCE_CLIENT_MASK	(((1 << RESOURCE_CLIENT_BITS) - 1) << CLIENTOFFSET)
/* extract the client mask from an XID */
#define CLIENT_BITS(id) ((id) & RESOURCE_CLIENT_MASK)
/* extract the client id from an XID */
#define CLIENT_ID(id) ((int)(CLIENT_BITS(id) >> CLIENTOFFSET))
#define SERVER_BIT		(Mask)0x40000000        /* use illegal bit */

#ifdef INVALID
#undef INVALID                  /* needed on HP/UX */
#endif

/* Invalid resource id */
#define INVALID	(0)

#define BAD_RESOURCE 0xe0000000

#define rClient(obj) (clients[CLIENT_ID((obj)->resource)])

/* Resource state callback */
extern _X_EXPORT CallbackListPtr ResourceStateCallback;

typedef enum { ResourceStateAdding,
    ResourceStateFreeing
} ResourceState;

typedef struct {
    ResourceState state;
    XID id;
    RESTYPE type;
    void *value;
} ResourceStateInfoRec;

typedef int (*DeleteType) (void *value,
                           XID id);

typedef void (*FindResType) (void *value,
                             XID id,
                             void *cdata);

typedef void (*FindAllRes) (void *value,
                            XID id,
                            RESTYPE type,
                            void *cdata);

typedef Bool (*FindComplexResType) (void *value,
                                    XID id,
                                    void *cdata);

/* Structure for estimating resource memory usage. Memory usage
 * consists of space allocated for the resource itself and of
 * references to other resources. Currently the most important use for
 * this structure is to estimate pixmap usage of different resources
 * more accurately. */
typedef struct {
    /* Size of resource itself. Zero if not implemented. */
    unsigned long resourceSize;
    /* Size attributed to pixmap references from the resource. */
    unsigned long pixmapRefSize;
    /* Number of references to this resource; typically 1 */
    unsigned long refCnt;
} ResourceSizeRec, *ResourceSizePtr;

typedef void (*SizeType)(void *value,
                         XID id,
                         ResourceSizePtr size);

extern _X_EXPORT RESTYPE CreateNewResourceType(DeleteType deleteFunc,
                                               const char *name);

typedef void (*FindTypeSubResources)(void *value,
                                     FindAllRes func,
                                     void *cdata);

extern _X_EXPORT SizeType GetResourceTypeSizeFunc(
    RESTYPE /*type*/);

extern _X_EXPORT void SetResourceTypeFindSubResFunc(
    RESTYPE /*type*/, FindTypeSubResources /*findFunc*/);

extern _X_EXPORT void SetResourceTypeSizeFunc(
    RESTYPE /*type*/, SizeType /*sizeFunc*/);

extern _X_EXPORT void SetResourceTypeErrorValue(
    RESTYPE /*type*/, int /*errorValue*/);

extern _X_EXPORT RESTYPE CreateNewResourceClass(void);

extern _X_EXPORT Bool InitClientResources(ClientPtr /*client */ );

extern _X_EXPORT XID FakeClientID(int /*client */ );

/* Quartz support on Mac OS X uses the CarbonCore
   framework whose AddResource function conflicts here. */
#ifdef __APPLE__
#define AddResource Darwin_X_AddResource
#endif
extern _X_EXPORT Bool AddResource(XID id,
                                  RESTYPE type,
                                  void *value);

extern _X_EXPORT void FreeResource(XID /*id */ ,
                                   RESTYPE /*skipDeleteFuncType */ );

extern _X_EXPORT void FreeResourceByType(XID /*id */ ,
                                         RESTYPE /*type */ ,
                                         Bool /*skipFree */ );

extern _X_EXPORT Bool ChangeResourceValue(XID id,
                                          RESTYPE rtype,
                                          void *value);

extern _X_EXPORT void FindClientResourcesByType(ClientPtr client,
                                                RESTYPE type,
                                                FindResType func,
                                                void *cdata);

extern _X_EXPORT void FindAllClientResources(ClientPtr client,
                                             FindAllRes func,
                                             void *cdata);

/** @brief Iterate through all subresources of a resource.

    @note The XID argument provided to the FindAllRes function
          may be 0 for subresources that don't have an XID */
extern _X_EXPORT void FindSubResources(void *resource,
                                       RESTYPE type,
                                       FindAllRes func,
                                       void *cdata);

extern _X_EXPORT void FreeClientNeverRetainResources(ClientPtr /*client */ );

extern _X_EXPORT void FreeClientResources(ClientPtr /*client */ );

extern _X_EXPORT void FreeAllResources(void);

extern _X_EXPORT Bool LegalNewID(XID /*id */ ,
                                 ClientPtr /*client */ );

extern _X_EXPORT void *LookupClientResourceComplex(ClientPtr client,
                                                     RESTYPE type,
                                                     FindComplexResType func,
                                                     void *cdata);

extern _X_EXPORT int dixLookupResourceByType(void **result,
                                             XID id,
                                             RESTYPE rtype,
                                             ClientPtr client,
                                             Mask access_mode);

extern _X_EXPORT int dixLookupResourceByClass(void **result,
                                              XID id,
                                              RESTYPE rclass,
                                              ClientPtr client,
                                              Mask access_mode);

extern _X_EXPORT void GetXIDRange(int /*client */ ,
                                  Bool /*server */ ,
                                  XID * /*minp */ ,
                                  XID * /*maxp */ );

extern _X_EXPORT unsigned int GetXIDList(ClientPtr /*client */ ,
                                         unsigned int /*count */ ,
                                         XID * /*pids */ );

extern _X_EXPORT RESTYPE lastResourceType;
extern _X_EXPORT RESTYPE TypeMask;

/** @brief A hashing function to be used for hashing resource IDs

    @param id The resource ID to hash
    @param numBits The number of bits in the resulting hash. Must be >=0.

    @note This function is really only for handling
    INITHASHSIZE..MAXHASHSIZE bit hashes, but will handle any number
    of bits by either masking numBits lower bits of the ID or by
    providing at most MAXHASHSIZE hashes.
*/
extern _X_EXPORT int HashResourceID(XID id, unsigned int numBits);

#endif /* RESOURCE_H */
