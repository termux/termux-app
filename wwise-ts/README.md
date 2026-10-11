# Wwise BNK / WEM TypeScript engine

Dependency-free runtime TypeScript for inspecting Wwise `BKHD/DIDX/DATA` banks and `RIFF/WAVE` WEM metadata, extracting audio IDs, and rebuilding bank data with 16-byte alignment while retaining unknown sections.

**Not a Wwise Vorbis encoder.** Wwise's Custom Vorbis format is not a standard OGG. This package refuses mismatched codecs by default and verifies repacked audio bytes, but even matching codec tags do not prove that MLBB will accept modified files. Bank events, loops, Wwise version, and other metadata may differ.

Use `npm install && npm test` to build and run the tests. The Android app is currently Java, so this TypeScript module is standalone until an Android JavaScript runtime or a build-time bridge is introduced. No proprietary Wwise codec code is included.

For real encoding, use a suitable licensed Wwise authoring tool and validate resulting WEMs before attempting installation. Never overwrite a verified original backup.

## Experimental TypeScript Vorbis encoder

`src/vorbis-encoder.ts` exports `parseOggVorbis(ogg)` and `oggVorbisToWem(ogg)`. This is an initial **legacy full-setup** Vorbis WEM writer. It accepts an already encoded Ogg Vorbis bitstream, parses its Ogg pages and packets, writes RIFF/WAVE `fmt ` / `vorb` / `data` chunks, and includes basic checks for sample rate, channels, block sizes, packet sizes and granule positions. It is **not** a PCM-to-Vorbis encoder.

Important: modern Wwise Custom Vorbis commonly uses stripped aoTuV 6.03 codebooks, mod_packets, and a different packet layout. **This implementation does not yet emit that modern layout, and should not be installed into MLBB.** The unit tests validate container fields with a synthetic stream only; they do not establish that vgmstream or the game can decode the result.

For a proven modern encoding route, evaluate `pas2k/wav2wem` (Go, GPL-2.0), which claims verified vgmstream/ww2ogg compatibility. Before deploying to Android, confirm native ARM64 encoding support and test against the actual game WEM version, looping metadata, and bank behavior.


## Android integration (experimental)

The debug APK CI compiles `src/vorbis-encoder.ts` with TypeScript and copies its JavaScript output into `app/src/main/assets/wwise-legacy-vorbis.js`. `MLBBOggWemBridge.java` runs that exact compiled JS in a non-network Android WebView, sends Ogg input as Base64, then retrieves encoded WEM in bounded chunks.

In the Audio Editor, open a BNK, select an embedded WEM, and choose **Experimental OGG → legacy Vorbis WEM (export only)**. Pick an Ogg Vorbis file; the resulting WEM can be exported for external vgmstream verification. It cannot yet be automatically installed by this path.

**Safety:** Legacy full-setup WEM is different from many MLBB modern Custom Vorbis formats. Passing TypeScript unit tests or RIFF parsing does not establish that the game will play the audio. This experimental option does not modify any game files. Android WebView needs BigInt support and currently limits input to 4 MiB and output to 8 MiB. Modern Wwise encoder integration remains a separate milestone.
