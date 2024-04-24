/*
 * BIGFONT extension for sharing font metrics between clients (if possible)
 * and for transmitting font metrics to clients in a compressed form.
 *
 * Copyright (c) 1999-2000  Bruno Haible
 * Copyright (c) 1999-2000  The XFree86 Project, Inc.
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

/*
 * Big fonts suffer from the following: All clients that have opened a
 * font can access the complete glyph metrics array (the XFontStruct member
 * `per_char') directly, without going through a macro. Moreover these
 * glyph metrics are ink metrics, i.e. are not redundant even for a
 * fixed-width font. For a Unicode font, the size of this array is 768 KB.
 *
 * Problems: 1. It eats a lot of memory in each client. 2. All this glyph
 * metrics data is piped through the socket when the font is opened.
 *
 * This extension addresses these two problems for local clients, by using
 * shared memory. It also addresses the second problem for non-local clients,
 * by compressing the data before transmit by a factor of nearly 6.
 *
 * If you use this extension, your OS ought to nicely support shared memory.
 * This means: Shared memory should be swappable to the swap, and the limits
 * should be high enough (SHMMNI at least 64, SHMMAX at least 768 KB,
 * SHMALL at least 48 MB). It is a plus if your OS allows shmat() calls
 * on segments that have already been marked "removed", because it permits
 * these segments to be cleaned up by the OS if the X server is killed with
 * signal SIGKILL.
 *
 * This extension is transparently exploited by Xlib (functions XQueryFont,
 * XLoadQueryFont).
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <sys/types.h>
#ifdef HAS_SHM
#ifdef SVR4
#include <sys/sysmacros.h>
#endif
#if defined(__CYGWIN__)
#include <sys/param.h>
#include <sys/sysmacros.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "dixfontstr.h"
#include "extnsionst.h"
#include "extinit.h"
#include "protocol-versions.h"

#include <X11/extensions/xf86bigfproto.h>
#include "xf86bigfontsrv.h"

static void XF86BigfontResetProc(ExtensionEntry *       /* extEntry */
    );

#ifdef HAS_SHM

/* A random signature, transmitted to the clients so they can verify that the
   shared memory segment they are attaching to was really established by the
   X server they are talking to. */
static CARD32 signature;

/* Index for additional information stored in a FontRec's devPrivates array. */
static int FontShmdescIndex;

static unsigned int pagesize;

static Bool badSysCall = FALSE;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__CYGWIN__) || defined(__DragonFly__)

static void
SigSysHandler(int signo)
{
    badSysCall = TRUE;
}

static Bool
CheckForShmSyscall(void)
{
    void (*oldHandler) (int);
    int shmid = -1;

    /* If no SHM support in the kernel, the bad syscall will generate SIGSYS */
    oldHandler = OsSignal(SIGSYS, SigSysHandler);

    badSysCall = FALSE;
    shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT);
    if (shmid != -1) {
        /* Successful allocation - clean up */
        shmctl(shmid, IPC_RMID, NULL);
    }
    else {
        /* Allocation failed */
        badSysCall = TRUE;
    }
    OsSignal(SIGSYS, oldHandler);
    return !badSysCall;
}

#define MUST_CHECK_FOR_SHM_SYSCALL

#endif

#endif

/* ========== Management of shared memory segments ========== */

#ifdef HAS_SHM

#ifdef __linux__
/* On Linux, shared memory marked as "removed" can still be attached.
   Nice feature, because the kernel will automatically free the associated
   storage when the server and all clients are gone. */
#define EARLY_REMOVE
#endif

typedef struct _ShmDesc {
    struct _ShmDesc *next;
    struct _ShmDesc **prev;
    int shmid;
    char *attach_addr;
} ShmDescRec, *ShmDescPtr;

static ShmDescPtr ShmList = (ShmDescPtr) NULL;

