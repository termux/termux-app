# UstadTermux

[![GitHub Actions](https://github.com/erdalgunes/ustad-termux/workflows/Build/badge.svg)](https://github.com/erdalgunes/ustad-termux/actions)

**UstadTermux** is a custom Android terminal emulator forked from [Termux](https://github.com/termux/termux-app), enhanced with integrated Kotlin Multiplatform (KMP) terminal capabilities and Jake Wharton's Mosaic UI framework.

## 🎯 Features

- **Built-in KMP Terminal**: Native integration of our Kotlin Multiplatform terminal with Mosaic UI
- **SOLID Architecture**: Clean separation of concerns with extensible plugin system  
- **Custom Package Repository**: Dedicated package ecosystem for Ustad tools
- **Interactive Commands**: Slash commands like `/help`, `/clear`, `/model` built-in
- **Process Execution**: Real-time streaming I/O with ANSI escape sequence support
- **Modern UI**: Powered by Jake Wharton's Mosaic terminal UI framework

## 📱 Installation

### Requirements
- Android 7.0 (API 24) or higher
- ARM64 or ARMv7 device (Pixel 7 compatible)

### Download
- [Latest Release](https://github.com/erdalgunes/ustad-termux/releases)

## 🏗️ Architecture

UstadTermux follows SOLID principles with these core modules:

```
dev.ustad.termux/
├── core/           # Terminal core functionality
├── ui/             # Mosaic UI integration
├── packages/       # Package management
└── plugins/        # Extensible plugin system
```

## 🚀 Built-in Tools

### KMP Terminal with Mosaic UI
```bash
# Launch integrated KMP terminal
ustad-terminal

# Available commands
/help       # Show available commands
/clear      # Clear terminal
/pwd        # Show working directory
/model      # Set AI model
/shell      # Execute shell commands
```

### Package Management
```bash
# Install from UstadTermux repository
pkg install ustad-kmp-tools
pkg install mosaic-ui-components
```

## 🔧 Building from Source

```bash
# Clone repository
git clone https://github.com/erdalgunes/ustad-termux.git
cd ustad-termux

# Build APK
./gradlew assembleDebug

# Install on device
adb install app/build/outputs/apk/debug/ustad-termux_*.apk
```

## 📦 Custom Package Repository

UstadTermux includes a custom package repository at:
- `https://packages.ustad.dev/termux`

### Adding Custom Packages
See [ustad-termux-packages](https://github.com/erdalgunes/ustad-termux-packages) for package building instructions.

## 🤝 Contributing

Contributions welcome! Please follow SOLID, DRY, KISS, and YAGNI principles.

## 📄 License

UstadTermux is licensed under [GPLv3](LICENSE.md), inheriting from the original Termux project.

## 🙏 Acknowledgments

- [Termux](https://github.com/termux/termux-app) - Original terminal emulator
- [Jake Wharton](https://github.com/JakeWharton/mosaic) - Mosaic UI framework  
- [Kotlin Multiplatform](https://kotlinlang.org/multiplatform/) - Cross-platform development

---

**UstadTermux** - Terminal redefined with KMP and Mosaic 🚀