/*
 * Copyright (C) 1998 Itai Nahshon, Michael Schimek
 *
 * The original code was derived from and inspired by
 * the I2C driver from the Linux kernel.
 *      (c) 1998 Gerd Knorr <kraxel@cs.tu-berlin.de>
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <sys/time.h>
#include <string.h>

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "validate.h"
#include "resource.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "xf86i2c.h"

#define I2C_TIMEOUT(x)	/*(x)*/ /* Report timeouts */
#define I2C_TRACE(x)    /*(x)*/ /* Report progress */

/* This is the default I2CUDelay function if not supplied by the driver.
 * High level I2C interfaces implementing the bus protocol in hardware
 * should supply this function too.
 *
 * Delay execution at least usec microseconds.
 * All values 0 to 1e6 inclusive must be expected.
 */

static void
I2CUDelay(I2CBusPtr b, int usec)
{
    struct timeval begin, cur;
    long d_secs, d_usecs;
    long diff;

    if (usec > 0) {
        X_GETTIMEOFDAY(&begin);
        do {
            /* It would be nice to use {xf86}usleep,
             * but usleep (1) takes >10000 usec !
             */
            X_GETTIMEOFDAY(&cur);
            d_secs = (cur.tv_sec - begin.tv_sec);
            d_usecs = (cur.tv_usec - begin.tv_usec);
            diff = d_secs * 1000000 + d_usecs;
        } while (diff >= 0 && diff < (usec + 1));
    }
}

/* Most drivers will register just with GetBits/PutBits functions.
 * The following functions implement a software I2C protocol
 * by using the promitive functions given by the driver.
 * ================================================================
 *
 * It is assumed that there is just one master on the I2C bus, therefore
 * there is no explicit test for conflits.
 */

#define RISEFALLTIME 2          /* usec, actually 300 to 1000 ns according to the i2c specs */

/* Some devices will hold SCL low to slow down the bus or until
 * ready for transmission.
 *
 * This condition will be noticed when the master tries to raise
 * the SCL line. You can set the timeout to zero if the slave device
 * does not support this clock synchronization.
 */

static Bool
I2CRaiseSCL(I2CBusPtr b, int sda, int timeout)
{
    int i, scl;

    b->I2CPutBits(b, 1, sda);
    b->I2CUDelay(b, b->RiseFallTime);

    for (i = timeout; i > 0; i -= b->RiseFallTime) {
        b->I2CGetBits(b, &scl, &sda);
        if (scl)
            break;
        b->I2CUDelay(b, b->RiseFallTime);
    }

    if (i <= 0) {
        I2C_TIMEOUT(ErrorF
                    ("[I2CRaiseSCL(<%s>, %d, %d) timeout]", b->BusName, sda,
                     timeout));
        return FALSE;
    }

    return TRUE;
}

/* Send a start signal on the I2C bus. The start signal notifies
 * devices that a new transaction is initiated by the bus master.
 *
 * The start signal is always followed by a slave address.
 * Slave addresses are 8+ bits. The first 7 bits identify the
 * device and the last bit signals if this is a read (1) or
 * write (0) operation.
 *
 * There may be more than one start signal on one transaction.
 * This happens for example on some devices that allow reading
 * of registers. First send a start bit followed by the device
 * address (with the last bit 0) and the register number. Then send
 * a new start bit with the device address (with the last bit 1)
 * and then read the value from the device.
 *
 * Note this is function does not implement a multiple master
 * arbitration procedure.
 */

static Bool
I2CStart(I2CBusPtr b, int timeout)
{
    if (!I2CRaiseSCL(b, 1, timeout))
        return FALSE;

    b->I2CPutBits(b, 1, 0);
    b->I2CUDelay(b, b->HoldTime);
    b->I2CPutBits(b, 0, 0);
    b->I2CUDelay(b, b->HoldTime);

    I2C_TRACE(ErrorF("\ni2c: <"));

    return TRUE;
}

/* This is the default I2CStop function if not supplied by the driver.
 *
 * Signal devices on the I2C bus that a transaction on the
 * bus has finished. There may be more than one start signal
 * on a transaction but only one stop signal.
 */

static void
I2CStop(I2CDevPtr d)
{
    I2CBusPtr b = d->pI2CBus;

    b->I2CPutBits(b, 0, 0);
    b->I2CUDelay(b, b->RiseFallTime);

    b->I2CPutBits(b, 1, 0);
    b->I2CUDelay(b, b->HoldTime);
    b->I2CPutBits(b, 1, 1);
    b->I2CUDelay(b, b->HoldTime);

    I2C_TRACE(ErrorF(">\n"));
}

