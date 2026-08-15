@echo off
REM Build APK for Navaja Mexx - Windows Automated Script

echo.
echo ========================================
echo  NAVAJA MEXX - APK BUILD AUTOMATION
echo ========================================
echo.

REM Check if we're in the right directory
if not exist "android" (
    echo ERROR: android/ folder not found
    echo Make sure you're in the navaja-mexx/ directory
    pause
    exit /b 1
)

echo [1/4] Checking Java...
where java >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Java not found. Install JDK 11+
    echo Download: https://www.oracle.com/java/technologies/downloads/
    pause
    exit /b 1
)
echo OK - Java found

echo.
echo [2/4] Checking Android SDK...
if not defined ANDROID_HOME (
    echo WARNING: ANDROID_HOME not set
    echo You need Android SDK installed
    echo Download: https://developer.android.com/studio
    pause
    exit /b 1
)
echo OK - Android SDK found at %ANDROID_HOME%

echo.
echo [3/4] Building APK...
echo Starting Gradle build...
cd android
call gradlew.bat assembleDebug

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    echo Check the errors above
    pause
    exit /b 1
)
cd ..

echo.
echo [4/4] Verifying APK...
if exist "android\app\build\outputs\apk\debug\app-debug.apk" (
    echo.
    echo ========================================
    echo  SUCCESS! APK COMPILED
    echo ========================================
    echo.
    for %%A in ("android\app\build\outputs\apk\debug\app-debug.apk") do (
        echo File: %%~nxA
        echo Size: %%~zA bytes
    )
    echo.
    echo Location: android\app\build\outputs\apk\debug\app-debug.apk
    echo.
    echo Next steps:
    echo 1. Connect Android phone via USB
    echo 2. Enable USB Debugging in phone settings
    echo 3. Run: adb install android\app\build\outputs\apk\debug\app-debug.apk
    echo.
    pause
) else (
    echo ERROR: APK not found after build
    pause
    exit /b 1
)
