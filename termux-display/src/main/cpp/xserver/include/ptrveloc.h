/*
 *
 * Copyright © 2006-2011 Simon Thum             simon dot thum at gmx dot de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef POINTERVELOCITY_H
#define POINTERVELOCITY_H

#include <input.h>

/* constants for acceleration profiles */

#define AccelProfileNone -1
#define AccelProfileClassic  0
#define AccelProfileDeviceSpecific 1
#define AccelProfilePolynomial 2
#define AccelProfileSmoothLinear 3
#define AccelProfileSimple 4
#define AccelProfilePower 5
#define AccelProfileLinear 6
#define AccelProfileSmoothLimited 7
#define AccelProfileLAST AccelProfileSmoothLimited

/* fwd */
struct _DeviceVelocityRec;

/**
 * profile
 * returns actual acceleration depending on velocity, acceleration control,...
 */
typedef double (*PointerAccelerationProfileFunc)
 (DeviceIntPtr dev, struct _DeviceVelocityRec * vel,
  double velocity, double threshold, double accelCoeff);

/**
 * a motion history, with just enough information to
 * calc mean velocity and decide which motion was along
 * a more or less straight line
 */
typedef struct _MotionTracker {
    double dx, dy;              /* accumulated delta for each axis */
    int time;                   /* time of creation */
    int dir;                    /* initial direction bitfield */
} MotionTracker, *MotionTrackerPtr;

/**
 * Contains all data needed to implement mouse ballistics
 */
typedef struct _DeviceVelocityRec {
    MotionTrackerPtr tracker;
    int num_tracker;
    int cur_tracker;            /* current index */
    double velocity;            /* velocity as guessed by algorithm */
    double last_velocity;       /* previous velocity estimate */
    double last_dx;             /* last time-difference */
    double last_dy;             /* phase of last/current estimate */
    double corr_mul;            /* config: multiply this into velocity */
    double const_acceleration;  /* config: (recipr.) const deceleration */
    double min_acceleration;    /* config: minimum acceleration */
    short reset_time;           /* config: reset non-visible state after # ms */
    short use_softening;        /* config: use softening of mouse values */
    double max_rel_diff;        /* config: max. relative difference */
    double max_diff;            /* config: max. difference */
    int initial_range;          /* config: max. offset used as initial velocity */
    Bool average_accel;         /* config: average acceleration over velocity */
    PointerAccelerationProfileFunc Profile;
    PointerAccelerationProfileFunc deviceSpecificProfile;
    void *profile_private;      /* extended data, see  SetAccelerationProfile() */
    struct {                    /* to be able to query this information */
        int profile_number;
    } statistics;
} DeviceVelocityRec, *DeviceVelocityPtr;

/**
 * contains the run-time data for the predictable scheme, that is, a
 * DeviceVelocityPtr and the property handlers.
 */
typedef struct _PredictableAccelSchemeRec {
    DeviceVelocityPtr vel;
    long *prop_handlers;
    int num_prop_handlers;
} PredictableAccelSchemeRec, *PredictableAccelSchemePtr;

extern _X_EXPORT void
InitVelocityData(DeviceVelocityPtr vel);

extern _X_EXPORT void
InitTrackers(DeviceVelocityPtr vel, int ntracker);

extern _X_EXPORT BOOL
ProcessVelocityData2D(DeviceVelocityPtr vel, double dx, double dy, int time);

extern _X_EXPORT double
BasicComputeAcceleration(DeviceIntPtr dev, DeviceVelocityPtr vel,
                         double velocity, double threshold, double acc);

extern _X_EXPORT void
FreeVelocityData(DeviceVelocityPtr vel);

extern _X_EXPORT int
SetAccelerationProfile(DeviceVelocityPtr vel, int profile_num);

extern _X_EXPORT DeviceVelocityPtr
GetDevicePredictableAccelData(DeviceIntPtr dev);

extern _X_EXPORT void
SetDeviceSpecificAccelerationProfile(DeviceVelocityPtr vel,
                                     PointerAccelerationProfileFunc profile);

extern _X_INTERNAL void
AccelerationDefaultCleanup(DeviceIntPtr dev);

extern _X_INTERNAL Bool
InitPredictableAccelerationScheme(DeviceIntPtr dev,
                                  struct _ValuatorAccelerationRec *protoScheme);

extern _X_INTERNAL void
acceleratePointerPredictable(DeviceIntPtr dev, ValuatorMask *val,
                             CARD32 evtime);

extern _X_INTERNAL void
acceleratePointerLightweight(DeviceIntPtr dev, ValuatorMask *val,
                             CARD32 evtime);

#endif                          /* POINTERVELOCITY_H */
