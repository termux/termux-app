#pragma once

#include <openxr/openxr.h>

#ifndef EPSILON
#define EPSILON 0.001f
#endif

double FromXrTime(const XrTime time);
XrTime ToXrTime(const double time_in_seconds);
float ToDegrees(float rad);
float ToRadians(float deg);

// XrQuaternionf
XrQuaternionf XrQuaternionfCreateFromVectorAngle(const XrVector3f axis, const float angle);
XrQuaternionf XrQuaternionfMultiply(const XrQuaternionf a, const XrQuaternionf b);
XrVector3f XrQuaternionfEulerAngles(const XrQuaternionf q);
void XrQuaternionfToMatrix4f(const XrQuaternionf* q, float* m);

// XrVector3f, XrVector4f
float XrVector3fDistance(const XrVector3f a, const XrVector3f b);
float XrVector3fLengthSquared(const XrVector3f v);
XrVector3f XrVector3fGetAnglesFromVectors(XrVector3f forward, XrVector3f right, XrVector3f up);
XrVector3f XrVector3fNormalized(const XrVector3f v);
XrVector3f XrVector3fScalarMultiply(const XrVector3f v, float scale);
XrVector4f XrVector4fMultiplyMatrix4f(const float* m, const XrVector4f* v);
