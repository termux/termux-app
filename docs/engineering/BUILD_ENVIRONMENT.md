# BUILD ENVIRONMENT — Termux Workstation Forge

**Branch:** `feature/termux-workstation`
**Date:** 2026-11-09
**Host:** Android Termux on aarch64 (Samsung Galaxy S23 Ultra SM-S918B)

---

## 1. Host System

| Field | Value |
|-------|-------|
| OS | Linux (Android kernel 5.15.197-android13) |
| Architecture | aarch64 (ARM64) |
| CPU | 8 cores |
| Memory | 10 GiB total, ~3.8 GiB available |
| Disk | 118 GiB available on /data |
| User | u0_a527 (uid 10527) |
| Working directory | `/data/data/com.termux/files/home/PROJECTS/GITHUB/termux-app` |

---

## 2. Toolchain

| Component | Version | Path | Status |
|-----------|---------|------|--------|
| Kilo CLI | 7.8.27 | `/data/data/com.termux/files/usr/bin/kilo` | Functional |
| Java (JDK) | 21.0.13 | `/data/data/com.termux/files/usr/lib/jvm/java-21-openjdk` | Functional (substitute for JDK 17) |
| Java (JDK 17) | — | `/data/data/com.termux/files/usr/lib/jvm/java-17-openjdk` | **Incomplete** — `lib/` only, no `bin/java` |
| Gradle Wrapper | 9.2.1 | `./gradlew` | Functional |
| AGP | 8.13.2 | `build.gradle` buildscript | Functional |
| Android SDK | — | `/data/data/com.termux/files/usr/opt/android-sdk` | Functional |
| ANDROID_HOME | — | `/data/data/com.termux/files/usr/opt/android-sdk` | Set |
| ANDROID_SDK_ROOT | — | `/data/data/com.termux/files/usr/opt/android-sdk` | Set |
| Platform android-36 | — | `platforms/android-36/` | Installed |
| Platform android-37 | — | `platforms/android-37/` | Installed |
| Build tools 35.0.0 | — | `build-tools/35.0.0/` | Installed (aapt2 is x86_64) |
| Build tools 36.0.0 | — | `build-tools/36.0.0/` | Installed |
| Build tools 37.0.0 | — | `build-tools/37.0.0/` | Installed |
| NDK 29.0.14206865 | — | `ndk/29.0.14206865/` | **Replaced** with aarch64 host build |
| NDK 28.2.13676358 | — | `ndk/28.2.13676358/` | Present (x86_64 host) |
| adb | — | `/data/data/com.termux/files/usr/bin/adb` | Present |
| sdkmanager | — | `cmdline-tools/latest/bin/sdkmanager` | Present |

---

## 3. Environment Modifications (Documented)

### 3.1 NDK Replacement (Required for aarch64 Host)

The SDK's NDK 29.0.14206865 provided only `linux-x86_64` host binaries, which cannot execute on this aarch64 Termux host ("exec format error"). The NDK directory was replaced with an aarch64 build from `lzhiyong/termux-ndk` (android-ndk-r29-aarch64.tar.xz, NDK 29.0.14206865, built with Zig/musl).

**Action taken:**
- Removed `/data/data/com.termux/files/usr/opt/android-sdk/ndk/29.0.14206865/` (x86_64)
- Copied aarch64 NDK r29 to the same path
- Fixed `build/ndk-build` shebang from `#!/usr/bin/env bash` to `#!/data/data/com.termux/files/usr/glibc/bin/bash` (since `/usr/bin/env` does not exist on this Termux system)

**Verification:** `ndk-build --version` returns "GNU Make 4.4.1, Built for aarch64-unknown-linux-musl"

### 3.2 JDK Substitution

gradle.properties and AGP 8.13.2 target JDK 17, but only JDK 21 is functional on this host (JDK 17 installation is incomplete — `lib/` directory only, no `bin/java`). Build executed with `JAVA_HOME` pointing to JDK 21.

**Warning:** Java compiler version 21 deprecates source/target version 8. This is a warning, not an error. Consider setting `android.javaCompile.suppressSourceTargetDeprecationWarning=true` in gradle.properties or upgrading sourceCompatibility.

### 3.3 SDK Components Installed via sdkmanager

| Component | Reason |
|-----------|--------|
| `ndk;29.0.14206865` | Required by gradle.properties `ndkVersion` |
| `platforms;android-36` | Required by `compileSdkVersion=36` |
| `build-tools;35.0.0` | Default build tools version for AGP 8.13.2 |

---

## 4. Build Command

```bash
export JAVA_HOME=/data/data/com.termux/files/usr/lib/jvm/java-21-openjdk
export ANDROID_HOME=/data/data/com.termux/files/usr/opt/android-sdk
export ANDROID_SDK_ROOT=/data/data/com.termux/files/usr/opt/android-sdk
TERMUX_PACKAGE_VARIANT=apt-android-7 TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1 ./gradlew :app:assembleDebug --no-daemon
```

---

## 5. Build Result

| Metric | Value |
|--------|-------|
| Status | **BUILD SUCCESSFUL** |
| Time | 1m 52s |
| Tasks | 118 actionable (78 executed, 40 up-to-date) |
| ARM64 APK | `app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk` |
| APK size | 38,461,311 bytes (~38.5 MB) |
| APK SHA-256 | `75cecdda861e44adfb9c35e55454286c94cd25d54eccc2d58c900bc483af6104` |
| Other ABIs | armeabi-v7a, x86, x86_64, universal |

---

## 6. Known Issues

| Issue | Impact | Workaround |
|-------|--------|------------|
| JDK 17 incomplete on host | Build uses JDK 21 instead | Set `JAVA_HOME` to JDK 21; document deviation |
| aapt2 is x86_64 | May fail on aarch64 host for resource processing | Build succeeded; aapt2 may be bundled by AGP from Maven |
| SDK package ID inconsistencies | Cosmetic warnings from sdkmanager | Does not block build |
| No aarch64 NDK from Google directly | Required third-party NDK from `lzhiyong/termux-ndk` | Documented; verify license compatibility |

---

## 7. Recommendations for WSL2/Linux Build

Per the MASTER DIRECTIVE, prefer Linux filesystem storage in WSL2 for the source tree and Gradle build cache. If continuing on this aarch64 Termux host:

1. Keep the aarch64 NDK replacement in place
2. Consider obtaining an aarch64 aapt2 from `lzhiyong/sdk-tools` if resource processing fails
3. Monitor JDK 21 deprecation warnings for source/target 8
4. For production builds, use an x86_64 Linux host with official NDK