static ShmDescPtr
shmalloc(unsigned int size)
{
    ShmDescPtr pDesc;
    int shmid;
    char *addr;

#ifdef MUST_CHECK_FOR_SHM_SYSCALL
    if (pagesize == 0)
        return (ShmDescPtr) NULL;
#endif

    /* On some older Linux systems, the number of shared memory segments
       system-wide is 127. In Linux 2.4, it is 4095.
       Therefore there is a tradeoff to be made between allocating a
       shared memory segment on one hand, and allocating memory and piping
       the glyph metrics on the other hand. If the glyph metrics size is
       small, we prefer the traditional way. */
    if (size < 3500)
        return (ShmDescPtr) NULL;

    pDesc = malloc(sizeof(ShmDescRec));
    if (!pDesc)
        return (ShmDescPtr) NULL;

    size = (size + pagesize - 1) & -pagesize;
    shmid = shmget(IPC_PRIVATE, size, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (shmid == -1) {
        ErrorF(XF86BIGFONTNAME " extension: shmget() failed, size = %u, %s\n",
               size, strerror(errno));
        free(pDesc);
        return (ShmDescPtr) NULL;
    }

    if ((addr = shmat(shmid, 0, 0)) == (char *) -1) {
        ErrorF(XF86BIGFONTNAME " extension: shmat() failed, size = %u, %s\n",
               size, strerror(errno));
        shmctl(shmid, IPC_RMID, (void *) 0);
        free(pDesc);
        return (ShmDescPtr) NULL;
    }

#ifdef EARLY_REMOVE
    shmctl(shmid, IPC_RMID, (void *) 0);
#endif

    pDesc->shmid = shmid;
    pDesc->attach_addr = addr;
    if (ShmList)
        ShmList->prev = &pDesc->next;
    pDesc->next = ShmList;
    pDesc->prev = &ShmList;
    ShmList = pDesc;

    return pDesc;
}

static void
shmdealloc(ShmDescPtr pDesc)
{
#ifndef EARLY_REMOVE
    shmctl(pDesc->shmid, IPC_RMID, (void *) 0);
#endif
    shmdt(pDesc->attach_addr);

    if (pDesc->next)
        pDesc->next->prev = pDesc->prev;
    *pDesc->prev = pDesc->next;
    free(pDesc);
}

#endif

/* Called when a font is closed. */
void
XF86BigfontFreeFontShm(FontPtr pFont)
{
#ifdef HAS_SHM
    ShmDescPtr pDesc;

    /* If during shutdown of the server, XF86BigfontCleanup() has already
     * called shmdealloc() for all segments, we don't need to do it here.
     */
    if (!ShmList)
        return;

    pDesc = (ShmDescPtr) FontGetPrivate(pFont, FontShmdescIndex);
    if (pDesc)
        shmdealloc(pDesc);
#endif
}

/* Called upon fatal signal. */
void
XF86BigfontCleanup(void)
{
#ifdef HAS_SHM
    while (ShmList)
        shmdealloc(ShmList);
#endif
}

/* Called when a server generation dies. */
static void
XF86BigfontResetProc(ExtensionEntry * extEntry)
{
    /* This function is normally called from CloseDownExtensions(), called
     * from main(). It will be followed by a call to FreeAllResources(),
     * which will call XF86BigfontFreeFontShm() for each font. Thus it
     * appears that we do not need to do anything in this function. --
     * But I prefer to write robust code, and not keep shared memory lying
     * around when it's not needed any more. (Someone might close down the
     * extension without calling FreeAllResources()...)
     */
    XF86BigfontCleanup();
}

/* ========== Handling of extension specific requests ========== */

static int
ProcXF86BigfontQueryVersion(ClientPtr client)
{
    xXF86BigfontQueryVersionReply reply;

    REQUEST_SIZE_MATCH(xXF86BigfontQueryVersionReq);
    reply = (xXF86BigfontQueryVersionReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_XF86BIGFONT_MAJOR_VERSION,
        .minorVersion = SERVER_XF86BIGFONT_MINOR_VERSION,
        .uid = geteuid(),
        .gid = getegid(),
#ifdef HAS_SHM
        .signature = signature,
        .capabilities = (client->local && !client->swapped)
                         ? XF86Bigfont_CAP_LocalShm : 0
#else
        .signature = 0,
        .capabilities = 0
#endif
    };
    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
        swapl(&reply.uid);
        swapl(&reply.gid);
        swapl(&reply.signature);
    }
    WriteToClient(client, sizeof(xXF86BigfontQueryVersionReply), &reply);
    return Success;
}

