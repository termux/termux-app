/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifndef _DRI2_H_
#define _DRI2_H_

#include <X11/extensions/dri2tokens.h>

/* Version 2 structure (with format at the end) */
typedef struct {
    unsigned int attachment;
    unsigned int name;
    unsigned int pitch;
    unsigned int cpp;
    unsigned int flags;
    unsigned int format;
    void *driverPrivate;
} DRI2BufferRec, *DRI2BufferPtr;

extern CARD8 dri2_major;        /* version of DRI2 supported by DDX */
extern CARD8 dri2_minor;

typedef DRI2BufferRec DRI2Buffer2Rec, *DRI2Buffer2Ptr;
typedef void (*DRI2SwapEventPtr) (ClientPtr client, void *data, int type,
                                  CARD64 ust, CARD64 msc, CARD32 sbc);

typedef DRI2BufferPtr(*DRI2CreateBuffersProcPtr) (DrawablePtr pDraw,
                                                  unsigned int *attachments,
                                                  int count);
typedef void (*DRI2DestroyBuffersProcPtr) (DrawablePtr pDraw,
                                           DRI2BufferPtr buffers, int count);
typedef void (*DRI2CopyRegionProcPtr) (DrawablePtr pDraw,
                                       RegionPtr pRegion,
                                       DRI2BufferPtr pDestBuffer,
                                       DRI2BufferPtr pSrcBuffer);
typedef void (*DRI2WaitProcPtr) (WindowPtr pWin, unsigned int sequence);
typedef int (*DRI2AuthMagicProcPtr) (int fd, uint32_t magic);
typedef int (*DRI2AuthMagic2ProcPtr) (ScreenPtr pScreen, uint32_t magic);

/**
 * Schedule a buffer swap
 *
 * This callback is used to support glXSwapBuffers and the OML_sync_control
 * extension (see it for a description of the params).
 *
 * Drivers should queue an event for the frame count that satisfies the
 * parameters passed in.  If the event is in the future (i.e. the conditions
 * aren't currently satisfied), the server may block the client at the next
 * GLX request using DRI2WaitSwap. When the event arrives, drivers should call
 * \c DRI2SwapComplete, which will handle waking the client and returning
 * the appropriate data.
 *
 * The DDX is responsible for doing a flip, exchange, or blit of the swap
 * when the corresponding event arrives.  The \c DRI2CanFlip and
 * \c DRI2CanExchange functions can be used as helpers for this purpose.
 *
 * \param client client pointer (used for block/unblock)
 * \param pDraw drawable whose count we want
 * \param pDestBuffer current front buffer
 * \param pSrcBuffer current back buffer
 * \param target_msc frame count to wait for
 * \param divisor divisor for condition equation
 * \param remainder remainder for division equation
 * \param func function to call when the swap completes
 * \param data data for the callback \p func.
 */
typedef int (*DRI2ScheduleSwapProcPtr) (ClientPtr client,
                                        DrawablePtr pDraw,
                                        DRI2BufferPtr pDestBuffer,
                                        DRI2BufferPtr pSrcBuffer,
                                        CARD64 * target_msc,
                                        CARD64 divisor,
                                        CARD64 remainder,
                                        DRI2SwapEventPtr func, void *data);
typedef DRI2BufferPtr(*DRI2CreateBufferProcPtr) (DrawablePtr pDraw,
                                                 unsigned int attachment,
                                                 unsigned int format);
typedef void (*DRI2DestroyBufferProcPtr) (DrawablePtr pDraw,
                                          DRI2BufferPtr buffer);
/**
 * Notifies driver when DRI2GetBuffers reuses a dri2 buffer.
 *
 * Driver may rename the dri2 buffer in this notify if it is required.
 *
 * \param pDraw drawable whose count we want
 * \param buffer buffer that will be returned to client
 */
typedef void (*DRI2ReuseBufferNotifyProcPtr) (DrawablePtr pDraw,
                                              DRI2BufferPtr buffer);
/**
 * Get current media stamp counter values
 *
 * This callback is used to support the SGI_video_sync and OML_sync_control
 * extensions.
 *
 * Drivers should return the current frame counter and the timestamp from
 * when the returned frame count was last incremented.
 *
 * The count should correspond to the screen where the drawable is currently
 * visible.  If the drawable isn't visible (e.g. redirected), the server
 * should return BadDrawable to the client, pending GLX spec updates to
 * define this behavior.
 *
 * \param pDraw drawable whose count we want
 * \param ust timestamp from when the count was last incremented.
 * \param mst current frame count
 */
typedef int (*DRI2GetMSCProcPtr) (DrawablePtr pDraw, CARD64 * ust,
                                  CARD64 * msc);
