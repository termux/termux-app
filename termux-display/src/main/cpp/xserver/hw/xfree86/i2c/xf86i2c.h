/*
 *  Copyright (C) 1998 Itai Nahshon, Michael Schimek
 */

#ifndef _XF86I2C_H
#define _XF86I2C_H

#include "regionstr.h"
#include "xf86.h"

typedef unsigned char I2CByte;
typedef unsigned short I2CSlaveAddr;

typedef struct _I2CBusRec *I2CBusPtr;
typedef struct _I2CDevRec *I2CDevPtr;

/* I2C masters have to register themselves */

typedef struct _I2CBusRec {
    char *BusName;
    int scrnIndex;
    ScrnInfoPtr pScrn;

    void (*I2CUDelay) (I2CBusPtr b, int usec);

    void (*I2CPutBits) (I2CBusPtr b, int scl, int sda);
    void (*I2CGetBits) (I2CBusPtr b, int *scl, int *sda);

    /* Look at the generic routines to see how these functions should behave. */

    Bool (*I2CStart) (I2CBusPtr b, int timeout);
    Bool (*I2CAddress) (I2CDevPtr d, I2CSlaveAddr);
    void (*I2CStop) (I2CDevPtr d);
    Bool (*I2CPutByte) (I2CDevPtr d, I2CByte data);
    Bool (*I2CGetByte) (I2CDevPtr d, I2CByte * data, Bool);

    DevUnion DriverPrivate;

    int HoldTime;               /* 1 / bus clock frequency, 5 or 2 usec */

    int BitTimeout;             /* usec */
    int ByteTimeout;            /* usec */
    int AcknTimeout;            /* usec */
    int StartTimeout;           /* usec */
    int RiseFallTime;           /* usec */

    I2CDevPtr FirstDev;
    I2CBusPtr NextBus;
    Bool (*I2CWriteRead) (I2CDevPtr d, I2CByte * WriteBuffer, int nWrite,
                          I2CByte * ReadBuffer, int nRead);
} I2CBusRec;

#define CreateI2CBusRec		xf86CreateI2CBusRec
extern _X_EXPORT I2CBusPtr xf86CreateI2CBusRec(void);

#define DestroyI2CBusRec	xf86DestroyI2CBusRec
extern _X_EXPORT void xf86DestroyI2CBusRec(I2CBusPtr pI2CBus, Bool unalloc,
                                           Bool devs_too);
#define I2CBusInit		xf86I2CBusInit
extern _X_EXPORT Bool xf86I2CBusInit(I2CBusPtr pI2CBus);

extern _X_EXPORT I2CBusPtr xf86I2CFindBus(int scrnIndex, char *name);
extern _X_EXPORT int xf86I2CGetScreenBuses(int scrnIndex,
                                           I2CBusPtr ** pppI2CBus);

/* I2C slave devices */

typedef struct _I2CDevRec {
    const char *DevName;

    int BitTimeout;             /* usec */
    int ByteTimeout;            /* usec */
    int AcknTimeout;            /* usec */
    int StartTimeout;           /* usec */

    I2CSlaveAddr SlaveAddr;
    I2CBusPtr pI2CBus;
    I2CDevPtr NextDev;
    DevUnion DriverPrivate;
} I2CDevRec;

#define CreateI2CDevRec		xf86CreateI2CDevRec
extern _X_EXPORT I2CDevPtr xf86CreateI2CDevRec(void);
extern _X_EXPORT void xf86DestroyI2CDevRec(I2CDevPtr pI2CDev, Bool unalloc);

#define I2CDevInit		xf86I2CDevInit
extern _X_EXPORT Bool xf86I2CDevInit(I2CDevPtr pI2CDev);
extern _X_EXPORT I2CDevPtr xf86I2CFindDev(I2CBusPtr, I2CSlaveAddr);

/* See descriptions of these functions in xf86i2c.c */

#define I2CProbeAddress		xf86I2CProbeAddress
extern _X_EXPORT Bool xf86I2CProbeAddress(I2CBusPtr pI2CBus, I2CSlaveAddr);

#define		I2C_WriteRead xf86I2CWriteRead
extern _X_EXPORT Bool xf86I2CWriteRead(I2CDevPtr d, I2CByte * WriteBuffer,
                                       int nWrite, I2CByte * ReadBuffer,
                                       int nRead);
#define 	xf86I2CRead(d, rb, nr) xf86I2CWriteRead(d, NULL, 0, rb, nr)

extern _X_EXPORT Bool xf86I2CReadStatus(I2CDevPtr d, I2CByte * pbyte);
extern _X_EXPORT Bool xf86I2CReadByte(I2CDevPtr d, I2CByte subaddr,
                                      I2CByte * pbyte);
extern _X_EXPORT Bool xf86I2CReadBytes(I2CDevPtr d, I2CByte subaddr,
                                       I2CByte * pbyte, int n);
extern _X_EXPORT Bool xf86I2CReadWord(I2CDevPtr d, I2CByte subaddr,
                                      unsigned short *pword);
#define 	xf86I2CWrite(d, wb, nw) xf86I2CWriteRead(d, wb, nw, NULL, 0)
extern _X_EXPORT Bool xf86I2CWriteByte(I2CDevPtr d, I2CByte subaddr,
                                       I2CByte byte);
extern _X_EXPORT Bool xf86I2CWriteBytes(I2CDevPtr d, I2CByte subaddr,
                                        I2CByte * WriteBuffer, int nWrite);
extern _X_EXPORT Bool xf86I2CWriteWord(I2CDevPtr d, I2CByte subaddr,
                                       unsigned short word);
extern _X_EXPORT Bool xf86I2CWriteVec(I2CDevPtr d, I2CByte * vec, int nValues);

#endif /*_XF86I2C_H */
