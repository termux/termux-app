/*
 * Copyright 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * them Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTIBILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Properties managed by the server. */

#ifndef _XSERVER_PROPERTIES_H_
#define _XSERVER_PROPERTIES_H_

/* Type for a 4 byte float. Storage format IEEE 754 in client's default
 * byte-ordering. */
#define XATOM_FLOAT "FLOAT"

/* STRING. Seat name of this display */
#define SEAT_ATOM_NAME "Xorg_Seat"

/* BOOL. 0 - device disabled, 1 - device enabled */
#define XI_PROP_ENABLED      "Device Enabled"
/* BOOL. If present, device is a virtual XTEST device */
#define XI_PROP_XTEST_DEVICE  "XTEST Device"

/* CARD32, 2 values, vendor, product.
 * This property is set by the driver and may not be available for some
 * drivers. Read-Only */
#define XI_PROP_PRODUCT_ID "Device Product ID"

/* Coordinate transformation matrix for absolute input devices
 * FLOAT, 9 values in row-major order, coordinates in 0..1 range:
 * [c0 c1 c2]   [x]
 * [c3 c4 c5] * [y]
 * [c6 c7 c8]   [1] */
#define XI_PROP_TRANSFORM "Coordinate Transformation Matrix"

/* STRING. Device node path of device */
#define XI_PROP_DEVICE_NODE "Device Node"

/* Pointer acceleration properties */
/* INTEGER of any format */
#define ACCEL_PROP_PROFILE_NUMBER "Device Accel Profile"
/* FLOAT, format 32 */
#define ACCEL_PROP_CONSTANT_DECELERATION "Device Accel Constant Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_ADAPTIVE_DECELERATION "Device Accel Adaptive Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_VELOCITY_SCALING "Device Accel Velocity Scaling"

/* Axis labels */
#define AXIS_LABEL_PROP "Axis Labels"

#define AXIS_LABEL_PROP_REL_X           "Rel X"
#define AXIS_LABEL_PROP_REL_Y           "Rel Y"
#define AXIS_LABEL_PROP_REL_Z           "Rel Z"
#define AXIS_LABEL_PROP_REL_RX          "Rel Rotary X"
#define AXIS_LABEL_PROP_REL_RY          "Rel Rotary Y"
#define AXIS_LABEL_PROP_REL_RZ          "Rel Rotary Z"
#define AXIS_LABEL_PROP_REL_HWHEEL      "Rel Horiz Wheel"
#define AXIS_LABEL_PROP_REL_DIAL        "Rel Dial"
#define AXIS_LABEL_PROP_REL_WHEEL       "Rel Vert Wheel"
#define AXIS_LABEL_PROP_REL_MISC        "Rel Misc"
#define AXIS_LABEL_PROP_REL_VSCROLL     "Rel Vert Scroll"
#define AXIS_LABEL_PROP_REL_HSCROLL     "Rel Horiz Scroll"

/*
 * Absolute axes
 */