/**
 * Schedule a frame count related wait
 *
 * This callback is used to support the SGI_video_sync and OML_sync_control
 * extensions.  See those specifications for details on how to handle
 * the divisor and remainder parameters.
 *
 * Drivers should queue an event for the frame count that satisfies the
 * parameters passed in.  If the event is in the future (i.e. the conditions
 * aren't currently satisfied), the driver should block the client using
 * \c DRI2BlockClient.  When the event arrives, drivers should call
 * \c DRI2WaitMSCComplete, which will handle waking the client and returning
 * the appropriate data.
 *
 * \param client client pointer (used for block/unblock)
 * \param pDraw drawable whose count we want
 * \param target_msc frame count to wait for
 * \param divisor divisor for condition equation
 * \param remainder remainder for division equation
 */
typedef int (*DRI2ScheduleWaitMSCProcPtr) (ClientPtr client,
                                           DrawablePtr pDraw,
                                           CARD64 target_msc,
                                           CARD64 divisor, CARD64 remainder);

typedef void (*DRI2InvalidateProcPtr) (DrawablePtr pDraw, void *data, XID id);

/**
 * DRI2 calls this hook when ever swap_limit is going to be changed. Default
 * implementation for the hook only accepts one as swap_limit. If driver can
 * support other swap_limits it has to implement supported limits with this
 * callback.
 *
 * \param pDraw drawable whose swap_limit is going to be changed
 * \param swap_limit new swap_limit that going to be set
 * \return TRUE if limit is support, FALSE if not.
 */
typedef Bool (*DRI2SwapLimitValidateProcPtr) (DrawablePtr pDraw,
                                              int swap_limit);

typedef DRI2BufferPtr(*DRI2CreateBuffer2ProcPtr) (ScreenPtr pScreen,
                                                  DrawablePtr pDraw,
                                                  unsigned int attachment,
                                                  unsigned int format);
typedef void (*DRI2DestroyBuffer2ProcPtr) (ScreenPtr pScreen, DrawablePtr pDraw,
                                          DRI2BufferPtr buffer);

typedef void (*DRI2CopyRegion2ProcPtr) (ScreenPtr pScreen, DrawablePtr pDraw,
                                        RegionPtr pRegion,
                                        DRI2BufferPtr pDestBuffer,
                                        DRI2BufferPtr pSrcBuffer);

/**
 * \brief Get the value of a parameter.
 *
 * The parameter's \a value is looked up on the screen associated with
 * \a pDrawable.
 *
 * \return \c Success or error code.
 */
typedef int (*DRI2GetParamProcPtr) (ClientPtr client,
                                    DrawablePtr pDrawable,
                                    CARD64 param,
                                    BOOL *is_param_recognized,
                                    CARD64 *value);

/**
 * Version of the DRI2InfoRec structure defined in this header
 */
#define DRI2INFOREC_VERSION 9

typedef struct {
    unsigned int version;       /**< Version of this struct */
    int fd;
    const char *driverName;
    const char *deviceName;

    DRI2CreateBufferProcPtr CreateBuffer;
    DRI2DestroyBufferProcPtr DestroyBuffer;
    DRI2CopyRegionProcPtr CopyRegion;
    DRI2WaitProcPtr Wait;

    /* added in version 4 */

    DRI2ScheduleSwapProcPtr ScheduleSwap;
    DRI2GetMSCProcPtr GetMSC;
    DRI2ScheduleWaitMSCProcPtr ScheduleWaitMSC;

    /* number of drivers in the driverNames array */
    unsigned int numDrivers;
    /* array of driver names, indexed by DRI2Driver* driver types */
    /* a name of NULL means that driver is not supported */
    const char *const *driverNames;

    /* added in version 5 */

    DRI2AuthMagicProcPtr AuthMagic;

    /* added in version 6 */

    DRI2ReuseBufferNotifyProcPtr ReuseBufferNotify;
    DRI2SwapLimitValidateProcPtr SwapLimitValidate;

    /* added in version 7 */
    DRI2GetParamProcPtr GetParam;

    /* added in version 8 */
    /* AuthMagic callback which passes extra context */
    /* If this is NULL the AuthMagic callback is used */
    /* If this is non-NULL the AuthMagic callback is ignored */
    DRI2AuthMagic2ProcPtr AuthMagic2;

    /* added in version 9 */
    DRI2CreateBuffer2ProcPtr CreateBuffer2;
    DRI2DestroyBuffer2ProcPtr DestroyBuffer2;
    DRI2CopyRegion2ProcPtr CopyRegion2;
} DRI2InfoRec, *DRI2InfoPtr;

extern _X_EXPORT Bool DRI2ScreenInit(ScreenPtr pScreen, DRI2InfoPtr info);

extern _X_EXPORT void DRI2CloseScreen(ScreenPtr pScreen);

extern _X_EXPORT Bool DRI2HasSwapControl(ScreenPtr pScreen);