/* Write/Read a single bit to/from a device.
 * Return FALSE if a timeout occurs.
 */

static Bool
I2CWriteBit(I2CBusPtr b, int sda, int timeout)
{
    Bool r;

    b->I2CPutBits(b, 0, sda);
    b->I2CUDelay(b, b->RiseFallTime);

    r = I2CRaiseSCL(b, sda, timeout);
    b->I2CUDelay(b, b->HoldTime);

    b->I2CPutBits(b, 0, sda);
    b->I2CUDelay(b, b->HoldTime);

    return r;
}

static Bool
I2CReadBit(I2CBusPtr b, int *psda, int timeout)
{
    Bool r;
    int scl;

    r = I2CRaiseSCL(b, 1, timeout);
    b->I2CUDelay(b, b->HoldTime);

    b->I2CGetBits(b, &scl, psda);

    b->I2CPutBits(b, 0, 1);
    b->I2CUDelay(b, b->HoldTime);

    return r;
}

/* This is the default I2CPutByte function if not supplied by the driver.
 *
 * A single byte is sent to the device.
 * The function returns FALSE if a timeout occurs, you should send
 * a stop condition afterwards to reset the bus.
 *
 * A timeout occurs,
 * if the slave pulls SCL to slow down the bus more than ByteTimeout usecs,
 * or slows down the bus for more than BitTimeout usecs for each bit,
 * or does not send an ACK bit (0) to acknowledge the transmission within
 * AcknTimeout usecs, but a NACK (1) bit.
 *
 * AcknTimeout must be at least b->HoldTime, the other timeouts can be
 * zero according to the comment on I2CRaiseSCL.
 */

static Bool
I2CPutByte(I2CDevPtr d, I2CByte data)
{
    Bool r;
    int i, scl, sda;
    I2CBusPtr b = d->pI2CBus;

    if (!I2CWriteBit(b, (data >> 7) & 1, d->ByteTimeout))
        return FALSE;

    for (i = 6; i >= 0; i--)
        if (!I2CWriteBit(b, (data >> i) & 1, d->BitTimeout))
            return FALSE;

    b->I2CPutBits(b, 0, 1);
    b->I2CUDelay(b, b->RiseFallTime);

    r = I2CRaiseSCL(b, 1, b->HoldTime);

    if (r) {
        for (i = d->AcknTimeout; i > 0; i -= b->HoldTime) {
            b->I2CUDelay(b, b->HoldTime);
            b->I2CGetBits(b, &scl, &sda);
            if (sda == 0)
                break;
        }

        if (i <= 0) {
            I2C_TIMEOUT(ErrorF("[I2CPutByte(<%s>, 0x%02x, %d, %d, %d) timeout]",
                               b->BusName, data, d->BitTimeout,
                               d->ByteTimeout, d->AcknTimeout));
            r = FALSE;
        }

        I2C_TRACE(ErrorF("W%02x%c ", (int) data, sda ? '-' : '+'));
    }

    b->I2CPutBits(b, 0, 1);
    b->I2CUDelay(b, b->HoldTime);

    return r;
}

/* This is the default I2CGetByte function if not supplied by the driver.
 *
 * A single byte is read from the device.
 * The function returns FALSE if a timeout occurs, you should send
 * a stop condition afterwards to reset the bus.
 *
 * A timeout occurs,
 * if the slave pulls SCL to slow down the bus more than ByteTimeout usecs,
 * or slows down the bus for more than b->BitTimeout usecs for each bit.
 *
 * ByteTimeout must be at least b->HoldTime, the other timeouts can be
 * zero according to the comment on I2CRaiseSCL.
 *
 * For the <last> byte in a sequence the acknowledge bit NACK (1),
 * otherwise ACK (0) will be sent.
 */

static Bool
I2CGetByte(I2CDevPtr d, I2CByte * data, Bool last)
{
    int i, sda;
    I2CBusPtr b = d->pI2CBus;

    b->I2CPutBits(b, 0, 1);
    b->I2CUDelay(b, b->RiseFallTime);

    if (!I2CReadBit(b, &sda, d->ByteTimeout))
        return FALSE;

    *data = (sda > 0) << 7;

    for (i = 6; i >= 0; i--)
        if (!I2CReadBit(b, &sda, d->BitTimeout))
            return FALSE;
        else
            *data |= (sda > 0) << i;

    if (!I2CWriteBit(b, last ? 1 : 0, d->BitTimeout))
        return FALSE;

    I2C_TRACE(ErrorF("R%02x%c ", (int) *data, last ? '+' : '-'));

    return TRUE;
}

