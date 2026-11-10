# SECURITY REVIEW — Termux Workstation Forge

**Branch:** `feature/termux-workstation`
**Date:** 2026-11-09
**Reviewer:** vibe-lead (Security Engineer)

---

## 1. Manifest Permissions Review

| Permission | Protection Level | Purpose | Concern | Recommendation |
|------------|-----------------|---------|---------|----------------|
| `INTERNET` | normal | Network access | Low | Keep |
| `ACCESS_NETWORK_STATE` | normal | Network state | Low | Keep |
| `READ_EXTERNAL_STORAGE` | dangerous | Read shared storage | Medium | Scoped storage preferred |
| `WRITE_EXTERNAL_STORAGE` | dangerous | Write shared storage | Medium | Scoped storage preferred |
| `MANAGE_EXTERNAL_STORAGE` | special | Broad file access | **High** | Deprecated on Android 10+; minimize |
| `WAKE_LOCK` | normal | Keep CPU awake | Low | Keep for long sessions |
| `VIBRATE` | normal | Haptic feedback | Low | Keep |
| `FOREGROUND_SERVICE` | normal | Background service | Low | Keep for terminal sessions |
| `REQUEST_IGNORE_BATTERY_OPTIMIZATIONS` | normal | Battery exemption | Medium | Justify need |
| `SYSTEM_ALERT_WINDOW` | special | Overlay windows | **High** | Only for Termux:Float; consider removing |
| `READ_LOGS` | dangerous | Read logcat | **High** | Remove unless essential |
| `DUMP` | dangerous | System dumps | **High** | Remove unless essential |
| `WRITE_SECURE_SETTINGS` | signature| System settings | **High** | Signature-level; review need |
| `REQUEST_INSTALL_PACKAGES` | normal | Install packages | Medium | For bootstrap; justify |
| `RECEIVE_BOOT_COMPLETED` | normal | Boot receiver | Low | For Termux:Boot |
| `PACKAGE_USAGE_STATS` | special | App usage | Medium | Remove unless essential |
| `SET_ALARM` | normal | Alarms | Low | Keep |
| `com.termux.permission.RUN_COMMAND` | dangerous | Custom permission | Medium | For plugin IPC |

**Overall:** 15 permissions declared, several high-concern (`MANAGE_EXTERNAL_STORAGE`, `SYSTEM_ALERT_WINDOW`, `READ_LOGS`, `DUMP`, `WRITE_SECURE_SETTINGS`). Minimize per directive: "Do not add privileged permissions without establishing a concrete need."

---

## 2. Exported Components Review

| Component | Exported | Permission | Concern |
|-----------|----------|------------|---------|
| `TermuxActivity` | true | none | Entry point; expected |
| `HomeActivity` (alias) | true | none | Boot launcher; expected |
| `SettingsActivity` | true | none | Settings entry; review if needed |
| `FileShareReceiverActivity` (alias) | true | none | File sharing; expected |
| `FileViewReceiverActivity` (alias) | true | none | File viewing; expected |
| `TermuxDocumentsProvider` | true | `MANAGE_DOCUMENTS` | Document provider; expected |
| `TermuxOpenReceiver$ContentProvider` | true | `RUN_COMMAND` | File provider; protected by custom permission |
| `RunCommandService` | true | `RUN_COMMAND` | Remote command execution; **high risk** |

**Concern:** `RunCommandService` is exported and protected only by the custom `RUN_COMMAND` permission. Any app holding that permission can execute commands. Review whether this should remain exported.

---

## 3. Custom Permission

- `com.termux.permission.RUN_COMMAND` — `dangerous` level
- Used by `TermuxOpenReceiver$ContentProvider` and `RunCommandService`
- Apps declaring `<uses-permission android:name="com.termux.permission.RUN_COMMAND">` can access these components
- **Recommendation:** Document which plugins legitimately need this; consider signature-level protection for stricter control

---

## 4. sharedUserId

- `android:sharedUserId="${TERMUX_PACKAGE_NAME}"` = `com.termux`
- Allows UID sharing with other Termux packages (Termux:API, Termux:Boot, etc.)
- **Implication:** All Termux packages must be signed with the same certificate
- **Implication:** Prevents independent distribution under a different applicationId
- **Recommendation:** Keep unchanged unless a complete fork strategy is approved

---

## 5. Backup and Data

- `android:allowBackup="false"` — **Good** (prevents backup-based data extraction)
- `android:requestLegacyExternalStorage="true"` — Legacy storage opt-out; deprecated
- No `android:dataExtractionRules` — consider adding for modern Android

---

## 6. Debug Signing

- Debug signing uses `testkey_untrusted.jks` included in repository
- Passwords in build.gradle (`storePassword`, `keyPassword`)
- **Critical:** This key must never be used for production
- **Recommendation:** Remove testkey from repo; production signing uses user-owned keystore outside repository

---

## 7. Native Code Security

- Compiler flags: `-std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector -Wl,--gc-sections`
- `-Werror` treats warnings as errors — good for catching issues
- `-fno-stack-protector` — disables stack buffer overflow protection; **consider enabling** for security
- `-Wl,--gc-sections` — removes unused sections; good for size
- JNI bridge between Java and C (`termux.c`) — review for buffer overflows, format string vulnerabilities

---

## 8. Plugin Compatibility Security

| Plugin | Integration Point | Security Consideration |
|--------|------------------|----------------------|
| Termux:API | `RUN_COMMAND` permission, content provider | Requires signature match |
| Termux:Boot | `RECEIVE_BOOT_COMPLETED` | Runs on boot; review |
| Termux:Styling | Theme resources | Low risk |
| Termux:Tasker | Intent-based | Review intent filtering |
| Termux:Widget | Widget provider | Low risk |
| Termux:Float | `SYSTEM_ALERT_WINDOW` | Overlay risk |

**Rule:** Never claim a plugin is compatible until tested. Never uninstall existing Termux or replace its data.

---

## 9. Root / Shizuku Integration (Planned)

- Root must **never** be assumed
- Features must degrade gracefully on non-rooted devices
- All privileged actions require explicit user consent
- Never silently execute root commands
- Never bypass Android security restrictions
- Shizuku as optional integration using documented API
- Keep privileged operations separated from ordinary terminal functionality

---

## 10. AI Integration Security

- No API keys stored in APK, source code, or project configuration
- Authentication via environment variables only
- No hardcoded gateway or provider
- Preserve environment-variable-based authentication (OmniRoute, OpenAI-style endpoints)
- Detect auth/config errors without printing secrets

---

## 11. Security Recommendations (Priority Order)

1. **Critical:** Remove `testkey_untrusted.jks` from repository; exclude debug signing keys from version control
2. **High:** Review `MANAGE_EXTERNAL_STORAGE` — migrate to scoped storage where possible
3. **High:** Review `READ_LOGS` and `DUMP` — remove unless essential for diagnostics
4. **High:** Review `RunCommandService` export — consider signature-level protection
5. **Medium:** Review `SYSTEM_ALERT_WINDOW` — limit to Termux:Float only
6. **Medium:** Add `android:dataExtractionRules` for modern Android backup control
7. **Medium:** Consider enabling `-fstack-protector` in native compiler flags
8. **Low:** Document all exported components and their legitimate consumers

---

## 12. Testing Requirements

- Test on Android 12+ (scoped storage, background restrictions)
- Test on target Android 17 (Galaxy S23 Ultra)
- Verify permission denial handling (graceful degradation)
- Verify exported component access control
- Verify no secrets in APK (strings analysis, secret scanning)
- Verify root/Shizuku features degrade gracefully on non-rooted devices

