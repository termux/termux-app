# Standalone Android release

Run `./gradlew :standalone:assembleRelease` to build the MLBB audio editor without Termux dependencies, bootstrap packages, or terminal runtime. Android ARM64 only.

The unsigned APK from the automatic GitHub Actions build cannot normally be installed. Publishing to Releases needs a stable signing key provided through GitHub Actions repository secrets: `MLBB_RELEASE_KEYSTORE_BASE64`, `MLBB_RELEASE_KEY_ALIAS`, `MLBB_RELEASE_KEYSTORE_PASSWORD`, `MLBB_RELEASE_KEY_PASSWORD`. Preserve the original release key offline for future APK upgrades. Once configured, manually dispatch `Standalone MLBB Editor release`, which signs and uploads an APK to the v0.1.0 prerelease.

The standalone module reuses only the eight MLBB Java classes, the dedicated AIDL broker and the VorbisLib JNI implementation. It intentionally does not include Termux terminal activities or download its bootstrap. This is an early build that requires in-device validation before a stable public release.
