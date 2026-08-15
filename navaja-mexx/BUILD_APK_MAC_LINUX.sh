#!/bin/bash

# Build APK for Navaja Mexx - macOS/Linux Automated Script

echo ""
echo "========================================"
echo "  NAVAJA MEXX - APK BUILD AUTOMATION"
echo "========================================"
echo ""

# Check if we're in the right directory
if [ ! -d "android" ]; then
    echo "ERROR: android/ folder not found"
    echo "Make sure you're in the navaja-mexx/ directory"
    exit 1
fi

echo "[1/4] Checking Java..."
if ! command -v java &> /dev/null; then
    echo "ERROR: Java not found. Install JDK 11+"
    echo "macOS: brew install openjdk@11"
    echo "Linux: apt-get install openjdk-11-jdk"
    exit 1
fi
java_version=$(java -version 2>&1 | head -1)
echo "OK - $java_version"

echo ""
echo "[2/4] Checking Android SDK..."
if [ -z "$ANDROID_HOME" ]; then
    echo "WARNING: ANDROID_HOME not set"
    echo "You need Android SDK installed"
    echo "Download: https://developer.android.com/studio"
    exit 1
fi
echo "OK - Android SDK found at $ANDROID_HOME"

echo ""
echo "[3/4] Building APK..."
echo "Starting Gradle build..."
cd android
chmod +x gradlew
./gradlew assembleDebug

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed!"
    echo "Check the errors above"
    exit 1
fi
cd ..

echo ""
echo "[4/4] Verifying APK..."
if [ -f "android/app/build/outputs/apk/debug/app-debug.apk" ]; then
    echo ""
    echo "========================================"
    echo "  SUCCESS! APK COMPILED"
    echo "========================================"
    echo ""
    ls -lh android/app/build/outputs/apk/debug/app-debug.apk
    echo ""
    echo "Location: android/app/build/outputs/apk/debug/app-debug.apk"
    echo ""
    echo "Next steps:"
    echo "1. Connect Android phone via USB"
    echo "2. Enable USB Debugging in phone settings"
    echo "3. Run: adb install android/app/build/outputs/apk/debug/app-debug.apk"
    echo ""
else
    echo "ERROR: APK not found after build"
    exit 1
fi