/* This is the default I2CAddress function if not supplied by the driver.
 *
 * It creates the start condition, followed by the d->SlaveAddr.
 * Higher level functions must call this routine rather than
 * I2CStart/PutByte because a hardware I2C master may not be able
 * to send a slave address without a start condition.
 *
 * The same timeouts apply as with I2CPutByte and additional a
 * StartTimeout, similar to the ByteTimeout but for the start
 * condition.
 *
 * In case of a timeout, the bus is left in a clean idle condition.
 * I. e. you *must not* send a Stop. If this function succeeds, you *must*.
 *
 * The slave address format is 16 bit, with the legacy _8_bit_ slave address
 * in the least significant byte. This is, the slave address must include the
 * R/_W flag as least significant bit.
 *
 * The most significant byte of the address will be sent _after_ the LSB,
 * but only if the LSB indicates:
 * a) an 11 bit address, this is LSB = 1111 0xxx.
 * b) a 'general call address', this is LSB = 0000 000x - see the I2C specs
 *    for more.
 */

static Bool
I2CAddress(I2CDevPtr d, I2CSlaveAddr addr)
{
    if (I2CStart(d->pI2CBus, d->StartTimeout)) {
        if (I2CPutByte(d, addr & 0xFF)) {
            if ((addr & 0xF8) != 0xF0 && (addr & 0xFE) != 0x00)
                return TRUE;

            if (I2CPutByte(d, (addr >> 8) & 0xFF))
                return TRUE;
        }

        I2CStop(d);
    }

    return FALSE;
}

/* These are the hardware independent I2C helper functions.
 * ========================================================
 */

/* Function for probing. Just send the slave address
 * and return true if the device responds. The slave address
 * must have the lsb set to reflect a read (1) or write (0) access.
 * Don't expect a read- or write-only device will respond otherwise.
 */

Bool
xf86I2CProbeAddress(I2CBusPtr b, I2CSlaveAddr addr)
{
    int r;
    I2CDevRec d;

    d.DevName = "Probing";
    d.BitTimeout = b->BitTimeout;
    d.ByteTimeout = b->ByteTimeout;
    d.AcknTimeout = b->AcknTimeout;
    d.StartTimeout = b->StartTimeout;
    d.SlaveAddr = addr;
    d.pI2CBus = b;
    d.NextDev = NULL;

    r = b->I2CAddress(&d, addr);

    if (r)
        b->I2CStop(&d);

    return r;
}

/* All functions below are related to devices and take the
 * slave address and timeout values from an I2CDevRec. They
 * return FALSE in case of an error (presumably a timeout).
 */

/* General purpose read and write function.
 *
 * 1st, if nWrite > 0
 *   Send a start condition
 *   Send the slave address (1 or 2 bytes) with write flag
 *   Write n bytes from WriteBuffer
 * 2nd, if nRead > 0
 *   Send a start condition [again]
 *   Send the slave address (1 or 2 bytes) with read flag
 *   Read n bytes to ReadBuffer
 * 3rd, if a Start condition has been successfully sent,
 *   Send a Stop condition.
 *
 * The function exits immediately when an error occurs,
 * not processing any data left. However, step 3 will
 * be executed anyway to leave the bus in clean idle state.
 */

static Bool
I2CWriteRead(I2CDevPtr d,
             I2CByte * WriteBuffer, int nWrite, I2CByte * ReadBuffer, int nRead)
{
    Bool r = TRUE;
    I2CBusPtr b = d->pI2CBus;
    int s = 0;

    if (r && nWrite > 0) {
        r = b->I2CAddress(d, d->SlaveAddr & ~1);
        if (r) {
            for (; nWrite > 0; WriteBuffer++, nWrite--)
                if (!(r = b->I2CPutByte(d, *WriteBuffer)))
                    break;
            s++;
        }
    }

    if (r && nRead > 0) {
        r = b->I2CAddress(d, d->SlaveAddr | 1);
        if (r) {
            for (; nRead > 0; ReadBuffer++, nRead--)
                if (!(r = b->I2CGetByte(d, ReadBuffer, nRead == 1)))
                    break;
            s++;
        }
    }

    if (s)
        b->I2CStop(d);

    return r;
}