#define AXIS_LABEL_PROP_ABS_X           "Abs X"
#define AXIS_LABEL_PROP_ABS_Y           "Abs Y"
#define AXIS_LABEL_PROP_ABS_Z           "Abs Z"
#define AXIS_LABEL_PROP_ABS_RX          "Abs Rotary X"
#define AXIS_LABEL_PROP_ABS_RY          "Abs Rotary Y"
#define AXIS_LABEL_PROP_ABS_RZ          "Abs Rotary Z"
#define AXIS_LABEL_PROP_ABS_THROTTLE    "Abs Throttle"
#define AXIS_LABEL_PROP_ABS_RUDDER      "Abs Rudder"
#define AXIS_LABEL_PROP_ABS_WHEEL       "Abs Wheel"
#define AXIS_LABEL_PROP_ABS_GAS         "Abs Gas"
#define AXIS_LABEL_PROP_ABS_BRAKE       "Abs Brake"
#define AXIS_LABEL_PROP_ABS_HAT0X       "Abs Hat 0 X"
#define AXIS_LABEL_PROP_ABS_HAT0Y       "Abs Hat 0 Y"
#define AXIS_LABEL_PROP_ABS_HAT1X       "Abs Hat 1 X"
#define AXIS_LABEL_PROP_ABS_HAT1Y       "Abs Hat 1 Y"
#define AXIS_LABEL_PROP_ABS_HAT2X       "Abs Hat 2 X"
#define AXIS_LABEL_PROP_ABS_HAT2Y       "Abs Hat 2 Y"
#define AXIS_LABEL_PROP_ABS_HAT3X       "Abs Hat 3 X"
#define AXIS_LABEL_PROP_ABS_HAT3Y       "Abs Hat 3 Y"
#define AXIS_LABEL_PROP_ABS_PRESSURE    "Abs Pressure"
#define AXIS_LABEL_PROP_ABS_DISTANCE    "Abs Distance"
#define AXIS_LABEL_PROP_ABS_TILT_X      "Abs Tilt X"
#define AXIS_LABEL_PROP_ABS_TILT_Y      "Abs Tilt Y"
#define AXIS_LABEL_PROP_ABS_TOOL_WIDTH  "Abs Tool Width"
#define AXIS_LABEL_PROP_ABS_VOLUME      "Abs Volume"
#define AXIS_LABEL_PROP_ABS_MT_TOUCH_MAJOR "Abs MT Touch Major"
#define AXIS_LABEL_PROP_ABS_MT_TOUCH_MINOR "Abs MT Touch Minor"
#define AXIS_LABEL_PROP_ABS_MT_WIDTH_MAJOR "Abs MT Width Major"
#define AXIS_LABEL_PROP_ABS_MT_WIDTH_MINOR "Abs MT Width Minor"
#define AXIS_LABEL_PROP_ABS_MT_ORIENTATION "Abs MT Orientation"
#define AXIS_LABEL_PROP_ABS_MT_POSITION_X  "Abs MT Position X"
#define AXIS_LABEL_PROP_ABS_MT_POSITION_Y  "Abs MT Position Y"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_TYPE   "Abs MT Tool Type"
#define AXIS_LABEL_PROP_ABS_MT_BLOB_ID     "Abs MT Blob ID"
#define AXIS_LABEL_PROP_ABS_MT_TRACKING_ID "Abs MT Tracking ID"
#define AXIS_LABEL_PROP_ABS_MT_PRESSURE    "Abs MT Pressure"
#define AXIS_LABEL_PROP_ABS_MT_DISTANCE    "Abs MT Distance"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_X      "Abs MT Tool X"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_Y      "Abs MT Tool Y"
#define AXIS_LABEL_PROP_ABS_MISC        "Abs Misc"

/* Button names */
#define BTN_LABEL_PROP "Button Labels"

/* Default label */
#define BTN_LABEL_PROP_BTN_UNKNOWN      "Button Unknown"
/* Wheel buttons */
#define BTN_LABEL_PROP_BTN_WHEEL_UP     "Button Wheel Up"
#define BTN_LABEL_PROP_BTN_WHEEL_DOWN   "Button Wheel Down"
#define BTN_LABEL_PROP_BTN_HWHEEL_LEFT  "Button Horiz Wheel Left"
#define BTN_LABEL_PROP_BTN_HWHEEL_RIGHT "Button Horiz Wheel Right"

/* The following are from linux/input.h */
#define BTN_LABEL_PROP_BTN_0            "Button 0"
#define BTN_LABEL_PROP_BTN_1            "Button 1"
#define BTN_LABEL_PROP_BTN_2            "Button 2"
#define BTN_LABEL_PROP_BTN_3            "Button 3"
#define BTN_LABEL_PROP_BTN_4            "Button 4"
#define BTN_LABEL_PROP_BTN_5            "Button 5"
#define BTN_LABEL_PROP_BTN_6            "Button 6"
#define BTN_LABEL_PROP_BTN_7            "Button 7"
#define BTN_LABEL_PROP_BTN_8            "Button 8"
#define BTN_LABEL_PROP_BTN_9            "Button 9"

