/*

Copyright 1995, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
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

    See the header set.h for a description of the set ADT.

    Implementation Strategy

    A bit vector is an obvious choice to represent the set, but may take
    too much memory, depending on the numerically largest member in the
    set.  One expected common case is for the client to ask for *all*
    protocol.  This means it would ask for minor opcodes 0 through 65535.
    Representing this as a bit vector takes 8K -- and there may be
    multiple minor opcode intervals, as many as one per major (extension)
    opcode).  In such cases, a list-of-intervals representation would be
    preferable to reduce memory consumption.  Both representations will be
    implemented, and RecordCreateSet will decide heuristically which one
    to use based on the set members.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "misc.h"
#include "set.h"

static int
maxMemberInInterval(RecordSetInterval * pIntervals, int nIntervals)
{
    int i;
    int maxMember = -1;

    for (i = 0; i < nIntervals; i++) {
        if (maxMember < (int) pIntervals[i].last)
            maxMember = pIntervals[i].last;
    }
    return maxMember;
}

static void
NoopDestroySet(RecordSetPtr pSet)
{
}

/***************************************************************************/

/* set operations for bit vector representation */

typedef struct {
    RecordSetRec baseSet;
    int maxMember;
    /* followed by the bit vector itself */
} BitVectorSet, *BitVectorSetPtr;

#define BITS_PER_LONG (sizeof(unsigned long) * 8)

static void
BitVectorDestroySet(RecordSetPtr pSet)
{
    free(pSet);
}

static unsigned long
BitVectorIsMemberOfSet(RecordSetPtr pSet, int pm)
{
    BitVectorSetPtr pbvs = (BitVectorSetPtr) pSet;
    unsigned long *pbitvec;

    if ((int) pm > pbvs->maxMember)
        return FALSE;
    pbitvec = (unsigned long *) (&pbvs[1]);
    return (pbitvec[pm / BITS_PER_LONG] &
            ((unsigned long) 1 << (pm % BITS_PER_LONG)));
}

static int
BitVectorFindBit(RecordSetPtr pSet, int iterbit, Bool bitval)
{
    BitVectorSetPtr pbvs = (BitVectorSetPtr) pSet;
    unsigned long *pbitvec = (unsigned long *) (&pbvs[1]);
    int startlong;
    int startbit;
    int walkbit;
    int maxMember;
    unsigned long skipval;
    unsigned long bits;
    unsigned long usefulbits;

    startlong = iterbit / BITS_PER_LONG;
    pbitvec += startlong;
    startbit = startlong * BITS_PER_LONG;
    skipval = bitval ? 0L : ~0L;
    maxMember = pbvs->maxMember;

    if (startbit > maxMember)
        return -1;
    bits = *pbitvec;
    usefulbits = ~(((unsigned long) 1 << (iterbit - startbit)) - 1);
    if ((bits & usefulbits) == (skipval & usefulbits)) {
        pbitvec++;
        startbit += BITS_PER_LONG;

        while (startbit <= maxMember && *pbitvec == skipval) {
            pbitvec++;
            startbit += BITS_PER_LONG;
        }
        if (startbit > maxMember)
            return -1;
    }

    walkbit = (startbit < iterbit) ? iterbit - startbit : 0;

    bits = *pbitvec;
    while (walkbit < BITS_PER_LONG &&
           ((!(bits & ((unsigned long) 1 << walkbit))) == bitval))
        walkbit++;

    return startbit + walkbit;
}

static RecordSetIteratePtr
BitVectorIterateSet(RecordSetPtr pSet, RecordSetIteratePtr pIter,
                    RecordSetInterval * pInterval)
{
    int iterbit = (int) (long) pIter;
    int b;

    b = BitVectorFindBit(pSet, iterbit, TRUE);
    if (b == -1)
        return (RecordSetIteratePtr) 0;
    pInterval->first = b;

    b = BitVectorFindBit(pSet, b, FALSE);
    pInterval->last = (b < 0) ? ((BitVectorSetPtr) pSet)->maxMember : b - 1;
    return (RecordSetIteratePtr) (long) (pInterval->last + 1);
}

static RecordSetOperations BitVectorSetOperations = {
    BitVectorDestroySet, BitVectorIsMemberOfSet, BitVectorIterateSet
};

