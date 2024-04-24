
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _XIBARRIERS_H_
#define _XIBARRIERS_H_

#include "resource.h"

extern _X_EXPORT RESTYPE PointerBarrierType;

struct PointerBarrier {
    INT16 x1, x2, y1, y2;
    CARD32 directions;
};

int
barrier_get_direction(int, int, int, int);
BOOL
barrier_is_blocking(const struct PointerBarrier *, int, int, int, int,
                        double *);
BOOL
barrier_is_blocking_direction(const struct PointerBarrier *, int);
void
barrier_clamp_to_barrier(struct PointerBarrier *barrier, int dir, int *x,
                             int *y);

#include <xfixesint.h>

int
XICreatePointerBarrier(ClientPtr client,
                       xXFixesCreatePointerBarrierReq * stuff);

int
XIDestroyPointerBarrier(ClientPtr client,
                        xXFixesDestroyPointerBarrierReq * stuff);

Bool XIBarrierInit(void);
void XIBarrierReset(void);

int SProcXIBarrierReleasePointer(ClientPtr client);
int ProcXIBarrierReleasePointer(ClientPtr client);

void XIBarrierNewMasterDevice(ClientPtr client, int deviceid);
void XIBarrierRemoveMasterDevice(ClientPtr client, int deviceid);

#endif /* _XIBARRIERS_H_ */