#define BTN_LABEL_PROP_BTN_LEFT         "Button Left"
#define BTN_LABEL_PROP_BTN_RIGHT        "Button Right"
#define BTN_LABEL_PROP_BTN_MIDDLE       "Button Middle"
#define BTN_LABEL_PROP_BTN_SIDE         "Button Side"
#define BTN_LABEL_PROP_BTN_EXTRA        "Button Extra"
#define BTN_LABEL_PROP_BTN_FORWARD      "Button Forward"
#define BTN_LABEL_PROP_BTN_BACK         "Button Back"
#define BTN_LABEL_PROP_BTN_TASK         "Button Task"

#define BTN_LABEL_PROP_BTN_TRIGGER      "Button Trigger"
#define BTN_LABEL_PROP_BTN_THUMB        "Button Thumb"
#define BTN_LABEL_PROP_BTN_THUMB2       "Button Thumb2"
#define BTN_LABEL_PROP_BTN_TOP          "Button Top"
#define BTN_LABEL_PROP_BTN_TOP2         "Button Top2"
#define BTN_LABEL_PROP_BTN_PINKIE       "Button Pinkie"
#define BTN_LABEL_PROP_BTN_BASE         "Button Base"
#define BTN_LABEL_PROP_BTN_BASE2        "Button Base2"
#define BTN_LABEL_PROP_BTN_BASE3        "Button Base3"
#define BTN_LABEL_PROP_BTN_BASE4        "Button Base4"
#define BTN_LABEL_PROP_BTN_BASE5        "Button Base5"
#define BTN_LABEL_PROP_BTN_BASE6        "Button Base6"
#define BTN_LABEL_PROP_BTN_DEAD         "Button Dead"

#define BTN_LABEL_PROP_BTN_A            "Button A"
#define BTN_LABEL_PROP_BTN_B            "Button B"
#define BTN_LABEL_PROP_BTN_C            "Button C"
#define BTN_LABEL_PROP_BTN_X            "Button X"
#define BTN_LABEL_PROP_BTN_Y            "Button Y"
#define BTN_LABEL_PROP_BTN_Z            "Button Z"
#define BTN_LABEL_PROP_BTN_TL           "Button T Left"
#define BTN_LABEL_PROP_BTN_TR           "Button T Right"
#define BTN_LABEL_PROP_BTN_TL2          "Button T Left2"
#define BTN_LABEL_PROP_BTN_TR2          "Button T Right2"
#define BTN_LABEL_PROP_BTN_SELECT       "Button Select"
#define BTN_LABEL_PROP_BTN_START        "Button Start"
#define BTN_LABEL_PROP_BTN_MODE         "Button Mode"
#define BTN_LABEL_PROP_BTN_THUMBL       "Button Thumb Left"
#define BTN_LABEL_PROP_BTN_THUMBR       "Button Thumb Right"

#define BTN_LABEL_PROP_BTN_TOOL_PEN             "Button Tool Pen"
#define BTN_LABEL_PROP_BTN_TOOL_RUBBER          "Button Tool Rubber"
#define BTN_LABEL_PROP_BTN_TOOL_BRUSH           "Button Tool Brush"
#define BTN_LABEL_PROP_BTN_TOOL_PENCIL          "Button Tool Pencil"
#define BTN_LABEL_PROP_BTN_TOOL_AIRBRUSH        "Button Tool Airbrush"
#define BTN_LABEL_PROP_BTN_TOOL_FINGER          "Button Tool Finger"
#define BTN_LABEL_PROP_BTN_TOOL_MOUSE           "Button Tool Mouse"
#define BTN_LABEL_PROP_BTN_TOOL_LENS            "Button Tool Lens"
#define BTN_LABEL_PROP_BTN_TOUCH                "Button Touch"
#define BTN_LABEL_PROP_BTN_STYLUS               "Button Stylus"
#define BTN_LABEL_PROP_BTN_STYLUS2              "Button Stylus2"
#define BTN_LABEL_PROP_BTN_TOOL_DOUBLETAP       "Button Tool Doubletap"
#define BTN_LABEL_PROP_BTN_TOOL_TRIPLETAP       "Button Tool Tripletap"

#define BTN_LABEL_PROP_BTN_GEAR_DOWN            "Button Gear down"
#define BTN_LABEL_PROP_BTN_GEAR_UP              "Button Gear up"

#endif
