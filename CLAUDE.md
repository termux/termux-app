# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Termux is an Android terminal emulator and Linux environment application. This repository contains the main Termux app (user interface and terminal emulation), not the installable packages (see termux/termux-packages).

**Important:** Termux may be unstable on Android 12+ due to phantom process killing. The app targets Android 7+ (minSdk 21, targetSdk 28).

## Architecture

The project is organized into four Gradle modules:

### Core Modules

1. **app** (`app/`) - Main Android application
   - Main components:
     - `TermuxActivity` - Primary UI activity with drawer navigation
     - `TermuxService` - Background service managing terminal sessions
     - `RunCommandService` - Handles RUN_COMMAND intents from other apps/plugins
     - `TermuxInstaller` - Manages bootstrap installation
   - Key packages:
     - `com.termux.app.activities` - Activity components
     - `com.termux.app.fragments` - Fragment components (settings, terminal)
     - `com.termux.app.terminal` - Terminal session management
     - `com.termux.app.api` - API for file operations and external interactions

2. **termux-shared** (`termux-shared/`) - Shared library for app and plugins
   - **Critical:** All Termux constants, paths, and shared utilities MUST be defined here
   - `TermuxConstants` - Central constants definition (paths, intents, package names)
   - `TermuxUtils` - Common utility functions
   - `TermuxBootstrap` - Bootstrap variant management
   - Never use hardcoded paths - always reference from this module
   - See `TermuxConstants` javadocs for forking/package name changes

3. **terminal-emulator** (`terminal-emulator/`) - Core terminal emulation
   - `TerminalEmulator` - VT100/xterm terminal protocol implementation
   - `TerminalSession` - Manages individual terminal sessions
   - `TerminalBuffer` - Screen buffer management
   - Independent of Android UI layer

4. **terminal-view** (`terminal-view/`) - Android UI for terminal
   - `TerminalView` - Custom view rendering terminal content
   - `TerminalRenderer` - Handles drawing terminal text and graphics
   - Bridges terminal-emulator with Android UI

### Key Architectural Principles

- **Separation of concerns:** Terminal emulation logic is separate from Android UI
- **Shared constants:** All paths and constants in `termux-shared` to support plugins
- **Plugin architecture:** Termux has optional plugin apps (API, Boot, Float, Styling, Tasker, Widget) that share the `com.termux` sharedUserId

## Building

### Basic Commands

```bash
# Build debug APKs (all architectures + universal)
./gradlew assembleDebug

# Build for specific package variant (apt-android-7 or apt-android-5)
TERMUX_PACKAGE_VARIANT=apt-android-7 ./gradlew assembleDebug

# Build release APKs
./gradlew assembleRelease

# Clean build artifacts
./gradlew clean
```

### Build Outputs

APKs are generated in `app/build/outputs/apk/debug/` or `app/build/outputs/apk/release/`

Architecture-specific APKs (~120MB each):
- `*_arm64-v8a.apk` - 64-bit ARM
- `*_armeabi-v7a.apk` - 32-bit ARM
- `*_x86_64.apk` - 64-bit x86
- `*_x86.apk` - 32-bit x86
- `*_universal.apk` - All architectures (~180MB)

### Package Variants

Two bootstrap variants are supported (set via `TERMUX_PACKAGE_VARIANT` env var):
- `apt-android-7` (default) - For Android 7+
- `apt-android-5` - For Android 5-6 (app only, no package updates)

## Testing

```bash
# Run all unit tests
./gradlew test

# Run tests for specific module
./gradlew :app:test
./gradlew :termux-shared:test
```

Test files are located in `*/src/test/` directories.

## Development Workflow

### Commit Message Requirements

**MUST** follow [Conventional Commits](https://www.conventionalcommits.org) spec for automatic changelog generation:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

**Rules:**
- First letter of `type` and `description` MUST be capitalized
- Description in present tense
- Include space after colon `:`
- For breaking changes: add `!` before colon (e.g., `Changed!: Breaking change`)

**Allowed types** (use exactly as shown):
- `Added` - New features
- `Changed` - Changes in existing functionality
- `Deprecated` - Soon-to-be removed features
- `Removed` - Removed features
- `Fixed` - Bug fixes
- `Security` - Vulnerability fixes

Examples:
```
Added: Add swipe keyboard support
Fixed(terminal): Fix cursor positioning bug
Changed!: Change API to support new intent format
Added|Fixed: Add feature X and fix bug Y
```

### Versioning

Version names in `build.gradle` files MUST follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html):

Format: `major.minor.patch(-prerelease)(+buildmetadata)`

Always include patch number (e.g., `v0.118.0`, not `v0.118`).

### Code Guidelines

1. **Shared constants:** Define in `termux-shared` library, never hardcode
2. **Pull requests with hardcoded values will be rejected**
3. **Termux-specific classes:** Place under `com.termux.shared.termux` package
4. **General classes:** Place outside termux-specific packages
5. **Licensing:** Update `termux-shared/LICENSE.md` when contributing code

## Key Files and Locations

### Configuration
- `gradle.properties` - Build configuration (SDK versions, NDK version)
- `app/build.gradle` - Main app build configuration
- `settings.gradle` - Module inclusion

### Important Constants
- `termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java` - All Termux constants
- Review this file for forking instructions and package name changes

### Build Properties
- `compileSdkVersion`: 30
- `targetSdkVersion`: 28
- `minSdkVersion`: 21 (Android 5.0)
- `ndkVersion`: 22.1.7171670

## Plugin Development

Termux plugins (Termux:API, Termux:Boot, etc.) do NOT execute commands themselves - they send execution intents to the main Termux app.

For plugin development:
- Import termux-shared library for constants
- See [Termux Libraries wiki](https://github.com/termux/termux-app/wiki/Termux-Libraries)
- All plugin apps must be signed with same signature key as main app

## Debugging

Set log level in Termux app settings:
- Settings → App → Debugging → Log Level

Log levels: Off, Normal, Debug, Verbose

View logs:
```bash
# Real-time logs (Ctrl+C to stop)
logcat

# Dump to file
logcat -d > logcat.txt

# Or use "More → Report Issue" from terminal long-press menu
```

For plugin debugging: Set log level for BOTH Termux app and plugin app.

## CI/CD

GitHub Actions workflows:
- `.github/workflows/debug_build.yml` - Build debug APKs on push/PR
- `.github/workflows/run_tests.yml` - Run unit tests
- `.github/workflows/attach_debug_apks_to_release.yml` - Attach APKs to releases

## Installation & Distribution

**Important:** APKs from different sources have different signatures and cannot be mixed:
- F-Droid releases (official, universal APK only)
- GitHub releases (test key, includes architecture-specific APKs)
- GitHub Actions artifacts (requires GitHub login, test key)
- Google Play (experimental, Android 11+ only)

Users must uninstall ALL Termux apps/plugins before switching sources.

## External Resources

- [Termux Packages Wiki](https://github.com/termux/termux-packages/wiki) - Package building
- [Building bootstrap](https://github.com/termux/termux-packages/wiki/For-maintainers#build-bootstrap-archives) - Required when changing package name
- [RUN_COMMAND Intent](https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent) - External app integration
- [Termux Libraries](https://github.com/termux/termux-app/wiki/Termux-Libraries) - Plugin development
