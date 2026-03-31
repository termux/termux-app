# OpenClaw Assistant (Android)

OpenClaw Assistant is a powerful Android application that brings the OpenClaw AI assistant and Ollama models directly to your mobile device. Built upon the robust foundation of Termux, it provides a comprehensive Linux environment for running local AI services with a dedicated management dashboard.

## Features

- **Integrated AI Dashboard**: Easily start and stop OpenClaw and Ollama services.
- **OpenClaw Gateway Support**: Run your personal AI assistant locally.
- **Ollama Integration**: Support for high-performance local models like `phi4-mini`, `llama3.2`, and `mistral`.
- **Embedded Web Interface**: Interact with the OpenClaw UI directly within the app via a native WebView.
- **Terminal Access**: Quick shortcuts to jump into background AI sessions for manual control and logging.
- **Linux Environment**: Full Termux capabilities for advanced users.

## Quick Start Guide

### Prerequisites

- An Android device (version 7.0 or higher).
- Internet connection for initial setup and model downloading.

### Installation

1.  **Install OpenClaw Assistant**: Install the APK from the [Releases](https://github.com/openclaw/openclaw/releases) section.
2.  **Open AI Dashboard**: Open the navigation drawer (swipe from left) and select **AI Dashboard**.
3.  **Start OpenClaw**:
    - Tap the **Start OpenClaw** button.
    - The app will automatically ensure `openclaw` is installed and then launch the gateway.
    - Once started, the **Web Interface** section will load the local dashboard (localhost:18789).
4.  **Run Ollama Models**:
    - Select your desired model from the dropdown menu (e.g., `phi4-mini`).
    - Tap **Run Model**.
    - Use the **Terminal** button next to it to see the progress and interact with the CLI.

### Manual Commands

You can also interact with these services manually in the terminal:

```bash
# Start OpenClaw Gateway
openclaw gateway

# Run a model with Ollama
ollama run phi4-mini
```

## For Developers

### Building from Source

To build the project yourself, you need the Android SDK and Java 17+.

```bash
# Clone the repository
git clone https://github.com/openclaw/openclaw.git

# Build the debug APK
./gradlew assembleDebug
```

### Contributing

Contributions are welcome! Please follow the standard GitHub workflow: fork, branch, and submit a PR.

## Working Mechanism

The application operates as a self-contained AI node on Android:
1.  **Terminal Engine**: Uses a high-performance terminal emulator based on the Termux core to execute shell commands and manage Linux packages.
2.  **Service Management**: Background services (Gateway, Ollama) run in dedicated terminal sessions, ensuring they persist even when the dashboard is closed.
3.  **Local Intelligence**: By utilizing Ollama, models run directly on your device hardware, ensuring privacy and offline capability.
4.  **Integrated UI**: The dashboard communicates with the background sessions to monitor status and provides a WebView bridge to the local OpenClaw gateway UI.

---
*OpenClaw Assistant is a fork of the Termux application.*
