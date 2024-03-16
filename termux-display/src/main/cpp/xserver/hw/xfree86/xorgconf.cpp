#
# Copyright (c) 1994-1998 by The XFree86 Project, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name of the XFree86 Project shall
# not be used in advertising or otherwise to promote the sale, use or other
# dealings in this Software without prior written authorization from the
# XFree86 Project.
#
# $XConsortium: XF86Conf.cpp /main/22 1996/10/23 11:43:51 kaleb $

# **********************************************************************
# This is a sample configuration file only, intended to illustrate
# what a config file might look like.  Refer to the xorg.conf(__filemansuffix__)
# man page for details about the format of this file.
# **********************************************************************

# The ordering of sections is not important in XFree86 4.0 and later,
# nor in any Xorg release.

# **********************************************************************
# Files section.  This allows default font and module paths to be set
# **********************************************************************

Section "Files"

# Multiple FontPath entries are allowed (which are concatenated together),
# as well as specifying multiple comma-separated entries in one FontPath
# command (or a combination of both methods).
# The default path is shown here.

#    FontPath	DEFAULTFONTPATH

# ModulePath can be used to set a search path for the X server modules.
# The default path is shown here.

#    ModulePath	MODULEPATH

EndSection

# **********************************************************************
# Module section -- this is an optional section which is used to specify
# which run-time loadable modules to load when the X server starts up.
# **********************************************************************

Section "Module"

# This loads the DBE extension module.

    Load	"dbe"

# This loads the miscellaneous extensions module, and disables
# initialisation of the XFree86-DGA extension within that module.

    SubSection	"extmod"
	Option	"omit xfree86-dga"
    EndSubSection

EndSection


# **********************************************************************
# Server flags section.  This contains various server-wide Options.
# **********************************************************************

Section "ServerFlags"

# Uncomment this to disable the <Ctrl><Alt><Fn> VT switch sequence
# (where n is 1 through 12).  This allows clients to receive these key
# events.

#    Option	"DontVTSwitch"

# Uncomment this to disable the <Ctrl><Alt><BS> server abort sequence
# This allows clients to receive this key event.

#    Option	"DontZap"	"true"

# Uncomment this to disable the <Ctrl><Alt><KP_+>/<KP_-> mode switching
# sequences.  This allows clients to receive these key events.

#    Option	"DontZoom"

# Uncomment this to disable tuning with the xvidtune client. With
# it the client can still run and fetch card and monitor attributes,
# but it will not be allowed to change them. If it tries it will
# receive a protocol error.

#    Option	"DisableVidModeExtension"

# Uncomment this to enable the use of a non-local xvidtune client.

#    Option	"AllowNonLocalXvidtune"

# Set the basic blanking screen saver timeout.

    Option	"BlankTime"	"10"	# 10 minutes

# Set the DPMS timeouts.  These are set here because they are global
# rather than screen-specific.  These settings alone don't enable DPMS.
# It is enabled per-screen (or per-monitor), and even then only when
# the driver supports it.

    Option	"StandbyTime"	"10"	# 10 minutes
    Option	"SuspendTime"	"10"	# 10 minutes
    Option	"OffTime"	"10"	# 10 minutes

EndSection

# **********************************************************************
# Input devices
# **********************************************************************

# **********************************************************************
# Core keyboard's InputDevice section
# **********************************************************************

Section "InputDevice"

    Identifier	"Keyboard1"
    Driver	"kbd"

# Set the keyboard auto repeat parameters.  Not all platforms implement
# this.

    Option	"AutoRepeat"	"500 5"

# Specify which keyboard LEDs can be user-controlled (eg, with xset(1)).

#    Option	"Xleds"	"1 2 3"

# To customise the XKB settings to suit your keyboard, modify the
# lines below (which are the defaults).  For example, for a European
# keyboard, you will probably want to use one of:
#
#    Option	"XkbModel"	"pc102"
#    Option	"XkbModel"	"pc105"
#
# If you have a Microsoft Natural keyboard, you can use:
#
#    Option	"XkbModel"	"microsoft"
#
# If you have a US "windows" keyboard you will want:
#
#    Option	"XkbModel"	"pc104"
#
# Then to change the language, change the Layout setting.
# For example, a german layout can be obtained with:
#
#    Option	"XkbLayout"	"de"
#
# or:
#
#    Option	"XkbLayout"	"de"
#    Option	"XkbVariant"	"nodeadkeys"
#
# If you'd like to switch the positions of your capslock and
# control keys, use:
#
#    Option	"XkbOptions"	"ctrl:swapcaps"


# These are the default XKB settings for xorg
#
#    Option	"XkbRules"	"xorg"
#    Option	"XkbModel"	"pc105"
#    Option	"XkbLayout"	"us"
#    Option	"XkbVariant"	""
#    Option	"XkbOptions"	""

EndSection


# **********************************************************************
# Core Pointer's InputDevice section
# **********************************************************************

Section "InputDevice"

# Identifier and driver

    Identifier	"Mouse1"
    Driver	"mouse"

