#include "math.h"

#include <math.h>
#include <string.h>

double FromXrTime(const XrTime time) {
    return (time * 1e-9);
}

XrTime ToXrTime(const double time_in_seconds) {
    return (XrTime)(time_in_seconds * 1e9);
}

float ToDegrees(float rad) {
    return (float)(rad / M_PI * 180.0f);
}

float ToRadians(float deg) {
    return (float)(deg * M_PI / 180.0f);
}

/*
================================================================================

XrQuaternionf

================================================================================
*/

XrQuaternionf XrQuaternionfCreateFromVectorAngle(const XrVector3f axis, const float angle) {
    XrQuaternionf r;
    if (XrVector3fLengthSquared(axis) == 0.0f) {
        r.x = 0;
        r.y = 0;
        r.z = 0;
        r.w = 1;
        return r;
    }

    XrVector3f unitAxis = XrVector3fNormalized(axis);
    float sinHalfAngle = sinf(angle * 0.5f);

    r.w = cosf(angle * 0.5f);
    r.x = unitAxis.x * sinHalfAngle;
    r.y = unitAxis.y * sinHalfAngle;
    r.z = unitAxis.z * sinHalfAngle;
    return r;
}

XrQuaternionf XrQuaternionfMultiply(const XrQuaternionf a, const XrQuaternionf b) {
    XrQuaternionf c;
    c.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    c.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    c.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    c.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return c;
}

XrVector3f XrQuaternionfEulerAngles(const XrQuaternionf q) {
    float M[16];
    XrQuaternionfToMatrix4f(&q, M);

    XrVector4f v1 = {0, 0, -1, 0};
    XrVector4f v2 = {1, 0, 0, 0};
    XrVector4f v3 = {0, 1, 0, 0};

    XrVector4f forwardInVRSpace = XrVector4fMultiplyMatrix4f(M, &v1);
    XrVector4f rightInVRSpace = XrVector4fMultiplyMatrix4f(M, &v2);
    XrVector4f upInVRSpace = XrVector4fMultiplyMatrix4f(M, &v3);

    XrVector3f forward = {-forwardInVRSpace.z, -forwardInVRSpace.x, forwardInVRSpace.y};
    XrVector3f right = {-rightInVRSpace.z, -rightInVRSpace.x, rightInVRSpace.y};
    XrVector3f up = {-upInVRSpace.z, -upInVRSpace.x, upInVRSpace.y};

    XrVector3f forwardNormal = XrVector3fNormalized(forward);
    XrVector3f rightNormal = XrVector3fNormalized(right);
    XrVector3f upNormal = XrVector3fNormalized(up);

    return XrVector3fGetAnglesFromVectors(forwardNormal, rightNormal, upNormal);
}

void XrQuaternionfToMatrix4f(const XrQuaternionf* q, float* m) {
    const float ww = q->w * q->w;
    const float xx = q->x * q->x;
    const float yy = q->y * q->y;
    const float zz = q->z * q->z;

    float M[4][4];
    M[0][0] = ww + xx - yy - zz;
    M[0][1] = 2 * (q->x * q->y - q->w * q->z);
    M[0][2] = 2 * (q->x * q->z + q->w * q->y);
    M[0][3] = 0;

    M[1][0] = 2 * (q->x * q->y + q->w * q->z);
    M[1][1] = ww - xx + yy - zz;
    M[1][2] = 2 * (q->y * q->z - q->w * q->x);
    M[1][3] = 0;

    M[2][0] = 2 * (q->x * q->z - q->w * q->y);
    M[2][1] = 2 * (q->y * q->z + q->w * q->x);
    M[2][2] = ww - xx - yy + zz;
    M[2][3] = 0;

    M[3][0] = 0;
    M[3][1] = 0;
    M[3][2] = 0;
    M[3][3] = 1;

    memcpy(m, &M, sizeof(float) * 16);
}

/*
================================================================================

XrVector3f, XrVector4f

================================================================================
*/


float XrVector3fDistance(const XrVector3f a, const XrVector3f b) {
    XrVector3f diff;
    diff.x = a.x - b.x;
    diff.y = a.y - b.y;
    diff.z = a.z - b.z;
    return sqrt(XrVector3fLengthSquared(diff));
}

float XrVector3fLengthSquared(const XrVector3f v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

XrVector3f XrVector3fGetAnglesFromVectors(XrVector3f forward, XrVector3f right, XrVector3f up) {
    float sp = -forward.z;

    float cp_x_cy = forward.x;
    float cp_x_sy = forward.y;
    float cp_x_sr = -right.z;
    float cp_x_cr = up.z;

    float yaw = atan2(cp_x_sy, cp_x_cy);
    float roll = atan2(cp_x_sr, cp_x_cr);

    float cy = cos(yaw);
    float sy = sin(yaw);
    float cr = cos(roll);
    float sr = sin(roll);

    float cp;
    if (fabs(cy) > EPSILON) {
        cp = cp_x_cy / cy;
    } else if (fabs(sy) > EPSILON) {
        cp = cp_x_sy / sy;
    } else if (fabs(sr) > EPSILON) {
        cp = cp_x_sr / sr;
    } else if (fabs(cr) > EPSILON) {
        cp = cp_x_cr / cr;
    } else {
        cp = cos(asin(sp));
    }

    float pitch = atan2(sp, cp);

    XrVector3f angles;
    angles.x = ToDegrees(pitch);
    angles.y = ToDegrees(yaw);
    angles.z = ToDegrees(roll);
    return angles;
}

XrVector3f XrVector3fNormalized(const XrVector3f v) {
    float rcpLen = 1.0f / sqrtf(XrVector3fLengthSquared(v));
    return XrVector3fScalarMultiply(v, rcpLen);
}

XrVector3f XrVector3fScalarMultiply(const XrVector3f v, float scale) {
    XrVector3f u;
    u.x = v.x * scale;
    u.y = v.y * scale;
    u.z = v.z * scale;
    return u;
}

XrVector4f XrVector4fMultiplyMatrix4f(const float* m, const XrVector4f* v) {
    float M[4][4];
    memcpy(&M, m, sizeof(float) * 16);

    XrVector4f out;
    out.x = M[0][0] * v->x + M[0][1] * v->y + M[0][2] * v->z + M[0][3] * v->w;
    out.y = M[1][0] * v->x + M[1][1] * v->y + M[1][2] * v->z + M[1][3] * v->w;
    out.z = M[2][0] * v->x + M[2][1] * v->y + M[2][2] * v->z + M[2][3] * v->w;
    out.w = M[3][0] * v->x + M[3][1] * v->y + M[3][2] * v->z + M[3][3] * v->w;
    return out;
}
