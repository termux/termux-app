<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
		<string>English</string>
	<key>CFBundleExecutable</key>
		<string>X11</string>
	<key>CFBundleGetInfoString</key>
		<string>BUNDLE_ID_PREFIX.X11</string>
	<key>CFBundleIconFile</key>
		<string>X11.icns</string>
	<key>CFBundleIdentifier</key>
		<string>BUNDLE_ID_PREFIX.X11</string>
	<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
	<key>CFBundleName</key>
		<string>APPLE_APPLICATION_NAME</string>
	<key>CFBundlePackageType</key>
		<string>APPL</string>
	<key>CFBundleShortVersionString</key>
		<string>BUNDLE_VERSION_STRING</string>
	<key>CFBundleVersion</key>
		<string>BUNDLE_VERSION</string>
	<key>CFBundleSignature</key>
		<string>x11a</string>
	<key>CSResourcesFileMapped</key>
		<true/>
	<key>NSSupportsAutomaticGraphicsSwitching</key>
		<true/>

	<!-- Add generic usage descriptions for TCC
	     https://github.com/XQuartz/XQuartz/issues/245
	  -->
	<key>NSAppleEventsUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants to use AppleScript.</string>
	<key>NSCalendarsUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your calendar data.</string>
	<key>NSCameraUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your camera.</string>
	<key>NSContactsUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your contacts.</string>
	<key>NSLocationAlwaysUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your location information, even when not in use.</string>
	<key>NSLocationUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your location information.</string>
	<key>NSLocationWhenInUseUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your location information when in use.</string>
	<key>NSMicrophoneUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your microphone.</string>
	<key>NSRemindersUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants access to your reminders.</string>
	<key>NSSystemAdministrationUsageDescription</key>
	<string>A process launched by APPLE_APPLICATION_NAME wants admin privileges.</string>

#ifdef XQUARTZ_SPARKLE
	<key>SUEnableAutomaticChecks</key>
		<true/>
	<key>SUPublicEDKey</key>
		<string>XQUARTZ_SPARKLE_PUBLIC_EDKEY</string>
        <key>SUFeedURL</key>
                <string>XQUARTZ_SPARKLE_FEED_URL</string>
#endif
	<key>LSApplicationCategoryType</key>
		<string>public.app-category.utilities</string>
	<key>NSHumanReadableCopyright</key>
		<string>© 2003-2023 Apple Inc.
© 2003 XFree86 Project, Inc.
© 2003-2023 X.org Foundation, Inc.
</string>
	<key>NSMainNibFile</key>
		<string>main</string>
	<key>NSPrincipalClass</key>
		<string>X11Application</string>
</dict>
</plist>
