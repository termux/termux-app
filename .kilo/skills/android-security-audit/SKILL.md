---
name: android-security-audit
description: Android security auditing — permissions, exported components, storage isolation, credential protection
---

# Android Security Audit Skill

## Purpose
Audit and harden Android application security — permissions, exported components, storage isolation, command execution, credential protection.

## Activation Conditions
- Reviewing AndroidManifest.xml
- Adding/modifying permissions
- Auditing exported activities/services/providers/receivers
- Credential/secret handling
- Root/Shizuku integration review
- Backup and data protection

## Technical Reference

### Current Permissions (app/src/main/AndroidManifest.xml)
| Permission | Level | Concern |
|------------|-------|---------|
| `INTERNET` | normal | Low |
| `ACCESS_NETWORK_STATE` | normal | Low |
| `READ_EXTERNAL_STORAGE` | dangerous | Medium |
| `WRITE_EXTERNAL_STORAGE` | dangerous | Medium |
| `MANAGE_EXTERNAL_STORAGE` | special | **High** |
| `WAKE_LOCK` | normal | Low |
| `VIBRATE` | normal | Low |
| `FOREGROUND_SERVICE` | normal | Low |
| `REQUEST_IGNORE_BATTERY_OPTIMIZATIONS` | normal | Medium |
| `SYSTEM_ALERT_WINDOW` | special | **High** |
| `READ_LOGS` | dangerous | **High** |
| `DUMP` | dangerous | **High** |
| `WRITE_SECURE_SETTINGS` | signature | **High** |
| `REQUEST_INSTALL_PACKAGES` | normal | Medium |
| `RECEIVE_BOOT_COMPLETED` | normal | Low |
| `PACKAGE_USAGE_STATS` | special | Medium |
| `SET_ALARM` | normal | Low |
| `com.termux.permission.RUN_COMMAND` | dangerous | Medium |

### Exported Components
- `TermuxActivity` (MAIN/LAUNCHER) — expected
- `HomeActivity` alias (IOT_LAUNCHER) — expected
- `SettingsActivity` — review if needed
- `FileShareReceiverActivity` alias — expected
- `FileViewReceiverActivity` alias — expected
- `TermuxDocumentsProvider` (MANAGE_DOCUMENTS) — expected
- `TermuxOpenReceiver$ContentProvider` (RUN_COMMAND) — protected
- `RunCommandService` (RUN_COMMAND) — **high risk**

### Custom Permission
- `com.termux.permission.RUN_COMMAND` — dangerous, used by provider and service

### Security Rules
1. Minimize permissions — no privileged permissions without documented need
2. `sharedUserId=com.termux` — requires signature compatibility
3. No API keys, OAuth tokens, passwords, private keys in source or APK
4. Root/Shizuku: never assumed, always consent-gated, graceful degradation
5. Never silently execute root commands
6. Never bypass Android security restrictions
7. `allowBackup="false"` — good
8. Debug signing key (`testkey_untrusted.jks`) NOT for production

## Required Tools
- Android Studio manifest merger
- `aapt2 dump permissions`
- Secret scanning (truffleHog, git-secrets)
- OWASP Mobile Top 10 reference

## Reusable Workflow
1. Audit manifest permissions — remove unused, downgrade where possible
2. Review exported components — add permissions, restrict intent filters
3. Check custom permissions — consider signature-level
4. Verify no secrets in source/APK
5. Test permission denial handling (graceful degradation)
6. Verify root/Shizuku features degrade gracefully
7. Document all security decisions

## Verification Procedure
```bash
./gradlew lintDebug
# Manual: aapt2 dump badging app/build/outputs/apk/debug/*.apk
# Manual: secret scan on source
# Manual: test on Android 12+ (scoped storage, background restrictions)
```

## Common Failure Modes
- Over-privileged permissions (MANAGE_EXTERNAL_STORAGE, READ_LOGS, DUMP)
- Exported components without proper protection
- Secrets in source code or APK
- Root assumed without consent
- Backup enabled with sensitive data
- Debug key used in production