# The mouse protocol and device.  The device is normally set to /dev/mouse,
# which is usually a symbolic link to the real device.

    Option	"Protocol"	"Microsoft"
    Option	"Device"	"/dev/mouse"

# On platforms where PnP mouse detection is supported the following
# protocol setting can be used when using a newer PnP mouse:

#    Option	"Protocol"	"Auto"

# When using mouse connected to a PS/2 port (aka "MousePort"), set the
# the protocol as follows.  On some platforms some other settings may
# be available.

#    Option "Protocol"	"PS/2"

# Baudrate and SampleRate are only for some older Logitech mice.  In
# almost every case these lines should be omitted.

#    Option	"BaudRate"	"9600"
#    Option	"SampleRate"	"150"

# Emulate3Buttons is an option for 2-button mice
# Emulate3Timeout is the timeout in milliseconds (default is 50ms)

#    Option	"Emulate3Buttons"
#    Option	"Emulate3Timeout"	"50"

# ChordMiddle is an option for some 3-button Logitech mice, or any
# 3-button mouse where the middle button generates left+right button
# events.

#    Option	"ChordMiddle"

EndSection

Section "InputDevice"
    Identifier	"Mouse2"
    Driver	"mouse"
    Option	"Protocol"	"MouseMan"
    Option	"Device"	"/dev/mouse2"
EndSection

# Some examples of extended input devices

# Section "InputDevice"
#    Identifier	"spaceball"
#    Driver	"magellan"
#    Option	"Device"	"/dev/cua0"
# EndSection
#
# Section "InputDevice"
#    Identifier	"spaceball2"
#    Driver	"spaceorb"
#    Option	"Device"	"/dev/cua0"
# EndSection
#
# Section "InputDevice"
#    Identifier	"touchscreen0"
#    Driver	"microtouch"
#    Option	"Device"	"/dev/ttyS0"
#    Option	"MinX"		"1412"
#    Option	"MaxX"		"15184"
#    Option	"MinY"		"15372"
#    Option	"MaxY"		"1230"
#    Option	"ScreenNumber"	"0"
#    Option	"ReportingMode"	"Scaled"
#    Option	"ButtonNumber"	"1"
#    Option	"SendCoreEvents"
# EndSection
#
# Section "InputDevice"
#    Identifier	"touchscreen1"
#    Driver	"elo2300"
#    Option	"Device"	"/dev/ttyS0"
#    Option	"MinX"		"231"
#    Option	"MaxX"		"3868"
#    Option	"MinY"		"3858"
#    Option	"MaxY"		"272"
#    Option	"ScreenNumber"	"0"
#    Option	"ReportingMode"	"Scaled"
#    Option	"ButtonThreshold"	"17"
#    Option	"ButtonNumber"	"1"
#    Option	"SendCoreEvents"
# EndSection

# **********************************************************************
# Monitor section
# **********************************************************************

# Any number of monitor sections may be present

Section "Monitor"

# The identifier line must be present.

    Identifier	"Generic Monitor"

# HorizSync is in kHz unless units are specified.
# HorizSync may be a comma separated list of discrete values, or a
# comma separated list of ranges of values.
# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR'S
# USER MANUAL FOR THE CORRECT NUMBERS.

#    HorizSync	31.5  # typical for a single frequency fixed-sync monitor
#    HorizSync	30-64         # multisync
#    HorizSync	31.5, 35.2    # multiple fixed sync frequencies
#    HorizSync	15-25, 30-50  # multiple ranges of sync frequencies

# VertRefresh is in Hz unless units are specified.
# VertRefresh may be a comma separated list of discrete values, or a
# comma separated list of ranges of values.
# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR'S
# USER MANUAL FOR THE CORRECT NUMBERS.

#    VertRefresh	60  # typical for a single frequency fixed-sync monitor

#    VertRefresh	50-100        # multisync
#    VertRefresh	60, 65        # multiple fixed sync frequencies
#    VertRefresh	40-50, 80-100 # multiple ranges of sync frequencies

# Modes can be specified in two formats.  A compact one-line format, or
# a multi-line format.

# A generic VGA 640x480 mode (hsync = 31.5kHz, refresh = 60Hz)
# These two are equivalent

#    ModeLine "640x480" 25.175 640 664 760 800 480 491 493 525

    Mode "640x480"
        DotClock	25.175
        HTimings	640 664 760 800
        VTimings	480 491 493 525
    EndMode

# These two are equivalent

#    ModeLine "1024x768i" 45 1024 1048 1208 1264 768 776 784 817 Interlace

#    Mode "1024x768i"
#        DotClock	45
#        HTimings	1024 1048 1208 1264
#        VTimings	768 776 784 817
#        Flags		"Interlace"
#    EndMode

# If a monitor has DPMS support, that can be indicated here.  This will
# enable DPMS when the monitor is used with drivers that support it.

#    Option	"dpms"

# If a monitor requires that the sync signals be superimposed on the
# green signal, the following option will enable this when used with
# drivers that support it.  Only a relatively small range of hardware
# (and drivers) actually support this.

#    Option	"sync on green"

EndSection

# **********************************************************************
# Graphics device section
# **********************************************************************

# Any number of graphics device sections may be present

Section "Device"

