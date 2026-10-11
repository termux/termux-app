# MLBB Resource Editor (Termux fork) - implementation plan

## Identity
- User-facing app name: **MLBB Resource Editor**
- Final Android application ID: **com.farizgd.mlbbeditor**
- Primary target: arm64-v8a
- Backend: Termux runtime, Python BNK parser, vgmstream, FFmpeg, wwav
- Privileged file operations: Shizuku UserService, explicitly user-authorized
- Resource tabs: Audio; Models, Emotes, Images, Other Resources (Coming Soon); Developer Terminal

## Critical bootstrap restriction
**Do not change applicationId blindly.** The existing Termux bootstrap and packaged binaries refer to /data/data/com.termux/files/usr. A separately signed package with com.farizgd.mlbbeditor requires a matching rebuild of the bootstrap and package repository or an independently validated prefix relocation mechanism. Do not ship a renamed package with Termux's original bootstrap.

## Implementation milestones
1. GUI dashboard that opens a functioning Termux developer terminal; branded application name.
2. Shizuku permission lifecycle and secure UserService file operations for /storage/emulated/0/Android/data/com.mobile.legends/files/dragon2017/assets/Audio/android.
3. BNK listing/search and support for DIDX/DATA banks; multiple WEMs per BNK; guard unusual bank layouts.
4. Preview: use vgmstream custom Vorbis decoder to produce temporary WAV, play and seek. Do not confuse the Linux/Termux vgmstream-cli binary with a standard Android JNI library.
5. Replacement: import WEM and supported WAV/MP3, perform explicit WAV/WEM conversion. wwav supports PCM/ADPCM but not Wwise Custom Vorbis encoding; show compatibility warning. Preserve extra BNK sections and audio alignment.
6. Backup and restoration: create backups transactionally, never overwrite the original backup, verify installed file SHA-256, retain a copy outside the game folder.
7. Optional first-run installer with progress and disk-space checks for Python, FFmpeg, vgmstream, wwav. Installation is blocked until a correct package-prefix bootstrap is available.
8. Rebuild custom bootstrap for com.farizgd.mlbbeditor, sign artifact, run device tests. Never require users to uninstall their existing com.termux.
9. Models, Emotes, Images and Other Resources remain Coming Soon until their formats are validated.

## Security and recovery
- Whitelist game-bank names, don't allow arbitrary shell command interpolation from filenames.
- Require explicit confirmation before overwriting game resources.
- Repack to a separate file and validate DIDX offsets, structure, and sample extraction before install.
- Stop if Shizuku is not granted or source/destination hashes cannot be verified.
- On cancellation, errors or crash, keep the backup and never claim the game accepted changed codec files.

## Build policy
Android debug builds are experimental. A successfully built APK is not proof the custom bootstrap, Wwise decoding or in-game playback works.