extern _X_EXPORT Bool DRI2Connect(ClientPtr client, ScreenPtr pScreen,
                                  unsigned int driverType,
                                  int *fd,
                                  const char **driverName,
                                  const char **deviceName);

extern _X_EXPORT Bool DRI2Authenticate(ClientPtr client, ScreenPtr pScreen, uint32_t magic);

extern _X_EXPORT int DRI2CreateDrawable(ClientPtr client,
                                        DrawablePtr pDraw,
                                        XID id,
                                        DRI2InvalidateProcPtr invalidate,
                                        void *priv);

extern _X_EXPORT int DRI2CreateDrawable2(ClientPtr client,
                                         DrawablePtr pDraw,
                                         XID id,
                                         DRI2InvalidateProcPtr invalidate,
                                         void *priv,
                                         XID *dri2_id_out);

extern _X_EXPORT DRI2BufferPtr *DRI2GetBuffers(DrawablePtr pDraw,
                                               int *width,
                                               int *height,
                                               unsigned int *attachments,
                                               int count, int *out_count);

extern _X_EXPORT int DRI2CopyRegion(DrawablePtr pDraw,
                                    RegionPtr pRegion,
                                    unsigned int dest, unsigned int src);

/**
 * Determine the major and minor version of the DRI2 extension.
 *
 * Provides a mechanism to other modules (e.g., 2D drivers) to determine the
 * version of the DRI2 extension.  While it is possible to peek directly at
 * the \c XF86ModuleData from a layered module, such a module will fail to
 * load (due to an unresolved symbol) if the DRI2 extension is not loaded.
 *
 * \param major  Location to store the major version of the DRI2 extension
 * \param minor  Location to store the minor version of the DRI2 extension
 *
 * \note
 * This interface was added some time after the initial release of the DRI2
 * module.  Layered modules that wish to use this interface must first test
 * its existence by calling \c xf86LoaderCheckSymbol.
 */
extern _X_EXPORT void DRI2Version(int *major, int *minor);

extern _X_EXPORT DRI2BufferPtr *DRI2GetBuffersWithFormat(DrawablePtr pDraw,
                                                         int *width,
                                                         int *height,
                                                         unsigned int
                                                         *attachments,
                                                         int count,
                                                         int *out_count);

extern _X_EXPORT void DRI2SwapInterval(DrawablePtr pDrawable, int interval);
extern _X_EXPORT Bool DRI2SwapLimit(DrawablePtr pDraw, int swap_limit);
extern _X_EXPORT int DRI2SwapBuffers(ClientPtr client, DrawablePtr pDrawable,
                                     CARD64 target_msc, CARD64 divisor,
                                     CARD64 remainder, CARD64 * swap_target,
                                     DRI2SwapEventPtr func, void *data);
extern _X_EXPORT Bool DRI2WaitSwap(ClientPtr client, DrawablePtr pDrawable);

extern _X_EXPORT int DRI2GetMSC(DrawablePtr pDrawable, CARD64 * ust,
                                CARD64 * msc, CARD64 * sbc);
extern _X_EXPORT int DRI2WaitMSC(ClientPtr client, DrawablePtr pDrawable,
                                 CARD64 target_msc, CARD64 divisor,
                                 CARD64 remainder);
extern _X_EXPORT int ProcDRI2WaitMSCReply(ClientPtr client, CARD64 ust,
                                          CARD64 msc, CARD64 sbc);
extern _X_EXPORT int DRI2WaitSBC(ClientPtr client, DrawablePtr pDraw,
                                 CARD64 target_sbc);
extern _X_EXPORT Bool DRI2ThrottleClient(ClientPtr client, DrawablePtr pDraw);

extern _X_EXPORT Bool DRI2CanFlip(DrawablePtr pDraw);

extern _X_EXPORT Bool DRI2CanExchange(DrawablePtr pDraw);

/* Note: use *only* for MSC related waits */
extern _X_EXPORT void DRI2BlockClient(ClientPtr client, DrawablePtr pDraw);

extern _X_EXPORT void DRI2SwapComplete(ClientPtr client, DrawablePtr pDraw,
                                       int frame, unsigned int tv_sec,
                                       unsigned int tv_usec, int type,
                                       DRI2SwapEventPtr swap_complete,
                                       void *swap_data);
extern _X_EXPORT void DRI2WaitMSCComplete(ClientPtr client, DrawablePtr pDraw,
                                          int frame, unsigned int tv_sec,
                                          unsigned int tv_usec);

extern _X_EXPORT int DRI2GetParam(ClientPtr client,
                                  DrawablePtr pDrawable,
                                  CARD64 param,
                                  BOOL *is_param_recognized,
                                  CARD64 *value);

extern _X_EXPORT DrawablePtr DRI2UpdatePrime(DrawablePtr pDraw, DRI2BufferPtr pDest);
#endif