# The Identifier must be present.

    Identifier	"Generic VESA"

# The Driver line must be present.  When using run-time loadable driver
# modules, this line instructs the server to load the specified driver
# module.  Even when not using loadable driver modules, this line
# indicates which driver should interpret the information in this section.

    Driver	"vesa"

# The chipset line is optional in most cases.  It can be used to override
# the driver's chipset detection, and should not normally be specified.

#    Chipset	"generic"

# Various other lines can be specified to override the driver's automatic
# detection code.  In most cases they are not needed.

#    VideoRam	256
#    Clocks	25.2 28.3

# The BusID line is used to specify which of possibly multiple devices
# this section is intended for.  When this line isn't present, a device
# section can only match up with the primary video device.  For PCI
# devices a line like the following could be used.  This line should not
# normally be included unless there is more than one video device
# intalled.

#    BusID	"PCI:0:10:0"

# Various option lines can be added here as required.  Some options
# are more appropriate in Screen sections, Display subsections or even
# Monitor sections.

#    Option	"hw cursor" "off"

EndSection

Section "Device"
    Identifier	"any supported Trident chip"
    Driver	"trident"
EndSection

Section "Device"
    Identifier	"MGA Millennium I"
    Driver	"mga"
    Option	"hw cursor" "off"
    BusID	"PCI:0:10:0"
EndSection

Section "Device"
    Identifier	"MGA G200 AGP"
    Driver	"mga"
    BusID	"PCI:1:0:0"
    Option	"pci retry"
EndSection


# **********************************************************************
# Screen sections.
# **********************************************************************

# Any number of screen sections may be present.  Each describes
# the configuration of a single screen.  A single specific screen section
# may be specified from the X server command line with the "-screen"
# option.

Section "Screen"

# The Identifier, Device and Monitor lines must be present

    Identifier	"Screen 1"
    Device	"Generic VESA"
    Monitor	"Generic Monitor"

# The favoured Depth and/or Bpp may be specified here

    DefaultDepth 8

    SubSection "Display"
        Depth		8
        Modes		"640x480"
        ViewPort	0 0
        Virtual 	800 600
    EndSubsection

    SubSection "Display"
	Depth		4
        Modes		"640x480"
    EndSubSection

    SubSection "Display"
	Depth		1
        Modes		"640x480"
    EndSubSection

EndSection


Section "Screen"
    Identifier		"Screen MGA1"
    Device		"MGA Millennium I"
    Monitor		"Generic Monitor"
    Option		"no accel"
    DefaultDepth	16
#    DefaultDepth	24

    SubSection "Display"
	Depth		8
	Modes		"1280x1024"
	Option		"rgb bits" "8"
	Visual		"StaticColor"
    EndSubSection
    SubSection "Display"
	Depth		16
	Modes		"1280x1024"
    EndSubSection
    SubSection "Display"
	Depth		24
	Modes		"1280x1024"
    EndSubSection
EndSection


Section "Screen"
    Identifier		"Screen MGA2"
    Device		"MGA G200 AGP"
    Monitor		"Generic Monitor"
    DefaultDepth	8

    SubSection "Display"
	Depth		8
	Modes		"1280x1024"
	Option		"rgb bits" "8"
	Visual		"StaticColor"
    EndSubSection
EndSection


# **********************************************************************
# ServerLayout sections.
# **********************************************************************

# Any number of ServerLayout sections may be present.  Each describes
# the way multiple screens are organised.  A specific ServerLayout
# section may be specified from the X server command line with the
# "-layout" option.  In the absence of this, the first section is used.
# When now ServerLayout section is present, the first Screen section
# is used alone.

Section "ServerLayout"

# The Identifier line must be present

    Identifier	"Main Layout"

# Each Screen line specifies a Screen section name, and optionally
# the relative position of other screens.  The four names after
# primary screen name are the screens to the top, bottom, left and right
# of the primary screen.  In this example, screen 2 is located to the
# right of screen 1.

    Screen	"Screen MGA 1"	""	""	""	"Screen MGA 2"
    Screen	"Screen MGA 2"	""	""	"Screen MGA 1"	""

# Each InputDevice line specifies an InputDevice section name and
# optionally some options to specify the way the device is to be
# used.  Those options include "CorePointer", "CoreKeyboard" and
# "SendCoreEvents".  In this example, "Mouse1" is the core pointer,
# and "Mouse2" is an extended input device that also generates core
# pointer events (i.e., both mice will move the standard pointer).

    InputDevice	"Mouse1" "CorePointer"
    InputDevice	"Mouse2" "SendCoreEvents"
    InputDevice "Keyboard1" "CoreKeyboard"

EndSection


Section "ServerLayout"
    Identifier	"another layout"
    Screen	"Screen 1"
    Screen	"Screen MGA 1"
    InputDevice	"Mouse1" "CorePointer"
    InputDevice "Keyboard1" "CoreKeyboard"
EndSection


Section "ServerLayout"
    Identifier	"simple layout"
    Screen	"Screen 1"
    InputDevice	"Mouse1" "CorePointer"
    InputDevice "Keyboard1" "CoreKeyboard"
EndSection