static void
swapCharInfo(xCharInfo * pCI)
{
    swaps(&pCI->leftSideBearing);
    swaps(&pCI->rightSideBearing);
    swaps(&pCI->characterWidth);
    swaps(&pCI->ascent);
    swaps(&pCI->descent);
    swaps(&pCI->attributes);
}

/* static CARD32 hashCI (xCharInfo *p); */
#define hashCI(p) \
	(CARD32)(((p->leftSideBearing << 27) + (p->leftSideBearing >> 5) + \
	          (p->rightSideBearing << 23) + (p->rightSideBearing >> 9) + \
	          (p->characterWidth << 16) + \
	          (p->ascent << 11) + (p->descent << 6)) ^ p->attributes)

static int
ProcXF86BigfontQueryFont(ClientPtr client)
{
    FontPtr pFont;

    REQUEST(xXF86BigfontQueryFontReq);
    CARD32 stuff_flags;
    xCharInfo *pmax;
    xCharInfo *pmin;
    int nCharInfos;
    int shmid;

#ifdef HAS_SHM
    ShmDescPtr pDesc = NULL;
#else
#define pDesc 0
#endif
    xCharInfo *pCI;
    CARD16 *pIndex2UniqIndex;
    CARD16 *pUniqIndex2Index;
    CARD32 nUniqCharInfos;

#if 0
    REQUEST_SIZE_MATCH(xXF86BigfontQueryFontReq);
#else
    switch (client->req_len) {
    case 2:                    /* client with version 1.0 libX11 */
        stuff_flags = (client->local &&
                       !client->swapped ? XF86Bigfont_FLAGS_Shm : 0);
        break;
    case 3:                    /* client with version 1.1 libX11 */
        stuff_flags = stuff->flags;
        break;
    default:
        return BadLength;
    }
#endif
    if (dixLookupFontable(&pFont, stuff->id, client, DixGetAttrAccess) !=
        Success)
        return BadFont;         /* procotol spec says only error is BadFont */

    pmax = FONTINKMAX(pFont);
    pmin = FONTINKMIN(pFont);
    nCharInfos =
        (pmax->rightSideBearing == pmin->rightSideBearing
         && pmax->leftSideBearing == pmin->leftSideBearing
         && pmax->descent == pmin->descent
         && pmax->ascent == pmin->ascent
         && pmax->characterWidth == pmin->characterWidth)
        ? 0 : N2dChars(pFont);
    shmid = -1;
    pCI = NULL;
    pIndex2UniqIndex = NULL;
    pUniqIndex2Index = NULL;
    nUniqCharInfos = 0;

    if (nCharInfos > 0) {
#ifdef HAS_SHM
        if (!badSysCall)
            pDesc = (ShmDescPtr) FontGetPrivate(pFont, FontShmdescIndex);
        if (pDesc) {
            pCI = (xCharInfo *) pDesc->attach_addr;
            if (stuff_flags & XF86Bigfont_FLAGS_Shm)
                shmid = pDesc->shmid;
        }
        else {
            if (stuff_flags & XF86Bigfont_FLAGS_Shm && !badSysCall)
                pDesc = shmalloc(nCharInfos * sizeof(xCharInfo)
                                 + sizeof(CARD32));
            if (pDesc) {
                pCI = (xCharInfo *) pDesc->attach_addr;
                shmid = pDesc->shmid;
            }
            else {
#endif
                pCI = xallocarray(nCharInfos, sizeof(xCharInfo));
                if (!pCI)
                    return BadAlloc;
#ifdef HAS_SHM
            }
#endif
            /* Fill nCharInfos starting at pCI. */
            {
                xCharInfo *prCI = pCI;
                int ninfos = 0;
                int ncols = pFont->info.lastCol - pFont->info.firstCol + 1;
                int row;

                for (row = pFont->info.firstRow;
                     row <= pFont->info.lastRow && ninfos < nCharInfos; row++) {
                    unsigned char chars[512];
                    xCharInfo *tmpCharInfos[256];
                    unsigned long count;
                    int col;
                    unsigned long i;

                    i = 0;
                    for (col = pFont->info.firstCol;
                         col <= pFont->info.lastCol; col++) {
                        chars[i++] = row;
                        chars[i++] = col;
                    }
                    (*pFont->get_metrics) (pFont, ncols, chars, TwoD16Bit,
                                           &count, tmpCharInfos);
                    for (i = 0; i < count && ninfos < nCharInfos; i++) {
                        *prCI++ = *tmpCharInfos[i];
                        ninfos++;
                    }
                }
            }
#ifdef HAS_SHM
            if (pDesc && !badSysCall) {
                *(CARD32 *) (pCI + nCharInfos) = signature;
                if (!xfont2_font_set_private(pFont, FontShmdescIndex, pDesc)) {
                    shmdealloc(pDesc);
                    return BadAlloc;
                }
            }
        }
#endif
        if (shmid == -1) {
            /* Cannot use shared memory, so remove-duplicates the xCharInfos
               using a temporary hash table. */
            /* Note that CARD16 is suitable as index type, because
               nCharInfos <= 0x10000. */
            CARD32 hashModulus;
            CARD16 *pHash2UniqIndex;
            CARD16 *pUniqIndex2NextUniqIndex;
            CARD32 NextIndex;
            CARD32 NextUniqIndex;
            CARD16 *tmp;
            CARD32 i, j;

            hashModulus = 67;
            if (hashModulus > nCharInfos + 1)
                hashModulus = nCharInfos + 1;

            tmp = xallocarray(4 * nCharInfos + 1, sizeof(CARD16));
            if (!tmp) {
                if (!pDesc)
                    free(pCI);
                return BadAlloc;
            }
            pIndex2UniqIndex = tmp;
            /* nCharInfos elements */
            pUniqIndex2Index = tmp + nCharInfos;
            /* max. nCharInfos elements */
            pUniqIndex2NextUniqIndex = tmp + 2 * nCharInfos;
            /* max. nCharInfos elements */
            pHash2UniqIndex = tmp + 3 * nCharInfos;
            /* hashModulus (<= nCharInfos+1) elements */

            /* Note that we can use 0xffff as end-of-list indicator, because
               even if nCharInfos = 0x10000, 0xffff can not occur as valid
               entry before the last element has been inserted. And once the
               last element has been inserted, we don't need the hash table
               any more. */
            for (j = 0; j < hashModulus; j++)
                pHash2UniqIndex[j] = (CARD16) (-1);

            NextUniqIndex = 0;
            for (NextIndex = 0; NextIndex < nCharInfos; NextIndex++) {
                xCharInfo *p = &pCI[NextIndex];
                CARD32 hashCode = hashCI(p) % hashModulus;

                for (i = pHash2UniqIndex[hashCode];
                     i != (CARD16) (-1); i = pUniqIndex2NextUniqIndex[i]) {
                    j = pUniqIndex2Index[i];
                    if (pCI[j].leftSideBearing == p->leftSideBearing
                        && pCI[j].rightSideBearing == p->rightSideBearing
                        && pCI[j].characterWidth == p->characterWidth
                        && pCI[j].ascent == p->ascent
                        && pCI[j].descent == p->descent
                        && pCI[j].attributes == p->attributes)
                        break;
                }
                if (i != (CARD16) (-1)) {
                    /* Found *p at Index j, UniqIndex i */
                    pIndex2UniqIndex[NextIndex] = i;
                }
                else {
                    /* Allocate a new entry in the Uniq table */
                    if (hashModulus <= 2 * NextUniqIndex
                        && hashModulus < nCharInfos + 1) {
                        /* Time to increate hash table size */
                        hashModulus = 2 * hashModulus + 1;
                        if (hashModulus > nCharInfos + 1)
                            hashModulus = nCharInfos + 1;
                        for (j = 0; j < hashModulus; j++)
                            pHash2UniqIndex[j] = (CARD16) (-1);
                        for (i = 0; i < NextUniqIndex; i++)
                            pUniqIndex2NextUniqIndex[i] = (CARD16) (-1);
                        for (i = 0; i < NextUniqIndex; i++) {
                            j = pUniqIndex2Index[i];
                            p = &pCI[j];
                            hashCode = hashCI(p) % hashModulus;
                            pUniqIndex2NextUniqIndex[i] =
                                pHash2UniqIndex[hashCode];
                            pHash2UniqIndex[hashCode] = i;
                        }
                        p = &pCI[NextIndex];
                        hashCode = hashCI(p) % hashModulus;
                    }
                    i = NextUniqIndex++;
                    pUniqIndex2NextUniqIndex[i] = pHash2UniqIndex[hashCode];
                    pHash2UniqIndex[hashCode] = i;
                    pUniqIndex2Index[i] = NextIndex;
                    pIndex2UniqIndex[NextIndex] = i;
                }
            }
            nUniqCharInfos = NextUniqIndex;
            /* fprintf(stderr, "font metrics: nCharInfos = %d, nUniqCharInfos = %d, hashModulus = %d\n", nCharInfos, nUniqCharInfos, hashModulus); */
        }
    }

    {
        int nfontprops = pFont->info.nprops;
        int rlength = sizeof(xXF86BigfontQueryFontReply)
            + nfontprops * sizeof(xFontProp)
            + (nCharInfos > 0 && shmid == -1
               ? nUniqCharInfos * sizeof(xCharInfo)
               + (nCharInfos + 1) / 2 * 2 * sizeof(CARD16)
               : 0);
        xXF86BigfontQueryFontReply *reply = calloc(1, rlength);
        char *p;

        if (!reply) {
            if (nCharInfos > 0) {
                if (shmid == -1)
                    free(pIndex2UniqIndex);
                if (!pDesc)
                    free(pCI);
            }
            return BadAlloc;
        }
        reply->type = X_Reply;
        reply->length = bytes_to_int32(rlength - sizeof(xGenericReply));
        reply->sequenceNumber = client->sequence;
        reply->minBounds = pFont->info.ink_minbounds;
        reply->maxBounds = pFont->info.ink_maxbounds;
        reply->minCharOrByte2 = pFont->info.firstCol;
        reply->maxCharOrByte2 = pFont->info.lastCol;
        reply->defaultChar = pFont->info.defaultCh;
        reply->nFontProps = pFont->info.nprops;
        reply->drawDirection = pFont->info.drawDirection;
        reply->minByte1 = pFont->info.firstRow;
        reply->maxByte1 = pFont->info.lastRow;
        reply->allCharsExist = pFont->info.allExist;
        reply->fontAscent = pFont->info.fontAscent;
        reply->fontDescent = pFont->info.fontDescent;
        reply->nCharInfos = nCharInfos;
        reply->nUniqCharInfos = nUniqCharInfos;
        reply->shmid = shmid;
        reply->shmsegoffset = 0;
        if (client->swapped) {
            swaps(&reply->sequenceNumber);
            swapl(&reply->length);
            swapCharInfo(&reply->minBounds);
            swapCharInfo(&reply->maxBounds);
            swaps(&reply->minCharOrByte2);
            swaps(&reply->maxCharOrByte2);
            swaps(&reply->defaultChar);
            swaps(&reply->nFontProps);
            swaps(&reply->fontAscent);
            swaps(&reply->fontDescent);
            swapl(&reply->nCharInfos);
            swapl(&reply->nUniqCharInfos);
            swapl(&reply->shmid);
            swapl(&reply->shmsegoffset);
        }
        p = (char *) &reply[1];
        {
            FontPropPtr pFP;
            xFontProp *prFP;
            int i;

            for (i = 0, pFP = pFont->info.props, prFP = (xFontProp *) p;
                 i < nfontprops; i++, pFP++, prFP++) {
                prFP->name = pFP->name;
                prFP->value = pFP->value;
                if (client->swapped) {
                    swapl(&prFP->name);
                    swapl(&prFP->value);
                }
            }
            p = (char *) prFP;
        }
        if (nCharInfos > 0 && shmid == -1) {
            xCharInfo *pci;
            CARD16 *ps;
            int i, j;

            pci = (xCharInfo *) p;
            for (i = 0; i < nUniqCharInfos; i++, pci++) {
                *pci = pCI[pUniqIndex2Index[i]];
                if (client->swapped)
                    swapCharInfo(pci);
            }
            ps = (CARD16 *) pci;
            for (j = 0; j < nCharInfos; j++, ps++) {
                *ps = pIndex2UniqIndex[j];
                if (client->swapped) {
                    swaps(ps);
                }
            }
        }
        WriteToClient(client, rlength, reply);
        free(reply);
        if (nCharInfos > 0) {
            if (shmid == -1)
                free(pIndex2UniqIndex);
            if (!pDesc)
                free(pCI);
        }
        return Success;
    }
}

