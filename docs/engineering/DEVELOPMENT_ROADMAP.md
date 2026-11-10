# DEVELOPMENT ROADMAP — Termux Workstation Forge

**Branch:** `feature/termux-workstation`
**Date:** 2026-11-09
**Milestones:** A through I (per MASTER DIRECTIVE)

---

## Milestone A — Baseline (COMPLETED)

| Goal | Status |
|------|--------|
| Inspect repository and git state | Done |
| Detect host OS and dev environment | Done (Android Termux aarch64) |
| Verify Kilo CLI version | Done (7.8.27) |
| Establish reproducible baseline build | Done |
| Create engineering audit | In progress |
| Configure Kilo project agent ecosystem | Pending |
| Produce first verified ARM64 debug APK | Done |

**Result:** `app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk`
**SHA-256:** `75cecdda861e44adfb9c35e55454286c94cd25d54eccc2d58c900bc483af6104`
**Build time:** 1m 52s

---

## Milestone B — Agent Ecosystem

| Task | Description | Status |
|------|-------------|--------|
| Create AGENTS.md | Project architecture, conventions, build, testing, git, review | Pending |
| Create .kilo/agent/ | 8 project agents (architect, android, terminal, ui-ux, ai, security, qa, build) | Pending |
| Create .kilo/skills/ | 8 skills (architecture, native, terminal, ui, security, build, ai-cli, device-testing) | Pending |
| Verify discovery | `kilo agent` and `kilo skill` discovery | Pending |

---

## Milestone C — Product Architecture

| Task | Description | Status |
|------|-------------|--------|
| Feature backlog | Prioritized list of workstation features | Pending |
| UI/UX direction | Material 3, dark/light themes, 7 navigation destinations | Pending |
| Architecture decisions | Documented in ARCHITECTURE.md | Done |

---

## Milestone D — First Implementation

| Feature | Risk | Value | Status |
|---------|------|-------|--------|
| Session naming and colors | Low | High | Pending |
| Workspace project directories | Low | High | Pending |
| AI tool detection | Low | High | Pending |
| Device diagnostics | Low | Medium | Pending |
| Toolchain detection | Low | Medium | Pending |
| Split-pane terminal | High | Medium | Pending (investigate first) |

---

## Milestone E — Integration

| Task | Description | Status |
|------|-------------|--------|
| Integrate new features without breaking terminal | Regression testing | Pending |
| Preserve session management | Existing tests must pass | Pending |
| Plugin compatibility | Termux:API, Styling, etc. | Pending |

---

## Milestone F — Quality

| Task | Description | Status |
|------|-------------|--------|
| Unit tests | terminal-emulator tests + new tests | Pending |
| Lint | `lintDebug` | Pending |
| Native compilation | NDK build for all ABIs | Pending |
| Security review | Manifest, permissions, exported components | Pending |
| Test report | Actual results, not invented | Pending |

---

## Milestone G — APK

| Task | Description | Status |
|------|-------------|--------|
| Debug APK build | ARM64 split APK | Done (Milestone A) |
| APK inspection | Signature, ABI, size | Pending |
| SHA-256 checksum | Documented | Done (Milestone A) |

---

## Milestone H — Device Testing

| Task | Description | Status |
|------|-------------|--------|
| ADB device check | `adb devices -l` | Pending (device not connected) |
| APK installation | Galaxy S23 Ultra, ARM64, Android 17 | Pending |
| First launch | Bootstrap init, shell startup | Pending |
| Package/signature compatibility | Verify before install | Pending |
| Defect log | Track and fix actual defects | Pending |

---

## Milestone I — Release Readiness

| Task | Description | Status |
|------|-------------|--------|
| Documentation | README, CHANGELOG, build, install, release | Pending |
| Engineering report | Complete audit + implementation summary | Pending |
| Remaining issues | List of future improvements | Pending |
| CI/CD | GitHub Actions workflow improvements | Pending |

---

## Current Blocker

**Real-device testing:** Galaxy S23 Ultra not connected via ADB. `adb devices -l` returns no devices. Device testing (Milestone H) is blocked until a device is connected.

**Environment note:** Native compilation on aarch64 Termux required replacing the SDK's x86_64 NDK 29 with an aarch64 build from `lzhiyong/termux-ndk`. This is documented in BUILD_ENVIRONMENT.md.
