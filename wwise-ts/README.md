# Wwise BNK / WEM TypeScript engine

Dependency-free runtime TypeScript for inspecting Wwise `BKHD/DIDX/DATA` banks and `RIFF/WAVE` WEM metadata, extracting audio IDs, and rebuilding bank data with 16-byte alignment while retaining unknown sections.

**Not a Wwise Vorbis encoder.** Wwise's Custom Vorbis format is not a standard OGG. This package refuses mismatched codecs by default and verifies repacked audio bytes, but even matching codec tags do not prove that MLBB will accept modified files. Bank events, loops, Wwise version, and other metadata may differ.

Use `npm install && npm test` to build and run the tests. The Android app is currently Java, so this TypeScript module is standalone until an Android JavaScript runtime or a build-time bridge is introduced. No proprietary Wwise codec code is included.

For real encoding, use a suitable licensed Wwise authoring tool and validate resulting WEMs before attempting installation. Never overwrite a verified original backup.