static int
ProcXF86BigfontDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XF86BigfontQueryVersion:
        return ProcXF86BigfontQueryVersion(client);
    case X_XF86BigfontQueryFont:
        return ProcXF86BigfontQueryFont(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcXF86BigfontQueryVersion(ClientPtr client)
{
    REQUEST(xXF86BigfontQueryVersionReq);

    swaps(&stuff->length);
    return ProcXF86BigfontQueryVersion(client);
}

static int _X_COLD
SProcXF86BigfontQueryFont(ClientPtr client)
{
    REQUEST(xXF86BigfontQueryFontReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86BigfontQueryFontReq);
    swapl(&stuff->id);
    return ProcXF86BigfontQueryFont(client);
}

static int _X_COLD
SProcXF86BigfontDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XF86BigfontQueryVersion:
        return SProcXF86BigfontQueryVersion(client);
    case X_XF86BigfontQueryFont:
        return SProcXF86BigfontQueryFont(client);
    default:
        return BadRequest;
    }
}

void
XFree86BigfontExtensionInit(void)
{
    if (AddExtension(XF86BIGFONTNAME,
                     XF86BigfontNumberEvents,
                     XF86BigfontNumberErrors,
                     ProcXF86BigfontDispatch,
                     SProcXF86BigfontDispatch,
                     XF86BigfontResetProc, StandardMinorOpcode)) {
#ifdef HAS_SHM
#ifdef MUST_CHECK_FOR_SHM_SYSCALL
        /*
         * Note: Local-clients will not be optimized without shared memory
         * support. Remote-client optimization does not depend on shared
         * memory support.  Thus, the extension is still registered even
         * when shared memory support is not functional.
         */
        if (!CheckForShmSyscall()) {
            ErrorF(XF86BIGFONTNAME
                   " extension local-client optimization disabled due to lack of shared memory support in the kernel\n");
            return;
        }
#endif

        srand((unsigned int) time(NULL));
        signature = ((unsigned int) (65536.0 / (RAND_MAX + 1.0) * rand()) << 16)
            + (unsigned int) (65536.0 / (RAND_MAX + 1.0) * rand());
        /* fprintf(stderr, "signature = 0x%08X\n", signature); */

        FontShmdescIndex = xfont2_allocate_font_private_index();

#if !defined(CSRG_BASED) && !defined(__CYGWIN__)
        pagesize = SHMLBA;
#else
#ifdef _SC_PAGESIZE
        pagesize = sysconf(_SC_PAGESIZE);
#else
        pagesize = getpagesize();
#endif
#endif
#endif
    }
}