/* wrapper - for compatibility and convenience */

Bool
xf86I2CWriteRead(I2CDevPtr d,
                 I2CByte * WriteBuffer, int nWrite,
                 I2CByte * ReadBuffer, int nRead)
{
    I2CBusPtr b = d->pI2CBus;

    return b->I2CWriteRead(d, WriteBuffer, nWrite, ReadBuffer, nRead);
}

/* Read a byte, the only readable register of a device.
 */

Bool
xf86I2CReadStatus(I2CDevPtr d, I2CByte * pbyte)
{
    return xf86I2CWriteRead(d, NULL, 0, pbyte, 1);
}

/* Read a byte from one of the registers determined by its sub-address.
 */

Bool
xf86I2CReadByte(I2CDevPtr d, I2CByte subaddr, I2CByte * pbyte)
{
    return xf86I2CWriteRead(d, &subaddr, 1, pbyte, 1);
}

/* Read bytes from subsequent registers determined by the
 * sub-address of the first register.
 */

Bool
xf86I2CReadBytes(I2CDevPtr d, I2CByte subaddr, I2CByte * pbyte, int n)
{
    return xf86I2CWriteRead(d, &subaddr, 1, pbyte, n);
}

/* Read a word (high byte, then low byte) from one of the registers
 * determined by its sub-address.
 */

Bool
xf86I2CReadWord(I2CDevPtr d, I2CByte subaddr, unsigned short *pword)
{
    I2CByte rb[2];

    if (!xf86I2CWriteRead(d, &subaddr, 1, rb, 2))
        return FALSE;

    *pword = (rb[0] << 8) | rb[1];

    return TRUE;
}

/* Write a byte to one of the registers determined by its sub-address.
 */

Bool
xf86I2CWriteByte(I2CDevPtr d, I2CByte subaddr, I2CByte byte)
{
    I2CByte wb[2];

    wb[0] = subaddr;
    wb[1] = byte;

    return xf86I2CWriteRead(d, wb, 2, NULL, 0);
}

/* Write bytes to subsequent registers determined by the
 * sub-address of the first register.
 */

Bool
xf86I2CWriteBytes(I2CDevPtr d, I2CByte subaddr,
                  I2CByte * WriteBuffer, int nWrite)
{
    I2CBusPtr b = d->pI2CBus;
    Bool r = TRUE;

    if (nWrite > 0) {
        r = b->I2CAddress(d, d->SlaveAddr & ~1);
        if (r) {
            if ((r = b->I2CPutByte(d, subaddr)))
                for (; nWrite > 0; WriteBuffer++, nWrite--)
                    if (!(r = b->I2CPutByte(d, *WriteBuffer)))
                        break;

            b->I2CStop(d);
        }
    }

    return r;
}

/* Write a word (high byte, then low byte) to one of the registers
 * determined by its sub-address.
 */

Bool
xf86I2CWriteWord(I2CDevPtr d, I2CByte subaddr, unsigned short word)
{
    I2CByte wb[3];

    wb[0] = subaddr;
    wb[1] = word >> 8;
    wb[2] = word & 0xFF;

    return xf86I2CWriteRead(d, wb, 3, NULL, 0);
}

/* Write a vector of bytes to not adjacent registers. This vector is,
 * 1st byte sub-address, 2nd byte value, 3rd byte sub-address asf.
 * This function is intended to initialize devices. Note this function
 * exits immediately when an error occurs, some registers may
 * remain uninitialized.
 */

Bool
xf86I2CWriteVec(I2CDevPtr d, I2CByte * vec, int nValues)
{
    I2CBusPtr b = d->pI2CBus;
    Bool r = TRUE;
    int s = 0;

    if (nValues > 0) {
        for (; nValues > 0; nValues--, vec += 2) {
            if (!(r = b->I2CAddress(d, d->SlaveAddr & ~1)))
                break;

            s++;

            if (!(r = b->I2CPutByte(d, vec[0])))
                break;

            if (!(r = b->I2CPutByte(d, vec[1])))
                break;
        }

        if (s > 0)
            b->I2CStop(d);
    }

    return r;
}

/* Administrative functions.
 * =========================
 */

/* Allocates an I2CDevRec for you and initializes with proper defaults
 * you may modify before calling xf86I2CDevInit. Your I2CDevRec must
 * contain at least a SlaveAddr, and a pI2CBus pointer to the bus this
 * device shall be linked to.
 *
 * See function I2CAddress for the slave address format. Always set
 * the least significant bit, indicating a read or write access, to zero.
 */

