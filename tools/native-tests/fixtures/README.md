# Fixture provenance and licensing notes

These are small **recorded terminal output fixtures**, not packaged application
executables or source distributions of nano/htop. Every `.ansi` file and every
nano original/expected/saved UTF-8 file is copied byte-for-byte from the verified
capture. Manifests retain recorded version strings, dimensions, actions, offsets,
key inputs, stream hashes, cursor expectations, and save expectations. Prefixes
and deltas intentionally remain redundant so replay can cross-check integrity.

## Real PTY captures

All real captures used an 80-column × 24-row controlling PTY (`pty.openpty`,
`TIOCSWINSZ`, bounded reads), `TERM=xterm-256color`, `LANG=LC_ALL=C.UTF-8`. Merged
stdout/stderr ANSI bytes are unmodified. These are capture-time outputs: host
statistics/PIDs and redraw timings need not match a newly recorded run.

| Directory | Actual program | Capture provenance | Boundaries |
|---|---|---|---:|
| `nano-8.3` | GNU nano 8.3 (`--enable-utf8`) | Amazon Linux 2023 distro package `nano 8.3-1.amzn2023`, x86_64 | 13 |
| `nano-9.1` | GNU nano 9.1 (`--disable-libmagic --disable-nls --enable-utf8`) | Isolated local build of the official nano 9.1 release; no system installation replacement | 13 |
| `htop` | htop 3.2.1 | Amazon Linux 2023 distro package `htop 3.2.1-87.amzn2023.0.3`, x86_64 | 8 |

Nano invoked `--ignorercfiles --constantshow --nowrap nano-edit-fixture.txt`.
The edits move right twice, insert Arabic teh, forward-delete hah, then backspace
teh, followed by combining/mixed/indented cursor motions, write-out, save, and exit.
Each nano directory preserves all three original, expected-saved, and actual-saved
UTF-8 files. Save verification is logical byte order, never visual/reversed order.

Htop invoked `--readonly --delay=100 --pid=261` with `HTOPRC=/dev/null`, filtered to
one disposable sleeping Python process with deliberately synthetic Arabic/English
argv text. That process name is test data, not a credential or user command list.
The tree-toggle recording clips the command to its first Arabic meem at column 78;
the logical-content assertion intentionally allows that real terminal clipping.
Htop is not an editor and has no saved-file expectation.

### GNU nano 9.1 release identification

- Source: <https://www.nano-editor.org/dist/v9/nano-9.1.tar.xz>
- Detached signature: <https://www.nano-editor.org/dist/v9/nano-9.1.tar.xz.asc>
- Source archive SHA-256: `5f47764274cb7532349ce0aa20ec10f1e8e851a6e9fa3eb66812c43d196db042`.
- Recorded binary SHA-256: `84c41983b1653c9cc2d5e3d76941a45a63578883462923495f487615639ed7ce`.
- Capture preparation recorded a valid detached signature checked against the GNU
  keyring from <https://ftp.gnu.org/gnu/gnu-keyring.gpg>, signer fingerprint
  `168E6F4297BFD7A79AFD4496514BBE2EB8E1961F`. This records capture provenance; replay
  does not redownload/reverify the archive or distribute the binary/keyring.

## Explicitly synthetic Claude-style fixture

`synthetic-claude-style` contains **3 hand-authored ANSI boundaries** modeling an
ASCII frame/prompt, Arabic bullet, two-space indented continuations, redraw, and a
cursor within Arabic. It is not recorded output from Claude Code or Codex. **No
Claude/Codex process, API, account, authentication, or credential was used.** It
proves only the explicitly encoded pattern, not compatibility with an authenticated
service or arbitrary future CLI output. The manifest carries the same disclaimer.

## Metadata hygiene and byte integrity

Only manifest metadata was normalized: absolute executable paths became program
names; irrelevant installation/build-directory links and OS read-chunk timing
arrays were omitted; nano's unused HTOPRC setting was removed; file references
were made directory-local. All action objects, recorded ANSI bytes, cursor/save
expectations, and original/expected/saved contents are preserved unchanged.
Internal installation logs, host filesystem paths, source snapshots, credentials,
third-party jars, executables, caches, and generated screenshots are not bundled.
The nano 9.1 capture is an 80×24 replay, not a claim to reproduce a screenshot
pixel-for-pixel.

| Stream | SHA-256 |
|---|---|
| nano 8.3 | `bb0aabe8f16f11bce1811820ef695269774335ccbde8f6fb25249ce46f67ca68` |
| nano 9.1 | `20584cdf69809921cdff768fe92f5392964d035ce87081836486d8eb1ac8f017` |
| htop 3.2.1 | `873dfa65adf71094a0831f5b9159bf429332ef513ae4b814f43406d9209b5209` |
| synthetic Claude-style | `5755af992bbc6ce64401c6ced8fe6a072a363ca4dc330d1a3401a63163f558ba` |

## Licensing / attribution

- GNU nano is by the Free Software Foundation and contributors, licensed under
  **GPL-3.0-or-later**. Upstream: <https://www.nano-editor.org/>; license:
  <https://www.gnu.org/licenses/gpl-3.0.html>.
- htop is by its contributors, licensed under **GPL-2.0-or-later**. Upstream and
  licensing: <https://github.com/htop-dev/htop> and
  <https://github.com/htop-dev/htop/blob/3.2.1/COPYING>.
- Those program licenses concern the originating programs; these captures do not
  bundle their binaries, source code, fonts, or runtime dependencies. This notice
  does not relicense third-party material or imply upstream endorsement.
- The authored fixture text, synthetic ANSI pattern, and standalone harness are
  included under this repository's [licensing terms](../../../LICENSE.md).
  Existing renderer/emulator source retains its own license notices. No claim
  of ownership over nano/htop UI text or the Claude/Codex names is made.