static RecordSetOperations BitVectorNoFreeOperations = {
    NoopDestroySet, BitVectorIsMemberOfSet, BitVectorIterateSet
};

static int
BitVectorSetMemoryRequirements(RecordSetInterval * pIntervals, int nIntervals,
                               int maxMember, int *alignment)
{
    int nlongs;

    *alignment = sizeof(unsigned long);
    nlongs = (maxMember + BITS_PER_LONG) / BITS_PER_LONG;
    return (sizeof(BitVectorSet) + nlongs * sizeof(unsigned long));
}

static RecordSetPtr
BitVectorCreateSet(RecordSetInterval * pIntervals, int nIntervals,
                   void *pMem, int memsize)
{
    BitVectorSetPtr pbvs;
    int i, j;
    unsigned long *pbitvec;

    /* allocate all storage needed by this set in one chunk */

    if (pMem) {
        memset(pMem, 0, memsize);
        pbvs = (BitVectorSetPtr) pMem;
        pbvs->baseSet.ops = &BitVectorNoFreeOperations;
    }
    else {
        pbvs = (BitVectorSetPtr) calloc(1, memsize);
        if (!pbvs)
            return NULL;
        pbvs->baseSet.ops = &BitVectorSetOperations;
    }

    pbvs->maxMember = maxMemberInInterval(pIntervals, nIntervals);

    /* fill in the set */

    pbitvec = (unsigned long *) (&pbvs[1]);
    for (i = 0; i < nIntervals; i++) {
        for (j = pIntervals[i].first; j <= (int) pIntervals[i].last; j++) {
            pbitvec[j / BITS_PER_LONG] |=
                ((unsigned long) 1 << (j % BITS_PER_LONG));
        }
    }
    return (RecordSetPtr) pbvs;
}

/***************************************************************************/

/* set operations for interval list representation */

typedef struct {
    RecordSetRec baseSet;
    int nIntervals;
    /* followed by the intervals (RecordSetInterval) */
} IntervalListSet, *IntervalListSetPtr;

static void
IntervalListDestroySet(RecordSetPtr pSet)
{
    free(pSet);
}

static unsigned long
IntervalListIsMemberOfSet(RecordSetPtr pSet, int pm)
{
    IntervalListSetPtr prls = (IntervalListSetPtr) pSet;
    RecordSetInterval *pInterval = (RecordSetInterval *) (&prls[1]);
    int hi, lo, probe;

    /* binary search */
    lo = 0;
    hi = prls->nIntervals - 1;
    while (lo <= hi) {
        probe = (hi + lo) / 2;
        if (pm >= pInterval[probe].first && pm <= pInterval[probe].last)
            return 1;
        else if (pm < pInterval[probe].first)
            hi = probe - 1;
        else
            lo = probe + 1;
    }
    return 0;
}

static RecordSetIteratePtr
IntervalListIterateSet(RecordSetPtr pSet, RecordSetIteratePtr pIter,
                       RecordSetInterval * pIntervalReturn)
{
    RecordSetInterval *pInterval = (RecordSetInterval *) pIter;
    IntervalListSetPtr prls = (IntervalListSetPtr) pSet;

    if (pInterval == NULL) {
        pInterval = (RecordSetInterval *) (&prls[1]);
    }

    if ((pInterval - (RecordSetInterval *) (&prls[1])) < prls->nIntervals) {
        *pIntervalReturn = *pInterval;
        return (RecordSetIteratePtr) (++pInterval);
    }
    else
        return (RecordSetIteratePtr) NULL;
}

static RecordSetOperations IntervalListSetOperations = {
    IntervalListDestroySet, IntervalListIsMemberOfSet, IntervalListIterateSet
};

static RecordSetOperations IntervalListNoFreeOperations = {
    NoopDestroySet, IntervalListIsMemberOfSet, IntervalListIterateSet
};

static int
IntervalListMemoryRequirements(RecordSetInterval * pIntervals, int nIntervals,
                               int maxMember, int *alignment)
{
    *alignment = sizeof(unsigned long);
    return sizeof(IntervalListSet) + nIntervals * sizeof(RecordSetInterval);
}