I2CDevPtr
xf86CreateI2CDevRec(void)
{
    return calloc(1, sizeof(I2CDevRec));
}

/* Unlink an I2C device. If you got the I2CDevRec from xf86CreateI2CDevRec
 * you should set <unalloc> to free it.
 */

void
xf86DestroyI2CDevRec(I2CDevPtr d, Bool unalloc)
{
    if (d && d->pI2CBus) {
        I2CDevPtr *p;

        /* Remove this from the list of active I2C devices. */

        for (p = &d->pI2CBus->FirstDev; *p != NULL; p = &(*p)->NextDev)
            if (*p == d) {
                *p = (*p)->NextDev;
                break;
            }

        xf86DrvMsg(d->pI2CBus->scrnIndex, X_INFO,
                   "I2C device \"%s:%s\" removed.\n",
                   d->pI2CBus->BusName, d->DevName);
    }

    if (unalloc)
        free(d);
}

/* I2C transmissions are related to an I2CDevRec you must link to a
 * previously registered bus (see xf86I2CBusInit) before attempting
 * to read and write data. You may call xf86I2CProbeAddress first to
 * see if the device in question is present on this bus.
 *
 * xf86I2CDevInit will not allocate an I2CBusRec for you, instead you
 * may enter a pointer to a statically allocated I2CDevRec or the (modified)
 * result of xf86CreateI2CDevRec.
 *
 * If you don't specify timeouts for the device (n <= 0), it will inherit
 * the bus-wide defaults. The function returns TRUE on success.
 */

Bool
xf86I2CDevInit(I2CDevPtr d)
{
    I2CBusPtr b;

    if (d == NULL ||
        (b = d->pI2CBus) == NULL ||
        (d->SlaveAddr & 1) || xf86I2CFindDev(b, d->SlaveAddr) != NULL)
        return FALSE;

    if (d->BitTimeout <= 0)
        d->BitTimeout = b->BitTimeout;
    if (d->ByteTimeout <= 0)
        d->ByteTimeout = b->ByteTimeout;
    if (d->AcknTimeout <= 0)
        d->AcknTimeout = b->AcknTimeout;
    if (d->StartTimeout <= 0)
        d->StartTimeout = b->StartTimeout;

    d->NextDev = b->FirstDev;
    b->FirstDev = d;

    xf86DrvMsg(b->scrnIndex, X_INFO,
               "I2C device \"%s:%s\" registered at address 0x%02X.\n",
               b->BusName, d->DevName, d->SlaveAddr);

    return TRUE;
}

I2CDevPtr
xf86I2CFindDev(I2CBusPtr b, I2CSlaveAddr addr)
{
    I2CDevPtr d;

    if (b) {
        for (d = b->FirstDev; d != NULL; d = d->NextDev)
            if (d->SlaveAddr == addr)
                return d;
    }

    return NULL;
}

static I2CBusPtr I2CBusList;

/* Allocates an I2CBusRec for you and initializes with proper defaults
 * you may modify before calling xf86I2CBusInit. Your I2CBusRec must
 * contain at least a BusName, a scrnIndex (or -1), and a complete set
 * of either high or low level I2C function pointers. You may pass
 * bus-wide timeouts, otherwise inplausible values will be replaced
 * with safe defaults.
 */

I2CBusPtr
xf86CreateI2CBusRec(void)
{
    I2CBusPtr b;

    b = (I2CBusPtr) calloc(1, sizeof(I2CBusRec));

    if (b != NULL) {
        b->scrnIndex = -1;
        b->pScrn = NULL;
        b->HoldTime = 5;        /* 100 kHz bus */
        b->BitTimeout = 5;
        b->ByteTimeout = 5;
        b->AcknTimeout = 5;
        b->StartTimeout = 5;
        b->RiseFallTime = RISEFALLTIME;
    }

    return b;
}

/* Unregister an I2C bus. If you got the I2CBusRec from xf86CreateI2CBusRec
 * you should set <unalloc> to free it. If you set <devs_too>, the function
 * xf86DestroyI2CDevRec will be called for all devices linked to the bus
 * first, passing down the <unalloc> option.
 */

