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
	  A Set Abstract Data Type (ADT) for the RECORD Extension
			   David P. Wiggins
			       7/25/95

    The RECORD extension server code needs to maintain sets of numbers
    that designate protocol message types.  In most cases the interval of
    numbers starts at 0 and does not exceed 255, but in a few cases (minor
    opcodes of extension requests) the maximum is 65535.  This disparity
    suggests that a single set representation may not be suitable for all
    sets, especially given that server memory is precious.  We introduce a
    set ADT to hide implementation differences so that multiple
    simultaneous set representations can exist.  A single interface is
    presented to the set user regardless of the implementation in use for
    a particular set.

    The existing RECORD SI appears to require only four set operations:
    create (given a list of members), destroy, see if a particular number
    is a member of the set, and iterate over the members of a set.  Though
    many more set operations are imaginable, to keep the code space down,
    we won't provide any more operations than are needed.

    The following types and functions/macros define the ADT.
*/

/* an interval of set members */
typedef struct {
    CARD16 first;
    CARD16 last;
} RecordSetInterval;

typedef struct _RecordSetRec *RecordSetPtr;     /* primary set type */

typedef void *RecordSetIteratePtr;

/* table of function pointers for set operations.
   set users should never declare a variable of this type.
*/
typedef struct {
    void (*DestroySet) (RecordSetPtr pSet);
    unsigned long (*IsMemberOfSet) (RecordSetPtr pSet, int possible_member);
     RecordSetIteratePtr(*IterateSet) (RecordSetPtr pSet,
                                       RecordSetIteratePtr pIter,
                                       RecordSetInterval * interval);
} RecordSetOperations;

/* "base class" for sets.
   set users should never declare a variable of this type.
 */
typedef struct _RecordSetRec {
    RecordSetOperations *ops;
} RecordSetRec;

RecordSetPtr RecordCreateSet(RecordSetInterval * intervals,
                             int nintervals, void *pMem, int memsize);
/*
    RecordCreateSet creates and returns a new set having members specified
    by intervals and nintervals.  nintervals is the number of RecordSetInterval
    structures pointed to by intervals.  The elements belonging to the new
    set are determined as follows.  For each RecordSetInterval structure, the
    elements between first and last inclusive are members of the new set.
    If a RecordSetInterval's first field is greater than its last field, the
    results are undefined.  It is valid to create an empty set (nintervals ==
    0).  If RecordCreateSet returns NULL, the set could not be created due
    to resource constraints.
*/

int RecordSetMemoryRequirements(RecordSetInterval * /*pIntervals */ ,
                                int /*nintervals */ ,
                                int *   /*alignment */
    );

#define RecordDestroySet(_pSet) \
	/* void */ (*_pSet->ops->DestroySet)(/* RecordSetPtr */ _pSet)
/*
    RecordDestroySet frees all resources used by _pSet.  _pSet should not be
    used after it is destroyed.
*/

#define RecordIsMemberOfSet(_pSet, _m) \
  /* unsigned long */ (*_pSet->ops->IsMemberOfSet)(/* RecordSetPtr */ _pSet, \
						   /* int */ _m)
/*
    RecordIsMemberOfSet returns a non-zero value if _m is a member of
    _pSet, else it returns zero.
*/

#define RecordIterateSet(_pSet, _pIter, _interval) \
 /* RecordSetIteratePtr */ (*_pSet->ops->IterateSet)(/* RecordSetPtr */ _pSet,\
	/* RecordSetIteratePtr */ _pIter, /* RecordSetInterval */ _interval)
/*
    RecordIterateSet returns successive intervals of members of _pSet.  If
    _pIter is NULL, the first interval of set members is copied into _interval.
    The return value should be passed as _pIter in the next call to
    RecordIterateSet to obtain the next interval.  When the return value is
    NULL, there were no more intervals in the set, and nothing is copied into
    the _interval parameter.  Intervals appear in increasing numerical order
    with no overlap between intervals.  As such, the list of intervals produced
    by RecordIterateSet may not match the list of intervals that were passed
    in RecordCreateSet.  Typical usage:

	pIter = NULL;
	while (pIter = RecordIterateSet(pSet, pIter, &interval))
	{
	    process interval;
	}
*/