static RecordSetPtr
IntervalListCreateSet(RecordSetInterval * pIntervals, int nIntervals,
                      void *pMem, int memsize)
{
    IntervalListSetPtr prls;
    int i, j, k;
    RecordSetInterval *stackIntervals = NULL;
    CARD16 first;

    if (nIntervals > 0) {
        stackIntervals = xallocarray(nIntervals, sizeof(RecordSetInterval));
        if (!stackIntervals)
            return NULL;

        /* sort intervals, store in stackIntervals (insertion sort) */

        for (i = 0; i < nIntervals; i++) {
            first = pIntervals[i].first;
            for (j = 0; j < i; j++) {
                if (first < stackIntervals[j].first)
                    break;
            }
            for (k = i; k > j; k--) {
                stackIntervals[k] = stackIntervals[k - 1];
            }
            stackIntervals[j] = pIntervals[i];
        }

        /* merge abutting/overlapping intervals */

        for (i = 0; i < nIntervals - 1;) {
            if ((stackIntervals[i].last + (unsigned int) 1) <
                stackIntervals[i + 1].first) {
                i++;            /* disjoint intervals */
            }
            else {
                stackIntervals[i].last = max(stackIntervals[i].last,
                                             stackIntervals[i + 1].last);
                nIntervals--;
                for (j = i + 1; j < nIntervals; j++)
                    stackIntervals[j] = stackIntervals[j + 1];
            }
        }
    }

    /* allocate and fill in set structure */

    if (pMem) {
        prls = (IntervalListSetPtr) pMem;
        prls->baseSet.ops = &IntervalListNoFreeOperations;
    }
    else {
        prls = (IntervalListSetPtr)
            malloc(sizeof(IntervalListSet) +
                   nIntervals * sizeof(RecordSetInterval));
        if (!prls)
            goto bailout;
        prls->baseSet.ops = &IntervalListSetOperations;
    }
    memcpy(&prls[1], stackIntervals, nIntervals * sizeof(RecordSetInterval));
    prls->nIntervals = nIntervals;
 bailout:
    free(stackIntervals);
    return (RecordSetPtr) prls;
}

typedef RecordSetPtr(*RecordCreateSetProcPtr) (RecordSetInterval * pIntervals,
                                               int nIntervals,
                                               void *pMem, int memsize);

static int
_RecordSetMemoryRequirements(RecordSetInterval * pIntervals, int nIntervals,
                             int *alignment,
                             RecordCreateSetProcPtr * ppCreateSet)
{
    int bmsize, rlsize, bma, rla;
    int maxMember;

    /* find maximum member of set so we know how big to make the bit vector */
    maxMember = maxMemberInInterval(pIntervals, nIntervals);

    bmsize = BitVectorSetMemoryRequirements(pIntervals, nIntervals, maxMember,
                                            &bma);
    rlsize = IntervalListMemoryRequirements(pIntervals, nIntervals, maxMember,
                                            &rla);
    if (((nIntervals > 1) && (maxMember <= 255))
        || (bmsize < rlsize)) {
        *alignment = bma;
        *ppCreateSet = BitVectorCreateSet;
        return bmsize;
    }
    else {
        *alignment = rla;
        *ppCreateSet = IntervalListCreateSet;
        return rlsize;
    }
}

/***************************************************************************/

/* user-visible functions */

int
RecordSetMemoryRequirements(RecordSetInterval * pIntervals, int nIntervals,
                            int *alignment)
{
    RecordCreateSetProcPtr pCreateSet;

    return _RecordSetMemoryRequirements(pIntervals, nIntervals, alignment,
                                        &pCreateSet);
}

RecordSetPtr
RecordCreateSet(RecordSetInterval * pIntervals, int nIntervals, void *pMem,
                int memsize)
{
    RecordCreateSetProcPtr pCreateSet;
    int alignment;
    int size;

    size = _RecordSetMemoryRequirements(pIntervals, nIntervals, &alignment,
                                        &pCreateSet);
    if (pMem) {
        if (((long) pMem & (alignment - 1)) || memsize < size)
            return NULL;
    }
    return (*pCreateSet) (pIntervals, nIntervals, pMem, size);
}