void
xf86DestroyI2CBusRec(I2CBusPtr b, Bool unalloc, Bool devs_too)
{
    if (b) {
        I2CBusPtr *p;

        /* Remove this from the list of active I2C buses */

        for (p = &I2CBusList; *p != NULL; p = &(*p)->NextBus)
            if (*p == b) {
                *p = (*p)->NextBus;
                break;
            }

        if (b->FirstDev != NULL) {
            if (devs_too) {
                I2CDevPtr d;

                while ((d = b->FirstDev) != NULL) {
                    b->FirstDev = d->NextDev;
                    xf86DestroyI2CDevRec(d, unalloc);
                }
            }
            else {
                if (unalloc) {
                    xf86Msg(X_ERROR,
                            "i2c bug: Attempt to remove I2C bus \"%s\", "
                            "but device list is not empty.\n", b->BusName);
                    return;
                }
            }
        }

        xf86DrvMsg(b->scrnIndex, X_INFO, "I2C bus \"%s\" removed.\n",
                   b->BusName);

        if (unalloc)
            free(b);
    }
}

/* I2C masters have to register themselves using this function.
 * It will not allocate an I2CBusRec for you, instead you may enter
 * a pointer to a statically allocated I2CBusRec or the (modified)
 * result of xf86CreateI2CBusRec. Returns TRUE on success.
 *
 * At this point there won't be any traffic on the I2C bus.
 */

Bool
xf86I2CBusInit(I2CBusPtr b)
{
    /* I2C buses must be identified by a unique scrnIndex
     * and name. If scrnIndex is unspecified (a negative value),
     * then the name must be unique throughout the server.
     */

    if (b->BusName == NULL || xf86I2CFindBus(b->scrnIndex, b->BusName) != NULL)
        return FALSE;

    /* If the high level functions are not
     * supplied, use the generic functions.
     * In this case we need the low-level
     * function.
     */
    if (b->I2CWriteRead == NULL) {
        b->I2CWriteRead = I2CWriteRead;

        if (b->I2CPutBits == NULL || b->I2CGetBits == NULL) {
            if (b->I2CPutByte == NULL ||
                b->I2CGetByte == NULL ||
                b->I2CAddress == NULL ||
                b->I2CStart == NULL || b->I2CStop == NULL)
                return FALSE;
        }
        else {
            b->I2CPutByte = I2CPutByte;
            b->I2CGetByte = I2CGetByte;
            b->I2CAddress = I2CAddress;
            b->I2CStop = I2CStop;
            b->I2CStart = I2CStart;
        }
    }

    if (b->I2CUDelay == NULL)
        b->I2CUDelay = I2CUDelay;

    if (b->HoldTime < 2)
        b->HoldTime = 5;
    if (b->BitTimeout <= 0)
        b->BitTimeout = b->HoldTime;
    if (b->ByteTimeout <= 0)
        b->ByteTimeout = b->HoldTime;
    if (b->AcknTimeout <= 0)
        b->AcknTimeout = b->HoldTime;
    if (b->StartTimeout <= 0)
        b->StartTimeout = b->HoldTime;

    /* Put new bus on list. */

    b->NextBus = I2CBusList;
    I2CBusList = b;

    xf86DrvMsg(b->scrnIndex, X_INFO, "I2C bus \"%s\" initialized.\n",
               b->BusName);

    return TRUE;
}

I2CBusPtr
xf86I2CFindBus(int scrnIndex, char *name)
{
    I2CBusPtr p;

    if (name != NULL)
        for (p = I2CBusList; p != NULL; p = p->NextBus)
            if (scrnIndex < 0 || p->scrnIndex == scrnIndex)
                if (!strcmp(p->BusName, name))
                    return p;

    return NULL;
}

/*
 * Return an array of I2CBusPtr's related to a screen.  The caller is
 * responsible for freeing the array.
 */
int
xf86I2CGetScreenBuses(int scrnIndex, I2CBusPtr ** pppI2CBus)
{
    I2CBusPtr pI2CBus;
    int n = 0;

    if (pppI2CBus)
        *pppI2CBus = NULL;

    for (pI2CBus = I2CBusList; pI2CBus; pI2CBus = pI2CBus->NextBus) {
        if ((pI2CBus->scrnIndex >= 0) && (pI2CBus->scrnIndex != scrnIndex))
            continue;

        n++;

        if (!pppI2CBus)
            continue;

        *pppI2CBus = xnfreallocarray(*pppI2CBus, n, sizeof(I2CBusPtr));
        (*pppI2CBus)[n - 1] = pI2CBus;
    }

    return n;
}
